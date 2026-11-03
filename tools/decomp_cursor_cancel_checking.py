#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 커서 취소 전체 몸체의 현재 checking/TYPE 재조회와 진단 복귀를 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_cursor_lifecycle import Oracle,TYPES
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ESP

CHECKING=0x5e4794  # 반환음 뒤 원문이 읽는 같은 진단 DWORD다.

def cases():
    """비트 폭/분기 우회와 반환음/진단의 현재 상태 변경을 명시 입력으로 교차한다."""
    result=[]
    base=[1,0x80,0x80,0,0,0x437ac000,0,50,0,0,0,0,1,0,0,0,0]
    # 상위 비트도 비영이며 0x100은 반환음과 진단 전체를 우회한다.
    for checking,flags,sound,diagnostic in itertools.product((0,1,0x80000000,0xffffffff),(0,0x80,0x100,0x180),range(3),range(3)):
        row=base.copy();row[1]=flags;result.append((checking,sound,diagnostic,*row))
    # 잠금/없는 미리보기/죽은 대상은 같은 꼬리만 유지한다.
    for checking,gate in itertools.product((0,1,0x80000000,0xffffffff),range(3)):
        row=base.copy();row[(6,7,8)[gate]]=(1,0,1)[gate];result.append((checking,0,0,*row))
    assert len(result)==len(set(result));return result

class CancelCheckingOracle(Oracle):
    """기존 전체 취소 장치를 재사용하고 진단/반환음의 명시 효과만 확장한다."""
    def instruction(self,mu,address,size,data):
        """몸체는 그대로 실행하며 소비 이전/이후의 현재 DWORD/TYPE를 바꿀 수 있다."""
        if address==0x446e70:self.put(CHECKING,self.control[0])
        if address==0x4a9cb0:
            super().instruction(mu,address,size,data)
            if self.control[1]:self.put(CHECKING,0 if self.control[1]==1 else 0x80000000);self.put(0x55a280,TYPES+101*500)
            return
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<3I',mu.mem_read(sp+4,12))
            self.events.append('D:'+self.string(args[0]).encode().hex()+':'+self.string(args[1]).encode().hex()+':'+str(args[2]))
            if self.control[2]==2:raise BoundaryFailure('assertion')
            if self.control[2]==1:self.put(CHECKING,0);self.put(0x55a280,TYPES+102*500)
            self.ret();return
        super().instruction(mu,address,size,data)

    def call(self,*args,**kwargs):
        """진단 장치의 명시 중단과 정상 반환을 구별하고 정상 시 기존 ABI 검사도 유지한다."""
        self.outcome='return'
        try:return super().call(*args,**kwargs)
        except BoundaryFailure as boundary:self.outcome=str(boundary)

    def observe(self,case,precision):
        """기존78DWORD/raw150BYTE에 현재 checking과 반환/중단 경계를 추가한다."""
        self.control=case[:3];self.put(TYPES+101*500+0xc4,0xc0700000);self.put(TYPES+102*500+0xc4,0x80000000)
        output=self.sample(case[3:],precision);return (*output,self.outcome,str(self.get(CHECKING)))

def main():
    """두 x87 정밀도에서 전체 결과가 일치한 최초 바이트를 별도 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    oracles=[CancelCheckingOracle(),CancelCheckingOracle()];rows=[]
    # 기대값을 다시 계산하지 않고 독립 두 원문 실행의 상태/효과를 비교한다.
    for case in cases():
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input20 events globals78 costText raw50 raw51 raw77 outcome checking\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),entry='00446e70',rows=len(rows),observations=len(rows)*2,returnCount=sum(row[-2]=='return' for row in rows),assertionStops=sum(row[-2]=='assertion' for row in rows),diagnostics=sum('D:66616c7365:' in row[1] for row in rows),precision=[0x027f,0x037f],gameExecutions=0,boundaries=['기존 전체 취소/Activate 몸체와 명시 가상/UI/음향/SP 장치','진단은 명시 복귀/중단 장치이며 전체 SEH/진단UI 아님'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
