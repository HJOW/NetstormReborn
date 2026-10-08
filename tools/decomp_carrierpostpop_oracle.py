#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Carrier/Damageable postPop 전체 몸체를 세 실제 PE에서 실행하고 외부 세 호출만 대체한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, TARGET, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 실제 두 판본의 몸체, carrier 가상 +0xcc, 외부 지면/공통 후처리 및 전역이다.
POST = {
    'originals': dict(carrier=0x427720, damageable=0x44bf80, check=0x426fc0, ground=0x44bd20, base=0x4b0d30,
        boss=0x540bc4, loading=0x5c89b8, geyser=0x541240, genus=0x4ad080, flags=0x4ac1e0, vtable=0x50f210),
    'originalCD': dict(carrier=0x4e6360, damageable=0x4621c0, check=0x4e50d0, ground=0x461f00, base=0x4ae180,
        boss=0x540a2c, loading=0x5178d4, geyser=0x51cb2c, genus=0x4acdb0, flags=0x4abac0, vtable=0x5003e0),
}
POST['original1037'] = POST['originalCD']
# 새 독립 관찰/근거 출력 경로다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/carrierpostpop-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-carrierpostpop-evidence.json'


def inputs():
    """조건의 입력 격자와 가상/지면 효과 뒤 raw 재조회 입력을 만든다. 기대 분기는 계산하지 않는다."""
    rows = []
    types = ((158, 122), (158, 158), (122, 122), (82, 122))
    # carrier에서는 boss/최초 flag와 damageable 로딩 조건을 모두 교차한다.
    for flags, boss, loading, pair, extra, f1 in itertools.product(
            (0, 1, 0x40, 0x41, 0x800, 0x801, 0x2000000, 0xffffffff), (0, 1), (0, 1, 3),
            types, (0, 1, 8, 9, 0xf6), (0x69012, 0x69412)):
        rows.append(dict(kind='Carrier', flags=flags, boss=boss, loading=loading, type=pair[0], geyser=pair[1], extra=extra, f1=f1))
    # 직접 Damageable 호출은 boss와 무관하다. 각 남은 조건을 별도로 실행한다.
    for flags, loading, pair, extra, f1 in itertools.product((0, 1, 0x40, 0x801), (0, 1, 3),
            types, (0, 1, 8, 9, 0xf6), (0x69012, 0x69412)):
        rows.append(dict(kind='Damageable', flags=flags, boss=0, loading=loading, type=pair[0], geyser=pair[1], extra=extra, f1=f1))
    # 가상 확인이 바꾼 타입/extra/좌표/소유자, 지면 효과가 바꾼 소유자를 다음 호출이 읽어야 한다.
    for flags, extra, after_type, after_extra in itertools.product((0, 0x40), (0, 1, 8, 9), (158, 122, 82), (0, 9)):
        rows.append(dict(kind='Carrier', flags=flags, boss=0, loading=1, type=158, geyser=122, extra=extra, f1=0x69412,
            after_type=after_type, after_extra=after_extra, after_owner=6, ground_owner=3))
    # raw 타입 표/필드 입력을 완성한다. NaN/음의 0도 부동소수 계산 없이 인자 비트를 전달한다.
    for index, case in enumerate(rows):
        case.update(state=(0, 2, 4)[index % 3], owner=(0, 1, 8, 9, 127, 255)[index % 6],
            x=(0x41a60000, 0x80000000, 0x7fc12abc)[index % 3], y=(0x41af3333, 0xc0200000)[index % 2])
        for name in ('after_type', 'after_extra', 'after_owner', 'ground_owner'): case.setdefault(name, -1)
    return rows


class CarrierOracle(OwnerOracle):
    """기존 쓰기/명령 경계 검사에 새로운 전체 몸체만 추가한다."""
    def __init__(self, edition):
        """실제 가상 표의 +0xcc 주소를 확인하고 새로운 내보내기를 읽는다."""
        super().__init__(edition)
        self.p = POST[edition]
        self.exports.extend(ROOT / f'extracted/carrierpostpop/{edition}/{name}' for name in ('creation.c', 'functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # Ghidra가 계산한 실제 몸체 범위만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        if struct.unpack('<I', self.mu.mem_read(self.p['vtable'] + 0xcc, 4))[0] != self.p['check']:
            raise RuntimeError('원본 carrier 가상 +0xcc 주소 불일치')
        self.events, self.stub_calls = [], collections.Counter()

    def on_instruction(self, mu, address, size, data):
        """가상 확인/지면 갱신/공통 후처리의 인자와 호출 시점 raw 필드를 관찰한다."""
        p = self.p
        if address in (p['check'], p['ground'], p['base']):
            esp, slot = mu.reg_read(UC_X86_REG_ESP), self.slot(TARGET)
            ret = struct.unpack('<I', mu.mem_read(esp, 4))[0]
            self.stub_calls[f'{address:08x}'] += 1
            if address == p['check']:
                if mu.reg_read(UC_X86_REG_ECX) != slot: raise RuntimeError('carrier 확인 this 오류')
                self.events.append(f'C:{TARGET}')
                # 원본 본문이 읽기 전에 외부 효과로 주입한다. 좌표/소유자를 이전 값으로 캐시하면 재생이 실패한다.
                if self.case['after_type'] >= 0:
                    for name, offset in (('after_type', 10), ('after_extra', self.o['extra']), ('after_owner', self.o['owner'])):
                        mu.mem_write(slot + offset, bytes([self.case[name]]))
                    mu.mem_write(slot + 14, struct.pack('<II', 0x41f20000, 0x41fc0000))
                purge = 0
            elif address == p['ground']:
                x, y, owner = struct.unpack('<3I', mu.mem_read(esp + 4, 12))
                self.events.append(f'G:{x}:{y}:{owner}')
                if self.case['ground_owner'] >= 0: mu.mem_write(slot + self.o['owner'], bytes([self.case['ground_owner']]))
                purge = 0
            else:
                if mu.reg_read(UC_X86_REG_ECX) != slot: raise RuntimeError('공통 postPop this 오류')
                flags = struct.unpack('<I', mu.mem_read(esp + 4, 4))[0]
                raw = mu.mem_read(slot, self.stride)
                self.events.append(f'B:{TARGET}:{flags}:{raw[10]}:{raw[self.o["extra"]]}:{raw[self.o["owner"]]}')
                purge = 4
            # carrier 확인의 임의 반환값은 caller가 사용하지 않는다.
            mu.reg_write(UC_X86_REG_EAX, 0xffffffff)
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        super().on_instruction(mu, address, size, data)

    def run_body(self, case):
        """이 판본의 전체 carrier 또는 damageable 몸체 정상 반환/스택과 사건·슬롯을 관찰한다."""
        self.case, self.events = case, []
        p, slot = self.p, self.slot(TARGET)
        raw = bytearray(self.stride)
        struct.pack_into('<I', raw, 0, p['vtable'])
        raw[10], raw[11], raw[self.o['extra']], raw[self.o['owner']] = case['type'], case['state'], case['extra'], case['owner']
        struct.pack_into('<HII', raw, 12, 0xab84, case['x'], case['y'])
        self.mu.mem_write(slot, bytes(raw))
        # 타입 변화 효과에서도 같은 입력 flags1을 읽게 한다. 모든 genus는 순수 조회 잡음을 포함한다.
        for number in (82, 122, 158):
            base = TYPES + number * self.type_stride
            self.mu.mem_write(base, bytes(self.type_stride))
            self.mu.mem_write(base + 0xe8, struct.pack('<II', case['f1'], 0x00210000))
        for name in ('boss', 'loading', 'geyser'): self.mu.mem_write(p[name], struct.pack('<I', case[name]))
        self.write_ranges = []
        # 대체 효과의 raw 쓰기는 호스트 입력 주입이다. 실제 명령은 스택 밖을 쓰지 않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, slot)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case['flags']))
        entry = p['carrier' if case['kind'] == 'Carrier' else 'damageable']
        self.mu.emu_start(entry, STOP, count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 8:
            raise RuntimeError('carrier/damageable 정상 반환/스택 복구 오류')
        return [';'.join(self.events), bytes(self.mu.mem_read(slot, self.stride)).hex()]


def generate(smoke=False):
    """원본이 정한 독립 관찰과 대체 경계/SHA/실제 진입 수를 기록한다."""
    cases, rows, editions = inputs(), [], {}
    if smoke: cases = cases[::83]
    paths = {Path(__file__), ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/carrierpostpop-functions.json', FIXTURE}
    for edition in SPECS:
        oracle = CarrierOracle(edition)
        for case in cases:
            rows.append([case['kind'], edition, *[case[name] for name in ('flags', 'boss', 'loading', 'type', 'geyser',
                'extra', 'f1', 'state', 'owner', 'x', 'y', 'after_type', 'after_extra', 'after_owner', 'ground_owner')], *oracle.run_body(case)])
        editions[edition] = dict(binary=oracle.spec['binary'], cases=len(cases),
            carrier=f'{oracle.p["carrier"]:08x}', damageable=f'{oracle.p["damageable"]:08x}',
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls),
            assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 전체 몸체 {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 PE Carrier/Damageable 전체 몸체. +0xcc/지면 갱신/공통 postPop만 기록 대체.\n'
        '# 종류 판본 flags boss loading type geyser extra flags1 state owner xbits ybits afterType afterExtra afterOwner groundOwner 사건 슬롯\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, primary_target='10.78', total=len(rows), editions=editions,
        stubbed=['vtable +0xcc', 'ground ownership', 'common Squid postPop'], os_calls=0, normal_return=True,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['synthetic asset type metadata and priest vtable carrier call target',
            'external effects recorded/injected; their complete bodies are not executed',
            'real common bookkeeping and priest prefix/ProcessHost composition verified separately in C++',
            'no original game, priest falling/force field/regen handler or GUI migration'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """입력/기록 SHA와 정상 반환/명시적 대체 수/실제 순수 조회 수를 검사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != report['total'] or report['os_calls'] or not report['normal_return']:
        raise RuntimeError('행 수/실행 경계 오류')
    if report['stubbed'] != ['vtable +0xcc', 'ground ownership', 'common Squid postPop']: raise RuntimeError('대체 경계 오류')
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[1] == edition]
        carrier = sum(row[0] == 'Carrier' for row in selected)
        if item['cases'] != len(selected) or item['assertions'] or item['native_calls'].get(item['carrier'], 0) != carrier:
            raise RuntimeError(f'몸체 진입 증거 오류: {edition}')
        if item['native_calls'].get(item['damageable'], 0) != len(selected): raise RuntimeError('damageable 진입 수 오류')
        for name, marker in (('check', 'C:'), ('ground', 'G:'), ('base', 'B:')):
            if item['stub_calls'].get(f'{POST[edition][name]:08x}', 0) != sum(row[-2].count(marker) for row in selected):
                raise RuntimeError('외부 효과 관찰 수 오류')
        if item['native_calls'].get(f'{POST[edition]["genus"]:08x}', 0) != sum(int(row[2]) & 1 != 0 for row in selected):
            raise RuntimeError('최초 flag 순수 genus 조회 수 오류')
    print(f'carrierpostpop 검증 통과: {len(rows)}개')


def main():
    """전체 생성/저장된 증거 감사/무저장 소규모 대조 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
