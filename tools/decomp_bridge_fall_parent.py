#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제00421f90의 현재checking·genus/진단과21530호출을 독립 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP,digest
from decomp_basefile_whole_read import BoundaryFailure
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

CHECKING=0x5e4794  # 원문 전체현재DWORD이며bool로축소하지않는다.
COORDS=((0x80000000,0x41780000),(0x42fa8000,0xc0600000),(0x7fc12345,0x7f800000))  # 원래float비트입력이며부호0/정상/NaN·무한대도부모는그대로넘긴다.

class BridgeFallOracle(MemoryOracle):
    """부모의분기/주소산술은실제코드이며genus/진단/자식예약만명시장치다."""
    def __init__(self):
        """확인한부모범위와공유checking/풀주소만연결한다."""
        super().__init__();add_ranges(self,(0x421f90,))

    def instruction(self,mu,address,size,data):
        """실제부모인자·진단/복귀효과열과변경된현재DWORD를읽는다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
        if address==0x4ac200:
            pointer=mu.reg_read(UC_X86_REG_ECX);assert pointer==POOL+self.sid*50;self.events.append('G:'+str(pointer)+':'+str(self.genus))
            if self.mutate:self.put(CHECKING,0)
            self.ret(self.genus);return
        if address==0x4e0620:
            self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if self.mutate:self.put(CHECKING,0xabcdef10)
            if not self.return_assert:raise BoundaryFailure('assertion')
            self.ret();return
        if address==0x421530:
            pointer=mu.reg_read(UC_X86_REG_ECX);assert pointer==POOL+self.sid*50;self.events.append('F:'+str(pointer)+':'+str(args[0])+':'+str(args[1]));self.ret(0,8);return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인다리낙하부모명령 '+hex(address))

    def sample(self,case,precision):
        """입력DWORD/float만공급하고정상반환또는명시진단중단접두를관측한다."""
        self.sid,checking,self.genus,coordinate,self.return_assert,self.mutate=case;self.events=[];self.put(0x5c8464,POOL);self.put(CHECKING,checking);self.mu.mem_write(STACK-0x1000,b'\x65'*0x1100);self.mu.mem_write(STACK,struct.pack('<4I',STOP,self.sid,*COORDS[coordinate]));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x421f90,STOP,count=10000)
        except BoundaryFailure:outcome='assertion'
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return '\t'.join([','.join(map(str,case)),outcome,str(self.words(CHECKING,1)[0]),';'.join(self.events)])

def cases():
    """4종checking전체비트와선택genus/진단복귀·효과중변형을독립조합한다."""
    result=[(sid,checking,genus,coordinate,resume,0) for sid,checking,genus,coordinate,resume in itertools.product((0,50),(0,1,0x80000000,0xffffffff),(0,4,5,0x10000),range(3),range(2))]
    result.extend((50,checking,genus,0,resume,1) for checking,genus,resume in itertools.product((1,0x80000000),(0,4),range(2)))
    assert len(result)==len(set(result))==200;return result

def main():
    """최초기대값을새파일로만저장하거나기존바이트와재관측을대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=BridgeFallOracle();rows=['# case outcome finalChecking events']
    for case in cases():  # 두x87정밀도에서같은비트전달/현재DWORD/진단접두를독립관측한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    record=dict(unit=793,rows=len(rows)-1,observations=2*(len(rows)-1),host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제421f90만실행·genus/진단/21530예약은명시. 비복귀진단은관측중단이며전체SEH/실제Kernel/낙하행동검사아님. 풀주소변경/해제된포인터동작은관측하지않음.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
