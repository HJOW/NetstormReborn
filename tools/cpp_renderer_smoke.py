#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C++ 글꼴 전수 해독·실제 창의 글자 픽셀·변경 없는 프레임 출력을 독립 Python 결과와 대조한다."""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
import pefile
from PIL import Image
from shp import read_blocks, decode_frame
from cpp_smoke_files import preserve_game_settings

# 저장소 루트와 검사 결과 경로. 원본의 파일을 생성하거나 수정하지 않는다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-renderer-smoke'
# 원본 글꼴 슬롯·파일 이름. 슬롯 2는 비어 있다.
FONTS = [(0, 'Arial', 14, 700), (1, 'Courier New', 13, 0), (3, 'Arial', 20, 700),
         (4, 'Arial', 48, 0), (5, 'Arial', 14, 0), (6, 'Arial', 12, 0)]
# 원본 문맥 순서의 스타일 이름.
STYLES = ['normal', 'italic', 'bold', 'strikeout', 'underline']
# 005423a8/CD 00516c60의 실제 리소스 표 기대값.
CURSORS = [0, 113, 110, 111, 108, 107, 109, 115, 116, 131, 117, 130, 129, 132, 133, 134, 136, 141, 148]


def snapshots(root):
    """원본 폴더의 모든 파일 이름과 내용을 읽기 전용으로 해시한다."""
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(root.rglob('*')) if p.is_file()}


def read_font(data):
    """원본의 네 정수 표와 독립 VFX 글리프를 Python 판독기로 읽는다."""
    assert data[:16] == b'BitmapFontData\x1a\0'
    version, height, ascent, descent, count = struct.unpack_from('<5I', data, 16)
    assert version == 1 and count == 256
    tables = [struct.unpack_from('<256i', data, 36 + i * 1024) for i in range(4)]
    position, glyphs = 4132, []
    # 저장된 바이트 크기로 각 독립 블록을 분리한다.
    for c in range(256):
        shape = data[position:position + tables[3][c]]
        position += len(shape)
        frame = read_blocks(shape)[0].frames[0] if shape else None
        rows = decode_frame(shape, frame) if frame and not frame.special else []
        glyphs.append({'advance': tables[0][c], 'bearing': tables[1][c], 'draw_advance': tables[2][c], 'frame': frame, 'rows': rows})
    assert position == len(data)
    return {'height': height, 'ascent': ascent, 'descent': descent, 'glyphs': glyphs}


def compare_font(executable, root, name, expected):
    """C++ 검사 스트림의 모든 표·픽셀·투명 마스크를 독립 해독값과 대조한다."""
    output = subprocess.run([str(executable), '--dump-font', str(root), 'd/' + name], check=True, stdout=subprocess.PIPE, timeout=30).stdout
    position, compared = 0, 0
    # 다음 정수들을 읽되 잘린 출력은 struct가 거부한다.
    def take(count):
        nonlocal position
        values = struct.unpack_from('<' + 'i' * count, output, position)
        position += count * 4
        return values
    assert take(3) == (expected['height'], expected['ascent'], expected['descent'])
    # 256개 바이트 글자의 표와 실제 이미지.
    for glyph in expected['glyphs']:
        assert take(4) == (glyph['advance'], glyph['bearing'], glyph['draw_advance'], int(glyph['frame'] is not None))
        if glyph['frame']:
            assert take(4) == glyph['frame'].rect
            width, height = take(2)
            rows = glyph['rows']
            assert (width, height) == (len(rows[0]) if rows else 0, len(rows))
            indices = bytes(value if value is not None else 0 for row in rows for value in row)
            opacity = bytes(255 if value is not None else 0 for row in rows for value in row)
            assert output[position:position + width * height] == indices
            position += width * height
            assert output[position:position + width * height] == opacity
            position += width * height
            compared += width * height
    assert position == len(output)
    return compared


def render_text(image, font, text, x, y):
    """원본의 B+C 폭과 글리프 기준점으로 독립 검사용 흰 글자를 합성한다."""
    pixels = image.load()
    # UTF-8이 아닌 원본 코드 페이지로 그린다.
    for c in text.encode('cp1252'):
        glyph = font['glyphs'][c]
        if glyph['frame']:
            left, top, _, _ = glyph['frame'].rect
            # 실제 불투명 행.
            for row, values in enumerate(glyph['rows']):
                # 화면 안의 불투명 글리프만 기록한다.
                for col, value in enumerate(values):
                    px, py = x + left + col, y + top + row
                    if value is not None and 0 <= px < image.width and 0 <= py < image.height:
                        pixels[px, py] = (255, 255, 255) if value == 100 else (0, 0, 0)
            x += glyph['draw_advance']


def main():
    """원본 해시를 보존하며 콘솔 전수 대조와 선택적 cpppj 창 검사를 실행한다. 창 설정은 복구한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--console-only', action='store_true', help='창/설정 변경 없이 원본 글꼴의 모든 표/픽셀과 커서 리소스만 대조한다')
    args = parser.parse_args()
    executable = args.exe.resolve()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {'fonts': 0, 'glyphs': 0, 'pixels_compared': 0, 'cursor_tables': {}, 'software_cursor_windows': {}}
    # 실제 두 판본의 커서 번호·리소스 존재를 확인한다. PE는 실행하지 않는다.
    for edition, name, address in [('originals', 'Netstorm.exe', 0x5423a8), ('originalCD', 'NETSTORM.EXE', 0x516c60)]:
        pe = pefile.PE(str(ROOT / edition / name))
        values = struct.unpack('<19I', pe.get_data(address - pe.OPTIONAL_HEADER.ImageBase, 76))
        assert list(values) == CURSORS
        groups = next(entry for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries if entry.id == 12)
        ids = {entry.id for entry in groups.directory.entries}
        assert set(CURSORS[1:]) <= ids
        report['cursor_tables'][edition] = len(CURSORS) - 1
    root = ROOT / 'originals'
    before = snapshots(root)
    expected = Image.new('RGB', (1024, 768), (0, 0, 0))
    y = 20
    # 창에 그릴 18개 원본 캐시도 모두 독립 해독한다.
    for slot, face, height, weight in FONTS:
        # 원본에서 생성되는 스타일만 읽는다.
        for style in range(1 if slot in (1, 3, 4) else 5):
            name = f'!{face}.{STYLES[style]}.{height}.{weight}.chfnt'
            font = read_font((root / 'd' / name).read_bytes())
            report['pixels_compared'] += compare_font(executable, root, name, font)
            report['fonts'] += 1
            report['glyphs'] += 256
            render_text(expected, font, f'Font {slot}/{style}: NetStorm Islands at War 0123456789', 20, y)
            y += font['height'] + 12
    if args.console_only:
        if snapshots(root) != before: raise RuntimeError('콘솔 글꼴 검사 뒤 원본 변경')
        report['original_files_unchanged'] = len(before)
        report['console_only'] = True
        (OUTPUT / 'console-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return
    screenshot = OUTPUT / 'fonts.bmp'
    with preserve_game_settings(root, 'd'):
        result = subprocess.run([str(executable), '--run', str(root), '--view', 'fonts', '--window', '--frames', '5',
                                 '--set', 'SCREENW=1024;SCREENH=768;windowScreenFlags=10;fontFaceName=Arial',
                                 '--screenshot', str(screenshot), '--render-stats'], check=True, capture_output=True, timeout=60)
    actual = Image.open(screenshot).convert('RGB')
    assert actual.size == expected.size and actual.tobytes() == expected.tobytes(), '원본 캐시 글자 화면 픽셀 불일치'
    assert b'Renderer draws=1 presents=1' in result.stdout, result.stdout
    expected.save(OUTPUT / 'fonts-expected.png')
    actual.save(OUTPUT / 'fonts.png')
    # 실제 리소스 모듈에서 18개 소프트웨어 커서를 생성하는 경로를 두 판본 모두 실행한다.
    for edition in ('originals', 'originalCD'):
        cursor_root = ROOT / edition
        cursor_before = snapshots(cursor_root)
        cursor_screenshot = OUTPUT / f'{edition}-software-cursor.bmp'
        command = [str(executable), '--run', str(cursor_root), '--view', 'types', '--window', '--frames', '5',
                   '--set', 'SCREENW=800;SCREENH=600;windowScreenFlags=74', '--screenshot', str(cursor_screenshot)]
        if edition == 'originalCD':
            command.append('--cd')
        with preserve_game_settings(cursor_root, 'd'):
            subprocess.run(command, check=True, capture_output=True, timeout=60)
        assert Image.open(cursor_screenshot).size == (800, 600)
        assert snapshots(cursor_root) == cursor_before, f'{edition}: 소프트웨어 커서 검사 뒤 원본 변경'
        report['software_cursor_windows'][edition] = {'generated_resource_cursors': 18, 'frames': 5}
    assert snapshots(root) == before, '원본 파일 목록/바이트 변경'
    report['window_pixels_compared'] = 1024 * 768
    report['unchanged_frames_not_redrawn'] = 4
    report['original_files_unchanged'] = len(before)
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
