#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 Squid 표시 부모의 현재 TYPE/진단/표시 인수 전체를 격리 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,POOL,TYPES,STACK,STOP
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

CHECKING=0x5e4794  # 소비 지점마다 다시 읽는 현재 진단 DWORD다.

def cases():
    """프레임/이중 개수/해제 표식과 선택 효과의 현재 자료 변경을 명시 입력으로 교차한다."""
    result=[(checking,frame,flags,loaded,head,0,1) for checking,frame,flags,loaded,head in itertools.product((0,1,0x80000000,0xffffffff),(-1,0,3,4,7,8),(0,0x40000,0x400000,0x440000),(0,1),(100,0xdddddddd,0xcdcdcdcd))]
    # 변경 효과1=첫 TYPE조회,2=설명생성,3=로그,4=진단,5=본문 TYPE조회다.
    result += [(checking,frame,0,1,100,mutation,resume) for checking,frame,mutation,resume in itertools.product((0,1,0x80000000,0xffffffff),(-1,0,4),range(1,6),(0,1))]
    assert len(result)==len(set(result))==696;return result

class SquidDisplayParentOracle(MemoryOracle):
    """표시 부모 전체만 실행하고 TYPE 조회/설명/로그/진단/표시 자식을 명시 장치로 둔다."""
    def __init__(self):
        """현재 정밀 함수표의 전체 부모 범위와 독립 메모리를 준비한다."""
        super().__init__();add_ranges(self,(0x4ad500,))

    def mutate(self,kind):
        """선택된 효과 한 번이 current raw/전역을 바꾸며 고정 지역 frame은 건드리지 않는다."""
        if not kind or self.mutated or self.case[5]!=kind:return
        self.mutated=True;self.put(CHECKING,0);self.put(POOL+50*50+10,71,1);self.put(POOL+50*50+36,2);self.put(POOL+50*50+14,0x40f00000);self.put(POOL+50*50+18,0x3f400000);self.put(POOL+50*50+40,0,1)
        if kind==3:self.put(TYPES+70*500+0x114,1);self.put(TYPES+70*500,0xdddddddd)

    def instruction(self,mu,address,size,data):
        """원문이 계산한 인수와 현재 TYPE 재조회 순서만 경계에서 기록한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,5)
        if address==0x49a840:
            selected=args[0];assert selected in (70,71);self.events.append('T:'+str(selected));self.lookups+=1;self.mutate(1 if self.lookups==1 else 0)
            if self.lookups==(2 if self.case[0] else 1):self.mutate(5)
            self.ret(TYPES+selected*500);return
        if address==0x49adc0:
            selected=(mu.reg_read(UC_X86_REG_ECX)-TYPES)//500;self.events.append('F:'+str(selected)+':'+str(args[0])+':'+str(args[1]));self.mutate(2);self.ret(0x70ad0000+(selected-70)*0x100+(args[0]&255));return
        if address==0x436d40:self.events.append('L:'+str(args[0]));self.mutate(3);self.ret();return
        if address==0x4e0620:
            self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.case[6]:raise BoundaryFailure('assertion')
            self.mutate(4);self.ret();return
        if address==0x4990d0:self.events.append('M:'+':'.join(map(str,args)));self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미확인 Squid 표시 명령 '+hex(address))

    def observe(self,case,precision):
        """전체 효과와 raw50BYTE·선택 타입의 부모 소비 필드·현재 checking을 기록한다."""
        self.case=case;self.events=[];self.lookups=0;self.mutated=False;self.put(CHECKING,case[0]);self.put(0x5c8464,POOL)
        raw=bytearray((index*17+3)&255 for index in range(50));raw[10]=70;raw[40]=5;struct.pack_into('<I',raw,36,case[1]&0xffffffff);struct.pack_into('<II',raw,14,0x40500000,0xc0200000);self.mu.mem_write(POOL+50*50,bytes(raw))
        for number,head,flags,count,loaded in ((70,case[4],case[2],4,case[3]),(71,200,0x440000,2,1)):  # 무관 필드는0이며 부모가 읽는 네 필드만 입력한다.
            self.mu.mem_write(TYPES+number*500,bytes(500));self.put(TYPES+number*500,head);self.put(TYPES+number*500+0xe8,flags);self.put(TYPES+number*500+0x114,count);self.put(TYPES+number*500+0xdc,loaded)
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,POOL+50*50);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='return'
        try:self.mu.emu_start(0x4ad500,STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        if outcome=='return':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        fields=[self.words(TYPES+number*500+offset,1)[0] for number in (70,71) for offset in (0,0xe8,0x114,0xdc)]
        return (';'.join(self.events),bytes(self.mu.mem_read(POOL+50*50,50)).hex(),','.join(map(str,fields)),str(self.words(CHECKING,1)[0]),outcome)

def main():
    """두 독립 정밀도의 실제 관측을 비교하고 최초 바이트를 별도 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[SquidDisplayParentOracle(),SquidDisplayParentOracle()];rows=[]
    for case in cases():  # 기대 결과를 구현해 만들지 않고 실제 두 몸체 실행끼리 비교한다.
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input7 events raw50 typeFields8 checking outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entry='004ad500',rows=len(rows),observations=len(rows)*2,normalReturns=sum(row[-1]=='return' for row in rows),assertionStops=sum(row[-1]=='assertion' for row in rows),diagnostics=sum('D:' in row[1] for row in rows),gameExecutions=0,boundaries=['표시 부모 전체와 명시 TYPE 조회/설명 생성/로그/진단/표시 자식','같은 raw/TYPE 주소 수명이 유지되는 변경 입력','실제 SquidDisplay/Renderer 연결과 진단UI/전체SEH는 별도']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
