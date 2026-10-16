#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Fiery Tail Draw→화면 wrapper→VFX 선을 대체 없이 실행해 전체 픽셀을 보존한다."""
import argparse
import csv
import json
import socket
import struct
from decomp_fierytail_frame import FieryOracle, ROOT, OBJECT, PARTICLES, PALETTE, COLORS, ENTRIES
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_GDTR, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES

CANVAS, SURFACE, GDT = 0x12500100, 0x12600000, 0x40000000  # 픽셀·표면과32비트 평면 세그먼트의 격리 주소다.
WIDTH, HEIGHT = 64, 48  # 실제 창을 만들지 않는 작은8비트 표면의 명시 입력 크기다.
PIXEL_ENTRIES = {0x4dade0, 0x4011f1}  # 기존 요청 관찰 경계 대신 실행할 실제 wrapper·VFX 몸체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fierytail-pixels-x86.tsv'  # 입력과 실제 전체 화면 바이트를 보존한다.
REPORT = ROOT / 'cpppj/fierytail-pixels.json'  # 실제 호출 수·정밀도·근거 SHA다.


def cases():
    """다양한 방향/점/수명/clip/카메라를 공급하고 출력 픽셀은 계산하지 않는다."""
    panes = ((0, 0, 64, 48), (7, 9, 51, 37), (-3, -5, 35, 29), (20, 10, 21, 11), (63, 47, 65, 49))
    ends = ((-20, 8), (10, 80), (30, 24), (80, -18), (12, 12), (12, 12))
    points = ((12, 12), (12, 30), (35, 12), (-13.5, 12), (12, -20), (80, 80),
              (60.125, 42.375), (3.25, 8.5), (25.5, 25.5), (30, 24), (0, 47), (63, 0))
    colors = struct.pack('<12I', 217, 226, 62, 56, 51, 200, 221, 218, 218, 197, 196, 0xabcdef01)
    # 같은 입력을 두 번 센 결과가 되지 않도록 pane와 현재 끝점/카메라/offset을 모두 기록한다.
    for pane_index, pane in enumerate(panes):
        # 마지막 두 끝점이 같아도 카메라와offset 입력은 다르다.
        for variant, end in enumerate(ends):
            camera = (0.0, 0.0) if variant == 4 else (.25 * variant, -.5 * variant)
            offset = (variant % 3 - 1) * 3 if variant != 4 else 0
            particles = b''.join(struct.pack('<5fI', x, y, .125, -1.25, (4, 3.75, 2.9, 2.1, 1.2, .1, -1, 0)[index % 8], index)
                                 for index, (x, y) in enumerate(points))
            yield f'pane{pane_index}-end{variant}', (*pane, *end, *camera, offset), particles, colors


class PixelOracle(FieryOracle):
    """Draw 하위 전체를 실제 코드로 실행하며 출력 메모리/스택 밖의 쓰기는 거부한다."""
    def __init__(self):
        """실제 VFX의 push/pop ES에도 스택 폭과 평면 주소를 유지한다."""
        super().__init__()
        self.mu.mem_map(CANVAS & ~4095, 0x10000)
        self.mu.mem_map(SURFACE, 0x1000)
        self.mu.mem_map(GDT, 0x1000)
        self.mu.mem_write(GDT, struct.pack('<QQQ', 0, 0x00cf9a000000ffff, 0x00cf92000000ffff))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 23, 0))
        self.mu.reg_write(UC_X86_REG_CS, 8)
        # 데이터/스택/문자열 명령 모두 같은32비트 평면을 사용한다.
        for register in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES):
            self.mu.reg_write(register, 16)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 새로 실행하는 두 함수의 실제 정밀 범위만 추가한다.
        for entry in PIXEL_ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """원본 선 요청을 가로채거나 픽셀/색/좌표를 호스트로 대신 계산하지 않는다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 Fiery Tail 픽셀 실행 {address:08x}')
        if address in ENTRIES | PIXEL_ENTRIES:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """실제 화면 바이트만 추가로 허용하고 바깥 보호 바이트는 쓰기부터 거부한다."""
        if CANVAS <= address and address + size <= CANVAS + WIDTH * HEIGHT:
            return
        super().write(mu, access, address, size, value, data)

    def draw(self, case, precision):
        """원본 pane의 반개방 변환·wrapper 원점까지 설정한 뒤 실제 Draw 한 번을 호출한다."""
        name, values, particles, colors = case
        left, top, right, bottom, end_x, end_y, camera_x, camera_y, offset = values
        raw = bytearray(0x68)
        struct.pack_into('<3I', raw, 0x24, 12, 12, PARTICLES)
        struct.pack_into('<Iii', raw, 0x50, PALETTE, 4, 0)
        struct.pack_into('<i', raw, 0x60, offset)
        self.capacity = 12
        self.mu.mem_write(OBJECT, bytes(raw))
        self.mu.mem_write(PARTICLES, particles)
        self.mu.mem_write(PALETTE, struct.pack('<iI', 11, COLORS))
        self.mu.mem_write(COLORS, colors)
        self.mu.mem_write(0x59a938, struct.pack('<2f', camera_x, camera_y))
        self.mu.mem_write(0x5318ec, bytes(4))
        self.mu.mem_write(0x5b5ddc, struct.pack('<I4i', SURFACE, left, top, right - 1, bottom - 1))
        self.mu.mem_write(SURFACE, struct.pack('<Iiii', CANVAS, WIDTH - 1, HEIGHT - 1, 1))
        self.mu.mem_write(CANVAS - 32, bytes([0x7d]) * (WIDTH * HEIGHT + 64))
        self.mu.mem_write(CANVAS, bytes([199]) * (WIDTH * HEIGHT))
        self.call(0x4583a0, OBJECT, struct.pack('<2i', end_x, end_y), 8, precision)
        assert bytes(self.mu.mem_read(OBJECT, len(raw))) == raw
        assert bytes(self.mu.mem_read(PARTICLES, len(particles))) == particles
        assert bytes(self.mu.mem_read(COLORS, len(colors))) == colors
        assert bytes(self.mu.mem_read(PALETTE, 8)) == struct.pack('<iI', 11, COLORS)
        assert bytes(self.mu.mem_read(CANVAS - 32, 32)) == bytes([0x7d]) * 32
        assert bytes(self.mu.mem_read(CANVAS + WIDTH * HEIGHT, 32)) == bytes([0x7d]) * 32
        return bytes(self.mu.mem_read(CANVAS, WIDTH * HEIGHT))


def main():
    """실제 전체 픽셀을 생성하거나 보호 원본/입력/결과의 SHA와 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 전체 파일 바이트의 핀을 확인하며 기대 픽셀은 다시 계산하지 않는다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == report['uniqueInputs'] * 2
        assert len({tuple(row[1:5]) for row in rows}) == len(rows)
        assert all(len(row) == 6 and len(bytes.fromhex(row[5])) == WIDTH * HEIGHT for row in rows)
        print('Fiery Tail 픽셀 SHA/고유 입력 감사:', report['uniqueInputs'])
        return
    inputs = list(cases())
    assert len({(case[1], *case[2:]) for case in inputs}) == len(inputs)
    oracle = PixelOracle()
    rows, different = [], []  # 정밀도별 실제 원문을 모두 남긴다.
    # 픽셀 기대값은 Python 래스터 모델을 쓰지 않고 원본 표면에서 읽는다.
    for case in inputs:
        name, values, particles, colors = case
        observations = []
        # 기본53비트·비교64비트의 정상 반환과 전체 화면 차이를 확인한다.
        for precision in (0x027f, 0x037f):
            pixels = oracle.draw(case, precision)
            observations.append(pixels)
            rows.append((name, precision, ','.join(map(str, values)), particles.hex(), colors.hex(), pixels.hex()))
        if observations[0] != observations[1]:
            different.append(name)
        print(name, '실제 Draw/VFX 정상 반환', flush=True)
    FIXTURE.write_text('# name cw clip,end,camera,offset particles palette pixels\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_fierytail_frame.py', 'tools/decomp_fierytail_pixels.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(inputs), drawReturns=2 * len(inputs), precisionDifferentInputs=different, nativeCalls=dict(sorted(oracle.calls.items())), sha256={path: digest(ROOT / path) for path in paths}, limitations=['실제004583a0→004dade0→004011f1 전체와 색 조회/__ftol을 대체 없이 실행했다. 정상 반환/비휘발/x87 균형과 쓰기 범위를 검사했다.', '입자/색/수명/인접DWORD는 명시 입력이다. 0xabcdef01은 실제 게임 heap 관찰값이 아니다.', '장치 대신 격리64×48 메모리 표면이다. 전체 Session/생성/프레임/장치·사용자 플레이 완주를 이 관찰로 입증하지 않는다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('Fiery Tail 실제 픽셀:', len(inputs), '고유 입력, Draw', len(inputs) * 2, '정상 반환')


if __name__ == '__main__':
    main()
