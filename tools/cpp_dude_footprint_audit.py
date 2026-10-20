#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""발자국0의 최초 원본·현재 실패/탐색 연결·빌드·과거 보고서 보존을 읽기 전용 감사한다."""
import argparse
import hashlib
import itertools
import json
import struct
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT/'cpppj/dude-footprint-session.json'  # 실제 제품 실행 이월과 현재 핀을 기록한 문서다.


def first_subset_hash(rows):
    """좌표 경계를 확장하기 전160입력을 현재 원문에서 되살려 최초 전체 바이트와 비교한다."""
    current = {tuple(map(int, row[0].split(','))): row[1:] for row in rows}
    lines = ['# width,height,position,candidateZero,retry,mutation success|effects|frame|remaining|areas|candidateExtra53 output64']
    positions = ((8.75, 8.75), (9.5, 9.5), (10.5, 10.5), (11.25, 10.25))  # 최초 실제 입력의 순서다.
    # 제품 수식으로 출력값을 만들지 않고 최초 입력에 대응하는 현재 실제 관측만 선택한다.
    for (width, height), position, zero, retry, mutation in itertools.product(((0, 0), (0, 1), (1, 0), (1, 1), (3, 2)), range(4), (0, 1), (0, 1), (0, 1)):
        x, y = positions[position]
        bits = tuple(struct.unpack('<I', struct.pack('<f', value))[0] for value in (9.2, 9.3, x, y))
        output = current[(width, height, *bits, zero, retry, mutation)]
        lines.append('\t'.join((','.join(map(str, (width, height, position, zero, retry, mutation))), *output)))
    return hashlib.sha256(('\n'.join(lines)+'\n').encode('utf-8')).hexdigest()


def main():
    """해시·원문·현재 guard/minmax·실제 빌드와 핀 역변환을 확인하며 제품 검사는 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보호 원본 근거의 실제 파일을 각각 확인한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT/name) == expected, name
    native = json.loads((ROOT/'cpppj/dude-footprint-native.json').read_text('utf-8'))
    # 최초 관측 도구·정밀 함수·원본·새 원문은 덮어쓰지 않는다.
    for name, expected in native['sha256'].items():
        assert sha(ROOT/name) == expected, name
    find = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dude-footprint-find-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    fail = [line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/dude-footprint-fail-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(find) == native['finderInputs'] == 480 and len(fail) == native['failedStartInputs'] == 640
    assert native['failedStartReturns'] == 1280 and all(row[-1] == row[-2] for row in find+fail)
    assert sum(edition['normalReturns'] for edition in native['finder'].values()) == 2080
    assert first_subset_hash(fail) == record['firstCapture']['failedStartSha256']
    assert sha(ROOT/'cpppj/tests/fixtures/dude-footprint-find-x86.tsv') == record['firstCapture']['finderSha256']
    source = (ROOT/'cpppj/src/o/RawPathStart.cpp').read_text('utf-8')
    assert 'record.footX<0 || record.footY<0' in source and 'std::min(point.x,other.x)' in source and 'std::max(point.x,other.x)' in source
    finder = (ROOT/'cpppj/src/o/RawSquidFinder.cpp').read_text('utf-8')
    assert 'type.footX<0 || height<0' in finder
    assert record['build']['returnCode'] == 0 and record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT/record['build']['log']) == record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"발자국0: 일반 탐색480·실패640 원문/최초160 보존·현재 minmax/후보·Release·{len(record['sourcePinChanges'])}개 핀 보존 확인, 제품 실행 이월")


if __name__ == '__main__':
    main()
