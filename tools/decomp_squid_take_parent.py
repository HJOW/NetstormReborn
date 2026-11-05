#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 순수 Take 부모를 격리 관측한다. 가상 자식은 명시 장치이며 게임/수신/통신은 실행하지 않는다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

TYPES,POOL,VT=0x12000000,0x13000000,0x14000000  # 호스트 주소와 무관한 TYPE/객체/가상 표 물리 입력이다.
CTOR,UNPOP,POST=SCRATCH+0x1000,SCRATCH+0x1100,SCRATCH+0x1200  # 실제 자식으로 주장하지 않는 명시 호출 장치다.
FIXTURE=ROOT/'cpppj/tests/fixtures/squid-take-parent-current-x86.tsv'  # 최초 순서/반환 객체/바이트만 동결한다.
SPEC={  # 현재 정밀 함수와 보호 PE에서 확인한 실제 주소다.
    'originals':dict(entry=0x4af610,helpers=[0x49a840],types=0x59ab20,stride=500,ctor=0x1d0,pool=0x5c8464,capacity=0x5c847c,slot=50,assertion=0x4e0620,format=0x4a89c0,detail=0x436d20,base=0x501dd0,post=0x4ad4a0),
    'originalCD':dict(entry=0x4ab440,helpers=[],types=0x51c960,stride=468,ctor=0x1b0,pool=0x5395dc,capacity=0x5395f4,slot=36,assertion=0x490040,format=0x420560,detail=0x4875b0,base=0x5058d8,post=0x4ae120)}


def sha(path):
    """보호 원본/현재 도구/동결 관측의 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class TakeOracle(Oracle):
    """허용 부모 명령과 명시 가상/생성자/진단 경계만 실행한다."""
    def __init__(self,edition):
        """현재 refined 범위와 분리된 순수 입력 메모리를 준비한다."""
        self.s=SPEC[edition]  # 판본별 함수/전역/폭이다.
        super().__init__(edition,[dict(name='take',**{edition:f'{self.s["entry"]:08x}'})],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x40000);self.mu.mem_map(POOL,0x10000);self.mu.mem_map(VT,0x1000);self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.events=[];self.number=74;self.sid=5;self.profile=0;self.base=False  # 각 입력/관측과 외부 변경 프로필이다.

    def put(self,address,value,width=4):
        """명시 자식 효과를 원문 정수 폭으로만 공급한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self,address,width=4):
        """계산 모델 없이 같은 메모리의 현재 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_read(self,mu,access,address,size,value,data):
        """원문 부모의 관문/상태/TYPE/VT/현재 생성자 읽기를 순서대로 관측한다."""
        if address==self.s['capacity']:
            self.events.append('Q'+str(self.get(address)))
        if address==TYPES+self.number*self.s['stride']+self.s['ctor']:
            self.events.append(f'C{self.number}:{self.get(address)}')
        for index in (5,9):  # 관측 대상 두 물리 객체만 읽기 장치와 대응한다.
            obj=POOL+index*self.s['slot'];offset=address-obj
            if (offset,size) in ((0,4),(10,1),(11,1)):
                self.events.append(f'{ {0:"V",10:"T",11:"R"}[offset]}{obj}:{self.get(address,size)}')

    def on_write(self,mu,access,address,size,value,data):
        """부모의 VT/TYPE/상태 쓰기와 명시 TYPE 쓰기 중 외부 변경만 관측한다."""
        for index in (5,9):  # 실제 반환 객체와 원래 객체를 혼동하지 않는다.
            obj=POOL+index*self.s['slot'];offset=address-obj
            if (offset,size) in ((0,4),(10,1),(11,1)):
                self.events.append(f'{ {0:"B",10:"Y",11:"W"}[offset]}{obj}:{value}')
            if offset==10 and size==1 and self.profile==2:
                self.put(obj,VT+0x100);self.put(obj+11,0x80,1)

    def resume(self,value,purge=0):
        """실제 반환 주소와 자식의 ret N만 적용한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,self.get(sp))

    def on_instruction(self,mu,address,size,data):
        """상세/진단·Unpop·cdecl·PostTake의 명시 정상 복귀만 공급한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s['assertion']:
            expression=self.read_string(self.get(sp+4)).decode('ascii');file=self.read_string(self.get(sp+8)).decode('ascii');self.events.append(f'D{expression}|{file}|{self.get(sp+12)}');self.resume(0);return
        if address==self.s['format']:
            server=(self.get(sp+12)-TYPES-4)//self.s['stride'];client=(self.get(sp+16)-TYPES-4)//self.s['stride'];self.events.append(f'F{server}:{client}');self.resume(SCRATCH+0x3000);return
        if address==self.s['detail']:
            self.events.append('L');self.resume(0);return
        if address==UNPOP:
            obj=mu.reg_read(UC_X86_REG_ECX);assert self.get(sp+4)==0;self.events.append(f'U{obj}:{VT}')
            if self.profile==1:
                self.put(obj+11,0x8f,1);self.put(TYPES+self.number*self.s['stride']+self.s['ctor'],0 if self.base else CTOR+0x10)
            self.resume(0,4);return
        if address in (CTOR,CTOR+0x10):
            self.events.append(f'A{address}:{self.get(sp+4)}');obj=POOL+(9 if self.profile==1 else 5)*self.s['slot'];self.put(obj,VT);self.resume(obj);return
        if address in (POST,POST+0x10,self.s['post']):
            obj=mu.reg_read(UC_X86_REG_ECX);table=self.s['base'] if address==self.s['post'] else VT+(0x100 if address==POST+0x10 else 0);self.events.append(f'P{obj}:{table}');self.resume(0);return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """각 입력의 명시 초기 상태 뒤 실제 부모 반환/trace/두 슬롯을 기록한다."""
        self.number,self.sid,state,self.profile,self.base=case;self.events=[]
        self.mu.mem_write(POOL,bytes((i*37+19)&255 for i in range(128*self.s['slot'])))
        self.put(self.s['pool'],POOL+(5-self.sid)*self.s['slot']);self.put(self.s['capacity'],32768);self.put(self.s['types'],TYPES)
        self.put(TYPES+self.number*self.s['stride']+self.s['ctor'],0 if self.base else CTOR);self.put(VT+0x48,UNPOP);self.put(VT+0x2c,POST);self.put(VT+0x12c,POST+0x10)
        for index in (5,9):  # C++ 장치도 동일 두 객체의 raw 입력만 공급한다.
            obj=POOL+index*self.s['slot'];self.put(obj,VT);self.put(obj+10,74,1);self.put(obj+11,state,1)
        result=self.call('take',[self.number,self.sid]);slots=';'.join(bytes(self.mu.mem_read(POOL+i*self.s['slot'],self.s['slot'])).hex() for i in (5,9))
        return result,';'.join(self.events),slots


def inputs():
    """정상/진단 SID·TYPE DWORD와 free/dead/void/contained·반환 장치 입력을 교차한다."""
    return list(itertools.product((74,330),(5,6000,15000,32768,0xffffffff),(0,1,4,7,8,13),(0,1,2),(0,1)))


def main():
    """최초 관측만 새 파일로 동결하며 재관측은 기존 기대값을 읽기만 한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();rows=['# 순수 Take 부모 명령. 진단/상세/가상Unpop/cdecl반환/+2c와 TYPE쓰기중외부변경은명시장치. 게임/MP/패킷미실행.'];editions={}
    for edition in SPEC:  # 두 보호 PE에서 같은 입력의 실제 읽기/쓰기를 각각 관측한다.
        oracle=TakeOracle(edition)
        for case in inputs():  # 원문 반환/trace/두 물리 슬롯 전체만 동결한다.
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*oracle.observe(case)))))
        editions[edition]=dict(samples=len(inputs()),binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'))
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=editions,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),actualVirtualChildren=False,actualConstructor=False,game=False,multiplayer=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(report['mode'],report['samples'],'Take 부모 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
