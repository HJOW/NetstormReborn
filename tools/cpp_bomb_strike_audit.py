#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""네 직접 피해 주문의 동결 원문·현재 구현·실제 검사 출처를 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 보존 원시 실행 기록의 기준이다.
REPORT = ROOT/'cpppj/bomb-strike-cpp.json'  # 실제 구현 범위와 검증 출처를 구별한 보고서다.


def sha(path):
    """전체 파일 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """과거 원문/실행을 고치지 않고 현재 소스와 실제 통과 기록을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 구현과 보존 증거의 해시를 각각 검사한다.
    for group in ('sources', 'evidence'):
        for name, wanted in record[group].items():
            assert sha(ROOT/name) == wanted, name
    native = json.loads((ROOT/'cpppj/bomb-strike-native.json').read_text('utf-8'))
    # 실제 원문 생성 도구와 보호PE·정밀 표의 해시를 재계산 없이 확인한다.
    for name, wanted in native['sha256'].items():
        assert sha(ROOT/name) == wanted, name
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/bomb-strike-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert native['uniqueInputs'] == len(rows) == 1420 and native['normalReturns'] == 2840 and native['instructions'] == 428590
    assert all(row[1:6] == row[6:11] for row in rows)
    assert native['nativeCalls']['004558f0'] == 4986
    # 새 네 주문과 기존 폭발은 같은 최종 검사 바이너리에서 모두 통과해야 한다.
    for name in record['validation']['focused']:
        path = ROOT/name; result = json.loads(path.read_text('utf-8'))
        assert result['returnCode'] == result['failed'] == 0 and result['tests'] == 2
        assert result['binarySha256'] == record['binaries']['tests']
        assert re.search(r'\b2 test\(s\), 0 failed check\(s\)', path.with_suffix('.stdout').read_text('utf-8'))
    assert len(record['validation']['focused']) == 2
    full = ROOT/record['validation']['full']; result = json.loads(full.read_text('utf-8'))
    assert result['returnCode'] == result['failed'] == 0 and result['tests'] == 1767
    assert result['binarySha256'] == record['binaries']['tests']
    assert re.search(r'\b1767 test\(s\), 0 failed check\(s\)', full.with_suffix('.stdout').read_text('utf-8'))
    previous = json.loads((ROOT/record['validation']['priorFocused']).read_text('utf-8'))
    assert previous['returnCode'] == previous['failed'] == 1 and previous['tests'] == 2
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead'] and pins['reports'] == 95 and pins['pins'] == 143
    # 현재 핀은 현재 소스와 비교하고 당시 핀 변경은 보존 이력과 비교한다.
    for pin in pins['changes']:
        report = json.loads((ROOT/pin['report']).read_text('utf-8')); value = report
        for key in pin['pointer']:
            value = value[key]
        assert value == sha(ROOT/pin['source'])
        group = '/'+'/'.join(map(str, pin['pointer'][:-1]))
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == group and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins']) for update in report.get('integration_updates', []))
    assert record['cppImplemented'] and record['sharedGameRandom']
    assert not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    print('직접 피해 주문 원문1420·집중4/0·전체1767/0 및 현재 소스 핀 감사 통과')


if __name__ == '__main__':
    main()
