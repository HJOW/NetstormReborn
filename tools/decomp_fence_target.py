#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 보호 PE의 전체 fencemark 표적·선분·포구 계산을 외부 효과 대체 없이 관찰한다."""
import collections
import csv
import itertools
import json
import random
from pathlib import Path
import socket
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, PLAYERS, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_EFLAGS)

FIXTURE = ROOT / 'cpppj/tests/fixtures/fence-target-x86.tsv'  # 기존 총 fixture와 구분한 전체 관찰 결과다.
REPORT = ROOT / 'cpppj/fence-target.json'  # 현재 입력/코드/몸체/반환 경계 근거다.
IDS = (15000, 15001, 15002, 15003, 15004, 15005, 15006, 15007)  # 발사자/표적/첫 끝점쌍/마크/둘째 마크/둘째 끝점쌍이다.
OUTPUT, SHAPES, MAPS = PLAYERS + 0x10000, PLAYERS + 0x11000, 0x11400000  # 격리 출력/SHP/네 해시 배열이다.
KEYS = ('kind', 'fx', 'fy', 'tx', 'ty', 'ax', 'ay', 'bx', 'by', 'fw', 'fh', 'tw', 'th', 'group',
        'owner', 'allied', 'sx', 'sy', 'hx', 'hy', 'ox', 'oy', 'frame', 'level', 'marks')  # 판독기와 C++가 공유하는 명시 입력 열이다.
PRESERVED = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789,
             UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}  # 실제 호출 규약 보존 레지스터의 입력 패턴이다.


def bits(value):
    """입력 좌표를 float32 비트로 바꾸며 결과 계산은 하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def value(word):
    """등록할 해시 버킷의 명시 입력 좌표만 읽는다."""
    return struct.unpack('<f', struct.pack('<I', word))[0]


def inputs():
    """축·양쪽·발자국·Cannon·동맹·포구·해시 단계의 고유 입력을 만든다."""
    base = dict(zip(KEYS, ('A', bits(90), bits(100), bits(110), bits(100), bits(100), bits(80), bits(100), bits(120),
                          1, 1, 1, 1, 0, 2, 0, bits(16), bits(11), 0, 0, 0, 0, 0, 0, 1)))
    cases = []
    positions = ((90, 100, 110, 100), (110, 100, 90, 100), (90, 90, 110, 110),
                 (95, 100, 90, 110), (101, 100, 99, 100), (100, 90, 110, 100), (110, 100, 100, 100))
    # 한 배리어의 실제 양 끝점 순서와 이동 방향을 독립 입력으로 조합한다.
    for axis, reverse, position, target_size, firing_size, group in itertools.product((0, 1), (0, 1), positions,
            ((1, 1), (3, 2), (5, 4)), ((1, 1), (3, 2)), (0, 1)):
        case = base.copy()
        ax, ay, bx, by = (100, 80, 100, 120) if not reverse else (100, 120, 100, 80)
        fx, fy, tx, ty = position
        if axis:
            ax, ay, bx, by, fx, fy, tx, ty = ay, ax, by, bx, fy, fx, ty, tx
        case.update(fx=bits(fx), fy=bits(fy), tx=bits(tx), ty=bits(ty), ax=bits(ax), ay=bits(ay), bx=bits(bx), by=bits(by),
                    tw=target_size[0], th=target_size[1], fw=firing_size[0], fh=firing_size[1], group=group)
        cases.append(case)
    # 같은 owner와 비대칭 현재 동맹의 조기 반환을 관찰한다.
    for owner, allied in ((1, 0), (2, 1), (2, 0)):
        case = base.copy();case.update(owner=owner, allied=allied);cases.append(case)
    # 전체 finder는 없음/한 개/두 배리어와 네 현재 해시 도장 단계에서 실행한다.
    for position, level, marks in itertools.product(positions[:3], range(4), (0, 1, 2, -2)):
        case = base.copy();fx, fy, tx, ty = position
        case.update(kind='P', fx=bits(fx), fy=bits(fy), tx=bits(tx), ty=bits(ty), level=level, marks=marks)
        cases.append(case)
    # 실제 일반 포구/시야 사각형의 SHP WORD·TYPE hotspot·분수/보드 끝·배율 입력이다.
    for kind, frame, position, scale in itertools.product(('M', 'R'), range(4), ((94.125, 108.375), (1.25, 2.5), (255.5, 255.75)), ((16, 11), (3, 7))):
        case = base.copy();case.update(kind=kind, frame=frame, fx=bits(position[0]), fy=bits(position[1]), fw=3, fh=2,
                                     sx=bits(scale[0]), sy=bits(scale[1]), hx=2, hy=-3, ox=-7, oy=9)
        cases.append(case)
    # 선분 교차의 수직/수평/사선·평행·양 대각 끝점과 분수 좌표를 직접 관찰한다.
    segments = (((20, 10), (20, 30), (10, 20), (30, 20)), ((10, 10), (30, 30), (10, 30), (30, 10)),
                ((10, 10), (30, 10), (10, 20), (30, 20)), ((20, 10), (20, 30), (20, 15), (20, 35)),
                ((10, 10), (30, 30), (30, 30), (40, 10)), ((10.25, 20.5), (100.75, 130.25), (20.5, 130.5), (95.25, 10.75)))
    for a, b, c, d in segments:
        case = base.copy();case.update(kind='I', ax=bits(a[0]), ay=bits(a[1]), bx=bits(b[0]), by=bits(b[1]),
                                     fx=bits(c[0]), fy=bits(c[1]), tx=bits(d[0]), ty=bits(d[1]));cases.append(case)
    rng = random.Random(0x4568b0)  # 입력만 결정하는 고정 난수이며 교차 결과는 계산하지 않는다.
    # 임의 대각 배리어의 반대편 두 위치·발자국·Cannon 분기를 실제 몸체로 관찰한다.
    for _ in range(256):
        ax, ay = rng.randrange(80, 151), rng.randrange(80, 151)
        dx, dy = rng.choice((-30, -20, -10, 10, 20, 30)), rng.choice((-25, -15, 0, 15, 25))
        middle = (ax + dx * 0.5, ay + dy * 0.5)
        first, second = rng.choice((0.5, 0.75, 1.25)), rng.choice((0.5, 1.0, 1.5))
        case = base.copy();case.update(ax=bits(ax), ay=bits(ay), bx=bits(ax+dx), by=bits(ay+dy),
            fx=bits(middle[0]-dy*first), fy=bits(middle[1]+dx*first),
            tx=bits(middle[0]+dy*second), ty=bits(middle[1]-dx*second),
            fw=rng.randrange(1, 5), fh=rng.randrange(1, 5), tw=rng.randrange(1, 6), th=rng.randrange(1, 6), group=rng.randrange(2))
        cases.append(case)
    # 픽셀 배율 변경에서도 하드코딩된16/11 픽셀 보정의 실제 나눗셈을 유지하는지 관찰한다.
    for scale, index in itertools.product(((3, 7), (17, 13)), range(16)):
        case = cases[index*317//15].copy();case.update(sx=bits(scale[0]), sy=bits(scale[1]));cases.append(case)
    # 분수 사선끼리의 float 절편 저장·평행·범위 밖 반환을 고정 난수 입력으로 관찰한다.
    for _ in range(128):
        points = [rng.randrange(80, 921)*0.25 for _ in range(8)]
        case = base.copy();case.update(kind='I', **dict(zip(('ax', 'ay', 'bx', 'by', 'fx', 'fy', 'tx', 'ty'), map(bits, points))));cases.append(case)
    return list({tuple(case[key] for key in KEYS): case for case in cases}.values())


class FenceOracle(OwnerOracle):
    """이번 새 정밀 범위만 허용하며 helper/가상 필터/CRT를 모두 실제 명령으로 실행한다."""
    def __init__(self):
        """같은 server SID/TYPE188과 별도 네 해시 배열을 준비한다."""
        super().__init__('originals')
        self.mu.mem_map(POOL + 0x2000, 0x1fe000)
        self.mu.mem_map(MAPS, 0x40000)
        self.paths = [ROOT / f'extracted/fence-target/originals/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed = []  # 이전 단위의 몸체 허용 범위를 사용하지 않는다.
        self.entries = set()  # 현재 범위 진입점만 센다.
        self.native_calls = collections.Counter()  # 모든 실제 하위 호출의 실행 수다.
        with self.paths[1].open(encoding='utf-8') as stream:
            # 실제 불연속 몸체의 각 구간을 허용한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 범위 사이의 코드는 실행할 수 없다.
                for part in row['ranges'].split(';'):
                    low, high = (int(item, 16) for item in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.write_ranges = [(OUTPUT, OUTPUT + 16)]
        self.bases = []  # 같은 네 단계의 명시 해시 머리 저장소다.
        address = MAPS
        # 원본 네 해시 배열 크기대로 이어 붙인다.
        for scale in (1, 2, 4, 16):
            self.bases.append(address);address += (256 // scale) ** 2 * 2
        self.put(0x5c847c, 24000)
        self.put(0x5411c0, 90)
        self.mu.mem_write(0x542514, struct.pack('<4I', *self.bases))

    def put(self, address, word, width=4):
        """명시 입력을 지정 폭의 little endian으로만 쓴다."""
        self.mu.mem_write(address, (int(word) & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))

    def setup(self, case):
        """TYPE/SHP/슬롯/등록 체인만 만든다. 표적·교차 결과는 계산하지 않는다."""
        self.mu.mem_write(POOL, bytes(24000 * 50));self.mu.mem_write(TYPES, bytes(188 * 500));self.mu.mem_write(MAPS, bytes(0x40000))
        self.mu.mem_write(0x595200, bytes(81 * 4));self.put(0x595200 + (9 + case['owner']) * 4, case['allied'])
        self.put(0x59a92c, case['sx']);self.put(0x59a930, case['sy'])
        scale = (1, 2, 4, 16)[case['level']]
        self.mu.mem_write(0x542508, struct.pack('<3I', case['level'], scale, 256 // scale))
        # 바이트가 겹치지 않는 실제 TYPE 필드와 네 물리 SHP 헤더를 준비한다.
        for number, width, height in ((71, case['fw'], case['fh']), (158, case['tw'], case['th']), (136, 1, 1), (90, 1, 1)):
            start = TYPES + number * 500
            self.put(start + 0x1d4, width);self.put(start + 0x1d8, height)
        self.put(TYPES + 71 * 500 + 0x9c, case['group']);self.put(TYPES + 71 * 500 + 0xdc, SHAPES)
        self.put(TYPES + 71 * 500 + 0x114, 4);self.put(TYPES + 71 * 500 + 0x1dc, case['hx']);self.put(TYPES + 71 * 500 + 0x1e0, case['hy'])
        # 현재 프레임이 읽는 포구 WORD를 정수 입력으로 준비한다.
        for frame in range(4):
            header = SHAPES + 0x80 + frame * 0x40
            self.put(SHAPES + 8 + frame * 8, header - SHAPES + 36)
            self.mu.mem_write(header + 32, struct.pack('<hh', case['ox'] + frame * 3, case['oy'] - frame))
        a, b = (value(case['ax']), value(case['ay'])), (value(case['bx']), value(case['by']))
        positions = [(case['fx'], case['fy']), (case['tx'], case['ty']), (case['ax'], case['ay']), (case['bx'], case['by']),
                     (bits((a[0] + b[0]) / 2), bits((a[1] + b[1]) / 2)),
                     (bits((a[0] + b[0]) / 2 + 4), bits((a[1] + b[1]) / 2)),
                     (bits(a[0] + 4), bits(a[1])), (bits(b[0] + 4), bits(b[1]))]
        types = (71, 158, 136, 136, 90, 90, 136, 136)
        # 정상 server 자산과 압축 끝점을 같은 풀에 쓴다.
        for index, sid in enumerate(IDS):
            raw = bytearray(50);raw[10] = types[index];raw[34] = 1 if index == 0 else case['owner']
            struct.pack_into('<II', raw, 14, *positions[index]);struct.pack_into('<I', raw, 36, case['frame'] if index == 0 else 0)
            if index in (4, 5):
                first, second = (IDS[2], IDS[3]) if index == 4 else (IDS[6], IDS[7])
                struct.pack_into('<H', raw, 12, first);struct.pack_into('<I', raw, 26, second >> 8);raw[30] = second & 255
            self.mu.mem_write(self.slot(sid), bytes(raw))
        registered = list(range(4)) + ([4] if abs(case['marks']) == 1 else [4, 5] if abs(case['marks']) == 2 else [])
        if case['marks'] < 0:
            registered.reverse()
        # 같은 버킷에 입력 순서대로 머리를 삽입한다. 검색 순서의 결과는 원본이 정한다.
        for index in registered:
            x, y = (int(value(word)) for word in positions[index])
            address = self.bases[1] + ((y // 2) * 128 + x // 2) * 2
            previous, = struct.unpack('<H', self.mu.mem_read(address, 2))
            self.put(self.slot(IDS[index]) + 4, previous, 2);self.put(address, IDS[index], 2)

    def sample(self, case):
        """실제 진입·모든 하위 계산·정상 반환과 원본 raw 불변을 확인한다."""
        self.setup(case);self.mu.mem_write(OUTPUT, bytes(16))
        kind = case['kind'];this = self.slot(IDS[4]) if kind == 'A' else 0
        if kind == 'A':entry, args, purge = 0x4568b0, (OUTPUT, IDS[0], IDS[1]), 12
        elif kind == 'P':entry, args, purge = 0x4571b0, (OUTPUT, IDS[0], IDS[1]), 0
        elif kind == 'I':entry, args, purge = 0x462800, (OUTPUT, *(case[key] for key in ('ax', 'ay', 'bx', 'by', 'fx', 'fy', 'tx', 'ty'))), 0
        else:entry, args, purge = (0x4819b0 if kind == 'M' else 0x4818d0), (OUTPUT, IDS[0]), 0
        self.mu.reg_write(UC_X86_REG_EAX, 0xdeadbeef);self.mu.reg_write(UC_X86_REG_EDX, 0xbaadf00d)
        # 모든 callee-saved 레지스터에 비영 패턴을 넣는다.
        for register, word in PRESERVED.items():self.mu.reg_write(register, word)
        self.mu.reg_write(UC_X86_REG_ECX, this);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f);self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.mem_write(STACK, struct.pack(f'<{len(args)+1}I', STOP, *args))
        self.mu.emu_start(entry, STOP, count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + purge
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x027f
        # 실제 호출 규약 보존을 전부 확인한다.
        for register, word in PRESERVED.items():assert self.mu.reg_read(register) == word
        return tuple(struct.unpack('<4I', self.mu.mem_read(OUTPUT, 16)))


def main():
    """현재 호스트의 정밀 Native 관찰을 새 fixture에 보관한다."""
    oracle = FenceOracle();cases = inputs();rows = []
    # 각 루트는 고유 입력이며 C++ 구현의 분기/좌표를 기대값으로 사용하지 않는다.
    for index, case in enumerate(cases):
        try: result = oracle.sample(case)
        except Exception:
            print('FAILED', index, case, flush=True);raise
        rows.append((*[case[key] for key in KEYS], *result))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('# ' + KEYS[0], *KEYS[1:], 'x', 'y', 'right', 'bottom'));writer.writerows(rows)
    report = {'host': socket.gethostname(), 'date': '2026-10-15', 'unique_inputs': len(rows), 'returns': len(rows),
              'kinds': dict(collections.Counter(case['kind'] for case in cases)), 'assertions': oracle.assertions,
              'external_replacements': [], 'x87_control': '027f', 'binary_sha256': oracle.sha256,
              'tool_sha256': digest(Path(__file__)), 'helper_sha256': digest(ROOT / 'tools/decomp_owner_oracle.py'),
              'fixture_sha256': digest(FIXTURE), 'exports': {str(path.relative_to(ROOT)): digest(path) for path in oracle.paths},
              'native_calls': dict(oracle.native_calls)}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
