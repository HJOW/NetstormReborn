#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제panic 전체 제어·임시 속도·원본 난수 소비를 보호 PE에서 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle,ROOT,HEAP,TYPES,STACK
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

POOL=0x15000000  # 기존 격리 메모리와 구분한 두50BYTE 슬롯의 풀이다.
ACTOR=POOL+50*50  # 명시 사제 입력 SID50이다.
TEMPLE=POOL+60*50  # 명시 현재 섬 신전 입력 SID60이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priest-panic-x86.tsv'  # 새원문의 입력/효과/슬롯/속도/난수 결과다.
REPORT=ROOT/'cpppj/priest-panic-20261023.json'  # 원문 범위·호스트·두정밀도·SHA 근거다.
ENTRIES=(0x491db0,0x455930,0x4e49c0)  # 전체 제어/실제float 난수/실제CRT절삭을 실행한다.


def bits(value):
    """결과를 계산하지 않고 입력float의 물리 비트만 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """신전/HP·owner/소수 속도·성공·영역/현재 자료 변화를 독립 교차한다."""
    rows=list(itertools.product((0,1),(0,100,101),(0,1),(bits(.75),bits(1.5),bits(-.75)),(0,0x80000000),(0,1,2),(0,0xdeadbeef),(0,), (0,)))
    for mutation,mode,result,speed in itertools.product(range(1,8),range(4),(0,0x80000000),(bits(.75),bits(3.99),bits(-.75))):
        rows.append((1,0,1,speed,result,mode,0xdeadbeef,0x12345678,mutation))  # 같은 원문 callback 뒤 재조회/옛TYPE복원/두좌표 snapshot이다.
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """실제root/난수/CRT를 실행하며 현재 지형/최대HP·경로 시작만 큰 경계로 둔다."""
    def __init__(self):
        """정확한 불연속 함수 범위와 격리 읽기/쓰기 가드를 준비한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.returns=0

    def number(self,address,width=4):
        """현재 비정렬 DWORD/BYTE를 원본 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """원본/SEH/명시 두슬롯·TYPE/스택 밖 읽기를 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'panic 읽기 범위 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """원본난수DWORD/두속도/스택 외 emulated 쓰기를 거부한다."""
        spans=((0,4),(0x532710,0x532714),(TYPES+158*500+0xe0,TYPES+158*500+0xe4),(TYPES+159*500+0xe0,TYPES+159*500+0xe4),(STACK-0x7f000,STACK+0x1000))
        if any(lo<=address and address+size<=hi for lo,hi in spans):return
        raise RuntimeError(f'panic 쓰기 범위 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """경계 입력을 공급하며 원문의 HP/owner/임시속도·후보 제어는 그대로 실행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=[self.number(sp+4+i*4) for i in range(8)];c=self.values;m=c[8]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x46f950:
            self.event('T',*args[:2])
            if m==1:self.put(ACTOR+14,bits(51.25));self.put(ACTOR+26,20);self.put(ACTOR+34,2,1);self.put(TEMPLE+34,2,1)
            self.ret(60 if c[0] else 0,8);return
        if address==0x4adcb0:
            self.event('H',50)
            if m==2:self.put(ACTOR+10,159,1);self.put(ACTOR+34,2,1);self.put(TEMPLE+34,2,1)
            self.ret(100);return
        if address==0x4ac550:self.ret(TYPES+self.number(ACTOR+10,1)*500);return
        if address==0x4894e0:
            self.event('V',*args[:4],self.number(TYPES+self.number(ACTOR+10,1)*500+0xe0))
            if m==3:self.put(ACTOR+10,159,1);self.put(ACTOR+18,bits(52.5));self.put(TYPES+158*500+0xe0,bits(9.25))
            self.ret(c[4]);return
        if address==0x46f6e0:
            self.event('L',*args[:2]);self.landCalls+=1
            if self.landCalls==1 and m==4:self.put(ACTOR+14,bits(61.25));self.put(ACTOR+18,bits(62.5))
            value=0 if c[5]==0 or (c[5]==1 and self.landCalls in (2,3)) else 1
            self.ret(value,8);return
        if address==0x46f530:
            self.event('G',*args[:2]);self.regionCalls+=1
            if m==6 and self.regionCalls==1:self.put(ACTOR+14,bits(81.25));self.put(ACTOR+18,bits(82.5))
            value=2 if c[5]==3 and self.regionCalls in (1,3,5) else 1
            self.ret(value,8);return
        if address==0x455930:
            self.event('R',*args[:2]);self.randomCalls+=1
            if m==5 and self.randomCalls==1:self.put(ACTOR+14,bits(71.25));self.put(ACTOR+18,bits(72.5))
        if address==0x489080:
            self.event('P',*args)
            if m==7:self.put(ACTOR+34,3,1)
            self.ret(c[6]);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'panic 미검토 명령 {address:x}')

    def sample(self,row,control):
        """독립 입력을 설치하고 원문이 만든 슬롯/속도/난수 상태와 순서를 반환한다."""
        self.values=row;self.events=[];self.landCalls=self.regionCalls=self.randomCalls=0
        self.mu.mem_write(ACTOR,bytes(50));self.mu.mem_write(TEMPLE,bytes(50));self.put(ACTOR+10,158,1);self.put(ACTOR+26,row[1]);self.put(ACTOR+34,1,1);self.put(TEMPLE+34,1 if row[2] else 2,1);self.put(ACTOR+14,bits(50.25));self.put(ACTOR+18,bits(50.75));self.put(0x5c8464,POOL);self.put(0x532710,row[7])
        self.put(TYPES+158*500+0xe0,row[3]);self.put(TYPES+159*500+0xe0,bits(2.5))
        self.call(0x491db0,ACTOR,b'',0,control);self.returns+=1
        assert self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        return ';'.join(self.events),bytes(self.mu.mem_read(ACTOR,50)).hex()+bytes(self.mu.mem_read(TEMPLE,50)).hex(),str(self.number(TYPES+158*500+0xe0))+','+str(self.number(TYPES+159*500+0xe0)),str(self.number(0x532710))


def main():
    """새 원문을 동결하거나 현재 원문/구현SHA만 읽기 전용으로 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for key,value in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/key)==value,key  # C++출력으로 fixture를 바꾸지 않는다.
        print('사제 panic 원문/SHA 확인');return
    first,second=Oracle(),Oracle();rows=[];different=[]
    for values in cases():
        a=first.sample(values,0x27f);b=second.sample(values,0x37f)
        if a!=b:different.append(','.join(map(str,values)))
        rows.append((','.join(map(str,values)),*a))  # 원본 CRT 기본53비트가 제품 대조 기준이다.
    FIXTURE.write_text('# input effects objects100 speedbits seed\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priest_panic.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,nativeCalls=dict(first.calls+second.calls),precisionDifferentInputs=different,sha256={key:digest(ROOT/key) for key in paths},limitations=['491db0/455930/CRT절삭 전체 명령이다. 지형·현재 최대HP/TYPE 포인터·경로 시작은 명시 큰 경계다.','원본 CRT 기본PC53을 제품 기준으로 사용한다. GUI/전체25종·미션·Edit 완료 근거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('panic 원문',len(rows),'정상반환',facts['normalReturns'],'정밀도차이',len(different))


if __name__=='__main__':main()
