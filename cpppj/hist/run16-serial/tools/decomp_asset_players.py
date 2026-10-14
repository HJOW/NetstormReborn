#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 신전 PE 몸체에서 현재 Player BYTE와 효과 뒤 owner 변경을 독립 관찰한다."""
import argparse
import csv
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_assetpostpop_oracle import AssetOracle, ROOT, PLAYERS, SPEC, TARGET

FIXTURE = ROOT / 'cpppj/tests/fixtures/asset-player-x86.tsv'  # 기존 입력을 덮지 않는 새 독립 관찰이다.
REPORT = ROOT / 'cpppj/asset-player.json'  # 두 PE 실행·함수 원문·현재 입력의 전체 SHA다.


def sha(path):
    """실행한 실제 원문 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    """결과를 계산하지 않고 원소·flags·extra·현재owner·기존신전·음향효과 입력을 교차한다."""
    # ownerBefore2/localBefore3과 이후 owner1/8을 구별해 callback 뒤의 현재 값을 조사한다.
    return [(kind, flags, extra, 2, owner, 3, local, initial, sound)
            for kind, flags, extra, owner, local, initial, sound in itertools.product(
                (9, 10, 11), (1, 3), (0, 1, 8), (1, 8), (1, 8), (0, 333), (-1, 777))]


class SharedAssetOracle(AssetOracle):
    """외부 Damageable·음향 callback에서 현재 전역을 바꾸고 원본 몸체의 후속 읽기를 관찰한다."""

    def __init__(self, edition):
        """기존 PE 매핑과 허용 함수 범위를 그대로 사용하며 별도 입력 상태만 추가한다."""
        super().__init__(edition)
        self.shared = None  # 현재 입력의 원소/flags/현재 전역 변경 인자다.
        self.injected = False  # 최상위 진입에서 입력을 한 번만 초기화한다.

    def on_instruction(self, mu, address, size, data):
        """실제 몸체를 대체하지 않고 지정된 외부 경계에만 입력 상태 변경을 공급한다."""
        kind, _, _, before, owner, local_before, local_after, initial, sound = self.shared
        field = 0x7c if self.edition == 'originals' else 0x74  # 원본 첫 신전DWORD의 물리 위치다.
        if address == self.p['entries'][kind] and not self.injected:
            self.injected = True
            records = bytearray([0xa5]) * (9 * self.player_stride)  # 신전 바깥의 stale BYTE 입력이다.
            # 각 Player의 네 신전DWORD에 입력만 기록한다. 기대출력은 PE가 결정한다.
            for index in range(9):
                struct.pack_into('<4I', records, index * self.player_stride + field,
                                 initial, 0x11112222, 0x33334444, 0x55556666)
            mu.mem_write(PLAYERS, bytes(records))
            mu.mem_write(self.slot(TARGET) + self.o['owner'], bytes([before]))
            mu.mem_write(self.p['local'], struct.pack('<I', local_before))
        if address == self.p['damageable']:
            mu.mem_write(self.slot(TARGET) + self.o['owner'], bytes([owner]))
        if address == self.p['element']:
            mu.mem_write(self.p['local'], struct.pack('<I', local_after))
            if sound >= 0:
                mu.mem_write(PLAYERS + owner * self.player_stride + field, struct.pack('<I', sound))
        super().on_instruction(mu, address, size, data)

    def observe(self, values):
        """전체 원본 몸체를 정상 반환까지 실행하고 전체9 Player를 수집한다."""
        self.shared = values
        self.injected = False
        kind, flags, extra, *_ = values
        case = dict(kind=kind, flags=flags, extra=extra, battle=1, load=1, boss=0,
                    reserve=0, animation=1, word=25, f1=0x1000, cost=1000,
                    selected=1, has=0, list_count=0)  # 기존 실행기의 외부 경계 입력이다.
        result = self.run_body(case)  # 사건/슬롯/목록의 독립 PE 출력이다.
        result[3] = bytes(self.mu.mem_read(PLAYERS, 9 * self.player_stride)).hex()
        return [*result, self.read(self.p['local'])]


def generate():
    """새 입력의 원본 관찰을 별도 fixture와 보고서에 기록한다."""
    rows = []  # 판본별 입력과 실제 전체 BYTE 결과다.
    files = {Path(__file__), ROOT / 'tools/decomp_assetpostpop_oracle.py', ROOT / 'tools/decomp_owner_oracle.py'}
    counts = {}  # 판본별 정상 반환/진단/명령 수다.
    # 같은 원본 몸체로 두 판본을 각각 실행한다.
    for edition in SPEC:
        oracle = SharedAssetOracle(edition)
        # 기대결과 계산 없이 정해진 독립 입력을 모두 실행한다.
        for values in inputs():
            rows.append([edition, *values, *oracle.observe(values)])
        assert oracle.assertions == 0
        counts[edition] = dict(inputs=len(inputs()), returns=oracle.returns, assertions=oracle.assertions,
                               instructions=oracle.instructions)
        files.update(oracle.exports)
        files.add(ROOT / oracle.spec['binary'])
    FIXTURE.write_text('# 실제 PE의 현재 Player·callback 후 owner 독립 관찰. 게임/OS 진입 없음.\n' +
                       '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8')
    files.add(FIXTURE)
    record = dict(schema=1, host='HJOW-Athlon', last_decompile_date='2026-10-14',
                  new_native_inputs=len(rows), prior_input_recount=0, editions=counts,
                  files={path.relative_to(ROOT).as_posix(): sha(path) for path in sorted(files)})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('현재 Player/효과 뒤 owner 독립 PE 관찰:', len(rows))


def verify():
    """보존한 독립 입력과 전체 원문 SHA·정상 반환 수를 검증한다."""
    record = json.loads(REPORT.read_text(encoding='utf-8'))  # 원본 실행 당시 고정 자료다.
    # 모든 함수 원문·PE·기대값의 전체 바이트를 검증한다.
    for name, expected in record['files'].items():
        assert sha(ROOT / name) == expected, name
    rows = [row for row in csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t')
            if row and not row[0].startswith('#')]  # 원본 입력과 출력 행이다.
    # 판본별 입력 순서를 계산된 기대출력과 무관하게 확인한다.
    for edition, counts in record['editions'].items():
        actual = [row[1:10] for row in rows if row[0] == edition]  # fixture의 입력 칸만 선택한다.
        assert actual == [[str(value) for value in values] for values in inputs()]
        assert counts['returns'] == len(actual) and counts['assertions'] == 0
    assert len(rows) == record['new_native_inputs']
    print('현재 Player 독립 PE 전체 SHA/입력/반환 감사 통과:', len(rows))


# 명시 실행에서만 새 관찰을 만들거나 기존 자료를 읽기 전용으로 검사한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 새 관찰과 기존 검증을 선택한다.
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()  # 요청한 실행 방식이다.
    verify() if args.verify else generate()
