#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 정기·초기화의 최초 원본과 현재 Kernel/목록 연결·빌드·이력 보존을 감사한다."""
import argparse
import hashlib
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT/'cpppj/dude-regular-session.json'  # 제품 검사 실행 이월과 이번 코드/빌드 근거다.


def main():
    """원문·현재 소스·빌드·핀 역변환만 읽으며 게임/C++검사는 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보호된 원본 관측 파일은 별도 해시로 확인한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT/name) == expected, name
    native = json.loads((ROOT/'cpppj/dude-regular-native.json').read_text('utf-8'))
    # 최초 관측 결과와 실제 도구/보호PE/정밀 자료의 바이트를 보존한다.
    for name, expected in native['sha256'].items():
        assert sha(ROOT/name) == expected, name
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dude-regular-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == native['uniqueInputs'] == len({row[0] for row in rows}) == 1600
    assert native['normalReturns'] == 3200 and all(row[1] == row[2] for row in rows)
    initializers = [row for row in rows if row[0].split(',')[0] == '6']
    assert len(initializers) == 3 and all(row[1].split('|')[-1] == '5278724,1,30,0' for row in initializers)
    # 최초 수집 뒤 바뀐 것은 보고서의 한국어 표기 문자열뿐이며 함수 몸체/결과는 같다.
    source = (ROOT/'tools/decomp_dude_regular.py').read_text('utf-8')
    original = source.replace('4508b0 및 실제 RNG', '4508b0'+chr(0x3068)+'실제 RNG')
    assert hashlib.sha256(original.encode('utf-8')).hexdigest() == native['observationToolSha256']
    assert sha(ROOT/'cpppj/tests/fixtures/dude-regular-x86.tsv') == record['firstCapture']['fixtureSha256']
    session = (ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'InitializeDudePopulation(dudePopulation);' in session and 'return dudeRegular->Handle(sid,event,count,payload)' in session
    assert 'DudeRegularState{gunState.checking,core.bookkeeping.providers,dudeProviderIndex}' in session and 'return dudeBehavior->CheckSpace(sid)' in session
    assert record['build']['returnCode'] == 0 and record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT/record['build']['log']) == record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"Dude 정기: 독립1600행·최초 결과 보존·실제 Kernel/30칸/공유 목록·Release·{len(record['sourcePinChanges'])}개 핀 보존 확인, 제품 실행 이월")


if __name__ == '__main__':
    main()
