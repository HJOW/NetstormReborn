#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스의 별도 TYPE 그림·실제 표시/캡처 연결과 빌드 기록을 읽기 전용으로 확인한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/dais-draw-session.json'  # 제품 실행 이월과 독립 원본 관측을 구분하는 기록이다.


def main():
    """현재 바이트·원본768행·소스 핀·빌드를 확인하며 제품 검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 새 제품과 기존 기대값을 각자의 실제 해시로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():
            assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/dais-draw-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():
        assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dais-draw-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==native['uniqueInputs']==768 and native['normalReturns']==1536 and all(row[1]==row[2] for row in rows)
    session=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    display=(ROOT/'cpppj/src/client/RawCampaignFactoryDisplay.cpp').read_text('utf-8')
    assert 'if (IsPatchDais(raw)) return -1;' in session and 'DrawDaisAdditional(core.pool.Slot(sid)' in session
    assert 'o::DrawDaisAdditional(core.pool.Slot(object)' in display and 'o::IsPatchDais(core.pool.Slot(sid))' in display
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"다이스 추가 그림: 독립 원본768행·표시/캡처·Release 빌드·{len(record['sourcePinChanges'])}개 핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
