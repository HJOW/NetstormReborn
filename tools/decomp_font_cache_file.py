#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 글꼴 캐시의 순차 읽기/쓰기와 File 정리 접두를 독립 CPU로 고정한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_basefile_whole_read import WholeReadOracle,BoundaryFailure,ROOT,PLAYERS,TYPES,TEXT,STACK,STOP,KEY,digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_FPCW

FONT,HEAP=TYPES+0x8000,TYPES+0xa000  # 원문0x1410 저장소와256개 독립 글리프의 격리 물리다.
SIGNATURE=b'BitmapFontData\x1a\0'  # 원문16BYTE 서명이며 reader는 읽은 마지막BYTE를0으로 덮는다.

def input_font():
    """기대 출력과 무관한 작은 세 글리프와 네256항목 표를 구성한다."""
    owner=bytearray(struct.pack('<4I',1,14,11,3))
    for table in range(4):  # 네 원문 표의 바이트 순서를 입력으로만 제공한다.
        owner.extend(struct.pack('<256I',*[(5 if table==0 else 0xfffffffe if table==1 else 6 if table==2 else {0:2,65:3,255:5}.get(code,0)) for code in range(256)]))
    glyphs=[bytes((code+offset*7)&255 for offset in range({0:2,65:3,255:5}.get(code,0))) for code in range(256)]
    return bytes(owner),glyphs

class FontCacheOracle(WholeReadOracle):
    """실제 글꼴 두 부모와 BaseFile 읽기/쓰기·Close/funclet을 실행한다."""
    def __init__(self):
        """검토한 함수 범위만 허용하며 OS/게임/통신은 실행하지 않는다."""
        super().__init__();add_ranges(self,(0x4a3240,0x4a3090,0x41aad0,0x41a910,0x4fb3f0))

    def instruction(self,mu,address,size,data):
        """CRT/할당/진단의 실제 인자와 BYTE를 기록하고 나머지는 원문 코드에 맡긴다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==0x41ab10:self.events.append('P');self.ret(self.scenario!=1);return
        if address==0x41b4c0:
            assert mu.reg_read(UC_X86_REG_ECX)==self.object and args[1:3]==(2 if self.writing else 0,1)
            self.events.append('O');mu.mem_write(self.object,struct.pack('<6I',0 if self.scenario==3 else PLAYERS,0x80|self.sub,37 if self.sub else 0,len(self.stored) if self.sub else 0,KEY if self.key else 0,3 if self.key else 0));self.position=37 if self.sub else 0
            if self.scenario==2:raise BoundaryFailure('open')
            self.ret(0,12);return
        if address==0x4e566c:self.events.append('T');self.ret(self.position);return
        if address==0x4e573c:
            self.events.append('S:'+str(args[1])+':'+str(args[2]));self.position=args[1];self.ret();return
        if address==0x4e586e:
            target,length,count,file=args[:4];assert file==PLAYERS;self.calls+=1
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes(4096))  # 명시0 작업 물리는 원문 미정stack 자체와 구별한다.
            start=self.position-(37 if self.sub else 0);actual=min(length*count,max(0,len(self.stored)-(2 if self.scenario==11 else 0)-start));self.events.append(f'R:{length}:{count}:{actual}')
            if self.scenario==4 and self.calls==self.fail:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.stored[start:start+actual])
            self.position+=actual;self.ret(count if actual==length*count else 0);return
        if address==0x4e44a6:
            target,length,count,file=args[:4];assert file==PLAYERS;self.calls+=1;sent=bytes(mu.mem_read(target,length*count));self.events.append(f'W:{length}:{count}:'+str(len(sent)))
            if self.scenario==4 and self.calls==self.fail:raise BoundaryFailure('write')
            actual=len(sent)//2 if self.scenario==11 else len(sent);self.output.extend(sent[:actual]);self.position+=actual;self.ret(count if actual==len(sent) else 0);return
        if address==0x43fa80:
            length=args[0];code=self.allocations;self.allocations+=1;self.events.append('N:'+str(code)+':'+str(length));assert length<=16
            if self.scenario==5 and self.allocations==self.fail:raise BoundaryFailure('allocation')
            pointer=HEAP+code*16;mu.mem_write(pointer,bytes(16));self.ret(pointer);return  # malloc0도 명시 존재 토큰이며 물리1BYTE 이상을 읽지 않는다.
        if address==0x4e7300:
            a,b,n=args[:3];left=bytes(mu.mem_read(a,n)).split(b'\0',1)[0];right=bytes(mu.mem_read(b,n)).split(b'\0',1)[0];self.ret((left>right)-(left<right));return
        if address==0x4e0620:
            self.events.append('D:'+str(args[2]))
            if not self.return_assert:raise BoundaryFailure('assertion')
            self.ret();return
        if address==0x4e54b9:
            self.events.append('C')
            if self.scenario==6:raise BoundaryFailure('close')
            self.ret();return
        if address==0x4e4354:self.events.append('K');self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 글꼴 명령 '+hex(address))

    def sample(self,case,precision):
        """동일 입력만 공급하고 실제 필드/설치/버퍼/호출열/쓴BYTE를 읽는다."""
        self.writing,self.key,self.sub,self.scenario,self.fail,self.return_assert=case
        owner,glyphs=input_font();payload=bytearray(SIGNATURE+owner[:16]+struct.pack('<I',255 if self.scenario==7 else 256)+owner[16:]+b''.join(glyphs))
        if self.scenario==8:payload[16:20]=struct.pack('<I',2)
        if self.scenario==9:payload[0]=ord('X')
        if self.scenario==10:payload[15]=255
        self.stored=bytes(value^(b'abc'[index%3] if self.key else 0) for index,value in enumerate(payload));self.object=STACK-36;self.events=[];self.output=bytearray();self.position=0;self.calls=0;self.allocations=0;self.scratch=None
        self.mu.mem_write(STACK-0x10000,b'\x65'*0x10100);self.mu.mem_write(FONT,owner if self.writing else bytes(4112));self.mu.mem_write(FONT+4112,bytes(1024));self.mu.mem_write(HEAP,bytes(4096));self.mu.mem_write(TEXT,b'font.chfnt\0');self.mu.mem_write(KEY,b'abc\0');self.put(0,0)
        if self.writing:
            for code,glyph in enumerate(glyphs):self.put(FONT+4112+code*4,HEAP+code*16);self.mu.mem_write(HEAP+code*16,glyph)  # 글리프는 입력 포인터/바이트이며 writer 출력을 계산하지 않는다.
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,TEXT));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,FONT);self.mu.reg_write(UC_X86_REG_EDX,TEXT);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok';returned='-'
        try:self.mu.emu_start(0x4a3090 if self.writing else 0x4a3240,STOP,count=1000000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.words(STACK-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP)
                try:self.mu.emu_start(0x4fb3f0,STOP,count=10000)
                except BoundaryFailure as cleanup:assert str(cleanup)==outcome=='close'  # 명시Close 실패 뒤 같은funclet의 재실패 접두만 관측하며 실제이중예외종료를합성하지않는다.
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4;returned=str(self.mu.reg_read(UC_X86_REG_EAX))
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        glyph_results=[]
        for code in range(256):  # 설치 전과 길이0 설치를 구별하고 설치한 실제 크기만 읽는다.
            pointer=self.words(FONT+4112+code*4,1)[0];length=self.words(FONT+0xc10+code*4,1)[0];glyph_results.append('-' if not pointer else bytes(self.mu.mem_read(pointer,length)).hex() or '0')
        return '\t'.join([','.join(map(str,case)),outcome,returned,','.join(map(str,self.fields())),bytes(self.mu.mem_read(FONT,4112)).hex(),','.join(glyph_results),';'.join(self.events),self.output.hex() or '-'])

def cases():
    """정상·서명/버전/개수·부분 I/O와 각 실패 단계의 독립 입력이다."""
    result=[(0,key,sub,scenario,0,return_assert) for key,sub,scenario in itertools.product(range(2),range(2),(0,1,2,3,6,7,8,9,10,11)) for return_assert in ((0,1) if scenario==7 else (0,))]
    result.extend((0,key,sub,scenario,fail,0) for key,sub in itertools.product(range(2),range(2)) for scenario,fail in ((4,1),(4,2),(4,6),(4,10),(4,11),(4,12),(5,1),(5,66),(5,256)))
    result.extend((1,key,0,scenario,fail,0) for key in range(2) for scenario,fail in ((0,0),(2,0),(3,0),(6,0),(11,0),(4,1),(4,6),(4,10),(4,11),(4,12)))
    assert len(result)==len(set(result));return result

def main():
    """새 최초 fixture 또는 재관측 검증만 허용하며 이전 기대값을 덮지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=FontCacheOracle();rows=['# case outcome returned fileFields ownerHex glyphs events outputHex']
    for case in cases():  # 두 x87 정밀도에서도 같은 원문 File 결과를 요구한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f),case;rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report=dict(unit=788,rows=len(rows)-1,observations=2*(len(rows)-1),host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제4a3240/4a3090·BaseFile읽기/쓰기·Close/소멸/4fb3f0. Probe/Open/CRT/strncmp/할당/진단은명시. 실제FontStore/OS/GDI/전체SEH는별도.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
