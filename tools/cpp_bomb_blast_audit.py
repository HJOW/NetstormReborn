#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""폭발 주문 자격/효과의 독립 원문·현재 구현·종료된 회귀를 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 별도 원시 검사 기록의 기준이다.
REPORT = ROOT/'cpppj/bomb-blast-cpp.json'  # 이번 구현 범위와 실제 검사의 출처다.


def sha(path):
    """전체 파일 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """과거 원문/바이너리를 바꾸지 않고 현재 구현과 원본 증거를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존된 원시 출력을 각각 확인한다.
    for group in ('sources', 'evidence'):
        # 실제 실행한 과거 바이너리 해시는 현재 실행 파일로 덮어쓰지 않는다.
        for name, wanted in record[group].items():
            assert sha(ROOT/name) == wanted, name
    total = 0
    # 기존2082입력과 별도64입력을 각각 독립 출처로 확인한다.
    for stem, expected in (('bomb-blast', 2082), ('bomb-blast-dais', 64)):
        native = json.loads((ROOT/f'cpppj/{stem}-native.json').read_text('utf-8'))
        # 보호PE와 실제 입력 도구/기대값의 각 전체 해시를 확인한다.
        for name, wanted in native['sha256'].items():
            assert sha(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in (ROOT/f'cpppj/tests/fixtures/{stem}-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert native['uniqueInputs'] == len(rows) == expected and native['normalReturns'] == 2*expected
        assert all(row[1:6] == row[6:11] for row in rows)
        total += expected
    assert total == 2146
    # 같은 최종 검사 바이너리에서 새 효과와 기존 공통 좌표 검사가 모두 종료됐는지 확인한다.
    for name in record['validation']['focused']:
        result = json.loads((ROOT/name).read_text('utf-8')); output = (ROOT/name).with_suffix('.stdout').read_text('utf-8')
        assert result['returnCode'] == result['failed'] == 0 and result['tests'] == 2
        assert result['binarySha256'] == record['binaries']['tests'] and re.search(r'\b2 test\(s\), 0 failed check\(s\)', output)
    assert len(record['validation']['focused']) == 2
    full = ROOT/record['validation']['full']; result = json.loads(full.read_text('utf-8'))
    assert result['returnCode'] == result['failed'] == 0 and result['tests'] == 1765
    assert result['binarySha256'] == record['binaries']['tests']
    assert re.search(r'\b1765 test\(s\), 0 failed check\(s\)', full.with_suffix('.stdout').read_text('utf-8'))
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead']
    # 현재 값은 현재 소스와, 당시 핀 변경은 보존 이력과 구별하여 확인한다.
    for pin in pins['changes']:
        report = json.loads((ROOT/pin['report']).read_text('utf-8')); value = report
        # 원시 기록의 정확한 JSON 경로만 조회한다.
        for key in pin['pointer']:
            value = value[key]
        assert value == sha(ROOT/pin['source'])
        group = '/'+'/'.join(map(str, pin['pointer'][:-1]))
        # 후속 정상 핀 갱신이 과거 실행 증거를 무효화하지 않도록 단위별 이력을 확인한다.
        assert any(update.get('unit') == record['unit'] and any(item['path'] == pin['source'] and item['group'] == group
            and item['previous'] == pin['old'] and item['current'] == pin['new'] for item in update['pins'])
            for update in report.get('integration_updates', []))
    assert record['cppImplemented'] and record['runtimeHooksPrepared']
    assert not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    print('Bomb 폭발 원문2146·집중4/0·전체1765/0 및 현재 소스 핀 감사 통과')


if __name__ == '__main__':
    main()
