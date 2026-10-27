#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 가장자리 입력 본체와 실제 카메라 목표 쓰기를 격리 x86에서 관찰한다. 게임/OS 실행은 없다."""
import argparse
import csv
import hashlib
import itertools
import json
import platform
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK
from unicorn.x86_const import *

ENTRY,END=0x4d65ac,0x4d67ad  # Guide 호출 뒤부터 다음 포인터 시간 처리 직전까지의 실제 명령 경계다.
RENDERER=POOL  # 현재 view/목표는 원문과 같은 오프셋으로 넣는 명시 입력이다.
FIELDS=('flags','since','enabled','borderX','borderY','dialog','x','y','width','height','left','shift','menu','popup','wall','speed','cameraX','cameraY','targetX','targetY','revision') # 앞21열은 결과 없는 입력이다.


def dbits(value):
    """실수 입력/출력을 손실 없는 double 비트로 기록한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


class Oracle(MemoryOracle):
    """원문 입력/FTOL/목표 계산을 실행하고 현재 GUI/OS 조회만 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 전체 정밀 내보내기의 실제 함수 범위를 읽기만 한다."""
        super().__init__();facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[(ENTRY,END)]
        for address in (0x4e49c0,0x497e00): # 실제 FTOL과 카메라 목표 부모는 stub으로 계산하지 않는다.
            for part in facts[address]['ranges'].split(';'): # 불연속 범위도 원본 허용 바이트만 포함한다.
                first,last=(int(value,16) for value in part.split('-'));self.allowed.append((first,last+1))

    def instruction(self,mu,address,size,data):
        """실제 호출 순서/인자를 관찰하고 선택된 OS/GUI 결과만 입력대로 반환한다."""
        if address in (END,0x4d67e4): self.completed=True;mu.emu_stop();return # 모달의 직접 건너뛰기와 일반 후속 구간 진입은 모두 이 가장자리 본체 밖이다.
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,2)
        if address==0x452e00:
            code=args[1];release=self.case['left'] if code==0x10000 else self.case['shift'];self.events.append(f'P:{code}')
            self.mu.mem_write(args[0],struct.pack('<3I',code|(0x80000000 if release else 0),self.case['x']&0xffffffff,self.case['y']&0xffffffff));self.ret(args[0]);return
        if address==0x464020: self.events.append('M:'+str(args[0]));self.ret(self.case['menu']);return
        if address==0x4641d0: self.events.append('B');self.ret(self.case['popup']);return
        if address==0x466710: self.events.append('C');self.ret();return
        if address==0x497e00: self.events.append('D:'+str(args[0])+':'+str(args[1]))
        if not any(first<=address and address+size<=last for first,last in self.allowed): raise RuntimeError(f'미등록 가장자리 명령 {address:08x}')
        self.instructions+=1

    def run(self,case,control):
        """실제 정적 초기화/시간/버튼/목표/카운터와 정상 구간 도착을 독립 대조한다."""
        self.case=case;self.events=[];self.instructions=0;self.completed=False
        for address,key in ((0x5cafe8,'flags'),(0x5318f8,'enabled'),(0x5c7904,'borderX'),(0x5c7908,'borderY'),(0x565dd4,'dialog'),(0x5c78f0,'width'),(0x5c78f4,'height'),(0x5318f0,'speed'),(0x5c85dc,'revision')): # 실제 현재 전역은 각각의 입력값으로 초기화한다.
            self.put(address,case[key])
        self.mu.mem_write(0x5cafc8,struct.pack('<Q',case['since']));self.mu.mem_write(0x55b4d8,struct.pack('<Q',case['wall']));self.put(0x5cad24,731);self.put(0x59a8bc,RENDERER)
        self.mu.mem_write(RENDERER,bytes(0x100));self.put(RENDERER+0x1c,case['cameraX']);self.put(RENDERER+0x20,case['cameraY']);self.put(RENDERER+0x50,case['targetX']);self.put(RENDERER+0x54,case['targetY'])
        self.mu.mem_write(STACK,bytes(256));self.put(STACK+0x18,case['x']);self.put(STACK+0x1c,case['y'])
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ESI,1);self.mu.reg_write(UC_X86_REG_EDI,0);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,END+1,count=10000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK and self.mu.reg_read(UC_X86_REG_FPCW)==control
        return dict(flags=self.words(0x5cafe8,1)[0],since=struct.unpack('<Q',self.mu.mem_read(0x5cafc8,8))[0],directions=self.words(STACK+0x24,2),target=self.words(RENDERER+0x50,2),revision=self.words(0x5c85dc,1)[0],events=';'.join(self.events) or '-',instructions=self.instructions)


def inputs():
    """호출 결과를 사용하지 않고 가장자리/시간/GUI/비정상 정수 경계 입력을 준비한다."""
    base=dict(zip(FIELDS,(4,dbits(9.0),1,0,0,0,0,0,1024,768,1,1,0,0,dbits(10.0),30,100,200,400,500,17)));rows=[]
    for x,y,left,shift in itertools.product((0,1,2,512,1023,1024),(0,2,384,767),(0,1),(0,1)): # 네 변/두 픽셀 경계/좌버튼과 Shift를 분리한다.
        rows.append(dict(base,x=x,y=y,left=left,shift=shift))
    for key,values in (('flags',(0,1,2,3,0xffffffff)),('enabled',(0,2)),('borderX',(-1,1)),('borderY',(-1,1)),('dialog',(1,0xffffffff)),('menu',(1,)),('popup',(1,)),('speed',(-2147483648,-1,0,1,2,3,2147483647)),('since',tuple(map(dbits,(0.0,10.0,10.01,9.999,1e30)))),('wall',tuple(map(dbits,(0.0,9.0,9.999,10.033333333333333,1e30))))): # 전역 관문·가속/FTOL 한계를 같은 입력에서 하나씩 바꾼다.
        for value in values: rows.append(dict(base,**{key:value})) # 기대 결과 없이 해당 현재 입력만 교체한다.
    for width,height in ((1,1),(2,2),(1280,720),(1366,768),(1280,800)): # 작은 화면의 양쪽 경계 우선순위와 와이드 입력이다.
        rows.append(dict(base,width=width,height=height,x=1,y=1))
    for x,y in ((2147483647,-2147483648),(-2147483648,2147483647)): rows.append(dict(base,cameraX=x,cameraY=y,revision=0xffffffff)) # 실제 목표 덧셈/카운터32비트 감김 입력이다.
    return rows


def main():
    """두 정밀도 관측을 그대로 fixture/보고서에 쓰며 기존 파일은 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracle=Oracle();rows=[]
    for case in inputs(): # 각 입력의 실제 두 정밀도 반환을 각각 기록한다.
        outputs=[oracle.run(case,control) for control in (0x27f,0x37f)];assert outputs[0]==outputs[1],case
        rows.append([*(str(case[key]) for key in FIELDS),str(outputs[0]['flags']),str(outputs[0]['since']),':'.join(map(str,outputs[0]['directions'])),':'.join(map(str,outputs[0]['target'])),str(outputs[0]['revision']),outputs[0]['events']])
    with args.fixture.open('x',encoding='utf-8',newline='\n') as stream: stream.write('# '+'\t'.join((*FIELDS,'outFlags','outSince','directionsYX','targetXY','outRevision','events'))+'\n');stream.writelines('\t'.join(row)+'\n' for row in rows)
    report=dict(host=platform.node(),cases=len(rows),runs=len(rows)*2,differences=0,originalExecutions=0,entry=ENTRY,end=END,deviceBoundaries=['452e00 OS 폴링','464020 실제 메뉴 serial 조회','4641d0 실제4000h 팝업 조회','466710 실제 팝업 Kill 요청'],executedBodies=['4d65ac..4d67ad','4e49c0','497e00'],sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'originals/Netstorm.exe',Path(__file__).resolve(),args.fixture.resolve())})
    with args.report.open('x',encoding='utf-8') as stream: json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(cases=len(rows),runs=len(rows)*2,differences=0)))


if __name__=='__main__': main()
