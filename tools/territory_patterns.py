# -*- coding: utf-8 -*-
"""원본 Canondecoder 영역 패턴을 추출한다. python tools/territory_patterns.py"""
import json
from pathlib import Path
import struct

import pefile

# 보유 패치판 실행 파일의 영역 패턴 배열 가상 주소.
PATTERN_ADDRESS = 0x5300F0
# Territory 첫 바이트의 하위 6비트로 선택할 수 있는 패턴 수.
PATTERN_COUNT = 64
# 패턴은 헤더 12바이트와 셀 최대 15개의 4바이트 표현으로 구성된다.
RECORD_SIZE = 72


def main():
    """패턴의 유효 셀과 빈 셀을 보존하여 런타임 내장 JSON으로 저장한다."""
    root = Path(__file__).resolve().parent.parent
    pe = pefile.PE(str(root / 'originals/Netstorm.exe'))
    data = pe.get_data(PATTERN_ADDRESS - pe.OPTIONAL_HEADER.ImageBase,
                       PATTERN_COUNT * RECORD_SIZE)
    patterns = []
    # 각 레코드의 행 우선 셀을 문자 하나로 축약한다.
    for index in range(PATTERN_COUNT):
        record = data[index * RECORD_SIZE:(index + 1) * RECORD_SIZE]
        _, width, height = struct.unpack_from('<3i', record)
        if not (0 < width * height <= 15):
            raise ValueError(f'잘못된 패턴 크기: {index}, {width}, {height}')
        cells = ''.join(chr(record[13 + cell * 4]) for cell in range(width * height))
        if any(cell != '.' and not 'A' <= cell <= 'P' for cell in cells):
            raise ValueError(f'잘못된 패턴 셀: {index}')
        patterns.append({'Width': width, 'Height': height, 'Cells': cells})
    target = root / 'src/Netstorm.Assets/TerritoryPatterns.json'
    target.write_text(json.dumps(patterns, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'{len(patterns)}개 패턴 저장: {target}')


if __name__ == '__main__':
    main()
