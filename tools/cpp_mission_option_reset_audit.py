#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미션 기본 옵션/실제 로그인/네 번째 범위표·최초 원문과 과거 실행 기록을 읽기 전용으로 감사한다."""
import argparse
import json
import re
import struct
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/mission-option-reset-session.json'  # 현재 연결/컴파일과 이월한 제품 실행을 구별한다.


def main():
    """게임/창/C++검사를 시작하지 않고 전체 파일 해시와 실제 관측 원문·소스 연결을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 직접 소스와 근거는 과거 실행 바이너리/기대값과 별개의 사전이다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/mission-option-reset-native.json').read_text('utf-8'))
    assert native['uniqueInputs'] == 296 and native['normalFunctionReturns'] == 320 and native['instructions'] == 47276
    assert not native['gameExecuted'] and not native['osExecuted']
    # 제품 변경 전에 동결한 도구/fixture/JSON의 전체 바이트를 유지한다.
    for name in ('tools/decomp_mission_option_reset.py', 'cpppj/tests/fixtures/mission-option-reset-x86.tsv', 'cpppj/mission-option-reset-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/mission-option-reset-20261021-first-capture' / name.split('/')[-1])
    rows = [line.split('\t') for line in (ROOT / 'cpppj/tests/fixtures/mission-option-reset-x86.tsv').read_text('utf-8').splitlines() if not line.startswith('#')]
    assert len(rows) == len({tuple(row[:4]) for row in rows}) == 296
    assert {kind: sum(row[0] == kind for row in rows) for kind in 'RWPF'} == dict(R=256, W=16, P=8, F=16)
    # 전체 출력/양쪽 이웃·예약 영역과 원본 기본 파생 결과를 원문에서 읽는다.
    for row in rows:
        before, after = bytes.fromhex(row[4]), bytes.fromhex(row[5])
        assert len(before) == len(after) == 103 and before[:8] == after[:8] and before[-8:] == after[-8:]
        if row[0] != 'F':
            assert list(after[8:30]) == native['defaults'] and after[30] == 0
            assert before[31:71] == after[31:71]  # 실제 예약 BYTE23..62다.
            assert after[79:87] == bytes(range(8))  # 원본 색 BYTE71..78이다.
        if row[0] == 'P':
            assert row[6:] == ['0000f04100006144', '3000', '50']
    assert struct.unpack('<4f', bytes.fromhex(native['rangeTableHex'])) == (14, 22, 30, 38)
    body = (ROOT / 'cpppj/src/o/RawMissionBattleOptions.cpp').read_text('utf-8')
    login = (ROOT / 'cpppj/src/client/RawCampaignSessionLogin.inc').read_text('utf-8')
    tests = (ROOT / 'cpppj/tests/MissionBattleOptionsTests.cpp').read_text('utf-8')
    defaults = re.search(r'kInitialOptions\{([^}]+)\}', body)
    assert defaults and list(map(int, defaults[1].split(','))) == native['defaults']
    assert 'std::fill_n(options.begin(),23,std::uint8_t{0})' in body
    assert 'std::fill(options.begin()+63,options.end(),std::uint8_t{0})' in body
    assert 'state.rangeTable.size()!=3 && state.rangeTable.size()!=4' in body
    assert 'state_.rangeTable[2]<state_.range' in body and 'if (index>=state_.rangeTable.size())' in body
    assert 'std::array<float,4> ranges{14,22,30,38}' in login
    ctor = login.split('Login(Impl& owner,', 1)[1]
    assert ctor.index('ResetMissionBattleOptions(OriginalEdition::Patch1078,s.battleOptions)') < ctor.index('transport=std::make_unique')
    assert 'MissionBattleOptionState{s.battleOptions' in login and 's.energyState.radiusSquared' in login
    for name in record['newTests']:
        assert 'TEST_CASE(' + name + ')' in tests
    assert record['build']['returnCode'] == record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"미션 기본 옵션 최초296입력·예약/기본4float·실제 로그인 연결/Release·{len(record['sourcePinChanges'])}핀 보존 확인; 제품 실행 이월")


if __name__ == '__main__':
    main()
