#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 보행 원문·발사체 깊이/겹침·작은 걸음과 연속 화면의 출처를 감사한다."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/display-motion-runtime.json'  # 현재 연결과 실행 당시의 관측을 구별한다.


def sha(path):
    """파일 전체 바이트의 현재 해시를 구하며 과거 실행 해시는 갱신하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 보고 좌표와 프레임의 변화만 세고 단순 조작 시도를 이동 성공으로 세지 않는다."""
    run=json.loads((directory/'run.json').read_text('utf-8'))
    observed=states(directory/'states.tsv')
    samples=[]
    # 같은 내 사제의 실제 raw 좌표/프레임을 보고 순서로 읽는다.
    for label,state in observed.items():
        # 한 보고 시점의 실제 자산 목록만 조회한다.
        for sid,row in state['objects'].items():
            if row[:2]==['priest','1']:
                samples.append(dict(label=label,sid=sid,x=float(row[5]),y=float(row[6]),frame=int(row[7])))
                break
    walk=[sample for sample in samples if sample['label'].startswith('walk')]
    largest=0.0
    # 연속 화면에 대응하는 보고 사이의 최대 거리를 확인한다.
    for previous,current in zip(walk,walk[1:]):
        assert previous['sid']==current['sid']
        largest=max(largest,math.hypot(current['x']-previous['x'],current['y']-previous['y']))
    return dict(returnCode=run['returnCode'],binarySha256=run['binarySha256'],originalsUnchanged=run['originalsUnchanged'],
                samples=samples,uniqueWalkPositions=len({(s['x'],s['y']) for s in walk}),uniqueWalkFrames=len({s['frame'] for s in walk}),
                largestReportedStep=largest,lastGate=list(observed.values())[-1].get('mission_gate'))


def main():
    """원본 기대값과 현재 소스/실행 증거를 게임 없이 다시 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--inspect',type=Path)
    options=parser.parse_args()
    if options.inspect:
        print(json.dumps(inspect(options.inspect.resolve()),ensure_ascii=False))
        return
    if not options.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    report=json.loads(REPORT.read_text('utf-8'))
    # 현재 핀만 확인하며 기존 관측/역사 기대값은 다시 생성하지 않는다.
    for group in ('sources','evidence'):
        # 실제 사용한 모든 출처의 바이트를 확인한다.
        for name,value in report[group].items():
            assert sha(ROOT/name)==value,name
    full=json.loads((ROOT/report['validation']['fullTests']).read_text('utf-8'))
    assert full['tests']==1746 and full['failed']==full['returnCode']==0
    # 집중 검사는 전체 검사와 같은 실행 파일을 사용해야 한다.
    for name,expectedCount in zip(report['validation']['focusedTests'],(5,76,4),strict=True):
        result=json.loads((ROOT/name).read_text('utf-8'))
        assert result['tests']==expectedCount and result['failed']==result['returnCode']==0 and result['binarySha256']==full['binarySha256']
    native=json.loads((ROOT/'cpppj/walk-shape-native.json').read_text('utf-8'))
    assert native['inputs']==native['normalReturns']==1170
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/walk-shape-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==1170 and sum(row[3].split(',')[:2]!=['1092616192','1101004800'] for row in rows)==481
    # 화면 결과도 원문 상태에서 다시 읽으며 관측 시도의 횟수를 성공으로 바꾸지 않는다.
    for name,value in report['observations'].items():
        actual=inspect(ROOT/name)
        assert actual==value and actual['returnCode']==0 and actual['originalsUnchanged']
        assert actual['uniqueWalkPositions']>10 and actual['uniqueWalkFrames']>5 and actual['largestReportedStep']<1.0
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('보행1170원문·실제 깊이/겹침 픽셀·연속 화면·전체1746검사 출처 통과')


if __name__=='__main__':
    main()
