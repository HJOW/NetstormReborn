#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 신전 Owner와 공통 Owner를 PE 명령으로 실행하고 현재 Player·목록·도장 요청을 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, PLAYERS, POOL, TARGET, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

SPEC = {  # 실제 원본 신전 가상 표와 하위 함수/전역 주소다.
    'originals': dict(entry=0x4dc4e0, table=0x519860, island=0x46f530, exists=0x46ee30,
                      set_island=0x46ef10, region=0x455380, refresh=0x47d890,
                      list=0x55a70c, revision=0x5949d4),
    'originalCD': dict(entry=0x44f2a0, table=0x501de0, island=0x4bdc50, exists=0x4bd4c0,
                       set_island=0x4bd670, region=0x47b3c0, refresh=0x4773a0,
                       list=0x5670e0, revision=None),
}
FIXTURE = ROOT / 'cpppj/tests/fixtures/vortex-owner-x86.tsv'  # 이전 기대값을 덮지 않는 새 독립 자료다.
REPORT = ROOT / 'cpppj/vortex-owner.json'  # 실행 당시 PE/함수/도구/출력의 전체 SHA다.
OUTPOSTS = 0x1131d000  # 에뮬레이터 안의 별도 현재 목록이며 실제 호스트 포인터가 아니다.


def inputs():
    """결과를 계산하지 않고 이전/새owner·상태·기존 신전·효과 뒤 변경 입력을 교차한다."""
    return list(itertools.product((0, 1, 8), (0, 1, 8), (0, 4), (0, 1, 8, 9), (0, 50, 333), range(4)))


class VortexOracle(OwnerOracle):
    """공통 Owner는 실제 명령을 재사용하고 섬/도장/생산만 명시 외부 경계로 관찰한다."""

    def __init__(self, edition):
        """원래 Owner PE 매핑에 신전의 실제 함수 범위만 추가한다."""
        super().__init__(edition)
        self.common_entry = self.spec['entry']  # 실행하는 실제 공통 Owner 진입점이다.
        self.spec = dict(self.spec)
        self.p = SPEC[edition]
        self.spec['entry'] = self.p['entry']
        self.exports.extend(ROOT / f'extracted/vortex-owner/{edition}/{name}' for name in ('creation.c', 'functions.tsv'))
        # 새 신전 함수의 실제 불연속 몸체를 기존 공통 함수 허용 범위에 추가한다.
        for row in csv.DictReader(self.exports[-1].open(encoding='utf-8'), delimiter='\t'):
            self.entries.add(int(row['entry'], 16))
            # Ghidra가 내보낸 현재 명령 범위만 실행한다.
            for part in row['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.values = None  # 이번 요청의 명시 입력6개다.
        self.started = False  # 최상위 진입의 입력 주입 여부다.
        self.events = []  # 실제 하위 요청의 인자 순서다.
        self.paints = 0  # 첫 도장 효과에서만 가변 입력을 변경한다.
        self.returns = 0  # 정상 최상위 반환 수이며 하위 호출을 새 입력으로 세지 않는다.

    def read(self, address, width=4):
        """현재 가상 메모리의 little endian 값을 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def on_instruction(self, mu, address, size, data):
        """원본 몸체를 대체하지 않고 외부 하위의 반환·입력 변경만 공급한다."""
        old, new, _, _, initial, mutation = self.values
        patch = self.edition == 'originals'  # 해당 PE의 실제 저장 폭과 주소다.
        field = 0x7c if patch else 0x74  # 초기 신전/복사/새 신전/복사의 첫DWORD 위치다.
        slot = self.slot(TARGET)  # 이번 실제 신전의 가상 raw 주소다.
        if address == self.p['entry'] and not self.started:
            self.started = True
            mu.mem_write(slot, struct.pack('<I', self.p['table']))
            mu.mem_write(slot + 14, struct.pack('<ff', 40.5, 50.25))
            # Player의 작업장 머리는 기존 공통 Owner 입력을 유지하며 신전 네 필드만 준비한다.
            for owner in range(9):
                mu.mem_write(PLAYERS + owner * self.player_stride + field,
                             struct.pack('<4I', initial, 0x11223344, 0x55667788, 0x99aabbcc))
            # 패치의 현재 추가 목록에는 같은/다른 소유자 및 다음에 활성화할 세 번째 항목을 넣는다.
            for index, owner in enumerate((old, new, 8)):
                mu.mem_write(self.slot(60 + index) + self.o['owner'], bytes([owner]))
                mu.mem_write(self.slot(60 + index) + 14, struct.pack('<ff', 70.5 + index, 80.25 + index))
            mu.mem_write(OUTPOSTS, struct.pack('<3I', 60, 61, 62))
            mu.mem_write(self.p['list'], struct.pack('<III', OUTPOSTS, 3, 2))
            if patch:
                mu.mem_write(self.p['revision'], struct.pack('<I', 37))
            self.write_ranges += [(PLAYERS, PLAYERS + 9 * self.player_stride), (slot, slot + self.stride)]
            if patch:
                self.write_ranges.append((self.p['revision'], self.p['revision'] + 4))
        if address == self.common_entry:
            self.events.append('B:' + str(new))
        callbacks = (self.p['island'], self.p['exists'], self.p['set_island'], self.p['region'], self.p['refresh'])
        if address not in callbacks:
            super().on_instruction(mu, address, size, data)
            return
        esp = mu.reg_read(UC_X86_REG_ESP)  # 명시 외부 경계의 원본 인자 스택이다.
        args = [self.read(esp + 4 + index * 4) for index in range(3)]
        purge, result = 0, 0  # 해당 실제 호출 규약의 retN과 반환값이다.
        self.instructions += 1
        if address == self.p['island']:
            result, purge = 3, 8
        elif address == self.p['exists']:
            result, purge = 1, 4
        elif address == self.p['set_island']:
            self.events.append(f'I:{args[0]}:{args[1]}')
            purge = 8
            if mutation == 1:
                mu.mem_write(slot + self.o['owner'], bytes([8]))
                mu.mem_write(slot + 14, struct.pack('<ff', 42.75, 60.5))
        elif address == self.p['region']:
            self.events.append(f'P:{args[0]}:{args[1]}')
            if self.paints == 0:
                if mutation in (1, 3):
                    mu.mem_write(self.p['list'] + 8, struct.pack('<I', 3))
                if mutation == 2:
                    mu.mem_write(self.p['list'] + 8, struct.pack('<I', 0))
                    mu.mem_write(slot + self.o['extra'], bytes([1]))
                if mutation == 3:
                    mu.mem_write(slot + self.o['owner'], bytes([1]))
            self.paints += 1
        else:
            self.events.append('R')
            if patch and mutation == 3:
                mu.mem_write(self.p['revision'], struct.pack('<I', 0xffffffff))
        mu.reg_write(UC_X86_REG_EAX, result)
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, self.read(esp))

    def observe(self, values):
        """기존 공통 Owner 입력과 새 신전 상태를 넣고 실제 정상 반환 결과를 수집한다."""
        self.values = values
        self.started = False
        self.events = []
        self.paints = 0
        old, new, state, extra, _, mutation = values
        case = dict(fort=0, battle=1, challenge=0, genus=0x200, state=state, extra=extra, owner=old, player=new,
                    lists=[([1, 2, 3, 4, 5, 6], 0) for _ in range(9)], capacity=[6] * 9)  # 기존 공통 Owner의 독립 입력이다.
        _, lists, _ = self.run(case)
        self.returns += 1
        field = 0x7c if self.edition == 'originals' else 0x74  # 판본별 신전DWORD 위치다.
        temples = ','.join(str(self.read(PLAYERS + owner * self.player_stride + field + index * 4))
                           for owner in range(9) for index in range(4))  # 실제9 Player의 네 신전 필드 출력이다.
        revision = self.read(self.p['revision']) if self.edition == 'originals' else 37  # CD는 이 전역을 쓰지 않는다.
        return [';'.join(self.events), bytes(self.mu.mem_read(self.slot(TARGET), self.stride)).hex(), temples, lists, revision]


def generate():
    """새 고유 입력을 두 PE에서 실행해 별도 fixture와 전체 원문 SHA를 기록한다."""
    rows, editions = [], {}  # 실제 입력/출력과 판본별 정상 반환 통계다.
    files = {Path(__file__), ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/vortex-owner-functions.json'}
    # 원본 판본마다 같은 고유 입력 격자를 실행한다.
    for edition in SPEC:
        oracle = VortexOracle(edition)
        # 기대값 계산 없이 원본 함수의 결과를 수집한다.
        for values in inputs():
            rows.append([edition, ','.join(map(str, values)), *oracle.observe(values)])
        assert oracle.assertions == 0
        editions[edition] = dict(unique_inputs=len(inputs()), normal_returns=oracle.returns,
                                 assertions=oracle.assertions, instructions=oracle.instructions)
        files.update(oracle.exports)
        files.add(ROOT / oracle.spec['binary'])
    FIXTURE.write_text('# 실제 신전 Owner·공통 Owner PE 명령. 섬/도장/생산은 명시 외부 경계. 게임/OS 진입 없음.\n' +
                       '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    files.add(FIXTURE)
    record = dict(schema=1, host='HJOW-Athlon', last_decompile_date='2026-10-15', total=len(rows), editions=editions,
                  original_owner_recount=0, missing_island_assertion_pending=True,
                  files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(files)})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('실제 신전/공통 Owner 독립 고유 입력:', len(rows))


def verify():
    """기존 결과를 재계산하지 않고 전체 SHA·고유 입력·정상 반환을 검사한다."""
    record = json.loads(REPORT.read_text(encoding='utf-8'))  # 실행 당시의 고정 원문이다.
    # 실제 PE·함수·도구·출력 전체 파일의 원문을 확인한다.
    for name, expected in record['files'].items():
        assert digest(ROOT / name) == expected, name
    rows = [row for row in csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t') if row and not row[0].startswith('#')]
    # 판본별 입력 순서와 정상 반환 수를 실제 기록에 대조한다.
    for edition, item in record['editions'].items():
        expected = [','.join(map(str, values)) for values in inputs()]  # 기대출력과 독립된 입력 문자열이다.
        assert [row[1] for row in rows if row[0] == edition] == expected
        assert item['normal_returns'] == len(expected) and item['assertions'] == 0
    assert len(rows) == record['total']
    print('신전 Owner 전체 SHA/입력/반환 감사 통과:', len(rows))


# 명시 CLI에서만 새 관찰 또는 읽기 감사를 실행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 실행할 생성/감사 방식이다.
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()  # 호출자의 모드 요청이다.
    verify() if args.verify else generate()
