#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort Badges의 전체 raw 장부와 Technology 부모 wrapper를 두 보호 PE에서 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS as PE_SPECS,STOP,STACK_BASE,STACK,digest,pefile
from decomp_priestgeometry_oracle import GeometryOracle
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 에뮬레이터 내부의 MemStream/저장 자료/변환표 주소이며 호스트 포인터가 아니다.
ARENA,STREAM,DATA,CONVERSION=0x11000000,0x11000100,0x11001000,0x11002000
FIXTURE=ROOT/'cpppj/tests/fixtures/fortmeta-x86.tsv'
REPORT=ROOT/'cpppj/recovery-fortmeta-evidence.json'
# 부모/장부 몸체와 명시 섹션·내용물·변환·UI·진단 경계의 실제 주소다.
PLAN={
 'originals':dict(badges=0x4be8e0,setter=0x47d800,technology=0x4beee0,lookup=0x44ceb0,
   parent=0x48fc60,prepare=0x4bd800,contents=0x4bd130,free=0x4e4354,ui=0x42f540,assertion=0x4e0620,
   table=0x593160,depth=0x5c89b8,conversion=0x5c89bc,templates=0x5c89f0,owner=0x540cac,
   lines={0x423:'B',0x165b:'C',0x4b5:'T',0x4b6:'M'}),
 'originalCD':dict(badges=0x428fc0,setter=0x475660,find=0x475560,technology=0x4293d0,lookup=0x44c4e0,
   parent=0x406d90,prepare=0x427e00,contents=0x426dd0,free=0x4f1a50,ui=0x4b2e70,assertion=0x490040,
   table=0x52ae20,depth=0x5178d4,conversion=0x5178d8,templates=0x564bd0,owner=0x50f6d4,
   lines={0x413:'B',0x10df:'C',0x460:'T',0x461:'M'})}
# 기대값과 분리된 입력 필드다. primitive count는 DWORD 비트로 전달한다.
KEYS=('kind','type','count','profile','template','slot','present','end','depth','oldConversion','newConversion','mutation')


def case(**changes):
    """기본 입력만 준비하고 진단/장부/호출 순서의 기대 결과는 계산하지 않는다."""
    values=dict(zip(KEYS,(0,80,1,0,1,1,1,6,0,0,1,0)));values.update(changes);return values


def inputs(edition):
    """직접 setter·BYTE 목록·현재 Template/깊이/변환 상태를 독립 교차한다."""
    rows=[]
    # setter의 범위 진단 뒤 full DWORD 쓰기와 첫 중복 타입을 방문한다.
    for typ,count,profile in itertools.product((70,80,97,255,999,0xffffffff),(0,1,255,256,0xffffffff,0x80000000),(0,1)):
        rows.append(case(kind=1,type=typ,count=count,profile=profile))
    # 진단 callback이 현재 표를 바꾼 뒤의 검색도 원본 명령으로 관찰한다.
    for count in (256,0xffffffff):rows.append(case(kind=1,type=999,count=count,mutation=3))
    # 목록의 count와 물리 bytes를 준비하며 end를 초과한 pos도 원본 그대로 보존한다.
    for count,template,present,end,depth,profile in itertools.product((0,1,3,28,255),(0,1),(0,1),(0,1,511),(0,0xffffffff),(0,1)):
        rows.append(case(count=count,template=template,present=present,end=end,depth=depth,profile=profile))
    # empty/missing Technology는 기존 변환표를 보존하고 getter/UI를 항상 호출한다.
    for slot,template,present,end,old,new in itertools.product((0,1,2),(0,1),(0,1),(0,1,6),(0,1),(0,1)):
        rows.append(case(kind=2,slot=slot,template=template,present=present,end=end,oldConversion=old,newConversion=new))
    # 하위 contents/UI 뒤의 현재 변환포인터/깊이 재조회와 DWORD 감김을 관찰한다.
    for depth,mutation,new in itertools.product((1,0xffffffff),(0,1,2),(0,1)):
        rows.append(case(kind=2,depth=depth,mutation=mutation,newConversion=new,oldConversion=1))
    # CD의 별도 linear lookup은 실제 함수로 실행한다. patch 내부 검색 관찰과 구분한다.
    if edition=='originalCD':
        for typ,profile in itertools.product((70,80,97,999,0xffffffff),(0,1)):rows.append(case(kind=3,type=typ,profile=profile))
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


def badge_bytes(profile):
    """padding까지 명시한 28×220 BYTE 입력 표이며 기대 count를 만들지 않는다."""
    raw=bytearray([0xa5])*(28*220)
    # 첫 중복 타입/마지막 unsigned 타입을 포함하며 모든 count는 초기 A5 패턴이다.
    for index in range(28):
        typ=70+index
        if profile and index in (1,8):typ=80
        if profile and index==27:typ=0xffffffff
        struct.pack_into('<I',raw,index*220,typ)
    return bytes(raw)


def rle(raw):
    """전체 raw bytes를 무손실 BYTE run으로 저장하여 padding 변화도 대조한다."""
    parts=[]
    # 동일 바이트의 길이와 값만 기록하며 badge 의미를 해석하지 않는다.
    for value,group in itertools.groupby(raw):parts.append(f'{sum(1 for _ in group)}:{value:02x}')
    return ','.join(parts)


class MetaOracle:
    """전체 장부/상위 wrapper를 실제 실행하고 하위 생성·변환·화면은 이름 있는 경계로 관찰한다."""
    prepare_registers=GeometryOracle.prepare_registers
    check=GeometryOracle.check

    def __init__(self,edition):
        """현재 PC의 readOnly 불연속 범위를 고정하고 보호 PE를 메모리에서만 읽는다."""
        self.edition,self.p,self.spec=edition,PLAN[edition],PE_SPECS[edition]
        pe=pefile.PE(str(ROOT/self.spec['binary']));data=pe.get_memory_mapped_image();self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(0x400000,(len(data)+4095)&~4095);self.mu.mem_write(0x400000,data)
        # FS/관찰 메모리/종료점/스택은 각각 독립 페이지다.
        for address,length in ((0,4096),(ARENA,0x4000),(STOP,4096),(STACK_BASE,0x40000)):self.mu.mem_map(address,length)
        self.allowed=bytearray(0x200000);self.entries=set();self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        self.exports=[ROOT/f'extracted/fortmeta/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.exports[1].open(encoding='utf-8-sig') as stream:
            # 실제 내보내기 범위만 실행한다. prepare는 전체 하위 복원으로 세지 않는 명시 경계다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed[low-0x400000:high+1-0x400000]=b'\1'*(high+1-low)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def read(self,address):
        """현재 DWORD만 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little')

    def put(self,address,value):
        """명시 입력/경계 변경의 DWORD를 저장한다."""
        self.mu.mem_write(address,(value&0xffffffff).to_bytes(4,'little'))

    def write(self,mu,access,address,size,value,data):
        """원본 명령의 쓰기를 장부/깊이/변환/FS/stream/stack으로 제한한다."""
        p=self.p
        if address==p['depth'] and size==4:self.events.append(f'L:{value&0xffffffff}')
        ranges=((0,4),(STACK_BASE,0x40000),(STREAM,12),(p['table'],6160),(p['depth'],8))
        if any(low<=address and address+size<=low+length for low,length in ranges):return
        raise RuntimeError(f'FortMeta 허용밖 쓰기 {self.edition} {address:08x}+{size}')

    def instruction(self,mu,address,size,data):
        """명시 하위 경계의 ABI만 공급하고 setter/lookup/SEH/depth는 원본 명령으로 실행한다."""
        p=self.p;targets={p[key] for key in ('lookup','parent','prepare','contents','free','ui','assertion')}
        if address not in targets:
            if not 0x400000<=address<0x600000 or not self.allowed[address-0x400000]:raise RuntimeError(f'FortMeta 허용밖 native {self.edition} {address:08x}')
            if address in self.entries:self.native[f'{address:08x}']+=1
            return
        self.boundaries[f'{address:08x}']+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);args=[self.read(sp+4+index*4) for index in range(4)]
        purge=value=0;c=self.case
        if address==p['lookup']:
            name=b'Badges\0' if c['kind']==0 else b'Technology\0'
            assert bytes(mu.mem_read(args[0],len(name)))==name and args[1]==1
            self.put(STREAM+4,0);value=STREAM;purge=8
        elif address==p['parent']:
            value=50;self.events.append('P:50')
        elif address==p['prepare']:
            self.events.append(f'N:{int(bool(self.read(p["conversion"])))}');self.put(p['conversion'],CONVERSION if c['newConversion'] else 0)
        elif address==p['contents']:
            assert args[:4]==[50,STREAM,17,1]
            self.events.append(f'C:50:17:1:{self.read(p["depth"])}:{self.read(STREAM+4)}')
            self.put(STREAM+4,5)
            if c['mutation']==2:self.put(p['conversion'],CONVERSION)
        elif address==p['free']:
            assert args[0]==CONVERSION;self.events.append('F')
        elif address==p['ui']:
            self.events.append('U')
            if c['mutation']==1:self.put(p['depth'],self.read(p['depth'])+5)
        elif address==p['assertion']:
            self.events.append('A:'+p['lines'][args[2]]);self.asserts+=1
            if c['mutation']==3:self.put(p['table'],999)
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control):
        """현재 입력으로 하나의 실제 wrapper/primitive를 실행하고 full raw/state/effect를 관찰한다."""
        self.case=case;self.events=[];self.asserts=0;p=self.p;self.mu.mem_write(0,bytes(4));self.mu.mem_write(ARENA,bytes(0x4000))
        self.mu.mem_write(p['table'],badge_bytes(case['profile']));self.put(p['depth'],case['depth']);self.put(p['conversion'],CONVERSION if case['oldConversion'] else 0)
        template=p['templates']+case['slot']*20;self.put(template,ARENA if case['template'] else 0);self.put(p['owner'],1)
        count=case['count'] if case['kind']==0 else 3;raw=bytearray([count])
        # 255개까지 실제 메모리를 공급하며 end가 작아도 원본 pointer 읽기를 허용한다.
        for index in range(count):raw.extend((255 if index%7==6 else 70+index%28,(index*13+1)&255))
        self.mu.mem_write(DATA,bytes(raw));self.put(STREAM,DATA if case['present'] else 0);self.put(STREAM+4,19);self.put(STREAM+8,case['end'])
        self.prepare_registers(control);self.mu.reg_write(UC_X86_REG_ECX,template)
        args=[case['type'],case['count']] if case['kind']==1 else ([case['type']] if case['kind']==3 else [])
        entry=p['setter'] if case['kind']==1 else (p['find'] if case['kind']==3 else p['technology'] if case['kind']==2 else p['badges'])
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP,count=100000)
        self.check(control,STACK+4);self.returns+=1
        result=str(self.mu.reg_read(UC_X86_REG_EAX)) if case['kind']==3 else '-'
        return [result,str(self.read(STREAM+4)),str(self.read(STREAM+8)),str(self.read(p['depth'])),str(int(bool(self.read(p['conversion'])))),
                ';'.join(self.events) or '-',rle(bytes(self.mu.mem_read(p['table'],6160))),str(self.asserts)]


def generate(smoke):
    """같은 고유 입력을 두 x87 제어값으로 관찰하고 동일 결과만 fixture에 저장한다."""
    rows=[];editions={};paths=[Path(__file__).resolve(),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',
       ROOT/'tools/ghidra/fortmeta-functions.json',ROOT/'cpppj/src/o/RawFortMetaLoad.h',ROOT/'cpppj/src/o/RawFortMetaLoad.cpp',ROOT/'cpppj/tests/FortMetaLoadTests.cpp']
    # 두 판본은 각각 자기 PE·원본 주소와 current raw table을 사용한다.
    for edition in PLAN:
        oracle=MetaOracle(edition);cases=inputs(edition)
        if smoke:cases=[cases[index] for index in range(0,len(cases),max(1,len(cases)//25))]
        normal=assertions=0
        # 반복 x87 관찰은 추가 고유 완료 입력으로 세지 않는다.
        for sample in cases:
            observed=[oracle.run(sample,control) for control in CONTROLS];assert observed[0]==observed[1],(edition,sample)
            row=observed[0];normal+=row[-1]=='0';assertions+=row[-1]!='0';rows.append([edition,','.join(str(sample[key]) for key in KEYS),*row])
        editions[edition]=dict(cases=len(cases),normal_cases=normal,assert_cases=assertions,returns=oracle.returns,native_calls=dict(oracle.native),boundaries=dict(oracle.boundaries))
        paths.extend([ROOT/oracle.spec['binary'],*oracle.exports]);print(f'{edition}: FortMeta {len(cases)} normal {normal}/assert {assertions}',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition\t'+','.join(KEYS)+'\tfindBits\tposition\tend\tdepth\tconversion\tevents\tfullBadgeRle\tasserts\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');paths.append(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',total=len(rows),controls=list(CONTROLS),game_runs=0,os_calls=0,editions=editions,
       files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limitations=['Badges wrapper/setter와 CD linear lookup·Technology 상위 wrapper/SEH/depth/isMine/free gate는 실제 원본 명령이다.',
       '파일 섹션 조회·현재 technology 부모 getter·TypeNames 변환표 준비·하위 Contents·free/UI/진단은 명시 외부 경계다.',
       '기존 Contents/ContainedCreate의 실제 같은 풀 소비자 검사는 새 wrapper 연결 검사이며 기존 원본 입력을 신규 완료로 합산하지 않는다.',
       '전체 Fort 부모/GameWorld 설치 및 TypeNames 전신의 복원 완료를 주장하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 전체 파일 SHA·행 고유성·normal/assert 분리·반복 ABI 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 보호 PE/현재 source/tool/원본 범위/fixture 전체를 하나씩 대조한다.
    for name,expected in report['files'].items():assert digest(ROOT/name)==expected,name
    assert len(rows)==report['total'] and len({(row[0],row[1]) for row in rows})==len(rows)
    # assert 보고 뒤 반환한 입력과 정상 입력은 행 마지막 열로 별도 센다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];assert len(selected)==item['cases']
        assert sum(row[-1]=='0' for row in selected)==item['normal_cases'] and sum(row[-1]!='0' for row in selected)==item['assert_cases']
        assert item['returns']==2*len(selected)
    print(f'FortMeta 현재 SHA/고유 normal/assert 감사: {len(rows)}')


# 표본은 저장하지 않고 전수만 기록한다. --verify는 원본/현재 source의 출처 감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
