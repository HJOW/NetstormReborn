#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미션 실패/중단 제어의 최초 PE·보호 본문·실제 Client 연결·빌드/과거 핀 보존을 감사한다."""
import argparse
import hashlib
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins
from cpp_goal_stage_entry_smoke import ARCHIVE
from cpp_menu_smoke import source

REPORT=ROOT/'cpppj/mission-abort-session.json'  # 현재 연결과 실제 실행 이월을 구분한다.


def main():
    """파일/정적 연결/과거 기록만 읽고 게임·C++검사·창은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 독립 원문을 각 그룹의 실제 전체 바이트로 감사한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/mission-abort-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/mission-abort-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==native['normalReturns']==1002 and len({tuple(row[:2]) for row in rows})==1002
    assert sum(row[0]=='A' for row in rows)==native['abortInputs']==960 and sum(row[0]=='S' for row in rows)==native['scriptInputs']==42
    assert native['instructions']==580682 and native['actualQueueCalls']==426 and not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/mission-abort-x86.tsv')==record['firstCapture']['fixtureSha256']
    tell=source(ROOT/'originals',ARCHIVE,'d/tell.english');assert hashlib.sha256(tell).hexdigest()==record['authoredTellSha256']
    text=tell.decode('cp1252');assert '$Button=Replay Mission,MissionRestart,0' in text and '$Button=Main Menu,LeaveBattle,1' in text
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    assert 'return MissionControlScriptCode(name)!=0xffffffffU' in client and 'QueueMissionControlAction(action.command,action.argument,client_.Input(),scriptStrings_)' in client
    assert 'DispatchMissionAbort(event,client_.Input()' in client and 'if (command == "missionabort" || command == "leavebattle")' not in client
    assert 'BeginMission(request.name,request.demo,request.mode)' in client and 'if ((event.code&0x03ffffffU)==kLeaveBattleCommand)' in client
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"미션 중단: 최초1002입력·보호 Replay/ABORT·실제 큐 수신·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
