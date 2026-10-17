#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""마나 볼트105의 실제 검색/도착 함수를 현재 보호 PE에서 격리 실행한다."""
import argparse
import bisect
import collections
import csv
import itertools
import json
import struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from decomp_constructionarrival_oracle import ArrivalOracle,ROOT,TYPES,POOL,VTABLE,MAP,STOP,STACK,DESTROY,SPECS,bits,sha

# 외부 효과의 실제 함수 진입점이며 검색/조건 계산을 대체하지 않는다.
SOUND,REMOVE,ALLOC,ATTACH=0x4a9d70,0x4b27a0,0x4e4391,0x496e80
FIXTURE=ROOT/'cpppj/tests/fixtures/mana-bolt-arrival-x86.tsv' # 기존 건설 도착 자료와 분리한다.
REPORT=ROOT/'cpppj/mana-bolt-arrival.json' # 현재 소스/보호 PE/관측 SHA와 실행 범위를 보존한다.

class Oracle(ArrivalOracle):
    """실제 Finder/좌표 절삭/후속 재조회와 두 정밀도를 관찰한다."""
    def __init__(self):
        """현재 PC의 정밀 함수 범위를 쓰며 가상 메모리만 할당한다."""
        self.edition='originals';self.s=SPECS['originals'];self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        pe=pefile.PE(str(ROOT/self.s['binary']));image=pe.get_memory_mapped_image();self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # 실제 게임 프로세스와 연결되지 않는 합성 입력 영역이다.
        for address,size in ((TYPES,0x20000),(POOL,0x200000),(VTABLE,0x1000),(MAP,0x40000),(STOP,0x1000),(0x30000000,0x40000),(0,0x1000)):self.mu.mem_map(address,size)
        self.ranges=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            # 원본 실행 바이트 범위만 허용한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.ranges.append((low,high+1))
        self.ranges.sort();self.starts=[a for a,b in self.ranges];self.bases=[];address=MAP
        # 원본 네 공간 해시의 실제 폭이다.
        for scale in (1,2,4,16):self.bases.append(address);address+=(256//scale)**2*2
        self.map_end=address;self.instructions=0;self.calls=collections.Counter();self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """원본 SEH의 FS:0 연결/복구와 기존 로컬 스택만 쓰기를 허용한다."""
        if address==0 and size==4:return
        return ArrivalOracle.write(self,mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """외부 소리/Host/할당을 기록하고 검색은 보호 PE가 실행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP)
        if address in (SOUND,REMOVE,ALLOC,ATTACH,DESTROY):
            self.calls[hex(address)]+=1
            if address==SOUND:self.events.append('S:'+':'.join(str(v) for v in struct.unpack('<II',mu.mem_read(sp+4,8))))
            elif address==REMOVE:self.events.append('R:'+':'.join(str(v) for v in struct.unpack('<II',mu.mem_read(sp+4,8))))
            elif address==ALLOC:
                assert int.from_bytes(mu.mem_read(sp+4,4),'little')==0x28;self.events.append('N:'+str(self.allocate));mu.reg_write(UC_X86_REG_EAX,VTABLE+0x800 if self.allocate else 0)
            elif address==ATTACH:
                event,parent,delay=struct.unpack('<III',mu.mem_read(sp+4,12));self.events.append(f'A:{event}:{parent}:{delay}')
            else:self.events.append('K:50:0')
            self.return_stub(12 if address==ATTACH else 4 if address==DESTROY else 0);return
        index=bisect.bisect_right(self.starts,address)-1
        if index<0 or address+size>self.ranges[index][1]:raise RuntimeError(f'미허용 실행 {address:08x}')

    def run_case(self,extra,state,owner,allocate,control,present=True):
        """기대값 없이 단일 후보와 원본 두 함수를 입력으로 호출한다."""
        mu=self.mu;s=self.s;self.events=[];self.allocate=allocate
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256);mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases))
        self.put(VTABLE+0x10,DESTROY);self.put(self.slot(50),VTABLE);self.put(self.slot(50)+14,bits(20.75));self.put(self.slot(50)+18,bits(21.9));self.put(self.slot(50)+34,1,1)
        if present:
            self.put(self.slot(60)+10,100,1);self.put(self.slot(60)+11,state,1);self.put(self.slot(60)+40,extra,1);self.put(self.slot(60)+34,owner,1)
            self.put(self.slot(60)+14,bits(20.5));self.put(self.slot(60)+18,bits(21.5));self.put(TYPES+100*500+0x1d4,1);self.put(TYPES+100*500+0x1d8,1);self.put(self.bases[1]+((21//2)*128+20//2)*2,60,2)
        self.put(0x541128,46);self.put(0x541164,61)
        results=[]
        # 검색의 실제 반환 SID와 도착의 반환/호출 순서를 각각 수집한다.
        for entry,args in ((0x45aab0,()),(0x45ab30,(0x12345678,0x87654321))):
            mu.reg_write(UC_X86_REG_ECX,self.slot(50));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=1000000)
            assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4
            assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800);results.append(mu.reg_read(UC_X86_REG_EAX))
        return results[0],results[1],';'.join(self.events)

def generate():
    """동일 입력의53/64비트 관측이 같을 때만 fixture로 저장한다."""
    oracle=Oracle();rows=[]
    # abstract/preview/dead/free/void와 소유권/할당 실패를 교차한다.
    for extra,state,owner,allocate in itertools.product((0,1,3,9,0x21),(0,1,2,4,8),(0,1),(0,1)):
        a=oracle.run_case(extra,state,owner,allocate,0x027f);b=oracle.run_case(extra,state,owner,allocate,0x037f);assert a==b;rows.append([extra,state,owner,allocate,1,*a])
    a=oracle.run_case(0,0,1,1,0x027f,False);b=oracle.run_case(0,0,1,1,0x037f,False);assert a==b;rows.append([0,0,1,1,0,*a])
    FIXTURE.write_text('# extra state owner allocate present target return events\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=['tools/decomp_mana_bolt_arrival.py','tools/decomp_constructionarrival_oracle.py','cpppj/src/o/RawManaBoltArrival.cpp','cpppj/src/o/RawManaBoltArrival.h','cpppj/tests/ConstructionArrivalTests.cpp','extracted/refined/originals/functions.tsv','originals/Netstorm.exe',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompiled_host='HJOW-Athlon / 2026-10-16',cases=len(rows),returns=len(rows)*4,precision_differences=0,instructions=oracle.instructions,external_calls=dict(oracle.calls),source_sha256={p:sha(ROOT/p) for p in files}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'마나 볼트 검색/도착 {len(rows)}입력, {len(rows)*4}정상 반환, {oracle.instructions}명령, 정밀도 차이0')

def verify():
    """현재 관측의 모든 원본/제품/fixture SHA를 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,expected in report['source_sha256'].items():assert sha(ROOT/path)==expected,path
    print('마나 볼트 도착 SHA 확인')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
