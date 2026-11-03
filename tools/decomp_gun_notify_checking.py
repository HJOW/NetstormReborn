#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GunProcess 통지/종속 정리의 현재 checking·진단 복귀를 원문에서 격리 관측한다."""
import argparse
import collections
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_gunprocess_oracle import GunOracle,SPEC,KEYS,OBJECT,PACKET,POOL,bits
from decomp_owner_oracle import OwnerOracle
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

def inputs():
    """통지와 종속 정리만 선택하며 Frame/Fire 입력은 생성하지 않는다."""
    base=dict(zip(KEYS,(5,3,60,0x20,1,128,0,0,0,1,1,0,0,0,0,0,bits(10),bits(10),0,0x3dcccccd,0,1,1,0,1,4,0,0,50)));rows=[]
    for stage,value,checking,anim,mutation,resume in itertools.product((3,6),(4,7,10,99),(0,1,0x80000000,0xffffffff),(0,80),range(4),(0,1)):  # 애니메이션 현재 조회/확정된 분기와 진단 복귀/중단이다.
        case=base.copy();case.update(stage=stage,value=value,anim=anim);rows.append((case,(checking,mutation,resume)))
    for target,value,server,checking,mutation,resume in itertools.product((0,60),(0,60,61),(0,1),(0,1,0x80000000,0xffffffff),range(4),(0,1)):  # 서버 관문/표적 재조회와 부모 free 변경을 교차한다.
        case=base.copy();case.update(event=2,target=target,value=value,server=server);rows.append((case,(checking,mutation,resume)))
    for server,checking in itertools.product((0,1),(0,1,0x80000000,0xffffffff)):  # 처리하지 않는 사건도 같은 현재 조건으로 복귀한다.
        case=base.copy();case.update(event=3,server=server);rows.append((case,(checking,0,1)))
    assert len(rows)==904;return rows

class GunNotifyOracle(GunOracle):
    """기존 입력/ABI 장치를 재사용하되 현재 정밀 통지/정리/getParent 범위만 실행한다."""
    def __init__(self):
        """현재 PC 전체 함수표로 허용 범위를 좁히고 공격 진입을 금지한다."""
        OwnerOracle.__init__(self,'originals');self.p=SPEC['originals'];self.mu.mem_map(0,0x1000);self.stub_calls=collections.Counter();self.returns=0;self.events=[];self.allowed=[];add_ranges(self,(0x46a340,0x46a290,0x41bfb0))

    def string(self,address):
        """진단의 NUL 종료 ASCII 식/파일을 바이트 그대로 읽는다."""
        result=bytearray()
        for offset in range(512):  # 알려진 짧은 원문 진단 문자열만 허용한다.
            value=self.mu.mem_read(address+offset,1)[0]
            if not value:return bytes(result)
            result.append(value)
        raise ValueError('Gun 통지 진단 문자열 종료 없음')

    def on_instruction(self,mu,address,size,data):
        """선택한 진단이 현재 필드를 바꾸거나 중단해도 원문 순서대로 처리한다."""
        if address in (self.p['entries'][1],self.p['entries'][6],self.p['shot'],self.p['start']):raise RuntimeError('통지 검사에서 발사/Frame 경계 도달')
        if address==self.p['entries'][5]:mu.mem_write(self.p['checking'],struct.pack('<I',self.control[0]));mu.reg_write(UC_X86_REG_FPCW,self.precision)
        if address==self.p['anim'] and self.control[1]==1:mu.mem_write(self.p['checking'],bytes(4));mu.mem_write(OBJECT+28,struct.pack('<I',9))
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);args=[self.read(sp+4+4*index) for index in range(3)];self.events.append('X:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.control[2]:raise BoundaryFailure('assertion')
            if self.control[1] in (2,3):mu.mem_write(self.p['checking'],bytes(4));mu.mem_write(OBJECT+24,struct.pack('<H',self.case['value']&65535));mu.mem_write(OBJECT+28,struct.pack('<I',2));mu.mem_write(POOL+50*self.stride+11,bytes([2 if self.control[1]==2 else 0]))
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.read(sp));return
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control,precision):
        """이전 raw56/packet64/효과에 현재checking·부모free·정상/중단을 더한다."""
        self.control=control;self.precision=precision;outcome='return'
        try:output=self.run_body(case)
        except BoundaryFailure as boundary:outcome=str(boundary);output=[';'.join(self.events),bytes(self.mu.mem_read(OBJECT,56)).hex(),bytes(self.mu.mem_read(PACKET,64)).hex(),0]
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return (*map(str,output),outcome,str(self.read(self.p['checking'])),str(self.read(POOL+50*self.stride+11,1)))

def main():
    """독립 두 정밀도에서 실제 통지 결과끼리 비교하고 첫 바이트를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[GunNotifyOracle(),GunNotifyOracle()];rows=[]
    for case,control in inputs():  # 기대값을 구현하지 않고 실제 원문 두 실행의 전체 결과를 비교한다.
        outputs=[oracle.observe(case,control,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],(case,control);rows.append((','.join(str(case[key]) for key in KEYS),','.join(map(str,control)),*outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input29 control3 events raw56 packet64 length outcome checking parentFlags\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entries=['0046a340','0046a290','0041bfb0'],rows=len(rows),observations=len(rows)*2,normalReturns=sum(row[-3]=='return' for row in rows),assertionStops=sum(row[-3]=='assertion' for row in rows),diagnostics=sum('X:' in row[2] for row in rows),gameExecutions=0,frameFireExecutions=0,boundaries=['통지/종속정리/getParent 전체·애니메이션조회/종속제거/Repop/진단은 명시 장치','현재 정밀 함수표 범위만 실행·Frame/Fire/발사 경계는 도달 시 오류','실제 공격/게임/MP송신/진단UI/전체SEH 관측 아님']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
