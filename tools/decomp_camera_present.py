#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 Present 말미의 진단 관문·두 목표 제한·적용 호출을 실제 x86로 관찰한다. 게임 실행은 없다."""
import argparse
import csv
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP
from unicorn.x86_const import *

ENTRY, END = 0x49a081, 0x49a1ca  # dirty 출력/clear와4a17e0 뒤부터 정상 함수 종료까지다.
FIELDS = ('diagnostic', 'width', 'height', 'cells', 'limitX', 'limitY', 'targetX', 'targetY', 'mutate',
          'afterDiagnostic', 'afterWidth', 'afterHeight', 'afterCells', 'afterLimitX', 'afterLimitY', 'afterTargetX', 'afterTargetY')  # 앞17열은 결과 없는 현재 입력이다.
GLOBALS = ((0x5318ec, 'diagnostic'), (0x5c78f0, 'width'), (0x5c78f4, 'height'), (0x531928, 'cells'))  # 실제 별도 전역 주소다.
TARGETS = ((0x48, 'limitX'), (0x4c, 'limitY'), (0x50, 'targetX'), (0x54, 'targetY'))  # 현재 Renderer 목표/한계만 넣는다.


class Oracle(MemoryOracle):
    """원문 제한·FTOL은 실행하고 화면 flip 및 뒤497960은 독립 호출 경계로 기록한다."""
    def __init__(self):
        """보호 PE를 읽고 실제 구간과 FTOL의 정밀 범위만 허용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = [(ENTRY, END)]
        for part in facts[0x4e49c0]['ranges'].split(';'):  # FTOL의 불연속 원문 범위도 그대로 허용한다.
            first, last = (int(value, 16) for value in part.split('-'))
            self.allowed.append((first, last+1))

    def install(self, prefix=''):
        """독립 현재 입력이나 flip 효과가 바꾼 현재 전역·목표를 실제 주소에 쓴다."""
        for address, key in GLOBALS:  # 원본이 읽는 네 전역은 사본으로 합치지 않는다.
            self.put(address, self.case[prefix+key[0].upper()+key[1:] if prefix else key])
        for offset, key in TARGETS:  # 현재 한계/목표도 flip 효과에서 바뀔 수 있다.
            self.put(POOL+offset, self.case[prefix+key[0].upper()+key[1:] if prefix else key])

    def instruction(self, mu, address, size, data):
        """선택된 호출과 인자를 관측하고 나머지 계산은 보호 명령만 실행한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        if address == 0x4a16d0:
            self.events.append('F:'+':'.join(map(str, self.words(POOL+0x50, 2))))
            if self.case['mutate']:
                self.install('after')
            self.ret()
            return
        if address == 0x497960:
            assert mu.reg_read(UC_X86_REG_ECX) == POOL
            self.events.append('C:'+':'.join(map(str, self.words(POOL+0x50, 2))))
            self.ret()
            return
        if not any(first <= address and address+size <= last for first, last in self.allowed):
            raise RuntimeError(f'미등록 Present 카메라 명령 {address:08x}')
        self.instructions += 1

    def run(self, case, control):
        """부모가 저장한 스택/this를 준비하고 정상 반환·전체 현재 출력을 확인한다."""
        self.case = case
        self.events = []
        self.completed = False
        self.instructions = 0
        self.mu.mem_write(POOL, bytes(0x100))
        self.install()
        self.mu.mem_write(STACK-20, bytes(64))
        self.put(STACK, STOP)
        self.mu.reg_write(UC_X86_REG_ESP, STACK-20)
        self.mu.reg_write(UC_X86_REG_ESI, POOL)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.emu_start(ENTRY, STOP+1, count=10000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP) == STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        values = [self.words(address, 1)[0] for address, _ in GLOBALS]
        values += list(self.words(POOL+0x48, 4))
        return dict(values=values, events=';'.join(self.events) or '-', instructions=self.instructions)


def inputs():
    """분기 결과를 계산하지 않고 부호 경계·화면/세계 크기·flip 재조회 입력을 구성한다."""
    base = dict(zip(FIELDS, (6,1024,768,256,3072,2048,100,200,0,6,1024,768,256,3072,2048,100,200)))
    rows = []
    for limits, targets, cells in itertools.product(((3072,2048),(-1,-1),(0,0),(2147483647,2147483647),(100,200)),
            ((100,200),(-1,-2),(4000,4000),(-2147483648,2147483647),(2147483647,-2147483648)),(256,64,0,-2147483648)):  # 첫 한계와 별도 세계 픽셀 한계를 교차한다.
        rows.append(dict(base,limitX=limits[0],limitY=limits[1],targetX=targets[0],targetY=targets[1],cells=cells))
    for diagnostic in (*range(9),0xffffffff):  # 정확1 관문과 다른 전체 DWORD를 구별한다.
        rows.append(dict(base,diagnostic=diagnostic))
    for key in ('width','height','cells','limitX','limitY','targetX','targetY'):  # 각 signed 입력을 독립적으로 바꾼다.
        for value in (-2147483648,-1,0,1,64,256,0x10000000,2147483647):  # 곱셈/시프트/뺄셈32비트 감김을 포함한다.
            rows.append(dict(base,**{key:value}))
    for cells, diagnostic in itertools.product((256,1,0x10000000,-1),(1,5,6)):  # flip 뒤 현재 데이터와 재검사하지 않는 진단값을 확인한다.
        rows.append(dict(base,mutate=1,afterDiagnostic=diagnostic,afterWidth=1280,afterHeight=800,afterCells=cells,
            afterLimitX=9999,afterLimitY=9999,afterTargetX=4000,afterTargetY=4000))
    return rows


def main():
    """독립 두 정밀도 결과를 그대로 쓰고 기존 fixture/보고서는 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args = parser.parse_args()
    oracle = Oracle()
    rows = []
    for case in inputs():  # 입력마다 원문 정상 반환과 두 정밀도 일치를 먼저 확인한다.
        results = [oracle.run(case, control) for control in (0x27f,0x37f)]
        assert results[0] == results[1], case
        rows.append([*(str(case[key]) for key in FIELDS),*(str(value) for value in results[0]['values']),results[0]['events']])
    with args.fixture.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# '+'\t'.join((*FIELDS,'outDiagnostic','outWidth','outHeight','outCells','outLimitX','outLimitY','outTargetX','outTargetY','events'))+'\n')
        stream.writelines('\t'.join(row)+'\n' for row in rows)
    report = dict(host=platform.node(),cases=len(rows),runs=len(rows)*2,differences=0,originalExecutions=0,
        entry=ENTRY,end=END,executedBodies=['49a081..49a1c9','4e49c0'],
        boundaries=['4a16d0 화면 flip/명시 현재 데이터 변경','497960 적용 호출만 기록, 본체 실행 제외'],
        sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'originals/Netstorm.exe',Path(__file__).resolve(),args.fixture.resolve())})
    with args.report.open('x',encoding='utf-8') as stream:
        json.dump(report,stream,ensure_ascii=False,indent=2)
        stream.write('\n')
    print(json.dumps(dict(cases=len(rows),runs=len(rows)*2,differences=0)))


if __name__ == '__main__':
    main()
