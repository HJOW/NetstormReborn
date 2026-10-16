#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Fiery Tail 실제 생성·표시 등록·공유 색 표·삭제를 격리 x86에서 관찰한다."""
import argparse
import csv
import json
import socket
import struct
from collections import Counter
from pathlib import Path
from decomp_particle_gradient import Oracle, ROOT, SOURCE, PALETTE, MERGED, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

DATA, POOL = 0x12500000, 0x12600000  # 수명 객체/목록/입자와 raw 부모의 격리 메모리다.
OBJECT, PARTICLES, TABLE, NEXT = (DATA + offset for offset in (0, 0x1000, 0x3000, 0x4000))  # 실제 요청을 받는 고정 외부 할당 주소다.
ENTRIES = {0x457ea0, 0x487fd0, 0x488190, 0x488100, 0x487ab0, 0x4882c0, 0x44fd70, 0x423aa0, 0x40f1b0, 0x487880, 0x41bd00, 0x4437f0, 0x4e4a40}  # 생성/초기화/스택 확보/소멸을 실제로 실행하는 몸체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fierytail-lifetime-x86.tsv'  # 입력·원본 의미 필드·입자·색/반납 순서를 보존한다.
REPORT = ROOT / 'cpppj/fierytail-lifetime.json'  # 외부 메모리 경계와 원본 정상 반환의 근거다.


def inputs():
    """cold/warm 공유 표, 빈/확장/빈칸 목록, 부모0/부착 중 변경과 삭제 flags를 교차한다."""
    values = []  # 기대 결과를 계산하지 않는 독립 생성 입력이다.
    # 실제 COL 세 개에 대해 동일 생성 경로를 관찰한다.
    for name in ('GIFCLOUD.COL', 'BULF.COL', 'SUNCANNON.COL'):
        # 초기 공유 표가 없거나 이미 생성된 두 상태를 구분한다.
        for warm in (0, 1):
            # 목록 확장과 기존 슬롯 재사용, 부모 없는 공통 등록을 방문한다.
            for capacity, hole, parent, depth, mutate in ((0, -1, 50, 127, 0), (40, 20, 50, 255, 1), (1, -1, 50, 128, 2), (40, 0, 0, 0, 0)):
                values.append((name, warm, capacity, hole, parent, depth, mutate, 1))
    # 상위 BYTE/bit가 삭제 bit0 판정을 바꾸지 않는지 관찰한다.
    for flags in (0, 2, 3, 256, 257, 0x80000000, 0xffffffff):
        values.append(('GIFCLOUD.COL', 1, 0, -1, 50, 127, 0, flags))
    assert len(set(values)) == len(values)
    return values


def slots(capacity, hole):
    """원본이 검색할 초기 목록만 구성하며 등록 결과를 계산하지 않는다."""
    return [0 if index == hole else 100 + index for index in range(capacity)]


def semantic(raw):
    """실제 필드88바이트를 보존하고 공유 팔레트 주소만 토큰1로 정규화한다."""
    assert struct.unpack_from('<I', raw, 0x50)[0] == MERGED
    return raw[4:0x2c] + raw[0x30:0x4c] + struct.pack('<I', 1) + raw[0x54:0x64]


class LifetimeOracle(Oracle):
    """기존 실제 색 띠/정렬에 생성자와 공통 등록/삭제를 더하며 new/free/부착만 경계로 둔다."""
    def __init__(self):
        """현재 PC의 전체 정밀 범위와 격리 SEH/입출력 메모리를 등록한다."""
        super().__init__()
        self.mu.mem_map(DATA, 0x10000)
        self.mu.mem_map(POOL, 0x10000)
        self.mu.mem_map(0, 0x1000)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 스택 helper와 파생/공통 몸체도 함수 간 공백을 허용하지 않는다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.phase = 'setup'  # 실제 색 기초 팔레트 준비와 생성/삭제 관찰을 구분한다.
        self.events = []  # 실제 요청의 시간 순서다.
        self.returns = Counter()  # 최상위 생성/삭제의 정상 반환 수다.

    def integer(self, address):
        """격리 주소의 little endian DWORD만 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def ret(self, value=0, purge=0):
        """외부 경계의 반환값과 실제 호출 규약을 공급한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.integer(esp))

    def instruction(self, mu, address, size, data):
        """원본 등록/필드/색 계산을 유지하고 확보/반납/Host 부착의 인자와 순서를 기록한다."""
        if address == 0x4e4391:
            amount = self.integer(mu.reg_read(UC_X86_REG_ESP) + 4)
            choices = {2880: PARTICLES, 8: MERGED, 44: MERGED + 0x1000, (self.capacity + 40) * 4: NEXT}
            assert amount in choices, amount
            self.events.append(f'N:{amount}')
            self.calls['new-boundary'] += 1
            self.ret(choices[amount])
            return
        if address == 0x4e4354:
            pointer = self.integer(mu.reg_read(UC_X86_REG_ESP) + 4)
            label = {TABLE: 'R', NEXT: 'R', PARTICLES: 'P', OBJECT: 'O'}[pointer]
            self.events.append('F:' + label)
            self.calls['free-boundary'] += 1
            entry = self.integer(self.integer(0x595138) + self.integer(OBJECT + 0x1c) * 4) if label != 'R' else None
            if label == 'P':
                assert entry == OBJECT
            elif label == 'O':
                assert entry == 0
            self.ret()
            return
        if address == 0x41bd20:
            kind, parent, form, flags = struct.unpack('<4I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 16))
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT and (kind, parent, form, flags) == (11, self.parent, 0, 16)
            slot, depth = struct.unpack('<2i', mu.mem_read(OBJECT + 0x1c, 8))
            assert self.integer(self.integer(0x595138) + slot * 4) == OBJECT
            self.events.append(f'B:{kind}:{parent}:{form}:{flags}:{slot}:{depth}')
            mu.mem_write(OBJECT + 4, struct.pack('<2I', 7, 51 if self.mutate == 2 else parent))
            if self.mutate == 1:
                mu.mem_write(POOL + parent * 50 + 35, bytes([128]))
            self.calls['attach-boundary'] += 1
            self.ret(OBJECT, 16)
            return
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        super().instruction(mu, address, size, data)

    def write(self, mu, access, address, size, value, data):
        """실제 객체/입자/목록/공유 전역/SEH와 기존 색 계산 범위 밖 쓰기는 거부한다."""
        regions = ((OBJECT, OBJECT + 0x68), (PARTICLES, PARTICLES + 2880), (TABLE, TABLE + 400), (NEXT, NEXT + 400),
                   (0, 4), (0x595138, 0x595140), (0x55a750, 0x55a754))
        if any(low <= address and address + size <= high for low, high in regions):
            return
        # 이 부모 BYTE 쓰기는 명시 부착 경계가 수행하며 원본 몸체가 쓸 수 있는 범위는 아니다.
        super().write(mu, access, address, size, value, data)

    def snapshot(self):
        """현재 목록의 객체 포인터만 토큰1로 바꾸고 다른 슬롯은 그대로 보존한다."""
        table, capacity = self.integer(0x595138), self.integer(0x59513c)
        result = [self.integer(table + index * 4) for index in range(capacity)]
        return ','.join(str(1 if value == OBJECT else value) for value in result) or '-'

    def sample(self, case, precision):
        """실제 생성자를 끝낸 뒤 실제 scalar 삭제를 실행하며 별도 가짜 기대값을 만들지 않는다."""
        name, warm, self.capacity, hole, self.parent, depth, self.mutate, flags = case
        mu = self.mu
        col = (ROOT / 'originals/d' / name).read_bytes()
        rgbx = b''.join(col[8 + index * 3:11 + index * 3] + b'\0' for index in range(256))
        mu.mem_write(SOURCE, rgbx)
        self.phase = 'setup'
        self.call(0x439de0, PALETTE, struct.pack('<I', SOURCE), 4, precision, PALETTE)
        self.phase = 'construct'
        mu.mem_write(OBJECT, b'\xa5' * 0x68)
        mu.mem_write(PARTICLES, b'\x5a' * 2880)
        mu.mem_write(MERGED, struct.pack('<2I', 11, MERGED + 0x1000))
        mu.mem_write(MERGED + 0x1000, struct.pack('<11I', *range(31, 42)))
        mu.mem_write(0, struct.pack('<I', 0x87654321))
        mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        mu.mem_write(0x54109c, struct.pack('<I', 11))
        mu.mem_write(0x55a750, struct.pack('<I', MERGED if warm else 0))
        mu.mem_write(0x595138, struct.pack('<2I', TABLE if self.capacity else 0, self.capacity))
        mu.mem_write(POOL, bytes(2600))
        mu.mem_write(POOL + 35, bytes([255]))
        mu.mem_write(POOL + 50 * 50 + 35, bytes([depth]))
        mu.mem_write(POOL + 51 * 50 + 35, bytes([127]))
        initial = slots(self.capacity, hole)
        if initial:
            mu.mem_write(TABLE, struct.pack('<' + 'I' * len(initial), *initial))
        self.events = []
        self.call(0x457ea0, OBJECT, struct.pack('<I', self.parent), 4, precision, OBJECT)
        self.returns['constructor'] += 1
        raw = bytes(mu.mem_read(OBJECT, 0x68))
        # delta/배율delta와 미사용 두DWORD는 원본 생성자가 쓰지 않는다.
        assert raw[0x40:0x48] == b'\xa5' * 8 and raw[0x4c:0x50] == b'\xa5' * 4 and raw[0x64:] == b'\xa5' * 4
        assert self.integer(0) == 0x87654321 and self.integer(0x55a750) == MERGED
        particles = bytes(mu.mem_read(PARTICLES, 2880))
        assert particles == bytes(2880)
        assert self.integer(MERGED) == 11
        palette = bytes(mu.mem_read(MERGED + 0x1000, 44))
        before, registered, constructed = semantic(raw).hex(), self.snapshot(), ';'.join(self.events)
        self.events = []
        self.call(0x4882c0, OBJECT, struct.pack('<I', flags), 4, precision, OBJECT)
        self.returns['destructor'] += 1
        after = bytes(mu.mem_read(OBJECT, 0x68))
        assert after[0xc:] == raw[0xc:] and self.integer(OBJECT) == 0x5019ec
        assert self.integer(0) == 0x87654321 and self.integer(0x55a750) == MERGED
        assert bytes(mu.mem_read(MERGED + 0x1000, 44)) == palette
        assert bytes(mu.mem_read(SOURCE, 1024)) == rgbx
        return (rgbx.hex(), before, particles.hex(), palette.hex(), registered, constructed, semantic(after).hex(), self.snapshot(), ';'.join(self.events))


def main():
    """생성·삭제 원본 결과를 보존하거나 전체 근거 해시와 입력 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        # 계산을 다시 수행하지 않고 원본/도구/관찰 바이트를 검사한다.
        for path, expected in facts['sha256'].items():
            assert digest(ROOT / path) == expected, path
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] * 2 and all(len(row) == 12 for row in rows)
        assert len({(row[1], row[2]) for row in rows}) == len(rows)
        print('Fiery Tail 수명 SHA/고유 입력 감사:', facts['uniqueInputs'])
        return
    cases = inputs()[:1] if args.smoke else inputs()
    oracle, rows, differences = LifetimeOracle(), [], []
    # 원본 기본53비트와 비교64비트의 출력을 각각 원문으로 보존한다.
    for index, case in enumerate(cases):
        outputs = []
        # 동일 입력을 정밀도별 고유 입력으로 중복 계산하지 않는다.
        for precision in (0x027f, 0x037f):
            result = oracle.sample(case, precision)
            outputs.append(result)
            rows.append((f'case{index}', precision, ','.join(map(str, case)), *result))
        if outputs[0] != outputs[1]:
            differences.append(index)
        print('case', index, '생성/삭제 정상 반환', flush=True)
    if args.smoke:
        return
    FIXTURE.write_text('# name cw input rgbx fields particles palette registered constructEvents destroyed unregistered destroyEvents\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_fierytail_lifetime.py', FIXTURE.relative_to(ROOT).as_posix()]
    paths.extend('originals/d/' + name for name in ('GIFCLOUD.COL', 'BULF.COL', 'SUNCANNON.COL'))
    facts = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(cases), returns=dict(oracle.returns), precisionDifferentInputs=differences, nativeCalls=dict(sorted(oracle.calls.items())), sha256={path: digest(ROOT / path) for path in paths}, limitations=['생성자/목록 등록/초기화/색 띠/CRT 정렬/병합/삭제는 실제 PE 몸체다.', 'new/free의 성공·주소와 BaseProcess 부착은 명시 외부 경계다. 실제 Host/Kernel이나 게임 heap을 실행하지 않았다.', '실제8+3색44바이트 뒤 메모리는 이 생성/삭제 관찰로 확인하지 않았다. 임의12번째 색을 공급하지 않는다.', '기본53비트027f를 C++에서 대조한다. 비교037f 차이는 별도로 보존한다.', '실제 게임/GUI/Session 진행·사용자 자연 완주의 증거가 아니다.'])
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('Fiery Tail 수명:', len(cases), '고유 입력', dict(oracle.returns))


if __name__ == '__main__':
    main()
