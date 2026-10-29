#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 피격 Warning의 비모달 수명과 원본 글꼴/색/그림자 픽셀을 독립 대조한다."""
import argparse
import json
import subprocess
from pathlib import Path
from PIL import Image
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots, read_font
from cpp_menu_smoke import source
from cpp_world_smoke import states
from cpp_window_smoke import screen_palette
from taff import TaffArchive

# 실제 클론만 실행하며 검사 결과는 보호 폴더 밖에 둔다.
ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'cpppj/build/bin/Release/Netstorm.exe'
TEXT = 'Our Priest has been Immobilized'  # 보호 tell.english의 실제 Warning 인자 본문이다.


def compare_pixels(directory, observed):
    """같은 raw 보고/캡처 시점의 노란 bold 글자와 검정1/1 그림자를 직접 합성한다."""
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    font = read_font(source(ROOT / 'originals', archive, 'd/!Arial.bold.14.700.chfnt'))
    palette = screen_palette(source(ROOT / 'originals', archive, 'd/gifcloud.col'))
    yellow = palette[min(range(256), key=lambda i: sum((palette[i][c] - (255, 255, 0)[c]) ** 2 for c in range(3)))]
    expected_width = sum(font['glyphs'][code]['advance'] for code in TEXT.encode('cp1252')) + 1
    result = []
    # 안내가 실제로 있는 캡처만 픽셀 대조한다. 안내가 없는 시점은 아래 수명 검사에서 확인한다.
    for label, state in observed.items():
        path = directory / f'{label}.bmp'
        if not path.exists() or state.get('warning_count') != ['1']:
            continue
        slot, serial, x, y, width, height, deadline = state['warning'][:7]
        x, y, width, height = map(int, (x, y, width, height))
        assert (x, y, width, height) == (1024 - expected_width, 0, expected_width, 15)
        expected = {}
        # 원본은 전체 그림자 패스 뒤 전체 본문 패스를 그린다.
        for offset_x, offset_y, color in ((1, 1, (0, 0, 0)), (0, 0, yellow)):
            at = x + offset_x
            # 원본 코드 페이지와 세 번째 폭 표를 출력 전진 폭으로 사용한다.
            for code in TEXT.encode('cp1252'):
                glyph = font['glyphs'][code]
                if not glyph['frame']:
                    continue
                left, top, _, _ = glyph['frame'].rect
                # 투명 픽셀은 비교 마스크에 넣지 않는다.
                for row, values in enumerate(glyph['rows']):
                    for column, value in enumerate(values):
                        point = (at + left + column, y + offset_y + top + row)
                        if value is not None and x <= point[0] < x + width and y <= point[1] < y + height:
                            assert value == 100
                            expected[point] = color
                at += glyph['draw_advance']
        image = Image.open(path).convert('RGB')
        mismatches = [(point, rgb, image.getpixel(point)) for point, rgb in expected.items() if image.getpixel(point) != rgb]
        result.append({'label': label, 'pixels': len(expected), 'mismatches': len(mismatches), 'first_mismatches': mismatches[:8]})
    return result


def main():
    """같은 실제 마우스 교전으로 안내 생성/지속/자동 소멸과 미션 진행을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'extracted/run35-warning-scene')
    parser.add_argument('--compare-only', action='store_true', help='기존 같은 프레임 캡처/상태만 독립 대조한다')
    args = parser.parse_args()
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    before = snapshots(ROOT / 'originals')
    if not args.compare_only:
        # 실제 Play/초기 패킷 처리 뒤에 선택한다. 초기7프레임 입력은 현재 Client에서 선택0으로 남는다.
        steps = [(5, 'report', 'briefing'), (10, 'click', 'Play Mission'), (20, 'report', 'mission'), (25, 'click', 'world:priest'),
            (26, 'report', 'selected'), (40, 'click', 'cell:122,114')]
        # 실제 피격 시점이 전진해도 생성 전/생존/만료를 놓치지 않도록 앞 구간부터 같은 프레임을 캡처한다.
        for frame in range(100, 2600, 100):
            label = f'warning{frame}'
            steps.append((frame, 'report', label))
            steps.append((frame, 'snapshot', str(directory / f'{label}.bmp')))
        script = directory / 'steps.tsv'
        script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')
        command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'thewarbegins', '--window', '--no-audio', '--frames', '2600',
            '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0', '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
        with preserve_game_settings(ROOT / 'originals', 'd'):
            run = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=90)
        (directory / 'stdout.log').write_bytes(run.stdout)
        (directory / 'stderr.log').write_bytes(run.stderr)
        assert run.returncode == 0, run.stderr.decode('utf-8', errors='replace')
    observed = states(directory / 'states.tsv')
    assert observed['selected']['selected'] == ['15000'], observed['selected']['selected']
    samples = {label: state for label, state in observed.items() if label.startswith('warning')}
    assert samples and all(state['phase'] == ['4'] and state['paused'] == ['0'] and state['page'] == ['mission'] for state in samples.values())
    active = [state for state in samples.values() if state.get('warning_count') == ['1']]
    assert active
    deadline = float(active[0]['warning'][6])
    assert all(state['warning'][0] == '0' and state['warning'][7] == '~B~E~y' + TEXT and float(state['time'][1]) < deadline for state in active)
    after = [state for state in samples.values() if float(state['time'][1]) >= deadline]
    assert after and all(state['warning_count'] == ['0'] for state in after)
    # 실제 피격 뒤 HP1과 이후 원본 PriestRegen의 회복은 함께 발생한다. 마지막 표본의 우연한 피격 주기는 고정하지 않는다.
    hp1_after_expiry = any(state['objects'][15000][12] == '1' for state in after)
    assert observed['selected']['objects'][15000][12] == '100' and hp1_after_expiry
    assert after[-1]['objects'][15000][5:7] == ['122', '114']
    assert float(after[-1]['raw_world'][0]) > float(active[0]['raw_world'][0]) + 5.0
    pixels = compare_pixels(directory, observed)
    unchanged = snapshots(ROOT / 'originals') == before
    passed = unchanged and pixels and all(row['pixels'] > 500 and row['mismatches'] == 0 for row in pixels)
    report = {'passed': bool(passed), 'warning_ui_verified': bool(passed), 'originals_unchanged': unchanged, 'audio_device': False,
        'initial_selection_verified': True, 'game_state_injected': False,
        'active_samples': len(active), 'expired_samples': len(after), 'deadline': deadline,
        'first_active_wall': active[0]['time'][1], 'last_wall': after[-1]['time'][1], 'hp_after': after[-1]['objects'][15000][12],
        'hp1_after_expiry_observed': hp1_after_expiry, 'pixels': pixels}
    (directory / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))
    assert passed, 'Warning 글꼴/색/그림자 픽셀 불일치: verification.json 확인'


if __name__ == '__main__':
    main()
