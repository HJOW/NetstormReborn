#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 미션 First/F8의 최초 근거·실제 연결·빌드·과거 보고서 보존을 읽기 전용으로 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/mission-first-session.json'  # 현재 제품 연결과 실행 이월을 구분하는 기록이다.


def main():
    """원본/현재 코드/정적 연결/최초 결과만 확인하며 제품이나 게임을 시작하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 동결된 원문은 별도 그룹으로 감사한다.
    for group in ('sources','evidence'):
        for name, expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/mission-first-native.json').read_text('utf-8'))
    for name, expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/mission-first-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==len({row[0] for row in rows})==native['uniqueInputs']==278
    assert sum(row[0].startswith('2,') for row in rows)==native['sliceObservations']==14
    assert native['normalReturns']==264 and native['instructions']==7407 and not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/mission-first-x86.tsv')==record['firstCapture']['fixtureSha256']
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    bindings=(ROOT/'cpppj/src/client/RawCampaignTutorialOneBindings.cpp').read_text('utf-8')
    assert 'QueueMissionFirstKey(event,client_.Input(),true)' in client
    assert 'frameEffects.frame.guideOpen=[this]' in client and 'frameEffects.frame.guideOpen=[] { return false; }' not in client
    assert 'DispatchMissionFirst(event,[this] { MissionFirst(startState_,[this] { TellStage(); }); })' in bindings
    assert 'TellMissionStage(state_,stageShown_' in bindings and 'gate_.PauseForDialog()' in bindings
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"MissionFirst: 최초278입력(함수264·내부 분기14)·실제 Early1~6 연결·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
