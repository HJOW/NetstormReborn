#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 일반 명령과 두 AI 직접 writer를 원본 프로세스 없이 격리 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path('H:/T5Workspace/NetstormReborn')  # 이 호스트의 보호 원본/정밀 함수표는 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle as BaseOracle,SCRATCH,STACK,STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

PLAYERS=SCRATCH  # 실제 9행 Player를 대신하는 격리 CPU 입력 표다.
BODIES=SCRATCH+0x2000  # owner0..8별 실제 크기0xb08 본체를 겹치지 않게 둔다.
OFFSETS=(0x220,0x228,0x230,0x234,0x24c,0x2d8,0x600)  # 두 writer가 변경할 수 있는 일곱 DWORD다.
COMMANDS=((0x020000ad,0x4d7eab),(0x020000c1,0x4d801f),(0x02000078,0x4d8aca))  # 실제 일반 입력의 선택된 call 위치다.


class Oracle(BaseOracle):
    """현재 Player/brain 메모리만 바꾸는 실제 두 함수와 선택된 명령 call을 실행한다."""
    def __init__(self):
        """정밀 범위를 검토한 두 writer와 명령 prefix/call만 허용한다."""
        super().__init__('originals',[dict(name='CrankMoney',originals='00413c60'),dict(name='ParamSet',originals='004143b0')],helpers=[])
        self.allowed.extend(((0x4d6eca,0x4d6ede),(0x4d7eab,0x4d7eb5),(0x4d801f,0x4d8029),(0x4d8aca,0x4d8ad8)))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.command_mode=False;self.trace=[];self.flip=False;self.flipped=False

    def word(self,address,value=None):
        """격리 메모리의 DWORD 원시비트를 읽거나 명시 입력으로 기록한다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def returned(self):
        """명령의 외부 경계만 자연 반환하며 cdecl 인자 정리는 실제 호출자에 둔다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.word(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4)

    def on_instruction(self,mu,address,size,data):
        """뗌/공통 꼬리를 기록하고 명령 관측에서만 하위 효과를 호출 경계로 둔다."""
        if address in (0x4da477,0x4d8bda):
            self.gate='release' if address==0x4da477 else 'command';mu.emu_stop();return
        if self.command_mode and address in (0x486440,0x413c60,0x4143b0):
            self.trace.append('Geysers' if address==0x486440 else 'Money' if address==0x413c60 else 'Param:'+str(self.word(mu.reg_read(UC_X86_REG_ESP)+4)))
            self.returned();return
        return super().on_instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """원문 쓰기를 지정 본체/스택에 한정하고 선택적 현재 brain 교체를 독립 입력으로 준다."""
        if STACK-0x200<=address and address+size<=STACK+0x100:return
        assert BODIES<=address and address+size<=BODIES+9*0x1000,(hex(address),size)
        owner,offset=divmod(address-BODIES,0x1000);assert offset+size<=0xb08
        self.writes.append((owner,offset,size,value&((1<<(8*size))-1)))
        if self.flip and not self.flipped and offset==0x220:
            self.word(PLAYERS+self.selected_owner*180+0xb0,BODIES+8*0x1000);self.flipped=True

    def reset(self,mask=0xff,precision=0x27f):
        """Player와 아홉 본체/레지스터/스택을 독립 시드로 복구한다."""
        self.mu.mem_write(PLAYERS,bytes(1620));self.word(0x59534c,PLAYERS)
        for owner in range(9):  # owner0도 본문 보존을 확인하지만 일반 money 방문은1..8이다.
            self.mu.mem_write(BODIES+owner*0x1000,bytes([(0xa0+owner)&255])*0xb08)
            self.word(PLAYERS+owner*180+0xb0,BODIES+owner*0x1000 if owner==0 or mask&(1<<(owner-1)) else 0)
        self.mu.mem_write(STACK-0x200,bytes(0x300));self.word(STACK,STOP)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 실행의 scratch 레지스터가 다음 관측에 섞이지 않는다.
            self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.trace=[];self.writes=[];self.gate='-';self.flipped=False

    def state(self,kind,mask,packed,flip,precision):
        """실제 writer 반환 뒤 전체 쓰기 외 바이트와 선택된 DWORD를 관측한다."""
        self.command_mode=False;self.flip=bool(flip);self.selected_owner=packed&7;self.reset(mask,precision)
        before=[bytes(self.mu.mem_read(BODIES+owner*0x1000,0xb08)) for owner in range(9)]
        self.word(STACK+4,packed);self.mu.emu_start(0x413c60 if kind=='Money' else 0x4143b0,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for owner in range(9):  # 실제 쓰기 trace 외의 모든 바이트가 보존됐는지도 함께 단언한다.
            expected=bytearray(before[owner])
            for current,at,size,value in self.writes:  # Python 계산 모델 대신 실제 기계어 쓰기만 원본 입력에 적용한다.
                if current==owner:expected[at:at+size]=value.to_bytes(size,'little')
            assert bytes(self.mu.mem_read(BODIES+owner*0x1000,0xb08))==expected
        values='/'.join(','.join(str(self.word(BODIES+owner*0x1000+offset)) for offset in OFFSETS) for owner in range(9))
        writes=';'.join(f'{owner}:{offset}:{size}:{value}' for owner,offset,size,value in self.writes) or '-'
        return values,writes

    def command(self,event,value,entry):
        """공통 release/명령 mask와 해당 실제 call/인자/공통 꼬리만 관측한다."""
        self.command_mode=True;self.flip=False;self.reset();self.word(STACK+0x14,event);self.word(STACK+0x1c,value)
        self.mu.emu_start(0x4d6eca,0x4d6ede,count=1000)
        if self.gate!='release':
            assert self.mu.reg_read(UC_X86_REG_EBP)==event&0x3ffffff
            self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.emu_start(entry,0,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK
        return ';'.join(self.trace) or '-',self.gate


def save(path,payload):
    """첫 fixture를 덮어쓰지 않고 반복 결과는 물리 바이트가 같은지만 검증한다."""
    raw=payload.encode('utf-8')
    if path.exists():assert path.read_bytes()==raw,path
    else:path.write_bytes(raw)


def main():
    """독립 입력을 두 x87 정밀도/반복으로 관측해 새 원시와 fixture만 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=True);oracle=Oracle();rows=[];commands=[]
    cases=[('Money',mask,0,0) for mask in range(256)]  # 여덟 현재 brain 부착 조합을 모두 사용한다.
    cases += [('Param',mask,(high<<16)|(selector<<4)|owner|low,0) for owner,selector,high,mask,low in itertools.product(range(8),(0,1,2,3,4,5,255),(0,1,0x7fff,0x8000,0xffff),(0,255),(0,8,0xf000))]
    cases += [('Param',255,(high<<16)|owner,1) for owner,high in itertools.product((1,7),(1,0xffff))]  # selector0의 두 번째 현재 brain 재조회다.
    for kind,mask,packed,flip in cases:  # 제품 출력과 독립된 실제 기계어 결과를 두 정밀도로 대조한다.
        first=oracle.state(kind,mask,packed,flip,0x27f);assert first==oracle.state(kind,mask,packed,flip,0x37f);rows.append((kind,mask,packed,flip,*first))
    for (code,entry),value,flags in itertools.product(COMMANDS,(0,1,7,0xffffffff,0x80000000,0x7fffffff),(0,0x04000000,0x7c000000,0x80000000)):
        event=code|flags;first=oracle.command(event,value,entry);assert first==oracle.command(event,value,entry);commands.append((event,value,*first))
    save(output/'ai-command-state-x86.tsv','# kind mask packed flip values writes\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
    save(output/'ai-utility-commands-x86.tsv','# event value trace gate\n'+''.join('\t'.join(map(str,row))+'\n' for row in commands))
    record=dict(unit=772,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),stateInputs=len(rows),stateExecutions=len(rows)*2,inputCases=len(commands),inputExecutions=len(commands)*2,gameExecutions=0,multiplayerTests=0,builds=0,productWrites=0,
        observedToolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),binarySha256=oracle.sha256,codeSha256={f'{lo:08x}..{hi-1:08x}':hashlib.sha256(bytes(oracle.mu.mem_read(lo,hi-lo))).hexdigest() for lo,hi in oracle.allowed},limitations=['일반입력은 Guide/Gump가 소비하지 않은 뒤 release/mask와 선택된 실제call/꼬리만 관측한다. 전체 일반입력 함수/실제 메뉴 노출은 별도다.','실제 두 writer는 정상 Player9/등록된 brain 물리만 사용하며 원문 감긴 NULL Player base의 주소 자체는 host 계약 밖이다.','selector0 재조회 mutation은 독립 검사 입력이며 일반 게임에서 포인터가 바뀐다고 주장하지 않는다.','CreateInitialGeysers 전체 하위 생성은 기존 원문 복원 부모에 연결하고 이번 CPU는 호출 경계만 관측한다.'])
    save(output/'native.json',json.dumps(record,ensure_ascii=False,indent=2)+'\n');print(json.dumps({key:record[key] for key in ('stateInputs','stateExecutions','inputCases','inputExecutions','gameExecutions','multiplayerTests')}))


if __name__=='__main__':main()
