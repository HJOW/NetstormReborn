#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 VFX 타원과 퇴화 선의 전체 픽셀을 격리 PE에서 관찰한다. 게임/창 실행은 없다."""
import argparse
import csv
import json
import struct
from decomp_fierytail_pixels import PixelOracle, ROOT, CANVAS, SURFACE, WIDTH, HEIGHT

FIXTURE = ROOT / 'cpppj/tests/fixtures/vfx-ellipse-x86.tsv'  # 새 독립 정수 입력과 실제 전체 화면 바이트다.


class EllipseOracle(PixelOracle):
    """실제32비트 세그먼트/픽셀 보호를 유지하며 타원 함수 범위만 추가한다."""

    def __init__(self):
        """현재 정밀 함수의 전체 범위를 추가하여 중간 오류 반환 뒤 몸체도 실행한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row = next(value for value in csv.DictReader(stream, delimiter='\t') if int(value['entry'], 16) == 0x405071)
        # 첫 RET에서 자르지 않고 실제347명령의 단일 함수 범위를 허용한다.
        for part in row['ranges'].split(';'):
            low, high = (int(value, 16) for value in part.split('-'))
            self.allowed.append((low, high + 1))

    def observe(self, values):
        """원문 pane 원점/clip과 sentinel 화면을 준비하고 실제 픽셀 결과만 읽는다."""
        left, top, right, bottom, x, y, rx, ry, color = values
        self.capacity = 0
        self.mu.mem_write(0x5b5ddc, struct.pack('<I4i', SURFACE, left, top, right - 1, bottom - 1))
        self.mu.mem_write(SURFACE, struct.pack('<Iiii', CANVAS, WIDTH - 1, HEIGHT - 1, 1))
        self.mu.mem_write(CANVAS - 32, bytes([0x7d]) * (WIDTH * HEIGHT + 64))
        self.mu.mem_write(CANVAS, bytes([199]) * (WIDTH * HEIGHT))
        self.call(0x405071, 0, struct.pack('<I4iI', 0x5b5ddc, x - left, y - top, rx, ry, color), 24, 0x027f)
        assert bytes(self.mu.mem_read(CANVAS - 32, 32)) == bytes([0x7d]) * 32
        assert bytes(self.mu.mem_read(CANVAS + WIDTH * HEIGHT, 32)) == bytes([0x7d]) * 32
        return bytes(self.mu.mem_read(CANVAS, WIDTH * HEIGHT)).hex()


def main():
    """퇴화/음수/두 축/clip 경계의 정수 입력을 교차하여 새 파일을 생성하거나 재관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle, rows = EllipseOracle(), []  # 실제 VFX 함수는 정수 연산이므로 제어워드027f 한 번만 실행한다.
    panes = ((0, 0, 64, 48), (7, 9, 51, 37), (-3, -5, 35, 29), (20, 10, 21, 11), (63, 47, 65, 49), (100, 100, 110, 110))  # 반개방 외부 clip 입력이다.
    centers = ((32, 24), (0, 0), (63, 47), (-3, 5), (70, -8))  # 안/바깥/모서리 절대 중심이다.
    radii = ((-1, -2), (0, -1), (1, 0), (2, 1), (3, 2), (8, 7), (3, 6), (6, 3), (0, 0))  # 실제 EvilRay 음수/퇴화 및 두 구간의 다른 축을 포함한다.
    # 현재 입력만 생성하며 기대 타원 좌표/픽셀은 계산하지 않는다.
    for pane in panes:
        for center in centers:
            for radius in radii:
                color = (0xabcdef01, 0, 255)[len(rows) % 3]
                values = (*pane, *center, *radius, color)
                rows.append((','.join(map(str, values)), oracle.observe(values)))
    content = '# clip center radii color, nativePixels; 64x48/배경199/실제 원문 정수 VFX/게임0\n' + '\n'.join('\t'.join(row) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify로 재관측한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
