#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 세 PE의 벽/게임 시계·중첩 정지 전체 몸체를 격리 x86에서 실행한다.

timeGetTime만 명시 DWORD 입력으로 대체하며 게임/창/OS를 실행하지 않는다.
53/64비트 x87 관찰을 모두 저장한다. C# 기대값은 53비트 모드만 사용한다.
"""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

# 저장소 루트와 격리 Python 의존성 경로.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 파일은 읽기 전용이며 같은 CD 배치라도 각각 독립적으로 로드한다.
BINARIES = {'originals': 'originals/Netstorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
    'original1037': 'original1037/netstorm.exe'}
# Windows 기본 53비트와 대안 64비트 x87 제어 워드.
CONTROLS = (0x027f, 0x037f)
# 격리된 결과/명시 OS 입력 경계/정상 반환/호출 스택 주소.
OUTPUT, TIME, STOP, STACK = 0x10000000, 0x13000000, 0x20000000, 0x3000f000
# 비영 보존 레지스터의 ABI 검사값.
REGISTERS = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x87654321,
    UC_X86_REG_EDI: 0x13572468, UC_X86_REG_EBP: 0x24681357}
# 판본별 전체 몸체 진입점과 숫자 시계 전역/IAT 주소.
SPECS = {
    'originals': dict(Wall=0x460d70, Game=0x460cd0, Pause=0x460df0, Resume=0x460e30,
        IsPaused=0x460de0, origin=0x55b4a0, epoch=0x55b4a4, adjustment=0x55b4ac,
        depth=0x55b4b0, held=0x55b4f0, total=0x55b4f8, iat=0x500394),
    'originalCD': dict(Wall=0x4011b0, Game=0x401130, Pause=0x401230, Resume=0x401270,
        origin=0x50f240, epoch=0x50f244, adjustment=0x50f24c, depth=0x50f250,
        held=0x548428, total=0x548498, iat=0x5bd8f8),
}
SPECS['original1037'] = SPECS['originalCD']
# 독립 관찰 fixture와 SHA/호출 경계 보고서 경로.
FIXTURE = ROOT / 'dotnetpj/tests/Netstorm.Core.Tests/Fixtures/clockkernel-x86.tsv'
REPORT = ROOT / 'dotnetpj/recovery-clockkernel-evidence.json'


def digest(path):
    """기계어/내보내기/도구/자료의 정확한 바이트 SHA를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def plan(edition):
    """정수 눈금/두 종류 감김/epoch 곱셈 감김/반복 중첩 정지 입력을 만든다. 결과는 계산하지 않는다."""
    rng = random.Random(0x107860cd)
    cases = []
    # 경계와 임의 입력을 섞되 같은 완성 입력을 세 판본에 제공한다.
    for index in range(72):
        origin = [0, 0xffffff00, 0xffffffff, 123456789][index % 4] if index < 24 else rng.getrandbits(32)
        epoch = [0, 1, 0xffffffff, 923456789, 4294967, 4294968][index % 6]
        adjustment = [0, 1, 999, 1001, 0xffffffff, 987654][index % 6]
        steps = [0, 1, 9, 13, 14, 999, 1000, 1001, 1009, 1014, 1017, 2001, 2002, 2999,
            3000, 3001, 4001, 9000, 9001, 10001]
        if index % 3 == 1: steps = [value + 0xfffff000 for value in steps]
        if index % 3 == 2: steps = [value + rng.randrange(1000000) for value in steps]
        operations = ['Wall', 'Game', 'Resume', 'Pause', 'Wall', 'Game', 'Pause', 'Game',
            'Resume', 'Game', 'Resume', 'Game', 'Resume', 'Pause', 'Game', 'Resume',
            'Pause', 'Pause', 'Resume', 'Resume']
        actions = [(op, (origin + step) & 0xffffffff) for op, step in zip(operations, steps, strict=True)]
        if 'IsPaused' in SPECS[edition]:
            actions.insert(7, ('IsPaused', actions[6][1]))
            actions.append(('IsPaused', actions[-1][1]))
        cases.append((origin, epoch, adjustment, actions))
    return cases


class ClockOracle:
    """실제 PE와 내보낸 불연속 함수 몸체의 허용 범위를 로드하는 시계 관찰기."""

    def __init__(self, edition):
        """읽기 전용 PE·원본 함수 허용 목록·명시 timeGetTime 응답을 준비한다."""
        self.edition, self.s = edition, SPECS[edition]
        pe = pefile.PE(str(ROOT / BINARIES[edition]))
        raw = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(raw) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, raw)
        # 스택/결과/명시 입력 경계/반환점만 추가로 매핑한다.
        for address, size in ((OUTPUT, 4096), (TIME, 4096), (STOP, 4096), (STACK - 0xf000, 0x10000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/clockkernel/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.names = [], {}
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수 사이의 미검토 코드를 허용하지 않는다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.names[int(row['entry'], 16)] = row['entry']
                # Ghidra가 내보낸 실제 몸체 조각만 실행할 수 있다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.put(self.s['iat'], TIME)
        self.writes = [(STACK - 0xf000, STACK + 4096), (OUTPUT, OUTPUT + 8)]
        self.writes += [(self.s[name], self.s[name] + width) for name, width in (('depth', 4), ('held', 8), ('total', 8))]
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def put(self, address, value):
        """DWORD 입력을 에뮬레이터 메모리에 손실 없이 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value))

    def uint(self, address):
        """현재 DWORD 상태를 실제 메모리에서 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def bits(self, address):
        """double 값을 Python 산술로 바꾸지 않고 실제 반환 비트를 읽는다."""
        return struct.unpack('<Q', self.mu.mem_read(address, 8))[0]

    def write(self, mu, access, address, size, value, user):
        """결과/스택/정지 상태 밖 쓰기를 즉시 거부한다."""
        if not any(low <= address and address + size <= high for low, high in self.writes):
            raise RuntimeError(f'시계 허용 밖 쓰기: {address:08x}+{size}')

    def instruction(self, mu, address, size, user):
        """실제 몸체 진입을 집계하며 timeGetTime만 완성 입력 DWORD로 응답한다."""
        if address == TIME:
            self.time_calls += 1
            esp = mu.reg_read(UC_X86_REG_ESP)
            mu.reg_write(UC_X86_REG_EAX, self.now)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, self.uint(esp))
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'시계 허용 밖 실행: {address:08x}')
        if address in self.names: self.entries[self.names[address]] += 1

    def run(self, case, control):
        """전체 연속 호출을 정상 반환까지 실행하고 ABI/x87/입력 보호/현재 상태를 관찰한다."""
        origin, epoch, adjustment, actions = case
        mu, s = self.mu, self.s
        # 호출열마다 전역 초기값을 독립 준비한다.
        for name, value in (('origin', origin), ('epoch', epoch), ('adjustment', adjustment), ('depth', 0)):
            self.put(s[name], value)
        mu.mem_write(s['held'], bytes(8)); mu.mem_write(s['total'], bytes(8))
        rows = []
        # 연속 호출 사이에는 정지 누계를 유지하고 호출 프레임만 재설정한다.
        for op, self.now in actions:
            self.entries, self.time_calls = collections.Counter(), 0
            # 보존 레지스터와 x87 제어 워드가 원본에서 유지되는지 대조한다.
            for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
            mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_ESP, STACK)
            mu.reg_write(UC_X86_REG_EFLAGS, 2); mu.reg_write(UC_X86_REG_FPCW, control); mu.reg_write(UC_X86_REG_FPSW, 0)
            mu.mem_write(OUTPUT, b'\xa5' * 4096)
            args = (STOP, OUTPUT) if op in ('Wall', 'Game') else (STOP,)
            mu.mem_write(STACK, struct.pack('<' + 'I' * len(args), *args))
            mu.emu_start(s[op], STOP, timeout=1000000, count=10000)
            if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
                raise RuntimeError('시계 정상 반환/스택 오류')
            if any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()): raise RuntimeError('시계 보존 레지스터 오류')
            if mu.reg_read(UC_X86_REG_FPCW) != control or mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
                raise RuntimeError('시계 x87 제어/스택 오류')
            if bytes(mu.mem_read(OUTPUT + 8, 4088)) != b'\xa5' * 4088: raise RuntimeError('시계 결과 보호 바이트 오류')
            if [self.uint(s[name]) for name in ('origin', 'epoch', 'adjustment')] != [origin, epoch, adjustment]:
                raise RuntimeError('시계 읽기 전용 입력 변경')
            value = str(self.bits(OUTPUT)) if op in ('Wall', 'Game') else str(mu.reg_read(UC_X86_REG_EAX)) if op == 'IsPaused' else '-'
            rows.append([str(control), str(origin), str(epoch), str(adjustment), op, str(self.now), value,
                str(self.uint(s['depth'])), str(self.bits(s['held'])), str(self.bits(s['total'])), str(self.time_calls),
                json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')), f'{s[op]:08x}'])
        return rows


def generate():
    """독립 PE 관찰만 fixture로 쓴다. 두 x87 모드의 차이를 숨기거나 제외하지 않는다."""
    rows, differences, files = [], 0, {FIXTURE, ROOT / 'tools/decomp_clockkernel_oracle.py', ROOT / 'tools/ghidra/clockkernel-functions.json'}
    # 세 판본은 독립 메모리에서 같은 완성 입력으로 실행한다.
    for edition in BINARIES:
        oracle = ClockOracle(edition)
        # 각 입력의 두 제어 워드 결과를 모두 기록한다.
        for index, case in enumerate(plan(edition)):
            observations = [oracle.run(case, control) for control in CONTROLS]
            differences += sum(a[6:10] != b[6:10] for a, b in zip(*observations, strict=True))
            # 대안 모드도 실제 관찰 자료로 보존하되 C#의 53비트 계약과 구분한다.
            for result in observations:
                rows.extend([[edition, str(index), *row] for row in result])
        files.update(oracle.exports); files.add(ROOT / BINARIES[edition])
        print(f'{edition}: {len(plan(edition))}개 연속 입력 통과', flush=True)
    FIXTURE.write_text('# edition case control origin epoch adjustment op now valueBits depth heldBits totalBits timeGetTimeCalls entries return\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon', date='2026-10-12', total=len(rows),
        primary=sum(row[2] == str(CONTROLS[0]) for row in rows), controls=list(CONTROLS), mode_differences=differences,
        os_calls=0, boundaries=['timeGetTime'], files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['벽/게임 시계·중첩 Pause/Resume 전체 몸체와 CD DWORD 분해/CRT 곱셈',
            '기본 x87 53비트 결과만 C# 비트 대조; 64비트 관찰은 별도 보존',
            'OS 초기화/초 경계 대기/우선순위/FPS 표본/시계 Capture/원본 실행 제외',
            '정지 깊이는 유효한 비음수 범위; signed 깊이 넘침은 C#에서 거부',
            '정상 반환/호출 규약/x87/허용 실행·쓰기/읽기 전용 입력/결과 보호/SHA 감사']), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """저장 SHA/완성 입력/진입·반환·OS 경계/두 정밀도 차이의 집계를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본/전체 몸체/도구/fixture의 정확한 바이트를 확인한다.
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError('시계 SHA 불일치: ' + name)
    expected_count = 0
    differences = 0
    # 계획과 기록의 순서·모드·호출 인자·실제 진입/반환을 대조한다.
    for edition in BINARIES:
        for index, (origin, epoch, adjustment, actions) in enumerate(plan(edition)):
            modes = []
            # 대안 정밀도의 차이도 실제 저장 관찰로만 집계한다.
            for control in CONTROLS:
                chosen = [row for row in rows if row[:3] == [edition, str(index), str(control)]]
                if len(chosen) != len(actions): raise RuntimeError('시계 연속 호출 누락')
                modes.append(chosen)
                expected_count += len(chosen)
                # 깊이/누계는 재계산하지 않고 출력 폭과 현재 최상위 정상 반환을 검사한다.
                for row, (op, now) in zip(chosen, actions, strict=True):
                    if len(row) != 15 or row[3:8] != [str(origin), str(epoch), str(adjustment), op, str(now)]:
                        raise RuntimeError('시계 입력/행 폭 오류')
                    if row[14] != f'{SPECS[edition][op]:08x}' or json.loads(row[13]).get(row[14], 0) != 1:
                        raise RuntimeError('시계 진입/정상 반환 누락')
                    if int(row[12]) not in (0, 1): raise RuntimeError('시계 외부 경계 호출 수 오류')
                    if any(not 0 <= int(row[i]) <= 0xffffffffffffffff for i in (10, 11)):
                        raise RuntimeError('시계 double 출력 폭 오류')
            differences += sum(a[8:12] != b[8:12] for a, b in zip(*modes, strict=True))
    primary = sum(row[2] == str(CONTROLS[0]) for row in rows)
    if report['total'] != len(rows) or expected_count != len(rows) or report['primary'] != primary or report['mode_differences'] != differences:
        raise RuntimeError('시계 관찰 집계 오류')
    if report['os_calls'] or report['controls'] != list(CONTROLS) or report['boundaries'] != ['timeGetTime']:
        raise RuntimeError('시계 OS/제어 경계 오류')
    print(f'clockkernel 감사 통과: 53비트 {primary}개, 두 모드 정상 반환 {len(rows)}회, 모드 차이 {differences}개, OS 실행 0')


def main():
    """원본 관찰 생성 또는 기존 근거 감사를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()


if __name__ == '__main__': main()
