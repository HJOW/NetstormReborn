#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Xlat 최초 적재·실제 파서/병합/검색을 격리 실행해 바이트와 번역값을 관측한다(711)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

OWNER,HEAP,PATH,QUERY=0x5e22b8,0x11000000,SCRATCH+0x10000,SCRATCH+0x11000  #원문 장부/실제 확보/이름/조회 주소의 독립 물리다.
TEXTS=(b'\0',b'header\n*\nKey\n-\nValue\n=\n',b'*\nKey\n-\nValue\n=',b'*\nKey\n-\nValue\n=tail',b'*\nKey\n-\nIncomplete',b'*\r\nKey\r\n-\r\nValue\r\n=\r\n',b'*\r\r\nKey\r\n-\nA\rB\n=\n',b'*\nKey\n-\nFirst\n=\n*\nKey\n-\nLast\n=\n',b'*\nAB\n-\nOne\n=\n*\nBA\n-\nTwo\n=\n',b'*\nKey\n-\n\n=\n',b'*\n\n-\nEmptyKey\n=\n',b'*\nKey\n-\nValue\n=\n\0*\nHidden\n-\nNo\n=\n',b'*\nK\xe4y\n-\nV\xf6lue\n=\n',b'*\nMulti\nline\n-\nTwo\nlines\n=\n')  #끝BYTE/CR/중복/해시 충돌/빈 값/원문 문자/여러 줄 입력이다.
KEYS=(b'',b'Key',b'key',b'AB',b'BA',b'Hidden',b'K\xe4y',b'Multi\nline',b'Key\0ignored')  #원본 strcmp/NULL 번역 칸을 관찰하는 문자열이다.


def hextext(value):
    """빈 바이트와 공백 없는 hex를 구별한다."""
    return value.hex() or '-'


class Oracle(BaseOracle):
    """Xlat 본체·목록·strchr만 실행하고 파일/할당/CRT 이동을 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 같은 PC의 정밀 함수 범위를 읽는다."""
        super().__init__('originals',[dict(name='load',originals='004de710'),dict(name='translate',originals='004de9a0'),dict(name='init',originals='004dea00')],[0x4de260,0x4de1e0,0x4de130,0x4e63e0])
        self.mu.mem_map(0,4096);self.mu.mem_map(HEAP,0x100000);self.files={};self.active=b'';self.allocated=0;self.trace=[]  #장치별 입력/할당/실제 호출을 같은 사례 안에서 유지한다.

    def put(self,address,value):
        """DWORD 한 칸만 수정한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """현재 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def reset(self):
        """사례 사이 BSS0 장부와 할당 장치만 초기화한다."""
        self.mu.mem_write(OWNER,bytes(0x241c));self.allocated=0;self.trace=[]

    def on_instruction(self,mu,address,size,data):
        """파일·할당·CR 이동만 공급하며 해시/정확 비교/중복 병합은 실제 CPU에 맡긴다."""
        selected=(0x459c60,0x41ab10,0x41b4c0,0x41aad0,0x41aa80,0x41a7a0,0x41b410,0x4c2630,0x4e4391,0x4e4354,0x4e5b40)
        if address not in selected: return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c,d,e=struct.unpack('<6I',mu.mem_read(sp,24));value=0;purge=0
        if address==0x459c60:
            self.active=b'd/'+self.read_string(d);mu.mem_write(PATH,self.active+b'\0');value=PATH
        elif address==0x41ab10: self.trace.append('exists:'+self.read_string(a).hex());value=int(self.active in self.files)
        elif address==0x41b4c0: self.trace.append('open:'+self.read_string(a).hex());assert b==0 and c==1;purge=12
        elif address==0x41aad0: value=1
        elif address==0x41aa80: value=len(self.files[self.active])
        elif address==0x41a7a0: assert b==len(self.files[self.active]) and c==0xffffffff;mu.mem_write(a,self.files[self.active]);value=b;purge=12
        elif address==0x41b410: self.trace.append('close')
        elif address==0x4e4391:
            value=HEAP+self.allocated;self.allocated+=(a+15)&~15;assert self.allocated<0x100000;mu.mem_write(value,b'\xcc'*a)
        elif address==0x4e4354: raise AssertionError('새 사례에서 기존 버퍼 해제를 선택함')
        elif address==0x4e5b40: mu.mem_write(a,bytes(mu.mem_read(b,c)));value=a
        elif address==0x4c2630: self.trace.append('log:'+str(a))
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)


def main():
    """최초 관측은 새 경로에만 저장하며 기대값을 Python 모델로 계산하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists(),'이전 관측 보존'
    native=Oracle();rows=['# C case | L which sourceHex returned bufferHex sizeAfter | Q language keyHex outputHex | I mask trace']
    cases=[[(which,text)] for which,text in itertools.product((2,3,6),TEXTS)]  #각 raw 입력은 다른 언어의 빈 칸도 함께 관측한다.
    cases.append([(3,TEXTS[7]),(1,b'*\nKey\n-\nEnglish\n=\n'),(6,b'*\nKey\n-\nPortuguese\n=\n*\nBA\n-\nPT\n=\n')])  #동일 키의 세 언어가 서로 덮어쓰지 않는다.
    for case,loads in enumerate(cases):  #각 사례는 완전히 독립된 Native 장부에서 시작한다.
        native.reset();rows.append(f'C\t{case}')
        for which,source in loads:  #같은 사례 안에서 입력 언어를 순서대로 병합한다.
            native.files={b'd/case':source};native.mu.mem_write(QUERY,b'case\0');returned=native.call('load',[which,QUERY],OWNER,8);assert returned==1
            pointer=native.get(OWNER+which*4);buffer=bytes(native.mu.mem_read(pointer,len(source)));count=0
            for bucket in range(256):  #기대 레코드 수는 Native 연결 목록의 실제 노드에서 센다.
                node=native.get(OWNER+0x1c+bucket*36)
                while node: count+=1;node=native.get(node)  #같은 버킷의 실제 next 목록을 끝까지 센다.
            rows.append(f'L\t{which}\t{hextext(source)}\t{returned}\t{buffer.hex()}\t{count}')
        for language,key in itertools.product((0,1,2,3,6),KEYS):  #영어 우회·없는 칸·정확 키·첫NUL을 원문 검색으로 관찰한다.
            native.put(0x5435b4,language);native.mu.mem_write(QUERY,key+b'\0');pointer=native.call('translate',[QUERY],OWNER,4);rows.append(f'Q\t{language}\t{hextext(key)}\t{hextext(native.read_string(pointer))}')
    startup=Oracle()
    for mask in range(8):  #세 실제 시작 파일의 모든 부재 조합과 고정 순서를 관측한다.
        startup.reset();startup.files={b'd/xlat.'+name:TEXTS[1] for bit,name in enumerate((b'german',b'english',b'portuguese')) if mask&(1<<bit)};startup.call('init',[])
        rows.append(f'I\t{mask}\t'+','.join(startup.trace))
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=711,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(cases),observations=len(rows)-1,nativeBodies=['004de710','004de260','004de1e0','004de130','004de9a0','004dea00','004e63e0 strchr'],explicitBoundaries=['459c60 path join','41ab10/41b4c0/41aad0/41aa80/41a7a0/41b410 file devices','4c2630 log','4e4391 new','4e5b40 memmove'],gameExecutions=0,multiplayerTests=0,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),peSha256=native.sha256)
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__': main()
