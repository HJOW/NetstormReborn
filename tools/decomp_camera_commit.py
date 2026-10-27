#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 카메라 적용497960/두497490/dirty 확장4834c0을 실제 x86로 관찰한다. 게임 실행은 없다."""
import argparse
import csv
import hashlib
import itertools
import json
import platform
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP
from unicorn.x86_const import *

FIELDS = ('x','y','targetX','targetY','zoom','targetZoom','snapshotZoom','baseX','baseY','snapshotBaseX','snapshotBaseY',
          'flags','denominator','viewX','viewY','viewWidth','viewHeight','width','height','mouseX','mouseY',
          'redraw','worldRedraw','dirtyProfile','mutation','afterX','afterY','afterZoom')  # 앞28열은 결과 없는 현재 입력이다.
GLOBAL_FIELDS = ((0x5c78d4,'flags'),(0x540ce4,'denominator'),(0x5ca99c,'viewX'),(0x5ca9a0,'viewY'),
    (0x5ca9b4,'viewWidth'),(0x5ca9b8,'viewHeight'),(0x5c78f0,'width'),(0x5c78f4,'height'),(0x5cae94,'mouseX'),(0x5cae98,'mouseY'))  # 원문에서 서로 다른 현재 전역이다.


def bits(value):
    """초기 명시 float 입력을 원문 DWORD로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def views(case):
    """결과를 계산하지 않고 독립 Renderer/snapshot의 서로 다른 초기64BYTE를 구성한다."""
    return ([bits(1.25),bits(2.5),16,11,case['baseX'],case['baseY'],case['zoom'],case['x'],case['y'],bits(16),bits(11),bits(16),bits(11),bits(1),bits(100),bits(200)],
        [bits(9),bits(8),32,22,case['snapshotBaseX'],case['snapshotBaseY'],case['snapshotZoom'],75,85,bits(32),bits(22),bits(16),bits(11),bits(1),bits(75),bits(85)])


def dirty(profile):
    """원문 변경 표의 좌표/flags 입력만 만들며 이동·합집합 결과는 만들지 않는다."""
    if profile==0:
        return []
    if profile==4:
        return [[i,i+1,i+10,i+20,i%8] for i in range(100)]  # 원문100항목의 모든 현재 플래그를 공급한다.
    result=[]
    for i in range(4):  # 보존·이동·확장과 다른 고비트를 교차한다.
        rect=[10+i,20+i,30+i,40+i] if profile!=3 else [-2147483648+i,2147483647-i,17+i,-33-i]
        result.append([*rect,i+(4 if profile==2 else 0)])
    return result


class Oracle(MemoryOracle):
    """원문 적용/투영/dirty 확장만 실행하고 기존 외부 장치·dirty 추가는 호출 경계로 기록한다."""
    def __init__(self):
        """전체 정밀 범위에서 실제 세 함수의 명령만 허용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[]
        for entry in (0x497960,0x497490,0x4834c0):  # 두 setter와 dirty2의 실제 합집합까지 stub으로 계산하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 불연속 원문 경계를 그대로 따른다.
                first,last=(int(value,16) for value in part.split('-'))
                self.allowed.append((first,last+1))

    def event(self,code,values=()):
        """호출 인자의 하위DWORD를 Native 순서대로 기록한다."""
        self.events.append(code+(':'+':'.join(str(value&0xffffffff) for value in values) if values else ''))

    def mutate(self,phase):
        """명시 callback 입력에서만 현재 목표/표/viewport를 바꾸어 뒤 재조회를 확인한다."""
        if self.case['mutation']!=phase or phase in self.mutated:
            return
        self.mutated.add(phase)
        if phase in (1,2,3):
            for offset,key in ((0x50,'afterX'),(0x54,'afterY'),(0x58,'afterZoom')):  # 계산 결과 없이 명시 새 목표만 쓴다.
                self.put(POOL+offset,self.case[key])
        elif phase==4:
            index=self.words(POOL+0x64,1)[0]
            for i,value in enumerate((100,110,120,130)):  # 마우스 장치가 새로 등록한 현재 dirty 입력이다.
                self.put(POOL+0x68+index*16+i*4,value)
            self.put(POOL+0x6a8+index*4,2)
            self.put(POOL+0x64,index+1)
        elif phase==5:
            self.put(0x59a910,63)
            self.put(0x59a914,31)
        elif phase==6:
            for address,value in ((0x5ca99c,7),(0x5ca9a0,9),(0x5ca9b4,333),(0x5ca9b8,444),(0x5c78d4,18)):  # 첫 화면 이동 뒤의 현재 장치 영역이다.
                self.put(address,value)

    def instruction(self,mu,address,size,data):
        """외부 경계의 순서/인자·현재 view를 기록하고 실제 본문 명령은 계속 실행한다."""
        if address==STOP:
            self.completed=True
            mu.emu_stop()
            return
        esp=mu.reg_read(UC_X86_REG_ESP)
        args=self.words(esp+4,6)
        if address==0x446a10:
            self.event('L',args[:1]);self.mutate(1);self.ret();return
        if address==0x446a40:
            self.event('U');self.ret();return
        if address==0x4660f0:
            self.event('R',(*self.words(POOL+0x18,3),*self.words(0x59a918,3),self.words(POOL+0x5c,1)[0],self.words(0x59a8b0,1)[0]))
            self.mutate(2);self.mutate(5);self.ret();return
        if address in (0x4a1ad0,0x4a1e00):
            self.event('S' if address==0x4a1ad0 else 'P',args)
            if address==0x4a1ad0:
                self.mutate(3);self.mutate(6)
            self.ret();return
        if address==0x497580:
            assert mu.reg_read(UC_X86_REG_ECX)==POOL
            self.event('D',(*self.words(args[0],4),args[1]));self.ret(purge=8);return
        if address==0x4977f0:
            self.event('B',args[:4]);self.ret();return
        if address==0x4a41c0:
            self.event('M',args[:2]);self.mutate(4);self.ret();return
        if address==0x4e0620:
            self.event('A',(args[2],));self.ret();return
        if not any(first<=address and address+size<=last for first,last in self.allowed):
            raise RuntimeError(f'미등록 카메라 적용 명령 {address:08x}')
        self.instructions+=1

    def run(self,case,control):
        """독립 초기 자료만 배치하고 원문 정상 반환·전체64BYTE/표/현재 전역을 읽는다."""
        self.case=case;self.events=[];self.mutated=set();self.instructions=0;self.completed=False
        self.mu.mem_write(POOL,bytes(0x900))
        first,second=views(case)
        self.mu.mem_write(POOL,struct.pack('<16I',*(value&0xffffffff for value in first)))
        self.mu.mem_write(0x59a900,struct.pack('<16I',*(value&0xffffffff for value in second)))
        for offset,key in ((0x50,'targetX'),(0x54,'targetY'),(0x58,'targetZoom'),(0x5c,'redraw')):  # 현재 view와 별도 목표/전체dirty다.
            self.put(POOL+offset,case[key])
        self.put(POOL+0x48,480);self.put(POOL+0x4c,490);self.put(0x59a8b0,case['worldRedraw'])
        for address,key in GLOBAL_FIELDS:  # 화면·배경 분모·마우스는 원문 주소별로 쓴다.
            self.put(address,case[key])
        self.mu.mem_write(0x5ca9cc,struct.pack('<4I',3,4,997,701))
        entries=dirty(case['dirtyProfile']);self.put(POOL+0x64,len(entries))
        for index,entry in enumerate(entries):  # 원문 사각형과 flags 배열은 서로 다른 오프셋이다.
            for i,value in enumerate(entry[:4]):  # 입력 사각형 네 좌표를 그대로 저장한다.
                self.put(POOL+0x68+index*16+i*4,value)
            self.put(POOL+0x6a8+index*4,entry[4])
        self.mu.mem_write(STACK-256,bytes(320));self.put(STACK,STOP)
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_ECX,POOL),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)):  # 같은 실제 caller/x87 환경을 매 호출 새로 준비한다.
            self.mu.reg_write(register,value)
        self.mu.emu_start(0x497960,STOP+1,count=100000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==control
        count=self.words(POOL+0x64,1)[0]
        current=[(*self.words(POOL+0x68+index*16,4),self.words(POOL+0x6a8+index*4,1)[0]) for index in range(count)]  # 미변경/이동/확장한 전체 현재 표를 읽는다.
        return dict(renderer=self.words(POOL,16),snapshot=self.words(0x59a900,16),target=self.words(POOL+0x48,6),
            dirty=current,globals=tuple(self.words(address,1)[0] for address,_ in GLOBAL_FIELDS)+(self.words(0x59a8b0,1)[0],),events=';'.join(self.events) or '-',instructions=self.instructions)


def inputs():
    """목표·배율·viewport·dirty·callback 변경 입력을 조합하며 기대 결과는 계산하지 않는다."""
    base=dict(zip(FIELDS,(100,200,100,200,65536,65536,65536,16,11,32,22,0,15,128,0,896,700,1024,768,99,88,17,19,1,0,317,419,32768)))
    rows=[]
    for target,flags,origin in itertools.product(((100,200),(107,209),(93,191),(100,199),(99,200),(1000,1900)),(0,2,16,18),((128,0),(0,28))):  # 동등/상하좌우·DIB/시차/두 viewport 원점을 교차한다.
        rows.append(dict(base,targetX=target[0],targetY=target[1],flags=flags,viewX=origin[0],viewY=origin[1]))
    for zoom,base_x,base_y in itertools.product((1,32768,65535,65537,98304,131072,0xffffffff),(16,0x7fffffff),(11,-17)):  # 배율의 signed FILD/Q16/하위32비트 곱을 공급한다.
        rows.append(dict(base,targetX=317,targetY=419,targetZoom=zoom,baseX=base_x,baseY=base_y))
    for key,values in (('denominator',(1,2,-15,128)),('dirtyProfile',(0,2,3,4)),('viewWidth',(0,-1,2147483647)),('viewHeight',(0,-1,2147483647)),('x',(-2147483648,2147483647)),('y',(-2147483648,2147483647))):  # 별도 정수/dirty 경계를 원문에 그대로 공급한다.
        for value in values:  # 시차가 선택되는 실제 입력에서 한 필드씩 바꾼다.
            rows.append(dict(base,flags=16,targetX=317,targetY=419,**{key:value}))
    for mutation,zoom in itertools.product(range(1,7),(65536,98304)):  # 효과 뒤 현재 데이터 재조회를 두 분기로 확인한다.
        rows.append(dict(base,targetX=317,targetY=419,targetZoom=zoom,mutation=mutation,flags=16))
    return rows


def encode(values):
    """원문 전체 DWORD 배열을 손실 없는 콜론 구분자로 직렬화한다."""
    return ':'.join(str(value&0xffffffff) for value in values)


def main():
    """두 정밀도 실제 결과만 fixture로 쓰고 기존 파일은 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    oracle=Oracle();rows=[]
    for case in inputs():  # 각 입력은 두 x87 정밀도에서 독립 실행한다.
        results=[oracle.run(case,control) for control in (0x27f,0x37f)]
        assert results[0]==results[1],case
        value=results[0];rows.append([*(str(case[key]) for key in FIELDS),encode(value['renderer']),encode(value['snapshot']),encode(value['target']),
            '|'.join(encode(entry) for entry in value['dirty']) or '-',encode(value['globals']),value['events']])
    with args.fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# '+'\t'.join((*FIELDS,'renderer16','snapshot16','target6','dirty5','globals11','events'))+'\n');stream.writelines('\t'.join(row)+'\n' for row in rows)
    report=dict(host=platform.node(),cases=len(rows),runs=len(rows)*2,differences=0,originalExecutions=0,executedBodies=['497960','497490','4834c0'],
        boundaries=['446a10/446a40 현재 커서 잠금/해제','4660f0 현재 Gump redraw','4a1ad0/4a1e00 화면/배경 이동','497580/4977f0 기존 dirty 추가','4a41c0 현재 마우스 장치'],
        sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'originals/Netstorm.exe',Path(__file__).resolve(),args.fixture.resolve())})
    with args.report.open('x',encoding='utf-8') as stream: json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(cases=len(rows),runs=len(rows)*2,differences=0)))


if __name__=='__main__':
    main()
