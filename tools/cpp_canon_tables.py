#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 실행 파일의 Canondecoder 표(영역·다리 패턴, 회전별 방향 글자, 반복 방향)를 C++ 포함 파일로 만든다.

원본 exe 는 읽기만 한다. 영역/회전 표와 다리 모양을 CD판과 대조하며 다른 다리 가중치는 판본별로 보존한다.
C++ 빌드는 생성된 파일(cpppj/src/o/CanonDecoderTables.inc)만 쓰며 원본 exe 가 필요 없다.

python tools/cpp_canon_tables.py
"""
from pathlib import Path
import struct

import pefile

# 저장소 루트.
ROOT = Path(__file__).resolve().parent.parent
# 패치판(10.78) 실행 파일의 표 주소.
PATTERN_ADDRESS = 0x5300F0   # 영역 패턴: 72바이트 × 68개 (타입 DAT_00541204 = puzzlePiece 용)
# 다리의 가중치·모양 26개. CD판 0x514a80은 모양이 같고 가중치는 다르다.
BRIDGE_ADDRESS = 0x52F998
BRIDGE_COUNT = 26
STEP_ADDRESS = 0x531530      # 반복 방향 표: 4개짜리 정수 배열 6개
SIDE_ADDRESS = 0x531590      # 회전별 방향 글자 표: 16개짜리 정수 배열 4개
# 패턴 레코드: 정수 3개(첫 값, 폭, 높이) + 4바이트 셀 15개.
RECORD_SIZE = 72
CELL_COUNT = 15
# 영역 패턴 표 바로 뒤에는 섬 타입용 표(0x531410)가 온다.
PATTERN_COUNT = (0x531410 - PATTERN_ADDRESS) // RECORD_SIZE
# 생성할 파일.
OUTPUT = ROOT / 'cpppj/src/o/CanonDecoderTables.inc'


def main():
    """표를 읽어 CD판과 대조하고 포함 파일을 쓴다."""
    patch = pefile.PE(str(ROOT / 'originals/Netstorm.exe'), fast_load=True)
    base = patch.OPTIONAL_HEADER.ImageBase
    patterns = patch.get_data(PATTERN_ADDRESS - base, RECORD_SIZE * PATTERN_COUNT)
    bridges = patch.get_data(BRIDGE_ADDRESS - base, RECORD_SIZE * BRIDGE_COUNT)
    steps = patch.get_data(STEP_ADDRESS - base, 4 * 4 * 6)
    sides = patch.get_data(SIDE_ADDRESS - base, 4 * 16 * 4)
    cd_bytes = (ROOT / 'originalCD/NETSTORM.EXE').read_bytes()
    cd = pefile.PE(data=cd_bytes, fast_load=True)
    cd_bridges = cd.get_data(0x514A80 - cd.OPTIONAL_HEADER.ImageBase, RECORD_SIZE * BRIDGE_COUNT)
    # 판본 대조: 세 표가 CD판 실행 파일에도 그대로 있어야 한다.
    for name, table in (('패턴', patterns), ('반복 방향', steps), ('방향 글자', sides)):
        if cd_bytes.find(table) < 0:
            raise RuntimeError(f'CD판 실행 파일에 같은 {name} 표가 없음')
    # 다리 모양은 같지만 22~24번 가중치는 다르다. 각 판본 표를 별도로 생성한다.
    for index in range(BRIDGE_COUNT):
        if bridges[index*RECORD_SIZE+4:(index+1)*RECORD_SIZE] != cd_bridges[index*RECORD_SIZE+4:(index+1)*RECORD_SIZE]:
            raise RuntimeError(f'CD판 다리 모양 차이: {index}')
    lines = [
        '// 자동 생성: python tools/cpp_canon_tables.py — 직접 고치지 말 것.',
        f'// 출처: 패치판 Netstorm.exe 의 0x{PATTERN_ADDRESS:x}(영역 {PATTERN_COUNT}개), 0x{BRIDGE_ADDRESS:x}(다리 {BRIDGE_COUNT}개), 0x{STEP_ADDRESS:x}(반복 방향), 0x{SIDE_ADDRESS:x}(방향 글자).',
        '// 영역·회전 표는 CD판과 바이트가 같다. 다리 모양은 같고 가중치는 판본별로 보존한다.',
        '',
        '// 영역 패턴: {첫 값, 폭, 높이, 셀(변형 글자, 방향 글자, 셋째 바이트, 번호 글자) 15개}',
        f'constexpr std::array<CanonPattern, {PATTERN_COUNT}> kTerritoryPatterns{{{{',
    ]
    # 패턴마다 한 줄.
    for index in range(PATTERN_COUNT):
        record = patterns[index * RECORD_SIZE:(index + 1) * RECORD_SIZE]
        first, width, height = struct.unpack_from('<3i', record)
        cells = []
        # 셀 15개의 4바이트를 그대로 옮긴다.
        for cell in range(CELL_COUNT):
            cells.append('{' + ','.join(f'0x{b:02x}' for b in record[12 + cell * 4:16 + cell * 4]) + '}')
        lines.append(f'    {{{first}, {width}, {height}, {{{{{", ".join(cells)}}}}}}},  // {index}')
    lines.append('}};')
    lines.append('')
    # 다리도 영역과 같은 72바이트 레코드이며 두 판본을 독립적으로 출력한다.
    for name, table, address in [('kBridgePatterns', bridges, BRIDGE_ADDRESS), ('kCdBridgePatterns', cd_bridges, 0x514A80)]:
        lines.append(f'// 0x{address:x} 다리 패턴: 첫 값은 추첨 가중치, 셀 15개는 원본 바이트 그대로다.')
        lines.append(f'constexpr std::array<CanonPattern, {BRIDGE_COUNT}> {name}{{{{')
        # 각 판본의 26개 모양을 보존한다.
        for index in range(BRIDGE_COUNT):
            record = table[index * RECORD_SIZE:(index + 1) * RECORD_SIZE]
            first, width, height = struct.unpack_from('<3i', record)
            cells = []
            # 비어 있는 셀의 '.'과 사용하지 않는 후행 셀도 보존한다.
            for cell in range(CELL_COUNT):
                cells.append('{' + ','.join(f'0x{b:02x}' for b in record[12 + cell * 4:16 + cell * 4]) + '}')
            lines.append(f'    {{{first}, {width}, {height}, {{{{{", ".join(cells)}}}}}}},  // {index}')
        lines.append('}};')
        lines.append('')
    names = ['kOuterStepX', 'kOuterStepY', 'kInnerStepX', 'kInnerStepY', 'kStartX', 'kStartY']
    comments = ['바깥 반복(줄)이 끝날 때 패턴 x 에 더하는 값', '바깥 반복이 끝날 때 패턴 y 에 더하는 값',
                '안쪽 반복(칸)마다 패턴 x 에 더하는 값', '안쪽 반복마다 패턴 y 에 더하는 값',
                '시작 패턴 x = (폭 - 1) × 이 값', '시작 패턴 y = (높이 - 1) × 이 값']
    # 회전 0~3 에 대한 정수 4개짜리 표 여섯 개.
    for index, (name, comment) in enumerate(zip(names, comments)):
        values = struct.unpack_from('<4i', steps, index * 16)
        lines.append(f'// 0x{STEP_ADDRESS + index * 16:x}: {comment}')
        lines.append(f'constexpr std::array<int, 4> {name}{{{", ".join(map(str, values))}}};')
    lines.append('')
    lines.append(f'// 0x{SIDE_ADDRESS:x}: 회전 r 에서 패턴의 방향 글자 A~P 가 바뀌는 글자')
    lines.append('constexpr std::array<std::string_view, 4> kRotatedSides{')
    # 회전마다 글자 16개.
    for rotation in range(4):
        letters = ''.join(chr(value) for value in struct.unpack_from('<16i', sides, rotation * 64))
        lines.append(f'    "{letters}",')
    lines.append('};')
    OUTPUT.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    print(f'영역 {PATTERN_COUNT}개·다리 {BRIDGE_COUNT}개(두 판본)·회전 표 7개 → {OUTPUT.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
