#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 네 해시를 사용하는 주문 대상 검색의 독립 원문/현재 소스/회귀를 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 원시 검사 자료의 기준이다.
REPORT = ROOT/'cpppj/bomb-targets-cpp.json'  # 구현 범위와 종료된 검사의 출처다.


def sha(path):
    """파일 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """과거 기대값/실행 기록을 변경하지 않고 이번 구현과 근거를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존된 원시 출력을 각각 확인한다.
    for group in ('sources', 'evidence'):
        # 과거 바이너리의 실행 해시는 현재 바이너리로 덮어쓰지 않는다.
        for name, wanted in record[group].items():
            assert sha(ROOT/name) == wanted, name
    native = json.loads((ROOT/'cpppj/bomb-targets-native.json').read_text('utf-8'))
    # 보호PE/실제 입력 도구/전체 기대값 출처를 확인한다.
    for name, wanted in native['sha256'].items():
        assert sha(ROOT/name) == wanted, name
    assert native['uniqueInputs'] == 300 and native['normalReturns'] == 600
    assert native['nativeCalls']['004b16d0'] == 600 and native['nativeCalls']['004b1810'] > 600
    assert native['nativeCalls']['0041e3c0'] == 600 and native['nativeCalls']['0041def0'] == 600
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/bomb-targets-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == 300 and all(row[1:8] == row[8:15] for row in rows)
    # 같은 최종 검사 바이너리로 실제 종료된 집중/전체 검사를 확인한다.
    for key, count in (('focused', 2), ('full', 1763)):
        result = json.loads((ROOT/record['validation'][key]).read_text('utf-8'))
        assert result['returnCode'] == 0 and result['binarySha256'] == record['binaries']['tests']
        output = (ROOT/record['validation'][key]).with_suffix('.stdout').read_text('utf-8')
        assert re.search(rf'\b{count} test\(s\), 0 failed check\(s\)', output)
        assert result['tests'] == count and result['failed'] == 0
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead']
    # 현재 핀은 현재 소스와 대조하고 당시 변경은 그 단위의 보존 이력으로 확인한다.
    for pin in pins['changes']:
        report = json.loads((ROOT/pin['report']).read_text('utf-8')); value = report
        # 보존된 경로의 정확한 한 필드만 조회한다.
        for key in pin['pointer']:
            value = value[key]
        assert value == sha(ROOT/pin['source'])
        group = '/'+'/'.join(map(str, pin['pointer'][:-1]))
        # 후속 구현의 정상 핀 갱신이 과거 검사 출처를 무효화하지 않도록 이력과 구별한다.
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == group
            and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins'])
            for update in report.get('integration_updates', []))
    assert record['cppImplemented'] and not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    print('Bomb 대상 실제해시/기하300·집중2/0·전체1763/0 및 현재핀 감사 통과')


if __name__ == '__main__':
    main()
