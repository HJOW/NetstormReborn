#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 언어 감지/이름/옵션 앞단을 격리 실행한다. 게임·OS·MP는 실행하지 않는다(710)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

API = SCRATCH+0x1f000  #실제 OS를 호출하지 않는 명시 LANGID 반환 주소다.
NAMES = (b'',b'english',b'English',b'ENGLISH',b'french',b'german',b'spanish',b'japanese',b'portuguese',b'unknown',b' french',b'french ',b'french\0german')  #정확 비교/첫NUL/미등록 입력이다.


class Oracle(BaseOracle):
    """현재 보호 PE의 세 본체와435220의 언어 prefix만 실행한다."""
    def __init__(self):
        """정밀 범위/원문 이름표를 읽고 OS import만 격리 주소로 교체한다."""
        super().__init__('originals',[dict(name='detect',originals='004de390'),dict(name='find',originals='004de420'),dict(name='options',originals='00435220')],[0x4dea40])
        self.put(0x500140,API);self.raw=b'';self.langid=0;self.loads=[]  #이번 독립 입력과 실제 load 관측 순서다.

    def put(self,address,value):
        """격리 DWORD 한 칸만 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """격리 DWORD 한 칸의 원시 비트를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def on_instruction(self,mu,address,size,data):
        """실제 선택 제어는 실행하고 Config/OS/진단과 prefix 끝만 경계로 둔다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x43529f:
            mu.reg_write(UC_X86_REG_EAX,mu.reg_read(UC_X86_REG_EBP));mu.reg_write(UC_X86_REG_ESP,STACK+4);mu.reg_write(UC_X86_REG_EIP,STOP);return  #전체435220 반환이 아닌 언어 부분의 명시 checkpoint다.
        if address not in (API,0x441300,0x441db0,0x4c2630): return super().on_instruction(mu,address,size,data)
        ret,a,b=struct.unpack('<III',mu.mem_read(sp,12));value=0
        if address==API: value=self.langid
        elif address==0x441300:
            assert self.read_string(a)==b'currentLanguage';mu.mem_write(b,self.raw+b'\0');value=1
        elif address==0x441db0: self.loads.append((self.read_string(a).hex() or '-',self.get(0x5435b4),self.get(0x542384)))
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)


def main():
    """독립 입력의 실제 관측만 새 fixture/보고서에 저장하며 기존 자료는 덮지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists(),'이전 관측 보존'
    native=Oracle();rows=['# D langid result | F nameHex result | O detected current published nameHex changed currentAfter publishedAfter loadHex,current,published;...']
    for primary,sub in itertools.product(range(1024),(0,1,2,63)):  #모든 primary와 영문 변형/최대 sublanguage를 독립 실행한다.
        native.langid=primary|(sub<<10);rows.append(f'D\t{native.langid}\t{native.call("detect",[])}')
    for raw in NAMES:  #원문 이름표 탐색과 미등록 값을 실제 strcmp 몸체로 관측한다.
        native.mu.mem_write(SCRATCH,raw+b'\0');rows.append(f'F\t{raw.hex() or "-"}\t{native.call("find",[SCRATCH])}')
    for detected,current,raw in itertools.product(range(7),(-1,0,1,2,3,6),NAMES):  #이전 번호를 유지하는 경로와 바뀌는 경로 모두 선택한다.
        published=0x12345678;native.raw=raw;native.loads=[];native.put(0x54db58,detected);native.put(0x5435b4,current);native.put(0x542384,published)
        changed=native.call('options',[]);trace=';'.join(f'{name},{now},{shown}' for name,now,shown in native.loads) or '-'
        rows.append(f'O\t{detected}\t{current&0xffffffff}\t{published}\t{raw.hex() or "-"}\t{changed}\t{native.get(0x5435b4)}\t{native.get(0x542384)}\t{trace}')
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=710,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),observations=len(rows)-1,detect=4096,find=len(NAMES),options=7*6*len(NAMES),nativeBodies=['004de390','004de420','004dea40','00435220..0043529f checkpoint'],explicitBoundaries=['GetUserDefaultLangID returns supplied WORD','00441300 ConfigReadString','00441db0 LoadLanguage records current/published','004c2630 log','0043529f prefix stop; not whole parent return'],gameExecutions=0,multiplayerExecutions=0,peSha256=native.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__': main()
