#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 postPop 전체 wrapper·실제 이동 불가/지면/프레임을 세 PE에서 정상 반환까지 대조한다.

Regular 경계와 Carrier·낙하·보호막 생성/삭제만 명시 대체한다. 게임/OS 실행은 없다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_priestpostpop_oracle import PriestPostPopOracle, POST, PRIEST_TYPE, EVENT, MEMORY
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, LISTS, TARGET, STACK, STOP, digest
from decomp_prieststate_oracle import STATE, SPOTS
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 판본별 실제 조건 조회와 명시 효과 경계다. 방향 9는 signed 프레임 side J다.
TAIL = {
    'originals': dict(fall=0x4941f0, shield=0x493d30, clear=0x491980, direction=0x4ad710),
    'originalCD': dict(fall=0x40c880, shield=0x40bfb0, clear=0x40c0d0, direction=0x4ae4b0),
}
TAIL['original1037'] = TAIL['originalCD']
# 원본과 분리된 코드 표와 고정 입력 좌표/정밀도 및 새 결과 경로다.
CODES = LISTS + 0x2000
CONTROLS = (0x027f, 0x037f)
FIXTURE = ROOT / 'cpppj/tests/fixtures/priestpostpoptail-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-priestpostpoptail-evidence.json'
KEYS = ('flags', 'extra', 'hp', 'floor', 'ceil', 'side', 'found', 'allocated', 'change')


def inputs():
    """분기 격자와 Carrier/낙하 효과의 입력 변이를 만든다. 출력 판단은 계산하지 않는다."""
    rows = []
    # 단순 절삭 지면과 거의 올림 지면이 서로 다를 때의 분기까지 교차한다.
    for values in itertools.product((0, 1, 8, 9, 32), (0, 1, 0x800, 0x801), (20, 100),
            (0, 6), (0, 6), (65, 74, 255), (0, 1), (0, 1)):
        extra, flags, hp, floor, ceil, side, found, allocated = values
        rows.append(dict(flags=flags, extra=extra, hp=hp, floor=floor, ceil=ceil, side=side,
            found=found, allocated=allocated, change=0))
    # Carrier가 extra/좌표/HP를 바꾸거나 낙하가 상태를 바꾸어도 원본의 재조회 시점을 관찰한다.
    for extra, flags, found, side, change in itertools.product((0, 1), (0, 1, 0x800, 0x801),
            (0, 1), (65, 74), (1, 2, 3, 4)):
        rows.append(dict(flags=flags, extra=extra, hp=20, floor=0, ceil=0, side=side,
            found=found, allocated=1, change=change))
    return rows


class TailOracle(PriestPostPopOracle):
    """기존 접두 실행기의 실제 HP/목록 실행을 재사용하고 전체 wrapper 반환을 검사한다."""
    def __init__(self, edition):
        """새 읽기 전용 범위만 허용하고 실제 spot/프레임 조회 메모리를 연결한다."""
        super().__init__(edition)
        self.tail, self.state = TAIL[edition], STATE[edition]
        self.mu.mem_map(SPOTS, 0x10000)
        self.mu.mem_write(self.state['spot'], struct.pack('<I', SPOTS))
        self.exports.extend(ROOT / f'extracted/priestpostpoptail/{edition}/{n}' for n in ('creation.c', 'functions.tsv'))
        self.allowed, self.entries = [], set()
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 최신 Ghidra 함수의 불연속 범위만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    lo, hi = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((lo, hi + 1))
        self.returns = 0

    def ret(self, purge=0):
        """대체 호출의 원본 callee 스택 정리 폭을 유지한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.u32(sp))

    def change(self, stage):
        """명시 외부 효과가 공급한 입력만 바꾼다. 이후 원본이 현재 값을 읽는다."""
        c, raw = self.case['change'], self.slot(TARGET)
        if stage == 'carrier' and c == 1:
            self.mu.mem_write(raw + self.o['extra'], b'\x01')
        elif stage == 'carrier' and c in (2, 3):
            self.mu.mem_write(raw + self.o['extra'], bytes([32 if c == 2 else 0]))
            self.mu.mem_write(raw + 26, struct.pack('<I' if self.stride == 50 else '<H', 150))
            self.mu.mem_write(raw + 14, struct.pack('<ff', 30.75, 31.9))
            self.mu.mem_write(SPOTS + 31 * 256 + 30, bytes([0 if c == 2 else 6]))
            self.mu.mem_write(SPOTS + 32 * 256 + 31, b'\x06')
        elif stage == 'fall' and c == 4:
            self.mu.mem_write(raw + self.o['extra'], b'\x01')

    def on_instruction(self, mu, address, size, data):
        """전체 wrapper의 실제 실행과 외부 효과 인자·순서만 관찰한다."""
        p, t = self.p, self.tail
        if address in (p['carrier'], t['fall'], t['shield'], t['clear']):
            if mu.reg_read(UC_X86_REG_ECX) != self.slot(TARGET): raise RuntimeError('후반 효과 this 오류')
            self.stub_calls[f'{address:08x}'] += 1
            if address == p['carrier']:
                flags = self.u32(mu.reg_read(UC_X86_REG_ESP) + 4)
                if flags != self.case['flags']: raise RuntimeError('Carrier flags 오류')
                self.events.append(f'C:{TARGET}:{flags}')
                self.change('carrier'); self.ret(4)
            else:
                marker = 'D' if address == t['fall'] else 'S' if address == t['shield'] else 'X'
                self.events.append(f'{marker}:{TARGET}')
                if marker == 'D': self.change('fall')
                self.ret()
            return
        if address in (p['find'], p['new'], p['regular']):
            PriestPostPopOracle.on_instruction(self, mu, address, size, data)
            return
        OwnerOracle.on_instruction(self, mu, address, size, data)

    def run(self, case, control):
        """원본 진입부터 ret 4와 SEH/보존 레지스터·x87 복구까지 실행한다."""
        self.case, self.events = case, []
        p, raw = self.p, self.slot(TARGET)
        data = bytearray(self.stride)
        struct.pack_into('<I', data, 0, p['vtable'])
        data[10], data[self.o['owner']], data[self.o['extra']] = PRIEST_TYPE, 1, case['extra']
        struct.pack_into('<Hff', data, 12, 1, 20.75, 21.9)
        struct.pack_into('<I' if self.stride == 50 else '<H', data, 26, case['hp'])
        self.mu.mem_write(raw, bytes(data))
        base = TYPES + PRIEST_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride)); self.mu.mem_write(base, struct.pack('<i', 200))
        self.mu.mem_write(base + 0xe8, struct.pack('<II', 0x00069012, 0x00210000))
        self.mu.mem_write(base + 0x124, struct.pack('<I', CODES))
        self.mu.mem_write(CODES, bytes([case['side'], 80, 1, 0]))
        self.mu.mem_write(SPOTS, bytes(65536))
        self.mu.mem_write(SPOTS + 21 * 256 + 20, bytes([case['floor']]))
        self.mu.mem_write(SPOTS + 22 * 256 + 21, bytes([case['ceil']]))
        self.mu.mem_write(p['priest_type'], struct.pack('<I', PRIEST_TYPE)); self.mu.mem_write(p['quarter'], bytes(4))
        self.mu.mem_write(0x5e4794 if self.stride == 50 else 0x546778, bytes(4))
        self.mu.mem_write(p['list'], struct.pack('<III', LISTS, 4, 0)); self.mu.mem_write(LISTS, struct.pack('<4I', 60, 50, 70, 80))
        self.mu.mem_write(p['owners'], struct.pack('<9I', *[0xabc000 + i for i in range(9)]))
        self.write_ranges = [(0, 4), (LISTS, LISTS + 16), (p['list'] + 8, p['list'] + 12), (p['owners'], p['owners'] + 36)]
        self.mu.mem_write(0, struct.pack('<I', 0x12345678))
        preserved = ((UC_X86_REG_EBX, 0x11223344), (UC_X86_REG_ESI, 0x22334455),
            (UC_X86_REG_EDI, 0x33445566), (UC_X86_REG_EBP, 0x44556677))
        # 호출 전의 보존 레지스터와 부동소수점 제어를 설정한다.
        for reg, value in preserved: self.mu.reg_write(reg, value)
        # 호출자가 값을 보장하지 않는 휘발 레지스터도 입력을 고정한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2); self.mu.reg_write(UC_X86_REG_FPCW, control); self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_ECX, raw)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case['flags']))
        self.mu.emu_start(p['entry'], STOP, count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 8:
            raise RuntimeError('전체 wrapper 반환/ESP 오류')
        if any(self.mu.reg_read(r) != v for r, v in preserved) or self.u32(0) != 0x12345678:
            raise RuntimeError('보존 레지스터/SEH 복구 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어/TOP 오류')
        self.returns += 1
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(raw, self.stride)).hex(),
            str(self.u32(p['list'] + 8)) + ':' + ','.join(map(str, struct.unpack('<4I', self.mu.mem_read(LISTS, 16)))),
            ','.join(map(str, struct.unpack('<9I', self.mu.mem_read(p['owners'], 36))))]


def generate(smoke=False):
    """같은 입력의 두 정밀도 관찰이 같을 때 한 행과 판본별 실제/대체 호출 근거를 저장한다."""
    cases, rows, editions = inputs(), [], {}
    if smoke: cases = cases[::97]
    paths = {Path(__file__), ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/decomp_priestpostpop_oracle.py',
        ROOT / 'tools/decomp_prieststate_oracle.py', ROOT / 'tools/ghidra/priestpostpoptail-functions.json', FIXTURE}
    # 각 PE는 자신의 원본 명령만 실행한다.
    for edition in SPECS:
        oracle = TailOracle(edition)
        # 기대값은 원본에서만 얻고 두 정밀도의 모든 관찰을 비교한다.
        for c in cases:
            observations = [oracle.run(c, control) for control in CONTROLS]
            if observations[0] != observations[1]: raise RuntimeError('두 x87 정밀도의 관찰 불일치')
            rows.append([edition, *[c[k] for k in KEYS], *observations[0]])
        editions[edition] = dict(cases=len(cases), returns=oracle.returns, native_calls=dict(oracle.native_calls),
            stub_calls=dict(oracle.stub_calls), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports); paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 전체 사제 postPop {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 사제 postPop 전체 wrapper. Carrier/낙하/보호막·Regular 경계만 대체.\n'
        '# edition flags extra hp floor ceil side found allocated change events raw list owners\n'
        + '\n'.join('\t'.join(map(str, r)) for r in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, primary_target='10.78', host='HJOW-Athlon', decompile_date='2026-10-10',
        total=len(rows), controls=list(CONTROLS), editions=editions, os_calls=0,
        stubbed=['find Regular', 'new(0x28)', 'Regular constructor', 'Carrier postPop', 'fall', 'ensure shield', 'clear shield'],
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['synthetic raw priest/type/spot/frames', 'Carrier/fall/shield bodies are explicit boundaries',
            'no general Priest Pop, GUI/audio or falling animation; CD FDIV workaround disabled']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA·입력 순서·정상 반환/실제 진입·효과 호출 수를 저장 관찰로 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    # 실행에 사용한 모든 원본/도구/내보내기와 fixture의 SHA를 비교한다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    if report['os_calls'] or len(rows) != report['total'] or len(rows) != len(inputs()) * 3: raise RuntimeError('행 수/OS 오류')
    # 판본별 정상 반환과 실제 진입 수를 관찰 행 수와 대조한다.
    for edition, item in report['editions'].items():
        selected = [r for r in rows if r[0] == edition]
        if item['assertions'] or item['cases'] != len(selected) or item['returns'] != len(selected) * 2: raise RuntimeError('반환 감사 오류')
        if item['native_calls'].get(f'{POST[edition]["entry"]:08x}', 0) != item['returns']: raise RuntimeError('실제 wrapper 진입 수 오류')
        # 입력 순서나 누락 열이 바뀌지 않았는지 확인한다.
        for row, c in zip(selected, inputs()):
            if len(row) != 14 or list(map(int, row[1:10])) != [c[k] for k in KEYS]: raise RuntimeError('입력/열 오류')
        # 사건에 기록된 대체 경계 호출 수만 세어 원본 진입 통계와 비교한다.
        for marker, address in (('F:', POST[edition]['find']), ('A:', POST[edition]['new']), ('R:', POST[edition]['regular']),
                ('C:', POST[edition]['carrier']), ('D:', TAIL[edition]['fall']), ('S:', TAIL[edition]['shield']), ('X:', TAIL[edition]['clear'])):
            count = sum(row[10].count(marker) for row in selected) * 2
            if item['stub_calls'].get(f'{address:08x}', 0) != count: raise RuntimeError('대체 호출 감사 오류')
        if item['native_calls'].get(f'{STATE[edition]["wrapper"]:08x}', 0) != sum('S:' in r[10] or 'X:' in r[10] for r in selected) * 2:
            raise RuntimeError('실제 이동 불가 조회 수 오류')
    print(f'사제 postPop 전체 감사 통과: {len(rows)}개')


def main():
    """새 fixture 생성·무저장 소규모 실행·저장 근거 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true'); parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
