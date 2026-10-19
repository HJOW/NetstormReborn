#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Bomb Regular의 독립 원문·현재 제품·종료된 회귀 검사의 출처를 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 보존된 검사 파일의 기준이다.
REPORT = ROOT/'cpppj/bomb-regular-cpp.json'  # 구현 범위와 실제 검사 출처를 고정한다.


def sha(path):
    """파일 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """기존 기대값이나 실행 기록을 다시 쓰지 않고 검증 범위와 일치를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 제품 현재 소스와 보존된 검사의 원시 바이트를 구별하여 확인한다.
    for group in ('sources', 'evidence'):
        # 바이너리의 과거 실행 해시를 현재 실행 파일로 덮어쓰지 않는다.
        for name, expected in record[group].items():
            assert sha(ROOT/name) == expected, name
    native = json.loads((ROOT/'cpppj/bomb-regular-native.json').read_text('utf-8'))
    # 원본과 독립 실행 도구/fixture의 출처를 그대로 확인한다.
    for name, expected in native['sha256'].items():
        assert sha(ROOT/name) == expected, name
    assert native['uniqueInputs'] == 741 and native['normalReturns'] == 1482
    rows = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/bomb-regular-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == 741 and all(row[1:4] == row[4:7] for row in rows)
    # 집중/전체 검사는 같은 최종 바이너리의 실제 종료 결과여야 한다.
    for key, count in (('focused', 2), ('full', 1759)):
        result = json.loads((ROOT/record['validation'][key]).read_text('utf-8'))
        assert result['returnCode'] == 0 and result['binarySha256'] == record['binaries']['tests']
        output = (ROOT/record['validation'][key]).with_suffix('.stdout').read_text('utf-8')
        assert re.search(rf'\b{count} test\(s\), 0 failed check\(s\)', output)
        if key == 'full':
            assert result['tests'] == count and result['failed'] == 0
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead']
    # 빌드 목록의현재핀만갱신했으며각새값은실제현재소스와같아야한다.
    for pin in pins['changes']:
        report = json.loads((ROOT/pin['report']).read_text('utf-8'))
        value = report
        # 원문에보존한JSON경로로정확한한필드만조회한다.
        for key in pin['pointer']:
            value = value[key]
        assert value == sha(ROOT/pin['source'])
        group = '/'+'/'.join(map(str, pin['pointer'][:-1]))
        # 당시 새 핀은 해당 단위의 보존 이력과 대조한다. 후속 구현의 현재 해시와 혼동하지 않는다.
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == group
            and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins'])
            for update in report.get('integration_updates', []))
    assert not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    print('Bomb Regular 원문741/1482반환·집중2/0·전체1759/0 및 현재핀 감사 통과')


if __name__ == '__main__':
    main()
