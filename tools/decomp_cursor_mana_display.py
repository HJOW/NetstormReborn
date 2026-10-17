#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 마나88와8c 꼬리 호출·부분 사각형·변경 영역 표를 대조한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_display_oracle import DisplayOracle, POOL, ROOT

FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-mana-display-x86.tsv'  # 기존 표시/마나 원문을 덮어쓰지 않는 새 입력이다.
REPORT=ROOT/'cpppj/cursor-mana-display.json'  # 수동 확인한 누락 함수 범위와 실제 읽은 모든 근거를 보존한다.


def digest(path):
    """근거의 전체 바이트를 감사한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class ManaDisplayOracle(DisplayOracle):
    """공통 실제 bounds/clip/dirty에 실제 마나88와 가상8c 꼬리 호출을 추가한다."""
    def __init__(self):
        """누락된473770은 보호 PE의 ret까지,499240은 현재 정밀 함수표의 범위로 제한한다."""
        super().__init__('originals')
        assert bytes(self.mu.mem_read(0x4737b2,1))==b'\xc3'
        assert bytes(self.mu.mem_read(0x4ad670,8))==bytes.fromhex('8b01ffa088000000')
        self.allowed.extend(((0x473770,0x4737b3),(0x4ad670,0x4ad678)))  # 직접 확인한 두 불연속 함수의 정확한 끝이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 499240은 부분 사각형을 실제 clip/dirty 함수로 넘긴다.
        for part in rows[0x499240]['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 마나의 공통 firstPop/postPop과 같은8c thunk를 실제 PE에서 확인한다.
        for offset in (0x10,0x14,0x18,0x24,0x48,0x7c,0x84,0x8c):assert bytes(self.mu.mem_read(0x50c3b0+offset,4))==bytes(self.mu.mem_read(0x501dd0+offset,4))

    def sample(self,case):
        """합성 SHP/카메라 입력에 실제 몸체를 실행하고 raw 불변과 변경표 전체를 읽는다."""
        word,extra,flags,zoom,position,suppressed,alternate=case  # 결과와 분리한 입력7개다.
        x,y=((20.,20.),(.25,.5),(38.,40.))[position]
        self.shape(flags,2,True,91,[(100,120,-3,5),(130,140,7,-9),(50,70,2,3),(80,90,4,6)])
        self.view(640,480,7,9,zoom,(12,9,620,450),suppressed);self.slot(1,extra,4,x,y)
        raw=POOL+self.sid*self.stride;self.mu.mem_write(raw,struct.pack('<I',0x50c3b0));self.mu.mem_write(raw+12,struct.pack('<H',word));before=self.raw()
        self.invoke(0x4ad670 if alternate else 0x473770,this=raw);assert before==self.raw();return before,*self.dirty_output()


def main():
    """두 실제 정밀도에서 원문을 비교하며 사용한 과거 보조 범위 자료도 SHA로 고정한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 기대 사각형은 다시 계산하지 않는다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('마나 표시 SHA 감사:',len(rows));return
    reads=set();original_open=Path.open  # 실제 읽은 기존 관찰기의 보조 범위/타입 근거를 추적한다.
    def tracked_open(path,*values,**keywords):
        """읽기 경로만 기록하며 원본 open의 인수/동작을 바꾸지 않는다."""
        mode=values[0] if values else keywords.get('mode','r')
        if 'r' in mode:reads.add(path.resolve())
        return original_open(path,*values,**keywords)
    Path.open=tracked_open
    try:
        oracles=[ManaDisplayOracle(),ManaDisplayOracle()]
        # 격리 실제 Reset/Create는 각 실행기에서 한 번씩만 수행한다.
        for oracle,control in zip(oracles,(0x027f,0x037f)):oracle.start(control)
        cases=list(itertools.product((0,1,0x8000),(0,4),(0,0x40000),(32768,65536,98304),range(3),(0,1),(0,1)));rows=[]
        # 변경 영역/억제/full 상태를 포함한 두 원문 반환 전체를 비교한다.
        for case in cases:
            outputs=[oracle.sample(case) for oracle in oracles];assert outputs[0]==outputs[1];rows.append((','.join(map(str,case)),*outputs[0]))
    finally:Path.open=original_open
    FIXTURE.write_text('# input7 raw full dirty\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={'originals/Netstorm.exe','tools/decomp_cursor_mana_display.py',FIXTURE.relative_to(ROOT).as_posix(),'extracted/refined/originals/functions.tsv'}  # 새 실제 진입의 핵심 근거다.
    # 가져온 관찰기 코드와 실제 읽은 범위 자료 전체를 포함한다.
    for name in ('display','pop','unpop','creation','sid',''):
        paths.add('tools/decomp_'+(name+'_' if name else '')+'oracle.py')
    for path in reads:
        if path.is_relative_to(ROOT) and path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=['00473770','004ad670','00499240'],manualRanges=['00473770-004737b2','004ad670-004ad677'],uniqueInputs=len(rows),normalReturns=2*len(rows),precisionDifferentInputs=[],actualDisplayCalls=[dict(oracle.display_calls) for oracle in oracles],sha256={path:digest(ROOT/path) for path in sorted(paths)},limitations=['473770은 현재 정밀표에 빠진 함수여서 PE의 실제67바이트/ret와 가상 표를 직접 확인했다. 정밀 산출물을 수정하지 않았다.','기존 공통 표시 관찰기의 실제 bounds/clip/dirty/Reset/Create와 보조 범위 자료를 재사용하며 읽은 자료를 모두 고정한다.','실제 OS 창/게임/GUI 실행은 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('마나 표시:',len(rows),'고유 입력, 두 정밀도 차이0, 정상 반환',facts['normalReturns'])


if __name__=='__main__':main()
