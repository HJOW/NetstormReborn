#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호11미션의 기술 허용 목록·최초 PE·현재 실제 Session 연결과 과거 기록 보존을 감사한다."""
import argparse
import hashlib
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins
from cpp_goal_stage_entry_smoke import MISSIONS, ARCHIVE
from cpp_menu_smoke import source
from cpp_fort_smoke import script_value

REPORT=ROOT/'cpppj/mission-technology-session.json'  # 현재 연결과 후속 실제 플레이를 구별하는 기록이다.


def script_evidence():
    """제품 parser를 실행하지 않고 보호 디스크/VFS의 실제11개 목록과 전체 바이트 SHA를 읽는다."""
    rows=[]
    # 원본 초기 목록의 순서와 빈 값은 별도 정답 생성 없이 보존한다.
    for mission in MISSIONS:
        data=source(ROOT/'originals',ARCHIVE,f'd/{mission}.english')
        rows.append({'mission':mission,'sha256':hashlib.sha256(data).hexdigest(),'techAllowed':script_value(data.decode('cp1252'),'techAllowed')})
    return rows


def main():
    """파일/연결/빌드/역변환만 확인하며 C++검사·게임·창을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 독립 원문/기존 기대값을 각 그룹의 실제 바이트로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/mission-technology-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/mission-technology-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==len({tuple(row[:3]) for row in rows})==native['inputs']==native['normalReturns']==55
    assert native['instructions']==665852 and not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/mission-technology-x86.tsv')==record['firstCapture']['fixtureSha256']
    assert script_evidence()==record['scriptEvidence'] and len(record['scriptEvidence'])==11
    assert all(row['techAllowed'].startswith('deny;all;allow;') for row in record['scriptEvidence'][:6])
    assert all(row['techAllowed'] is None for row in record['scriptEvidence'][6:])
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    load=client.split('void UberGump::LoadMissionWorld(',1)[1].split('void UberGump::StartMissionWorld()',1)[0]
    apply='ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),world_->Session().TotalMade())'
    # 단위259의 guided Header는 같은 전역을 설치 전에 쓰며 기존 비guided 어댑터만 설치 뒤 적용한다.
    assert load.count(apply)==1 and load.index('world_->BeginLocalConnection()')<load.index(apply)
    assert 'if (type=="tutorial" && !missionRuntime_->guided)' in load
    assert 'ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),std::span(*missionRestrictions_))' in client
    assert 'services.missionRestrictions=missionRestrictions_' in load
    binding=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'hooks.allowed=[current](std::uint32_t type) { return current->totalMade->Allowed' in binding
    assert '*productionDeck,*totalMade,contentFinder' in binding
    body=(ROOT/'cpppj/src/client/MissionTechnology.cpp').read_text('utf-8')
    assert 'std::int32_t allowed=1' in body and 'ConfigListItem(visible,index)' in body
    assert 'totalMade.AllowAll(value)' in body and 'totalMade.Allow(type,value)' in body
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"미션 기술 허용: 최초55입력·보호11목록·실제 공용 표/Init 연결·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
