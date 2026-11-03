#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""연소 세 부모의 현재 SID·진단·HP 앞단을 실제 코드에서 독립 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,TEXT,STACK,STOP,digest
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

OBJECT=TEXT+0x1000  #144바이트 연소 객체의 격리 주소다.
CHECKING=0x5e4794  #같은 원문 현재 진단 DWORD다.
ENTRIES=(0x423d50,0x424110,0x424530)  #캡처·연소점·프레임의 실제 부모 진입점이다.
BODY=(0x423e0b,0x42418f,0x4245e0)  #모든 앞단 진단 뒤 기존 본문 첫 명령이며 여기서는 실행을 중단한다.

class BurningVictimOracle(MemoryOracle):
    """실제 앞단 분기를 실행하며 robust/HP/진단과 캡처 선행 clear만 명시 장치다."""
    def __init__(self):
        """세 정밀 함수 범위만 추가하고 게임/OS에는 진입하지 않는다."""
        super().__init__();add_ranges(self,ENTRIES)

    def instruction(self,mu,address,size,data):
        """현재 SID의 두 조회와 진단 복귀 뒤 고정한 슬롯을 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
        if address==BODY[self.entry]:raise BoundaryFailure('body')
        if address==0x487ab0:assert self.entry==0 and mu.reg_read(UC_X86_REG_ECX)==OBJECT+0x24;self.ret();return
        if address==0x4ac140:
            assert args[1]==1;result=(self.mask>>min(self.calls,1))&1;self.events.append('R:'+str(args[0])+':'+str(result));self.calls+=1
            if self.mutate==1 and self.calls==1:self.put(OBJECT+0x6c,1);self.put(CHECKING,0)
            self.ret(result);return
        if address==0x423dcf:
            sid=self.words(OBJECT+0x6c,1)[0];self.events.append('V:'+str(sid)+':'+str(self.vtable))
        if address==0x4adc60:
            pointer=mu.reg_read(UC_X86_REG_ECX);assert (pointer-POOL)%50==0;self.selected=(pointer-POOL)//50;self.events.append('H:'+str(self.selected)+':'+str(self.hp))
            if self.mutate==2:self.put(OBJECT+0x6c,1);self.put(CHECKING,0)
            self.ret(self.hp);return
        if address==0x4e0620:
            self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.resume:raise BoundaryFailure('assertion')
            self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 연소 앞단 명령 '+hex(address))

    def sample(self,case,precision):
        """독립 입력을 다시 공급하고 정상 조기 반환·본문 도달·진단 중단을 구별한다."""
        self.entry,sid,checking,self.mask,self.vtable,self.hp,self.resume,self.mutate=case;self.events=[];self.calls=0;self.selected=None;self.mu.mem_write(OBJECT,bytes(144));self.put(OBJECT+0x24,1);self.put(OBJECT+0x54,1);self.put(OBJECT+0x6c,sid);self.put(CHECKING,checking);self.put(0x5c8464,POOL)
        for number in (0,1,50):self.put(POOL+number*50,self.vtable)  #현재 번호 변경의 후보 슬롯은 실제 주소에서만 읽는다.
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,OBJECT);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='empty'
        try:self.mu.emu_start(ENTRIES[self.entry],STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        if outcome=='empty':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return '\t'.join([','.join(map(str,case)),outcome,str(self.words(CHECKING,1)[0]),str(self.words(OBJECT+0x6c,1)[0]),str(self.selected) if outcome=='body' else '-',';'.join(self.events)])

def cases():
    """실제 번호·4종 현재 DWORD·두 robust 반환·가상 표/HP와 진단 복귀를 조합한다."""
    result=[(entry,50,checking,mask,vtable,hp,resume,0) for entry,checking,mask,vtable,hp,resume in itertools.product(range(3),(0,1,0x80000000,0xffffffff),range(4),range(2),range(2),range(2))]
    result.extend((entry,50,checking,mask,1,1,resume,mutate) for entry,checking,mask,resume,mutate in itertools.product(range(3),(1,0x80000000),range(4),range(2),(1,2)))
    result.extend((entry,sid,checking,0,1,1,resume,0) for entry,sid,checking,resume in itertools.product(range(3),(0,65535),(0,0x80000000),range(2)))
    assert len(result)==len(set(result))==504;return result

def main():
    """최초 관측은 새 파일에만 저장하고 재관측은 기존 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists());oracle=BurningVictimOracle();rows=['# case outcome checking current selected events']
    for case in cases():  #두 x87 제어값에서 같은 앞단 효과와 현재 SID를 관측한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.write_text(content,encoding='utf-8',newline='\n')
    record=dict(unit=797,rows=len(rows)-1,observations=2*(len(rows)-1),host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제 세 부모의 robust/가상 표/HP 진단 앞단만·본문 첫 명령 전에 중단. robust/HP/진단·캡처 선행 clear는 명시 장치. 실제SID진단/캡처/입자/전체SEH/게임은 아님.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
