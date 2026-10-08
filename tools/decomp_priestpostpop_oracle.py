#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 postPop을 실제 PE에서 carrier 진입까지만 실행하여 독립 기대값을 얻는다.

목록/최대 HP/소유자 초기화는 실제 명령이다. Regular 검색·할당·생성만 입력/기록 대체한다.
carrier 이후 몸체와 원본 게임/OS는 실행하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path

from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, LISTS, TARGET, STACK, STACK_BASE, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 판본마다 사용하는 몸체·실행 중단·대체 호출·최대 HP/목록/소유자 전역이다.
POST = {
    'originals': dict(entry=0x4950f0, carrier=0x427720, find=0x4afb40, new=0x4e4391, regular=0x496e80,
        max_hp=0x4adcb0, type_hp=0x49fce0, vtable=0x50f210, list=0x5954c4, owners=0x595428,
        priest_type=0x5412d0, quarter=0x5ca8f0, scale=0x50f20c),
    'originalCD': dict(entry=0x40c110, carrier=0x4e6360, find=0x4ac890, new=0x4f1650, regular=0x48efd0,
        max_hp=0x4aed30, type_hp=0x444720, vtable=0x5003e0, list=0x549188, owners=0x5119a0,
        priest_type=0x51cbbc, quarter=0x511c90, scale=0x500388),
}
POST['original1037'] = POST['originalCD']
# 원본 실제 타입·이벤트, 두 x87 정밀도, 고정 입력 시드, 대체 할당 주소와 출력 파일이다.
PRIEST_TYPE, EVENT = 158, 0x25a
CONTROLS = (0x027f, 0x037f)
SEED, MEMORY = 0x4950f0, LISTS + 0x1000
FIXTURE = ROOT / 'cpppj/tests/fixtures/priestpostpop-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-priestpostpop-evidence.json'


def inputs():
    """분기 격자와 큰/음수 HP 경계, 순서·중복 잡음 입력을 만든다. 기대값은 계산하지 않는다."""
    rows = []
    lists = ((0, 0, [50, 60, 70, 80]), (4, 0, [50, 60, 70, 80]),
             (4, 2, [60, 50, 50, 80]), (4, 4, [60, 70, 80, 90]))
    # extra 차단/flags 초기 배치/이벤트 중복/할당 실패/목록 포화를 각각 교차한다.
    for extra, flags, process, storage, quarter in itertools.product(
            (0, 1, 8, 9, 0xf6), (0, 1, 0x40, 0x41, 0x800, 0x801),
            ((1, 1), (0, 0), (0, 1)), lists, (0, 1)):
        index = len(rows)
        rows.append(dict(extra=extra, flags=flags, found=process[0], allocated=process[1],
            capacity=storage[0], count=storage[1], entries=storage[2], quarter=quarter, hp=200,
            state=(0, 2, 4, 6, 8)[index % 5], owner=index % 9, word=0xab80 | (index % 9)))
    # FILD를 float 변환으로 바꾸면 달라질 수 있는 signed 정수 범위도 직접 관찰한다.
    for hp, quarter, owner in itertools.product((6, 7, 23, 24, 25, 200, 1001, 16777217,
            2147483647, -7, -25, -16777217, -2147483647), (0, 1), (0, 1, 8, 9, 127, 255)):
        rows.append(dict(extra=0, flags=1, found=0, allocated=1, capacity=4, count=1,
            entries=[60, 50, 70, 80], quarter=quarter, hp=hp, state=0, owner=owner, word=0xff80 | (owner & 0x7f)))
    rng = random.Random(SEED)
    # 미사용 저장소의 같은 SID와 현재/원래 소유자가 다른 입력을 추가한다.
    for _ in range(320):
        capacity = rng.choice((0, 1, 2, 4))
        rows.append(dict(extra=rng.choice((0, 2, 4, 8, 9, 255)), flags=rng.choice((0, 1, 0x40, 0x41, 0x800, 0xffffffff)),
            found=rng.randrange(2), allocated=rng.randrange(2), capacity=capacity, count=rng.randrange(capacity + 1),
            entries=[rng.choice((50, 60, 70, 0)) for _ in range(4)], quarter=rng.randrange(2),
            hp=rng.choice((23, 200, 1001, 16777217, 2147483647)), state=rng.choice((0, 2, 4, 8)),
            owner=rng.choice((0, 1, 8, 9, 127, 255)), word=rng.choice((0, 1, 0x81, 0xff08, 0xab09, 0xffff))))
    return rows


class PriestPostPopOracle(OwnerOracle):
    """기존 실행기의 메모리/쓰기 감사를 재사용하고 prefix 경계를 명시한다."""
    def __init__(self, edition):
        """읽기 전용 내보내기만 추가하고 SEH의 FS:[0] 메모리를 격리한다."""
        super().__init__(edition)
        self.p = POST[edition]
        self.mu.mem_map(0, 0x1000)
        self.exports.extend(ROOT / f'extracted/priestpostpop/{edition}/{name}' for name in ('creation.c', 'functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # Ghidra 몸체의 불연속 범위 밖 실행은 계속 거부한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.events, self.stub_calls = [], collections.Counter()
        self.boundaries = 0
        self.scale_bits = bytes(self.mu.mem_read(self.p['scale'], 4)).hex()
        if self.scale_bits != '00004842': raise RuntimeError('원본 회복 배율이 50.0이 아님')

    def on_instruction(self, mu, address, size, data):
        """검색/할당/생성 인자를 기록하고 carrier 진입에서 실행을 중단한다."""
        p = self.p
        if address == p['carrier']:
            esp = mu.reg_read(UC_X86_REG_ESP)
            if mu.reg_read(UC_X86_REG_ECX) != self.slot(TARGET) or self.u32(esp + 4) != self.case['flags']:
                raise RuntimeError('carrier 경계 this/flags 오류')
            self.boundaries += 1
            mu.emu_stop()
            return
        if address in (p['find'], p['new'], p['regular']):
            esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
            ret, first = struct.unpack('<II', mu.mem_read(esp, 8))
            self.stub_calls[f'{address:08x}'] += 1
            if address == p['find']:
                if ecx != self.slot(TARGET) or first != EVENT: raise RuntimeError('회복 검색 인자 오류')
                self.events.append(f'F:{TARGET}:{first}:{self.u32(p["list"] + 8)}')
                result, purge = (MEMORY if self.case['found'] else 0), 4
            elif address == p['new']:
                if first != 0x28: raise RuntimeError('Regular 확보 크기 오류')
                self.events.append(f'A:{first}')
                result, purge = (MEMORY if self.case['allocated'] else 0), 0
            else:
                parent, payload = struct.unpack('<II', mu.mem_read(esp + 8, 8))
                if ecx != MEMORY or first != EVENT or parent != TARGET: raise RuntimeError('회복 생성 인자 오류')
                self.events.append(f'R:{parent}:{first}:{payload}')
                result, purge = MEMORY, 12
            mu.reg_write(UC_X86_REG_EAX, result)
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        super().on_instruction(mu, address, size, data)

    def u32(self, address):
        """격리 메모리의 DWORD만 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def run_prefix(self, case, control):
        """실제 prefix를 실행하고 목록 저장소·전 소유자 칸·payload·슬롯 전체를 관찰한다."""
        self.case, self.events = case, []
        p, slot = self.p, self.slot(TARGET)
        raw = bytearray(self.stride)
        struct.pack_into('<I', raw, 0, p['vtable'])
        raw[10], raw[11], raw[self.o['owner']], raw[self.o['extra']] = PRIEST_TYPE, case['state'], case['owner'], case['extra']
        struct.pack_into('<H', raw, 12, case['word'])
        struct.pack_into('<ff', raw, 14, 20.75, 21.9)
        struct.pack_into('<H' if self.stride == 36 else '<I', raw, 26, 42)
        self.mu.mem_write(slot, bytes(raw))
        base = TYPES + PRIEST_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride))
        self.mu.mem_write(base, struct.pack('<i', case['hp']))
        self.mu.mem_write(base + 0xe8, struct.pack('<II', 0x00069012, 0x00210000))
        self.mu.mem_write(p['priest_type'], struct.pack('<I', PRIEST_TYPE))
        self.mu.mem_write(p['quarter'], struct.pack('<I', case['quarter']))
        if self.edition == 'originals': self.mu.mem_write(0x5e4794, bytes(4))
        else: self.mu.mem_write(0x546778, bytes(4))
        self.mu.mem_write(p['list'], struct.pack('<III', LISTS, case['capacity'], case['count']))
        self.mu.mem_write(LISTS, struct.pack('<4I', *case['entries']))
        self.mu.mem_write(p['owners'], struct.pack('<9I', *[0xabc000 + i for i in range(9)]))
        self.write_ranges = [(0, 4), (LISTS, LISTS + 16), (p['list'] + 8, p['list'] + 12), (p['owners'], p['owners'] + 36)]
        # SEH를 보유한 중간 경계이므로 정상 반환/FS 복구를 주장하지 않는다. x87 스택은 비어야 한다.
        self.mu.mem_write(0, struct.pack('<I', 0xffffffff))
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, slot)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case['flags']))
        self.mu.emu_start(p['entry'], STOP, count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP) != p['carrier'] or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('prefix 중단/x87 스택 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control: raise RuntimeError('x87 제어어 변경')
        storage = struct.unpack('<4I', self.mu.mem_read(LISTS, 16))
        owners = struct.unpack('<9I', self.mu.mem_read(p['owners'], 36))
        return [f'{self.u32(p["list"] + 8)}:' + ','.join(map(str, storage)), ','.join(map(str, owners)),
            ';'.join(self.events) or '-', bytes(self.mu.mem_read(slot, self.stride)).hex()]


def generate(smoke=False):
    """세 PE/두 정밀도의 독립 관찰과 명시적 대체/중단 경계, SHA를 저장한다."""
    cases, rows, editions = inputs(), [], {}
    if smoke: cases = cases[::71]
    paths = {Path(__file__), ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/priestpostpop-functions.json', FIXTURE}
    for edition in SPECS:
        oracle = PriestPostPopOracle(edition)
        # 매 정밀도에서 같은 입력을 각각 실제 PE로 실행한다.
        for control, case in itertools.product(CONTROLS, cases):
            rows.append(['Prefix', edition, control, *[case[key] for key in
                ('flags', 'state', 'extra', 'owner', 'word', 'hp', 'quarter', 'found', 'allocated', 'capacity', 'count')],
                ','.join(map(str, case['entries'])), *oracle.run_prefix(case, control)])
        editions[edition] = dict(binary=oracle.spec['binary'], cases=len(cases) * len(CONTROLS),
            entry=f'{oracle.p["entry"]:08x}', carrier=f'{oracle.p["carrier"]:08x}', scale_bits=oracle.scale_bits,
            boundaries=oracle.boundaries, native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls),
            assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: prefix {editions[edition]["cases"]}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 PE carrier 이전 prefix. Regular 검색/할당/생성만 대체.\n'
        '# Prefix 판본 x87 flags state extra owner word HP quarter found allocated capacity count entries 목록결과 소유자칸 사건 슬롯\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, primary_target='10.78', total=len(rows), controls=list(CONTROLS), editions=editions,
        stubbed=['find Regular', 'new(0x28)', 'Regular constructor'], stopped_before='carrier postPop', os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['synthetic priest raw slot/type HP, list storage and owner state',
            'nonzero effective maximum HP; CD Pentium FDIV workaround disabled',
            'Regular attachment verified separately in C++ with real ProcessHost',
            'no carrier/falling/path continuation, priest regen handler or GUI world'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """각 입력/출력 SHA, 중단/대체 횟수와 실제 진입 수를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != report['total'] or report['os_calls'] or report['stopped_before'] != 'carrier postPop':
        raise RuntimeError('fixture 수/실행 경계 오류')
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[1] == edition]
        if len(selected) != item['cases'] or item['boundaries'] != len(selected) or item['assertions']:
            raise RuntimeError(f'prefix 증거 오류: {edition}')
        if item['native_calls'][item['entry']] != len(selected): raise RuntimeError('몸체 진입 수 불일치')
        for name, marker in (('find', 'F:'), ('new', 'A:'), ('regular', 'R:')):
            count = sum(row[-2].count(marker) for row in selected)
            if item['stub_calls'].get(f'{POST[edition][name]:08x}', 0) != count: raise RuntimeError('대체 관찰 수 불일치')
        scheduled = sum('R:' in row[-2] for row in selected)
        if item['native_calls'].get(f'{POST[edition]["max_hp"]:08x}', 0) != scheduled:
            raise RuntimeError('최대 HP 실제 실행 수 불일치')
        if item['native_calls'].get(f'{POST[edition]["type_hp"]:08x}', 0) != 2 * scheduled:
            raise RuntimeError('타입 HP 실제 실행 수 불일치')
    print(f'priestpostpop 검증 통과: {len(rows)}개')


def main():
    """전체 생성·저장된 증거 감사·무저장 소규모 대조 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
