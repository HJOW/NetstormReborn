#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Guide 교체의 최초 PE 관측과 실제 Client 큐/문자열 연결·빌드/과거 기록 보존을 감사한다."""
import argparse
import hashlib
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins
from cpp_goal_stage_entry_smoke import ARCHIVE
from cpp_menu_smoke import source

REPORT=ROOT/'cpppj/guide-transitions-session.json'  # 현재 연결과 실제 플레이 검증을 구별하는 기록이다.


def main():
    """현재 파일과 독립 원문만 읽으며 게임·C++검사·창을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 보존된 원본 근거를 각 그룹의 전체 바이트로 검사한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/guide-transitions-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/guide-transitions-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert sum(row[0]=='T' for row in rows)==native['transitionInputs']==256
    assert sum(row[0]=='S' for row in rows)==native['scriptInputs']==30
    assert len(rows)==native['normalReturns']==286 and native['instructions']==393149 and native['formatBoundaryCalls']==100
    assert not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/guide-transitions-x86.tsv')==record['firstCapture']['fixtureSha256']
    authored=source(ROOT/'originals',ARCHIVE,'d/TheWarBegins.english')
    assert hashlib.sha256(authored).hexdigest()==record['authoredMissionSha256']
    assert '<$Config,Done{mission.fileName}=1>' in authored.decode('cp1252') and '$Button=Next Mission,MissionBegin,MasterOfWhirligigs' in authored.decode('cp1252')
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    transitions=(ROOT/'cpppj/src/client/GuideMissionTransitions.cpp').read_text('utf-8')
    scriptHeader=(ROOT/'cpppj/src/client/ScriptCommand.h').read_text('utf-8')
    script=(ROOT/'cpppj/src/client/ScriptCommand.cpp').read_text('utf-8')
    assert 'QueueMissionControlAction(action.command,action.argument,client_.Input(),scriptStrings_)' in client
    assert 'DispatchGuideMissionCommand(event,hooks)' in client and 'ForkGuideMission(parameter,MissionTransitionEffects())' in client
    assert 'RestartGuideMission(request,MissionTransitionEffects())' in client and 'EndGuideMission(MissionTransitionEffects())' in client
    assert 'const std::string name(scriptStrings_.Resolve(token))' in client
    assert 'hooks.release=[this] { ClearWorld();mission_.reset(); }' in client
    assert 'RunTutorialMissionStage(missionRuntime_->state,6,{})' in client
    assert 'std::bit_cast<std::int32_t>(parameter)' in transitions and 'event.y<=100' in transitions
    assert 'serial_{100}' in scriptHeader and 'serial_==0x7fffffffU' in script
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"Guide 교체: 최초286입력·실제 Begin/Fork/Restart/End 큐 연결·소유 문자열·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
