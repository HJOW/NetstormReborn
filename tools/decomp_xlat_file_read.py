#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Xlat4de710의 실제 File/키·버퍼 설치·닫기 전 정규화와 닫은 뒤 파서를 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 보호 원본과 기존711장치를 읽기만 한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
from decomp_xlat_runtime import Oracle as XlatOracle,OWNER,HEAP,PATH,QUERY,TEXTS
from decomp_oracle import Oracle as BaseOracle,STACK,STOP,SCRATCH
from decomp_basefile_whole_read import BoundaryFailure,digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_FPCW

FILE,KEY=SCRATCH+0x16000,SCRATCH+0x17000  # 가상 FILE과 키는 문자열/힙과 겹치지 않는다.
SEED=b'*\nKey\n-\nSeed\n=\n'  # 실패 전에 보존해야 할 다른 언어 장부의 독립 입력이다.
DIRECT_FILE=(0x41aad0,0x41aa80,0x41a7a0,0x41b410,0x41a680,0x4fc280)  # 기존711의 파일 스텁 대신 실제 실행할 몸체다.

class XlatFileOracle(XlatOracle):
    """기존 실제 파서/검색에 현재 File 본체와 C++ EH cleanup funclet을 더한다."""
    def __init__(self):
        """확인한 File/정리 범위만 추가하며 게임/OS/소켓은 실행하지 않는다."""
        super().__init__();add_ranges(self,DIRECT_FILE);self.allowed.append((0x4e4a40,0x4e4a7e))

    def words(self,address,count):
        """현재 원문 DWORD를 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(address,4*count))

    def ret(self,value=0,purge=0):
        """명시 장치의 원문 반환 규약만 수행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target=self.get(sp);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def buffer(self):
        """현재 which 슬롯에 실제 설치된 파일 버퍼를 복사한다."""
        pointer=self.get(OWNER+self.which*4)
        if not pointer:return b''
        assert pointer in self.allocation_sizes
        return bytes(self.mu.mem_read(pointer,self.allocation_sizes[pointer]))

    def on_instruction(self,mu,address,size,data):
        """파일/할당/로그만 공급하고 설치·정규화/파서/검색·File 정리는 실제 코드다."""
        if address in DIRECT_FILE:return BaseOracle.on_instruction(self,mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==0x459c60:
            assert args[0]==0xfffffffe and self.read_string(args[1])==b'd' and args[2]==0xfffffffd and args[4]==0
            path=b'.\\d\\'+self.read_string(args[3]);mu.mem_write(PATH,path+b'\0');self.events.append('F:'+path.hex());self.ret(PATH);return
        if address==0x41ab10:self.events.append('P');self.ret(self.scenario!=1);return
        if address==0x41b4c0:
            assert mu.reg_read(UC_X86_REG_ECX)==self.object and args[1:3]==(0,1);self.events.append('O')
            mu.mem_write(self.object,struct.pack('<6I',0 if self.scenario==3 else FILE,0x80|self.sub,37 if self.sub else 0,(0xffffffff if self.scenario==4 else self.length) if self.sub else 0,KEY if self.key else 0,3 if self.key else 0));self.position=37 if self.sub else 0
            if self.scenario==2:raise BoundaryFailure('open')
            self.ret(0,12);return
        if address==0x4e566c:self.events.append('T');self.ret(self.position);return
        if address==0x4e573c:
            self.events.append('S:'+str(args[1])+':'+str(args[2]));self.position=(-1 if self.scenario==4 else (37 if self.sub else 0)+self.length) if args[2]==2 else args[1];self.ret();return
        if address==0x4e586e:
            target,length,count,file=args[:4];assert file==FILE
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes(4096))  # 미작성 stack은 명시0을 한 번만 시드한다.
            start=self.position-(37 if self.sub else 0);actual=min(length*count,max(0,self.length-self.partial-start));self.events.append('R:'+str(length)+':'+str(count)+':'+str(actual))
            if self.scenario==6:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.stored[start:start+actual])
            self.position+=actual;self.ret(count if actual==length*count else 0);return
        if address==0x4e4391:
            if not self.data_allocated:
                self.events.append('N:'+str(args[0]))
                if self.scenario==5:raise BoundaryFailure('allocation')
                self.data_allocated=True
            value=HEAP+self.allocated;self.allocated+=(args[0]+15)&~15;assert self.allocated<0x100000;self.allocation_sizes[value]=args[0]
            mu.mem_write(value,bytes(args[0]));self.ret(value);return  # 실제 미정 malloc과의 동일성을 주장하지 않는 명시0 입력이다.
        if address==0x4e0620:self.events.append('D:'+str(args[2]));raise BoundaryFailure('assertion')
        if address==0x4c2630:
            text=self.read_string(args[1]);self.events.append('L:'+('length' if b'length' in text else 'missing'));self.ret();return
        if address==0x4e54b9:
            self.close_hex=self.buffer().hex() or '-';self.events.append('C')
            if self.scenario==7:raise BoundaryFailure('close')
            self.ret();return
        if address==0x4e4354:
            assert args[0]==KEY;self.events.append('K');self.ret();return  # 중복 버퍼 진단 뒤 기존 노드/키 UAF 경로는 정상으로 합성하지 않는다.
        if address==0x4de260 and not self.parsing:
            self.parsing=True;assert self.words(self.object,6)[0]==0 and self.words(self.object,6)[4]==0;self.events.append('Q')
            if self.scenario==8:raise BoundaryFailure('parse')
        return super().on_instruction(mu,address,size,data)

    def load_case(self,source,which,key,sub,partial,scenario,precision):
        """현재 파일 부모 한 번만 실행하며 명시 C++ 예외 때 실제 funclet만 호출한다."""
        self.which=which;self.length=len(source);self.key=key;self.sub=sub;self.partial=partial;self.scenario=scenario
        self.stored=bytes(value^(b'abc'[index%3] if key else 0) for index,value in enumerate(source));self.events=[];self.scratch=None;self.data_allocated=False;self.parsing=False;self.close_hex='-';self.object=STACK-72
        self.mu.mem_write(STACK-0x8000,b'\x65'*0x8100);self.put(0,0);self.mu.mem_write(KEY,b'abc\0');self.mu.mem_write(QUERY,b'case\0')
        self.mu.mem_write(STACK,struct.pack('<3I',STOP,which,QUERY));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,OWNER);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok';returned='-'
        try:self.mu.emu_start(self.entries['load'],STOP,count=700000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.get(STACK-4)==0:
                self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4fc280,STOP,count=10000)
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+12;returned=str(self.mu.reg_read(UC_X86_REG_EAX))
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return outcome,returned

    def sample(self,case,precision):
        """파일 필드와 닫기 시 버퍼를 먼저 보관한 뒤 실제 조회로 다른 언어 보존도 읽는다."""
        source,which,key,sub,partial,scenario,seed=case;self.reset();self.allocation_sizes={}  # 직전 사례의 버퍼/노드 논리 크기를 재사용하지 않는다.
        if seed or scenario==9:
            seed_which=which if scenario==9 else 3 if which!=3 else 6
            assert self.load_case(SEED,seed_which,0,0,0,0,precision)==('ok','1')
        outcome,returned=self.load_case(TEXTS[source],which,key,sub,partial,scenario,precision)
        fields=','.join(map(str,self.words(self.object,6)));present=bool(self.get(OWNER+which*4));buffer_hex=self.buffer().hex() or '-';close_hex=self.close_hex;events=';'.join(self.events);count=0
        for bucket in range(256):  # Native 목록의 실제 노드 수만 세며 기대 장부를 계산하지 않는다.
            node=self.get(OWNER+0x1c+bucket*36)
            while node:count+=1;assert count<4096;node=self.get(node)
        translations=[]
        for language in (3,6):  # 완성한 다른 언어/현재 언어의 Key를 실제 getter에서 읽는다.
            self.put(0x5435b4,language);self.mu.mem_write(QUERY,b'Key\0');pointer=self.call('translate',[QUERY],OWNER,4);translations.append(self.read_string(pointer).hex() or '-')
        return '\t'.join([','.join(map(str,case)),outcome,returned,fields,str(int(present)),buffer_hex,close_hex,events,str(count),','.join(translations)])

def cases():
    """기존 파서 입력과 새 현재 파일/키·부분 I/O·실패 단계만 조합한다."""
    result=[(source,which,key,sub,0,0,0) for source,which,key,sub in itertools.product(range(len(TEXTS)),(1,3,6),range(2),range(2))]
    result.extend((source,which,key,sub,2,0,0) for source,which,key,sub in itertools.product((1,5,7,11),(3,6),range(2),range(2)))
    result.extend((5,3,key,sub,0,scenario,0) for key,sub,scenario in itertools.product(range(2),range(2),range(1,10)))
    result.extend((5,6,key,sub,0,scenario,1) for key,sub,scenario in itertools.product(range(2),range(2),(0,5,6,8)))
    assert len(result)==len(set(result));return result

def main():
    """새 최초 관측 또는 바이트 verify만 허용하고 기존 기대값을 덮지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=XlatFileOracle();rows=['# case outcome returned fields present bufferHex closeBufferHex events count translations3and6']
    for case in cases():  # 같은 장부/파일 효과를 두 x87 정밀도에서 독립 실행한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report=dict(unit=787,rows=len(rows)-1,observations=2*(len(rows)-1),host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제4de710/파서/병합/조회와41aad0/41aa80/41a7a0/41b410/4fc280. Path/Probe/Open/CRT/할당/memmove/로그는명시. 길이0의할당밖쓰기·중복진단복귀뒤UAF·전체SEH/OS실패는미관측.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
