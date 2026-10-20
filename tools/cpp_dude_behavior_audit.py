#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 부모의 원본 관측·현재 Session/수명 연결과 빌드를 읽기 전용으로 확인한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/dude-behavior-session.json'  # 이번 구현/빌드와 제품 실행 이월을 구별하는 기록이다.


def main():
    """기존 기대값과 현재 바이트·핀·빌드를 검사하며 게임/제품 검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 원본 근거는 각자의 실제 해시로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():
            assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/dude-behavior-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():
        assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dude-behavior-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==native['uniqueInputs']==410 and native['normalReturns']==820 and all(row[1]==row[2] for row in rows)
    session=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'dudeBehavior->PostDestroy(event.sid,event.flags)' in session and 'DudeBehaviorState{gunState.checking,dudePopulation}' in session
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"Dude 부모: 독립410행·Session 삭제/공통 후처리·Release 빌드·{len(record['sourcePinChanges'])}개 핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
