#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""프레임 진행→실제 지정/단계 계산을 세 PE의 격리된 x86 실행으로 대조한다.

글자별 표 생성과 구판 방향 helper도 원본 명령이다. 표시/Unpop/Pop만 기존 호출 기록으로 대체한다.
게임/OS/창은 실행하지 않는다. --verify는 저장된 SHA/행 수/ABI/원본 실행 근거를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct

from decomp_setframe_oracle import (FrameOracle, ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE,
    TYPES, CODES, STOP, KINDS, SIZES, digest)
from decomp_bridgeevent_oracle import STACK, MAX_INSTRUCTIONS
from unicorn import UcError
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 진행 함수와 그 안의 타입/방향 helper다. 실제 지정은 부모 실행기의 진입점을 유지한다.
ENTRIES = {
    'originals': dict(Advance=0x4afc90, AdvanceType=0x49a840),
    'originalCD': dict(Advance=0x4acce0, AdvanceType=0x4abf50, Direction=0x4ae4b0, Side=0x4ae3f0),
}
ENTRIES['original1037'] = ENTRIES['originalCD']
# 연속/비연속 글자 배열이다. number/variant는 구간 진행의 판단 입력이 아님을 함께 검사한다.
LETTERS = ('AAABBJJJ', 'JAJBAPJP')
# signed DWORD 넘침과 한 번 보정 뒤에도 구간 밖에 남는 증분을 포함한다.
DELTAS = (-2147483648, -257, -9, -3, -1, 0, 1, 2, 3, 9, 257, 2147483647)
# 이전 fixture/근거를 수정하지 않고 새 독립 입력을 저장한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/frameadvance-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-frameadvance-evidence.json'
# 보존 레지스터에 서로 다른 비영 값을 넣어 실제 정상 반환 때 같은지 검사한다.
SAVED = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789,
         UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}


class AdvanceOracle(FrameOracle):
    """진행/지정/구간 생성의 원본 몸체만 허용하고 전체 슬롯과 반환/효과를 관찰한다."""
    def __init__(self, edition):
        """현재 PC의 읽기 전용 내보내기를 추가하고 구간 생성 결과의 캐시를 준비한다."""
        super().__init__(edition)
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **ENTRIES[edition]))
        self.tables = {}
        self.abi_checks = 0
        paths = [ROOT / f'extracted/frameadvance/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 불연속 원본 함수 범위만 실행 가능하다.
            for row in csv.DictReader(fp, delimiter='\t'):
                # 함수 몸체의 여러 구간을 하나씩 허용한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def call(self, name, args, this, purge, control, returns_float=False):
        """정상 EIP/스택/보존 레지스터/SEH/x87을 검사하고 정수 EAX를 반환한다."""
        if returns_float:
            raise RuntimeError('프레임 진행에는 float 반환 경로가 없다')
        # 호출자 저장 레지스터와 보존 레지스터를 별도로 입력한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX):
            self.mu.reg_write(register, 0)
        # 원본 함수의 push/pop 누락이 0 초기값으로 가려지지 않게 한다.
        for register, value in SAVED.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.mem_write(0, struct.pack('<I', 0xffffffff))
        words = [STOP] + [value & 0xffffffff for value in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.events = []
        try:
            self.mu.emu_start(self.spec['entries'][name], STOP, count=MAX_INSTRUCTIONS)
        except UcError as error:
            raise RuntimeError(f'{self.edition} {name}: 제한 실행 오류 {error}') from error
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{name}: 정상 반환/스택 불일치')
        # 전체 보존 집합을 검사한다.
        for register, value in SAVED.items():
            if self.mu.reg_read(register) != value:
                raise RuntimeError(f'{name}: 보존 레지스터 불일치 {register}')
        if bytes(self.mu.mem_read(0, 4)) != b'\xff' * 4:
            raise RuntimeError(f'{name}: SEH 체인 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError(f'{name}: x87 제어/TOP 불일치')
        self.abi_checks += 1
        return self.mu.reg_read(UC_X86_REG_EAX)

    def table_blob(self, pattern, control):
        """합성 코드 배열의 구간 표를 실제 원본 Table로 만들고 입력용 바이트로만 재사용한다."""
        key = pattern, control
        if key not in self.tables:
            codes = b''.join(bytes((ord(letter), 80 + index % 2, 11 - index, index & 1))
                             for index, letter in enumerate(LETTERS[pattern]))
            self.table(codes, control)
            base = TYPES + BRIDGE_TYPE * self.type_stride
            self.tables[key] = codes, bytes(self.mu.mem_read(base + 0x12c, 0x84))
        return self.tables[key]

    def run_advance(self, pattern, kind, level, frame, delta, flags, control):
        """판단 결과를 계산하지 않고 원본 진행→지정의 반환/호출 순서/raw 전체를 얻는다."""
        codes, table = self.table_blob(pattern, control)
        self.prepare_object(KINDS[kind], level, frame)
        base = TYPES + BRIDGE_TYPE * self.type_stride
        self.mu.mem_write(CODES, codes)
        self.mu.mem_write(base + 0x124, struct.pack('<I', CODES))
        self.mu.mem_write(base + 0x12c, table)
        result = self.call('Advance', [delta, flags], self.slot(BRIDGE), 8, control)
        return result, ';'.join(self.events) or '-', bytes(self.mu.mem_read(self.slot(BRIDGE), self.stride)).hex()


def inputs():
    """코드 표/종류/저장 단계/현재 프레임/증분/flags의 순서가 고정된 입력 격자다."""
    return list(itertools.product(range(2), range(4), (0, 1, 2, 3, 255), range(8), DELTAS, (0, 0x2000)))


def generate(smoke=False):
    """각 입력을 두 x87 정밀도에서 실행하고 일치한 관찰과 SHA/정상 반환 근거를 저장한다."""
    cases = inputs()
    if smoke:
        cases = cases[::281]
    rows, reports = [], {}
    paths = {ROOT / 'tools/ghidra/frameadvance-functions.json', FIXTURE}
    # 부모에 의존하는 모든 Python 모듈을 SHA 근거에 포함한다.
    for name in ('frameadvance', 'setframe', 'neighbor', 'bridgeevent'):
        paths.add(ROOT / f'tools/decomp_{name}_oracle.py')
    # 세 PE는 각자 원본 명령을 실행한다.
    for edition in SPECS:
        oracle = AdvanceOracle(edition)
        # 증분의 반환값이나 목표 프레임을 Python에서 계산하지 않는다.
        for case in cases:
            first, second = (oracle.run_advance(*case, control) for control in CONTROLS)
            if first != second:
                raise RuntimeError(f'x87 정밀도 관찰 불일치: {edition} {case}')
            rows.append([edition, *case, *first])
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
            entries={key: f'{value:08x}' for key, value in oracle.spec['entries'].items()},
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls),
            assertions=oracle.assertions, instructions=oracle.instructions, abi_checks=oracle.abi_checks,
            replaced_entry_points={f'{key:08x}': value for key, value in oracle.stubs.items()})
        paths.add(ROOT / oracle.spec['binary'])
        paths.update(oracle.exports)
        print(f'{edition}: {len(cases)}개·정상 반환 {oracle.abi_checks}회', flush=True)
    if smoke:
        print('\n'.join('\t'.join(map(str, row)) for row in rows[:5]))
        return
    header = ['# 실제 PE 프레임 진행→지정의 관찰. 표 생성/단계 계산/구판 방향은 실제 명령, 표시/Unpop/Pop만 기록 대체다.',
              '# 판본 코드표 종류 저장단계 현재프레임 signed증분 flags 정수반환 사건 전체슬롯. x87 53/64비트 일치.']
    FIXTURE.write_text('\n'.join(header + ['\t'.join(map(str, row)) for row in rows]) + '\n', encoding='utf-8', newline='\n')
    report = dict(format=1, decompile_host='HJOW-Athlon', decompile_date='2026-10-10',
        cases_per_edition=len(cases), total=len(rows), editions=reports,
        files={str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in sorted(paths)},
        scope=['실제 진행/지정/현재 프레임 단계/글자 구간 생성/구판 방향',
               '표시·Unpop·Pop은 호출 기록 대체', '합성 타입/SHP/코드 표',
               'GUI/일반 사제 Pop/소리 수명/OS 실행은 검증 범위 밖'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'frameadvance 생성 완료: {len(rows)}개', flush=True)


def verify():
    """저장된 입력/도구/PE/내보내기/fixture의 SHA·정확한 입력 격자·ABI와 실제 몸체 실행을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    # 파일 하나라도 변경되면 이전 독립 관찰 근거를 승인하지 않는다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    expected = inputs()
    if len(rows) != len(expected) * 3 or report['total'] != len(rows) or report['cases_per_edition'] != len(expected):
        raise RuntimeError('fixture 행 수 불일치')
    # 중복/누락/입력 순서와 원본 몸체/반환 검사를 판본마다 확인한다.
    for edition, item in report['editions'].items():
        actual = [tuple(map(int, row[1:7])) for row in rows if row[0] == edition]
        if actual != expected or item['assertions'] or item['abi_checks'] != len(expected) * 2 + 4:
            raise RuntimeError(f'입력/ABI/assert 근거 오류: {edition}')
        if any(item['native_calls'].get(name) != len(expected) * 2 for name in ('Advance', 'SetFrame')) or item['native_calls'].get('Table') != 4:
            raise RuntimeError(f'실제 진행/지정/구간 생성 실행 근거 오류: {edition}')
    print(f'frameadvance 검증 통과: {len(rows)}개')


def main():
    """기대값 생성·저장된 감사·일부 입력의 비저장 점검 가운데 요청한 작업을 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
    else:
        generate(args.smoke)


if __name__ == '__main__':
    main()
