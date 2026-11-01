#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004dea70의 설정/현재 사건/실제 입력 큐/카운터 순서를 격리 관측한다(712)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

CURRENT,REFRESH,CHANGED=0x5435b4,0x5c85d8,0x54298c  #원문의 서로 다른 현재 번호/공유 카운터/알림 명령이다.


class Oracle(BaseOracle):
    """요청과 실제40칸 큐 writer를 실행하고 동기 Config만 명시 장치로 둔다."""
    def __init__(self):
        """현재 보호 PE와 같은 PC의 정밀 함수 범위를 읽는다."""
        super().__init__('originals',[dict(name='request',originals='004dea70')],[0x452fe0,0x452f90]);self.mutation=0;self.requested=0;self.trace=[]  #독립 callback 효과와 관측 순서다.

    def get(self,address):
        """격리 DWORD의 현재 비트를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def put(self,address,value):
        """격리 DWORD의 하위32비트를 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def on_instruction(self,mu,address,size,data):
        """Config 경계의 독립 효과와 실제 큐 진입의 시점을 기록하고 나머지 본체를 실행한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c=struct.unpack('<4I',mu.mem_read(sp,16))
        if address==0x4414d0:
            assert self.read_string(a)==b'currentLanguage';self.trace.append('S,'+self.read_string(b).hex()+','+str(self.get(REFRESH)))
            if self.mutation&1: self.put(CURRENT,self.requested);self.put(REFRESH,100);self.put(CHANGED,0xa2001234)
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==0x452fe0:
            self.trace.append(f'Q,{a},{b},{c},{self.get(REFRESH)}')
            if self.mutation&2: self.put(REFRESH,0xffffffff)
        return super().on_instruction(mu,address,size,data)


def main():
    """각 입력의 실제 결과/12BYTE 사건을 최초 fixture에만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists(),'이전 관측 보존'
    native=Oracle();rows=['# requested current refresh mutation currentAfter refreshAfter queueHex trace'];cases=list(itertools.product(range(7),(0xffffffff,0,1,2,3,6),(0,0xffffffff),range(4)));cases.append((0xffffffff,0xffffffff,7,3))  #같은 미정 번호는 잘못된 문자열 표 주소를 읽지 않고 반환한다.
    for requested,current,refresh,mutation in cases:  #모든 callback 효과는 기대값 없이 입력으로만 공급한다.
        native.requested=requested;native.mutation=mutation;native.trace=[];native.put(CURRENT,current);native.put(REFRESH,refresh);native.put(CHANGED,0x020000a6);native.put(0x55a508,0);native.put(0x55a50c,0);native.mu.mem_write(0x55a510,bytes(480));native.call('request',[requested]);count=native.get(0x55a50c);assert count in (0,1);event=bytes(native.mu.mem_read(0x55a510,12)).hex() if count else '-';trace=';'.join(native.trace) or '-';rows.append(f'{requested}\t{current}\t{refresh}\t{mutation}\t{native.get(CURRENT)}\t{native.get(REFRESH)}\t{event}\t{trace}')
    args.fixture.write_text('\n'.join(rows)+'\n','utf-8',newline='\n');record=dict(unit=712,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(cases),nativeBodies=['004dea70','00452fe0','00452f90'],explicitBoundaries=['004414d0 currentLanguage Set with supplied synchronous callback effects','00452fe0 entry records/optional counter mutation; actual queue body runs'],gameExecutions=0,multiplayerTests=0,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),peSha256=native.sha256);args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__': main()
