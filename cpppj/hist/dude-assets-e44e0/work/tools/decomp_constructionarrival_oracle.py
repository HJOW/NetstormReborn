#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 teleportEffect 도착 +0x3c/좌표 절삭/actual finder를 실행한다. 건설시작·contained삭제·자기삭제만 대체한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

# 격리 분석 의존성과 저장소 루트다. 게임/GUI/OS를 실행하지 않는다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW
# 모든 주소는 에뮬레이터 내부의 합성 메모리다.
TYPES,POOL,VTABLE,MAP,STOP,STACK = 0x11000000,0x12000000,0x13000000,0x14000000,0x20000000,0x3003f000
START,REMOVE,DESTROY = STOP+0x100,STOP+0x200,STOP+0x300
FIXTURE = ROOT/'cpppj/tests/fixtures/constructionarrival-x86.tsv'
REPORT = ROOT/'cpppj/constructionarrival.json'
SPECS = {
    'originals':dict(binary='originals/Netstorm.exe',entry=0x444080,stride=50,type_stride=500,extra=40,first=15000,
        types=0x59ab20,pool=0x5c8464,capacity=0x5c847c,grid=0x542508,board=0x531928,begin=0x4b16d0,
        start=0x443fe0,remove=0x4b1a90,checking=0x5e4794,streak=0x5410a0,duration=0x5325c0),
    'originalCD':dict(binary='originalCD/NETSTORM.EXE',entry=0x4d2c80,stride=36,type_stride=468,extra=35,first=6000,
        types=0x51c960,pool=0x5395dc,capacity=0x5395f4,grid=0x5670c0,board=0x52e9a8,begin=0x4eae20,
        start=0x4d06a0,remove=0x4eb310,checking=0x531454,streak=0x51c990,duration=0x53fc38),
}
# 도착 운반 객체의 번호와 두 x87 제어 워드다. 정밀도 재실행은 새 독립 입력으로 더하지 않는다.
BUILDER,CONTROLS = 50,(0x027f,0x037f)


def bits(value):
    """합성 float 입력을 IEEE DWORD로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def sha(path):
    """현재 파일 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases(edition):
    """원본 기대값을 계산하지 않고 후보/현재 전역/효과 뒤 변형 입력을 구성한다."""
    first = SPECS[edition]['first'];result=[]
    # 후보 없는 경로와 자유/죽음/void/contained 상태의 abstract 필터를 구별한다.
    for sid,extra,flags,state in itertools.product((60,first),(0,1,3,9),(0x8000,0x7fff),(0,1,2,4,8)):
        result.append(((bits(20.75),bits(21.9),0,12,bits(0.6),0),[(sid,100,state,extra,bits(20.75),bits(21.9),1,1,flags,1)]))
    result.append(((bits(20.75),bits(21.9),0,12,bits(0.6),0),[]))
    # 서버 경계 포함, 좌표 절삭/각 해시 단계/큰 발자국/범위 밖 후보와 debug 대기를 확인한다.
    for position,level,sid in itertools.product(((1.1,1.9),(20.99999,21.00001),(255.9,254.5)),range(4),(first-1,first,first+1)):
        x,y=map(bits,position);result.append(((x,y,level%2,12,bits(0.6),0),[(sid,100,0,1,x,y,3,3,0x8000,level)]))
    # 같은 칸 여러 후보에서 start가 후행 raw/타입/링크/좌표 또는 현재 duration을 바꾼다.
    for mutation,checking,reverse in itertools.product(range(6),(0,1),(0,1)):
        nodes=[(first+1,100,0,1,bits(20.5),bits(21.25),1,1,0x8000,1),
               (first+2,101,0,1,bits(20.5),bits(21.25),1,1,0x8000,1),
               (60,102,0,1,bits(20.5),bits(21.25),1,1,0x8000,2)]
        if reverse:nodes.reverse()
        result.append(((bits(20.9),bits(21.9),checking,47,bits(0.6),mutation),nodes))
    # 도착 지점과 다른 칸/큰 발자국의 경계 접촉은 actual finder에 맡긴다.
    for offset,extra,flags in itertools.product((0,1,4),(1,0),(0x8000,0)):
        result.append(((bits(20.5),bits(21.5),0,12,bits(0.6),0),[(first,100,0,extra,bits(20+offset),bits(21),3,2,flags,2)]))
    return result


class ArrivalOracle:
    """실제 도착/정수화/finder/타입 몸체만 허용하는 독립 PE 실행기다."""
    def __init__(self,edition):
        """PE/고유 export/합성 pool/type/hash와 명시 효과 진입점을 준비한다."""
        self.edition,self.s=edition,SPECS[edition];s=self.s
        pe=pefile.PE(str(ROOT/s['binary']));self.base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(image)+4095)&~4095);self.mu.mem_write(self.base,image)
        # 전체 원본 SID와 네 버킷 배열을 수용하는 격리 메모리다.
        for address,size in ((TYPES,0x20000),(POOL,0x200000),(VTABLE,0x1000),(MAP,0x40000),(STOP,0x1000),(0x30000000,0x40000)):
            self.mu.mem_map(address,size)
        self.exports=[ROOT/f'extracted/constructionarrival/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries={}
        with self.exports[1].open(encoding='utf-8') as handle:
            # 허용한 실제 함수의 모든 분리 범위와 실제 진입 횟수를 기록한다.
            for row in csv.DictReader(handle,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.bases=[];address=MAP
        # 원본 1/2/4/16 칸 크기의 네 단계 WORD 머리 배열이다.
        for scale in (1,2,4,16):
            self.bases.append(address);address+=(256//scale)**2*2
        self.map_end=address;self.native=collections.Counter();self.boundaries=collections.Counter()
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value,width=4):
        """합성 입력/경계 변형의 little endian DWORD/WORD를 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def slot(self,sid):
        """가상 SID 주소를 현재 판본 stride로 구한다."""
        return POOL+sid*self.s['stride']

    def write(self,mu,access,address,size,value,data):
        """원본 몸체의 쓰기는 도착의 로컬 스택만 허용한다. 합성 자료는 읽기 전용이다."""
        if 0x30000000<=address and address+size<=0x30040000:return
        raise RuntimeError(f'도착 몸체 미허용 쓰기 {address:08x}/{size}')

    def return_stub(self,purge=0):
        """명시 cdecl/thiscall 효과의 정상 반환 규약을 따른다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,ret)

    def instruction(self,mu,address,size,data):
        """하위 효과 세 개만 가로채고 actual finder/float/flags는 그대로 실행한다."""
        s=self.s;esp=mu.reg_read(UC_X86_REG_ESP)
        if address==s['start']:
            sid,duration=struct.unpack('<II',mu.mem_read(esp+4,8));self.events.append(f'S:{sid}:{duration}');self.boundaries['S']+=1
            # 첫 start만 변형하며 after raw/type은 후속 actual finder에서 재조회된다.
            if not self.mutated:
                self.mutated=True;target=s['first']+1
                if sid==target:target=s['first']+2
                if self.mutation==1:self.put(self.slot(target)+s['extra'],0,1)
                elif self.mutation==2:self.put(TYPES+(100 if target==s['first']+1 else 101)*s['type_stride']+0xe8,0)
                elif self.mutation==3:self.put(self.slot(sid)+4,0,2)
                elif self.mutation==4:self.put(self.slot(target)+14,bits(200.0))
                elif self.mutation==5:self.put(s['duration'],bits(0.9))
            self.return_stub();return
        if address==s['remove']:
            sid,type_number,zero,one=struct.unpack('<4I',mu.mem_read(esp+4,16));self.events.append(f'R:{sid}:{type_number}:{zero}:{one}')
            self.boundaries['R']+=1;self.return_stub();return
        if address==DESTROY:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//s['stride'];flags=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            self.events.append(f'K:{sid}:{flags}');self.boundaries['K']+=1;self.return_stub(4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'도착 몸체 미허용 실행 {self.edition} {address:08x}')
        if address in self.entries:self.native[self.entries[address]]+=1
        if address==s['begin']:
            self.area=struct.unpack('<4i',mu.mem_read(esp+4,16))

    def run(self,values,nodes,control):
        """입력 등록 상태에서 실제 +0x3c 두 무시 인자로 호출하고 정상 ret8/x87/readonly 자료를 확인한다."""
        s=self.s;mu=self.mu;self.events=[];self.mutated=False;self.mutation=values[-1]
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256)
        mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases));self.put(VTABLE+0x10,DESTROY)
        self.put(self.slot(BUILDER),VTABLE);self.put(self.slot(BUILDER)+14,values[0]);self.put(self.slot(BUILDER)+18,values[1])
        for name,value in zip(('checking','streak','duration'),values[2:5]):self.put(s[name],value)
        # 입력 순서대로 버킷 머리에 삽입한다. 실제 Pop/삭제 효과를 검증한다는 뜻은 아니다.
        for sid,typ,state,extra,x,y,width,height,flags,level in nodes:
            raw=bytearray(s['stride']);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=state;raw[s['extra']]=extra
            struct.pack_into('<II',raw,14,x,y);address=TYPES+typ*s['type_stride'];self.put(address+0xe8,flags)
            mu.mem_write(address+(0x1d4 if self.edition=='originals' else 0x1b4),struct.pack('<ii',width,height))
            px,py=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (x,y));scale=(1,2,4,16)[level]
            head=self.bases[level]+((py//scale)*(256//scale)+px//scale)*2;struct.pack_into('<H',raw,4,struct.unpack('<H',mu.mem_read(head,2))[0])
            mu.mem_write(self.slot(sid),bytes(raw));self.put(head,sid,2)
        mu.reg_write(UC_X86_REG_ECX,self.slot(BUILDER));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<3I',STOP,0x12345678,0x87654321))
        mu.emu_start(s['entry'],STOP,count=1000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+12:raise RuntimeError('도착 ret8 규약 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control:raise RuntimeError('도착 x87 규약 오류')
        return (mu.reg_read(UC_X86_REG_EAX),','.join(map(str,self.area)),';'.join(self.events) or '-')


def generate():
    """두 정밀도에서 동일한 관찰만 fixture/근거에 기록한다."""
    rows=[];native={};boundaries={};files=set()
    for edition in SPECS:
        oracle=ArrivalOracle(edition)
        for values,nodes in cases(edition):
            observations=[oracle.run(values,nodes,control) for control in CONTROLS]
            assert observations[0]==observations[1],(edition,values)
            result,area,events=observations[0];description=';'.join(','.join(map(str,node)) for node in nodes) or '-'
            rows.append('\t'.join(map(str,(edition,','.join(map(str,values)),description,result,area,events))))
        native[edition]=dict(oracle.native);boundaries[edition]=dict(oracle.boundaries);files.update(oracle.exports);files.add(ROOT/SPECS[edition]['binary'])
        print(f'{edition} 건설 도착 {len(cases(edition))}개 native 대조',flush=True)
    FIXTURE.write_text('# 판본 전역입력 후보입력 반환 정수범위 효과; 기대값은 actual PE만 계산한다.\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/path for path in ('tools/decomp_constructionarrival_oracle.py','tools/ghidra/constructionarrival-functions.json','cpppj/src/o/RawConstructionArrival.h','cpppj/src/o/RawConstructionArrival.cpp'))
    report=dict(host='VM-W11-CODEX',date='2026-10-13',total=len(rows),native=native,boundaries=boundaries,fixture_sha256=sha(FIXTURE),
                files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},
                limits=['actual finder/좌표절삭/flags/두 PE ret8 실행','건설 시작/첫 contained 삭제/가상 자기 삭제만 대체','경로 시작/진행/표시/실제 건설 완료 및 게임GUI 실행 아님'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """보호 PE/몸체/코드/fixture SHA와 입력/효과/정상 반환 코드를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert sha(FIXTURE)==report['fixture_sha256']
    for name,expected in report['files'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total']==sum(len(cases(edition)) for edition in SPECS)
    assert {row[3] for row in rows}=={'1001','1002'}
    for edition in SPECS:
        assert set(report['boundaries'][edition])=={'S','R','K'}
    print(f'건설 도착 SHA/입력/두 반환/효과 감사 {len(rows)}개 통과')


def main():
    """생성 또는 저장 근거의 재감사를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    verify() if options.verify else generate()


if __name__=='__main__':main()
