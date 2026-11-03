#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문4a4850의 현재 File·직접 팔레트 기록·후단 뒤 정리를 독립 CPU에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 보호 원본과 기존 메모리 관측장치를 읽기만 하는 저장소다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
from decomp_basefile_whole_read import WholeReadOracle,BoundaryFailure,PLAYERS,TYPES,TEXT,STACK,STOP,KEY,digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_FPCW

SAVED,LOADED,FLAGS,DEPTH=0x5acd28,0x555e08,0x5c78d4,0x5c7920  # 같은 원문 saved1024BYTE·loaded·화면flags·잠금깊이다.
COLOR_TABLE=TYPES+0x18000  # 원문의 색 목록 할당을 다른 관측장치와 겹치지 않게 제공한다.

class PaletteFileOracle(WholeReadOracle):
    """파일 읽기·팔레트 직접 기록·호출 부모/반납은 실제 코드이며 OS/CRT/색 검색만 장치다."""
    def __init__(self):
        """같은 File 몸체와 확인한 palette 부모·seek·cleanup 범위를 준비한다."""
        super().__init__()
        add_ranges(self,(0x4a4850,0x41a700,0x4fb3f0))

    def instruction(self,mu,address,size,data):
        """CRT 입력과 화면 장치 경계를 기록하며 raw 팔레트 쓰기는 명령어가 수행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,5)
        if address==0x41b4c0:
            assert mu.reg_read(UC_X86_REG_ECX)==self.object and args[1:3]==(0,1)
            self.events.append('O')
            mu.mem_write(self.object,struct.pack('<6I',0 if self.scenario==2 else PLAYERS,0x80|self.sub,37 if self.sub else 0,self.length if self.sub else 0,KEY if self.key else 0,3 if self.key else 0))
            self.position=37 if self.sub else 0
            if self.scenario==1:raise BoundaryFailure('open')
            self.ret(0,12);return
        if address==0x4e566c:self.events.append('T');self.ret(self.position);return
        if address==0x4e573c:
            self.events.append('S:'+str(args[1])+':'+str(args[2]))
            self.position=(37 if self.sub else 0)+self.length if args[2]==2 else self.position+args[1] if args[2]==1 else args[1]
            self.ret();return
        if address==0x4e586e:
            target,length,count,file=args[:4]
            if not file:
                self.events.append('R:null:'+str(length)+':'+str(count));raise BoundaryFailure('nullRead')  # 가상CRT의 명시 실패이며 실제 fread(NULL)의 OS 동작이나 성공을 합성하지 않는다.
            assert file==PLAYERS,(hex(file),length,count,self.length,self.key,self.sub,self.scenario)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes(4096))  # 미정 stack을 명시0으로 한 번만 시드하며 후속 Read에서는 이전 BYTE를 보존한다.
            start=self.position-(37 if self.sub else 0);actual=min(length*count,max(0,len(self.stored)-self.partial-start))
            self.reads+=1;self.events.append('R:'+str(length)+':'+str(count)+':'+str(actual))
            if self.scenario==3 and self.reads==self.fail_read:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.stored[start:start+actual])
            self.position+=actual;self.ret(count if actual==length*count else 0);return
        if address==0x4e0620:
            self.events.append('D:'+str(args[2]))
            if not self.return_assert:raise BoundaryFailure('assertion')
            self.ret();return
        if address==0x4a14e0:self.put(DEPTH,self.words(DEPTH,1)[0]+1);self.ret();return
        if address in (0x4a2060,0x4a0950,0x4a1fb0,0x4a46d0,0x4a16d0):self.ret();return  # 화면/대기/Present는 명시 장치이며 부분 픽셀 동일성을 주장하지 않는다.
        if address==0x4a2640:
            assert self.fields()[0]==PLAYERS and args[:4]==(0,256,0,1)
            self.events.append('A')
            if self.scenario==4:raise BoundaryFailure('apply')
            self.ret();return
        if address==0x4e4391:
            assert args[0]==36 and self.fields()[0]==PLAYERS
            mu.mem_write(COLOR_TABLE,bytes(36));self.ret(COLOR_TABLE);return
        if address==0x4a2820:
            assert self.fields()[0]==PLAYERS
            self.colors+=1;self.ret(self.colors);return  # 실제 색 검색은 기존 단위의 경계이며 후단까지 File이 열렸는지 확인한다.
        if address==0x4e54b9:
            self.events.append('C')
            if self.scenario==5:raise BoundaryFailure('close')
            self.ret();return
        if address==0x4e4354:self.events.append('K');self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 팔레트 명령 '+hex(address))

    def sample(self,case,precision):
        """입력 파일과 기존1024BYTE만 공급하고 새 색·예외 prefix·File 반납을 직접 읽는다."""
        self.length,self.key,self.sub,self.partial,clear,flags,depth,self.scenario,self.fail_read,self.return_assert=case
        payload=bytes((index*29+7)&255 for index in range(self.length))
        self.stored=bytes(value^(b'abc'[index%3] if self.key else 0) for index,value in enumerate(payload))
        self.object=STACK-36;self.events=[];self.scratch=None;self.reads=0;self.colors=0
        self.mu.mem_write(STACK-0x10000,b'\x65'*0x10100);self.mu.mem_write(TEXT,b'palette.col\0');self.mu.mem_write(KEY,b'abc\0');self.put(0,0)
        self.mu.mem_write(SAVED,b'\xa5'*1024);self.put(LOADED,7);self.put(FLAGS,flags);self.put(DEPTH,depth)
        for address,value in ((0x531860,640),(0x531864,480),(0x531858,9),(0x54d928,0),(0x5c7948,0),(0x5c78f8,11),(0x5c78ec,12),(0x59af88,13)):self.put(address,value)  # 같은 Screen의 독립 원문 전역을 입력으로 제공한다.
        self.mu.mem_write(STACK,struct.pack('<3I',STOP,TEXT,clear));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x4a4850,STOP,count=700000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.words(STACK-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4fb3f0,STOP,count=10000)
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        globals_=[self.words(address,1)[0] for address in (LOADED,DEPTH,0x5c78f8,0x5c78ec,0x59af88)]
        return '\t'.join([','.join(map(str,case)),outcome,','.join(map(str,self.fields())),bytes(self.mu.mem_read(SAVED,1024)).hex(),';'.join(self.events),','.join(map(str,globals_)),str(self.colors)])

def cases():
    """정상/짧은읽기·키·부분파일·실패단계·진단복귀를 각각 독립 입력으로 제공한다."""
    result=[(length,key,sub,partial,0,0x8a,0,0,0,0) for length,key,sub,partial in itertools.product((776,1024),range(2),range(2),(0,2,17))]
    result.extend((776,1,1,0,1,flags,depth,0,0,0) for flags,depth in itertools.product((0,0x8a),(0,3)))
    result.extend((length,0,sub,0,0,0x8a,0,0,0,return_assert) for length,sub,return_assert in itertools.product((0,12,775,777),range(2),range(2)))
    result.extend((length,key,sub,0,1,0x8a,0,scenario,0,0) for length,key,sub,scenario in itertools.product((776,1024),range(2),range(2),(1,2,4,5)))
    result.extend((776,key,sub,0,0,0x8a,0,3,fail,0) for key,sub,fail in itertools.product(range(2),range(2),(1,3,256,768)))
    assert len(result)==len(set(result))
    return result

def main():
    """새 관측 최초 작성 또는 기존 바이트 verify만 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=PaletteFileOracle();rows=['# case outcome fields saved1024Hex events globals colorSearchCalls']
    for case in cases():  # 두 x87 정밀도의 직접 쓰기·cleanup·효과열을 독립 비교한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report=dict(unit=786,host=socket.gethostname(),rows=len(rows)-1,observations=2*(len(rows)-1),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제4a4850/41aa80/41a700/41a7a0/41b410/4fb3f0. Open/CRT/색검색/OS표면/표시/할당은명시장치. 실제색검색·픽셀·32비트전체SEH dispatcher를검증한것이아님.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
