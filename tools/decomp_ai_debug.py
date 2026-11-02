#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""선택적 AI 메뉴와 두 진단 writer의 실제 x86을 게임 없이 격리 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 같은 호스트의 보호 PE/정밀 함수표는 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle as BaseOracle,SCRATCH,STACK,STOP
from unicorn.x86_const import *

PLAYERS=SCRATCH  # 정상 아홉 Player의 독립 격리 입력 표다.
BODY=SCRATCH+0x2000  # 같은 owner의 재조회 교체를 구별할 두 실제0xb08 본체다.
EVENT=SCRATCH+0x6000  # 413d50이 참조할 세 DWORD 입력이다.
FORMATTED=SCRATCH+0x7000  # 실제 서식기 대신 수명 안정된 명시 반환 문자열이다.
DEBUG=(0x52f3cc,0x52f3e4,0x52f3d0,0x52f3d4,0x52f3d8,0x52f3dc,0x52f3e0)  # 메뉴 작성 순서의 실제 진단 DWORD다.
FIELDS=(0x258,0x2d8,0x2d4,0xb00,0x220,0x228,0x230,0x234,0x24c)  # decimal600 초기화와 hex600 별도필드를 혼동하지 않는다.


class Oracle(BaseOracle):
    """원문 메뉴/직접 writer만 실행하고 GUI/서식기 하위는 인자 기록 경계다."""
    def __init__(self):
        """실제 정밀 두 함수 범위와 PE 초기 명령/문자열을 읽는다."""
        super().__init__('originals',[dict(name='AiDebugMenu',originals='00413dd0'),dict(name='AiDebugInput',originals='00413d50')],helpers=[])
        self.menu=False

    def word(self,address,value=None):
        """격리 메모리 DWORD 원시비트를 조회하거나 명시 입력으로 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def string(self,address):
        """정상 NUL까지의 실제 좁은문자열만 관측한다."""
        data=bytearray()
        while len(data)<4096:  # 지정 경계 밖 무한 문자열 읽기는 거부한다.
            value=bytes(self.mu.mem_read(address+len(data),1))[0]
            if not value:return bytes(data)
            data.append(value)
        raise AssertionError('NUL 없는 관측 문자열')

    def ret(self,value=0):
        """cdecl 호출만 자연 반환하고 인자 정리는 실제 호출자에 남긴다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.word(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4)

    def emit(self,*parts):
        """원문 효과의 시간 순서를 독립 문자열 trace로 기록한다."""
        self.events.append(':'.join(map(str,parts)))

    def on_instruction(self,mu,address,size,data):
        """GUI/서식/하위 메뉴 인자와 독립 callback mutation을 기록한다."""
        if self.menu:
            args=[self.word(mu.reg_read(UC_X86_REG_ESP)+4+4*i) for i in range(6)]
            if address==0x478020:
                label=self.string(args[0]);self.emit('H',label.hex())
                if self.mutation==1 and label==b'NOT INITIALIZED\n':self.word(PLAYERS+self.owner*180+0xb0,BODY+0x1000)
                self.ret();return
            if address==0x4b9390:
                pattern=self.string(args[1]);value=self.string(args[2]).hex() if b'%s' in pattern else str(args[2]);self.emit('F',args[0],pattern.hex(),value)
                self.formats+=1;result=f'F{self.formats}'.encode();self.mu.mem_write(FORMATTED,result+b'\0')
                if self.mutation==2 and self.formats==1:self.word(PLAYERS+self.owner*180+0xb0,BODY+0x1000)
                self.ret(FORMATTED);return
            if address==0x477dd0:
                self.emit('C',args[0],self.string(args[1]).hex(),*args[2:6]);self.checks+=1
                if self.mutation==4 and self.checks==9:self.word(self.captured+0x220,0x3f800000)
                self.ret();return
            if address==0x477bb0:
                self.emit('B',args[0]);self.ret(0x300+self.taken);return
            if address==0x475c00:
                self.taken+=1;self.emit('T',self.taken);self.ret(0x700+self.taken);return
            if address==0x478160:
                label=self.string(args[0]);self.emit('S',label.hex(),*args[1:4])
                if self.mutation==3 and label==b'AI Debug':self.word(PLAYERS+self.owner*180+0xb0,BODY+0x1000)
                if label==b'AI Debug':self.captured=self.word(PLAYERS+self.owner*180+0xb0)
                self.ret();return
        return super().on_instruction(mu,address,size,data)

    def reset(self,precision=0x27f):
        """정상 Player/스택/두 본체/레지스터를 독립 입력 시드로 복구한다."""
        self.mu.mem_write(PLAYERS,bytes(1620));self.word(0x59534c,PLAYERS)
        self.mu.mem_write(BODY,bytes([0xa5])*0xb08);self.mu.mem_write(BODY+0x1000,bytes([0x5a])*0xb08)
        self.mu.mem_write(STACK-0x400,bytes(0x500));self.word(STACK,STOP)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 앞 관측 scratch가 다음 관측에 섞이지 않는다.
            self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.events=[];self.formats=0;self.checks=0;self.taken=0;self.captured=BODY
        self.word(0x594fc8,0)  # 신규 MP 분기를 관측하지 않는다.

    def menu_case(self,values,precision):
        """현재 Player 재조회·메뉴 효과와 제어 저장소 읽기 순서를 관측한다."""
        self.reset(precision);self.menu=True
        checking,owner,present,mutation,*fields=values;self.owner=owner;self.mutation=mutation
        self.word(0x5e4794,checking)
        if 0<=owner<9:self.word(PLAYERS+owner*180+0xb0,BODY if present else 0)
        second=(1,1234,4,9,0x40c00000,0x40800000,9,10,30)
        for body,source in ((BODY,fields[:9]),(BODY+0x1000,second)):  # 교체 본체도 결과에서 계산하지 않는 독립 입력이다.
            for offset,value in zip(FIELDS,source):self.word(body+offset,value)  # 현재 본체의 명시 필드만 쓴다.
        for address,value in zip(DEBUG,fields[9:]):self.word(address,value)  # 일곱 현재 전역은 각각 독립 값이다.
        self.word(STACK+4,owner);self.mu.emu_start(0x413dd0,STOP,count=20000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return ';'.join(self.events) or '-'

    def input_case(self,code,owner,present,value,precision):
        """실제413d50의 mask·owner·현재brain·동일Debug DWORD 쓰기를 관측한다."""
        self.reset(precision);self.menu=False
        if 0<=owner<9:self.word(PLAYERS+owner*180+0xb0,BODY if present else 0)
        self.word(BODY+0xb00,value);self.word(DEBUG[0],value)
        parameter=DEBUG[0] if (code&0x3ffffff)==0x20000d1 else owner
        self.word(EVENT,code);self.word(EVENT+4,0xaabbccdd);self.word(EVENT+8,parameter);self.word(STACK+4,EVENT)
        self.mu.emu_start(0x413d50,STOP,count=2000);assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return self.word(BODY+0xb00),self.word(DEBUG[0])


def main():
    """첫 원문 관측만 새 폴더에 쓰고 반복 때 기존 기대값을 덮지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    oracle=Oracle();menus=[];inputs=[]
    base=[0,30000,0,0,0,0x40000000,3,6,8,99,29,99,29,99,99,99]  # 정상 body/PE 전역의 명시 입력이다.
    for checking,owner,present in itertools.product((0,1),(0,1,8,0xffffffff),(0,1)):  # gate의 단락은 정상/음수 owner까지만 관측한다.
        values=[checking,owner,present,0,*base];a=oracle.menu_case(values,0x27f);assert a==oracle.menu_case(values,0x37f);menus.append((values,a))
    for goal,initialized,choice in itertools.product(range(5),(0,1),range(4)):  # 각 목표/초기화와16개 체크 조건을 독립 조합한다.
        fields=base.copy();fields[0]=initialized;fields[2]=goal;fields[3]=(0,1,2,0xffffffff)[choice];fields[4:9]=[(0,0x3f800000,0x40000000,0x40c00000)[choice],(0x40000000,0x40400000,0x40800000,0x80000000)[choice],(3,6,9,0xffffffff)[choice],(3,6,10,0)[choice],(0,8,30,0xffffffff)[choice]]
        fields[9:]=[29 if (choice+i)%2 else 99 for i in range(7)];values=[1,1,1,0,*fields];a=oracle.menu_case(values,0x27f);assert a==oracle.menu_case(values,0x37f);menus.append((values,a))
    for mutation in range(1,5):  # callback 뒤 현재 Player/body를 실제로 다시 읽는 위치를 구별한다.
        values=[1,1,1,mutation,*base];a=oracle.menu_case(values,0x27f);assert a==oracle.menu_case(values,0x37f);menus.append((values,a))
    for code,owner,present,value in itertools.product((0x20000d0,0x20000d1,0x860000d0,0x820000d1,0x2000078),(0,1,8,0xffffffff),(0,1),(0,1,29,99,0xffffffff)):  # raw child는release를거부하지않고공통입력부모가앞에서거부한다.
        a=oracle.input_case(code,owner,present,value,0x27f);assert a==oracle.input_case(code,owner,present,value,0x37f);inputs.append((code,owner,present,value,*a))
    outputs={'ai-debug-menu-x86.tsv':'# checking owner present mutation fields16 trace\n'+''.join(','.join(map(str,values))+'\t'+trace+'\n' for values,trace in menus),'ai-debug-input-x86.tsv':'# code owner present before reporting debug\n'+''.join('\t'.join(map(str,row))+'\n' for row in inputs)}
    for name,payload in outputs.items():  # 관측 원시를 최초 UTF8/LF 바이트 그대로 고정한다.
        (args.output/name).write_text(payload,'utf-8',newline='\n')
    report=dict(unit=775,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',menuInputs=len(menus),inputInputs=len(inputs),observations=2*(len(menus)+len(inputs)),gameExecutions=0,multiplayerTests=0,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha256={name:hashlib.sha256((args.output/name).read_bytes()).hexdigest() for name in outputs},limitations=['메뉴하위GUI/서식기는명시호출경계다. actualCRT/제어메뉴스택몸체/픽셀을실행한것은아니다.','입력은413d50rawchild이며앞선Guide/Gump/release/Console/정지전체부모는별도다.','메타맵양성/임의잘못된Player주소/Goal범위밖/해제메모리는관측하지않았다.'])
    (args.output/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8');print(json.dumps({key:report[key] for key in ('menuInputs','inputInputs','observations','gameExecutions','multiplayerTests')}))


if __name__=='__main__':main()
