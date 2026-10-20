#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""지정11미션의 공통 프레임 연결·보호 Header·최초 분기/빌드와 과거 핀 보존을 감사한다."""
import argparse
import hashlib
import json
import re
from cpp_dais_load_audit import ROOT, sha, verify_pins
from cpp_goal_stage_entry_smoke import MISSIONS, ARCHIVE
from cpp_menu_smoke import source
from cpp_fort_smoke import script_value

REPORT=ROOT/'cpppj/mission-common-session.json'  # 이번 연결과 후속 통합 실행을 구별한다.


def script_evidence():
    """원본 디스크 우선 VFS로 지정11개 스크립트의 바이트/관련 Header/안내 존재를 읽는다."""
    rows=[]
    # 제품 출력으로 기대값을 만들지 않고 보호 원문을 직접 읽는다.
    for mission in MISSIONS:
        data=source(ROOT/'originals',ARCHIVE,f'd/{mission}.english');text=data.decode('cp1252')
        header={key:script_value(text,key) for key in ('missionType','tutorialNumber','missionNumber','demo','demoRestartTimer')}
        rows.append({'mission':mission,'sha256':hashlib.sha256(data).hexdigest(),'header':header,'sections':re.findall(r'^\[([^\]]+)\]',text,re.M)})
    return rows


def main():
    """코드/기록/원본만 읽고 C++/게임/창을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 독립 원문/기존 fixture의 전체 바이트를 각각 감사한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/mission-common-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/mission-common-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==native['uniqueInputs']==native['sliceObservations']==108 and native['normalFunctionReturns']==0
    assert native['instructions']==9844 and native['actualAtolCalls']==40 and not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/mission-common-x86.tsv')==record['firstCapture']['fixtureSha256']
    scripts=script_evidence();assert scripts==record['scriptEvidence'] and len(scripts)==11
    assert all(row['header']['missionType']=='Tutorial' for row in scripts)
    assert scripts[8]['header']['demo']=='1' and 'Demo' not in scripts[10]['sections']
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    binding=(ROOT/'cpppj/src/client/RawCampaignTutorialOneBindings.cpp').read_text('utf-8')
    assert 'if (type=="tutorial") {' in client and 'if (missionRuntime_->gate.tutorialNumber>=1' not in client
    assert 'client_.Input().Push(kMissionReloadCommand,0,0)' in client and 'BeginMission(request.name,request.demo,request.mode)' in client
    assert 'ApplyMissionDemo(mission_->Get("demo").value_or(""),missionRuntime_->gate.demo)' in client  # 현재 Header 조회는 실제 Guide 단계1로 이동했다.
    assert 'if (state_.demo || state_.tutorialNumber<1 || state_.tutorialNumber>6) return;' in binding
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"공통 미션: 보호11스크립트·최초108분기·모든 Tutorial 프레임/큐 연결·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
