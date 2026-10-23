#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 Subscriber와 Editor/post의 PE 초기값·직접 참조·쓰기 명령 구간을 먼저 고정한다."""
import argparse
import bisect
import csv
import json
import re
import socket
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_REG, X86_OP_IMM
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn import x86_const

# 같은 주소를 읽는 서로 다른 소비자를 분류하며 이16BYTE는 실제 인접 배열이 아닌 네 주소의 이어붙인 관측이다.
GLOBALS=(0x540c74,0x54265c,0x5c85a4,0x594fa4)
NAMES=('mySubscriber','loginSubscriber','editor','missionPost')
FIXTURE=ROOT/'cpppj/tests/fixtures/subscriber-lifetime-edges-x86.tsv'
REPORT=ROOT/'cpppj/subscriber-lifetime-edges-native.json'
AUDIT=ROOT/'extracted/cpp-client-subscriber-globals-20261024-static.json'


def static_audit(oracle):
    """실제 PE 초기 DWORD와 모든 직접 메모리/주소 상수 참조를 정밀 C와 함께 분류한다."""
    import pefile
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
    section=next(part for part in pe.sections if part.Name.rstrip(b'\0')==b'.text')
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    decoder.detail=True
    instructions=list(decoder.disasm(section.get_data(),0x400000+section.VirtualAddress))
    facts=list(csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t'))
    entries=sorted((int(row['entry'],16),row['name']) for row in facts)
    starts=[entry[0] for entry in entries]
    references=[]
    writes=[]
    for index,instruction in enumerate(instructions):  # 명시 주소 참조와 간접 포인터 분석의 범위를 구별한다.
        for number,operand in enumerate(instruction.operands):
            address=operand.mem.disp if operand.type==X86_OP_MEM and not operand.mem.base and not operand.mem.index else operand.imm if operand.type==X86_OP_IMM else None
            if address not in GLOBALS:
                continue
            direct_write=operand.type==X86_OP_MEM and number==0 and instruction.mnemonic in ('mov','and','or','xor','add','sub','inc','dec','xchg')
            function=entries[max(0,bisect.bisect_right(starts,instruction.address)-1)]
            reference={'address':f'{instruction.address:08x}','global':f'{address:08x}','name':NAMES[GLOBALS.index(address)],'function':f'{function[0]:08x}','functionName':function[1],'assembly':instruction.mnemonic+' '+instruction.op_str,'kind':'directWrite' if direct_write else 'immediateAddressReference' if operand.type==X86_OP_IMM else 'directRead'}
            references.append(reference)
            if direct_write:
                if instruction.mnemonic!='mov' or len(instruction.operands)!=2 or instruction.operands[0].size!=4:
                    raise RuntimeError('Subscriber 쓰기 구간의 별도 명령 계약 필요')
                entry=instruction.address
                source=instruction.operands[1]
                source_kind='immediate' if source.type==X86_OP_IMM else 'register' if source.type==X86_OP_REG else 'unsupported'
                previous=instructions[index-1]
                if source.type==X86_OP_REG and previous.mnemonic=='mov' and len(previous.operands)==2 and previous.operands[0].type==X86_OP_REG and previous.operands[0].reg==source.reg:
                    loaded=previous.operands[1]
                    if loaded.type==X86_OP_MEM and not loaded.mem.base and not loaded.mem.index and loaded.mem.disp==GLOBALS[1]:
                        entry=previous.address
                        source_kind='currentLoginLoad'
                if source_kind=='unsupported':
                    raise RuntimeError('Subscriber 직접쓰기 source형식 추가 필요')
                writes.append((instruction,entry,source_kind))
    text=(ROOT/'extracted/refined/originals/Netstorm.c').read_text(encoding='utf-8').splitlines()
    refined=[]
    current=''
    for line_number,line in enumerate(text,1):  # 정밀 C의 직접 대입은 실제 PE 참조와 독립 목록으로 남긴다.
        if line.startswith('// ==== '):
            current=line
        if re.search(r'(?:_?DAT_)(?:00540c74|0054265c|00594fa4|005c85a4)\s*=(?!=)',line):
            refined.append({'line':line_number,'function':current,'statement':line.strip()})
    initial=[oracle.words(address,1)[0] for address in GLOBALS]
    if initial!=[0xffffffff,1,0,0]:
        raise RuntimeError('Subscriber/Editor/post 실제 PE 초기값 차이')
    audit={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','initialPeDwords':{name:{'address':f'{address:08x}','value':value} for name,address,value in zip(NAMES,GLOBALS,initial)},'allExplicitPeReferences':references,'refinedDirectAssignments':refined,'directWrites':len(writes),'loginDirectWrites':sum(item['global']=='0054265c' and item['kind']=='directWrite' for item in references),'limits':'선형.text의명시주소/즉시주소만분류. indirect alias쓰기·전체큰부모진행의완전성증명은아님. 정밀C 중복·별명표현보다PE사이트우선.'}
    return audit,writes


class Oracle(MemoryOracle):
    """각 직접쓰기의 실제 명령1개 또는 현재login load+store2개만 실행한다."""
    def __init__(self):
        """보호 원본을 가상 메모리에 복사하고 선택 구간의 쓰기를 감사한다."""
        super().__init__()
        self.running=False
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """관측 대상 네 DWORD만 쓰도록 제한하고 실제 주소/폭/값을 남긴다."""
        if self.running:
            if address not in GLOBALS or size!=4:
                raise RuntimeError('Subscriber 명령구간의 예상 밖 쓰기')
            self.trace.append(f'{address:08x}:{size}:{value:08x}')

    def instruction(self,mu,address,size,data):
        """큰부모나외부자식으로진행하지않고선택한최소구간만실행한다."""
        if not self.entry<=address<self.end:
            raise RuntimeError('Subscriber 선택 쓰기 구간 밖 명령')
        self.instructions+=1
        if self.instructions>2:
            raise RuntimeError('Subscriber 최소 구간 명령 수 초과')

    def observe(self,instruction,entry,source_kind,source_value,control):
        """명시 레지스터 입력의 쓰기 구간이며 큰부모에서의 도달가능성을 가정하지 않는다."""
        self.running=False
        self.entry,self.end=entry,instruction.address+instruction.size
        self.instructions,self.trace=0,[]
        for index,address in enumerate(GLOBALS):  # 선택 필드 이외의 이전값 보존도 관측한다.
            self.put(address,0x89abcdef+index*17)
        self.mu.reg_write(x86_const.UC_X86_REG_ESP,STACK)
        self.mu.reg_write(x86_const.UC_X86_REG_FPCW,control)
        self.mu.reg_write(x86_const.UC_X86_REG_FPSW,0)
        self.mu.reg_write(x86_const.UC_X86_REG_FPTAG,0xffff)
        if source_kind=='currentLoginLoad':
            self.put(GLOBALS[1],source_value)
        elif source_kind=='register':
            name=instruction.reg_name(instruction.operands[1].reg).upper()
            self.mu.reg_write(getattr(x86_const,'UC_X86_REG_'+name),source_value)
        before=''.join(bytes(self.mu.mem_read(address,4)).hex() for address in GLOBALS)
        self.running=True
        self.mu.emu_start(self.entry,self.end)
        self.running=False
        if self.mu.reg_read(x86_const.UC_X86_REG_EIP)!=self.end or self.mu.reg_read(x86_const.UC_X86_REG_ESP)!=STACK:
            raise RuntimeError('Subscriber 명령 checkpoint 또는 ESP 차이')
        if self.mu.reg_read(x86_const.UC_X86_REG_FPCW)!=control or self.mu.reg_read(x86_const.UC_X86_REG_FPSW)!=0 or self.mu.reg_read(x86_const.UC_X86_REG_FPTAG)!=0xffff:
            raise RuntimeError('Subscriber 명령구간 FPU 차이')
        after=''.join(bytes(self.mu.mem_read(address,4)).hex() for address in GLOBALS)
        return before,after,';'.join(self.trace),str(self.instructions)


def main():
    """제안제품전의최초정적/명령구간관측을고정하며나중에는SHA만감사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    if arguments.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative,expected in report['sha256'].items():  # 원시 fixture를 후속 소유자 제안으로 다시 생성하지 않는다.
            if digest(ROOT/relative)!=expected:
                raise RuntimeError('Subscriber 최초 SHA 차이: '+relative)
        print(json.dumps({'verified':True,'observations':report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists() or AUDIT.exists():
        raise RuntimeError('Subscriber 최초 자료 덮어쓰기 금지')
    if (ROOT/'extracted/cpp-client-subscriber-globals-20261024-proposed/copy/cpppj/src/client/ClientSubscriberGlobals.h').exists():
        raise RuntimeError('Subscriber 제안 제품 선작성 금지')
    oracle=Oracle()
    audit,writes=static_audit(oracle)
    AUDIT.write_text(json.dumps(audit,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    rows=['# storeSite / rangeEntry / sourceKind / sourceInput / control / before4DwordsConcat / after4DwordsConcat / actualStores / instructionCounter']
    total=0
    for instruction,entry,source_kind in writes:  # 각 직접쓰기의 실제구간에만 명시입력을 공급한다.
        for source in (0,1,0x80000000,0xffffffff,0x12345678):
            for control in (0x27f,0x37f):
                observed=oracle.observe(instruction,entry,source_kind,source,control)
                rows.append('\t'.join((f'{instruction.address:08x}',f'{entry:08x}',source_kind,str(source),str(control),*observed)))
                total+=oracle.instructions
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_subscriber_lifetime_edges.py',FIXTURE.relative_to(ROOT).as_posix(),AUDIT.relative_to(ROOT).as_posix())
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','scope':'직접쓰기 최소기계어구간만. 큰Init/Editor/Login/leave부모의분기·실제함수전체호출은아님. 기존Native자료불변. 4DWORD연결hex는관측표현이며실제인접배열이아님.','initialPeDwords':audit['initialPeDwords'],'directStoreSites':len(writes),'observations':len(rows)-1,'instructionCounter':total,'proposalProductAbsentBeforeObservation':True,'wholeParentOrIndirectWritesVerified':False,'sha256':{relative:digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:report[key] for key in ('directStoreSites','observations','instructionCounter')}))


if __name__=='__main__':
    main()
