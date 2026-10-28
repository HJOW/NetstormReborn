#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 Guide/Gump 뒤 F2/F3 분기와 실제40칸 큐를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_production_shortcuts as gate
from unicorn.x86_const import UC_X86_REG_ESP

# 기존 관측기는 읽기 전용 PE·원래 사건 프레임·Guide·정지/큐 본체를 제공한다.
ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'cpppj/tests/fixtures/view-key-input-x86.tsv'
REPORT=ROOT/'extracted/cpp-view-keys-544/native.json'
HIDE,REDRAW=0x59a8ac,0x59a8b0  # 두 원본 DWORD만 CPU 쓰기 대상으로 추가한다.
SCHEMA=('name','events','guideResult','gumpResult','pauseDepth','console','command','read','filled',
        'hide','redraw','themeRead','themeValue','completed','readout','writeout','queueout',
        'physicalqueue','trace','stackdelta','nativeinstructions','hideout','redrawout')  # 앞13열이 입력이다.


class Oracle(gate.Oracle):
    """새 기대값을 계산하지 않고 실제 분기와 명시 설정/Tell 장치의 인자를 읽는다."""

    def cstring(self,address):
        """현재 외부 CALL이 받은 NUL 문자열만 읽는다."""
        output=bytearray()  # 실제 문자 바이트를 순서대로 보관한다.
        for offset in range(128):  # 이 분기의 짧은 원문 이름/섹션을 제한해 읽는다.
            value=self.number(address+offset,width=1)
            if not value:return output.decode('ascii')
            output.append(value)
        raise RuntimeError('View 키 외부 문자열 한도')

    def guard(self,mu,access,address,size,value,data):
        """원본 F2가 쓰는 숨김/dirty 두 DWORD 외에는 기존 격리 쓰기 규칙을 유지한다."""
        if HIDE<=address and address+size<=REDRAW+4:self.writes['visibility']+=1
        else:super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """설정 조회와 Tell만 명시 외부 장치로 제공하고 분기/명령 큐는 원문을 실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)  # 현재 실제 CALL 인자를 읽는 스택이다.
        if address==0x441270:
            target=self.number(esp+8)
            self.event('readInt',(self.cstring(self.number(esp+4)),self.number(target)))
            if self.case['themeRead']:self.number(target,self.case['themeValue'])
            self.returned(self.case['themeRead']);return
        if address==0x4cf960:
            self.event('tell',(self.cstring(self.number(esp+4)),))
            self.returned();return
        super().instruction(mu,address,size,data)

    def run(self,case,control):
        """같은 시작 전역을 제공하고 원문 종료 후의 두 DWORD를 추가 관측한다."""
        self.number(HIDE,case['hide']);self.number(REDRAW,case['redraw'])
        self.number(0x55a480,0);self.number(0x55a484,0)  # 개발자 전용 별도 기능은 정상 시작0이다.
        result=super().run(case,control)
        result.update(hideout=self.number(HIDE),redrawout=self.number(REDRAW))
        return result


def cases():
    """누름/뗌·상위 플래그·정지·선행 소비·설정·큐 경계의 명시 입력만 만든다."""
    rows=[]  # 입력 순서가 두 정밀도에서 동일한 독립 사례다.

    def add(name,events,**changes):
        """출력 기대값 없이 원본 사건과 장치 입력을 한 행으로 보관한다."""
        row=dict(name=name,events=';'.join(':'.join(str(value&0xffffffff) for value in event) for event in events),
                 guideResult=0,gumpResult=0,pauseDepth=0,console=0,command=0x020000f4,read=0,filled=0,
                 hide=9,redraw=17,themeRead=1,themeValue=3)
        row.update(changes);rows.append(row)

    # 모든 상위 플래그와 release를 실제 pause0/3에서 교차한다.
    for key,flags,release,pause in itertools.product((0x710000,0x720000),range(32),(0,0x80000000),(0,3)):
        add(f'flags-{key:x}-{flags}-{release:x}-{pause}',[(key|(flags<<26)|release,71,83)],pauseDepth=pause)
    # Guide/Gump 성공과 console이 일반 키 효과보다 먼저인 관문을 각각 대조한다.
    for key,guide,gump,console in itertools.product((0x710000,0x720000,0x20720000),(0,7),(0,11),(0,1)):
        add(f'gates-{key:x}-{guide}-{gump}-{console}',[(key,71,83)],guideResult=guide,gumpResult=gump,console=console)
    # ReadInt의 부재/0/음수도 현재0 지역값과 원래 Tell 선택을 보존해야 한다.
    for success,value,pause in itertools.product((0,1),(0,1,0xffffffff),(0,3)):
        add(f'theme-{success}-{value}-{pause}',[(0x20720000,0,0)],themeRead=success,themeValue=value,pauseDepth=pause)
    # 실제40칸 FIFO의 감김/포화와 숨김 DWORD의 논리 반전을 교차한다.
    for read,filled,hide in itertools.product((0,39),(0,38,39),(0,1,0xffffffff)):
        add(f'fifo-{read}-{filled}-{hide}',[(0x710000,0,0),(0x80710000,0,0),(0x720000,0,0),(0x80720000,0,0),(0x710000,0,0)],read=read,filled=filled,hide=hide)
    return rows


def main():
    """보호 PE SHA와 두 x87 정밀도의 같은 관측을 확인한 뒤 새 fixture만 저장한다."""
    assert gate.digest(gate.PE_PATH)==gate.PE_SHA
    oracle=Oracle();rows=cases();observed=[]  # 같은 입력마다 첫 정밀도 관측을 기준으로 비교한다.
    for row in rows:  # 매 행은 별도 장치/스택/물리 큐로 초기화한다.
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f)
        assert first==second,(row['name'],first,second)
        observed.append({**row,**first})
    text='\t'.join(SCHEMA)+'\n'+'\n'.join('\t'.join(str(row[key]) for key in SCHEMA) for row in observed)+'\n'
    FIXTURE.write_bytes(text.encode('utf8'))
    report={'host':socket.gethostname(),'peSha256':gate.PE_SHA,'inputs':len(rows),'nativeObservations':len(rows)*2,
            'precisionDifferences':0,'fixture':FIXTURE.relative_to(ROOT).as_posix(),'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
            'ranges':[[hex(low),hex(high)] for low,high in gate.RANGES],'externalDevices':['Guide virtual+24','Gump Route','general input cleanup','ReadInt','Tell'],
            'nativeBodiesUnchanged':True,'totals':oracle.totals,'boundaries':dict(oracle.boundaries),'writes':dict(oracle.writes),
            'scope':'개발자 flags55a480/55a484=0인 F2/F3 일반 입력. F7 배너/스크린샷은 후속'}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf8'))
    print(json.dumps({'inputs':len(rows),'nativeObservations':len(rows)*2,'precisionDifferences':0},ensure_ascii=False))


if __name__=='__main__':main()
