#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Fiery Tail의 실제 프레임·난수·시간 조각·색 조회·선 요청을 독립 관찰한다."""
import argparse
import csv
import json
import socket
import struct
from collections import Counter
from pathlib import Path
from decomp_particle_gradient import Oracle, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 새 자료의 기준 경로다.
OBJECT, PARTICLES, POOL, PALETTE, COLORS = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000  # 객체/입자/부모/색수/읽을 수 있는 색 메모리의 격리 영역이다.
STACK, STOP = 0x3007f000, 0x20000000  # 공통 정상 반환 검사와 같은 스택/정지 주소다.
ENTRIES = {0x4580f0, 0x4583a0, 0x41bfb0, 0x418400, 0x418470, 0x455930, 0x487b40, 0x4239f0, 0x487ba0, 0x487e50, 0x4e49c0}  # 대체 없이 실행하는 실제 몸체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fierytail-frame-x86.tsv'  # 전체 의미 필드·입자·dirty/선 기대값이다.
REPORT = ROOT / 'cpppj/fierytail-frame.json'  # 관찰 범위와 외부 경계·전체 바이트 근거다.


def semantic(raw):
    """원본 객체에서 호스트 포인터/미사용 바이트를 제외한68바이트 의미 필드를 보존한다."""
    return raw[0xc:0x1c] + raw[0x24:0x2c] + raw[0x30:0x4c] + raw[0x54:0x64]


def cases():
    """현재 입자/타이머/전역만 준비하며 프레임 결과나 색 번호를 계산하지 않는다."""
    values = []  # 최초 방출·작은/가득 찬 저장소·시간 조각·현재 입자 판정을 방문한다.
    # 용량이 부족해도 난수를 소비하는 최초 방출을 관찰한다.
    for capacity in (0, 1, 2, 10, 12, 120):
        values.append((capacity, 0, 0, 0, 1.13, 0, 0, 0, 0))
    # 같은 원본 몸체의 시간 조각/부호/offset과 실제120개 저장소를 교차한다.
    for capacity in (0, 2, 12, 120):
        for now in (1.0, 1.025, 1.05, 1.051, 1.13, 1.3):
            for pattern in (0, 1):
                values.append((capacity, min(capacity, 2) if pattern else 0, 1, pattern, now, -8 if pattern else 0, 0x12345678 if pattern else 0, pattern, 0))
    # 꼬리 여유 없이 살아 있는 입자/죽은 슬롯/quiet NaN/무한대 수명을 명시 공급한다.
    for pattern in (1, 2, 3):
        for now in (1.0, 1.13, 1.3):
            values.append((12, 12, 1, pattern, now, 7, 0xdeadbeef, 1, 0))
    # 원본 진단2 분기와 signed 끝점 감김을 별도로 관찰한다.
    values.extend(((12, 0, 0, 0, 1.13, 7, 0, 1, 2), (12, 12, 1, 1, 1.13, 7, 0, 1, 2)))
    result = []  # 실제 포인터와 현재 필드의 입력 객체를 조립한다.
    # raw 배열의 미사용 필드도 sentinel로 채워 쓰기/보존을 확인한다.
    for index, (capacity, total, first, pattern, now, offset, seed, variant, diagnostic) in enumerate(values):
        raw = bytearray(b'\xa5' * 0x68)
        struct.pack_into('<3I4f2i', raw, 0, 0x509538, 1234, 50, 12.25, 13.75, 30.5, 35.5, 0, -25)
        struct.pack_into('<3I2d2f2I', raw, 0x24, capacity, total, PARTICLES, 1.0, .123456789, 123.25, -456.5, first, 0)
        struct.pack_into('<Ii2fi', raw, 0x50, PALETTE, 4, 9.75, -4.25, offset)
        particles = bytearray()  # 입력 용량 전체의24바이트를 보존한다.
        # 실제 꼬리 밖 입자도 active일 수 있으므로 용량 전체를 입력한다.
        for particle in range(capacity):
            life = 0.0 if not pattern else (4.0 if particle % 2 == 0 else .1) if pattern == 1 else (0.0 if particle % 2 == 0 else -1.0) if pattern == 2 else (float('nan'), float('-inf'), float('inf'), -0.0)[particle % 4]
            particles += struct.pack('<5fI', 25.125 + particle, 30.375 - particle, .125, -1.25, life, 0xcafe0000 + particle)
        parent = bytearray(50)
        struct.pack_into('<I', parent, 0, 0x501dd0)
        struct.pack_into('<H', parent, 6, 1)
        struct.pack_into('<2f', parent, 14, -7.75 if variant else 20.75, 21.9)
        state = struct.pack('<d5f3i', now, .75 if variant else 16.0, 1.25 if variant else 11.0, .3, .25, -.5, 0x7ffffffe if diagnostic else 100, -0x7ffffffe if diagnostic else 200, diagnostic)
        colors = [index * 3 + 17 for index in range(64)]  # 읽을 수 있는 인접 메모리도 명시 입력이며 실제 게임 할당 결과로 세지 않는다.
        colors[11] = 0xabcdef01
        palette = struct.pack('<i64I', 11, *colors)
        result.append((f'case{index}', bytes(raw), bytes(particles), bytes(parent), state, palette, seed))
    # 고유성은 포인터를 제거한 의미 필드·입자·부모·전역·색/난수 입력 전체로 판정한다.
    assert len({(semantic(row[1]), *row[2:]) for row in result}) == len(result)
    return result


class FieryOracle(Oracle):
    """실제 프레임/Draw 하위는 실행하고 dirty 소비와 선 래스터 장치만 요청 경계로 관찰한다."""
    def __init__(self):
        """현재 전체 정밀 함수표의 실제 범위만 허용한다. 기존 팔레트 Oracle의 호출 균형 검사만 재사용한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        # 입출력과 스택을 서로 겹치지 않게 확보한다.
        for address in (OBJECT, PARTICLES, POOL, PALETTE, COLORS):
            self.mu.mem_map(address, 0x10000)
        self.mu.mem_map(STACK - 0x7f000, 0x80000)
        self.mu.mem_map(STOP, 0x1000)
        self.allowed = []  # 무관한 전역 코드 영역은 허용하지 않는다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 함수의 실제 불연속 범위를 그대로 등록한다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.calls = Counter()  # 실제 몸체와 명시 출력 경계의 호출 수다.
        self.dirty, self.lines = [], []  # 매 입력의 출력 요청이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """실제 dirty/선 출력의 요청만 기록하며 계산/색/난수 하위는 대체하지 않는다."""
        if address in (0x499310, 0x4dade0):
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, *arguments = struct.unpack('<6I', mu.mem_read(esp, 24))
            target = self.dirty if address == 0x499310 else self.lines
            target.append(struct.pack('<5I', *arguments))
            if address == 0x499310:
                assert arguments[-1] == 0
            self.calls['dirty-boundary' if address == 0x499310 else 'line-boundary'] += 1
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 Fiery Tail 실행 {address:08x}')
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """실제 객체·입자·난수·색수·스택 외의 쓰기를 거부한다."""
        regions = ((OBJECT, OBJECT + 0x68), (PARTICLES, PARTICLES + self.capacity * 24), (0x532710, 0x532714), (PALETTE, PALETTE + 4), (STACK - 0x7f000, STACK + 0x1000))
        if not any(low <= address and address + size <= high for low, high in regions):
            raise RuntimeError(f'허용하지 않은 Fiery Tail 쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """실제 Frame 다음 Draw까지 호출하고 전체 의미 상태와 외부 인자를 보존한다."""
        name, raw, particles, parent, state, palette, seed = case
        mu = self.mu
        self.capacity = struct.unpack_from('<I', raw, 0x24)[0]
        mu.mem_write(OBJECT, raw)
        mu.mem_write(PARTICLES, particles or bytes(4))
        mu.mem_write(POOL + 50 * 50, parent)
        mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        mu.mem_write(PALETTE, palette[:4] + struct.pack('<I', COLORS))
        mu.mem_write(COLORS, palette[4:])
        now, scale_x, scale_y, rate, camera_x, camera_y, end_x, end_y, diagnostic = struct.unpack('<d5f3i', state)
        mu.mem_write(0x55b4d0, struct.pack('<d', now))
        mu.mem_write(0x59a924, struct.pack('<2f', scale_x, scale_y))
        mu.mem_write(0x59a934, struct.pack('<3f', rate, camera_x, camera_y))
        mu.mem_write(0x532710, struct.pack('<I', seed))
        mu.mem_write(0x5318ec, struct.pack('<i', diagnostic))
        self.dirty, self.lines = [], []
        self.call(0x4580f0, OBJECT, b'', 0, precision)
        self.call(0x4583a0, OBJECT, struct.pack('<2i', end_x, end_y), 8, precision)
        after = bytes(mu.mem_read(OBJECT, 0x68))
        # 원본 몸체가 쓸 수 없는 객체의 미사용 필드/포인터를 함께 확인한다.
        assert after[:0xc] == raw[:0xc] and after[0x1c:0x24] == raw[0x1c:0x24] and after[0x2c:0x30] == raw[0x2c:0x30] and after[0x4c:0x54] == raw[0x4c:0x54] and after[0x64:] == raw[0x64:]
        assert bytes(mu.mem_read(POOL + 2500, 50)) == parent
        assert bytes(mu.mem_read(COLORS, 256)) == palette[4:]
        return (semantic(after).hex(), bytes(mu.mem_read(PARTICLES, len(particles))).hex() or '-', (bytes(mu.mem_read(PALETTE, 4)) + palette[4:]).hex(), str(struct.unpack('<I', mu.mem_read(0x532710, 4))[0]), b''.join(self.dirty).hex() or '-', b''.join(self.lines).hex() or '-')


def main():
    """기본/비교 정밀도의 원본 기대값을 보존하거나 SHA와 입력 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 결과 재계산 없이 보호 PE/도구/기대값의 전체 바이트를 확인한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == report['uniqueInputs'] * 2 and all(len(row) == 14 for row in rows)
        assert len({tuple(row[1:8]) for row in rows}) == len(rows)
        print('Fiery Tail SHA/고유 입력 감사:', report['uniqueInputs'])
        return
    inputs = cases()
    if args.smoke:
        inputs = inputs[:1]
    oracle = FieryOracle()
    rows, different = [], []  # 같은 입력의 두 정밀도 결과를 구별한다.
    # 기대 필드는 Python으로 계산하지 않고 실제 원본 몸체에서 읽는다.
    for case in inputs:
        name, raw, particles, parent, state, palette, seed = case
        observations = []
        # 기본53비트와 비교64비트의 원문을 모두 보존한다.
        for precision in (0x027f, 0x037f):
            result = oracle.sample(case, precision)
            observations.append(result)
            rows.append((name, precision, semantic(raw).hex(), particles.hex() or '-', parent.hex(), state.hex(), palette.hex(), seed, *result))
        if observations[0] != observations[1]:
            different.append(name)
        print(name, '정상 반환', flush=True)
    if args.smoke:
        return
    FIXTURE.write_text('# name cw fields particles parent state palette seed fieldsAfter particlesAfter paletteAfter seedAfter dirtyRequests lineRequests\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_fierytail_frame.py', FIXTURE.relative_to(ROOT).as_posix())  # 재현에 필요한 전체 파일을 고정한다.
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(inputs), frameReturns=len(inputs)*2, drawReturns=len(inputs)*2, precisionDifferentInputs=different, nativeCalls=dict(sorted(oracle.calls.items())), sha256={path:digest(ROOT/path) for path in paths}, limitations=['실제 Frame/Draw·부모 조회·timer·난수·Append·영역·색·__ftol은 원본 몸체다.', 'dirty 큐 소비499310과 선 래스터4dade0은 명시 요청 관찰 경계다. 실제 픽셀·장치 검증이 아니다.', '기존 객체/입자/부모와 팔레트 인접 메모리는 명시 입력이다. 생성자·공통 등록/소멸·Session 연결·실제 게임 할당을 관찰한 것이 아니다.', '기본53비트027f를 C++ 대조 대상으로 삼고 비교037f도 보존한다. CRT 실제 기본023f와 연산 설정은 같다.', 'TYPE11 생성자의120개/수명4와 TYPE12 생성 자체를 이 프레임 입력 관찰로 완료 처리하지 않는다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('Fiery Tail:', len(inputs), '고유 입력, Frame/Draw 각각', len(inputs)*2, '정상 반환')


if __name__ == '__main__':
    main()
