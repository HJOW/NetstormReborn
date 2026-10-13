#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 두PE의 섬owner/territory 검색·territory 검색·현재owner계수를 실제3204BYTE에서 독립 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_regionpaint_oracle import ROOT,digest,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 원본포인터와호스트자료를분리한 실제기록/복귀주소/스택 입력공간이다.
ISLANDS,STOP,STACK_BASE,STACK=0x1b000000,0x20000000,0x30000000,0x3000f000
BYTES,CONTROLS=3204,(0x027f,0x037f)
# 세검색만신규독립입력으로계수하며 기존getter는 실제callee범위와별도통계로보존한다.
SPECS={
 'originals':dict(binary='originals/Netstorm.exe',fort=0x594fb8,special=0x5c85a4,chaff=0x594fa4,
    entries=dict(FindOwnerTerritory=0x46f380,FindTerritory=0x46f3f0,CountOwned=0x46f430),helpers=(0x46ee50,0x46ee30,0x46f320)),
 'originalCD':dict(binary='originalCD/NETSTORM.EXE',fort=0x540a1c,special=0x518904,chaff=0x52e170,
    entries=dict(FindOwnerTerritory=0x4bdaa0,FindTerritory=0x4bdb10,CountOwned=0x4bdb60),helpers=(0x4bd4f0,0x4bd4c0,0x4bda70,0x4bd480))
}
FIXTURE=ROOT/'cpppj/tests/fixtures/islandqueries-x86.tsv'
REPORT=ROOT/'cpppj/islandqueries.json'


def inputs():
    """기대결과계산없이 signedBYTE/currentDWORD/비정규exists/fort와순서·빈기록·100기록경계를교차한다."""
    cases=[]
    def add(kind,**values):
        """명시12개필드의기본입력을수정해실제검색진입점에전달한다."""
        names=('count','scene','exists','current','original','territory','wantowner','wantterritory','fort','special','chaff','target')
        case=[1,0,1,1,1,2,1,2,0,0,0,0]
        for name,value in values.items():case[names.index(name)]=value
        cases.append((kind,tuple(case)))
    # 원래owner signedBYTE와현재ownerDWORD가다른경우와fort의0현재owner특례를직접관찰한다.
    for current,original,territory,owner,want,fort,exists in itertools.product((0,1,2,255,0xffffffff),(0,1,255),(0,2,255),(0,1,2,0xffffffff),(0,2,0xffffffff),(0,1),(0,1,2)):
        add('FindOwnerTerritory',current=current,original=original,territory=territory,wantowner=owner,wantterritory=want,fort=fort,exists=exists)
    # territory는BYTE부호확장이며현재owner나원래owner·fort와는별개의검색이다.
    for territory,want,exists,fort in itertools.product((0,2,127,128,255),(0,2,127,128,255,0xffffffff,0xffffff80),(0,1,2),(0,1)):
        add('FindTerritory',territory=territory,wantterritory=want,exists=exists,fort=fort)
    # owner0/상위BYTE/부호비트는현재DWORD를그대로비교한다.
    for current,owner,exists in itertools.product((0,1,2,255,256,0x80000000,0xffffffff),(0,1,2,255,256,0x80000000,0xffffffff),(0,1,2)):
        add('CountOwned',current=current,wantowner=owner,exists=exists)
    # 여러일치/삭제구멍/최초번호·번호99와signedcount0/음수에서원본순회를관찰한다.
    for kind,count,scene,exists,target,fort in itertools.product(('FindOwnerTerritory','FindTerritory','CountOwned'),(0,1,3,100,0x80000000,0xffffffff),(0,1,2),(1,2),(0,2,99),(0,1)):
        add(kind,count=count,scene=scene,exists=exists,target=target,fort=fort)
    # 기존Matches가읽는special/chaff를검색3몸체의조건으로추가하지않도록별도확인한다.
    for kind,special,chaff in itertools.product(('FindOwnerTerritory','FindTerritory','CountOwned'),(0,1),(0,1)):
        add(kind,current=2,original=1,special=special,chaff=chaff)
    return list(dict.fromkeys(cases))


def input_records(case):
    """원본기록의입력만만든다. 검색반환/일치개수는계산하지않는다."""
    count,scene,exists,current,original,territory,owner,want,fort,special,chaff,target=case
    records=bytearray(b'\xa5'*BYTES);struct.pack_into('<I',records,0,count)
    # 실제100개기록의패딩/신전/bounds와두종류owner를별도필드로입력한다.
    for index in range(100):
        record=4+index*32;struct.pack_into('<3I',records,record,0x11110000+index,0,current)
        records[record+12]=territory;records[record+13]=original
        if scene==1:struct.pack_into('<I',records,record+4,exists if index%3 else 0)
        if scene==2:
            struct.pack_into('<I',records,record+4,exists if index in (0,2,99) else 0)
            if index%2:struct.pack_into('<I',records,record+8,current^0x01000100);records[record+13]=original^0x80;records[record+12]=territory^0x80
    struct.pack_into('<I',records,4+target*32+4,exists)
    return bytes(records)


class IslandQueriesOracle:
    """새6/6정밀범위의원본명령만실행한다. getter·메모리·진단을가짜값으로대체하지않는다."""
    def __init__(self,edition):
        """보호PE를읽기만해매핑하고현재호스트의정밀함수범위를연결한다."""
        self.edition=edition;self.s=SPECS[edition];self.native=collections.Counter();self.helpers=collections.Counter();self.returns=0
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);pe=pefile.PE(str(ROOT/self.s['binary']));data=pe.get_memory_mapped_image()
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(len(data)+4095)&~4095);self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase,data)
        # 기록은3204BYTE만유효하며스택/복귀주소는별도페이지다.
        for address,size in ((ISLANDS,4096),(STOP,4096),(STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.exports=[ROOT/f'extracted/islandqueries/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=bytearray(0x200000);self.selected_ranges={}
        selected=[]
        with self.exports[1].open(encoding='utf-8') as stream:selected.extend(csv.DictReader(stream,delimiter='\t'))
        if edition=='originalCD':
            paths=[ROOT/f'extracted/island-runtime/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:selected.extend(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x4bd480)
        # CD Exists의이미복원된validId한몸체만기존14함수자료에서선택해재사용한다.
        for row in selected:
            self.selected_ranges[row['entry']]=row['ranges']
            # Ghidra의불연속원본몸체만허용하며이웃함수로실행범위를넓히지않는다.
            for span in row['ranges'].split(';'):
                low,high=(int(value,16) for value in span.split('-'));self.allowed[low-0x400000:high+1-0x400000]=b'\x01'*(high+1-low)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid)

    def instruction(self,mu,address,size,data):
        """세신규몸체와기존getter호출을분리계수하며허용밖원본/OS실행은실패한다."""
        if address==STOP:mu.emu_stop();return
        if not(0x400000<=address<0x600000 and self.allowed[address-0x400000]):raise RuntimeError(f'IslandQueries 허용밖native {address:08x} {self.kind} {self.case}')
        if address in self.s['entries'].values():self.native[f'{address:08x}']+=1
        if address in self.s['helpers']:self.helpers[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """세검색은읽기전용이다. 실제스택저장외원본기록/전역쓰기를허용하지않는다."""
        if not(STACK_BASE<=address and address+size<=STACK_BASE+0x10000):raise RuntimeError(f'IslandQueries 예상밖쓰기 {address:08x}+{size} {self.kind} {self.case}')

    def invalid(self,mu,access,address,size,value,data):
        """물리100기록밖/미매핑주소를임의보충하지않고진단한다."""
        raise RuntimeError(f'IslandQueries 미매핑 {address:08x}+{size} eip={mu.reg_read(UC_X86_REG_EIP):08x} {self.kind} {self.case}')

    def run(self,kind,case,control):
        """기대값없이입력3204BYTE를전달하고실제EAX/전체기록·모드불변/ABI반환을읽는다."""
        self.kind=kind;self.case=case;mu=self.mu;mu.mem_write(ISLANDS,input_records(case))
        for name,value in zip(('fort','special','chaff'),case[8:11]):mu.mem_write(self.s[name],struct.pack('<I',value))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        for register,value in preserved.items():mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        args=(case[6],case[7]) if kind=='FindOwnerTerritory' else ((case[7],) if kind=='FindTerritory' else (case[6],))
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,ISLANDS)
        mu.emu_start(self.s['entries'][kind],STOP,count=20000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+4*len(args):raise RuntimeError('IslandQueries 정상복귀/stack오류')
        if any(mu.reg_read(register)!=value for register,value in preserved.items()) or mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('IslandQueries 보존레지스터/x87오류')
        self.returns+=1
        raw=bytes(mu.mem_read(ISLANDS,BYTES))
        if raw!=input_records(case):raise RuntimeError('IslandQueries 실제전체3204BYTE 불변오류')
        return [mu.reg_read(UC_X86_REG_EAX),zlib.adler32(raw),','.join(str(struct.unpack('<I',mu.mem_read(self.s[name],4))[0]) for name in ('fort','special','chaff'))]


def generate(smoke=False):
    """새검색의두x87반환만수집하고입력/반환·기존callee호출·PE/범위/SHA를분리기록한다."""
    cases=inputs();cases=cases[::97] if smoke else cases;rows=[];editions={};files=[Path(__file__),ROOT/'tools/ghidra/islandqueries-functions.json',FIXTURE]
    for edition in SPECS:
        oracle=IslandQueriesOracle(edition)
        for kind,case in cases:
            output=oracle.run(kind,case,CONTROLS[0]);repeat=oracle.run(kind,case,CONTROLS[1])
            if output!=repeat:raise RuntimeError(f'IslandQueries 두정밀도차이 {edition} {kind} {case}')
            rows.append([edition,kind,','.join(map(str,case)),*output])
        editions[edition]=dict(cases=len(cases),normal_cases=len(cases),assert_cases=0,returns=oracle.returns,
            by_body=dict(collections.Counter(kind for kind,case in cases)),native=dict(oracle.native),existing_getter_calls=dict(oracle.helpers),selected_ranges=oracle.selected_ranges)
        files.extend(oracle.exports+[ROOT/oracle.s['binary']]);print(f'{edition}: 정상{len(cases)}/assert0 고유입력, 반환{oracle.returns}',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition kind input result rawAdler modes\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.extend(ROOT/name for name in ('tools/decomp_regionpaint_oracle.py','cpppj/src/o/RawIslandQueries.h','cpppj/src/o/RawIslandQueries.cpp','cpppj/tests/IslandQueriesTests.cpp',
        'cpppj/src/o/RawIslandRuntime.h','cpppj/src/o/RawIslandRuntime.cpp','cpppj/islandruntime.json') if (ROOT/name).exists())
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=editions,os_calls=0,external_calls=0,new_existing_getter_observations=0,
        readonly_full_bytes_checked=True,files={path.relative_to(ROOT).as_posix():digest(path) for path in dict.fromkeys(files)},limits=[
        '실제3204BYTE/currentCount/Exists/현재ownerDWORD·원래owner와territory signedBYTE의세검색몸체를실행한다.',
        '기존count/Exists/Owner getter는허용callee와회귀근거이며새독립관찰로합산하지않는다. 외부stub0/OS0이다.',
        '물리100기록초과양수count는원본배열밖접근이므로정상입력에포함하지않는다.',
        '동일월드의실제mutable목록연결이가능하다. GameWorld초기순서/승패·캠페인완주검증은아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows)}),flush=True)


def verify():
    """현재SHA·고유신규검색입력·반환/몸체별분류와기존getter비중복계수를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('IslandQueries SHA불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(inputs())*2 or report['os_calls'] or report['external_calls'] or report['new_existing_getter_observations'] or not report['readonly_full_bytes_checked']:raise RuntimeError('IslandQueries 입력/외부계수오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({(row[1],row[2]) for row in selected})!=len(selected) or item['returns']!=len(selected)*2 or item['normal_cases']!=len(selected) or item['assert_cases'] or item['by_body']!=dict(collections.Counter(row[1] for row in selected)):raise RuntimeError('IslandQueries 고유입력/반환분류오류')
        expected={f'{SPECS[edition]["entries"][kind]:08x}':count*2 for kind,count in item['by_body'].items()}
        if item['native']!=expected:raise RuntimeError('IslandQueries 신규진입점반환계수오류')
        for row in selected:
            case=tuple(map(int,row[2].split(',')))
            if int(row[4])!=zlib.adler32(input_records(case)) or row[5]!=','.join(map(str,case[8:11])):raise RuntimeError('IslandQueries 읽기전용불변오류')
    print(f'IslandQueries {len(rows)}개 현재SHA/신규검색·기존getter분리/두정밀도반환감사 통과')


# 명시한독립생성/부분실행/현재출처감사만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
