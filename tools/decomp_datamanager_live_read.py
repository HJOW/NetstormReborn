#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제0044d980→41baa0의 선택존재/최소4·서명/본문/현재할당 반납을 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_basefile_whole_read import WholeReadOracle, BoundaryFailure, ROOT, PLAYERS, TYPES, TEXT, STACK, STOP, ALLOCATION, KEY, digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW

MANAGER,TABLE,NAME,NAME_TABLE,NAMES=0x12050000,0x12060000,STOP+0x100,STOP+0x200,TYPES+0x1f000  # 실제 가상 이름 getter만 명시하는 격리 manager/표/호출 경계다.
POISON=0x65656565  # 지역File 미작성 필드의 독립 seed다.


class DataManagerOracle(WholeReadOracle):
    """실제 부모/전체read/Template 서명/두 cleanup을 실행하고 큰 섹션 본문은 실제 빈 이름표 경로까지 실행한다."""
    def __init__(self):
        """보호 PE와 기존 전체읽기 장치를 빌리며 게임/OS API는 실행하지 않는다."""
        super().__init__();add_ranges(self,(0x44d980,0x4bce10,0x44d040,0x4fa070,0x44d380,0x44cdb0,0x4a8800,0x4a8850))

    def instruction(self,mu,address,size,data):
        """원문 가상 getter/본문 및 FILE/new 효과의 순서를 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==NAME:self.events.append('N:'+self.string(TEXT).hex());self.ret(TEXT);return
        if address==NAME_TABLE:self.ret(NAMES);return
        if address==0x4e0620:self.events.append('D:'+str(args[2]));raise BoundaryFailure('assertion')
        if address==0x41baa0:
            assert args[1]==1 and args[3]==0;self.entry=esp;self.object=esp-36;mu.mem_write(self.object,struct.pack('<6I',*([POISON]*6)));self.events.append('W:1:0')
        if address==0x44d9cc:self.fileFields=','.join(map(str,self.fields()))
        if address==0x41ab10:self.events.append('P:'+self.string(args[0]).hex());self.ret(self.probe);return
        if address==0x4bce10:
            self.source=args[0];self.events.append('G:'+','.join(map(str,self.words(self.source,5))))
            if self.scenario==6:raise BoundaryFailure('signature')
        if address==0x44d380:
            assert mu.reg_read(UC_X86_REG_ECX)==MANAGER;self.events.append('B:'+','.join(map(str,self.words(args[0],5))))
            if self.scenario in (7,9):raise BoundaryFailure('body')
        if address==0x41aef0:self.events.append('X:'+self.string(args[0]).hex());self.ret(0);return
        if address==0x4e586e:
            target,length,count,file=args;actual=min(length*count,self.available)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes([101])*4096)
            self.events.append(f'R:{file}:{length}:{count}:{actual}')
            if self.scenario==4:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.payload[self.used:self.used+actual])
            self.used+=actual;self.available-=actual;self.position+=actual;self.ret(count if length and actual==length*count else 0);return
        if address==0x4e4354 and args[0]==ALLOCATION:
            self.events.append('A:'+str(args[0]))
            if self.scenario==8:raise BoundaryFailure('release')
            self.ret();return
        super().instruction(mu,address,size,data)

    def sample(self,case,precision):
        """동일 MarkStream20·가상서명·현재포인터 free를 실제 부모 결과에서 읽는다."""
        payload,self.key,self.subfile,self.probe,available,self.scenario=case;self.payload=bytes.fromhex(payload);self.length=len(self.payload);self.available=available;self.used=0;self.scratch=None;self.events=[];self.allocated=0;self.entry=0;self.source=0;self.fileFields=None
        self.mu.mem_write(STACK-0x10000,bytes([101])*0x10100);self.mu.mem_write(MANAGER,bytes(276));self.put(MANAGER,TABLE);self.put(MANAGER+0x110,0xffffffff);self.put(TABLE,NAME);self.put(TABLE+4,NAME_TABLE);self.put(TABLE+12,0x4bce10);self.mu.mem_write(NAMES,b"//\0");self.put(0x541348,188);self.mu.mem_write(TEXT,b'input.fort\0');self.mu.mem_write(KEY,b'abc\0');self.put(0,0)
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,MANAGER);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x44d980,STOP,count=500000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.entry and self.words(self.entry-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,self.entry);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4f93a0,STOP,count=10000)
            if self.fileFields is None:self.fileFields=','.join(map(str,self.fields()))
            if self.words(STACK-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4fa070,STOP,count=10000)
        result=self.mu.reg_read(UC_X86_REG_EAX) if outcome=='ok' else 0
        physical=bytes(self.mu.mem_read(ALLOCATION,self.allocated)).hex() if self.allocated else '-'
        return '\t'.join([str(x) for x in case]+[outcome,str(result),str(self.words(MANAGER+0x10c,1)[0]),str(self.words(MANAGER+0x110,1)[0]),self.fileFields or '-',';'.join(self.events),physical])


def stored(payload,key):
    """독립 plaintext 입력을 명시3BYTE 키의 저장파일 바이트로 만든다."""
    return bytes(value^(b"abc"[index%3] if key else 0) for index,value in enumerate(bytes.fromhex(payload))).hex()


def cases():
    """없음/빈파일/서명과 부분read·각 예외 및 최소4 할당의 독립 입력이다."""
    result=[(stored(payload,key),key,sub,2,len(bytes.fromhex(payload)),0) for payload,key,sub in itertools.product(('', '46fe','58fe','46','460002000200'),range(2),range(2))]
    result += [(stored('46fe',key),key,sub,0,2,0) for key,sub in itertools.product(range(2),range(2))]
    result += [(stored('46fe',key),key,sub,2,2,scenario) for scenario,key,sub in itertools.product((1,2,3,4,5,6,7,8),range(2),range(2))]
    result += [(stored('460002000200',key),key,sub,2,3,0) for key,sub in itertools.product(range(2),range(2))]
    return result


def body_cases():
    """기존7번mallocNULL과겹치지않는9번으로본문예외입력만별도관측한다."""
    return [(stored('46fe',key),key,sub,2,2,9) for key,sub in itertools.product(range(2),range(2))]


def main():
    """새 준비 fixture를 생성하거나 원문 재관측만 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');parser.add_argument('--body-only',action='store_true',help='기존60행과분리한본문예외4입력');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=DataManagerOracle();rows=['# payloadHex key subfile probe available scenario outcome result signature flag fileFields events allocationHex']
    for case in (body_cases() if args.body_only else cases()):  # 두 정밀도의 결과를 제품 구현 없이 실제 명령으로 대조한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    payload='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text('utf-8')==payload
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(payload,encoding='utf-8',newline='\n')
    report=dict(unit=769,scenarioInterpretation='7=inherited WholeRead allocationNULL with explicit mapped zero-page;9=separate body exception',rows=len(rows)-1,observations=(len(rows)-1)*2,host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제44d980→41baa0/Template4bce10/FILE·키반납 및관측marker0의실제4f93a0/4fa070→44d040 cleanup.41b4c0/Probe/CRT는독립장치,44d380/44cdb0/SNART는실제빈이름표경로이며큰섹션노드할당분기는별도검사범위.암호원문scratch는명시101seed.전체SEH dispatcher/할당ABI/전체35섹션파서는범위밖',sha256={name:digest(ROOT/name) for name in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv')})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=report['rows'],observations=report['observations'])))


if __name__=='__main__':main()
