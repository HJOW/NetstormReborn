#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 기도 명령4940b0/수신493f80/605 분기494580의 실제 호출·패킷·현재 슬롯을 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STOP,HEAP,TYPES,STACK,VTABLE
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

POOL=0x15000000  # 현재50BYTE 슬롯을 별도 물리 영역에 둔다.
ACTOR=POOL+60*50  # 모든 명시 입력이 가리키는 동일한 사제 슬롯이다.
EVENT,PACKET,OUTPUT=HEAP+0x200,HEAP+0x300,HEAP+0x400  # 입력 사건/수신 패킷/float 반환이다.
WRAPPER=STOP+0x900  # 원본 x87 반환을 float32로 저장하는 실제 명령 구간이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priest-prayer-x86.tsv'  # 기존 사제 fixture와 분리한 새 근거다.
REPORT=ROOT/'cpppj/priest-prayer-20261023.json'  # 원본·실행·호스트 및 SHA를 기록한다.


def cases():
    """정확한605/의식 방향/상태/권한/할당/정지/내용물/일치 횟수를 교차한다."""
    base=(0,0,0,1,1,0,0,0,0,5,0x3f800000,0x030000de,168,0)
    result=[]
    def add(mode,**changes):
        """명시 입력만 바꾸며 원본 기대 결과는 계산하지 않는다."""
        row=list(base)
        for index,value in changes.items():row[int(index)]=value  # 필드 번호를 가진 명시 변화만 적용한다.
        result.append((mode,*row))
    for exists,direction,code in itertools.product((0,1),(-1,0,9,10,14,15),(0x030000de,0x830000de,0x03000103,0x12345678)):
        add('C',**{'1':exists,'2':direction,'11':code})  # 메뉴 명령과 상위 무시 비트·부모 위임이다.
    for state,exists,direction,boss,allocation in itertools.product(range(8),(0,1),(-1,0,9,10,14,15),(0,1),(0,1)):
        add('R',**{'0':state,'1':exists,'2':direction,'3':boss,'4':allocation})  # free/dead와void를 구별하며 실패 할당 뒤 의식 호출도 관찰한다.
    for count,pray,movement,immobile,boss,content in itertools.product((0,1,4,5,6,60,0xffffffff,0x80000000),(-1,0,1,5,60),(0,1),(0,1),(0,1),(0,1)):
        add('T',**{'3':boss,'5':movement,'6':immobile,'7':content,'8':count,'9':pray})  # >=가 아닌 DWORD/정수 일치와 정지·이동 반환이다.
    for mode,mutation,payload in itertools.product(('C','R','T'),range(1,7),(0x3f800000,0x3e800000,0x7fc01234)):
        add(mode,**{'8':5,'9':5,'10':payload,'13':mutation})  # 큰 호출 뒤 current TYPE/기한 종류/권한 재조회를 검사한다.
    return list(dict.fromkeys(result))


class Oracle(BaseOracle):
    """명령/수신/진행 전체 root와 실제6d 헤더를 실행하고 큰 하위만 경계로 둔다."""
    def __init__(self):
        """원본 코드 범위와 격리 풀·x87 반환 명령을 준비한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,(0x4940b0,0x493f80,0x494580,0x40afd0,0x4df230));self.returns=0
        code=b''.join(b'\xff\x35'+struct.pack('<I',OUTPUT+0x20+i*4) for i in (2,1,0));at=WRAPPER+len(code);code+=b'\xe8'+struct.pack('<i',0x494580-at-5)+b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';self.mu.mem_write(WRAPPER,code);self.wrapperEnd=WRAPPER+len(code)

    def number(self,address,width=4):
        """현재 little endian 필드와 float 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """원본/격리SEH/풀/명시 자료/스택 밖 읽기는 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(0x400000,0x600000),(POOL,POOL+0x200000),(TYPES,TYPES+0x20000),(HEAP,HEAP+0x10000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'기도 읽기 범위 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """원본이 직접 바꾸는 물리 슬롯/헤더/스택/격리SEH만 허용한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'기도 쓰기 범위 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """실제 제어 흐름과 필드 재조회는 실행하고 큰 호출/요청의 순서를 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=[self.number(sp+4+i*4) for i in range(6)];sid=(ecx-POOL)//50;c=self.values;mutation=c[13]
        if address in (0x4940b0,0x493f80,0x494580):self.calls[f'{address:08x}']+=1
        if address==0x4afb40:
            self.event('E',args[0]);self.ret(HEAP+0x800 if c[1] else 0,4);return
        if address==0x4ad710:self.event('D',sid);self.ret(c[2]);return
        if address==0x488fe0:self.event('M',args[0]);self.ret(54);return
        if address==0x4b1a90:
            self.event('L',*args[:4])
            if mutation==2:self.put(ACTOR+42,113)
            self.ret(77 if c[5] else 0);return
        if address in (0x4e2dc0,0x4e2e30):
            pointer=args[1] if address==0x4e2dc0 else args[0];self.events.append(('P:'+str(args[0])+':' if address==0x4e2dc0 else 'B:')+bytes(mu.mem_read(pointer,8)).hex())
            if mutation==3:self.put(0x540bc4,0);self.put(ACTOR+10,159,1)
            self.ret();return
        if address==0x4e4391:self.event('A',args[0]);self.ret(HEAP+0x900 if c[4] else 0);return
        if address==0x496f00:self.event('R',*args[:3]);self.ret(ecx,12);return
        if address==0x491cf0:self.event('S',*args[:2]);self.ret();return
        if address==0x427030:
            self.event('I',sid)
            if mutation==4:self.put(0x540bc4,0);self.put(ACTOR+42,113)
            self.ret(c[6]);return
        if address==0x49a840:self.ret(TYPES+args[0]*500);return
        if address==0x4acac0:
            self.event('H',sid,*args[:3])
            if mutation==5:self.put(0x540bc4,0)
            self.ret(168 if c[7] else 0,12);return
        if address==0x4264d0:
            self.event('Q',args[0])
            if mutation==6:self.put(ACTOR+42,113);self.put(ACTOR+10,159,1)
            self.ret();return
        if address==0x4afa70:self.event('C',sid,*args[:5]);self.ret(123,20);return
        if address==0x4ac550:self.ret(TYPES+self.number(ecx+10,1)*500);return
        if address==0x4acee0:self.event('F',sid,*args[:2]);self.put(ACTOR+36,args[0]);self.ret(purge=8);return
        if address==0x491db0:self.event('V',sid);self.ret();return
        if address==0x42aa50:self.event('X',sid,self.number(args[0]),self.number(args[0]+8));self.ret(0x12345678,4);return
        if WRAPPER<=address<self.wrapperEnd:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'기도 미검토 명령 {address:x}')

    def sample(self,row,control):
        """원본 기대값을 만들지 않고 입력 슬롯/패킷/TYPE 속성만 설치한다."""
        self.mode,*self.values=row;c=self.values;self.events=[];self.mu.mem_write(ACTOR,bytes(50));self.put(ACTOR,VTABLE);self.put(ACTOR+10,158,1);self.put(ACTOR+11,c[0],1);self.put(ACTOR+36,7);self.put(ACTOR+42,c[12]);self.put(ACTOR+46,0xaabbccdd)
        for a,v in ((0x5c8464,POOL),(0x540bc4,c[3]),(0x5412f8,168),(0x542a6c,0x030000de),(0x542b00,0x03000103)):self.put(a,v)  # 현재 공유 전역만 공급한다.
        self.mu.mem_write(EVENT,bytes(16));self.put(EVENT,c[11]);self.put(EVENT+8,(c[12]<<16)|60);self.mu.mem_write(PACKET,struct.pack('<BBHHH',1,0x6d,8,60,c[12]));self.put(TYPES+158*500+0x118,7);self.put(TYPES+159*500+0x118,9)
        for kind in (113,168):self.put(TYPES+kind*500+0x1bc,c[9])  # 실제 타입 정수 praytime 입력이다.
        if self.mode=='C':self.call(0x4940b0,ACTOR,struct.pack('<I',EVENT),4,control)
        elif self.mode=='R':self.call(0x493f80,0,struct.pack('<III',9,0,PACKET),0,control)
        else:
            for index,value in enumerate((605,c[8],c[10])):self.put(OUTPUT+0x20+index*4,value)  # x87 반환을 실제 fstp로 저장한다.
            self.call(WRAPPER,ACTOR,b'',0,control)
        self.returns+=1;result=self.number(OUTPUT) if self.mode=='T' else self.mu.reg_read(UC_X86_REG_EAX) if self.mode=='C' else 0
        return ';'.join(self.events) or '-',bytes(self.mu.mem_read(ACTOR,50)).hex(),str(result),str(self.number(0x540bc4))


def main():
    """독립 원문 생성 또는 기존 결과의 읽기 전용 SHA 검증을 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # C++로 원문을 덮어쓰지 않는다.
        assert len([s for s in FIXTURE.read_text(encoding='utf-8').splitlines() if s and not s.startswith('#')])==facts['uniqueInputs'];print('사제 기도 원문/SHA 확인');return
    first,second=Oracle(),Oracle();rows=[]
    for row in cases():  # 두 정밀도에서 호출/전체 슬롯/반환 비트가 같아야 한다.
        a=first.sample(row,0x27f);b=second.sample(row,0x37f);assert a==b,row;rows.append((','.join(map(str,row)),*a))
    FIXTURE.write_text('# input effects raw60 resultBits boss\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n');paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priest_prayer.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix());facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,nativeCalls=dict(first.calls+second.calls),precisionDifferentInputs=[],entries=['004940b0','00493f80','00494580(event605)'],packet={'type':'6d','version':1,'length':8},sha256={p:digest(ROOT/p) for p in paths},limitations=['명령/수신 전체 root와494580의605 분기 및 실제6d 헤더를 실행한다. 큰 조회/이동제거/내용물/의식/송신/공유예약은 명시 경계다.','다른 사제 사건 및 panic491db0 전체나117/118 주문 효과 완료의 증거가 아니다.']);REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('사제 기도:',len(rows),facts['normalReturns'],facts['nativeInstructions'])


if __name__=='__main__':main()
