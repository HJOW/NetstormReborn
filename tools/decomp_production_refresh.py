#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제Factory목록→Deck재계산과Factory mode0전체를10.78기계어로대조한다.

표시모드0의빈Mana문자열도실제원문을실행한다.설정값과assert만명시외부입력/관찰경계다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path

import decomp_production_deck as base
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 새재계산관찰은기존Deck/Indicate입력과구분한다.
ROOT=base.ROOT
FIXTURE=ROOT/'cpppj/tests/fixtures/production-refresh-x86.tsv'
REPORT=ROOT/'cpppj/production-refresh.json'
TYPES,FACTORIES=0x11400000,0x11500000
# 원본작은하위전체와가상getter를허용한다.함수표에서빠진12BYTE도실제PE로확인한다.
EXTRA=(0x44f7c0,0x475530,0x4752b0,0x4ac200,0x4ac1e0,0x49a840,0x4c2400,0x4735c0,0x4ad0f0,0x4ad0a0)
MANUAL=(0x453f60,0x453f6b)


def nodes(variant,level):
    """순서/중복/비생산종속/원소유자와free/dead/void의명시입력을만든다."""
    return [(5,85,(level<<5)|(0,1,2,4)[variant%4],1,10,0,0,0),
        (6,124,0,2 if variant%5==1 else 1,0,0,0,0),
        (7,86,0,2 if variant%5==2 else 1,13,0,0,0),
        (10,6,8,0,0,11,71,0x10),(11,6,1,0,0,12,83 if variant%4==2 else 71,0x10 if variant%3 else 1),
        (12,6,2,0,0,0,158,0x10),(13,6,0,0,0,0,83,0x10)]


def deck_input():
    """기존네항목의명시입력만준비하며결과Deck은계산하지않는다."""
    raw=bytearray([0xa7]*0x160)
    # 이전가중치와power를일부남겨기존활성화/누락압축을구별한다.
    for index,(typ,chance,power) in enumerate(((71,3,9),(133,5,1),(153,7,0),(88,2,255))):raw[index*5:index*5+5]=bytes((typ,chance,power,13,index))
    struct.pack_into('<5I',raw,0x14c,4,17,0x11111111,0x22222222,0x33333333);return bytes(raw)


def inputs():
    """모드/현재허용표/무료기술/상태/owner만교차하며기대Deck은계산하지않는다."""
    result=[]
    # fullDeck는미션0일때만재계산을생략한다.일반생산금지와신전생산금지를독립입력한다.
    for mission,full,free,blocked,level,owner in itertools.product((0,1),(0,1),(0,1),(0,1,2,3),(0,1,7),(1,2)):
        variant=len(result)%6;result.append(['Refresh',owner,mission,0,1,full,free,blocked,level,variant])
    # Factorymode0은currentowner와무관하며실제capacity/distinct종류반환도확인한다.
    for blocked,level,variant in itertools.product((0,1,2,3),(0,1,7),(0,1,2,3)):
        result.append(['Factory',1,1,0,1,0,0,blocked,level,variant])
    # 원본모드진단이반환하면같은재계산을계속한다.
    result.extend([['Refresh',1,1,0,0,0,free,0,0,0] for free in (0,1)])
    return result


class Oracle(base.Oracle):
    """기존Deck의실제몸체위에전체재계산/현재TYPE/허용표읽기를더한다."""
    def __init__(self):
        """원문범위를더하고작은누락getter의실제12BYTE를명시고정한다."""
        super().__init__();self.mu.mem_map(TYPES,0x20000);self.mu.mem_map(FACTORIES,0x1000)
        # 전체원문의이전허용함수와새전체함수외실행은계속거부한다.
        for row in csv.DictReader(base.BODY.open(encoding='utf-8'),delimiter='\t'):
            entry=int(row['entry'],16)
            if entry not in EXTRA:continue
            self.entries.add(entry);self.ranges[f'{entry:08x}']=row['ranges']
            # 실제불연속범위를빠짐없이포함한다.
            for part in row['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        low,high=MANUAL;actual=bytes(self.mu.mem_read(low,high+1-low))
        if actual.hex()!='8b01ff908000000083c002c3':raise RuntimeError('Factory+b4실제12BYTE차이')
        self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low);self.entries.add(low);self.ranges[f'{low:08x}']=f'{low:x}-{high:x}'

    def instruction(self,mu,address,size,data):
        """현재설정입력외계산은기존원문훅으로실행한다."""
        if address==0x441090:
            self.boundaries['config']+=1;self.events.append('Q:getTeleportTechFree');self.returned(self.free);return
        return super().instruction(mu,address,size,data)

    def run_refresh(self,case,control):
        """같은현재월드입력에서정상반환/전체Deck/입력불변을관찰한다."""
        mode,owner,mission,fort,battle,full,free,blocked,level,variant=case;self.case=['Refresh',owner,0,0,4,0xa7,1,1,owner];self.free=free;self.events=[]
        self.mu.mem_write(base.PLAYERS,bytes(1620));self.put(base.PLAYERS+owner*180+0x68,base.DECK);self.mu.mem_write(base.DECK,deck_input())
        self.put(0x59534c,base.PLAYERS);self.put(0x540cac,owner);self.put(0x5c8464,base.POOL);self.put(0x5c847c,base.CAPACITY);self.put(0x541088,6);self.put(0x540bc4,1);self.put(0x5e4794,0)
        self.put(0x594fa0,mission);self.put(0x594fb8,fort);self.put(0x594fbc,battle);self.put(0x54db14,full);self.put(0x5412bc,153);self.put(0x54126c,133);self.put(0x59ab20,TYPES);self.put(0x541348,188)
        types=bytearray(256*500)
        # 합성TYPE의명시입력비트만준비하고원본연산결과는계산하지않는다.
        for typ,flags1,flags2 in ((85,0x4000000,0x4000),(86,0x4000000,0x4000),(124,0x4000000,0x200)):
            struct.pack_into('<I',types,typ*500+0xe8,flags1);struct.pack_into('<I',types,typ*500+0xec,flags2)
        self.mu.mem_write(TYPES,bytes(types));self.mu.mem_write(0x5ca0d0,bytes(255*4));self.put(0x5ca0d0+71*4,blocked&1);self.put(0x5ca0d0+133*4,blocked&2)
        raw=bytearray(64*50)
        # 현재객체/내용물과next링크를같은순서의입력으로준비한다.
        for sid,typ,state,own,head,nxt,kind,mask in nodes(variant,level):
            start=sid*50;struct.pack_into('<I',raw,start,0x509148);raw[start+10]=typ;raw[start+11]=state;raw[start+34]=own;struct.pack_into('<HH',raw,start+4,nxt,head);struct.pack_into('<HH',raw,start+18,kind,mask)
        self.mu.mem_write(base.POOL,bytes(raw));self.mu.mem_write(FACTORIES,struct.pack('<3I',5,6,7));self.put(0x55a4d4,FACTORIES);self.put(0x55a4dc,3)
        before=bytes(raw);players=bytes(self.mu.mem_read(base.PLAYERS,1620));restrictions=bytes(self.mu.mem_read(0x5ca0d0,255*4))
        args=(owner,) if mode=='Refresh' else (0,base.DECK);entry=0x44f7c0 if mode=='Refresh' else 0x4752b0;pop=0 if mode=='Refresh' else 8
        # 각호출의보존레지스터와x87을독립된입력값으로초기화한다.
        for register,value in base.common.SAVED.items():self.mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,base.POOL+5*50);self.mu.reg_write(UC_X86_REG_ESP,base.STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(base.STACK,struct.pack('<I',base.STOP)+b''.join(struct.pack('<I',value) for value in args));self.mu.emu_start(entry,base.STOP,count=500000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=base.STOP or self.mu.reg_read(UC_X86_REG_ESP)!=base.STACK+4+pop:raise RuntimeError('생산재계산반환/스택오류')
        if any(self.mu.reg_read(register)!=value for register,value in base.common.SAVED.items()):raise RuntimeError('생산재계산보존레지스터오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('생산재계산x87오류')
        if bytes(self.mu.mem_read(base.POOL,len(before)))!=before or bytes(self.mu.mem_read(base.PLAYERS,1620))!=players or bytes(self.mu.mem_read(TYPES,len(types)))!=types or bytes(self.mu.mem_read(0x5ca0d0,255*4))!=restrictions:raise RuntimeError('생산재계산읽기전용입력변경')
        self.returns+=1;return [str(self.mu.reg_read(UC_X86_REG_EAX)),bytes(self.mu.mem_read(base.DECK,0x160)).hex(),';'.join(self.events) or '-']


def generate(smoke=False):
    """모든결과가두정밀도에서같을때만새재계산fixture를저장한다."""
    oracle=Oracle();rows=[];plan=inputs()[::37] if smoke else inputs()
    # 원본함수에서만최종Deck과반환값을관찰한다.
    for case in plan:
        result=[oracle.run_refresh(case,control) for control in base.CONTROLS]
        if result[0]!=result[1]:raise RuntimeError('생산재계산두정밀도차이')
        rows.append([','.join(map(str,case)),deck_input().hex(),*result[0]])
    if smoke:print('refresh smoke',len(rows),oracle.returns,dict(oracle.boundaries));return
    FIXTURE.write_text('#10.78현재Factory/Deck/TotalMade재계산과Factorymode0전체원문관찰.설정/진단외대체없음.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),Path(base.__file__),ROOT/'tools/decomp_missionframe_oracle.py',base.BODY,ROOT/'extracted/refined/originals/Netstorm.c',base.common.BINARY,FIXTURE]
    report=dict(host=__import__('socket').gethostname(),date='2026-10-15',last_full_refined_host='HJOW-Athlon',total=len(rows),returns=oracle.returns,controls=list(base.CONTROLS),ranges=oracle.ranges,
        manual_body=dict(address='00453f60',bytes='8b01ff908000000083c002c3',meaning='현재가상80getLevel()+2.실제VT509148/509210+b4에서동일주소를확인'),
        native_calls=dict(oracle.native),boundaries=dict(oracle.boundaries),os_calls=0,gui_runs=0,files={path.relative_to(ROOT).as_posix():base.common.digest(path) for path in paths},
        limits=['합성현재TYPE/Factory목록/내용물/Deck/금지DWORD입력.실제Pop/전체생산트랜잭션/GUI를검증한것이아님',
            '0044f7c0/00475530/004752b0(mode0)/004735c0(빈문자열)/가상capacity와작은하위는모두실제명령',
            '설정getTeleportTechFree와원본assert만명시입력/관찰경계.제작메뉴를생성하는mode1/2는범위밖'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('refresh 완료',len(rows),oracle.returns)


def verify():
    """생성시점출처와고정입력/전체Deck폭/두정밀도정상반환수를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 현재바이트가관찰당시근거와같아야한다.
    for name,digest in report['files'].items():
        if base.common.digest(ROOT/name)!=digest:raise RuntimeError('생산재계산SHA불일치:'+name)
    if report['total']!=len(rows) or report['returns']!=2*len(rows) or report['os_calls'] or report['gui_runs']:raise RuntimeError('생산재계산합계오류')
    # 기대결과를재계산하지않고입력과물리Deck의길이만비교한다.
    for row,case in zip(rows,inputs(),strict=True):
        if len(row)!=5 or row[0]!=','.join(map(str,case)) or row[1]!=deck_input().hex() or len(row[3])!=704:raise RuntimeError('생산재계산입력/출력폭오류')
    print('production-refresh SHA/입력검증통과:',len(rows))


def main():
    """명시옵션으로전체생성/무저장표본/저장근거감사를선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
