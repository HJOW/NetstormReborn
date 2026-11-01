#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""선택 키의 원문 정지/문자/수정키 분배와 View 큐를 부분 격리 실행한다(704)."""
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
    """실제 키/정지/수정키와 현재 owner DWORD를 독립 입력으로 만든다."""
    rows=[]  # code/paused/local의3DWORD다. 입력 분기는 key-up 관문 뒤부터 실행한다.
    for key,modifiers,paused,owner in itertools.product((ord('H'),ord('h'),ord('N'),ord('n'),ord('P'),ord('p'),ord('R'),ord('r'),0x730000,0x740000),range(8),(0,1),(0,1,0xffffffff)):  # Shift/Control/Alt의 모든 조합을 별도로 공급한다.
        rows.append((key|((modifiers&1)*0x20000000)|((modifiers&2)*0x08000000)|((modifiers&4)*0x02000000),paused,owner))
    return rows

class Oracle(BaseOracle):
    """4d96b6의 실제 공통 정지와 jump table을 실행하고 기존 선택 부모는 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE의 같은 전체 입력 몸체를 적재하되 필요한 부분만 허용한다."""
        super().__init__('originals',[dict(name='unused',originals='004d62b0')],helpers=[]);self.allowed=[(0x4d96b6,0x4d9bbb)];self.row=[];self.trace=[]
        assert struct.unpack('<I',self.mu.mem_read(0x5429ac,4))[0]==0x020000ae and struct.unpack('<I',self.mu.mem_read(0x5429d4,4))[0]==0x020000b8

    def event(self,name,*args):
        """실제로 선택한 큰 경계의 호출 순서/인자를 보존한다."""
        self.trace.append(':'.join(map(str,(name,*args))))

    def on_instruction(self,mu,address,size,data):
        """정지·큐·다음 유닛과 사제 몸체 입구만 명시 경계로 구별한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c=struct.unpack('<4I',mu.mem_read(sp,16))
        if address==0x4d9884:
            self.event('R');mu.reg_write(UC_X86_REG_EIP,0x4d9bbb);return
        if address==0x460de0: self.event('P',self.row[1]);mu.reg_write(UC_X86_REG_EAX,self.row[1])
        elif address==0x452fe0: self.event('Q',a,b,c)
        elif address==0x4da880: self.event('N',a)
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """원래 분기 후속점까지 실행하고 입력 대비 스택 증가0을 확인한다."""
        self.row,self.trace=row,[];self.mu.mem_write(0x540c70,struct.pack('<I',row[2]));self.mu.mem_write(STACK+0x14,struct.pack('<I',row[0]));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(0x4d96b6,0x4d9bbb,timeout=500000,count=2000);assert self.mu.reg_read(UC_X86_REG_EIP)==0x4d9bbb and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        return ','.join(map(str,row)),';'.join(self.trace)

def main():
    """기존 fixture를 덮지 않고 새 부분 분기 입력·해시·장치 경계를 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# input3 trace']
    for row in cases(): rows.append('\t'.join(oracle.run(row)))  # 실제 원문 CPU가 기대 분기와 큐 인자를 결정한다.
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n');record=dict(unit=704,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeRange='004d96b6..004d9bba',stopBefore='004d9bbb',stackDelta=0,explicitBoundaries=['paused00460de0','queue00452fe0','next004da880','priest entry004d9884'],releaseGuardOutsideRange=True,homeCommand=0x020000ae,priestCommand=0x020000b8,gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
