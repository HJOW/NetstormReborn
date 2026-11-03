#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Gunanim 프레임의 signed 인덱스/현재 checking/진단 앞단을 원문에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,STACK,STOP
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_FPCW

FRAMES=(-2147483648,-101,-100,-99,-11,-10,-9,-6,-5,-4,-1,0,4,5,89,90,94,95,96,99,100,101,500,2147483647)  # signed 절삭/상한과 전체 정수 폭의 명시 입력이다.
CHECKING=0x5e4794  # 실제 같은 진단 DWORD다.

class GunFrameCheckingOracle(MemoryOracle):
    """첫 표 역참조 직전까지 실제 몸체를 실행하며 진단 복귀/중단만 장치로 둔다."""
    def __init__(self):
        """현재 정밀 함수 범위와 실제 표 읽기 경계를 연결한다."""
        super().__init__();add_ranges(self,(0x467610,))

    def instruction(self,mu,address,size,data):
        """진단 효과가 인자/현재 checking을 바꿔도 지역 인덱스는 원문이 유지한다."""
        if address==0x467658:self.events.append('B');raise BoundaryFailure('body')
        if address==0x4e0620:
            args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3);self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.case[2]:raise BoundaryFailure('assertion')
            if self.case[3]:self.put(CHECKING,0);self.put(STACK+4,95)
            self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미확인 Gunanim 명령 '+hex(address))

    def observe(self,case,precision):
        """선택한 진단/표 읽기 경계와 같은 지역 인덱스·현재 인자/전역을 기록한다."""
        self.case=case;self.events=[];self.put(CHECKING,case[1]);self.mu.mem_write(STACK,struct.pack('<II',STOP,case[0]&0xffffffff));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='return'
        try:self.mu.emu_start(0x467610,STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        assert outcome in ('body','assertion') and self.mu.reg_read(UC_X86_REG_FPCW)==precision
        index=self.mu.reg_read(UC_X86_REG_ESI);index=index if index<0x80000000 else index-0x100000000
        return (';'.join(self.events),str(index),str(self.words(CHECKING,1)[0]),str(self.words(STACK+4,1)[0]),outcome)

def main():
    """독립 두 정밀도의 실제 관측끼리 비교하고 최초 출력을 별도 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[GunFrameCheckingOracle(),GunFrameCheckingOracle()];rows=[]
    for case in itertools.product(FRAMES,(0,1,0x80000000,0xffffffff),(0,1),(0,1)):  # 진단 미선택/복귀/중단과 현재 상태 변경을 독립 입력으로 준다.
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    assert len(rows)==384;args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input4 events index checking argument outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entry='00467610',checkpoint='00467658 fld table',rows=len(rows),observations=len(rows)*2,bodyEntries=sum(row[-1]=='body' for row in rows),assertionStops=sum(row[-1]=='assertion' for row in rows),diagnostics=sum('D:' in row[1] for row in rows),gameExecutions=0,boundaries=['첫 표 역참조 전 접두·진단은 명시 복귀/중단 장치','음수 인덱스의 표 밖 메모리를 읽지 않음·유효 전체 방향은 기존518행','실제 공격/게임/진단UI/전체SEH/32비트ABI 관측 아님']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
