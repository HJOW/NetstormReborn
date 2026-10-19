#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 폭발 원문을 유지하고 dais HP 보유 분기를 구별하는 독립64입력을 추가한다."""
import argparse
import collections
import itertools
import json
import socket
from decomp_bomb_blast import Oracle, ROOT, base, digest

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-blast-dais-x86.tsv'  # 원래2082입력과 별도로 보존하는 추가 원문이다.
REPORT = ROOT/'cpppj/bomb-blast-dais-native.json'  # 기존 원문을 변경하지 않았음을 함께 고정한다.


def cases():
    """dais+허용genus에서 TYPE_HAS_HP보다 abstract가 우선하는 분기를 구별한다."""
    rows = []
    # 입력만 교차하며 HP 보유/피해 결과는 실제 PE가 계산한다.
    for op, kind, genus, flags1, extra in itertools.product((0, 2), (70, 158), (0x600000, 0x410000), (0, 0x10), (0, 1, 16, 17)):
        row = base(); row[0:5] = op, kind, genus, flags1, extra; row[22] = 0; rows.append(tuple(row))
    return rows


def main():
    """기존2082입력을 고치지 않고 추가 원문만 최초 동결하거나 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    primary = json.loads((ROOT/'cpppj/bomb-blast-native.json').read_text('utf-8'))
    # 원래2082입력과 그 실행 도구/보호PE를 먼저 읽기 전용 확인한다.
    for name, wanted in primary['sha256'].items():
        assert digest(ROOT/name) == wanted, name
    if options.verify:
        record = json.loads(REPORT.read_text('utf-8'))
        # 추가 원문의 출처도 각각 확인한다.
        for name, wanted in record['sha256'].items():
            assert digest(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == record['uniqueInputs'] == 64 and record['normalReturns'] == 128
        assert all(row[1:6] == row[6:11] for row in rows)
        print('Bomb dais 추가 원문 감사64/128반환·기존2082입력 유지'); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 추가 원문을 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    # 동일한 원본 몸체를 독립 추가 입력에서 두 정밀도로 실행한다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input return53 effects53 raw53 boss53 width53 return64 effects64 raw64 boss64 width64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('cpppj/bomb-blast-native.json', 'tools/decomp_bomb_blast_dais.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=234, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'),
                  uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles),
                  nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())),
                  scope='dais+허용genus에서abstract/TYPE_HAS_HP 우선순위와자격→효과를원본으로구별했다. 큰효과경계는기존2082입력과동일하며원문/기존도구는변경하지않았다.',
                  sha256={name: digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
