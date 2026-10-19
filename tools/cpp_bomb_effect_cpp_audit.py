#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""0x5f 수신 C++의 독립 원문·현재 소스·종료된 회귀 검사를 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 별도 원시 검사 기록의 기준이다.
REPORT = ROOT/'cpppj/bomb-effect-cpp.json'  # 이번 구현/검증 범위를 기록한 보고서다.


def sha(path):
    """파일 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """기대값이나 과거 바이너리 해시를 바꾸지 않고 실제 근거를 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 제품 소스와 보존된 실제 출력은 각각의 전체 바이트로 확인한다.
    for group in ('sources', 'evidence'):
        # 과거 실행 기록을 현재 실행 파일 해시로 갱신하지 않는다.
        for name, wanted in record[group].items():
            assert sha(ROOT/name) == wanted, name
    native = json.loads((ROOT/'cpppj/bomb-effect-native.json').read_text('utf-8'))
    # 제품 구현 전에 동결한 원본/도구/전체 원문 해시를 확인한다.
    for name, wanted in native['sha256'].items():
        assert sha(ROOT/name) == wanted, name
    assert native['uniqueInputs'] == 1676 and native['normalReturns'] == 3352
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/bomb-effect-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == 1676 and all(row[1:7] == row[7:13] for row in rows)
    before = json.loads((ROOT/record['validation']['historicalPinAudit']['before']).read_text('utf-8'))
    after = json.loads((ROOT/record['validation']['historicalPinAudit']['after']).read_text('utf-8'))
    assert before['expectedFailure'] and before['returnCode'] != 0 and after['returnCode'] == 0
    # 같은 최종 검사 바이너리의 집중/전체 종료 결과만 이번 검증으로 센다.
    for key, count in (('focused', 2), ('full', 1761)):
        result = json.loads((ROOT/record['validation'][key]).read_text('utf-8'))
        assert result['returnCode'] == 0 and result['binarySha256'] == record['binaries']['tests']
        output = (ROOT/record['validation'][key]).with_suffix('.stdout').read_text('utf-8')
        assert re.search(rf'\b{count} test\(s\), 0 failed check\(s\)', output)
        if key == 'full':
            assert result['tests'] == count and result['failed'] == 0
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead']
    # 해당 JSON 경로의 현재 소스 핀만 바뀌었으며 원본 기대값은 유지한다.
    for pin in pins['changes']:
        report = json.loads((ROOT/pin['report']).read_text('utf-8'))
        value = report
        # 보존된 경로로 정확한 한 필드를 조회한다.
        for key in pin['pointer']:
            value = value[key]
        assert value == sha(ROOT/pin['source'])
        group = '/'+'/'.join(map(str, pin['pointer'][:-1]))
        # 과거 새 해시는 당시 단위의 이력으로 확인하여 후속 정상 핀 갱신을 허용한다.
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == group
            and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins'])
            for update in report.get('integration_updates', []))
    assert record['cppImplemented'] and not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    print('0x5f 원문1676·집중2/0·전체1761/0 및 현재 소스 핀 감사 통과')


if __name__ == '__main__':
    main()
