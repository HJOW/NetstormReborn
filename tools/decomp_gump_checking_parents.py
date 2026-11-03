#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 Gump overlay/기본 소멸의 현재 checking·진단 복귀를 원문에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP,digest
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

TOKEN=POOL+0x1000  # 호스트 UI 토큰과 구별하는 원문 this의 격리 주소다.
OVERLAY,IDLE,KILL,CHECKING=0x563ca8,0x5634d0,0x564de0,0x5e4794  # 원문 현재 고정 표/진단 DWORD다.
MARKER=0x12345678  # 현재 대상과 구별되는 비영 입력이며 역참조하지 않는다.

def rle(values):
    """전체 표를 값 변경 없이 연속 길이/값으로 줄인다."""
    return ';'.join(str(len(list(group)))+':'+str(value) for value,group in itertools.groupby(values))

def overlay(pattern):
    """빈 표·혼합·중복·소진·소진 중복 입력만 만든다."""
    result=[0 if pattern<3 else MARKER for _ in range(100)]
    if pattern==1:result=[0 if index%2 else MARKER for index in range(100)]
    if pattern in (2,4):result[3]=result[99]=TOKEN
    return result

class GumpCheckingOracle(MemoryOracle):
    """원문 두 부모만 실행하고 진단/CRT free의 명시 효과를 받는다."""
    def __init__(self):
        """확인한 부모 범위를 추가한다. OS/가상 UI/allocator는 실행하지 않는다."""
        super().__init__();add_ranges(self,(0x4653c0,0x464420))

    def mutate_fields(self,which):
        """원문 callback 중 변형할 현재 WORD/포인터/표 입력을 공급한다."""
        self.put(CHECKING,0)
        if which==1:self.put(TOKEN+4,0x13579bd);self.put(TOKEN+124,0x3333);self.put(OVERLAY+99*4,MARKER);self.put(OVERLAY+98*4,MARKER)
        else:self.put(TOKEN+4,0xcafebabe);self.put(TOKEN+124,0x4444);self.put(OVERLAY+50*4,TOKEN);self.put(OVERLAY+99*4,MARKER)

    def instruction(self,mu,address,size,data):
        """진단과 순차 free의 정확한 인자/효과 접두를 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
        if address==0x4e0620:
            self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if self.mutate==1:self.mutate_fields(1)
            if not self.resume:raise BoundaryFailure('assertion')
            self.ret();return
        if address==0x4e4354:
            self.events.append('F:'+str(args[0]))
            if self.mutate==2 and args[0]==0x1111:self.mutate_fields(2)
            self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 Gump 부모 명령 '+hex(address))

    def sample(self,case,precision):
        """실제 변경된 노드 필드·전후 표와 인접 DWORD까지 읽는다."""
        kind,pattern,enabled,flags,checking,self.resume,self.mutate=case;self.events=[];self.mu.mem_write(TOKEN,b'\x65'*160);self.put(TOKEN,MARKER);self.put(TOKEN+4,flags);self.put(TOKEN+120,0x1111);self.put(TOKEN+124,0x2222);self.put(CHECKING,checking)
        self.mu.mem_write(OVERLAY,struct.pack('<100I',*overlay(pattern)));self.put(OVERLAY-4,0xa5a5a5a5)
        for address,count in ((IDLE,500),(KILL,1000)):  # 모든 일치/다른 토큰과 비활성 끝 칸도 입력에 포함한다.
            values=[0]*count;values[0]=values[-1]=TOKEN;values[1]=MARKER;self.mu.mem_write(address,struct.pack('<'+'I'*count,*values))
        args=[STOP,enabled] if kind=='O' else [STOP];self.mu.mem_write(STACK,struct.pack('<'+'I'*len(args),*args));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,TOKEN);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x4653c0 if kind=='O' else 0x464420,STOP,count=100000)
        except BoundaryFailure:outcome='assertion'
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4*len(args)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return '\t'.join([','.join(map(str,case)),outcome,';'.join(self.events) or '-',*[str(self.words(address,1)[0]) for address in (CHECKING,TOKEN+4,TOKEN,TOKEN+120,TOKEN+124,OVERLAY-4)],rle(self.words(OVERLAY,100)),rle(self.words(IDLE,500)),rle(self.words(KILL,1000))])

def cases():
    """중복/소진의 무조건 진단과 missing disable의 현재 checking을 교차한다."""
    result=[('O',pattern,enabled,flags,checking,resume,mutate) for pattern,enabled,flags,checking,resume,mutate in itertools.product(range(5),range(2),(0,0xffffffff),(0,1,0x80000000,0xffffffff),range(2),range(2))]
    result.extend(('B',2,0,flags,checking,resume,mutate) for flags,checking,resume,mutate in itertools.product((0,4),(0,1,0x80000000,0xffffffff),range(2),range(3)))
    assert len(result)==len(set(result))==368;return result

def main():
    """두 정밀도 최초 관측을 새 파일에 고정하거나 별도 재관측으로 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists());oracle=GumpCheckingOracle();rows=['# case outcome events checking flags vtable text alternate beforeOverlay overlay idle kill']
    for case in cases():  # 각각의 실제 부모 호출은 독립 입력에서 시작한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.write_text(content,encoding='utf-8',newline='\n')
    record=dict(unit=796,rows=368,observations=736,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제4653c0/464420·진단과free는명시장치. 소진후복귀의overlay[-1]쓰기까지가상메모리로관측하며실제호스트배열에는허용하지않음. 비복귀진단은명시중단·전체SEH/UI아님.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=368,observations=736)))

if __name__=='__main__':main()
