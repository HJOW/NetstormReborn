#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제4c5380→41baa0·전체read·cleanup funclet과 LF/NUL/성공 캐시를 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_quick_help_window import Oracle as WindowOracle, ROOT, WINDOW, SOURCE, TYPES, STOP, digest
from decomp_priestmenu import PLAYERS, TEXT, STACK
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

KEY=TYPES+0x18000  # GUI/TYPE 입력과 겹치지 않는 명시 키 할당 물리다.
POISON=0x65656565  # 미작성 지역 File 네 필드의 명시 입력이며 원문 스택 미정값의 재현 주장은 아니다.


class BoundaryFailure(Exception):
    """Open/new/read/close에서 멈춘 실제 예외 진입의 종류를 보존한다."""


class HelpReadOracle(WindowOracle):
    """전체 Help 생성자와 실제41baa0을 함께 실행하며 GUI와 FILE/new만 명시한다."""
    def __init__(self):
        """보호 이미지는 읽기 전용으로 가져오고 추가 허용 범위만 설치한다."""
        self.active=False
        super().__init__()
        ranges(self,(0x41baa0,0x41aa80,0x41a7a0,0x41b410,0x41a680,0x4f93a0))

    def fields(self):
        """현재 실제 지역 File의 여섯 DWORD를 읽는다."""
        return ','.join(map(str,self.words(self.object,6))) if self.object else '-'

    def instruction(self,mu,address,size,data):
        """원문 부모/읽기/반납은 실제 명령으로 실행하고 선택된 장치 인자만 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        args=self.words(esp+4,4)
        if address==0x41baa0:
            assert args[1]==0 and args[3]==2
            self.active=True;self.entry=esp;self.object=esp-36;self.calls+=1
            mu.mem_write(self.object,struct.pack('<6I',*([POISON]*6)))
            self.fileEvents.append('W:'+self.string(args[0]).hex()+':'+str(args[1])+':'+str(args[3]))
        if address==0x4c580e:self.lastFields=self.fields();self.active=False
        if address==0x41ab10:raise RuntimeError('필수 Help 읽기가 사전Probe를 실행함')
        if address==0x41b4c0:
            assert self.active and mu.reg_read(UC_X86_REG_ECX)==self.object and args[1:3]==(0,1)
            self.fileEvents.append('O:'+self.string(args[0]).hex()+':0:1:'+self.fields())
            mu.mem_write(self.object,struct.pack('<6I',0 if self.scenario==2 else PLAYERS,0x80|self.subfile,37 if self.subfile else 0,self.length if self.subfile else 0,KEY if self.key else 0,3 if self.key else 0))
            self.position=37 if self.subfile else 0
            if self.scenario==1:raise BoundaryFailure('open')
            self.ret(0,12);return
        if address==0x4e4391 and self.active:
            self.fileEvents.append('N:'+str(args[0]))
            if self.scenario==3:raise BoundaryFailure('allocation')
            self.allocation=args[0];mu.mem_write(SOURCE,bytes([205])*self.allocation);self.ret(SOURCE);return
        if address==0x4e566c:
            self.fileEvents.append('T:'+str(args[0]));self.ret(self.position);return
        if address==0x4e573c:
            self.fileEvents.append('S:'+':'.join(map(str,args[:3])));self.position=self.length if args[2]==2 else args[1];self.ret(0);return
        if address==0x4e586e:
            target,length,count,file=args;actual=min(length*count,self.available)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes([101])*4096)
            self.fileEvents.append(f'R:{file}:{length}:{count}:{actual}')
            if self.scenario==4:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,bytes(((self.used+n)*29+7)&255 for n in range(actual)))
            self.used+=actual;self.available-=actual;self.position+=actual;self.ret(count if length and actual==length*count else 0);return
        if address==0x4e54b9:
            self.fileEvents.append('C:'+str(args[0]))
            if self.scenario==5:raise BoundaryFailure('close')
            self.ret(0);return
        if address==0x4e4354:self.fileEvents.append('F:'+str(args[0]));self.ret();return
        if address==0x4c2630:
            self.fileEvents.append('E:'+str(args[0]));self.ret();return
        super().instruction(mu,address,size,data)

    def constructor(self,precision):
        """실제 함수 시작에서 실행하고 정상 반환의 stack/비휘발/x87 계약을 확인한다."""
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        for register,value in saved.items():self.mu.reg_write(register,value)  # 호출 전 독립 비휘발 입력을 설치한다.
        self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,WINDOW)
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,0));self.mu.emu_start(0x4c5380,STOP,count=3000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        assert all(self.mu.reg_read(register)==value for register,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==precision

    def sample(self,case,precision):
        """입력만 공급하고 캐시 전체 바이트·장치 순서·실제 부분 File 반납을 반환한다."""
        self.prepare();self.length,self.key,self.subfile,self.available,self.scenario,cached,repeat=case
        self.active=False;self.object=0;self.entry=0;self.fileEvents=[];self.allocation=0;self.used=0;self.scratch=None;self.calls=0;self.lastFields='-'
        self.mu.mem_write(KEY,b'abc\0');self.mu.mem_write(SOURCE,b'OLD\n\0'+bytes([205])*8192);self.put(0x5ca994,SOURCE if cached else 0)
        outcome='ok'
        try:
            for _ in range(repeat):self.allocated={};self.constructor(precision)  # 새 명시 GUI 할당 입력만 다시 공급하고 Help 전역 캐시는 유지한다.
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.words(self.entry-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,self.entry);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4f93a0,STOP,count=10000)
        cachedNow=bool(self.words(0x5ca994,1)[0]);logical=bytes(self.mu.mem_read(SOURCE,4 if cached else self.length+1)).hex() if cachedNow else '-'
        physical=bytes(self.mu.mem_read(SOURCE,self.allocation)).hex() if self.allocation else '-'
        return '\t'.join([str(x) for x in case]+[outcome,str(int(cachedNow)),logical,physical,self.lastFields if outcome=='ok' else self.fields(),';'.join(self.fileEvents) or '-',str(self.calls)])


def cases():
    """길이0/짧은/4096경계·암호·부분FILE·실패·기존/새 캐시의 독립 입력을 만든다."""
    result=[(n,key,sub,n,0,0,2) for n,key,sub in itertools.product((0,1,2,4,9,4097),range(2),range(2))]
    result += [(n,key,sub,n//2,0,0,1) for n,key,sub in itertools.product((1,4,4097),range(2),range(2))]
    result += [(4,key,sub,4,scenario,0,1) for scenario,key,sub in itertools.product((1,2,3,4,5),range(2),range(2))]
    result += [(4,1,1,4,scenario,1,repeat) for scenario,repeat in itertools.product((0,1,4),(1,2))]
    return result


def main():
    """새 준비 fixture만 생성하거나 원문을 재관측해 바이트 일치를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=HelpReadOracle();rows=['# length key subfile available scenario cached repeat outcome cachedNow logicalHex allocationHex fields events calls']
    for case in cases():  # 제품 기대를 계산하지 않고 두 x87 정밀도의 실제 결과를 비교한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    payload='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text('utf-8')==payload
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(payload,encoding='utf-8',newline='\n')
    report=dict(unit=767,host=socket.gethostname(),rows=len(rows)-1,observations=(len(rows)-1)*2,gameExecutions=0,multiplayerTests=0,lastFullDecompilation='HJOW-Athlon / 2026-10-23',boundary='실제4c5380→41baa0/길이/읽기/소멸·LF/NUL·같은성공cache. GUI/Config/Open/FILE/new는명시입력. 예외에는41baa0 marker0일때실제4f93a0만호출하며외부창ctor SEH/부분GUI 소멸을실행하지않음. 부분암호scratch는명시101seed이며미정스택동일성주장아님',sha256={name:digest(ROOT/name) for name in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv')})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=report['rows'],observations=report['observations'])))


if __name__=='__main__':main()
