#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 Create 부모의 실제 명령을 격리 실행한다. 할당/cdecl/+28 효과는 명시 입력 경계다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

TYPES=0x12000000  # 원문 전체DWORD TYPE 주소 산술을 관찰하는 격리 표다.
POOL=0x13000000  # 원문 base 생성자가 실제 SID 산술로 접근하는 물리 입력이다.
VT=0x14000000  # 반환 객체와 TYPE 쓰기 중 VT 교체를 구별하는 가상 표 입력이다.
CTOR=SCRATCH+0x1000  # 실제 자식 구현으로 주장하지 않는 cdecl 반환 장치 주소다.
POST=SCRATCH+0x1100  # 실제 게임 효과를 실행하지 않는 가상+28 관찰 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/squid-creation-parent-x86.tsv'  # 최초 원문 반환/추적/바이트를 동결한다.
SPEC={  # 현재 HJOW-Athlon 정밀 산출물에서 확인한 원문 부모와 전역이다.
    'originals':dict(entry=0x4af530,allocate=0x4af1d0,helpers=[0x49a840],types=0x59ab20,stride=500,ctor=0x1d0,pool=0x5c8464,slot=50,fake=0x5412e0,assertion=0x4e0620,base=0x501dd0,post=0x4b0c60),
    'originalCD':dict(entry=0x4ab390,allocate=0x4aae40,helpers=[],types=0x51c960,stride=468,ctor=0x1b0,pool=0x5395dc,slot=36,fake=0,assertion=0x490040,base=0x5058d8,post=0x4ae040)}


def sha(path):
    """도구/보호 PE/고정 입력의 실제 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class CreationOracle(Oracle):
    """부모의 실제 TYPE 읽기/분기/객체 선택/저장/가상 호출을 실행한다."""
    def __init__(self,edition):
        """정밀 부모 범위와 외부 장치용 격리 메모리만 준비한다."""
        self.s=SPEC[edition]  # 현재 판본의 원문 주소와 폭이다.
        super().__init__(edition,[dict(name='create',**{edition:f'{self.s["entry"]:08x}'})],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x40000);self.mu.mem_map(POOL,0x10000);self.mu.mem_map(VT,0x1000)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.events=[];self.profile=0;self.type=0;self.sid=0;self.flags=0  # 각 독립 입력과 원문 추적이다.

    def put(self,address,value):
        """명시 입력을 원문 DWORD 비트로 저장한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """계산 모델 대신 실제 원문 메모리를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little')

    def event(self,value):
        """기계어가 관찰한 장치/필드 선후 관계를 순서대로 보관한다."""
        self.events.append(value)

    def on_read(self,mu,access,address,size,value,data):
        """실제 생성자 멤버와 객체 VT 읽기 시점을 관찰한다."""
        if address==TYPES+self.type*self.s['stride']+self.s['ctor']:
            self.event(f'T{self.type}:{self.get(address)}')
        if POOL<=address<POOL+128*self.s['slot'] and (address-POOL)%self.s['slot']==0 and size==4:
            self.event(f'V{address}:{self.get(address)}')

    def on_write(self,mu,access,address,size,value,data):
        """base VT/TYPE BYTE 저장을 기록하며 명시 프로필의 외부 VT 교체만 공급한다."""
        if POOL<=address<POOL+128*self.s['slot']:
            offset=(address-POOL)%self.s['slot'];obj=address-offset
            if offset==0 and size==4:
                self.event(f'B{obj}:{value}')
            if offset==10 and size==1:
                self.event(f'W{obj}:{value}')
                if self.profile==2:
                    self.put(obj,VT+0x100)

    def resume(self,value):
        """cdecl/thiscall 경계의 실제 반환 주소로 복귀하며 인자 제거는 부모에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);ret=self.get(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EIP,ret)

    def on_instruction(self,mu,address,size,data):
        """할당·cdecl·가상 자식·진단 UI를 결과가 명시된 장치 입력으로만 대체한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s['assertion']:
            expression=self.read_string(self.get(sp+4)).decode('ascii');file=self.read_string(self.get(sp+8)).decode('ascii');line=self.get(sp+12)
            self.event(f'D{expression}|{file}|{line}');self.resume(0);return
        if address==self.s['allocate']:
            self.event(f'A{self.get(sp+4)}')
            if self.profile==1:
                self.put(TYPES+self.type*self.s['stride']+self.s['ctor'],CTOR+0x10)
            self.resume(self.sid);return
        if address in (CTOR,CTOR+0x10):
            self.event(f'C{address}:{self.get(sp+4)}');obj=POOL+self.s['slot']*(9 if self.profile==1 else 5)
            self.put(obj,VT);self.resume(obj);return
        if address in (POST,POST+0x10,self.s['post']):
            obj=mu.reg_read(UC_X86_REG_ECX);table=self.s['base'] if address==self.s['post'] else VT+(0x100 if address==POST+0x10 else 0)
            self.event(f'P{obj}:{table}');self.resume(0);return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """원문 반환 객체/추적/반환 슬롯의 모든 바이트를 독립 기록한다."""
        self.type,self.flags,self.sid,self.profile,base=case;self.events=[]
        self.mu.mem_write(POOL,bytes((i*37+19)&255 for i in range(128*self.s['slot'])))
        self.put(self.s['types'],TYPES);self.put(self.s['pool'],POOL)
        if self.s['fake']:
            self.put(self.s['fake'],162)
        self.put(TYPES+self.type*self.s['stride']+self.s['ctor'],0 if base else CTOR)
        self.put(VT+0x28,POST);self.put(VT+0x128,POST+0x10)
        returned=self.call('create',[self.type,self.flags])
        raw=bytes(self.mu.mem_read(returned,self.s['slot'])).hex()
        return returned,';'.join(self.events),raw


def inputs():
    """출력과 독립인 signed SID/전체 TYPE/진단/할당 후 변경/VT 시점 입력을 교차한다."""
    cases=list(itertools.product((74,162,330),(0,1,2,0x80000001),(0,5,13999,14000,23000,23001,0xffffffff,0x80000000),(0,1,2),(0,)))
    cases.extend((74,flags,sid,0,1) for flags,sid in itertools.product((0,1),(0,5,17)))
    return cases


def main():
    """동결 입력을 최초 한 번 생성하거나 원문 재관측으로 읽기 전용 검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    rows=['# 원문 Create 부모만 실행. 할당/cdecl 반환/+28 효과는 명시 장치 경계이며 자식 구현 검증으로 집계하지 않는다.'];reports={}
    for edition in SPEC:  # 각 보호 PE의 실제 부모를 별도로 실행한다.
        oracle=CreationOracle(edition)
        for case in inputs():  # 같은 입력과 원문 관찰 결과만 동결한다.
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*oracle.observe(case)))))
        reports[edition]=dict(samples=len(inputs()),binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'))
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=reports,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),boundaries=['allocator return/current TYPE mutation','cdecl return object/initial VT','virtual +28 observation','diagnostic UI return','external VT mutation during TYPE store'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['mode'],report['samples'],'Create 부모 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
