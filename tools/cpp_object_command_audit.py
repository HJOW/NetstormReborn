#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 객체 명령의 원본 자료, 현재 소스 연결과 빌드 기록을 읽기 전용으로 확인한다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 소스와 보존한 분석/빌드 기록의 저장소 기준이다.
REPORT = ROOT / 'cpppj/object-command-session.json'  # 구현 범위와 실행 검증 이월을 구별한다.


def sha(path):
    """파일 전체 바이트를 해시하며 기대값이나 원본을 수정하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """실제 실행을 시작하지 않고 현재 바이트/보존 이력/빌드 종료를 점검한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 제품 소스와 과거 관측/빌드 증거를 각자의 실제 바이트로 확인한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/object-command-native.json').read_text('utf-8'))
    # 독립 원본 관측 자료는 제품 변경에 맞춰 다시 생성하지 않는다.
    for name, expected in native['sha256'].items():
        assert sha(ROOT / name) == expected, name
    rows = [line.split('\t') for line in (ROOT / 'cpppj/tests/fixtures/object-command-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == native['normalReturns'] == 320
    assert sum(row[0] == 'originals' for row in rows) == 160
    assert sum(row[0] == 'originalCD' for row in rows) == 160
    build = json.loads((ROOT / record['validation']['build']).read_text('utf-8'))
    assert build['returnCode'] == 0 and not build['testsExecuted'] and not build['gameExecuted']
    assert sha(ROOT / build['log']) == build['logSha256']
    pins = json.loads((ROOT / record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToBaseline']
    # 현재 소스 핀과 이번 갱신 이력은 과거 실행/원본 기대값과 구별해 확인한다.
    for pin in pins['changes']:
        report = json.loads((ROOT / pin['report']).read_text('utf-8'))
        value = report
        for key in pin['pointer']:
            value = value[key]
        assert value == pin['new'] == sha(ROOT / pin['source'])
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == '/' + '/'.join(map(str, pin['pointer'][:-1])) and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins']) for update in report.get('integration_updates', []))
    assert record['carrierParentIntegrated'] and record['commonMenuInputIntegrated']
    assert record['runtimeTestsDeferred'] and not record['fullRegressionCompleted'] and not record['goalComplete']
    assert record['originalGameExecutions'] == record['cloneGameExecutions'] == 0
    print('공통 객체 명령: 원본320반환·현재 소스/핀·Release 빌드 기록 확인, 제품 실행 검증은 이월')


if __name__ == '__main__':
    main()
