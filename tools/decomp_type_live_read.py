#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제49c160→459c60→41baa0의TYPE 이름/고정인수/길이·FILE·키 수명을 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_basefile_whole_read import WholeReadOracle, BoundaryFailure, ROOT, TEXT, STACK, STOP, ALLOCATION, KEY, digest
from decomp_file_name_leaf import add_ranges
from decomp_file_name_formatter import RING, INDEX
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_FPCW

POISON=0x65656565  # 지역File 미작성 필드의 독립 입력이며 원문 미정값의 동일성 주장이 아니다.
SIZE_SEED=0x76543210  #49c160이 wholeRead 전에 기존 전역 길이를 초기화하는지 구별한다.


class TypeReadOracle(WholeReadOracle):
    """TYPE 부모/실제formatter/wholeRead만 실행하고 FILE/메모리/진단 장치는 분리한다."""
    def __init__(self):
        """보호 PE의 기존 전체읽기 관측에 실제TYPE/formatter/strchr 범위만 더한다."""
        super().__init__();add_ranges(self,(0x49c160,0x459c60,0x4e63e0))

    def instruction(self,mu,address,size,data):
        """현재 길이/이름과 고정0인수를 원문 명령에서 읽고 실제 FILE cleanup을 유지한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x41bb53 and self.scenario==7: self.null_after_read=bytes(mu.mem_read(0,self.length))  # Read 반환 직후·File 소멸/FS0 복구 전의 실제 대상 물리다.
        if address==0x4e0620:self.events.append('D:'+str(args[2]));raise BoundaryFailure('assertion')
        if address==0x41baa0:
            assert args[1:]==(0,0x59ab44,0);self.entry=esp;self.object=esp-36;mu.mem_write(self.object,struct.pack('<6I',*([POISON]*6)));self.events.append('W:'+self.string(args[0]).hex()+':0:0:'+str(self.words(0x59ab44,1)[0]))
        if address==0x49c1b7:self.fileFields=','.join(map(str,self.fields()))
        if address==0x4e586e:
            target,length,count,file=args;actual=min(length*count,self.available)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes([101])*4096)
            self.events.append(f'R:{file}:{length}:{count}:{actual}')
            if self.scenario==4:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.payload[self.used:self.used+actual])
            self.used+=actual;self.available-=actual;self.position+=actual;self.ret(count if length and actual==length*count else 0);return
        super().instruction(mu,address,size,data)

    def sample(self,case,precision):
        """일반 합성TYPE 이름/내용만 입력하고 부모 저장·예외 이후 현재 물리를 반환한다."""
        name,payload,self.key,self.subfile,self.available,self.scenario=case;self.payload=bytes.fromhex(payload);self.length=len(self.payload);self.used=0;self.scratch=None;self.events=[];self.allocated=0;self.entry=0;self.fileFields=None;self.null_after_read=None
        self.mu.mem_write(STACK-0x10000,bytes([101])*0x10100);self.mu.mem_write(RING,bytes(2560));self.put(INDEX,8);self.put(0x532728,92,1);self.mu.mem_write(TEXT,bytes.fromhex(name)+b'\0');self.mu.mem_write(KEY,b'abc\0');self.put(0,0)
        self.put(0x59ab3c,0);self.put(0x59ab44,SIZE_SEED);self.put(0x59ab40,71);self.put(0x59a9e8,88);self.put(0x59ab34,TEXT+2000)
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,TEXT));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x49c160,STOP,count=500000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.entry and self.words(self.entry-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,self.entry);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4f93a0,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        if self.fileFields is None:self.fileFields=','.join(map(str,self.fields()))
        current=[self.words(address,1)[0] for address in (INDEX,0x59ab34,0x59ab3c,0x59ab44,0x59ab40,0x59a9e8)]  # parent가 쓴 각 전역을 독립 열로 보관한다.
        physical=bytes(self.mu.mem_read(ALLOCATION,self.allocated)).hex() if self.allocated else '-'
        return '\t'.join([str(value) for value in case]+[outcome]+[str(value) for value in current]+[self.fileFields,';'.join(self.events),physical])


def stored(payload,key):
    """파일 저장 입력만3BYTE 키로 만들며 기대값을 제품 구현에서 얻지 않는다."""
    return bytes(value^(b'abc'[index%3] if key else 0) for index,value in enumerate(payload)).hex()


def cases():
    """일반 이름/빈·짧은 본문/암호·부분FILE/각 실제 예외를 독립 입력으로 만든다."""
    result=[(b'neutral'.hex(),stored(payload,key),key,sub,len(payload),0) for payload,key,sub in itertools.product((b'',b'x',b'abc',b'typename Neutral {}'),range(2),range(2))]
    result += [(b'neutral'.hex(),stored(b'typename Neutral {}',key),key,sub,19,scenario) for scenario,key,sub in itertools.product((1,2,3,4,5,7),range(2),range(2))]
    result += [(b'neutral'.hex(),stored(b'typename Neutral {}',key),key,sub,3,0) for key,sub in itertools.product(range(2),range(2))]
    result += [(name.hex(),stored(b'abc',0),0,0,3,0) for name in (b'Name.WithDot',b'folder\\neutral',b'Neutral\0ignored')]
    return result


def main():
    """최초fixture나 독립 재관측만 쓰고 보호 원본/기존fixture를 덮어쓰지 않는다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--verify',action='store_true');p.add_argument('--null-fixture',type=Path);a=p.parse_args();assert not a.output.exists() and (a.fixture.exists() if a.verify else not a.fixture.exists())
    if a.null_fixture: assert not a.null_fixture.exists()  # 반환0의 대상 물리도 최초 바이트만 새 경로에 보관한다.
    oracle=TypeReadOracle();rows=['# nameHex payloadHex key subfile available scenario outcome index currentName currentBuffer currentSize offset line fileFields events allocationHex']
    null_rows=['# key subfile afterReadDestinationHex']  # allocator 예약 블록과 반환0의 실제 읽기 대상, 후속 FS0 복구를 구별한다.
    for case in cases():  # 두FPCW의 실제 명령 결과를 비교하며 제품 C++를 실행하지 않는다.
        first=oracle.sample(case,0x027f);null_first=oracle.null_after_read
        assert first==oracle.sample(case,0x037f);rows.append(first)
        if null_first is not None:  # 원문이 쓴0주소만 읽으며 반환하지 않은 ALLOCATION 블록으로 치환하지 않는다.
            assert null_first==oracle.null_after_read;null_rows.append(f'{case[2]}\t{case[3]}\t{null_first.hex()}')
    payload='\n'.join(rows)+'\n'
    if a.verify:assert a.fixture.read_text('utf-8')==payload
    else:a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(payload,encoding='utf-8',newline='\n')
    if a.null_fixture:  # 원래47행의 할당 블록 기대는 그대로 두고 추가 관측만 별도 저장한다.
        a.null_fixture.parent.mkdir(parents=True,exist_ok=True);a.null_fixture.write_text('\n'.join(null_rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=777,rows=len(rows)-1,observations=(len(rows)-1)*2,host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제49c160/459c60/strchr/41baa0와 marker0의4f93a0 cleanup. Open/CRT/new/진단은명시장치. TYPE scanner/parser/49c0f0 EOF·전체SEH dispatcher·24BYTE ABI·미정할당동일성은범위밖',sha256={name:digest(ROOT/name) for name in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv')});a.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=report['rows'],observations=report['observations'])))


if __name__=='__main__':main()
