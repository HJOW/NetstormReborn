#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 TYPE HP getter의 현재 번호/진단 복귀/부호 있는1/4 값을 격리 관찰한다."""
import argparse
import collections
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, STACK, STOP
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS

TYPES=0x12000000  # 게임/OS와 분리한 유효 TYPE this의 물리 입력 영역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/rifttype-hit-points-x86.tsv'  # 최초 입력/원문 반환과 후상태를 고정한다.
SPEC={  # 현재 정밀 산출물/보호 PE에서 직접 대조한 실제 함수/전역이다.
    'originals':dict(entry=0x49fce0,helpers=[],base=0x59ab20,stride=500,quarter=0x5ca8f0,priest=0x5412d0,dais=0x5412ec,checking=0x5e4794,assertion=0x4e0620),
    'originalCD':dict(entry=0x444720,helpers=[0x4448e0],base=0x51c960,stride=468,quarter=0x511c90,priest=0x51cbbc,dais=0x51cbd8,checking=0x540a24,assertion=0x490040)}


def sha(path):
    """실제 원본/도구/고정 입력의 같은 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class HitPointOracle(Oracle):
    """실제 정수 명령을 실행하며 진단의 외부 UI/복귀만 명시 입력 경계로 둔다."""
    def __init__(self,edition):
        """현재 refined의 함수 범위와 격리 TYPE/스택만 준비한다."""
        self.s=SPEC[edition]  # 현재 판본의 함수/전역 주소다.
        super().__init__(edition,[dict(name='hp',**{edition:f'{self.s["entry"]:08x}'})],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x20000)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.number_reads=0;self.diagnostics=[]  # 실제 base 읽기 횟수와 외부 진단 인자다.
        self.profile=0;self.selected=0;self.shift=0  # 현재 입력의 복귀 프로필/저장 this/현재 표 기준 이동이다.

    def put(self,address,value):
        """입력 정수를 원문 DWORD 비트로 직접 저장한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address,signed=False):
        """원본 반환/후상태를 계산 없이 같은 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little',signed=signed)

    def on_read(self,mu,access,address,size,value,data):
        """getNum의 실제 현재 base 읽기만 세며 TYPE 선택 횟수와 혼동하지 않는다."""
        if address==self.s['base']:
            self.number_reads+=1

    def on_write(self,mu,access,address,size,value,data):
        """원문 getter는 스택 밖 쓰기를 하지 않으므로 다른 프로그램 쓰기는 거부한다."""
        if not (0x30000000<=address and address+size<=0x30010000):
            raise RuntimeError(f'HP getter의 스택 밖 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """선택된 원문 진단의 정상 복귀 중 명시한 전역/물리 TYPE 변경만 공급한다."""
        if address==self.s['assertion']:
            sp=mu.reg_read(UC_X86_REG_ESP)
            self.diagnostics.append(self.read_string(self.get(sp+4)).decode('ascii')+'|'+self.read_string(self.get(sp+8)).decode('ascii')+'|'+str(self.get(sp+12)))
            if self.profile:
                self.put(self.s['quarter'],0x80000000);self.put(self.s['checking'],0)
                self.put(self.selected,(-13,-37,41)[self.profile-1])
                self.shift=(0,1,-1)[self.profile-1]
                self.put(self.s['base'],TYPES+self.shift*self.s['stride'])
                self.put(self.s['priest'],74-self.shift)
            mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.get(sp));return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """기대값을 만들지 않고 실제 명령의 반환과 후상태를 읽는다."""
        number,hp,flags,checking,quarter,priest,dais,shift,profile=case
        self.profile,self.shift=profile,shift;self.selected=TYPES+number*self.s['stride'];self.number_reads=0;self.diagnostics=[]
        self.put(self.selected,hp);self.put(self.selected+0xe8,flags)
        for address,value in [(self.s['base'],TYPES+shift*self.s['stride']),(self.s['checking'],checking),(self.s['quarter'],quarter),(self.s['priest'],priest),(self.s['dais'],dais)]:  # 입력 전역의 폭/위치를 그대로 사용한다.
            self.put(address,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 실행의 레지스터를 새 입력으로 숨기지 않는다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,self.selected);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.put(STACK,STOP);self.mu.emu_start(self.s['entry'],STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        returned=struct.unpack('<i',struct.pack('<I',self.mu.reg_read(UC_X86_REG_EAX)))[0]
        post=','.join(map(str,(self.get(self.selected,True),self.get(self.s['checking']),self.get(self.s['quarter']),self.get(self.s['priest']),self.get(self.s['dais']),self.shift)))
        return returned,';'.join(self.diagnostics) or '-',self.number_reads,post


def inputs():
    """부호 있는HP/전체DWORD비영·HP비트·같음/상위비트다름을 출력과 독립으로 교차한다."""
    cases=[]
    for number,quarter,checking,has_hp,priest_equal,dais_equal,hp in itertools.product((0,74,158),(0,0x80000000),(0,0x80000000),(0,16),(0,1),(0,1),(-2147483648,-7,-1,0,7,2147483647)):  # 논리 번호 하위BYTE만 비교하는 오류도 구별한다.
        cases.append((number,hp,has_hp,checking,quarter,number if priest_equal else number|0x10000,number if dais_equal else number|0x10000,0,0))
    cases.extend((74,29,0,0x80000000,0,158,165,0,profile) for profile in (1,2,3))
    cases.append((0,-7,0,0x80000000,0x80000000,0xffffffff,0xffffffff,1,0))
    return cases


def main():
    """최초 fixture만 생성하거나 원문을 다시 관측해 같은 내용인지 검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    rows=['# 실제 양판본 TYPE HP getter. 진단 UI는 명시 복귀 경계이며 게임/의식/공중3종/MP는 실행하지 않는다.'];reports={}
    cases=inputs()
    for edition in SPEC:  # 실제 각 보호 PE의 독립 명령을 실행한다.
        oracle=HitPointOracle(edition);count=collections.Counter()
        for case in cases:  # 같은 입력의 모든 반환/진단/후상태를 원문에서 직접 기록한다.
            result=oracle.observe(case);count[result[1]!='-']+=1
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*result))))
        reports[edition]=dict(samples=len(cases),withDiagnostic=count[True],withoutDiagnostic=count[False],binarySha256=sha(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')),refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'))
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=reports,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),substitutedGetterFunctions=0)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['mode'],report['samples'],'HP 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
