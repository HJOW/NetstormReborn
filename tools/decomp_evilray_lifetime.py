#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EvilRay 생성/중단/소멸을 격리 PE에서 실행한다. 실제 게임/창/통신은 실행하지 않는다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_fierytail_lifetime import LifetimeOracle, ROOT, OBJECT, PARTICLES, POOL, TABLE, NEXT, MERGED, SOURCE, PALETTE, slots
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT / 'cpppj/tests/fixtures/evilray-lifetime-x86.tsv'  # 새 원문 의미 필드/색/반납 결과이며 기존 파일을 덮어쓰지 않는다.
ENTRIES = (0x4530b0, 0x453240, 0x487ee0)  # 실제 생성자/Stop/현재 source 조회만 기존 원문 허용 범위에 추가한다.


def semantic(raw):
    """포인터/패딩을 제외한80BYTE를 보존한다. source/stopping도 전체 DWORD다."""
    return raw[4:0x2c] + raw[0x30:0x4c] + raw[0x50:0x5c]


class EvilRayOracle(LifetimeOracle):
    """공통 등록/입자 초기화/실제 색 정렬/소멸을 그대로 실행하고 외부 할당/Host/진단만 공급한다."""

    def __init__(self):
        """현재 PC의 정밀 함수 범위에 원문 세 몸체를 추가한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 각 함수의 실제 분할 범위만 허용하고 공백/다른 함수를 실행하지 않는다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.diagnostics = []  # 실제 진단 문자열과 줄의 순서다.

    def instruction(self, mu, address, size, data):
        """50입자/3색 확보와 실제 TYPE 인자·부착 뒤 source 변경 및 진단 복귀를 기록한다."""
        if address == 0x4e4391:
            amount = self.integer(mu.reg_read(UC_X86_REG_ESP) + 4)
            choices = {1200: PARTICLES, 8: MERGED, 12: MERGED + 0x1000, (self.capacity + 40) * 4: NEXT}  # 실제 원본 요청 폭과 독립 외부 주소다.
            assert amount in choices, amount
            self.events.append(f'N:{amount}')
            self.ret(choices[amount])
            return
        if address == 0x41bd20:
            kind, parent, form, flags = struct.unpack('<4I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 16))
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT and (kind, parent, form, flags) == (self.kind, self.parent, 0, 16)
            slot, depth = struct.unpack('<2i', mu.mem_read(OBJECT + 0x1c, 8))
            self.events.append(f'B:{kind}:{parent}:{form}:{flags}:{slot}:{depth}')
            mu.mem_write(OBJECT + 4, struct.pack('<2I', 7, 51 if self.mutate else parent))
            if self.mutate:
                mu.mem_write(0x541098, struct.pack('<I', 88))
                mu.mem_write(0x595128, struct.pack('<I', 0xffffffff))
                mu.mem_write(POOL + parent * 50 + 35, bytes([128]))
            self.ret(OBJECT, 16)
            return
        if address == 0x4e0620:
            expression, file, line = struct.unpack('<3I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 12))
            self.diagnostics.append([bytes(mu.mem_read(expression, 80)).split(b'\0')[0].decode('ascii'), bytes(mu.mem_read(file, 80)).split(b'\0')[0].decode('ascii'), line])
            self.ret()
            return
        super().instruction(mu, address, size, data)

    def write(self, mu, access, address, size, value, data):
        """EvilRay 공유 색 전역의 원문 쓰기만 기존 공통 메모리 경계에 추가한다."""
        if 0x55a6f0 <= address and address + size <= 0x55a6f4:
            return
        super().write(mu, access, address, size, value, data)

    def observe(self, name, warm, parent, mutate, kind, source, control):
        """입력만 설치하고 원본 생성/입자/색/목록/소멸 결과를 읽는다."""
        self.parent, self.mutate, self.kind, self.capacity = parent, mutate, kind, 40
        mu = self.mu
        col = (ROOT / 'originals/d' / name).read_bytes()
        rgbx = b''.join(col[8 + index * 3:11 + index * 3] + b'\0' for index in range(256))
        mu.mem_write(SOURCE, rgbx)
        self.call(0x439de0, PALETTE, struct.pack('<I', SOURCE), 4, control, PALETTE)
        mu.mem_write(OBJECT, b'\xa5' * 0x68)
        mu.mem_write(PARTICLES, b'\x5a' * 1200)
        mu.mem_write(POOL, bytes(2600))
        mu.mem_write(POOL + 35, bytes([255]))
        mu.mem_write(POOL + 50 * 50 + 35, bytes([127]))
        mu.mem_write(POOL + 51 * 50 + 35, bytes([64]))
        mu.mem_write(0, struct.pack('<I', 0x87654321))
        mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        mu.mem_write(0x541098, struct.pack('<I', kind))
        mu.mem_write(0x595128, struct.pack('<I', source))
        mu.mem_write(0x55a6f0, struct.pack('<I', MERGED if warm else 0))
        mu.mem_write(MERGED, struct.pack('<2I', 3, MERGED + 0x1000))
        mu.mem_write(MERGED + 0x1000, struct.pack('<3I', 31, 32, 33))
        mu.mem_write(0x595138, struct.pack('<2I', TABLE, 40))
        mu.mem_write(TABLE, struct.pack('<40I', *slots(40, 20)))
        self.events = []
        self.call(0x4530b0, OBJECT, struct.pack('<I', parent), 4, control, OBJECT)
        raw = bytes(mu.mem_read(OBJECT, 0x68))
        assert bytes(mu.mem_read(PARTICLES, 1200)) == bytes(1200)
        assert raw[0x40:0x48] == b'\xa5' * 8 and raw[0x4c:0x50] == b'\xa5' * 4 and raw[0x5c:0x60] == b'\xa5' * 4
        colors = bytes(mu.mem_read(MERGED + 0x1000, 12)).hex()
        before, registered, created = semantic(raw).hex(), self.snapshot(), ';'.join(self.events)
        self.events = []
        self.call(0x4882c0, OBJECT, struct.pack('<I', 1), 4, control, OBJECT)
        assert self.integer(0x55a6f0) == MERGED and bytes(mu.mem_read(SOURCE, 1024)) == rgbx
        return rgbx.hex(), before, colors, registered, created, semantic(bytes(mu.mem_read(OBJECT, 0x68))).hex(), self.snapshot(), ';'.join(self.events)

    def stop(self, mode, stopping, control):
        """원문 mode/중단 DWORD를 입력하고 실제 진단/쓰기만 관찰한다."""
        self.mu.mem_write(OBJECT, b'\xa5' * 0x60)
        self.mu.mem_write(OBJECT + 0x58, struct.pack('<I', stopping))
        self.diagnostics = []
        self.call(0x453240, OBJECT, struct.pack('<3I', mode, 0x11223344, 0x55667788), 12, control)
        return self.integer(OBJECT + 0x58), json.dumps(self.diagnostics, separators=(',', ':'))


def main():
    """독립 입력의 두 x87 관측을 새 파일로 보존하거나 기대값을 고치지 않고 재관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows = []  # 각 원문 호출의 입력과 관찰값만 보존한다.
    oracle = EvilRayOracle()
    # 실제 두 COL·공유 cold/warm·부모0/부착 변경·전체 source/TYPE를 교차한다.
    for name in ('GIFCLOUD.COL', 'BULF.COL'):
        for warm in (0, 1):
            for parent, mutate, kind, source in ((0, 0, 10, 0), (50, 0, 57, 0x12345678), (50, 1, 10, 77)):
                for control in (0x027f, 0x037f):
                    result = oracle.observe(name, warm, parent, mutate, kind, source, control)
                    rows.append(('create', name, warm, parent, mutate, kind, source, control, *result))
    # 알 수 없는 mode·이미 중단/처음 중단의 전체 DWORD를 원문에서 관찰한다.
    for mode, stopping in ((0, 0), (2, 7), (3, 0), (3, 1), (3, 0xffffffff)):
        for control in (0x027f, 0x037f):
            rows.append(('stop', mode, stopping, control, *oracle.stop(mode, stopping, control)))
    content = '# kind nativeInputs nativeOutputs; create는24행, stop은10행. 실제 PE 관측이며 게임 실행0.\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify로 재관측한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'create': 24, 'stop': 10, 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
