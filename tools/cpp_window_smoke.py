#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 C++ 영역 배치와 Win32 창(화면 장치)을 실제 두 판본의 파일로 검사한다. 원본 게임은 실행하지 않는다.

1. 영역 배치: 모든 `.fort`의 `Territory` 레코드를 C++(CanonDecoder·ChunkMap·Islandbuilder)로 놓은 결과를,
   기존 패턴 자료(dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json)로 Python에서 따로 계산한 결과와 줄 단위로 비교한다.
   영역마다 놓인 청크 수가 `TerrNN` 섹션의 청크 레코드 수와 같은지도 확인한다.
2. 창: 새 실행 파일(NetstormCpp --run)을 몇 프레임만 띄워 화면 버퍼를 BMP로 받고,
   - 타입 표 화면을 Python 해독기(tools/shp.py)와 팔레트 파일로 따로 그린 그림과 픽셀 단위로 비교한다
     (8비트 DIB 섹션 → 팔레트 적용 → VFX 그리기·자르기의 전 구간).
   - 미션 화면은 크기, 팔레트에 있는 색만 쓰였는지, 그려진 픽셀 수를 확인한다.
3. 원본 파일이 바뀌지 않았는지 확인한다.

창이 잠깐 화면에 뜬다(판본마다 1초 안팎). 바탕 화면이 없는 환경에서는 --no-window 로 1번만 실행한다.

python tools/cpp_window_smoke.py
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from PIL import Image

import cpp_fort_smoke as fort_smoke
import fort
from shp import TYPE_LOAD_ORDER, decode_frame, read_blocks
from taff import TaffArchive
from typefile import parse_type

# C++ 빌드와 보존된 원본 자산의 공통 루트.
ROOT = Path(__file__).resolve().parent.parent
# 검사 산출물을 두는 Git 제외 폴더.
OUTPUT = ROOT / 'extracted/cpp-window-smoke'
# 독립 대조에 쓰는 영역 패턴 자료(tools/territory_patterns.py가 실행 파일에서 뽑은 것).
PATTERNS = ROOT / 'dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json'
# 청크 지도의 한 변, 영역 수, 레코드 크기, 영역을 놓는 범위의 원점(원본 FUN_00431fc0의 (1,1)).
MAP_SIDE = 16
TERRITORY_COUNT = 20
RECORD_BYTES = 6
ORIGIN = 1
# `.fort` 섹션 번호: Territory, 첫 TerrNN.
SECTION_TERRITORY = 6
SECTION_TERR00 = 15
# 창 검사에서 그리는 프레임 수. 첫 프레임부터 화면 전체를 그리므로 적어도 된다.
FRAMES = 5
# 타입 표 화면의 칸 크기(cpppj/src/app/InspectView.cpp와 같은 값).
TYPE_CELL_WIDTH = 64
TYPE_CELL_HEIGHT = 96
# 판본별 창 검사: (판본 폴더, 판본 인자, 명령줄 설정, 미션). CD 폴더는 설치본이 아니라 options.cfg가 없어 화면 크기를 준다.
WINDOW_CASES = [
    ('originals', [], '', 'TEST01'),
    ('originalCD', ['--cd'], 'SCREENW=800;SCREENH=600', 'thewarbegins'),
]
# 미션 화면에서 바탕이 아닌 픽셀이 이 수보다 많아야 한다(오브젝트가 실제로 그려졌는지).
MIN_DRAWN_PIXELS = 5000


def truncate_half(value):
    """C++ 정수 나눗셈(0 쪽으로 버림)과 같은 2로 나누기."""
    return int(value / 2)


def expected_territories(name, data, catalog, patterns):
    """`.fort` 하나의 영역 배치를 Python으로 계산해 C++ `--dump-territories`와 같은 글로 적는다."""
    lines = [f'FORT {name}']
    sections, _ = fort_smoke.split_sections(data)
    territory = sections[SECTION_TERRITORY] if len(sections) > SECTION_TERRITORY else b''
    if len(territory) < TERRITORY_COUNT * RECORD_BYTES:
        return lines, 0
    typenames = sections[7] if len(sections) > 7 else b''
    conversion = catalog.conversion(typenames)
    owner_of = {}   # (x, y) → 영역 번호
    side_of = {}    # (x, y) → 방향 글자
    # 레코드를 번호순으로 놓는다. 나중에 놓은 영역이 겹친 청크를 차지한다.
    for index in range(TERRITORY_COUNT):
        shape_byte, flags, position = territory[index * RECORD_BYTES:index * RECORD_BYTES + 3]
        if not (flags & 1) or (flags & 2):
            continue
        if shape_byte >> 6:
            raise AssertionError(f'{name}: 회전된 영역(방향 {shape_byte >> 6})은 독립 계산이 다루지 않는다')
        pattern = patterns[shape_byte & 0x3f]
        # 패턴 칸을 줄 우선으로 훑는다. '.'은 빈 칸이다.
        for cell, side in enumerate(pattern['Cells']):
            if side == '.':
                continue
            x = min(ORIGIN + (position & 0xf) + cell % pattern['Width'], MAP_SIDE - 1)
            y = min(ORIGIN + (position >> 4) + cell // pattern['Width'], MAP_SIDE - 1)
            owner_of[(x, y)] = index
            side_of[(x, y)] = side
    placed = 0
    # 영역마다 한 줄: 레코드 값, 저장된 청크 레코드 수, 놓인 청크(y 바깥, x 안쪽).
    for index in range(TERRITORY_COUNT):
        section = sections[SECTION_TERR00 + index] if len(sections) > SECTION_TERR00 + index else b''
        stored = len(fort.read_chunks(section, conversion, catalog)[1]) if len(section) > 1 else 0
        chunks = [(x, y) for y in range(MAP_SIDE) for x in range(MAP_SIDE) if owner_of.get((x, y)) == index]
        if not chunks and len(section) <= 1:
            continue
        shape_byte, flags, position = territory[index * RECORD_BYTES:index * RECORD_BYTES + 3]
        if chunks and len(chunks) != stored:
            raise AssertionError(f'{name} 영역 {index}: 놓인 청크 {len(chunks)}개, 저장된 청크 레코드 {stored}개')
        placed += bool(chunks)
        lines.append(f'T {index} shape={shape_byte & 0x3f} dir={shape_byte >> 6} flags={flags} pos={position & 0xf},{position >> 4} '
                     f'stored={stored} chunks=' + ''.join(f'{x},{y}{side_of[(x, y)]};' for x, y in chunks))
    return lines, placed


def check_territories(executable, edition, data_dir, archive_name, flag, type_count, patterns):
    """판본의 모든 `.fort`에서 영역 배치를 비교한다."""
    root = ROOT / edition
    catalog = fort_smoke.Catalog(root, data_dir, type_count)
    expected = []
    files = territories = 0
    # 파일마다 Python 계산 결과를 모은다(C++ 명령과 같은 순서).
    for name, data in fort_smoke.iter_forts(root, data_dir, archive_name):
        lines, placed = expected_territories(name, data, catalog, patterns)
        expected += lines
        files += 1
        territories += placed
    actual = fort_smoke.run(executable, '--dump-territories', root, *flag).decode('utf-8').splitlines()
    if actual != expected:
        (OUTPUT / f'{edition}-territories-actual.txt').write_text('\n'.join(actual) + '\n', encoding='utf-8', newline='\n')
        (OUTPUT / f'{edition}-territories-expected.txt').write_text('\n'.join(expected) + '\n', encoding='utf-8', newline='\n')
        first = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b), min(len(actual), len(expected)))
        raise AssertionError(f'영역 배치 불일치: {edition} 줄 {first + 1} (extracted/cpp-window-smoke 에 두 출력을 남김)')
    return {'fort_files': files, 'placed_territories': territories, 'compared_lines': len(expected)}


def read_data(root, data_dir, archive, name):
    """낱개 파일을 먼저, 없으면 아카이브에서 읽는다(대소문자 무시)."""
    # 데이터 폴더의 파일 이름을 대소문자 구분 없이 찾는다.
    for path in (root / data_dir).iterdir():
        if path.name.lower() == name.lower():
            return path.read_bytes()
    wanted = f'\\d\\{name.lower()}'
    entry = next(entry for entry in archive.entries if entry.name.lower() == wanted)
    return archive.read(entry)


def screen_palette(col):
    """COL 파일(8바이트 머리 + RGB 256개)을 화면 팔레트로 바꾼다. 원본처럼 0번은 검정, 255번은 흰색으로 고정한다."""
    colors = [tuple(col[8 + i * 3:11 + i * 3]) for i in range(256)]
    colors[0] = (0, 0, 0)
    colors[255] = (255, 255, 255)
    return colors


def run_window(executable, root, flag, settings, view, output):
    """새 실행 파일의 창을 몇 프레임 띄우고 화면 버퍼 그림을 RGB로 읽는다."""
    arguments = ['--run', root, '--view', view, '--window', '--frames', FRAMES, '--screenshot', output]
    if settings:
        arguments += ['--set', settings]
    fort_smoke.run(executable, *arguments, *flag)
    with Image.open(output) as image:
        return image.convert('RGB')


def render_types(width, height, shape_data, type_texts, order, palette):
    """타입 표 화면을 Python 해독기로 따로 그린다: 타입마다 기본 프레임을 칸 가운데에, 칸 밖은 자른다."""
    image = Image.new('RGB', (width, height), palette[0])
    pixels = image.load()
    blocks = read_blocks(shape_data)
    columns = max(width // TYPE_CELL_WIDTH, 1)
    drawn = 0
    # 로딩 순서대로 왼쪽 위부터.
    for index, name in enumerate(order):
        clusters = parse_type(type_texts[name.lower()]).clusters
        default = 0
        # 마지막으로 나온 default 클러스터가 기본 프레임이다.
        for position, cluster in enumerate(clusters):
            if 'default' in str(cluster[1]).lower().split():
                default = position
        frames = blocks[index].frames
        if default >= len(frames) or frames[default].special:
            continue
        frame = frames[default]
        left, top, right, bottom = frame.rect
        cell_x = index % columns * TYPE_CELL_WIDTH
        cell_y = index // columns * TYPE_CELL_HEIGHT
        base_x = cell_x + TYPE_CELL_WIDTH // 2 - truncate_half(left + right)
        base_y = cell_y + TYPE_CELL_HEIGHT // 2 - truncate_half(top + bottom)
        # 풀어 낸 줄마다 불투명 픽셀만 찍는다.
        for row_index, row in enumerate(decode_frame(shape_data, frame)):
            y = base_y + top + row_index
            if not (cell_y <= y < cell_y + TYPE_CELL_HEIGHT and 0 <= y < height):
                continue
            # 한 줄의 픽셀.
            for column, value in enumerate(row):
                x = base_x + left + column
                if value is None or not (cell_x <= x < cell_x + TYPE_CELL_WIDTH and 0 <= x < width):
                    continue
                pixels[x, y] = palette[value]
                drawn += 1
    return image, drawn


def check_window(executable, edition, data_dir, archive_name, flag, settings, mission, type_count):
    """판본 하나의 창 검사: 타입 표는 픽셀 단위 비교, 미션 화면은 팔레트·그려진 양 확인."""
    root = ROOT / edition
    archive = TaffArchive(str(root / archive_name))
    palette_name = fort_smoke.run(executable, '--config-get', root, 'fortPal', *flag).decode('utf-8').strip()
    palette = screen_palette(read_data(root, data_dir, archive, palette_name + '.col'))
    texts = fort_smoke.load_type_texts(str(root)) if data_dir == 'd' else fort_smoke.load_type_texts_cd(root)
    shape_data = read_data(root, data_dir, archive, '_shapes.shp')
    # 1) 타입 표.
    actual = run_window(executable, root, flag, settings, 'types', OUTPUT / f'{edition}-types.bmp')
    expected, drawn = render_types(actual.width, actual.height, shape_data, texts, TYPE_LOAD_ORDER[:type_count], palette)
    if actual.tobytes() != expected.tobytes():
        expected.save(OUTPUT / f'{edition}-types-expected.png')
        actual.save(OUTPUT / f'{edition}-types-actual.png')
        raise AssertionError(f'타입 표 화면 불일치: {edition} (extracted/cpp-window-smoke 에 두 그림을 남김)')
    # 2) 미션 화면.
    mission_image = run_window(executable, root, flag, settings, mission, OUTPUT / f'{edition}-{mission}.bmp')
    mission_image.save(OUTPUT / f'{edition}-{mission}.png')
    colors = mission_image.getcolors(1 << 24)
    unknown = [color for _, color in colors if color not in set(palette)]
    if unknown:
        raise AssertionError(f'{edition} {mission}: 팔레트에 없는 색 {unknown[:5]}')
    background = next((count for count, color in colors if color == palette[0]), 0)
    drawn_mission = mission_image.width * mission_image.height - background
    if drawn_mission < MIN_DRAWN_PIXELS:
        raise AssertionError(f'{edition} {mission}: 그려진 픽셀이 {drawn_mission}개뿐이다')
    return {'screen': [actual.width, actual.height], 'palette': palette_name,
            'types_view_pixels_compared': actual.width * actual.height, 'types_view_drawn_pixels': drawn,
            'mission': mission, 'mission_colors': len(colors), 'mission_drawn_pixels': drawn_mission}


def digest(paths):
    """파일 묶음의 내용 해시."""
    return hashlib.sha256(b''.join(hashlib.sha256(path.read_bytes()).digest() for path in paths)).hexdigest()


def main():
    """두 판본의 영역 배치와 창 화면을 검사하고 보고서를 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--no-window', action='store_true', help='창을 띄우는 검사를 건너뛴다')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    patterns = json.loads(PATTERNS.read_text(encoding='utf-8'))
    report = {}
    # 판본마다 영역 배치와 창을 검사한다.
    for (edition, data_dir, archive_name, flag, type_count), (_, _, settings, mission) in zip(fort_smoke.EDITIONS, WINDOW_CASES):
        root = ROOT / edition
        watched = [root / archive_name] + sorted(path for path in (root / data_dir).iterdir() if path.is_file())
        watched += sorted(path for path in root.iterdir() if path.is_file())
        before = digest(watched)
        report[edition] = {'territories': check_territories(args.exe, edition, data_dir, archive_name, flag, type_count, patterns)}
        if not args.no_window:
            report[edition]['window'] = check_window(args.exe, edition, data_dir, archive_name, flag, settings, mission, type_count)
        if digest(watched) != before:
            raise AssertionError(f'원본 파일 변경 감지: {edition}')
        report[edition]['original_files_unchanged'] = len(watched)
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
