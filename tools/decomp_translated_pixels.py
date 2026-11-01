#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004dab90→0040351d의 실제 대상 픽셀 변환을 게임 실행 없이 독립 관측한다(단위678)."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_scaled_pixels import Oracle as ScaledOracle
from decomp_graphics_oracle import synthetic_shapes, SCRATCH, SHAPE, CANVAS, ROOT
from decomp_gump_redraw import digest
from shp import read_blocks, TYPE_LOAD_ORDER
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

COLORS = SCRATCH+0x8000  # 실제 커널이 읽는256BYTE 표이며 가상 화면/압축 자료와 겹치지 않는다.


class Oracle(ScaledOracle):
    """기존 실제 RLE·좌표·배율 몸체에 translated 커널과 래퍼만 더 허용한다."""
    def __init__(self):
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for entry in (0x40351d, 0x4dab90):  # 실제 함수의 불연속 구간도 빠짐없이 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high+1))
        self.entries['TranslatedWrapper'] = 0x4dab90
        self.entries['TranslatedKernel'] = 0x40351d

    def pixels(self, case, control):
        """원문 표면 전체를 읽고 입력/양끝 guard/제어어의 보존을 확인한다."""
        mu = self.mu
        shape, colors, background = (bytes.fromhex(case[key]) for key in ('shape', 'colors', 'background'))
        width, height = case['size']
        mu.mem_write(0x5b5ddc, struct.pack('<Iiiii', SCRATCH+32, *case['pane']))
        mu.mem_write(SCRATCH+32, struct.pack('<Iiii', CANVAS, width-1, height-1, 1))
        mu.mem_write(SHAPE, shape)
        mu.mem_write(COLORS, colors)
        mu.mem_write(CANVAS-16, b'\xa5'*16+background+b'\xa5'*16)
        mu.reg_write(UC_X86_REG_FPCW, control)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        if case['scale'] == (65536, 65536):
            self.call('TranslatedWrapper', [SHAPE, 0, *case['position'], COLORS], purge=0)
        else:
            x, y = case['position']
            self.call('TranslatedKernel', [0x5b5ddc, SHAPE, 0, x-case['pane'][0], y-case['pane'][1], 0x5caff0, 0, *case['scale'], 0, COLORS], purge=44)
        assert bytes(mu.mem_read(CANVAS-16, 16))+bytes(mu.mem_read(CANVAS+width*height, 16)) == b'\xa5'*32
        assert bytes(mu.mem_read(SHAPE, len(shape))) == shape and bytes(mu.mem_read(COLORS, 256)) == colors
        assert mu.reg_read(UC_X86_REG_FPCW) == control and not mu.reg_read(UC_X86_REG_FPSW) & 0x3800
        return bytes(mu.mem_read(CANVAS, width*height)).hex()


def plan():
    """소스0/255·모든RLE·한행·clip·배경·색표·단위/분수 배율의 명시 입력만 만든다."""
    maps = (bytes(range(256)), bytes((index*37+5) & 255 for index in range(256)))
    cases = []
    for shape, scale, pane, position, colors in itertools.product(itertools.islice(synthetic_shapes(), 12),
            ((65536, 65536), (32768, 32768), (98304, 98304)), ((0, 0, 31, 23), (3, 2, 27, 20)), ((12, 10), (-1, 1)), maps):
        cases.append({'shape': shape.hex(), 'size': (32, 24), 'pane': pane, 'position': position, 'scale': scale,
                      'colors': colors.hex(), 'background': bytes((index*17+11) & 255 for index in range(32*24)).hex()})
    data = (ROOT/'originals/d/_shapes.shp').read_bytes()
    blocks = read_blocks(data)
    offsets = sorted({frame.offset for block in blocks for frame in block.frames})
    ends = dict(zip(offsets, offsets[1:]+[len(data)]))
    for name in ('geyser', 'emptyGeyser'):  # 실제 간헐천 두 TYPE의 본체/마지막 그림자 프레임도 고정한다.
        frames = blocks[TYPE_LOAD_ORDER.index(name)].frames
        for frame in (frames[0], frames[-1]):
            assert not frame.special
            shape = struct.pack('<IIII', 0x30312e31, 1, 16, 0)+data[frame.offset:ends[frame.offset]]
            for colors in maps:
                cases.append({'shape': shape.hex(), 'size': (256, 192), 'pane': (0, 0, 255, 191), 'position': (128, 96),
                              'scale': (65536, 65536), 'colors': colors.hex(), 'background': bytes((index*17+11) & 255 for index in range(256*192)).hex()})
    return cases


def main():
    """현재 호스트의 원문을 두정밀도/재관측으로 동결하며 복원 측 코드는 읽지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    cases, oracle = plan(), Oracle()
    first = [oracle.pixels(case, 0x27f) for case in cases]
    assert first == [oracle.pixels(case, 0x27f) for case in cases] == [oracle.pixels(case, 0x37f) for case in cases]
    fixture = out/'translated-pixels-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# id\tshape\tsize\tpane\tposition\tscale\tcolors\tbackground\tpixels\n')
        for index, (case, pixels) in enumerate(zip(cases, first)):  # 기대값은 실제 원문 표면 바이트만 쓴다.
            fields = (str(index), case['shape'], ','.join(map(str, case['size'])), ','.join(str(v & 0xffffffff) for v in case['pane']),
                      ','.join(str(v & 0xffffffff) for v in case['position']), ','.join(map(str, case['scale'])), case['colors'], case['background'], pixels)
            stream.write('\t'.join(fields)+'\n')
    report = {'cases': len(cases), 'observations': len(cases)*3, 'differences': 0, 'host': socket.gethostname(),
              'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23',
              'sourceSha256': digest(ROOT/'originals/Netstorm.exe'), 'toolSha256': digest(Path(__file__)), 'fixtureSha256': digest(fixture),
              'scope': '실제004dab90/0040351d·압축 해제·가상 대상 전체 픽셀. 출력/CRT/OS 대체 없음. 게임/공중3종/MP/금지재현 없음.'}
    (out/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
