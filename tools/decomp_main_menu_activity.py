#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문004cee80의 실제 메인 활동/만료를 명시 시계·팝업·큐·Kill 경계에서 실행한다(700)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ECX

SELF = SCRATCH  # 실제 부모의 격리된120BYTE 메모리 주소다.
TIMES = (44.99999999999999,45.0,45.00000000000001,float('nan'),float('inf'),float('-inf'))  # 만료 경계/NaN/무한대의 실제 입력이다.

def bits(value):
    """double의 원시 비트를 십진 DWORD와 구별해 fixture에 보존한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]

class Oracle(BaseOracle):
    """원문 활동 몸체만 실행하고 외부 효과의 순서/인자를 관측한다."""
    def __init__(self):
        """같은 PC의 정밀 경계와 보호 PE를 읽어 격리 CPU에 적재한다."""
        super().__init__('originals',[dict(name='idle',originals='004cee80')],helpers=[])
        self.popup = 0  # 현재 명시 루트 팝업 helper 결과다.
        self.times = []  # 순서대로 반환할 두 실제 벽시각 입력이다.
        self.trace = []  # 실제로 호출된 helper 순서이며 기대 분기를 합성하지 않는다.
        self.clock_calls = 0  # 첫 재설정/둘째 만료 시각 조회를 구별한다.

    def on_instruction(self,mu,address,size,data):
        """외부 네 helper의 현재 스택과 출력만 공급하고 나머지는 원문에 맡긴다."""
        if address not in (0x4641d0,0x460d70,0x452fe0,0x466480):
            return super().on_instruction(mu,address,size,data)
        esp=mu.reg_read(UC_X86_REG_ESP)
        ret,a,b,c=struct.unpack('<IIII',mu.mem_read(esp,16))
        result=0
        if address == 0x4641d0:
            self.trace.append('p')
            result=self.popup
        elif address == 0x460d70:
            self.trace.append('n')
            mu.mem_write(a,struct.pack('<d',self.times[min(self.clock_calls,1)]))
            self.clock_calls+=1
            result=a
        elif address == 0x452fe0:
            assert a==0x020000c0 and self.read_string(b)==self.read_string(c)==b'DemoStormRages'
            self.trace.append('q')
        else:
            assert mu.reg_read(UC_X86_REG_ECX)==SELF  #실제 call 지점은 ECX에 부모를 넣는 thiscall이며 추가 스택 인자는 없다.
            self.trace.append('k')
        mu.reg_write(UC_X86_REG_EAX,result)
        mu.reg_write(UC_X86_REG_ESP,esp+4)
        mu.reg_write(UC_X86_REG_EIP,ret)

def main():
    """1944개 명시 입력의 원문 필드/외부 호출 순서를 새 자료로만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists(), '이전 기대값/원시는 덮지 않는다'
    oracle=Oracle()
    assert oracle.mu.mem_read(0x540459,1)==b'\x3e'
    assert struct.unpack('<d',oracle.mu.mem_read(0x50e450,8))[0]==45.0
    rows=['# enabled html popup pointerX pointerY modal deadlineBits now1Bits now2Bits outDeadlineBits outX outY trace']
    for enabled,html,popup,pointer,modal,deadline,second in itertools.product((0,1,0xffffffff),(0,1,0xffffffff),(0,1),((0,0),(1,0),(0,-1)),(0,SELF,SELF+4),TIMES,(45.0,90.0)):  # 분기/반환 모델을 쓰지 않고 전체 조합을 실제 CPU에 공급한다.
        oracle.mu.mem_write(SELF,b'\0'*0x120)
        oracle.mu.mem_write(SELF+0x110,struct.pack('<dii',deadline,0,0))
        for address,value in ((0x54db90,enabled),(0x5cab2c,html),(0x565dd4,modal),(0x5cae94,pointer[0]),(0x5cae98,pointer[1])):  # 원문이 직접 읽는 같은 다섯 전역을 매번 명시한다.
            oracle.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        oracle.popup,oracle.times,oracle.trace,oracle.clock_calls=popup,[45.0,second],[],0
        oracle.call('idle',[],this=SELF)
        out,x,y=struct.unpack('<Qii',oracle.mu.mem_read(SELF+0x110,16))
        rows.append('\t'.join(map(str,(enabled,html,popup,pointer[0],pointer[1],0 if modal==0 else 1 if modal==SELF else 2,bits(deadline),bits(45.0),bits(second),out,x,y,''.join(oracle.trace) or '-'))))
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=700,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004cee80'],explicitBoundaries=['004641d0 popup','00460d70 wall clock','00452fe0 queue','00466480 Kill'],mainMenuTypeByte=62,delaySeconds=45,gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
