#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 MakeAi wrapper·현재 객체 이전/삭제를 원본 프로세스 없이 격리 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path('H:/T5Workspace/NetstormReborn')  # 동일 호스트의 보호 PE/정밀 함수표는 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle as BaseOracle,SCRATCH,STACK,STOP
from unicorn.x86_const import *

PLAYERS=SCRATCH  # 실제 Player9를 대신하는 독립 메모리다.
POOL=SCRATCH+0x4000  # 실제50BYTE 객체 슬롯을 독립 배치한다.
TYPES=SCRATCH+0x10000  # 작은 실제500BYTE TYPE 표의 flags2를 사용한다.
VTABLE=SCRATCH+0x15000  # 가상+74/10의 명시 경계 주소표다.
OWNER=STOP+0x100  # 실제 가상Owner 대신 전달/깊이/물리 소유자 쓰기만 기록한다.
DESTROY=STOP+0x200  # 가상삭제0의 전달만 기록하며 실제 전투 효과는 없다.


class Oracle(BaseOracle):
    """검토한 실제 부모/iterator/기본설정/ftol만 실행하고 실제 OS/AI생성/소유자 효과는 기록한다."""
    def __init__(self):
        """정밀 함수 범위와 두 명시 가상 효과만 준비한다."""
        pairs=[dict(name=str(a),originals=f'{a:08x}') for a in (0x417fa0,0x417f50,0x40ef80,0x4e49c0,0x4c27c0,0x414c80,0x40eb40,0x4ac200,0x4c25c0,0x4c25d0)]
        super().__init__('originals',pairs,helpers=[])
        self.allowed.extend(((0x4d6eca,0x4d6ede),(0x4d7f6e,0x4d7f9c)))
        self.mode='parent';self.trace=[];self.created=None;self.sp=0;self.mutate_post=-1

    def word(self,address,value=None):
        """정렬 없는 실제 DWORD를 격리 메모리에 읽고 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def returned(self,value=0,purge=0):
        """선택된 외부 경계의 자연 반환과 실제 호출자 인자 정리를 구별한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.word(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def on_instruction(self,mu,address,size,data):
        """명시 SP/AI생성/가상효과만 기록하고 나머지는 실제 기계어를 실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address in (0x4da477,0x4d8bda):self.gate='release' if address==0x4da477 else 'command';mu.emu_stop();return
        if self.mode=='input' and address==0x417fa0:
            self.trace.append('Make:'+str(self.word(esp+4)));self.returned();return
        if self.mode=='parent':
            if address==0x40ef80:self.trace.append('Init')
            if address==0x417fbc and self.mutate_post>=0:self.word(0x594fa4,self.mutate_post)
            if address==0x40ec50:
                self.trace.append('Sp:'+str(self.word(esp+4)));self.word(self.word(esp+8),self.sp);self.returned();return
            if address==0x417e50:
                self.trace.append('Create:'+str(self.word(esp+4)));self.created=bytes(mu.mem_read(self.word(esp+8),600)).hex();self.returned(0x12345678);return
            if address==0x4c27c0:self.trace.append('Transfer:'+str(self.word(esp+4)));self.returned();return
            if address==0x414c80:self.trace.append('Clear:'+str(self.word(esp+4)));self.returned(purge=4);return
        if self.mode=='objects':
            index=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            if address==0x4ac200:
                kind=mu.mem_read(POOL+index*50+10,1)[0];self.trace.append('Flags:'+str(index)+':'+str(self.word(TYPES+kind*500+0xec)))
            if address==0x49a840:self.returned(TYPES+self.word(esp+4)*500);return
            if address==0x46f530:
                x=self.word(esp+4);index=int(struct.unpack('<f',struct.pack('<I',x))[0]);region=self.regions.get(index,0);self.trace.append('Region:'+str(index)+':'+str(region));self.returned(region,8);return
            if address==0x4c25c0:self.trace.append('Local:'+str(self.word(esp+4)))
            if address==0x4c25d0:self.trace.append('Built:'+str(self.word(esp+4)))
            if address==OWNER:
                owner=self.word(esp+4);self.trace.append(f'Owner:{index}:{owner}:{self.word(0x5c89b8)}');mu.mem_write(POOL+index*50+34,bytes([owner&255]));self.returned(purge=4);return
            if address==DESTROY:
                self.trace.append(f'Destroy:{index}:{self.word(esp+4)}');self.returned(purge=4);return
        return super().on_instruction(mu,address,size,data)

    def reset(self,precision):
        """독립 stack/Player/레지스터 입력으로 이전 관측의 미정값을 배제한다."""
        self.mu.mem_write(STACK-0x2000,bytes([205])*0x2100);self.mu.mem_write(PLAYERS,bytes(1620));self.word(STACK,STOP);self.word(0x59534c,PLAYERS)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 가상 scratch 레지스터는 새로 제공한다.
            self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.trace=[];self.created=None;self.gate='-'

    def parent(self,owner,battle,post,known,local,sp,old,mutate,refresh,start,precision):
        """실제 기본600BYTE 쓰기/일반 override/ftol/owner0 꼬리 순서를 관측한다."""
        self.mode='parent';self.reset(precision);self.sp=sp;self.mutate_post=mutate
        for address,value in ((0x594fbc,battle),(0x594fa4,post),(0x595344,known),(0x540c70,local),(0x5318d4,refresh),(0x52f3c8,start),(PLAYERS+(local+1)*180+0xb0,old),(STACK+4,owner)):self.word(address,value) #독립현재전역과원문호출인자만명시한다.
        self.mu.emu_start(0x417fa0,STOP,count=10000);assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        settings=bytes(self.mu.mem_read(STACK-0x258,600)).hex();return settings,';'.join(self.trace) or '-',self.created or '-'

    def objects(self,operation,local,object_owner,buried,flags,same_region,kind,free,precision):
        """실제 iterator/현재TYPE/소유자/두계수/첫region/깊이 쓰기를 독립 단일후보로 관측한다."""
        self.mode='objects';self.reset(precision);self.mu.mem_write(POOL,bytes(50*6));self.mu.mem_write(TYPES,bytes(500*8));self.mu.mem_write(VTABLE,bytes(0x80));self.word(VTABLE+0x74,OWNER);self.word(VTABLE+0x10,DESTROY)
        for index in range(6):  # SID0/1은 예약이고 일반 후보 이외에는free다.
            self.word(POOL+index*50,VTABLE);self.mu.mem_write(POOL+index*50+11,bytes([1]));self.mu.mem_write(POOL+index*50+14,struct.pack('<f',float(index)));self.mu.mem_write(POOL+index*50+18,struct.pack('<f',float(index)))
        self.mu.mem_write(POOL+100+10,bytes([kind,buried|free]));self.mu.mem_write(POOL+100+34,bytes([object_owner]));self.word(TYPES+kind*500+0xec,flags)
        self.word(0x5c8464,POOL);self.word(0x5c847c,6);self.word(0x540c70,local);self.word(0x5412b8,4);self.word(0x5411a0,5);self.word(0x5c89b8,7);self.word(PLAYERS+local*180+0x78,41);self.word(PLAYERS+(local+1)*180+0x78,99);self.regions={2:41 if same_region else 42}
        self.mu.mem_write(0x5c94d0,bytes(1024));self.mu.mem_write(0x5c98d0,bytes(1024));self.word(STACK+4,local+1 if operation=='Transfer' else 0x2000 if operation=='MakeClear' else flags)
        self.mu.emu_start(0x4c27c0 if operation=='Transfer' else 0x414c80,STOP,count=10000);assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+(4 if operation!='Transfer' else 0)
        return bytes(self.mu.mem_read(POOL+100,50)).hex(),self.word(PLAYERS+(local+1)*180+0x78),self.word(0x5c89b8),self.word(0x5c94d0+kind*4),self.word(0x5c98d0+kind*4),';'.join(self.trace) or '-'

    def command(self,event,value,editor,post,precision):
        """선택된 원문 MakeAi 비교/Editor OR missionPost/공통 뗌만 실행한다."""
        self.mode='input';self.reset(precision);self.word(STACK+0x14,event);self.word(STACK+0x1c,value);self.word(0x5c85a4,editor);self.word(0x594fa4,post)
        self.mu.emu_start(0x4d6eca,0x4d6ede,count=1000)
        if self.gate!='release':self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.emu_start(0x4d7f6e,0,count=1000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK;return ';'.join(self.trace) or '-',self.gate


def save(path,payload):
    """최초 고정 기대값을 뒤 관측으로 덮어쓰지 않는다."""
    raw=payload.encode('utf-8')
    if path.exists():assert path.read_bytes()==raw,path
    else:path.write_bytes(raw)


def main():
    """명시 입력을 두 x87 정밀도로 실행하고 독립 원시/새fixture만 작성한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path);args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=True);oracle=Oracle();parents=[];objects=[];commands=[]
    cases=list(itertools.product((0,1,7,0xffffffff),(0,1),(0,1),(0,1),(1,7),(0,0x3fc00000,0xbfc00000,0x4f800001,0x5f000000,0x7fc00000),(0,1)))
    cases=[(*case,-1,3,2500) for case in cases]
    cases += [(1,1,0,1,1,0x42280000,0,1,0x7fffffff,0xffffffff),(0,1,1,0,1,0,0,0,0x80000000,0x7fffffff)]
    for values in cases:  # 실제 기본 설정과 signed64 하위DWORD를 제품 계산 없이 관측한다.
        first=oracle.parent(*values,0x27f);assert first==oracle.parent(*values,0x37f);parents.append((*values,*first))
    for values in itertools.product(('Transfer','Clear','MakeClear'),(1,7),(0,1,3),(0,8),(0,1,0x200,0x2000,0x200000,0x200200),(0,1),(3,4,5),(0,1)):  # 일반mask와실제owner0의고정2000을독립으로대조한다.
        first=oracle.objects(*values,0x27f);assert first==oracle.objects(*values,0x37f);objects.append((*values,*first))
    for value,editor,post,flags in itertools.product((0,1,7,0xffffffff),(0,1),(0,1),(0,0x4000000,0x7c000000,0x80000000)):  # 명령 수식과 모든 모드 관문/뗌을 관측한다.
        event=0x020000b7|flags;first=oracle.command(event,value,editor,post,0x27f);assert first==oracle.command(event,value,editor,post,0x37f);commands.append((event,value,editor,post,*first))
    for name,header,rows in [('make-ai-parent-x86.tsv','# owner battle post known local sp old mutate refresh start settings trace created',parents),('make-ai-objects-x86.tsv','# operation local owner buried flags sameRegion type free raw firstRegion depth localCount builtCount trace',objects),('make-ai-input-x86.tsv','# event value editor post trace gate',commands)]:  # 최초독립관측을평면C++기대값으로고정한다.
        save(output/name,header+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows))
    record=dict(unit=773,host=socket.gethostname(),lastPreciseDecompile='HJOW-Athlon / 2026-10-23',parentInputs=len(parents),objectInputs=len(objects),inputInputs=len(commands),observations=2*(len(parents)+len(objects)+len(commands)),gameExecutions=0,multiplayerTests=0,productWrites=0,builds=0,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),binarySha256=oracle.sha256,codeSha256={f'{a:08x}..{b-1:08x}':hashlib.sha256(bytes(oracle.mu.mem_read(a,b-a))).hexdigest() for a,b in oracle.allowed},limitations=['SP/AI실제생성/가상소유자/가상삭제/섬getter는명시CPU경계다. 실제게임·AI전략프레임·포획/공격/희생/승리는실행하지않는다.','owner0실제전이/삭제는독립명시슬롯만관측하며전체실제월드동일성을주장하지않는다.','600BYTE미작성stack는205로명시했고실제게임의미정stack와같다고주장하지않는다.'])
    save(output/'native.json',json.dumps(record,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:record[k] for k in ('parentInputs','objectInputs','inputInputs','observations')}))


if __name__=='__main__':main()
