#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude24의 최초 원본·현재 경로/속도 연결·빌드·기존 보고서 보존을 읽기 전용 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT/'cpppj/dude-path-session.json'  # 제품 실행을 이월한 이번 단위의 실제 기록이다.


def main():
    """실제 파일의 해시와 분배를 확인하며 C++검사·게임·창은 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보호 원본 근거를 각각 해시로 확인한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT/name) == expected, name
    native = json.loads((ROOT/'cpppj/dude-path-native.json').read_text('utf-8'))
    # 최초 독립 관측 도구·원문·보호PE는 변경하지 않는다.
    for name, expected in native['sha256'].items():
        assert sha(ROOT/name) == expected, name
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dude-path-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == native['uniqueInputs'] == 30 and native['normalReturns'] == 60 and all(row[1] == row[2] for row in rows)
    session = (ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'runtime.Point(runtime.Count()-1)' in session and 'motion->Paths().StartDude({sid,{x,y}})' in session
    assert 'kind==70 ? dudeSpeeds[kind].speed' in session and 'effects.path.dudeHalt=' in session and 'effects.path.dudeArrival=' in session
    assert 'FillMissing(hitEffects.dudes.pathEndpoint)' not in session and 'FillMissing(hitEffects.dudes.startPath)' not in session
    process = (ROOT/'cpppj/src/o/RawPathProcess.cpp').read_text('utf-8')
    assert 'priest_ || worm_ || dude_' in process and 'process->dude_' in process
    assert record['build']['returnCode'] == 0 and record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT/record['build']['log']) == record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"Dude24: 독립30행·실제 BFS/부모/현재 속도 연결·Release 빌드·{len(record['sourcePinChanges'])}개 핀 보존 확인, 제품 실행 이월")


if __name__ == '__main__':
    main()
