#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비멀티 일반설정3명령의 실제 부모/Win32요청/State+11과 현재값 재조회를 격리 관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import decomp_game_menu_actions as base
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX

STATE,SOURCE,SELECTED=base.ARENA+0x680,base.ARENA+0x700,base.ARENA+0x740  # 실제 상태와 두 문자열의 겹치지 않는 명시 수명이다.
CURRENT,THREAD=base.gate.HEAP+0x3200,base.gate.HEAP+0x3210  # 실제 IAT CALL을 받는 명시 OS 경계다.
COMMANDS=(0x02000029,0x0200008a,0x020000c7)  # 실제54279c/54291c/542a10이다.
SCHEMA=('event','first','second','paused','console','guide','gump','battle','editor','owner','priority','source','selected','osresult','mutation','trace','completed','stackdelta','priorityout','ownerout','stateout')  # 앞15열은 외부 입력이다.


class Oracle(base.Oracle):
    """일반입력/435c80/460ca0/Notify thunk는 실행하고 기존 큰 부모와 OS만 명시한다."""

    def __init__(self):
        """현호스트 전체 함수표에서 원문 래퍼 범위와 불변 본문을 추가한다."""
        super().__init__()
        facts={int(row['entry'],16):row for row in csv.DictReader((base.ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.extra=[]  #세 실제 래퍼의 불연속 실행 범위다.
        for entry in (0x435c80,0x460ca0,0x4bd540):  # 마지막 범위는 실제 Notify tail jump다.
            # 정밀표의 각 몸체 구간만 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.extra.append((low,high+1));self.original[low]=bytes(self.mu.mem_read(low,high+1-low))
        self.number(0x5001d8,CURRENT);self.number(0x5001dc,THREAD)

    def guard(self,mu,access,address,size,value,data):
        """원문 쓰기는 공유 priority DWORD와 선택 반환 State의 한 BYTE만 추가 허용한다."""
        if address==0x54db2c and size==4:self.writes['priority']+=1;return
        if address==STATE+0x11 and size==1:self.writes['fort-type']+=1;return
        return super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """실제 Config 키/재조회·OS 인자·현재 owner를 경계 인자로 관측한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if any(low<=address<high for low,high in self.extra):
            self.instructions+=1;self.visited.add(address);return
        if address==0x4414d0:
            key,value=(self.cstring(self.number(sp+offset)).encode('ascii').hex() for offset in (4,8))
            self.body_event=True;self.event('set',(key,value));self.returned(0);return
        if address==0x4410f0:
            key=self.cstring(self.number(sp+4)).encode('ascii').hex()
            self.body_event=True;self.event('get',(key,*(self.number(sp+offset) for offset in (8,12,16))));self.returned(SELECTED);return
        if address==0x4b6100:
            self.body_event=True;self.event('palette',(self.cstring(self.number(sp+4)).encode('ascii').hex(),));self.returned();return
        if address==CURRENT:
            self.body_event=True;self.event('current-thread');self.returned(0xfffffffe);return
        if address==THREAD:
            self.body_event=True;self.event('thread',(self.number(sp+4),self.number(sp+8)));self.returned(self.case['osresult'],8);return
        if address in (0x4bd550,0x4bd360):
            owner=(mu.reg_read(UC_X86_REG_ECX)-0x5c89f0)//20
            self.body_event=True;self.event('state' if address==0x4bd550 else 'notify',(owner,))
            self.returned(STATE if address==0x4bd550 else 0);return
        return super().instruction(mu,address,size,data)

    def run(self,case,control):
        """독립 명시 전역을 공급하고 큰 State/Notify 부모는 기존 복원 범위와 구별한다."""
        self.number(0x594fbc,case['battle']);self.number(0x540cac,case['owner']);self.number(0x54db2c,case['priority'])
        result=super().run(case,control)
        result.update(priorityout=self.number(0x54db2c),ownerout=self.number(0x540cac),stateout=bytes(self.mu.mem_read(STATE,28)).hex())
        return result


def cases():
    """공통 관문과 signed 폭/OS false/설정 뒤 모드·owner·공유 DWORD 변화를 독립 입력으로 만든다."""
    seed,rows=base.cases()[0],[]

    def add(command,flags=0,paused=0,console=0,guide=0,gump=0,battle=0,editor=1,owner=1,priority=7,source=b'clouds',selected=b'clouds2',osresult=1,mutation='-',second=None):
        """MP challenge 값은0으로 유지하고 기존 부모의 결과를 새 출력 기대값으로 계산하지 않는다."""
        values=list(map(int,seed['globals'].split(':')))
        values[0],values[2],values[3],values[4],values[8],values[9]=editor,0,1,1,paused,console
        devices=bytearray.fromhex(seed['devices']);devices[0x680:0x69c]=bytes([0xa5])*28
        devices[0x700:0x700+len(source)+1]=source+b'\0';devices[0x740:0x740+len(selected)+1]=selected+b'\0'
        second=SOURCE if second is None else second
        rows.append(dict(seed,name='configuration-'+str(len(rows)),event=base.packed((command|flags,0x12345678,second)),guideResult=guide,gumpResult=gump,globals=base.packed(values),devices=devices.hex(),read=0,filled=0,mutation=mutation,stop='-',battle=battle,owner=owner,priority=priority,source=source.hex(),selected=selected.hex(),osresult=osresult))

    # 비MP 명령의 선행 소비/뗌/정지/Console 입력을 독립 교차한다.
    for command,flags,paused,console,guide,gump in itertools.product(COMMANDS,(0,0x80000000,0x3c000000,0xbc000000),(0,3),(0,1),(0,7),(0,11)):
        add(command,flags,paused,console,guide,gump,second=None if command==COMMANDS[0] else 2)
    # 현재 비MP 모드와 첫NUL/빈 palette 이름을 별도로 공급한다.
    for battle,source,selected in itertools.product((0,1),(b'',b'clouds',b'before\0after'),(b'',b'new-clouds')):
        add(COMMANDS[0],battle=battle,source=source,selected=selected)
    # raw priority 전체 비트와 OS BOOL 반환을 교차한다.
    for second,osresult in itertools.product((0,1,2,3,0x7fffffff,0x80000000,0xffffffff),(0,1)):
        add(COMMANDS[1],second=second,osresult=osresult)
    # State 낮은BYTE 쓰기와 Editor 진단의 현재 입력을 관측한다.
    for second,editor in itertools.product((0,1,255,256,0x80000000,0xffffffff),(0,1)):
        add(COMMANDS[2],second=second,editor=editor)
    # OS 장치 반환 뒤 공유 요청 재조회가 관측되도록 입력을 바꾼다.
    for updated in (0,2,0x80000000,0xffffffff):
        add(COMMANDS[1],second=1,mutation=f'thread1/{0x54db2c}/{updated}',osresult=0)
    add(COMMANDS[0],battle=1,mutation=f'set1/{0x594fbc}/0')
    add(COMMANDS[0],battle=0,mutation=f'set1/{0x594fbc}/1')
    add(COMMANDS[2],second=257,mutation=f'state1/{0x540cac}/2')
    add(COMMANDS[2],second=258,editor=0,mutation=f'assert1/{0x540cac}/3')
    return rows


def main():
    """두 x87 정밀도와 별도 반복의 동일 관측만 새 fixture로 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle,inputs,lines,raw=Oracle(),cases(),['\t'.join(SCHEMA)],[]
    # 각 독립 입력의 두 정밀도와 반복 결과 전체가 일치해야 한다.
    for case in inputs:
        results=[oracle.run(case,control) for control in (0x27f,0x37f,0x27f)]
        assert results[0]==results[1]==results[2],case['name']
        result,event=results[0],tuple(map(int,case['event'].split(':')));values=list(map(int,case['globals'].split(':')))
        assert result['completed']==1 and result['stackdelta']==0,case['name']
        row=(*event,values[8],values[9],case['guideResult'],case['gumpResult'],case['battle'],values[0],case['owner'],case['priority'],case['source'],case['selected'],case['osresult'],case['mutation'],result['bodytrace'],result['completed'],result['stackdelta'],result['priorityout'],result['ownerout'],result['stateout'])
        lines.append('\t'.join(map(str,row)));raw.append(dict(input=case,observations=results))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=762,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(inputs),observations=len(inputs)*3,differences=0,nativeBodiesUnchanged=True,gameExecutions=0,multiplayerTests=0,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),extraBodies=['00435c80 Win32 priority selection','00460ca0 State+11/current owner reread','004bd540 Notify thunk'],boundaries=['Config Set004414d0/Get004410f0','palette004b6100','Win32 GetCurrentThread/SetThreadPriority','State004bd550/Notify004bd360','format004a89c0/Config00441580'],limits=['challenge/MP조건은코드대조만이며새관측하지않음','실제Stategetter/Notify저장과palette reader는기존복원부모이며이번CPU관측의명시경계다','실제OS우선순위/게임/GUI/파일I/O/통신0'],raw=raw)
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(dict(inputs=len(inputs),observations=len(inputs)*3,fixtureSha256=record['fixtureSha256'])))


if __name__=='__main__':main()
