#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""개별 다리 편집00421f20과 실제 품질 하위를 게임/창 없이 독립 CPU로 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path

from decomp_editor_bridges import BridgeOracle, ROOT, POOL, BRIDGE

# 전체 변경의 이전 fixture와 구분하는 개별 선택/free 슬롯 입력이다.
OBSERVED = (1, 5, 6, 7, 8, 9, 10, 11, 15000)


class OneBridgeOracle(BridgeOracle):
    """기존 하위 품질 몸체를 공유하되 새 진입과 Editor 관문은 실제421f20만 실행한다."""
    def __init__(self):
        """현재 PC의 완료된 디컴파일 범위로 진입을 허용하고 기존 fixture는 건드리지 않는다."""
        super().__init__()
        self.entries['One'] = 0x421f20
        self.function_entries.add(0x421f20)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(source, delimiter='\t')}
        for part in facts[0x421f20]['ranges'].split(';'):
            # inclusive 내보내기의 실제 코드 범위만 실행 허용 목록에 추가한다.
            first, last = (int(value, 16) for value in part.split('-'))
            self.allowed.append((first, last + 1))

    def prepare_one(self, seed, editor):
        """현재 Editor와 free이면서 TYPE는 bridge인 슬롯도 기대값 계산 없이 설치한다."""
        self.prepare(seed, 0)
        self.mu.mem_write(0x5c85a4, struct.pack('<I', editor))
        free = bytearray(self.mu.mem_read(POOL + 11 * 50, 50))
        struct.pack_into('<I', free, 0, 0x5034c8)
        free[10], free[11] = BRIDGE, 1
        struct.pack_into('<H', free, 12, 0xa5ff)
        self.mu.mem_write(POOL + 11 * 50, bytes(free))

    def observe(self):
        """선택 전후의 관심9슬롯 전체 바이트를 읽고 상태를 보정하지 않는다."""
        return ';'.join(f'{sid}:{bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex()}' for sid in OBSERVED)


def main():
    """미지원 품질/비Editor/비다리/예약·dead·free SID를 새 원문 fixture로 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    target = ROOT / 'cpppj/tests/fixtures/editor-bridge-one-x86.tsv'
    if target.exists() != args.verify:
        raise RuntimeError('신규 fixture만 생성하며 기존 관측 비교에는 --verify가 필요합니다')
    oracle = OneBridgeOracle()
    evidence = oracle.code_evidence()
    rows, runs = [], []  # 입력과 독립 기계어 관측을 제품 코드와 구분한다.
    for editor in (0, 1, 0x80000000):
        # Editor는 단순 참/거짓이며 missionPost만으로 개별 변경을 허용하지 않는다.
        for quality in (0, 1, 2, 3, 0xffffffff):
            # 원본은 품질 미지원이어도 Editor/TYPE를 먼저 읽는다.
            for sid in (1, 5, 6, 9, 10, 11, 15000, 0):
                # SID1·dead6·free11을 임의로 제외하지 않는다.
                for seed in (1, 0x12345678):
                    # Hard의 실제 후보 선택 난수 소비도 보존한다.
                    oracle.prepare_one(seed, editor)
                    oracle.call('One', [sid, quality])
                    assert not oracle.heap_live and oracle.allocations == oracle.frees
                    random = struct.unpack('<I', oracle.mu.mem_read(0x532710, 4))[0]
                    rows.append((editor, quality, sid, seed, oracle.observe(), ';'.join(oracle.trace), random))
                    runs.append(dict(editor=editor, quality=quality, sid=sid, seed=seed,
                                     instructions=oracle.instructions, redraws=len(oracle.trace),
                                     allocations=oracle.allocations, frees=oracle.frees,
                                     functions={f'{a:08x}': n for a, n in sorted(oracle.entered.items())}))
    output = io.StringIO(newline='')
    writer = csv.writer(output, delimiter='\t', lineterminator='\n')
    writer.writerow(('editor', 'quality', 'sid', 'seed', 'slots', 'trace', 'random'))
    writer.writerows(rows)
    encoded = output.getvalue().encode('utf-8')
    if args.verify:
        assert target.read_bytes() == encoded, '이전 독립 관측과 불일치'
    else:
        target.write_bytes(encoded)
    assert oracle.code_evidence() == evidence
    report = dict(unit=516, host='HJOW-Athlon', lastFullDecompile='HJOW-Athlon / 2026-10-23',
                  originalGameExecuted=False, attackFlyersExecuted=False,
                  binarySha256=oracle.sha256, codeEvidence=evidence, codeImageUnchanged=True,
                  fixtureSha256=hashlib.sha256(encoded).hexdigest(), rows=len(rows),
                  normalReturns=len(rows), verifiedExistingFixture=args.verify, observedSids=OBSERVED,
                  replaced=['operator_new(1024)', 'free', 'virtual+88'], runs=runs)
    (ROOT / 'cpppj/editor-bridge-one-native-20261028.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(rows=len(rows), fixtureSha256=report['fixtureSha256'], verified=args.verify)))


if __name__ == '__main__':
    main()
