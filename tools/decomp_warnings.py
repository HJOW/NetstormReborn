#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH의 메시지 표·기한 저장·idle 비교를 실제 기계어로 관찰한다. 창/장치는 실행하지 않는다."""
import csv
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

# 현재 PC의 격리 분석 라이브러리와 보호 원본 기준 경로다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 명령이 참조할 독립 입력/스택/반환/부착 관찰 주소다.
NODES, CLOCK, STACK, STOP = 0x12000000, 0x13000000, 0x3001f000, 0x20000000
NEW, VT, ATTACH = NODES + 0x8000, NODES + 0xa000, STOP + 0x100
# 실제 전역20칸/루트/억제/화면 폭 주소다.
SLOTS, ROOT_GUMP, SUPPRESS, SCREEN_WIDTH = 0x5426b0, 0x563e38, 0x594fa4, 0x5c78f0


def digest(path):
    """원본/내보내기/도구/fixture의 SHA-256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    """double 입력/기대값을 십진수 재파싱 없이 보존한다."""
    return struct.pack('<d', value).hex()


class Oracle:
    """명시한 외부 경계만 관찰하고 나머지는 export의 실제 명령으로 실행한다."""
    def __init__(self):
        """읽기 전용 PE 사본과 제한 실행/쓰기 구역을 준비한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        image = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        # SEH의 FS0, 노드, 벽 시각, 스택과 정상 반환 주소를 겹치지 않게 둔다.
        for address, size in ((0, 0x1000), (NODES, 0x10000), (CLOCK, 0x1000), (STACK - 0x1f000, 0x20000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.mu.mem_write(ROOT_GUMP, struct.pack('<I', NODES))
        self.mu.mem_write(VT + 0x20, struct.pack('<I', ATTACH))
        self.mu.mem_write(CLOCK + 0x100, b'warning\0')
        self.allowed = []
        with (ROOT / 'extracted/warnings/originals/functions.tsv').open(encoding='utf-8') as fp:
            # Ghidra가 확인한 불연속 몸체 밖의 명령 실행을 거부한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.checking, self.events, self.clock_calls, self.instructions = False, [], 0, 0
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def put(self, address, value):
        """분석기의 명시 입력 DWORD를 준비한다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def return_to_caller(self, cleanup=0, value=0):
        """관찰 경계의 실제 호출 규약에 맞춰 복귀한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', self.mu.mem_read(esp, 4))[0]
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, mu, address, size, data):
        """생성/부착/삭제/OS 벽 시각의 경계 인자만 대체 관찰한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e4391:
            assert struct.unpack('<I', mu.mem_read(esp + 4, 4))[0] == 0xc0
            self.return_to_caller(value=NEW)
            return
        if address == 0x4d0600:
            kind, player, parameter, text = struct.unpack('<iiii', mu.mem_read(esp + 4, 16))
            assert mu.reg_read(UC_X86_REG_ECX) == NEW and text == CLOCK + 0x100
            self.put(NEW, VT)
            self.put(NEW + 0x2c, 999)
            self.put(NEW + 0xa4, kind)
            self.put(NEW + 0xa8, player)
            self.put(NEW + 0xb8, parameter)
            mu.mem_write(NEW + 0x3c, struct.pack('<f', self.width))
            self.events.append(f'C,{NEW},{kind},{player},{parameter}')
            self.return_to_caller(16, NEW)
            return
        if address == ATTACH:
            x, y, parent, mode = struct.unpack('<ffII', mu.mem_read(esp + 4, 16))
            assert mu.reg_read(UC_X86_REG_ECX) == NEW and parent == NODES and mode == 0
            self.events.append(f'A,{NEW},{int(x)},{int(y)}')
            self.return_to_caller(16)
            return
        if address == 0x466480:
            self.events.append(f'K,{mu.reg_read(UC_X86_REG_ECX)}')
            self.return_to_caller()
            return
        if address == 0x460d70:
            output = struct.unpack('<I', mu.mem_read(esp + 4, 4))[0]
            mu.mem_write(output, struct.pack('<d', self.wall))
            self.clock_calls += 1
            self.return_to_caller(value=output)
            return
        assert any(low <= address and address + size <= high for low, high in self.allowed), f'미등록 명령 {address:08x}'
        self.instructions += 1

    def write(self, mu, access, address, size, value, data):
        """실제 실행 중 스택/SEH/20칸 표/기한 밖 쓰기를 거부한다."""
        ranges = [(STACK - 0x1f000, STACK + 0x1000), (0, 4), (SLOTS, SLOTS + 80), (NEW + 0xb0, NEW + 0xb8)]
        if self.checking:
            assert any(low <= address and address + size <= high for low, high in ranges), f'미등록 쓰기 {address:08x}+{size}'

    def call(self, address, ecx=0, args=()):
        """x87 double 정밀도와 보존 레지스터·스택·SEH 복구를 고정한다."""
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 모든 정상 반환에서 원본 규약의 보존 레지스터를 확인한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, ecx)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        self.put(0, 0)
        self.events, self.clock_calls, self.checking = [], 0, True
        self.mu.emu_start(address, STOP, count=100000)
        self.checking = False
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        expected = STACK + (8 if address == 0x418070 else 4)
        assert self.mu.reg_read(UC_X86_REG_ESP) == expected
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert bytes(self.mu.mem_read(0, 4)) == bytes(4)
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x027f

    def board(self, kind, player, enabled, parameter, suppressed, width, serials, nodes):
        """현재 직계 자식/serial 입력으로 실제 표 분배와 조회/ftol을 실행한다."""
        self.width = width
        self.mu.mem_write(NODES, bytes(0x9000))
        self.put(SUPPRESS, suppressed)
        self.put(SCREEN_WIDTH, 1024)
        self.mu.mem_write(SLOTS, struct.pack('<20I', *serials))
        self.put(NODES + 0x18, NODES + 0x100 if nodes else 0)
        # 노드 주소는 입력 순서로 정하며 표시/수명 효과는 별도 관찰 경계다.
        for index, (serial, old_kind, old_player, old_parameter, dead) in enumerate(nodes):
            token = NODES + (index + 1) * 0x100
            self.put(token + 0xc, token + 0x100 if index + 1 < len(nodes) else 0)
            self.put(token + 4, 0x10000 if dead else 1)
            self.put(token + 0x2c, serial)
            self.put(token + 0xa4, old_kind)
            self.put(token + 0xa8, old_player)
            self.put(token + 0xb8, old_parameter)
        self.call(0x4d3090, args=(kind, player, enabled, parameter, CLOCK + 0x100))
        output = ','.join(map(str, struct.unpack('<20I', self.mu.mem_read(SLOTS, 80))))
        return output, ';'.join(self.events) or '-'


def board_inputs():
    """원본 판단을 계산하지 않고 빈 칸/stale/dead/중복 입력만 구성한다."""
    scenarios = []
    scenarios.append(([0xffffffff] + [0] * 19, []))
    scenarios.append(([0] * 20, []))
    # 유효/stale/dead와 일치 위치가 다른 독립 입력을 구성한다.
    for mode in range(9):
        nodes = [(100 + i, 5 if mode in (3, 4, 5, 6) else 17, 3 if mode in (3, 4, 5, 6) else 0, 7 if mode in (3, 4, 5, 6) else 0, int(mode == 2)) for i in range(20)]
        serials = [100 + i for i in range(20)]
        if mode == 1:
            nodes = []
        if mode == 3:
            serials[0] = 0
        if mode == 4:
            serials[0] = 888
        if mode == 5:
            serials[0:2] = [888, 0]
        if mode == 6:
            nodes[0] = (100, 5, 3, 7, 1)
        if mode == 7:
            serials[::2] = [888] * 10
        if mode == 8:
            nodes = nodes[:2]
        scenarios.append((serials, nodes))
    commands = [(17, 0, 1, 0), (17, 0, 0, 0), (5, 3, 1, 7), (5, 3, 0, 7), (5, 3, 1, 99), (11, 0, 1, 0), (16, 0, 1, 0)]
    # 명령과 억제를 독립 교차하며 같은 Warning도 다른 칸을 사용하는 입력을 포함한다.
    for index, (serials, nodes) in enumerate(scenarios):
        for command in commands:
            for suppressed in (0, 1):
                yield (*command, suppressed, (123.75, 2048.0, 0.0)[index % 3], serials, nodes)


def main():
    """실제 표/기한/idle 결과를 독립 C++ 대조용 TSV에 보존한다."""
    oracle = Oracle()
    rows, counts = [], {}
    # 실제 정상 입력을 생성하며 기대 표/호출 순서는 원본 명령에서만 얻는다.
    for case in board_inputs():
        kind, player, enabled, parameter, suppressed, width, serials, nodes = case
        output, events = oracle.board(*case)
        rows.append('\t'.join(map(str, ['Board', kind, player, enabled, parameter, suppressed, width, ','.join(map(str, serials)), ';'.join(','.join(map(str, node)) for node in nodes) or '-', output, events])))
    counts['Board'] = len(rows)
    clocks = [0.0, 0.0000000001, 0.1, 1.234567890123, 9.999999999, 10.0, 10.000000001, 32768.000000001, 65535.9999999, 1000000.1234567]
    # 실제 double 저장을 관찰해 디컴파일 C의 잘못된 float 시각 변환을 검출한다.
    for clock in clocks:
        oracle.wall = clock
        oracle.call(0x418070, NEW + 0xb0, (0x41200000,))
        rows.append(f'Deadline\t{bits(clock)}\t{bytes(oracle.mu.mem_read(NEW + 0xb0, 8)).hex()}')
    counts['Deadline'] = len(clocks)
    deadlines = [0.0, -0.0, -1.0, 10.0, 10.000000001, 1000000.1234567, math.inf, -math.inf, math.nan]
    # 0・경계 양쪽・NaN/무한대에서 clock 조회 여부와 Kill 호출을 함께 관찰한다.
    for deadline in deadlines:
        for clock in clocks:
            oracle.wall = clock
            oracle.mu.mem_write(NEW + 0xb0, struct.pack('<d', deadline))
            oracle.call(0x4d3030, NEW)
            rows.append(f'Expired\t{bits(deadline)}\t{bits(clock)}\t{int(bool(oracle.events))}\t{oracle.clock_calls}')
    counts['Expired'] = len(deadlines) * len(clocks)
    fixture = ROOT / 'cpppj/tests/fixtures/warnings-x86.tsv'
    fixture.write_text('# PATCH 메시지 표/기한/idle. 명시 생성·부착·삭제·벽 시각 경계의 관찰이며 픽셀/장치는 별도다.\n' + '\n'.join(rows) + '\n', encoding='utf-8')
    report = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'counts': counts, 'unique_inputs': len(rows), 'native_instructions': oracle.instructions,
        'x87_control_word': '027f', 'boundaries': ['new', 'message-constructor', 'attach', 'kill', 'wall-clock'],
        'sha256': {str(path.relative_to(ROOT)): digest(path) for path in [ROOT / 'originals/Netstorm.exe', ROOT / 'extracted/warnings/originals/creation.c', ROOT / 'extracted/warnings/originals/functions.tsv', fixture, Path(__file__).resolve()]}}
    (ROOT / 'cpppj/warnings.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('Warning native', counts, 'normal returns/registers/stack/SEH/CW OK')


if __name__ == '__main__':
    main()
