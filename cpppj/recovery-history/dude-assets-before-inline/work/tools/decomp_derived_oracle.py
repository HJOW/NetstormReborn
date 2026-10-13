#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""자산 파생 생성자의 실제 x86 쓰기/가상 초기화를 창 없이 격리 검증한다."""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import zlib
from decomp_creation_oracle import CreationOracle, CREATION
from decomp_sid_oracle import POOL
from decomp_oracle import ROOT, STOP, STACK
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP
from unicorn import x86_const as uc_registers
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

# null 반환/명시적 assert 생성자는 원본 주소 표에 남기고 안전한 dispatch에서는 제외한다.
EXCLUDED={'originals':{0x4429e0:'null',0x44a420:'assert'},
          'originalCD':{0x414110:'null',0x44bfb0:'null',0x483520:'assert'}}
# 동일 입력을 재현하는 시드와 원본 자산 타입 시작 번호다.
SEED=0xDE1078
FIRST_ASSET=70


class DerivedOracle(CreationOracle):
    """Ghidra가 확인한 생성자 몸체와 순수 보조 함수만 추가로 허용한다."""
    def __init__(self,edition):
        """파생 생성자 범위와 쓰기 명령 해독기를 준비한다."""
        self.record_slot=None
        self.writes=[]
        self.expressions={}
        self.decoder=Cs(CS_ARCH_X86,CS_MODE_32)
        self.decoder.detail=True
        super().__init__(edition)
        folders=[f'extracted/derived/{edition}']
        if edition=='originals':
            folders.append('extracted/derived/patch-helpers')
        # CD Bomb 보조 함수는 같은 내보내기에 포함되고 CD HP 보조 함수는 기존 범위에 있다.
        for folder in folders:
            with (ROOT/folder/'functions.tsv').open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.facts.append(row)
                    for part in row['ranges'].split(';'):
                        lo,hi=(int(value,16) for value in part.split('-'))
                        self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """컴파일러의 load→register OR→store도 동일 슬롯 OR 계약으로 추적한다."""
        if self.record_slot is not None:
            instruction=next(self.decoder.disasm(bytes(mu.mem_read(address,size)),address,count=1))
            operands=instruction.operands
            if instruction.mnemonic=='mov' and operands[0].type==X86_OP_REG:
                name=instruction.reg_name(operands[0].reg)
                self.expressions.pop(name,None)
                if operands[1].type==X86_OP_MEM:
                    memory=operands[1].mem
                    # 레지스터 별칭 이름은 Unicorn의 같은 크기 레지스터 값으로 읽는다.
                    def register_value(reg):
                        return mu.reg_read(getattr(uc_registers,'UC_X86_REG_'+instruction.reg_name(reg).upper())) if reg else 0
                    location=register_value(memory.base)+register_value(memory.index)*memory.scale+memory.disp
                    if self.record_slot<=location<self.record_slot+self.stride:
                        self.expressions[name]=(location-self.record_slot,operands[1].size,0)
            elif instruction.mnemonic=='or' and operands[0].type==X86_OP_REG and operands[1].type==X86_OP_IMM:
                name=instruction.reg_name(operands[0].reg)
                if name in self.expressions:
                    offset,width,mask=self.expressions[name]
                    self.expressions[name]=(offset,width,mask|operands[1].imm)
            else:
                # 다른 레지스터 연산은 계약 추적에서 제거하며 입력 두 패턴 대조로 거부한다.
                for register in instruction.regs_access()[1]:
                    self.expressions.pop(instruction.reg_name(register),None)
        super().on_instruction(mu,address,size,data)

    def on_write(self,mu,access,address,size,value,data):
        """대상 슬롯의 mov/or를 해독하여 순서·폭·OR 마스크를 기록한다."""
        if self.record_slot is not None and POOL<=address<POOL+self.capacity*self.stride:
            offset=address-self.record_slot
            assert 0<=offset and offset+size<=self.stride
            pc=mu.reg_read(UC_X86_REG_EIP)
            instruction=next(self.decoder.disasm(bytes(mu.mem_read(pc,15)),pc,count=1))
            assert instruction.mnemonic in ('mov','or') and instruction.operands[0].type==X86_OP_MEM
            is_or=instruction.mnemonic=='or'
            if is_or:
                assert instruction.operands[1].type==X86_OP_IMM
                value=instruction.operands[1].imm
            elif instruction.operands[1].type==X86_OP_REG:
                expression=self.expressions.get(instruction.reg_name(instruction.operands[1].reg))
                if expression is not None:
                    assert expression[:2]==(offset,size)
                    is_or=True
                    before=int.from_bytes(mu.mem_read(address,size),'little')
                    assert value==(before|expression[2])
                    value=expression[2]
            self.writes.append((offset,size,is_or,value&((1<<(size*8))-1)))
        super().on_write(mu,access,address,size,value,data)

    def snapshot(self,sid):
        """기존 fixture와 같은 전체 풀/목록/슬롯 출력을 반환한다."""
        glob=self.spec['globals']
        free,cursor=[struct.unpack('<I',self.mu.mem_read(glob[i],4))[0] for i in (5,7)]
        heads=[struct.unpack('<H',self.mu.mem_read(POOL+i,2))[0] for i in (4,18)]
        tails=[(struct.unpack('<I',self.mu.mem_read(glob[i],4))[0]-POOL)//self.stride for i in (2,4)]
        return [sid,free,cursor,heads[0],tails[0],heads[1],tails[1],
                zlib.adler32(self.mu.mem_read(POOL,self.capacity*self.stride)),
                bytes(self.mu.mem_read(POOL+sid*self.stride,self.stride)).hex()]

    def construct(self,address,sid,record=False):
        """cdecl SID 생성자를 실제 호출하고 반환 포인터·스택을 확인한다."""
        self.registers()
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,sid))
        self.record_slot=POOL+sid*self.stride if record else None
        self.writes=[]
        self.expressions={}
        try:
            self.execute(address,STOP,STACK+4)
        finally:
            self.record_slot=None
        assert self.mu.reg_read(UC_X86_REG_EAX)==POOL+sid*self.stride
        return self.snapshot(sid)

    def audit(self,address):
        """오염 패턴 두 개에서 쓰기 계약을 추출하고 실제 vtable의 세 메서드를 검사한다."""
        sid=self.spec['server_first']+500
        sequences=[]
        # OR 비트와 frame/HP 이외 payload 보존은 서로 반대인 입력 패턴에서도 확인한다.
        for seed in (0x00,0xff):
            self.fill(sid,seed,4,74)
            self.construct(address,sid,True)
            sequences.append(tuple(self.writes))
        assert sequences[0]==sequences[1],f'입력 의존 mov: {address:08x}'
        operations=sequences[0]
        assert 1<=len(operations)<=5
        # C/어셈블리에서 검토한 vtable·frame·HP·플래그 외의 쓰기는 표로 옮기지 않는다.
        for offset,width,is_or,value in operations:
            if offset==0:
                assert width==4 and not is_or and 0x500000<=value<0x520000
            elif is_or:
                assert (offset,width,value)==(40 if self.edition=='originals' else 35,1,2)
            else:
                assert value==0 and (offset,width) in ((8,2),(26,4 if self.edition=='originals' else 2))
        vtable=struct.unpack('<I',self.mu.mem_read(POOL+sid*self.stride,4))[0]
        expected=(self.creation['entries']['PostCreate'],self.creation['entries']['PostTake'],self.creation['unpop'])
        actual=tuple(struct.unpack('<I',self.mu.mem_read(vtable+offset,4))[0] for offset in (40,44,72))
        assert actual==expected,f'별도 가상 효과: {address:08x} {actual}'
        return {'address':address,'vtable':vtable,'writes':operations,'virtual_methods':actual}


def generate():
    """모든 지원 자산 바인딩과 서버/클라이언트·mana 옵션에서 독립 기대값을 저장한다."""
    catalog=collections.defaultdict(dict)
    # 기존 검증된 생성자 표를 읽기만 하며 같은 주소의 별도 타입 바인딩도 모두 실행한다.
    for line in (ROOT/'cpppj/tests/fixtures/constructors-x86.tsv').read_text(encoding='utf-8').splitlines():
        if line and not line.startswith('#'):
            edition,number,address=line.split('\t')
            if int(number)>=FIRST_ASSET and int(address):
                catalog[edition][int(number)]=int(address)
    rows=['# 실제 자산 생성자/Create/가상 초기화/free 또는 void Take. 게임/OS API/대체 함수 없음.']
    lines=['// 실제 자산 생성자의 순서 있는 raw 쓰기. 주소는 호스트 포인터로 사용하지 않는다.']
    counts=collections.Counter()
    editions={}
    # 두 PE는 별도 Unicorn 메모리에서 실행하며 파일은 읽기만 한다.
    for edition,bindings in catalog.items():
        oracle=DerivedOracle(edition)
        oracle.setup(32768,True,False)
        recipes={address:oracle.audit(address) for address in sorted(set(bindings.values())-EXCLUDED[edition].keys())}
        lines.append('// '+edition+' 자산 파생 생성자. 중간 vtable 쓰기도 원본 순서로 보존한다.')
        lines.append(f'constexpr std::array<ConstructorRecipe,{len(recipes)}> k{"Patch" if edition=="originals" else "Cd"}Recipes{{{{')
        # 고정 길이 배열의 빈 항목은 C++ 값 초기화로 채운다.
        for address,recipe in recipes.items():
            ops=','.join('{'+f'{offset},{width},{str(is_or).lower()},0x{value:08x}U'+'}' for offset,width,is_or,value in recipe['writes'])
            lines.append('    {'+f'0x{address:08x}U,0x{recipe["vtable"]:08x}U,{len(recipe["writes"])},'+'{{'+ops+'}}},')
        lines.append('}};')
        supported={number:address for number,address in bindings.items() if address in recipes}
        rng=random.Random(SEED)
        for server in (False,True):
            for weak in (False,True):
                rows.append(f'Begin\t{edition}\t32768\t{int(server)}\t{int(weak)}')
                oracle.setup(32768,server,weak)
                # 각각의 바인딩은 실제 ctor 포인터를 가진 합성 타입 필드로 dispatch한다.
                for index,(number,address) in enumerate(supported.items()):
                    f1=16 if index%3 else 0
                    f2=0x400000 if number==165 else 0
                    hp=(-2147483648,-7,0,65536,0x12345678,2147483647)[index%6]
                    z=rng.randrange(-512,513)
                    rows.append('\t'.join(map(str,['Type',number,f1,f2,hp,z,address])))
                    oracle.type_input(number,f1,f2,hp,z,address)
                    # 실제 호출 출력만 저장한다. Python/C++ 모델로 기대값을 계산하지 않는다.
                    def step(name,arg):
                        result=oracle.construct(address,arg) if name=='Construct' else oracle.creation_step(name,number,arg)
                        rows.append('\t'.join(map(str,['Step',name,number,arg,*result])))
                        counts[name]+=1
                        return result[0]
                    flags=(0,1,2,3)[index%4] if server else (1,2,3)[index%3]
                    sid=step('Create',flags)
                    seed=rng.randrange(256)
                    state=(4,6,0x84,0xe4)[index%4]
                    rows.append(f'Fill\t{sid}\t{seed}\t{state}\t{number}\t{recipes[address]["vtable"]}')
                    oracle.fill(sid,seed,state,number)
                    oracle.mu.mem_write(POOL+sid*oracle.stride,struct.pack('<I',recipes[address]['vtable']))
                    step('Construct',sid)
                    step('Construct',sid)
                    step('PostCreate',sid)
                    step('PostTake',sid)
                    if sid>=oracle.spec['server_first']:
                        step('Take',sid)
                        step('Take',sid)
                    # free 슬롯을 받은 뒤 반복 void Take에서 실제 base Unpop을 호출한다.
                    received=oracle.spec['server_first']+600+index
                    step('Take',received)
                    step('Take',received)
        editions[edition]={'binary_sha256':oracle.sha256,'function_ranges':oracle.facts,
            'unique_constructors':len(recipes),'type_bindings':len(supported),'recipes':list(recipes.values()),
            'excluded':{f'{address:08x}':reason for address,reason in EXCLUDED[edition].items()},
            'audit_constructor_calls':len(recipes)*2,'assert_reports':oracle.assertions,
            'void_unpop_calls':oracle.unpop_calls,'max_instructions_observed':oracle.max_instructions}
        print(f'{edition}: {len(recipes)}개 생성자·{len(supported)}개 타입 연결',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/derived-x86.tsv'
    generated=ROOT/'cpppj/src/o/DerivedConstructors.inc'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    generated.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    sources=['tools/decomp_derived_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py',
             'tools/decomp_oracle.py','tools/ghidra/ExportCreation.java']
    artifacts=[fixture,generated,ROOT/'cpppj/tests/fixtures/constructors-x86.tsv']
    report={'schema':1,'classification':'scoped-asset-derived-constructors-and-void-take',
        'cases':dict(counts),'total_cases':sum(counts.values()),'setup_resets':10,'editions':editions,
        'artifact_sha256':{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in artifacts},
        'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
        'limits':['자산 타입만; null/assert 생성자와 form/process 제외',
            '합성 타입/이미 할당한 메모리; malloc/소진 복구/파일 로딩 없음',
            '파생 ctor의 raw 쓰기와 실제 공통 postCreate/postTake/void Unpop만',
            '대체 함수 없음; 전체 슬롯 바이트 대조, 전체 풀은 Adler-32',
            'non-void 공간/삭제/게임 월드/GUI/OS API/게임 진입점 미실행']}
    (ROOT/'cpppj/recovery-derived-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'total_cases':report['total_cases']}),flush=True)


def verify():
    """고정 기록의 원본/도구/표 해시와 fixture 호출 행 수를 확인한다."""
    report=json.loads((ROOT/'cpppj/recovery-derived-evidence.json').read_text(encoding='utf-8'))
    for group in ('artifact_sha256','source_sha256'):
        for path,sha in report[group].items():
            assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==sha,path
    for edition,item in report['editions'].items():
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==item['binary_sha256']
        assert item['assert_reports']==0
    counts=collections.Counter(line.split('\t')[1] for line in (ROOT/'cpppj/tests/fixtures/derived-x86.tsv').read_text(encoding='utf-8').splitlines() if line.startswith('Step\t'))
    assert dict(counts)==report['cases'] and sum(counts.values())==report['total_cases']
    print(json.dumps({'verified':True,'cases':dict(counts),'total_cases':sum(counts.values())}),flush=True)


# 명시적으로 호출한 경우에만 원본 함수를 격리 실행하거나 고정 해시를 확인한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate()
