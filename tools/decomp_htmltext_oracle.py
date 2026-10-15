#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 기본 HTML 순회/태그 몸체만 격리 실행해 StyleText 바이트 근거를 만든다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

# 보호 PE를 읽고 합성 객체/문자열/스택은 별도 Unicorn 공간에만 둔다.
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP

# 원본과 겹치지 않는 객체·본문·출력·스택·반환 표식 주소다.
OBJECT,SOURCE,OUTPUT,STACK_BASE,STACK,STOP=0x11000000,0x11100000,0x11200000,0x30000000,0x3000f000,0x20000000
FIXTURE=ROOT/'cpppj/tests/fixtures/htmltext-x86.tsv'
REPORT=ROOT/'cpppj/htmltext.json'
SPEC={
 'originals':dict(binary='originals/Netstorm.exe',ctor=0x46bea0,output=0x46bed0,input=0x46bf00,run=0x46c810,
     lower=0x4e53dc,suppress=0x565ec0,helpers=[0x4437f0,0x40b380],
     ranges=[(0x46bea0,0x46beca),(0x46bed0,0x46bef9),(0x46bf00,0x46bf0a),(0x46bf10,0x46bf37),(0x46bfc0,0x46c087),(0x46c090,0x46c761),(0x46c810,0x46c9b9),(0x4437f0,0x4437f1),(0x40b380,0x40b383)]),
 'originalCD':dict(binary='originalCD/NETSTORM.EXE',ctor=0x495270,output=0x4952a0,input=0x4952d0,run=0x4957b0,
     lower=0x4f2a30,suppress=0x536368,helpers=[0x495c60,0x495c70,0x495c80],
     ranges=[(0x495270,0x49529a),(0x4952a0,0x4952c9),(0x4952d0,0x4952da),(0x4952f0,0x495310),(0x495370,0x49541f),(0x495420,0x495720),(0x4957b0,0x495933),(0x495c60,0x495c85)])}
SAVED={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}


def digest(path):
    """근거의 현재 원본 바이트를 SHA-256으로 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cases():
    """서식·문자·경계·상태를 독립적으로 정한 입력을 만든다. 기대 변환은 계산하지 않는다."""
    result=[]
    # 실제 태그 첫 글자 case와 다음 literal 문자의 차이를 검사한다.
    tags=['b','B','br','BR','Br','bR','c','C','h','H','h1','h2','h3','h4','h9','i','I','p','P','q','Q','~j','~B','~','unknown','a','A','ahref="x"','Ahref="x"','aHref="x"','a href="x"','A href="x"','a Href="x"']
    for tag,close in itertools.product(tags,(False,True)):
        result.append([('pre<'+('/' if close else '')+tag+'>post').encode(),8192,1,0,0])
    # 행/공백/제어 바이트와 signed high byte를 원문 그대로 전달한다.
    texts=[b'',b'plain',b'a\nb',b'a \nb',b'a\n\nb',b'\r\t\x01\x1f \x80\xff',b'a\0later',b'<',b'>',b'<>',b'</>',b'<b',b'<b>bold</b>',b'walk <i>edge</i> and <b>open</b>.',b'<h1>Title</h1> <p>Body',b'<h1>Title</h1>X<p>Body',b'<p>a</p><p>b',b'</a>stop',b'<a>inside</a>after',b'<A>inside</A>after',b'<b<b>nested>after',b'<\x80>z',b'<\xff>z']
    for text,shown,hidden,suppress in itertools.product(texts,(0,1,0xffffffff),(0,2),(0,1)):
        result.append([text,8192,shown,hidden,suppress])
    # 평문은 strict > capacity-256 조건 때문에 경계에서 한 글자를 더 쓴다.
    for capacity,length in itertools.product((2,255,256,257,258,512,8192),(0,1,2,3,255,256,257,8000,8100)):
        result.append([b'x'*length,capacity,1,0,0])
    for text in (b'<h3>x</h3><p>y',b'<i>x</i><br>y',b'<p><p><p>',b'<~j>\xff<~.>'):
        result.append([text,512,1,0,0])
    # 같은 입력 조합은 한 번만 원본에 공급한다.
    return list(dict.fromkeys((text.hex(),capacity,shown,hidden,suppress) for text,capacity,shown,hidden,suppress in result))


class Oracle:
    """기본 VT와 변환 몸체만 실행하며 CRT ASCII 소문자 변환만 명시 ABI 경계로 공급한다."""
    def __init__(self,edition):
        """읽기 전용 PE를 독립 VM에 복제하고 실행/쓰기 범위를 제한한다."""
        self.spec=SPEC[edition]  # 현재 판본 함수와 전역 주소다.
        pe=pefile.PE(str(ROOT/self.spec['binary']));self.base=pe.OPTIONAL_HEADER.ImageBase;data=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(data)+4095)&~4095);self.mu.mem_write(self.base,data)
        # 합성 입력/출력은 실제 PE와 분리한다.
        for address,size in ((OBJECT,0x1000),(SOURCE,0x10000),(OUTPUT,0x10000),(STACK_BASE,0x10000),(STOP,0x1000)):self.mu.mem_map(address,size)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.instructions=0;self.returns=0;self.lower_arguments=set()  # 관찰 명령/최상위 반환/CRT signed 인자를 센다.

    def put(self,address,value):
        """합성 또는 명시 원본 전역 DWORD만 입력한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """현재 VM DWORD를 직접 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def instruction(self,mu,address,size,data):
        """정한 몸체 외 진입은 거부하고 tolower의 ASCII 계약만 공급한다."""
        if address==STOP:return
        if address==self.spec['lower']:
            stack=mu.reg_read(UC_X86_REG_ESP);value=struct.unpack('<i',mu.mem_read(stack+4,4))[0];self.lower_arguments.add(value)
            mu.reg_write(UC_X86_REG_EAX,(value+32 if 65<=value<=90 else value)&0xffffffff)
            mu.reg_write(UC_X86_REG_EIP,self.get(stack));mu.reg_write(UC_X86_REG_ESP,stack+4);return
        if not any(low<=address and address+size<=high for low,high in self.spec['ranges']):raise RuntimeError(f'미허용 native 진입 {address:08x}')
        self.instructions+=1

    def write(self,mu,access,address,size,value,data):
        """native 쓰기는 객체/출력/스택/명시 P전역만 허용한다."""
        if any(low<=address and address+size<=low+length for low,length in ((OBJECT,0x1000),(OUTPUT,0x10000),(STACK_BASE,0x10000),(self.spec['suppress'],4))):return
        raise RuntimeError(f'미허용 native 쓰기 {address:08x}+{size}')

    def call(self,address,args):
        """실제 thiscall의 스택 정리와 비휘발 레지스터 보존을 확인한다."""
        mu=self.mu
        # 매 최상위 호출의 비휘발 입력을 고정한다.
        for reg,value in SAVED.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(address,STOP,count=2000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4*(len(args)+1):raise RuntimeError('native 정상반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in SAVED.items()):raise RuntimeError('native 보존 레지스터 오류')

    def run(self,case):
        """생성/출력설정/입력설정/순회를 모두 실제 몸체로 수행하고 최종 필드를 저장한다."""
        text,capacity,shown,hidden,suppress=case;mu=self.mu;s=self.spec
        mu.mem_write(SOURCE,bytes.fromhex(text)+b'\0\0\0');mu.mem_write(OBJECT,b'\xcc'*0x100);mu.mem_write(OUTPUT,b'\xcd'*0x10000)
        self.call(s['ctor'],[]);self.call(s['output'],[OUTPUT,capacity,0,0]);self.call(s['input'],[SOURCE])
        self.put(OBJECT+0x1c,shown);self.put(OBJECT+0x20,hidden);self.put(s['suppress'],suppress);self.call(s['run'],[])
        length=self.get(OBJECT+8)-OUTPUT
        if length>=capacity or bytes(mu.mem_read(OUTPUT+length,1))!=b'\0':raise RuntimeError('실제 출력 물리 범위/NUL 오류')
        self.returns+=1
        return [bytes(mu.mem_read(OUTPUT,length)).hex(),self.get(OBJECT+4)-SOURCE,*[self.get(OBJECT+x) for x in (0x1c,0x20,0x24,0x28,0x2c)],self.get(s['suppress'])]


def generate():
    """C++를 읽거나 실행하지 않고 원본의 두 판본 관찰 결과를 저장한다."""
    rows=[];counts={};instructions={};arguments={}
    # 두 판본 전역/가상 표를 섞지 않고 각 입력을 실행한다.
    for edition in SPEC:
        oracle=Oracle(edition)
        # 변환 기대값은 오직 실제 출력 버퍼/필드에서 읽는다.
        for case in cases():rows.append([edition,*case,*oracle.run(case)])
        counts[edition]=oracle.returns;instructions[edition]=oracle.instructions;arguments[edition]=sorted(oracle.lower_arguments)
    FIXTURE.write_text('# 두 PE 기본 HTML→StyleText 실제 바이트. link 저장소0; tolower ASCII ABI 경계; 게임/OS 실행 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),FIXTURE,*[ROOT/s['binary'] for s in SPEC.values()]]
    report={'host':socket.gethostname(),'date':'2026-10-15','unique_inputs':len(rows),'returns':counts,'instructions':instructions,'tolower_arguments':arguments,
            'boundaries':['CRT tolower: signed input/ASCII mapping; nonASCII locale behavior unclaimed'],'link_storage':0,'os_calls':0,'spec':SPEC,'files':{p.relative_to(ROOT).as_posix():digest(p) for p in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'inputs':len(rows),'returns':counts},indent=2))


def verify():
    """실제 입력 계획/원본/생성기/fixture와 집계를 재실행 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 근거 파일의 바이트 해시를 고정한다.
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError('근거 SHA 불일치 '+path)
    plan=[[edition,*case] for edition in SPEC for case in cases()]
    if len(rows)!=len(plan) or report['unique_inputs']!=len(rows) or report['os_calls']:raise RuntimeError('입력/반환 집계 오류')
    # 저장한 입력은 실제 실행 전에 정한 순서와 같아야 한다.
    for row,case in zip(rows,plan):
        if row[:6]!=list(map(str,case)):raise RuntimeError('입력 계획 불일치')
    if any(value!=len(cases()) for value in report['returns'].values()):raise RuntimeError('정상반환 집계 오류')
    print('HTML 독립 근거 감사 통과',len(rows))


def main():
    """새 native 관찰과 저장 근거 감사만 구분하여 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()


if __name__=='__main__':main()
