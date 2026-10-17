#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""마나 볼트의45ac90/8c 표시를 기존 마나 표시와 별도 원본 자료로 검증한다."""
import argparse
import csv
import itertools
import json
import struct
from decomp_cursor_mana_display import ManaDisplayOracle,ROOT,POOL,digest

FIXTURE=ROOT/'cpppj/tests/fixtures/mana-bolt-display-x86.tsv' # 실제105의 WORD 무관·음수 여백 관측이다.
REPORT=ROOT/'cpppj/mana-bolt-display.json' # 기존 마나103의 fixture/보고서를 덮어쓰지 않는다.

class Oracle(ManaDisplayOracle):
    """현재 정밀표의45ac90을 공통 bounds/clip/dirty 몸체에 연결한다."""
    def __init__(self):
        """실제 함수의 정확한 불연속 범위를 추가한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x45ac90)
        for part in row['ranges'].split(';'):
            low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))

    def sample(self,case):
        """실제 생성자 VT와 같은 TYPE 입력에서 두 가상 진입/dirty를 관측한다."""
        word,extra,flags,zoom,position,suppressed,alternate=case;x,y=((20.,20.),(.25,.5),(38.,40.))[position]
        self.shape(flags,2,True,91,[(100,120,-3,5),(130,140,7,-9),(50,70,2,3),(80,90,4,6)])
        self.view(640,480,7,9,zoom,(12,9,620,450),suppressed);self.slot(1,extra,4,x,y)
        raw=POOL+self.sid*self.stride;self.mu.mem_write(raw,struct.pack('<I',0x509810));self.mu.mem_write(raw+12,struct.pack('<H',word));before=self.raw()
        self.invoke(0x4ad670 if alternate else 0x45ac90,this=raw);assert before==self.raw();return before,*self.dirty_output()

def main():
    """두 정밀도의 실제 부분 갱신 결과 전체를 저장하거나 SHA를 확인한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,sha in report['sha256'].items():assert digest(ROOT/path)==sha,path
        print('마나 볼트 표시 SHA 확인');return
    oracles=[Oracle(),Oracle()]
    # 실제 변경 영역 초기화/작성 몸체도 그대로 실행한다.
    for oracle,control in zip(oracles,(0x027f,0x037f)):oracle.start(control)
    rows=[]
    # WORD/선택 비트/그림자/확대/위치/억제/가상8c 입력을 교차한다.
    for case in itertools.product((0,1,0x8000),(0,4),(0,0x40000),(32768,65536,98304),range(3),(0,1),(0,1)):
        a,b=(oracle.sample(case) for oracle in oracles);assert a==b;rows.append((','.join(map(str,case)),*a))
    FIXTURE.write_text('# input7 raw full dirty\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=set(json.loads((ROOT/'cpppj/cursor-mana-display.json').read_text(encoding='utf-8'))['sha256']);paths.update(['tools/decomp_mana_bolt_display.py','cpppj/src/o/SquidDisplay.cpp','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidUnpop.cpp','cpppj/tests/CursorManaTests.cpp',FIXTURE.relative_to(ROOT).as_posix()])
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon / 2026-10-16',uniqueInputs=len(rows),normalReturns=len(rows)*2,precisionDifferentInputs=[],sha256={p:digest(ROOT/p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(f'마나 볼트 표시 {len(rows)}입력/{len(rows)*2}정상 반환, 정밀도 차이0')

if __name__=='__main__':main()
