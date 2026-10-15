#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Deck/ProductionContains와IndicateTechnology의전체상위몸체를제한x86으로관찰한다.

CRT new는명시할당경계이며Indicate의기존004afa70은인자/호출경계다.
게임/GUI/OS는실행하지않는다.하위생성/부착전체검증과이번상위관찰을구분한다.
"""
import argparse
import collections
import csv
import itertools
import json
import socket
import struct
from pathlib import Path

import decomp_missionframe_oracle as common
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 보호바이너리/기존full refined원문과별도관찰출력이다.
ROOT=common.ROOT
BODY=ROOT/'extracted/refined/originals/functions.tsv'
FIXTURE=ROOT/'cpppj/tests/fixtures/production-deck-x86.tsv'
REPORT=ROOT/'cpppj/production-deck.json'
# 서로분리된에뮬레이터의Player9/Deck/풀/정상반환/스택이다.
PLAYERS,DECK,POOL,STOP,STACK_BASE,STACK=0x11000000,0x11100000,0x12000000,0x20000000,0x30000000,0x3003f000
CAPACITY,STRIDE=24000,50
# 실제전체몸체만허용하며작은하위와기본true가상필터도원문명령을실행한다.
ENTRIES=dict(Initialize=0x44f200,Clear=0x44f230,Append=0x44f290,Activate=0x44f3d0,Compact=0x44f430,
    Get=0x44f700,Contains=0x44f970,Indicate=0x4929e0)
HELPERS=(0x44f160,0x48fc60,0x4acac0,0x4b2270,0x4b1b20,0x44daa0)
CONTROLS=(0x027f,0x037f)
# 기존하위생성/CRT할당과assert의관찰경계다.반환값으로상위판정을계산하지않는다.
BOUNDARIES={0x4e4391:'new',0x4afa70:'create',0x4e0620:'assert'}


def inputs():
    """signed개수/전체DWORD타입/첫일치/미초기화영역과kind0의명시입력만만든다."""
    result=[]
    # 전체초기화와부분Clear의보존영역을서로다른할당바이트로구별한다.
    for mode,pattern in itertools.product(('Initialize','Clear','Compact'),(0,0x5a,0xa7,255)):
        result.append([mode,1,0,0,4,pattern,0,1,1])
    # 다른owner의기존포인터와신규할당을각각실제로사용한다.
    for owner,present,pattern in itertools.product((0,1,8),(0,1),(0,0xa7)):
        result.append(['Get',owner,0,0,4,pattern,present,1,1])
    # type인자는BYTE로미리자르지않으며signed음수개수에서는항목을읽지않는다.
    for typ,count,present in itertools.product((0,1,71,158,255,256,0xffffffff),(0,1,4,66,0x80000000,0xffffffff),(0,1)):
        result.append(['Contains',1,typ,0,count,0xa7,present,1,1])
    # 동일type중복과BYTE절단진단뒤의원본쓰기/계속진행을확인한다.
    for mode,typ,chance in itertools.product(('Append','Activate'),(0,71,158,255,256),(0,1,255,256,0xffffffff)):
        result.append([mode,1,typ,chance,4,0x5a,1,1,1])
    # 첫종속kindWORD를반환하므로query0/저장kind0은SID검사와다르다.
    for typ,kind,flags,present in itertools.product((0,1,71,158,256),(0,71,158),(0,4,5),(0,1)):
        result.append(['Indicate',1,typ,0,kind,flags,present,1,1])
    unique={} # 같은명시입력을독립관찰로중복합산하지않는다.
    # 입력의중복만제거하며원본반환은계산하지않는다.
    for row in result:unique.setdefault(tuple(row),row)
    return list(unique.values())


def initial_deck(case):
    """물리352BYTE와최대66항목을명시입력으로만준비한다."""
    mode,owner,typ,chance,count,pattern,present,known,battle=case;raw=bytearray([pattern]*0x160)
    # 활성개수밖의물리꼬리도같은입력으로두어쓸데없는초기화/압축을검출한다.
    for index in range(66):
        raw[index*5:index*5+5]=bytes(((0,71,158,255)[index%4],index%7+1,(255,0,1,128,127)[index%5],37,index&255))
    struct.pack_into('<5I',raw,0x14c,count,0x12345678,0x23456789,0x3456789a,0x456789ab)
    return raw


class Oracle:
    """총14개실제몸체만허용하고세외부경계만기록한다."""
    def __init__(self):
        """보호PE를가상메모리에복사하고물리범위·쓰기·ABI검사를등록한다."""
        if common.digest(common.BINARY)!=common.SHA:raise RuntimeError('보호PE SHA불일치')
        pe=common.pefile.PE(str(common.BINARY));self.base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image();self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(image)+4095)&~4095);self.mu.mem_write(self.base,image)
        self.allowed=bytearray(len(image));self.ranges={};self.entries=set(ENTRIES.values())|set(HELPERS)
        # 선택된불연속함수몸체만허용한다.진단/CRT/생성하위는별도관찰경계다.
        for row in csv.DictReader(BODY.open(encoding='utf-8'),delimiter='\t'):
            entry=int(row['entry'],16)
            if entry not in self.entries:continue
            self.ranges[f'{entry:08x}']=row['ranges']
            # Ghidra몸체의포함끝주소를반열린허용배열로옮긴다.
            for part in row['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        if len(self.ranges)!=len(self.entries):raise RuntimeError('실제몸체범위누락')
        # 원본포인터와무관한격리메모리이며OS가상할당은실행하지않는다.
        for address,size in ((PLAYERS,0x1000),(DECK,0x1000),(POOL,CAPACITY*STRIDE),(STOP,0x1000),(STACK_BASE,0x40000)):
            self.mu.mem_map(address,(size+4095)&~4095)
        self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value,width=4):
        """명시현재입력을원래정수폭으로기록한다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def get(self,address,width=4):
        """현재기계어메모리의정수비트를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def returned(self,value=0,pop=0):
        """명시외부경계의호출규약만반환한다.상위계산/분기를대신하지않는다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);destination=self.get(sp);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+pop);self.mu.reg_write(UC_X86_REG_EIP,destination)

    def instruction(self,mu,address,size,data):
        """전체원문실행과명시할당/하위생성/assert경계를구분해기록한다."""
        if address in BOUNDARIES:
            name=BOUNDARIES[address];self.boundaries[name]+=1;sp=mu.reg_read(UC_X86_REG_ESP)
            if name=='new':
                if self.get(sp+4)!=0x160:raise RuntimeError('원본Deck할당크기차이')
                self.events.append('N:352');mu.mem_write(DECK,bytes([self.case[5]])*0x160);self.returned(DECK)
            elif name=='create':
                parent=(mu.reg_read(UC_X86_REG_ECX)-POOL)//STRIDE;args=[self.get(sp+4+4*index) for index in range(5)];self.events.append('C:'+','.join(map(str,[parent,*args])));self.returned(5,20)
            else:self.events.append('A:'+str(self.get(sp+12)));self.returned()
            return
        offset=address-self.base
        if not 0<=offset<len(self.allowed) or not self.allowed[offset]:raise RuntimeError(f'허용밖명령{address:08x}')
        if address in self.entries:self.native[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """Deck/현재Player포인터/스택밖제품쓰기를거부한다.기술상위는풀을쓰지않는다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if DECK<=address and address+size<=DECK+0x160:return
        if PLAYERS<=address and address+size<=PLAYERS+1620 and size==4 and (address-PLAYERS)%180==0x68:return
        raise RuntimeError(f'생산허용밖쓰기{address:08x}+{size}')

    def run(self,case,control):
        """전체상위정상반환/현재Player/전체Deck/외부요청/원본풀불변을관찰한다."""
        self.case=case;mode,owner,typ,chance,count,pattern,present,known,battle=case;self.events=[]
        self.mu.mem_write(PLAYERS,bytes(1620));self.mu.mem_write(DECK,bytes(initial_deck(case)));self.put(PLAYERS+owner*180+0x68,DECK if present else 0);self.put(PLAYERS+battle*180+0x9c,50)
        self.put(0x59534c,PLAYERS);self.put(0x540cac,owner);self.put(0x540c70,battle);self.put(0x595344,known);self.put(0x5c8464,POOL);self.put(0x5c847c,CAPACITY);self.put(0x541088,6);self.put(0x540bc4,1);self.put(0x5e4794,0)
        raw=bytearray(64*STRIDE);raw[50*STRIDE+10]=152
        if mode=='Indicate' and present:
            struct.pack_into('<H',raw,50*STRIDE+6,60);raw[60*STRIDE+10]=6;struct.pack_into('<HH',raw,60*STRIDE+18,count,pattern)
        self.mu.mem_write(POOL,bytes(raw));before=bytes(raw)
        # 직접thiscall함수와cdeclwrapper의인자/호출자스택회수를구별한다.
        args=[owner] if mode=='Get' else [typ] if mode in ('Contains','Indicate') else [typ,chance] if mode in ('Append','Activate') else []
        pop=8 if mode in ('Append','Activate') else 0;self.mu.mem_write(STACK,struct.pack('<I',STOP)+b''.join(struct.pack('<I',value&0xffffffff) for value in args))
        for register,value in common.SAVED.items():self.mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,DECK);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRIES[mode],STOP,count=200000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4+pop:raise RuntimeError('생산정상반환/스택오류')
        if any(self.mu.reg_read(register)!=value for register,value in common.SAVED.items()):raise RuntimeError('생산보존레지스터오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('생산x87균형오류')
        if bytes(self.mu.mem_read(POOL,len(before)))!=before:raise RuntimeError('상위기술조회풀변경')
        self.returns+=1;result=self.mu.reg_read(UC_X86_REG_EAX)
        return [str(result) if mode in ('Get','Contains') else '-',bytes(self.mu.mem_read(DECK,0x160)).hex(),bytes(self.mu.mem_read(PLAYERS,1620)).hex(),';'.join(self.events) or '-']


def generate(smoke=False):
    """두정밀도결과가일치할때만원본출력과출처보고서를저장한다."""
    oracle=Oracle();rows=[];plan=inputs()[::43] if smoke else inputs()
    # 실제원문에서만반환/바이트/요청순서를읽는다.
    for case in plan:
        observations=[oracle.run(case,control) for control in CONTROLS]
        if observations[0]!=observations[1]:raise RuntimeError('생산두정밀도차이')
        rows.append([','.join(map(str,case)),*observations[0]])
    if smoke:print('production smoke',len(rows),oracle.returns,dict(oracle.boundaries));return
    FIXTURE.write_text('#10.78Deck/Contains전체와Indicate상위몸체관찰.할당/하위생성/진단은명시경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_missionframe_oracle.py',BODY,ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE,common.BINARY]
    report=dict(host=socket.gethostname(),date='2026-10-15',last_full_refined_host='HJOW-Athlon',total=len(rows),returns=oracle.returns,controls=list(CONTROLS),ranges=oracle.ranges,native_calls=dict(oracle.native),boundaries=dict(oracle.boundaries),
        os_calls=0,gui_runs=0,files={path.relative_to(ROOT).as_posix():common.digest(path) for path in paths},
        limits=['Deck Get/Contains/Init/Clear/Append/Activate/Compact 전체와Indicate전체상위/실제kindWORD조회/기본true필터',
            'CRT new0x160은명시바이트를갖는가상할당경계.진단은요청line을기록하고원본처럼계속한다.',
            'Indicate하위004afa70은인자/호출경계다.그본문이나실제생성/부착효과를이번native관찰에합산하지않는다.',
            '실제production재계산/Factory표시/메뉴Gump/게임자연진행은이fixture의범위밖이다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('production 완료',len(rows),oracle.returns)


def verify():
    """원본재실행없이출처/입력계획/행수/실제함수/두정밀도정상반환을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 생성당시모든원본/도구/관찰파일을정확한바이트SHA로확인한다.
    for name,digest in report['files'].items():
        if common.digest(ROOT/name)!=digest:raise RuntimeError('생산SHA불일치:'+name)
    if report['total']!=len(rows) or report['returns']!=2*len(rows) or report['os_calls'] or report['gui_runs'] or report['controls']!=list(CONTROLS):raise RuntimeError('생산행수/반환경계오류')
    # 출력모델을계산하지않고고정입력/물리출력의폭만확인한다.
    for row,case in zip(rows,inputs(),strict=True):
        if len(row)!=5 or row[0]!=','.join(map(str,case)) or len(row[2])!=704 or len(row[3])!=3240:raise RuntimeError('생산입력/출력폭오류')
    print('production-deck SHA/입력검증통과:',len(rows))


def main():
    """전체생성/무저장표본/저장보고서감사를명시적으로선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
