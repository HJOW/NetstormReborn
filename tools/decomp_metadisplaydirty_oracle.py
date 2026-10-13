#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 Sky Overview 고유 dirty 큐·bitmap 확보/초기화·view 비교·전체/부분 소비를 격리 실행한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_oracle import ROOT, SCRATCH, STACK, STOP
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)
import pefile

# 독립 fixture·출처 보고서는 기존 Pop/Unpop oracle의 비활성 큐 근거와 구별한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/metadisplaydirty-x86.tsv'
REPORT=ROOT/'cpppj/recovery-metadisplaydirty-evidence.json'
# bitmap·표시 객체는 호스트와 분리한 에뮬레이터 메모리에만 둔다.
BITMAP,DISPLAY=SCRATCH+0x1000,SCRATCH+0x6000
# x87 두 정밀도 반복은 고유 입력 개수에 더하지 않는다.
CONTROLS=(0x027f,0x037f)
# 원본 큐/표시/bitmap/view/counter 및 명시 효과 경계 주소다.
SPEC={
    'originals':dict(queue=0x481510,refresh=0x481320,count=0x5949c0,bitmap=0x5949c4,width=0x5949cc,
        display=0x5949d0,full=0x5949d4,cells=0x5949e0,initialized=0x594ea8,camera=0x59a900,
        rectangle=0x5ca9cc,cachedCamera=0x594e90,cachedRectangle=0x594e98,frames=0x540bb0,
        allocate=0x4e4391,rebuild=0x480270,cell=0x4808d0,atexit=0),
    'originalCD':dict(queue=0x442f30,refresh=0x442d10,count=0x51c540,bitmap=0x51c544,width=0x51c548,
        display=0x51c54c,full=0x51c550,cells=0x5654d0,initialized=0x5654c0,camera=0x565c18,
        rectangle=0x583e20,cachedCamera=0x5654c8,cachedRectangle=0x565980,frames=0x51c554,
        allocate=0x4f1650,rebuild=0x4420d0,cell=0x442680,atexit=0x4f1560),
}


def bits(value):
    """좌표 입력은 TSV에 float 문자열 대신 실제 DWORD 비트로 보관한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def sha(path):
    """보호 PE와 입력/구현/정밀 몸체의 SHA를 그대로 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fnv(data):
    """전체 큐·bitmap·관찰 사건의 모든 바이트를 재생 가능한 FNV32로 접는다."""
    value=2166136261
    # 순서를 보존한 전체 바이트를 입력한다.
    for byte in data:value=((value^byte)*16777619)&0xffffffff
    return value


def inputs():
    """활성/중복/포화·절삭·view 비교·전체/부분·callback 변경 입력만 만든다."""
    result=[]
    coordinates=((0.,0.),(3.99,3.99),(4.,4.),(-3.99,-3.99),(-4.,-4.),(-7.99,7.99),
                 (255.99,256.),(10.5,12.25),(0.,40.),(396.,436.),(400.,440.),(2147483520.,-2147483648.))
    # 큐 등록은 같은 셀 중복이 포화 판단보다 먼저인지도 실제 코드에서 확인한다.
    for present,flags,count,point in itertools.product((0,1),(0,1,2,3),(0,2,99,100),coordinates):
        result.append((0,present,flags,count,7,1,3,0,0,0,0,bits(point[0]),bits(point[1])))
    # 최초 bitmap 확보는 비활성 표시에서도 실행한다. 기존 bitmap/포화와 각각 교차한다.
    for present,flags,bitmap,count,full in itertools.product((0,1),(0,1),(0,1),(0,2,100),(0,7)):
        result.append((1,present,flags,count,full,bitmap,3,0xffffffff,0,0,0,0,0))
    # static 초기화·signed zero·동일/다른 NaN·카메라/사각형 변경 비교는 두 판본 결과를 따로 남긴다.
    for initialized,camera,rectangle in itertools.product((0,1,2,3),range(6),range(3)):
        result.append((1,1,1,0,0,1,initialized,0,camera,rectangle,0,0,0))
    # rebuild 뒤 full 변경·부분 cell callback의 신규 고유 셀은 그 프레임에 다시 읽는다.
    for count,full,mutation in itertools.product((0,2),(0,1),(1,2,3,4)):
        result.append((1,1,1,count,full,1,3,0,3,1,mutation,0,0))
    return list(dict.fromkeys(result))


def view(case):
    """현재/기존 카메라와 사각형 입력을 명시하며 결과 비교는 계산하지 않는다."""
    camera,rectangle=case[8:10]
    cached=[0,0];current=[0,0]
    if camera==1:current[0]=0x80000000
    elif camera==2:cached[0]=current[0]=0x7fc00001
    elif camera==3:current=[bits(12.5),bits(22.75)]
    elif camera==4:cached[0]=0x7fc00001;current[0]=0x7fc00002
    elif camera==5:cached=[bits(12.5),bits(22.75)];current=cached[:]
    old=[11,22,33,44] if rectangle==2 else [0,0,0,0]
    now=[11,22,33,44] if rectangle else [0,0,0,0]
    return cached,current,old,now


class MetadisplayOracle:
    """실제 queue/refresh/ftol/view 비교를 실행하며 표시 그림 경계만 관찰한다."""
    def __init__(self,edition):
        """읽기 전용 PE·정밀 허용 범위와 bitmap/큐 쓰기 계약을 준비한다."""
        self.edition=edition # 두 원본 판본의 다른 전역·몸체를 고른다.
        self.spec=SPEC[edition] # 이번 몸체와 명시 그림 경계의 실제 주소다.
        self.binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        pe=pefile.PE(str(self.binary));self.mu=Uc(UC_ARCH_X86,UC_MODE_32) # 격리된 에뮬레이터와 PE 메타데이터다.
        self.base=pe.OPTIONAL_HEADER.ImageBase # 허용 코드 범위의 실제 원점이다.
        mapped=pe.get_memory_mapped_image();self.mu.mem_map(self.base,(len(mapped)+4095)&~4095);self.mu.mem_write(self.base,mapped)
        self.mu.mem_map(SCRATCH,0x20000);self.mu.mem_map(0x30000000,0x10000);self.mu.mem_map(STOP,0x1000)
        self.paths=[ROOT/f'extracted/metadisplaydirty/{edition}/{name}' for name in ('creation.c','functions.tsv')] # 정밀 몸체 자료다.
        self.allowed=[];self.entries=set() # 실제 코드 허용 범위·진입 집계 주소다.
        with self.paths[1].open(encoding='utf-8') as stream:
            # 복구한 불연속 실제 함수 범위만 허용한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.native=collections.Counter();self.external=collections.Counter();self.returns=0 # 고유 입력과 구별한 실제 호출·정상 반환 수다.
        self.events=[];self.case=();self.mutated=False # 현재 관찰 사건과 callback 입력 상태다.
        self.mu.hook_add(UC_HOOK_CODE,self.on_instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def read(self,address,width=4):
        """원본 little endian DWORD나 BYTE를 정렬과 무관하게 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력 또는 명시 효과의 주소를 지정한 폭만 갱신한다."""
        self.mu.mem_write(address,(int(value)&((1<<(width*8))-1)).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """실제 body는 bitmap·고유 큐·counter/cache·스택 밖을 쓰지 못한다."""
        p=self.spec
        if 0x30000000<=address and address+size<=0x30010000:return
        ranges=[(BITMAP,BITMAP+16384),(p['cells'],p['cells']+1200),(p['cachedCamera'],p['cachedCamera']+8),
                (p['cachedRectangle'],p['cachedRectangle']+16)]
        ranges.extend((p[key],p[key]+4) for key in ('count','bitmap','width','full','initialized','frames'))
        if not any(low<=address and address+size<=high for low,high in ranges):
            raise RuntimeError(f'미니맵 허용 밖 쓰기 {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """몸체·실제 ftol은 대체하지 않으며 표시 그림/확보/종료 예약만 경계로 관찰한다."""
        p=self.spec
        if address in self.entries:self.native[f'{address:08x}']+=1
        if address in (p['allocate'],p['rebuild'],p['cell'],p['atexit']):
            sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);value=0
            self.external[f'{address:08x}']+=1
            if address==p['allocate']:
                assert self.read(sp+4)==16384;value=BITMAP
            elif address==p['rebuild']:
                assert mu.reg_read(UC_X86_REG_ECX)==DISPLAY;self.events.append((1,0,0))
                if self.case[10]==1:self.put(p['full'],0)
                elif self.case[10]==2:self.put(p['full'],1)
                elif self.case[10]==3:self.put(p['count'],2)
            elif address==p['cell']:
                self.events.append((2,self.read(sp+4),self.read(sp+8)))
                if self.case[10]==4 and not self.mutated:
                    self.mutated=True;count=self.read(p['count']);assert count<100
                    self.mu.mem_write(p['cells']+count*12,struct.pack('<3I',40,41,0));self.put(p['count'],count+1)
            else:assert self.read(sp+4) in (0x442f10,0x442f20)
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'미니맵 허용 밖 코드 {self.edition} {address:08x}')

    def run(self,case,control):
        """현재 queue/view/bitmap 입력을 두고 정상 반환·x87 스택 보존과 결과 전체를 기록한다."""
        self.case=case;self.events=[];self.mutated=False
        kind,present,flags,count,full,bitmap,initialized,frames,camera,rectangle,mutation,xb,yb=case;p=self.spec
        self.mu.mem_write(0x30000000,bytes(0x10000));self.mu.mem_write(BITMAP,bytes([0xa5])*16384)
        self.mu.mem_write(DISPLAY,bytes(16));self.put(DISPLAY+4,flags,1)
        # count 밖의 옛 큐 바이트도 유지하는지 검사한다. 첫 두 후보는 중복 입력에 해당한다.
        for index in range(100):self.mu.mem_write(p['cells']+12*index,struct.pack('<3I',index,index+10,0xa5a5a5a5))
        for key,value in (('count',count),('full',full),('bitmap',BITMAP if bitmap else 0),('width',128 if bitmap else 0),
                          ('display',DISPLAY if present else 0),('initialized',initialized),('frames',frames)):
            self.put(p[key],value)
        old,current,rectOld,rectCurrent=view(case)
        self.mu.mem_write(p['camera'],struct.pack('<2I',*current));self.mu.mem_write(p['cachedCamera'],struct.pack('<2I',*old))
        self.mu.mem_write(p['rectangle'],struct.pack('<4I',*rectCurrent));self.mu.mem_write(p['cachedRectangle'],struct.pack('<4I',*rectOld))
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<3I',STOP,xb,yb))
        self.mu.emu_start(p['queue'] if kind==0 else p['refresh'],STOP,count=500000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        self.returns+=1
        output=[self.read(p[key]) for key in ('count','full','width','initialized','frames')]
        output.extend(struct.unpack('<2I',self.mu.mem_read(p['cachedCamera'],8)));output.extend(struct.unpack('<4I',self.mu.mem_read(p['cachedRectangle'],16)))
        output.extend((fnv(self.mu.mem_read(p['cells'],1200)),fnv(self.mu.mem_read(BITMAP,16384)) if self.read(p['bitmap']) else 0,
                       len(self.events),fnv(b''.join(struct.pack('<3I',*event) for event in self.events))))
        return ','.join(map(str,output))


def generate(smoke=False):
    """원본의 결과만 fixture로 저장하고 두 정밀도의 출력 일치·진입·정상 반환 근거를 기록한다."""
    cases=inputs()[:3] if smoke else inputs();rows=[];editions={};paths=[]
    # 각 원본은 독립 에뮬레이터를 사용한다.
    for edition in SPEC:
        oracle=MetadisplayOracle(edition)
        # 고유 입력마다 두 x87 정밀도를 실행하되 fixture 행은 하나만 만든다.
        for case in cases:
            outputs=[oracle.run(case,control) for control in CONTROLS];assert outputs[0]==outputs[1],(edition,case,outputs)
            rows.append('\t'.join((edition,','.join(map(str,case)),outputs[0])))
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native=dict(oracle.native),external=dict(oracle.external),pe_sha256=sha(oracle.binary),assertions=0)
        paths.extend((oracle.binary,*oracle.paths));print(f'{edition}: {len(cases)}개',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 두 PE의 미니맵 dirty 큐/프레임 갱신. 그림/확보/atexit 경계 명시.\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths.extend((Path(__file__),FIXTURE,ROOT/'tools/ghidra/metadisplaydirty-functions.json',
        ROOT/'cpppj/src/o/RawMetadisplayDirty.h',ROOT/'cpppj/src/o/RawMetadisplayDirty.cpp',ROOT/'cpppj/tests/MetadisplayDirtyTests.cpp'))
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),editions=editions,os_calls=0,gui_calls=0,x87_controls=[hex(value) for value in CONTROLS],
        files={path.relative_to(ROOT).as_posix():sha(path) for path in dict.fromkeys(paths)},limits=[
        '미니맵 bitmap/표시 포인터의 확보·표시 rebuild·실제 셀 그림·CD atexit은 명시 관찰 경계다.',
        'queue/refresh·bitmap 0 채움·현재 view 비교·실제 ftol·전체/부분 순서·callback 뒤 현재 count/full 재조회는 원본 명령이다.',
        'queue의 세 번째 DWORD는 미초기화 스택 잔여값이며 두 실제 셀 몸체가 읽지 않는다. C++은 새 항목에서0으로 정규화한다.',
        '게임/GUI/실제 미니맵 그림·Pop/Unpop 전체 연결·캠페인 완주를 검증하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows),'editions':{key:value['cases'] for key,value in editions.items()}},ensure_ascii=False))


def verify():
    """정밀 입력·행 수·몸체/구현 SHA·정상 반환·명시 경계를 재감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total']==2*len(inputs()) and report['os_calls']==report['gui_calls']==0
    # 기록한 구현/입력/PE/몸체가 현재 바이트와 같은지 검사한다.
    for name,expected in report['files'].items():assert sha(ROOT/name)==expected,name
    # 판본별 모든 고유 입력의 순서·정상 반환·assert0을 검증한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition]
        assert [row[1] for row in chosen]==[','.join(map(str,case)) for case in inputs()]
        assert item['returns']==2*len(chosen) and item['assertions']==0
        assert all(len(row)==3 and len(row[2].split(','))==15 for row in chosen)
    print(f'미니맵 dirty {len(rows)}개 입력/정상 반환/현재 SHA 감사 통과')


# 명시 실행에서만 원본 명령을 격리 실행하거나 이전 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)
