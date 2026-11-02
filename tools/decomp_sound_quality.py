#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""음질 일반명령의 실제 CRT 서식과 옵션 장치/음량 사슬을 게임 없이 격리 관측한다."""
import csv
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  #같은 호스트의 보호 PE는 읽기만 한다.
OUT = None  # 명시 --output으로 새 원시 디렉터리를 지정하기 전에는 파일을 쓰지 않는다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle as BaseOracle, STACK, SCRATCH
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
STACK-=0x3000  #원문 지역 서식버퍼가 기존 가상 스택 매핑 안에 들어가도록 한다.


class Oracle(BaseOracle):
    """명령/정수 CRT/장치 분기를 실행하며 OS·Config 본체·PCM 효과는 기록 경계다."""
    def __init__(self):
        """원문 포맷터와 CRT의 정밀 함수 범위를 읽는다."""
        super().__init__('originals',[dict(name='unused',originals='004d62b0')],helpers=[])
        self.allowed=[(0x4d6eca,0x4d6ed6),(0x4d7049,0x4d70ba),(0x4355df,0x435680)]
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for address in (0x4a89c0,0x4de490,0x4e7990,0x4eaac6,0x4e8790,0x4eaa38,0x4eaa6b,0x4eaa8f,0x4f0d0e,0x4f3cb0):  #signed%d의 실제 자식이며 __aulldvrm의 순수 정수 나눗셈도 실행한다.
            for part in facts[address]['ranges'].split(';'):
                lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def word(self,address,value=None):
        """명시 입력/출력 DWORD를 원시 비트로 읽고 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def guard(self,mu,access,address,size,value,data):
        """원문 쓰기를 검토된 지역 스택과 포맷터 scratch에 한정한다."""
        assert STACK-0x1000<=address and address+size<=STACK+0x3000 or 0x5e18b8<=address and address+size<=0x5e22b8,(hex(address),size)

    def returned(self,value=0,purge=0):
        """외부 cdecl/명시 API의 반환만 수행하며 호출자 정리는 원문에 둔다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,self.word(sp))

    def on_instruction(self,mu,address,size,data):
        """종료·Config 실제 서식·음향 인자와 옵션 부모의 호출 순서를 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address in (0x4d7b1a,0x4da477):self.gate='release' if address==0x4da477 else 'command';mu.emu_stop();return
        if address==0x4e6fa1:
            value=self.word(sp+4);self.returned(4 if 48<=value<=57 else 0);return #숫자 문자 분류만 TLS 없는 ASCII 외부 경계다.
        if address==0x4a8ef0:self.trace.append('Close');self.returned();return
        if address==0x441580:self.trace.append('Config:'+self.read_string(self.word(sp+4)).hex());self.returned();return
        if address==0x4a9cb0:
            args=[self.word(sp+4+i*4) for i in range(6)];assert args[1:]==[0]*5;self.trace.append('Sound:'+self.read_string(args[0]).hex());self.returned();return
        if address==0x4aa600:self.trace.append('Open:'+str(self.word(sp+4)));self.returned(self.open_result);return
        if address in (0x4a9f10,0x4aa5d0):self.trace.append(('EffectVolume:' if address==0x4a9f10 else 'MusicVolume:')+str(self.word(sp+4)));self.returned();return
        if address==0x435160:self.trace.append('Music:'+str(self.word(sp+8)));self.returned();return
        return super().on_instruction(mu,address,size,data)

    def reset(self):
        """이전 반복의 스택/레지스터/효과와 시계 sentinel을 분리한다."""
        self.trace=[];self.gate='-';self.open_result=0
        self.mu.mem_write(STACK-0x1000,bytes(0x4000))
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(0x565e68,bytes.fromhex('efcdab8967452301')) #같은 songEnd의 원시8BYTE를 시작 전에만 명시한다.

    def command(self,event,value):
        """공통 release 뒤 원문 음질 명령/실제 signed%d CRT를 관측한다."""
        self.reset();self.word(STACK+0x14,event);self.word(STACK+0x1c,value)
        self.mu.emu_start(0x4d6eca,0x4d6ed6,count=10000)
        if self.gate!='release':
            self.mu.reg_write(UC_X86_REG_EBP,event&0x3ffffff);self.mu.emu_start(0x4d7049,0,count=100000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK
        assert bytes(self.mu.mem_read(0x565e68,8)).hex()=='efcdab8967452301'
        return ';'.join(self.trace) or '-',self.gate

    def options(self,sound,quality,effect,music,music_on,opened):
        """최초해석531930=0 뒤의 장치→두 음량→현재 곡 분기를 관측한다."""
        self.reset();self.open_result=opened;self.mu.reg_write(UC_X86_REG_EAX,sound)
        for address,value in ((0x531930,0),(0x5318fc,quality),(0x531900,effect),(0x531904,music),(0x531880,music_on)):self.word(address,value)
        self.mu.emu_start(0x4355df,0x435680,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK-12 #음악음량1개와현재곡2개인자는원문435680뒤바람음인자와합쳐정리된다.
        assert bytes(self.mu.mem_read(0x565e68,8)).hex()=='efcdab8967452301'
        return ';'.join(self.trace)


def main():
    """독립 명시 입력을 두 번 실행해 새 두 TSV와 원시 경계를 보관한다."""
    global OUT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True,help='기존 파일을 덮어쓰지 않을 새 원시 디렉터리')
    OUT=parser.parse_args().output.resolve()
    if OUT.exists(): raise FileExistsError(OUT)
    OUT.mkdir(parents=True)
    oracle=Oracle();commands=[];options=[]
    # release/수식과 signed 값의 독립 조합을 원문으로 두 번 관측한다.
    for value,flags in itertools.product((0,1,2,3,4,0xffffffff,0x80000000,0x7fffffff),(0,0x4000000,0x7c000000,0x80000000)):
        event=0x20000dd|flags;result=oracle.command(event,value);assert result==oracle.command(event,value);commands.append((event,value,*result))
    # 장치 자연 실패와 음량/음악 조합의 순서를 두 번 관측한다.
    for values in itertools.product((0,1),(0,3,0xffffffff),(0,1,3,5),(1,4),(0,1),(0,1)):
        result=oracle.options(*values);assert result==oracle.options(*values);options.append((*values,result))
    # 새 관측 디렉터리에 두 fixture와 원시 보고서를 보관한다.
    for name,header,rows in [('sound-quality-command-x86.tsv','# event value trace gate',commands),('audio-reinterpret-x86.tsv','# sound quality effectStep musicStep musicOn openResult trace',options)]:
        (OUT/name).write_text(header+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),'utf-8',newline='\n')
    report=dict(unit=764,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),commandInputs=len(commands),optionInputs=len(options),nativeExecutions=2*(len(commands)+len(options)),gameExecutions=0,multiplayerTests=0,codeSha256={f'{a:08x}..{b-1:08x}':hashlib.sha256(bytes(oracle.mu.mem_read(a,b-a))).hexdigest() for a,b in oracle.allowed},limitations=['Config와OS/장치/음량/음향몸체는호출경계이며signed정수서식은실제CRT다.','옵션전역ReadInt앞단/최초해석531930=1/바람음선택은이번관측밖이다.','실제장치지원/PCM·게임/MP관측이아니다.'],commands=commands,options=options)
    (OUT/'sound-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('commandInputs','optionInputs','nativeExecutions','gameExecutions','multiplayerTests')}))


if __name__=='__main__':main()
