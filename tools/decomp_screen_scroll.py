#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호4a1ad0 DIB 행 이동을 실제 x86로 실행해 겹침·방향·padding의 전체 픽셀을 관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP
from unicorn.x86_const import *

PIXELS=POOL+0x2000  # 실제 PE 주소와 겹치지 않는 명시 인덱스 버퍼다.
PITCH,WIDTH,HEIGHT=16,13,9  # 홀수 논리 폭과 padding을 갖는 독립 입력이다.


class Oracle(MemoryOracle):
    """행/가드 계산은 실제 명령을 실행하고 장치 Lock/Unlock과 CRT memmove만 경계로 둔다."""
    def __init__(self):
        """보호 함수의 정밀 범위와 같은 픽셀 표면을 준비한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x4a1ad0)
        self.allowed=[]
        for part in row['ranges'].split(';'):  # 정확한 함수 명령 범위를 허용한다.
            first,last=(int(value,16) for value in part.split('-'));self.allowed.append((first,last+1))

    def instruction(self,mu,address,size,data):
        """원문이 고른 실제 행 순서/인자를 관측하고 CRT와 장치 경계만 수행한다."""
        if address==STOP:
            self.completed=True;mu.emu_stop();return
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,3)
        if address==0x4a14e0:
            self.events.append('L');self.put(0x5c7920,1);self.ret();return
        if address==0x4a0950:
            self.events.append('U');self.ret();return
        if address==0x4e5b40:
            destination,source,count=args
            assert PIXELS<=destination and destination+count<=PIXELS+PITCH*HEIGHT and PIXELS<=source and source+count<=PIXELS+PITCH*HEIGHT
            self.events.append(f'M:{destination-PIXELS}:{source-PIXELS}:{count}')
            mu.mem_write(destination,bytes(mu.mem_read(source,count)));self.ret(destination);return
        if not any(first<=address and address+size<=last for first,last in self.allowed):
            raise RuntimeError(f'미등록 DIB 이동 명령 {address:08x}')

    def run(self,dx,dy):
        """원문 가드/행 이동을 실행하고 padding을 포함한 전체 버퍼를 읽는다."""
        self.events=[];self.completed=False;self.mu.mem_write(PIXELS,bytes(range(PITCH*HEIGHT)))
        for address,value in ((0x5c78d4,2),(0x5c78ec,PITCH),(0x5c7930,POOL),(POOL+0x88,PIXELS),(0x5c7920,0)):  # 실제 DIB 플래그·표면·pitch와 잠금 깊이 입력이다.
            self.put(address,value)
        for index,value in enumerate((STOP,2,1,9,7,dx,dy)):  # 실제 cdecl 인수이며 좌표/폭/높이는 고정된 독립 입력이다.
            self.put(STACK+index*4,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.emu_start(0x4a1ad0,STOP+1,count=10000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return bytes(self.mu.mem_read(PIXELS,PITCH*HEIGHT)).hex(),';'.join(self.events) or '-'


def main():
    """기대값을 계산하지 않고 원문 전체 픽셀을 새 fixture/보고서에 그대로 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracle=Oracle();rows=[]
    for dy,dx in itertools.product(range(-8,9),range(-10,11)):  # 양쪽 이동과 겹침 없는 가드를 원문에 모두 공급한다.
        pixels,events=oracle.run(dx,dy);rows.append((dx,dy,pixels,events))
    with args.fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# dx\tdy\tpixels144\tevents\n');stream.writelines('\t'.join(map(str,row))+'\n' for row in rows)
    report=dict(host=platform.node(),cases=len(rows),normalReturns=len(rows),originalExecutions=0,executedBodies=['4a1ad0 DIB 분기'],boundaries=['4a14e0/4a0950 DIB 잠금/해제','4e5b40 CRT memmove의 원시 바이트 복사'],
        pitch=PITCH,width=WIDTH,height=HEIGHT,sha256={str(path.resolve().relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'originals/Netstorm.exe',Path(__file__),args.fixture)})
    with args.report.open('x',encoding='utf-8') as stream: json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(cases=len(rows),normalReturns=len(rows))))


if __name__=='__main__':
    main()
