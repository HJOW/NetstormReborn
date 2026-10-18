#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본004d5c60과 실제 다중 정리/강조를 기존 선택 관찰과 분리해 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import decomp_user_selection as original

ROOT = original.ROOT  # 원본과 새 관측 파일의 공통 작업 경로다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/user-selection-clear-x86.tsv'  # 기존420개 자료를 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/user-selection-clear.json'  # 실제 CPU 관측과 명시 가상 경계를 기록한다.
original.ENTRIES = (original.ENTRIES[0], 0x4d5c60, *original.ENTRIES[2:])  # 인자 없는 operation1만 새 실제 입구로 바꾼다.
original.HELPERS = (*original.HELPERS, 0x4d5490)  # 그 안의 다중 선택 정리도 실제 몸체로 허용한다.


def cases():
    """enabled/선택/모드/물리 배열/현재 개수와 callback 재조회의 입력만 만든다."""
    base = [1, 0, 0, 0, 51, 0, 1, 0, 0, 0, 0, 0x10000, 1, 0, 1, 0, 0, 0, 0, 4, 73,
            original.bits(12.5), original.bits(-3.25), original.bits(12.5), original.bits(-3.25), 2, 0, 0, 0, 320, 240]
    rows = []  # 기대 반환/현재 전역/효과 순서는 실제 기계어만 계산한다.
    for enabled, selected, count, variant, mode in itertools.product((0, 1, 2), (0, 51), (0, 2, 5), (0, 1), (0, 4)):
        row = base.copy();row[6], row[4], row[25], row[26], row[19] = enabled, selected, count, variant, mode;rows.append(tuple(row))
    # 정지/콘솔은 이 입구가 읽지 않으며 가상 통지가 현재 개수/기억 좌표를 바꿀 수 있다.
    for paused, blocked, mutation in itertools.product((0, 1), (0, 1), (0, 1, 7, 12)):
        row = base.copy();row[5], row[7], row[27] = paused, blocked, mutation;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


def main():
    """새 원본 관측 또는 기존 관측의 출처 검사만 수행하며 게임/OS 창을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for path, wanted in report['sha256'].items():
            assert original.digest(ROOT / path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2
        assert not report['precisionDifferentInputs'] and report['nativeCalls']['004d5c60'] == len(rows) * 2
        print('후속 선택 해제 원본 출처 통과:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [original.Oracle(), original.Oracle()];rows = []  # 기존 ABI/실행 범위/쓰기 감시를 그대로 사용한다.
    for row in cases():
        outputs = [oracle.sample(row, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], row;rows.append((','.join(map(str, row)), *outputs[0]))
    FIXTURE.write_text('# input result events current\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', 'utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'tools/decomp_user_selection.py', 'tools/decomp_user_selection_clear.py',
             'cpppj/tests/fixtures/user-selection-clear-x86.tsv', 'extracted/refined/originals/functions.tsv']
    report = dict(host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'),
                  entry='004d5c60', controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles),
                  instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())),
                  precisionDifferentInputs=[], originalGameExecutions=0,
                  scope='후속 해제/다중 정리/강조의 실제 몸체다. 가상 통지/커서/역투영/이동/장치만 명시 경계이며 실제 선택 생성이나 게임 완주를 주장하지 않는다.',
                  sha256={path:original.digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps({k:report[k] for k in ('uniqueInputs', 'normalReturns', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
