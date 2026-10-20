#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Guide 수명·월드 진입 Pause/단계3 Start·현재 핀/보호 원문과 빌드 근거를 감사한다."""
import argparse
import json
import struct
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/mission-lifecycle-session.json'  # 이번 단계 연결과 후속 통합 실행을 구별한다.


def tutorial_vtable():
    """보호 PE 섹션 표를 직접 읽어 Tutorial 가상표의 실제11개 함수 주소를 반환한다."""
    data=(ROOT/'originals/Netstorm.exe').read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0]
    sections=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0];imageBase=struct.unpack_from('<I',data,pe+24+28)[0]
    target=0x5149ec-imageBase;table=pe+24+optional
    # 실제 RVA가 들어 있는 한 섹션에서만 원본 가상표 바이트를 읽는다.
    for index in range(sections):
        virtualSize,virtualAddress,rawSize,rawPointer=struct.unpack_from('<4I',data,table+index*40+8)
        if virtualAddress<=target and target+44<=virtualAddress+rawSize:
            return list(struct.unpack_from('<11I',data,rawPointer+target-virtualAddress))
    raise RuntimeError('보호 Tutorial 가상표의 PE 섹션 없음')


def main():
    """현재 코드/독립 원문/과거 보고서만 읽고 게임·C++검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 동결된 원본 관측을 별도 그룹으로 검사한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/mission-lifecycle-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/mission-lifecycle-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==len({row[0] for row in rows})==native['uniqueInputs']==831
    assert native['normalReturns']==1262 and native['instructions']==44306 and not native['gameExecuted'] and not native['osExecuted']
    assert native['calls']['00463a40']==56 and native['calls']['004c2a40']==60
    table=tutorial_vtable();assert table==record['tutorialVtable'] and table[4]==0x4c2b20 and table[10]==0x4c2a40
    assert sha(ROOT/'cpppj/tests/fixtures/mission-lifecycle-x86.tsv')==record['firstCapture']['fixtureSha256']
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    begin=client.split('void UberGump::BeginMission(',1)[1].split('void UberGump::CreateMission(',1)[0]
    create=client.split('void UberGump::CreateMission(',1)[1].split('GuideMissionTransitionHooks UberGump::MissionTransitionEffects()',1)[0]
    load=client.split('void UberGump::LoadMissionWorld(',1)[1].split('void UberGump::StartMissionWorld()',1)[0]
    initialize=client.split('void UberGump::InitializeMission()',1)[1].split('void UberGump::LoadMissionWorld(',1)[0]
    assert 'BeginGuideMission(name,effects)' in begin and 'CreateMission(std::string(target),demo,mode)' in begin
    assert 'make_unique<MissionRuntime>(*mission_,demo,mode)' in create and 'FortTemplate::Parse' not in create
    assert 'PauseMissionWorldEntry' in load and 'missionRuntime_->start->Start()' not in load
    # 단위259의 실제 Init은 Header/flags를 먼저 쓰고 설치는 같은 큐의 다음 루프에 수행한다.
    assert 'InitializeTutorialMission(' in initialize and 'state_.Post({"LoadMissionFort"' in initialize
    assert 'missionRuntime_->state.flags|=1U' not in load and '*missionRuntime_->start' in load
    assert 'if (guide_) guide_->Run()' in client and 'hooks.initialize=[this] { InitializeMission(); };hooks.start=[this] { StartMissionWorld(); }' in client
    assert 'guideHooks.pending=[this] { return state_.Pending() ? 1U : 0U; }' in client
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"미션 수명: 최초831입력·실제 Guide/단계1·3 분리·진입 Pause·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
