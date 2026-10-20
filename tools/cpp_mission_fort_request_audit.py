#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 요청 작성/Init Zone 소유·설치 소비와 독립 첫 원문/Release·과거 JSON 보존을 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/mission-fort-request-session.json'  # 전체 State7/전투 정리 실행과 요청 연결을 구별한다.


def main():
    """제품/게임/C++검사/창을 시작하지 않고 실제 파일/정적 연결/첫 관측을 읽기만 한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 이번 소스/독립 원문/빌드된 바이트는 별도 사전으로 확인한다.
    for group in ('sources', 'evidence', 'binaries'):
        for name, expected in record[group].items():assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/mission-fort-request-native.json').read_text('utf-8'))
    assert native['uniqueInputs'] == native['normalFunctionReturns'] == 144 and native['instructions'] == 83232
    assert not native['gameExecuted'] and not native['osExecuted']
    # 제품 변경 전에 동결한 첫 도구/모든 관측 바이트를 후속 구현으로 교체하지 않는다.
    for name in ('tools/decomp_mission_fort_request.py', 'cpppj/tests/fixtures/mission-fort-request-x86.tsv', 'cpppj/mission-fort-request-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/mission-fort-request-20261021-first-capture' / name.split('/')[-1])
    rows = [line.split('\t') for line in (ROOT / 'cpppj/tests/fixtures/mission-fort-request-x86.tsv').read_text('utf-8').splitlines() if not line.startswith('#')]
    assert len(rows) == len({row[0] for row in rows}) == 144
    # 원문 전체 출력에서 주소의 뒤3BYTE와 이웃/예약·복제의 폭을 확인한다.
    for row in rows:
        assert len(row) == 7
        before, after, packet, copy = (bytes.fromhex(row[index]) for index in (1, 2, 4, 5))
        assert len(before) == len(after) == 160 and len(packet) == 512 and len(copy) == 86
        assert before[:8] == after[:8] and before[-8:] == after[-8:] and before[8+0x5d:8+0x60] == after[8+0x5d:8+0x60]
        assert packet[:86] == copy and int.from_bytes(after[8:12], 'little') == 7
    source = (ROOT / 'cpppj/src/client/MissionFortRequest.cpp').read_text('utf-8')
    assert source.index('Need(hooks.selectFort)') < source.index('MakeLocalMissionZone()') < source.index('globals.next[0x54/4]') < source.index('globals.next[0]=7') < source.index('globals.next[0x7c/4]=globals.port') < source.index('Need(hooks.duplicate)') < source.index('++globals.sequence')
    assert 'globals.next[0x5c/4]&0xffffff00U' in source and 'packet.storage[2]=4;packet.storage[3]=0' in source
    assert 'request.zone.assign(bytes.begin(),bytes.end())' in source and 'return 1' in source
    client = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    initialize = client.split('void UberGump::InitializeMission()', 1)[1].split('void UberGump::LoadMissionWorld(', 1)[0]
    assert initialize.index('CaptureMissionFortZone') < initialize.index('WriteMissionFortRequest(name,') < initialize.index('state_.Post({"LoadMissionFort",missionFortRequest_->fort})')
    assert 'services.missionFortRequest=missionFortRequest_' in client and 'mission_fort_request\\t' in client
    clear = client.split('void UberGump::ClearWorld()', 1)[1].split('bool UberGump::HasWorld()', 1)[0]
    assert clear.index('state_.Cancel("LoadMissionFort")') < clear.index('missionFortRequest_.reset()') < clear.index('missionRuntime_.reset()')
    login = (ROOT / 'cpppj/src/client/RawCampaignSessionLogin.inc').read_text('utf-8')
    assert 'blocks[3]=request->next' in login and 'else fallback=netstorm::zacket::MakeLocalMissionZone()' in login
    assert 'request ? std::span<const std::uint8_t>(request->zone) : fallback->Bytes()' in login
    assert login.index('InstallLocalMissionZone(zone,') < login.index('std::vector<std::uint8_t>{}.swap(request->zone);request->consumed=true') < login.index('blocks[3][0]=8')
    assert record['build']['returnCode'] == record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert record['requestParentConnected'] and record['preparedZoneConsumed'] and not record['wholeState7Restored'] and not record['wholeFortSelectionRestored']
    verify_pins(record)
    print(f"Fort 요청 최초144입력/9BYTE·144/512/86·실제 Init/Zone소비·Release·{len(record['sourcePinChanges'])}핀 보존 확인; State7/제품 실행 이월")


if __name__ == '__main__':main()
