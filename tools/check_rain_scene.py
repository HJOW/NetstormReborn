#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 클론 화면의 레인 신전 픽셀을 독립 Python SHP 합성과 대조한다."""
import argparse
import json
import subprocess
from pathlib import Path
from PIL import Image
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_menu_smoke import source
from cpp_world_smoke import states
from cpp_window_smoke import screen_palette
from shp import TYPE_LOAD_ORDER, read_blocks, decode_frame
from taff import TaffArchive

# 보호 원본과 이번 클론 실행 파일의 기준 경로다.
ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe'


def compare(directory):
    """같은 프레임의 실제 raw 상태·화면을 읽고 본체/물결 불투명 픽셀을 모두 대조한다."""
    observed = states(directory / 'states.tsv')
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    data = source(ROOT / 'originals', archive, 'd/_shapes.shp')
    block = read_blocks(data)[TYPE_LOAD_ORDER.index('rainVortex')]
    palette = screen_palette(source(ROOT / 'originals', archive, 'd/gifcloud.col'))
    records = []
    # 서로 다른 네 시점에서 기본 본체와 현재 물결을 같은 기준점에 합성한다.
    for label, state in observed.items():
        sid, temple = next((sid, obj) for sid, obj in state['objects'].items() if obj[0] == 'rainVortex' and obj[1] == '1')
        frame = int(temple[7])
        camera_x, camera_y = map(int, state['camera'])
        x, y = round(float(temple[5]) * 16) - camera_x, round(float(temple[6]) * 11) - camera_y
        expected, base_pixels, wave_pixels = {}, set(), set()
        # 압축 해제는 기존 독립 Python 판독기이며 C++ Renderer를 호출하지 않는다.
        for index in (0, frame):
            shape = block.frames[index]
            # 기준점 상대 압축 영역의 픽셀만 화면에 덮는다.
            for dy, row in enumerate(decode_frame(data, shape)):
                for dx, color in enumerate(row):
                    point = (x + shape.rect[0] + dx, y + shape.rect[1] + dy)
                    if color is not None and 0 <= point[0] < 1024 and 28 <= point[1] < 768:
                        expected[point] = palette[color]
                        (base_pixels if index == 0 else wave_pixels).add(point)
        actual = Image.open(directory / f'{label}.bmp').convert('RGB')
        assert actual.size == (1024, 768)
        mismatches = [(point, rgb, actual.getpixel(point)) for point, rgb in expected.items() if actual.getpixel(point) != rgb]
        records.append({'label': label, 'sid': sid, 'frame': frame, 'compared_pixels': len(expected),
            'base_only_pixels': len(base_pixels - wave_pixels), 'mismatches': len(mismatches), 'first_mismatches': mismatches[:8]})
    return records


def main():
    """보호 설정을 복구하는 실제 클론 실행 뒤 독립 픽셀 판정을 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'extracted/run34-rain-scene')
    parser.add_argument('--compare-only', action='store_true', help='기존 캡처만 독립 대조한다')
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    before = snapshots(ROOT / 'originals')
    if not args.compare_only:
        steps = ['5\tclick\tPlay Mission\n']
        # 보고와 캡처를 같은 루프 시점에 배치해 물결 프레임 차이를 제거한다.
        for frame in (12, 22, 32, 42):
            steps += [f'{frame}\treport\train{frame}\n', f'{frame}\tsnapshot\t{args.output / f"rain{frame}.bmp"}\n']
        script = args.output / 'steps.tsv'
        script.write_text(''.join(steps), encoding='utf-8')
        command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'thewarbegins', '--window', '--no-audio',
            '--frames', '45', '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0',
            '--ui-script', str(script), '--ui-report', str(args.output / 'states.tsv')]
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        with preserve_game_settings(ROOT / 'originals', 'd'):
            result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=55)
        (args.output / 'stdout.log').write_bytes(result.stdout)
        (args.output / 'stderr.log').write_bytes(result.stderr)
        assert result.returncode == 0, result.stderr.decode('utf-8', errors='replace')
    unchanged = snapshots(ROOT / 'originals') == before
    records = compare(args.output)
    passed = unchanged and len(records) == 4 and len({row['frame'] for row in records}) > 1 and all(row['mismatches'] == 0 and row['base_only_pixels'] > 1000 for row in records)
    report = {'passed': passed, 'originals_unchanged': unchanged, 'audio_device': False, 'captures': records}
    (args.output / 'pixels.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))
    assert passed, '실제 레인 신전 기본 그림/물결 픽셀 불일치: pixels.json 확인'


if __name__ == '__main__':
    main()
