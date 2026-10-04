#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""VFX 그리기 두 함수만 x86으로 실행하여 픽셀·클리핑 기대값을 만든다.

원본 게임 프로세스·OS API를 시작하지 않는다. CRT·assert 대체도 사용하지 않는다.
의존성: tools/requirements-decomp-oracle.txt (extracted/oracle-python에 격리 설치).
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import (UC_X86_REG_GDTR, UC_X86_REG_CS, UC_X86_REG_SS,
                              UC_X86_REG_DS, UC_X86_REG_ES)
from shp import read_blocks, TYPE_LOAD_ORDER

# 호출 규약·호출 관계·실제 RLE 처리를 검토한 두 함수. 둘 다 ret 20이다.
PAIRS = [
    {'name': 'ShapeDraw', 'originals': '00401d92', 'originalCD': '00465772'},
    {'name': 'ShapeDrawUnclipped', 'originals': '004021b5', 'originalCD': '00465b95'},
]
# 재현 가능한 RLE 입력 생성 시드.
INPUT_SEED = 0x56465810
# 원본 문자열 복사 명령의 ES·DS·SS가 쓰는 32비트 flat GDT 가상 주소.
GDT = 0x40000000
# pane·surface와 겹치지 않는 가상 셰이프 주소.
SHAPE = SCRATCH + 0x10000
# 화면 출력의 가상 주소. 호스트의 화면이나 창을 가리키지 않는다.
CANVAS = SCRATCH + 0x100000


class GraphicsOracle(Oracle):
    """정확히 두 VFX 함수만 허용하고 세그먼트 복사 환경을 구성한다."""
    def __init__(self, edition):
        """원본 PE는 읽기만 하고 가상 코드·화면·32비트 세그먼트를 준비한다."""
        super().__init__(edition, PAIRS, helpers=[])
        self.stubs = {}
        self.mu.mem_map(SCRATCH+0x20000, 0x200000)
        self.mu.mem_map(GDT, 0x1000)
        self.mu.mem_write(GDT, struct.pack('<QQQ', 0, 0x00cf9a000000ffff, 0x00cf92000000ffff))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 23, 0))
        self.mu.reg_write(UC_X86_REG_CS, 8)
        # push/pop ES 후에도 스택이 16비트 SP로 잘리지 않게 flat data descriptor를 지정한다.
        for register in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES):
            self.mu.reg_write(register, 16)

    def draw(self, shape, width, height, pane, x, y, background):
        """원본 pane·surface 배치로 호출하고 반환 코드·화면 전체 바이트를 읽는다."""
        size = width*height if width > 0 and height > 0 else 0
        if size > 0x100000 or len(shape) > CANVAS-SHAPE:
            raise RuntimeError('그래픽 oracle 입력 범위 초과')
        self.mu.mem_write(SCRATCH, struct.pack('<Iiiii', SCRATCH+32, *pane))
        self.mu.mem_write(SCRATCH+32, struct.pack('<Iiii', CANVAS, width-1, height-1, 1))
        self.mu.mem_write(SHAPE, shape)
        if size:
            self.mu.mem_write(CANVAS, bytes([background])*size)
        value = self.call('ShapeDraw', [SCRATCH, SHAPE, 0, x, y], purge=20)
        result = value if value < 0x80000000 else value-0x100000000
        return result, bytes(self.mu.mem_read(CANVAS, size)) if size else b''


def shape_bytes(width, height, rect, runs):
    """원본 VFX 문법의 블록 하나·프레임 하나를 구성한다. 출력 픽셀 모델은 만들지 않는다."""
    return struct.pack('<IIIIHHHHiiii', 0x30312e31, 1, 16, 0,
                       height, width, 0, 0, *rect)+runs


def synthetic_shapes():
    """색 0·255·모든 run 종류·행 끝·공유되지 않은 작은 프레임을 생성한다."""
    rng = random.Random(INPUT_SEED)
    # 원본에서 출력할 픽셀은 계산하지 않고 압축 입력만 만든다.
    for _ in range(64):
        width, height = rng.randrange(1, 18), rng.randrange(1, 8)
        runs = bytearray()
        # 매 행에 크기가 다른 투명·직접·반복 run을 배치한다.
        for _row in range(height):
            column = 0
            # 일부 행은 끝의 투명 구간을 생략해 원본의 행 끝 처리도 검사한다.
            while column < width and rng.random() > 0.08:
                count = rng.randrange(1, min(width-column, 127)+1)
                kind = rng.randrange(3)
                if kind == 0:
                    runs.extend([1, count])
                elif kind == 1:
                    runs.append((count << 1)|1)
                    runs.extend(rng.choice([0, 255, rng.randrange(256)]) for _pixel in range(count))
                else:
                    runs.extend([count << 1, rng.choice([0, 255, rng.randrange(256)])])
                column += count
            runs.append(0)
        left, top = rng.randrange(-5, 3), rng.randrange(-3, 2)
        yield shape_bytes(width, height, (left, top, left+width-1, top+height-1), runs)


def asset_shapes():
    """실제 두 판본에서 대표 타입의 압축 프레임을 읽기만 하여 oracle 입력으로 쓴다."""
    selected = ['dude', 'sunCannon', 'rainCannon', 'priest', 'altar', 'sunFactory',
                'bridge', 'island', 'mana', 'rainBalloon', 'sunWalker', 'manabolt']
    # CD판도 공통 앞 101개의 정확한 타입 순서를 사용한다.
    for edition in ('originals', 'originalCD'):
        data = (ROOT / edition / 'd/_shapes.shp').read_bytes()
        blocks = read_blocks(data)
        offsets = sorted({frame.offset for block in blocks for frame in block.frames})
        ends = dict(zip(offsets, offsets[1:]+[len(data)]))
        # 특수 레코드는 이미지가 아니므로 픽셀 함수의 입력에서 제외한다.
        for name in selected:
            block = blocks[TYPE_LOAD_ORDER.index(name)]
            frame = next(frame for frame in block.frames if not frame.special and frame.width*frame.height <= 20000)
            yield struct.pack('<IIII', 0x30312e31, 1, 16, 0)+data[frame.offset:ends[frame.offset]]


def inputs():
    """프레임 전체·좌상단·우하단 클리핑과 빈 pane을 두 배경 색으로 검사한다."""
    # 투명 영역을 칠해 버리는 오류가 색 0 또는 255에서 가려지지 않게 두 번 검사한다.
    for shape in [*synthetic_shapes(), *asset_shapes()]:
        left, top, right, bottom = struct.unpack_from('<iiii', shape, 24)
        width, height = right-left+9, bottom-top+9
        placements = [(0, 0, width-1, height-1, 4-left, 4-top),
                      (0, 0, width-1, height-1, -left-2, -top-1),
                      (-2, 1, width-3, height-2, width-left-2, height-top-2),
                      (5, 4, 3, 2, 0, 0)]
        # pane 원점 이동을 포함한 각 배치를 실제 x86으로 계산한다.
        for pane_left, pane_top, pane_right, pane_bottom, x, y in placements:
            # 색 0과 255는 모두 정상 불투명 픽셀이 될 수 있다.
            for background in (0, 255):
                yield shape, width, height, (pane_left, pane_top, pane_right, pane_bottom), x, y, background
    normal = shape_bytes(1, 1, (0, 0, 0, 0), b'\x02\x00\x00')
    # 화면·완전 화면 밖·역전된 프레임의 원본 반환 코드도 기록한다.
    for width, height, pane, x, y in [(0, 4, (0, 0, 4, 4), 0, 0), (4, 0, (0, 0, 4, 4), 0, 0),
                                      (4, 4, (0, 0, 3, 3), 8, 8), (4, 4, (0, 0, 3, 3), -8, -8)]:
        yield normal, width, height, pane, x, y, 93
    yield shape_bytes(1, 1, (1, 0, 0, 0), b''), 4, 4, (0, 0, 3, 3), 0, 0, 93


def main():
    """두 판본의 기계어 결과가 같을 때만 고정 기대값과 출처 해시를 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'cpppj/tests/fixtures/graphics-x86.tsv')
    parser.add_argument('--report', type=Path, default=ROOT / 'cpppj/recovery-graphics-evidence.json')
    args = parser.parse_args()
    patch, cd = GraphicsOracle('originals'), GraphicsOracle('originalCD')
    rows = ['# shape-hex width height pane-left top right bottom x y background return canvas-hex']
    # 결과 모델·렌더러 재구현은 사용하지 않고 원본 출력만 저장한다.
    for case, values in enumerate(inputs(), 1):
        shape, width, height, pane, x, y, background = values
        a, b = patch.draw(*values), cd.draw(*values)
        if a != b:
            raise RuntimeError(f'VFX 두 판본 불일치: case {case}')
        result, pixels = a
        rows.append('\t'.join([shape.hex(), str(width), str(height), *map(str, pane), str(x), str(y),
                               str(background), str(result), pixels.hex() or '-']))
        if case % 128 == 0:
            print(f'VFX x86 대조 {case}개 완료', flush=True)
    payload = '\n'.join(rows)+'\n'
    report = {'schema': 1, 'method': 'Unicorn x86 32-bit flat segments; exactly two VFX functions; no stubs',
              'binary_sha256': {'originals': patch.sha256, 'originalCD': cd.sha256},
              'functions': PAIRS, 'seed': INPUT_SEED, 'total_cases': len(rows)-1,
              'fixture_sha256': hashlib.sha256(payload.encode('utf-8')).hexdigest(),
              'limitations': ['기본 8비트 그리기·클리핑만 검증', '색 변환·확대·반전·그림자 효과는 후속 복원',
                             '타입 파서 전체를 x86으로 실행한 검증은 아님', '게임 프로세스·OS API는 실행하지 않음']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(payload, encoding='utf-8', newline='\n')
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(f'두 판본 VFX 동일 결과 {len(rows)-1}개 → {args.output}')


if __name__ == '__main__':
    main()
