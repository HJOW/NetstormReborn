#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tutorial Init 제어/공유 전역·다음 루프 로드와 첫 원문/빌드·과거 보고서 보존을 읽기 전용으로 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/tutorial-init-session.json'  # 실제 큰 State7/공장/제품 실행을 이번 제어 연결과 구별한다.


def main():
    """코드/기록/첫 원문만 읽고 제품/게임/창을 시작하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 독립 근거/원문은 별개의 전체 바이트로 확인한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/tutorial-init-native.json').read_text('utf-8'))
    assert native['uniqueInputs'] == native['normalFunctionReturns'] == 152 and native['instructions'] == 167373
    assert native['crtCalls'] == dict(atol=398, lower=2128, upper=152)
    assert not native['gameExecuted'] and not native['osExecuted']
    # 제품 변경 전에 동결한 도구/전체 관측은 후속 결과로 덮어쓰지 않는다.
    for name in ('tools/decomp_tutorial_init.py', 'cpppj/tests/fixtures/tutorial-init-x86.tsv', 'cpppj/tutorial-init-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/tutorial-init-20261021-first-capture' / name.split('/')[-1])
    rows = [row.split('\t') for row in (ROOT / 'cpppj/tests/fixtures/tutorial-init-x86.tsv').read_text('utf-8').splitlines() if not row.startswith('#')]
    assert len(rows) == len({row[0] for row in rows}) == 152 and all(len(row) == 6 for row in rows)
    body = (ROOT / 'cpppj/src/client/TutorialInit.cpp').read_text('utf-8')
    assert body.index('Effect(hooks.clearTemplates)()') < body.index('globals.tutorialNumber=') < body.index('globals.missionNumber=') < body.index('Effect(hooks.resetOptions)()') < body.index('state.namePrefix=0') < body.index('Effect(hooks.set)') < body.index('globals.loginSubscriber=') < body.index('Effect(hooks.requestFort)') < body.index('Effect(hooks.header)()') < body.index('flags|=1U')
    assert 'if (globals.tutorialNumber) globals.tutorialSeen=1' in body and 'state.scratch[name.size()]=0' in body
    client = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    initialize = client.split('void UberGump::InitializeMission()', 1)[1].split('void UberGump::LoadMissionWorld(', 1)[0]
    load = client.split('void UberGump::LoadMissionWorld(', 1)[1].split('void UberGump::StartMissionWorld()', 1)[0]
    assert 'state_.Post({"LoadMissionFort"' in initialize and 'InitializeTutorialMission(' in initialize
    assert 'make_unique<GameWorld>' not in initialize and 'std::span(*missionRestrictions_)' in initialize
    assert 'requestedFort ? *requestedFort : mission_->FortName()' in load
    assert 'services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_' in load
    assert 'LoadMissionWorld(action.argument)' in client and 'hooks.initialize=[this] { InitializeMission(); }' in client
    assert 'missionRuntime_->state.flags|=1U' not in load and 'PauseMissionWorldEntry' in load
    assert 'tutorialSeen_=0' in load and 'SetTotalMadeAllowAll(*missionRestrictions_,1)' in client
    clear = client.split('void UberGump::ClearWorld()', 1)[1].split('bool UberGump::HasWorld()', 1)[0]
    assert clear.index('state_.Cancel("LoadMissionFort")') < clear.index('missionRuntime_.reset()')
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'std::array<std::uint8_t,87>& battleOptions' in session and 'std::array<std::uint32_t,256>& restrictions' in session
    assert 'battleOptions(services.missionOptions ? *services.missionOptions : localBattleOptions)' in session
    assert 'restrictions(services.missionRestrictions ? *services.missionRestrictions : localRestrictions)' in session
    login = (ROOT / 'cpppj/src/client/RawCampaignSessionLogin.inc').read_text('utf-8')
    assert 'if (!s.services.missionOptions) ResetMissionBattleOptions' in login and 's.services.missionSubscriber.value_or(1)' in login
    assert record['build']['returnCode'] == record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert record['initControlConnected'] and record['sharedTablesConnected'] and not record['wholeState7Restored'] and not record['wholeTemplateManagerRestored']
    verify_pins(record)
    print(f"Tutorial Init 최초152입력·실제 CRT/옵션·공유 Header/다음 루프 로드·Release·{len(record['sourcePinChanges'])}핀 보존 확인; 제품 실행 이월")


if __name__ == '__main__':
    main()
