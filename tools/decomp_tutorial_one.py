#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early1 단계와 두 판본 Totalmade의 실제 명령을 격리하여 독립 기대값을 보존한다."""
import csv
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

# 저장소와 격리 분석 라이브러리의 경로다. 원본 실행 파일은 읽기만 한다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 분리된 raw Player/TYPE/이벤트/스택/정상 반환 영역이다.
PLAYERS, TYPES, EVENT, STACK, STOP = 0x12000000, 0x14000000, 0x16000000, 0x3001f000, 0x20000000
# 판본별 실제 전역과 복원 getter/초기화 주소다.
PROFILES = {
    'originals': {'binary': 'originals/Netstorm.exe', 'num': 188, 'count': 0x541348, 'base': 0x59ab20, 'stride': 500,
        'tables': [0x5c94d0, 0x5c98d0, 0x5c9cd0, 0x5ca0d0],
        'calls': [0x4c23c0, 0x4c23e0, 0x4c2400, 0x4c2450, 0x4c2480, 0x4c24c0, 0x4c2560, 0x4c2500]},
    'originalCD': {'binary': 'originalCD/NETSTORM.EXE', 'num': 171, 'count': 0x51cbf0, 'base': 0x51c960, 'stride': 468,
        'tables': [0x5109a0, 0x510da0, 0x5111a0, 0x5115a0],
        'calls': [0x40ba30, 0x40ba50, 0x40ba70, 0x40ba90, 0x40bac0, 0x40bb30, 0x40bba0, 0x40bb50]}}


def digest(path):
    """근거 파일의 원래 바이트 SHA256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def seed_table(index, kind):
    """실제 결과를 계산하지 않고 경계·상위비트가 다른 독립 DWORD 입력을 만든다."""
    return ((index + 1) * (0x1020304 + kind * 17)) & 0xffffffff


class Oracle:
    """선택 export 범위의 명령과 선언한 외부 관찰 경계만 실행한다."""
    def __init__(self, edition):
        """PE 사본과 판본별 raw 표·TYPE 저장소를 준비한다."""
        self.edition, self.p = edition, PROFILES[edition]
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        binary = pefile.PE(str(ROOT / self.p['binary']))
        image = binary.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        # 각 입력 영역을 겹치지 않는 별도 사본으로 매핑한다.
        for address, size in [(0, 0x1000), (PLAYERS, 0x1000), (TYPES, 0x20000), (EVENT, 0x1000), (STACK - 0x1f000, 0x20000), (STOP, 0x1000)]:
            self.mu.mem_map(address, size)
        self.ranges = []
        with (ROOT / f'extracted/tutorial-one/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 선택 Ghidra의 실제 포함 범위만 실행을 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = [int(value, 16) for value in part.split('-')]
                    self.ranges.append((low, high + 1))
        self.instructions, self.checking, self.trace, self.assertions = 0, False, [], 0
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def put(self, address, value):
        """명시 입력 DWORD를 little endian으로 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def ret(self, value=0, cleanup=0):
        """관찰 경계의 원래 호출 규약에 따라 복귀한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', self.mu.mem_read(esp, 4))[0]
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, mu, address, size, data):
        """단계 전이·Graph·SP·간헐천 생성·debug 진단은 실제 인자 관찰 경계다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e0620:
            self.assertions += 1
            self.ret()
            return
        if self.tutorial and address == 0x4c2500:
            assert struct.unpack('<I', mu.mem_read(esp + 4, 4))[0] == 4
            self.trace.append('B')
            self.ret(self.built)
            return
        if self.tutorial and address == 0x4c33f0:
            assert mu.reg_read(UC_X86_REG_ECX) == PLAYERS + 0x800
            assert struct.unpack('<I', mu.mem_read(esp + 4, 4))[0] == 0
            self.trace.append('A')
            self.ret(cleanup=4)
            return
        if self.tutorial and address == 0x486360:
            self.trace.append('S')
            self.ret()
            return
        if self.tutorial and address == 0x462bc0:
            sid = struct.unpack('<I', mu.mem_read(esp + 4, 4))[0]
            self.trace.append(f'G{sid}')
            self.ret(self.graphs[sid])
            return
        if self.tutorial and address == 0x40ec50:
            owner, output = struct.unpack('<II', mu.mem_read(esp + 4, 8))
            self.trace.append(f'M{owner}')
            mu.mem_write(output, struct.pack('<f', self.money))
            self.ret(output)
            return
        assert any(low <= address and address + size <= high for low, high in self.ranges), f'미등록 명령 {address:08x}'
        self.instructions += 1

    def write(self, mu, access, address, size, value, data):
        """원본 쓰기는 스택과 선언한 네 실제 표에서만 허용한다."""
        allowed = [(STACK - 0x1f000, STACK + 0x1000)] + [(base, base + 1024) for base in self.p['tables']]
        if self.checking:
            assert any(low <= address and address + size <= high for low, high in allowed), f'미등록 쓰기 {address:08x}+{size}'

    def prepare(self):
        """각 호출 전에 입력만 초기화하여 이전 실행의 변경을 섞지 않는다."""
        self.put(self.p['count'], self.p['num'])
        self.put(self.p['base'], TYPES)
        # 첫 실제 타입 전후와 여러 genus를 교차하는 고정 TYPE 입력이다.
        for index in range(self.p['num']):
            self.put(TYPES + index * self.p['stride'] + 0xec, 1 << (index % 6))
        # 네 실제 배열의256번째 칸도 sentinel로 채워 초기화 밖 쓰기를 검출한다.
        for kind, base in enumerate(self.p['tables']):
            for index in range(256):
                self.put(base + index * 4, seed_table(index, kind))
        self.trace, self.assertions, self.tutorial = [], 0, False

    def call(self, address, args=(), ecx=0, cleanup=0):
        """실제 정상 반환과 보존 레지스터/스택/SEH/x87 제어어를 검사한다."""
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 원본 보존 규약을 각 입력마다 같은 sentinel로 확인한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, ecx)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *[value & 0xffffffff for value in args]))
        self.checking = True
        self.mu.emu_start(address, STOP, count=100000)
        self.checking = False
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + cleanup
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x027f
        assert bytes(self.mu.mem_read(0, 4)) == bytes(4)
        return self.mu.reg_read(UC_X86_REG_EAX)

    def totals(self, operation, first, second=0):
        """허용/초기화/조회/부류 합계의 실제 결과와 네 배열을 반환한다."""
        self.prepare()
        args = () if operation == 3 else ((first, second) if operation == 1 else (first,))
        result = self.call(self.p['calls'][operation], args)
        if operation in (0, 1, 3):
            result = 0  # void 반환의 미정 EAX는 결과로 주장하지 않는다.
        tables = [bytes(self.mu.mem_read(base, 1024)).hex() for base in self.p['tables']]
        return result, self.assertions, tables

    def stage(self, stage, event, built, shown, money, graphs):
        """외부 조회 결과 입력으로 실제 PATCH switch 전체와 호출 순서를 관찰한다."""
        self.prepare()
        self.tutorial, self.built, self.money, self.graphs = True, built, money, graphs
        self.put(0x5ca8e8, stage)
        self.put(0x540c70, 1)
        self.put(0x595344, shown)
        self.put(0x5429ac, 0x730000)
        self.put(0x54271c, 0x123456)
        self.put(0x59534c, PLAYERS)
        self.put(PLAYERS + 0xb4 + 0x7c, 55)
        self.put(EVENT, event)
        candidates = [sid for sid in graphs if sid != 55]
        self.put(0x595094, len(candidates))
        # 실제 공유 간헐천 배열의 입력 순서를 보존한다.
        for index, sid in enumerate(candidates):
            self.put(0x595098 + 4 * index, sid)
        result = self.call(0x4c3a20, (EVENT,), PLAYERS + 0x800, 4)
        return result, ','.join(self.trace) or '-'


def main():
    """독립 입력 TSV와 입력/원본/도구의 바이트 식별 보고서를 저장한다."""
    rows, counts, instructions = [], {}, 0
    # 두 판본에서 실제 Totalmade 전체 몸체를 실행한다.
    for edition in PROFILES:
        oracle, inputs = Oracle(edition), []
        inputs += [(0, value, 0) for value in (0, 1, -1)]
        inputs += [(1, index, value) for index, value in [(0, 1), (70, 0), (187, -1), (254, 1), (255, 0)]]
        inputs += [(operation, index, 0) for operation in (2, 4, 5, 6) for index in (-1, 0, 1, 69, 70, 170, 187, 188, 255)]
        inputs += [(7, value, 0) for value in (0, 1, 4, 7, 32, 63, 0xffffffff)] + [(3, 0, 0)]
        # 기대값은 원본 결과만 기록하고 C++의 분기를 재계산하지 않는다.
        for operation, first, second in inputs:
            result, assertions, tables = oracle.totals(operation, first, second)
            rows.append('\t'.join(map(str, ['Total', edition, operation, first, second, result, assertions, *tables])))
        counts[edition] = len(inputs)
        instructions += oracle.instructions
    oracle = Oracle('originals')
    cases = []
    # A의 명령 단락평가/반환과 B/C의 포함 경계를 독립 교차한다.
    for stage in (0, 64, 65, 66, 67, 71, 0xffffffff):
        for event in (0, 0x730000, 0x123456, 0x8c730000):
            for built in (-1, 0, 1, 7, 8, 18, 19):
                cases.append((stage, event, built, 1, 0.0, {55: 2}))
    # D는 빈 목록/불일치/다중일치/무효Graph의 같은 값도 실제 비교 그대로 관찰한다.
    for graphs in ({55: 0}, {55: 0, 80: 0}, {55: 254, 80: 254, 81: 254}, {55: 2, 80: 1, 81: 2, 82: 2}):
        cases.append((68, 0, 0, 1, 0.0, graphs))
    # SP의 NaN/무한대와 두 임계값의 float32 바로 아래/동일/바로 위를 검사한다.
    money_inputs = [-math.inf, -1.0, 0.0, 199.99998474121094, 200.0, 200.00001525878906, 599.9999389648438, 600.0, 600.0000610351562, math.inf, math.nan]
    for stage in (69, 70):
        for shown in (0, 1, 0x80000000):
            for money in money_inputs:
                cases.append((stage, 0, 0, shown, money, {55: 2}))
    # 각 입력의 실제 호출 순서와 반환값을 보존한다.
    for stage, event, built, shown, money, graphs in cases:
        result, trace = oracle.stage(stage, event, built, shown, money, graphs)
        encoded_graphs = ','.join(f'{sid}:{graph}' for sid, graph in graphs.items())
        rows.append('\t'.join(map(str, ['Stage', stage, event, built, shown, struct.pack('<f', money).hex(), encoded_graphs, result, trace])))
    counts['stage'] = len(cases)
    instructions += oracle.instructions
    fixture = ROOT / 'cpppj/tests/fixtures/tutorial-one-x86.tsv'
    fixture.write_text('# Totalmade 두 판본 실제 몸체 / Early1 PATCH 실제 switch. 단계·Graph·SP·생성은 명시 관찰 경계다.\n' + '\n'.join(rows) + '\n', encoding='utf-8')
    paths = [Path(__file__).resolve(), fixture, ROOT / 'tools/ghidra/tutorial-one-functions.json']
    # 모든 선택 export와 보호 PE의 바이트 식별자를 남긴다.
    for edition, profile in PROFILES.items():
        paths += [ROOT / profile['binary'], ROOT / f'extracted/tutorial-one/{edition}/creation.c', ROOT / f'extracted/tutorial-one/{edition}/functions.tsv']
    report = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'counts': counts, 'unique_inputs': len(rows), 'instructions': instructions,
        'boundaries': ['debug-assertion', 'stage-advance', 'graph-getter', 'SP-getter', 'geyser-spawn', 'tutorial-totalmade-getter'],
        'normal_returns': True, 'sha256': {str(path.relative_to(ROOT)): digest(path) for path in paths}}
    (ROOT / 'cpppj/tutorial-one.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'counts': counts, 'unique_inputs': len(rows), 'normal_returns': True}, ensure_ascii=False))


if __name__ == '__main__':
    main()
