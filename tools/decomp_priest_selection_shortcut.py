#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 P/R의004d9884 분기를 격리 실행해 현재 카운터/선택과 반복 View 명령을 기록한다(703)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, STACK
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

def cases():
    """화면/사제 행동과 무관한 현재 DWORD와 명시 경계 변경만 만든다."""
    rows=[]  # selected/count/local/candidate/findEffect/selectEffect/command의7DWORD다.
    for selected,count,owner,candidate in itertools.product((0,50,51),(0,1,7,0xffffffff),(0,1,8,0xffffffff),(0,50,51)):  # 후보0과 같은 선택/변경·비정규 카운터/owner 비트를 교차한다.
        rows.append((selected,count,owner,candidate,0,0,0x020000b8))
    for find_effect,select_effect,same in itertools.product(range(5),range(4),(0,1)):  # 조회 뒤 selected/owner/count 변경과 선택 뒤 현재 count 증가를 대조한다.
        rows.append((50,0,1,50 if same else 51,find_effect,select_effect,0x020000b8))
        rows.append((50,7,1,50 if same else 51,find_effect,select_effect,0x020000b8))
    return list(dict.fromkeys(rows))

class Oracle(BaseOracle):
    """검토한 입력 분기와 두 점프 경로만 실행하며 함수 전체 반환을 합성하지 않는다."""
    def __init__(self):
        """같은 PC의 보호 PE를 적재하고 명시 분기 범위만 허용한다."""
        super().__init__('originals',[dict(name='unused',originals='004da880')],helpers=[]);self.allowed=[(0x4d9884,0x4d98e2),(0x4d9bb3,0x4d9bbb)];self.row=[];self.trace=[]

    def put(self,address,value):
        """원문의 전체 unsigned DWORD 비트를 저장한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def event(self,name,*args):
        """실제 장치 경계에 전달된 순서/인자만 기록한다."""
        self.trace.append(':'.join(map(str,(name,*args))))

    def on_instruction(self,mu,address,size,data):
        """기존 실제 참여 조회·선택·입력 큐만 명시 경계로 구별한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c=struct.unpack('<4I',mu.mem_read(sp,16))
        if address==0x494f10:
            self.event('F',a)
            if self.row[4]==1: self.put(0x5caea0,51)
            if self.row[4]==2: self.put(0x540c70,8)
            if self.row[4]==3: self.put(0x5caf40,0xffffffff)
            if self.row[4]==4: self.put(0x5caf40,0)
            mu.reg_write(UC_X86_REG_EAX,self.row[3])
        elif address==0x4d5c90:
            self.event('S',a,b,c)
            if self.row[5]==1: self.put(0x5caea0,a)
            if self.row[5]==2: self.put(0x5caf40,0xffffffff)
            if self.row[5]==3: self.put(0x5caf40,77)
        elif address==0x452fe0: self.event('Q',a,b,c)
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """부분 분기의 원래 공통 후속점까지 실행하고 스택 증가0을 확인한다."""
        self.row,self.trace=row,[]
        for address,value in zip((0x5caea0,0x5caf40,0x540c70,0x5429d4),(row[0],row[1],row[2],row[6])): self.put(address,value)  # 독립 현재 전역만 설치한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.emu_start(0x4d9884,0x4d9bbb,timeout=500000,count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==0x4d9bbb and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        current=[struct.unpack('<I',self.mu.mem_read(address,4))[0] for address in (0x5caea0,0x5caf40,0x540c70)]
        return ','.join(map(str,row)),';'.join(self.trace),','.join(map(str,current))

def main():
    """새 fixture만 생성하고 부분 실행 범위·큰 장치 경계·해시를 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# input7 trace selected_count_local3']
    for row in cases(): rows.append('\t'.join(oracle.run(row)))  # 기대값은 실제 기계어 분기가 결정한다.
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=703,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeRanges=['004d9884..004d98e1','004d9bb3..004d9bba'],stopBefore='004d9bbb',stackDelta=0,explicitBoundaries=['find00494f10','select004d5c90','queue00452fe0'],gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
