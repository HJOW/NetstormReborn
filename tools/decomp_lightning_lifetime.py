#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 공유 빔의 실제 생성·등록·공유 팔레트·경로 소유·소멸을 독립 관찰한다."""
import argparse
import csv
import json
import socket
import struct
from decomp_fierytail_lifetime import LifetimeOracle, ENTRIES as LIFETIME_ENTRIES, slots, OBJECT, PARTICLES, TABLE, NEXT, POOL, DATA
from decomp_particle_gradient import ROOT, SOURCE, PALETTE, MERGED, digest
from decomp_lightning_path import ENTRIES as PATH_ENTRIES
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX

PATH, TYPES, SOUND = DATA + 0x5000, 0x12700000, DATA + 0x6000  # 실제 요청을 받는 경로/readonly 타입/파일명 주소다.
ENTRIES = PATH_ENTRIES | {0x471d10, 0x471e10, 0x471df0, 0x458480, 0x41bfb0}  # 생성/삭제와 실제 경로 하위의 원문 몸체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/lightning-lifetime-x86.tsv'  # 고유 입력과 두 정밀도의 전체 원문이다.
REPORT = ROOT / 'cpppj/lightning-lifetime.json'  # 정상 반환·경계·전체 바이트 근거다.


def inputs():
    """cold/warm·팔레트·목록/부모 재진입·파일명·삭제 bit0을 교차한다."""
    rows = []
    # 실제 COL과 공통 목록의 확장/빈칸/부모0을 방문한다.
    for col in ('GIFCLOUD.COL', 'BULF.COL', 'SUNCANNON.COL'):
        # 이미 존재하는 공유 표는 별도 입력으로 보존한다.
        for warm in (0, 1):
            # 부착 중 부모/깊이 변경을 포함한다.
            for capacity, hole, parent, depth, mutate in ((0, -1, 50, 127, 0), (40, 20, 50, 255, 1), (1, -1, 50, 128, 2), (40, 0, 0, 0, 0)):
                # 맨 오 워의 NULL 파일명과 일반 shortZap 파일명을 구별한다.
                for sound in (0, 1):
                    rows.append((col, warm, capacity, hole, parent, depth, mutate, sound, 1))
    # 상위 비트와 bit0의 독립성을 확인한다.
    for flags in (0, 2, 3, 256, 257, 0x80000000, 0xffffffff):
        rows.append(('GIFCLOUD.COL', 1, 0, -1, 50, 127, 0, 1, flags))
    assert len(set(rows)) == len(rows)
    return rows


def semantic(raw):
    """88BYTE 의미 필드에서 팔레트/경로/선택 파일명 포인터만 토큰1/2/3으로 바꾼다."""
    raw = bytearray(raw); assert struct.unpack_from('<I', raw, 0x50)[0] == MERGED and struct.unpack_from('<I', raw, 0x58)[0] == PATH
    struct.pack_into('<I', raw, 0x50, 1); struct.pack_into('<I', raw, 0x58, 2)
    sound = struct.unpack_from('<I', raw, 0x60)[0]; assert sound in (0, SOUND); struct.pack_into('<I', raw, 0x60, 3 if sound else 0)
    return bytes(raw[4:0x2c] + raw[0x30:0x4c] + raw[0x50:0x64])


class Oracle(LifetimeOracle):
    """실제 색 띠/CRT/공통 수명과 경로를 실행하며 new/free/부착만 외부 경계로 둔다."""
    def __init__(self):
        """기존 격리 수명 실행기에 원문 빔/경로 함수와 readonly 타입 영역을 더한다."""
        super().__init__(); self.mu.mem_map(TYPES, 0x20000)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 원문 몸체의 실제 구간만 허용한다.
        for entry in ENTRIES:
            # 함수별 불연속 범위를 각각 등록한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """실제 확보량/반납 순서와 등록 중 재진입을 기록한다. 계산은 대체하지 않는다."""
        if address == 0x4e4391:
            amount = self.integer(mu.reg_read(UC_X86_REG_ESP) + 4)
            choices = {2880:PARTICLES, 8:MERGED, 44:MERGED + 0x1000, 64:PATH, (self.capacity + 40) * 4:NEXT}
            assert amount in choices, amount; self.events.append(f'N:{amount}'); self.calls['new-boundary'] += 1; self.ret(choices[amount]); return
        if address == 0x4e4354:
            pointer = self.integer(mu.reg_read(UC_X86_REG_ESP) + 4); label = {TABLE:'R', NEXT:'R', PARTICLES:'P', PATH:'L', OBJECT:'O'}[pointer]
            if label in ('P', 'L', 'O'):
                entry = self.integer(self.integer(0x595138) + self.integer(OBJECT + 0x1c) * 4); assert entry == (0 if label == 'O' else OBJECT)
            self.events.append('F:' + label); self.calls['free-boundary'] += 1; self.ret(); return
        if address == 0x41bd20:
            kind, parent, form, flags = struct.unpack('<4I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 16))
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT and (kind, parent, form, flags) == (19, self.parent, 0, 16)
            slot, depth = struct.unpack('<2i', mu.mem_read(OBJECT + 0x1c, 8)); assert self.integer(self.integer(0x595138) + slot * 4) == OBJECT
            self.events.append(f'B:{kind}:{parent}:{form}:{flags}:{slot}:{depth}'); mu.mem_write(OBJECT + 4, struct.pack('<2I', 7, 51 if self.mutate == 2 else parent))
            if self.mutate == 1:
                mu.mem_write(POOL + parent * 50 + 35, bytes([128]))
            self.calls['attach-boundary'] += 1; self.ret(OBJECT, 16); return
        if address in ENTRIES - LIFETIME_ENTRIES:
            self.calls[f'{address:08x}'] += 1
        super().instruction(mu, address, size, data)

    def write(self, mu, access, address, size, value, data):
        """경로64BYTE와 빔 공유 전역 외에는 기존 수명/색 계산의 제한을 유지한다."""
        if PATH <= address and address + size <= PATH + 64 or 0x55a758 <= address and address + size <= 0x55a75c:
            return
        super().write(mu, access, address, size, value, data)

    def sample(self, case, precision):
        """실제 생성과 scalar 삭제를 실행해 전체 상태·팔레트·사건을 읽는다."""
        name, warm, self.capacity, hole, self.parent, depth, self.mutate, sound, flags = case; mu = self.mu
        col = (ROOT / 'originals/d' / name).read_bytes(); rgbx = b''.join(col[8 + index * 3:11 + index * 3] + b'\0' for index in range(256))
        mu.mem_write(SOURCE, rgbx); self.call(0x439de0, PALETTE, struct.pack('<I', SOURCE), 4, precision, PALETTE)
        mu.mem_write(OBJECT, b'\xa5' * 0x68); mu.mem_write(PARTICLES, b'\x5a' * 2880); mu.mem_write(PATH, b'\xa5' * 64); mu.mem_write(SOUND, b'shortZap.wav\0')
        mu.mem_write(MERGED, struct.pack('<2I', 11, MERGED + 0x1000)); mu.mem_write(MERGED + 0x1000, struct.pack('<11I', *range(31, 42)))
        mu.mem_write(0, struct.pack('<I', 0x87654321)); mu.mem_write(0x5c8464, struct.pack('<I', POOL)); mu.mem_write(0x5410bc, struct.pack('<I', 19))
        mu.mem_write(0x55a758, struct.pack('<I', MERGED if warm else 0)); mu.mem_write(0x595138, struct.pack('<2I', TABLE if self.capacity else 0, self.capacity))
        mu.mem_write(0x59ab20, struct.pack('<I', TYPES)); mu.mem_write(0x541348, struct.pack('<I', 188)); mu.mem_write(0x59a924, struct.pack('<2f', 16, 11)); mu.mem_write(POOL, bytes(2700))
        # 부착 전/변경 후 부모와 표적의 실제 타입/중심을 다르게 준비한다.
        for sid, typeid, x, y, z in ((0, 149, 1.5, 1.5, 255), (50, 148, 20.5, 21.25, depth), (51, 144, -7.75, 8.25, 127), (52, 149, 40.125, 35.375, 0)):
            raw = bytearray(50); raw[10] = typeid; raw[35] = z; struct.pack_into('<H', raw, 6, 1); struct.pack_into('<2f', raw, 14, x, y); mu.mem_write(POOL + sid * 50, bytes(raw))
            foot = 2 if typeid == 144 else 3; mu.mem_write(TYPES + typeid * 500, bytes(500)); mu.mem_write(TYPES + typeid * 500 + 0x1d4, struct.pack('<2i', foot, foot))
        initial = slots(self.capacity, hole)
        if initial:
            mu.mem_write(TABLE, struct.pack('<' + 'I' * len(initial), *initial))
        self.events = []; self.call(0x471d10, OBJECT, struct.pack('<3I', self.parent, 52, SOUND if sound else 0), 12, precision, OBJECT); self.returns['constructor'] += 1
        raw = bytes(mu.mem_read(OBJECT, 0x68)); path = bytearray(mu.mem_read(PATH, 64)); assert struct.unpack_from('<I', path, 60)[0] == OBJECT; struct.pack_into('<I', path, 60, 1)
        assert raw[0x40:0x48] == b'\xa5' * 8 and raw[0x4c:0x50] == b'\xa5' * 4 and raw[0x64:] == b'\xa5' * 4 and bytes(mu.mem_read(PARTICLES, 2880)) == bytes(2880)
        assert self.integer(0) == 0x87654321 and self.integer(0x55a758) == MERGED and self.integer(MERGED) == 11
        palette = bytes(mu.mem_read(MERGED + 0x1000, 44)); before, registered, events = semantic(raw).hex(), self.snapshot(), ';'.join(self.events)
        self.events = []; self.call(0x471df0, OBJECT, struct.pack('<I', flags), 4, precision, OBJECT); self.returns['destructor'] += 1
        after = bytes(mu.mem_read(OBJECT, 0x68)); assert after[0xc:] == raw[0xc:] and self.integer(OBJECT) == 0x5019ec and bytes(mu.mem_read(PATH, 60)) == path[:60]
        assert self.integer(0) == 0x87654321 and self.integer(0x55a758) == MERGED and bytes(mu.mem_read(MERGED + 0x1000, 44)) == palette and bytes(mu.mem_read(SOURCE, 1024)) == rgbx
        return (rgbx.hex(), before, path.hex(), bytes(mu.mem_read(PARTICLES, 2880)).hex(), palette.hex(), registered, events, semantic(after).hex(), self.snapshot(), ';'.join(self.events))


def main():
    """독립 원문 기대값을 보존하거나 재계산 없이 SHA/고유 입력을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); parser.add_argument('--smoke', action='store_true'); args = parser.parse_args()
    if args.verify:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        # 전체 근거 파일 바이트를 검사한다.
        for path, wanted in facts['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] * 2 and all(len(row) == 13 for row in rows) and len({(row[1], row[2]) for row in rows}) == len(rows)
        print('빔 수명 SHA/고유 입력 감사:', facts['uniqueInputs']); return
    cases = inputs()[:1] if args.smoke else inputs(); oracle = Oracle(); rows = []; different = []
    # 두 정밀도 원문을 모두 보존하되 고유 입력 수에 중복 가산하지 않는다.
    for index, case in enumerate(cases):
        outputs = []
        # 독립 동일 입력에서 기본53비트/비교64비트를 실행한다.
        for precision in (0x027f, 0x037f):
            result = oracle.sample(case, precision); outputs.append(result); rows.append((f'case{index}', precision, ','.join(map(str, case)), *result))
        if outputs[0] != outputs[1]:
            different.append(index)
    if args.smoke:
        print('빔 생성/삭제 smoke 정상 반환'); return
    FIXTURE.write_text('# name cw input rgbx fields88 path64 particles palette registered constructEvents destroyed unregistered destroyEvents\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_fierytail_lifetime.py', 'tools/decomp_lightning_path.py', 'tools/decomp_lightning_lifetime.py', FIXTURE.relative_to(ROOT).as_posix()]
    paths.extend('originals/d/' + name for name in ('GIFCLOUD.COL', 'BULF.COL', 'SUNCANNON.COL'))
    facts = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(cases), returns=dict(oracle.returns), precisionDifferentInputs=different, nativeCalls=dict(oracle.calls), sha256={path:digest(ROOT / path) for path in paths}, limitations=['생성/색 띠/CRT 정렬/공통 등록/경로/입자 초기화/소멸은 실제 PE 몸체다.', 'new/free 성공·주소·등록 중 Host 부착/재진입은 명시 외부 경계다. 게임 heap/Host/Kernel 실행 증거가 아니다.', '공유 팔레트/경로/선택 파일명 포인터만 토큰1/2/3으로 정규화했다. 미초기화 바이트는 명시 입력으로 보존한다.', '별도 Frame/Draw 대조와 구별하며 실제 캠페인 발사/사용자 자연 완주의 증거가 아니다.'])
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); print('빔 수명:', len(cases), '고유 입력, 정밀도 차이', len(different))


if __name__ == '__main__':
    main()
