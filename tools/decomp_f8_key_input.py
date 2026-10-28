#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 Guide/Gump/console 뒤 F8·ShiftF8·Editor 분기를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_view_key_input as view
import decomp_production_shortcuts as gate
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP

# 기존 관측기의 원본 사건/Guide/실제 큐를 그대로 빌리며 옛 fixture는 덮어쓰지 않는다.
ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'cpppj/tests/fixtures/f8-key-input-x86.tsv'
REPORT=ROOT/'extracted/cpp-f8-input-545/native.json'
READERS={0x594fa0:'fort',0x594fc8:'metamap',0x5c85a4:'editor'}  # 실제 직접 읽기의 주소/관측 이름이다.
EXTRA=(0x4d53f0,0x4d5480)  # Shift F8 뒤 숫자 fallback의 실제 순수10문자 탐색 본체다.
SCHEMA=('name','events','guideResult','gumpResult','pauseDepth','console','read','filled','fort','metamap',
        'editor','themeRead','themeValue','completed','readout','writeout','queueout','physicalqueue','trace','stackdelta','nativeinstructions')  # 앞13열만 입력이다.


class Oracle(view.Oracle):
    """현재 세 모드 직접 읽기와 원문 F8의 실제 큐/하위 인자를 함께 관측한다."""

    def __init__(self):
        """읽기 전용 본체 확인 범위와 현재 전역의 CPU 읽기 관측만 확장한다."""
        super().__init__()
        self.original[EXTRA[0]]=bytes(self.mu.mem_read(EXTRA[0],EXTRA[1]-EXTRA[0]))
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read_global)

    def read_global(self,mu,access,address,size,value,data):
        """CPU가 해당 DWORD를 실제로 읽은 순간의 현재 값을 기록한다."""
        if address in READERS and size==4:self.event(READERS[address],(self.number(address),))

    def instruction(self,mu,address,size,data):
        """이미 독립 복원된4d5bb0은 mode 인자만 기록하고 숫자 fallback은 원문을 실행한다."""
        if address==0x4d5bb0:
            self.event('highlight',(self.number(mu.reg_read(UC_X86_REG_ESP)+4),));self.returned();return
        if EXTRA[0]<=address<EXTRA[1]:
            self.instructions+=1;self.visited.add(address);return
        super().instruction(mu,address,size,data)

    def run(self,case,control):
        """F8의 현재 모드만 명시하며 metamap와 개발자 분기의 새 실행은 금지한다."""
        assert case['metamap']==0
        for address,name in READERS.items():self.number(address,case[name])  # 각 독립 입력의 직접 읽기 전역이다.
        result=super().run(case,control)
        result.pop('hideout');result.pop('redrawout')
        return result


def cases():
    """F8 상위 비트·정지·Editor·선행 소비·설정·실제 고리 경계만 생성한다."""
    rows=[];seen=set()  # 이름을 제외한 실제 입력이 같은 행은 중복 집계하지 않는다.

    def add(name,events,**changes):
        """기대값을 계산하지 않고 원문에 공급할 장치 입력만 등록한다."""
        row=dict(name=name,events=';'.join(':'.join(str(value&0xffffffff) for value in event) for event in events),
                 guideResult=0,gumpResult=0,pauseDepth=0,console=0,read=0,filled=0,fort=1,metamap=0,
                 editor=0,themeRead=1,themeValue=3,command=0x020000f4,hide=9,redraw=17)
        row.update(changes);key=tuple(row[field] for field in SCHEMA[1:13])
        if key not in seen:seen.add(key);rows.append(row)

    # 정상 두 모드에서 모든 상위 플래그/release와 정지를 교차한다.
    for flags,release,pause,fort,editor in itertools.product(range(32),(0,0x80000000),(0,3),(0,1),(0,1)):
        add(f'flags-{flags}-{release:x}-{pause}-{fort}-{editor}',[(0x770000|(flags<<26)|release,71,83)],pauseDepth=pause,fort=fort,editor=editor)
    # Guide/Gump/console이 F8·Shift F8의 모든 효과보다 먼저인 실제 관문이다.
    for key,guide,gump,console in itertools.product((0x770000,0x20770000,0x10770000,0x30770000),(0,7),(0,11),(0,1)):
        add(f'gates-{key:x}-{guide}-{gump}-{console}',[(key,71,83)],guideResult=guide,gumpResult=gump,console=console,editor=1)
    # ReadInt의 부재/0/비0 및 정지 중 테마 Tell을 직접 관측한다.
    for success,value,pause in itertools.product((0,1),(0,1,0xffffffff),(0,3)):
        add(f'theme-{success}-{value}-{pause}',[(0x20770000,0,0)],themeRead=success,themeValue=value,pauseDepth=pause)
    # 입력 사건 사이에 큐/현재 모드를 초기화하지 않고 감김/포화를 확인한다.
    for read,filled in itertools.product((0,39),(0,38,39)):
        add(f'fifo-{read}-{filled}',[(0x770000,0,0),(0x80770000,0,0),(0x20770000,0,0),(0x770000,0,0)],read=read,filled=filled,editor=1)
    return rows


def main():
    """서로 다른 명시 입력과 두 x87의 같은 실제 관측을 새 fixture에 고정한다."""
    assert gate.digest(gate.PE_PATH)==gate.PE_SHA
    oracle=Oracle();rows=cases();observed=[]  # 새 입력과 실제 첫 정밀도 출력이다.
    for row in rows:  # 각 입력은 별도 메모리/물리 고리에서 두 번 실행한다.
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f)
        assert first==second,(row['name'],first,second)
        observed.append({**row,**first})
    FIXTURE.write_bytes(('\t'.join(SCHEMA)+'\n'+'\n'.join('\t'.join(str(row[field]) for field in SCHEMA) for row in observed)+'\n').encode('utf8'))
    report={'host':socket.gethostname(),'peSha256':gate.PE_SHA,'uniqueInputs':len(rows),'nativeObservations':len(rows)*2,
            'precisionDifferences':0,'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
            'ranges':[[hex(a),hex(b)] for a,b in (*gate.RANGES,EXTRA)],'nativeBodiesUnchanged':True,'totals':oracle.totals,
            'externalDevices':['Guide virtual+24','Gump Route','general input cleanup','ReadInt','Tell','Highlight mode'],
            'boundaries':dict(oracle.boundaries),'scope':'metamap594fc8=0·개발자55a480/55a484=0인 일반/Editor F8. 멀티플레이 새 실행/검사 없음'}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf8'))
    print(json.dumps({'uniqueInputs':len(rows),'nativeObservations':len(rows)*2,'precisionDifferences':0}))


if __name__=='__main__':main()
