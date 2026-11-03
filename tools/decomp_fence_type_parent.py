#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원문 펜스 타입 부모의 전체 진단/복귀/현재 BYTE/DWORD 재판정을 격리 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

ENTRIES=(0x457890,0x457e20)  # 각 부모의 전체 명령 범위다.
CHECKING,SUN_TYPE=0x5e4794,0x541278  # 원문 checking DWORD와 sunFenceType DWORD의 현재 저장소다.

class FenceTypeOracle(MemoryOracle):
    """원문 부모 전체를 실행하고 이미 복원한 두 자식과 진단만 명시 장치로 둔다."""
    def __init__(self):
        """현재 PC 함수 범위와 보호 PE의 독립 메모리를 준비한다."""
        super().__init__();add_ranges(self,ENTRIES)

    def instruction(self,mu,address,size,data):
        """진단 복귀가 같은 raw BYTE/전역 DWORD를 바꾸면 후속 원문이 재판정하게 한다."""
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3);self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.case[5]:raise BoundaryFailure('assertion')
            if self.case[4]==1:self.put(POOL+50*50+10,self.words(SUN_TYPE,1)[0],1);self.put(CHECKING,0)
            if self.case[4]==2:self.put(SUN_TYPE,(self.words(SUN_TYPE,1)[0]&0xffffff00)|mu.mem_read(POOL+50*50+10,1)[0]);self.put(CHECKING,0)
            self.ret();return
        if address in (0x457460,0x4578f0):
            assert address==(0x457460,0x4578f0)[self.case[0]];assert mu.reg_read(UC_X86_REG_ECX)==POOL+50*50;self.events.append(('B' if self.case[0]==0 else 'F')+':50');self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미확인 펜스 타입 명령 '+hex(address))

    def observe(self,case,precision):
        """50BYTE 전체와 현재 두 전역·호출 효과·정상 반환/진단 중단을 기록한다."""
        self.case=case;self.events=[];raw=bytearray((index*17+3)&255 for index in range(50));raw[10]=case[2];self.mu.mem_write(POOL+50*50,bytes(raw));self.put(0x5c8464,POOL);self.put(CHECKING,case[1]);self.put(SUN_TYPE,case[3]);self.mu.mem_write(STACK,struct.pack('<II',STOP,50));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='return'
        try:self.mu.emu_start(ENTRIES[case[0]],STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        if outcome=='return':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.words(STACK+4,1)[0]==50
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return (';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,50)).hex(),str(self.words(CHECKING,1)[0]),str(self.words(SUN_TYPE,1)[0]),outcome)

def main():
    """기대값을 계산하지 않고 두 정밀도의 실제 원문 관측을 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[FenceTypeOracle(),FenceTypeOracle()];rows=[]
    # 슬롯 타입은 BYTE에서 확장하고 전역은 DWORD 전체를 비교하며 진단 복귀/중단을 독립 입력으로 준다.
    for case in itertools.product(range(2),(0,1,0x80000000,0xffffffff),(0,128,136),(136,128,0x180),range(3),(0,1)):
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    assert len(rows)==432;args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input6 events raw50 checking typeGlobal outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entries=['00457890','00457e20'],rows=len(rows),observations=len(rows)*2,normalReturns=sum(row[-1]=='return' for row in rows),assertionStops=sum(row[-1]=='assertion' for row in rows),diagnostics=sum('D:' in row[1] for row in rows),gameExecutions=0,boundaries=['두 dispatcher 전체·진단과 기존 재생성/정리 자식만 명시 장치','같은 raw BYTE/전역 DWORD 변경·주소/수명 변경은 입력 밖','진단UI/전체SEH/32비트ABI와 실제 게임 관측 아님']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
