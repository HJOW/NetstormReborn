#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 창의 메뉴→캠페인→원본 브리핑→미션 진입/복귀와 GIF 번호를 검사한다. 이동은 cpp_world_smoke.py가 검사한다."""
import argparse
import io
import json
import struct
import subprocess
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops
from taff import TaffArchive
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

# 원본 자료를 건드리지 않는 검사 결과 폴더.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-menu-smoke'


def run(executable, arguments):
    """새 C++ 실행 파일만 구동하고 실패 메시지를 보존한다."""
    result = subprocess.run([str(executable), *map(str, arguments)], capture_output=True, timeout=60)
    if result.returncode:
        raise AssertionError(result.stderr.decode('utf-8', errors='replace') + result.stdout.decode('utf-8', errors='replace'))
    return result.stdout


def source(root, archive, path):
    """독립 Python VFS로 디스크 우선, 없으면 TAFF의 원본 바이트를 읽는다."""
    disk = root / path
    if disk.exists():
        return disk.read_bytes()
    entry = next(e for e in archive.entries if e.name.replace('\\', '/').lstrip('/').lower() == path.lower())
    return archive.read(entry)


def check_gifs(executable, root, archive):
    """Pillow의 해독 결과와 C++ 색 번호/투명을 대조한다. 실제 자료가 LZW 사전 폭 증가도 검사한다."""
    count = 0
    # 메뉴와 캠페인 배경을 모두 읽어 타이틀 하나만 맞는 경우를 막는다.
    for path in ['d/titleMenu.gif', 'd/gifcloud.gif', 'd/gifcloud2.gif']:
        expected = Image.open(io.BytesIO(source(root, archive, path)))
        assert expected.mode == 'P'
        data = run(executable, ['--dump-gif', root, path])
        width, height = struct.unpack_from('<2I', data)
        size = width * height
        assert (width, height) == expected.size and len(data) == 8 + size * 2
        assert data[8:8 + size] == expected.tobytes(), path
        transparent = expected.info.get('transparency', -1)
        opacity = bytes(0 if value == transparent else 255 for value in expected.tobytes())
        assert data[8 + size:] == opacity, path
        count += size
    return count


def reports(path):
    """프레임마다 기록된 실제 상태와 판정 영역을 읽는다."""
    result = {}
    current = None
    # report 사건마다 새 스냅샷을 시작한다.
    for line in path.read_text(encoding='utf-8').splitlines():
        fields = line.split('\t')
        if fields[0] == 'frame':
            current = {'frame': int(fields[1]), 'controls': {}}
            result[fields[2]] = current
        elif fields[0] == 'control':
            current['controls'][fields[1]] = list(map(int, fields[2:]))
        else:
            current[fields[0]] = fields[1:]
    return result


def check_title(root, archive, image_path):
    """돌 버튼 영역을 제외한 실제 창 전체를 독립 GIF/게임 팔레트로 합성해 비교한다."""
    data = source(root, archive, 'd/gifcloud.col')
    colors = list(data[8:8 + 768])
    colors[:3] = [0, 0, 0]
    colors[-3:] = [255, 255, 255]
    cloud = Image.open(io.BytesIO(source(root, archive, 'd/gifcloud.gif')))
    title = Image.open(io.BytesIO(source(root, archive, 'd/titleMenu.gif')))
    cloud.putpalette(colors)
    title.putpalette(colors)
    expected = Image.new('RGB', (1024, 768))
    # 원본 크기의 구름 타일을 화면 전체에 반복한다.
    for y in range(0, 768, cloud.height):
        # 오른쪽 경계는 Pillow paste가 잘라 낸다.
        for x in range(0, 1024, cloud.width):
            expected.paste(cloud.convert('RGB'), (x, y))
    expected.paste(title.convert('RGB'), (192, 144))
    actual = Image.open(image_path).convert('RGB')
    assert actual.size == expected.size
    mask = Image.new('L', expected.size, 255)
    draw = ImageDraw.Draw(mask)
    # 버튼은 별도 입력/좌표 검사 대상으로 두고 GIF 픽셀 대조에서는 제외한다.
    for i in range(8):
        x, y = 356 + i % 4 * 79, 311 + i // 4 * 23
        draw.rectangle((x, y, x + 74, y + 19), fill=0)
    expected.paste((0, 0, 0), mask=ImageChops.invert(mask))
    actual.paste((0, 0, 0), mask=ImageChops.invert(mask))
    assert ImageChops.difference(actual, expected).getbbox() is None, 'GIF 색 번호/화면 합성 불일치'
    return 1024 * 768 - 8 * 75 * 20


def check_window(executable, root):
    """마우스 사건으로 탐색한다. 명령/상태를 직접 바꾸지 않는다."""
    steps = [
        (1, 'report', 'main'), (1, 'snapshot', str(OUTPUT / 'main.bmp')),
        (2, 'right', 'Campaign'), (3, 'report', 'right_ignored'),
        (4, 'click', 'Campaign'), (5, 'report', 'campaign'), (5, 'snapshot', str(OUTPUT / 'campaign.bmp')),
        (6, 'click', 'Struggle For Freedom'), (7, 'report', 'overview'),
        (8, 'click', '2 Master of Whirligigs'), (9, 'report', 'locked'),
        (10, 'click', '1 The War Begins!'), (11, 'report', 'loading'),
        (12, 'report', 'briefing'), (12, 'snapshot', str(OUTPUT / 'briefing.bmp')),
        (13, 'esc', ''), (14, 'report', 'briefing_esc'),
        (15, 'click', 'Play Mission'), (16, 'report', 'mission'), (16, 'snapshot', str(OUTPUT / 'mission.bmp')),
        (17, 'click', 'Game'), (18, 'report', 'game'),
        (19, 'click', 'Leave Battle'), (20, 'report', 'leave'),
        (21, 'click', 'Stay'), (22, 'report', 'resumed'),
        (23, 'esc', ''), (24, 'report', 'game_esc'),
        (25, 'click', 'Leave Battle'), (26, 'report', 'leave2'),
        (27, 'click', 'Leave Honorably'), (28, 'report', 'returned'),
        (29, 'click', 'Campaign'), (30, 'report', 'campaign2'),
        (31, 'click', 'Struggle For Freedom'), (32, 'report', 'overview2'),
        (33, 'click', 'Back'), (34, 'report', 'campaign_back'),
        (35, 'click', 'Back'), (36, 'report', 'main_back'),
        (37, 'click', 'Options'), (38, 'report', 'options'),
        (39, 'click', 'Direct Draw / Full Screen'), (40, 'report', 'disabled_option'),
        (41, 'click', 'Sound On'), (42, 'report', 'sound_toggle'),
        (43, 'click', 'Resolution >'), (44, 'report', 'resolutions'),
        (45, 'click', '800 by 600'), (46, 'report', 'size800'), (46, 'snapshot', str(OUTPUT / '800.bmp')),
        (47, 'click', 'Resolution >'), (48, 'report', 'resolutions2'),
        (49, 'click', '640 by 480'), (50, 'report', 'size640'), (50, 'snapshot', str(OUTPUT / '640.bmp')),
        (51, 'click', 'Resolution >'), (52, 'report', 'resolutions3'),
        (53, 'click', '1024 by 768'), (54, 'report', 'size1024'),
        (55, 'outside', ''), (56, 'report', 'outside_closed'),
        (57, 'click', 'Credits'), (58, 'report', 'credits'),
        (59, 'click', 'Cancel'), (60, 'report', 'credits_back'),
        (61, 'click', 'Help'), (62, 'report', 'help'),
        (63, 'click', 'About NetStorm'), (64, 'report', 'about'),
        (65, 'click', 'OK'), (66, 'report', 'about_back'),
        (67, 'click', 'Demo'), (68, 'report', 'demo'),
        (69, 'click', 'Cancel'), (70, 'report', 'demo_back'),
        (71, 'click', 'Campaign'), (72, 'report', 'campaign3'),
        (73, 'click', 'Early Missions'), (74, 'report', 'early'),
        (75, 'click', '1 Bridge the Gap'), (77, 'report', 'tutorial_brief'),
        (78, 'click', 'MORE'), (79, 'report', 'tutorial_more'),
        (80, 'click', 'BACK'), (81, 'report', 'tutorial_back'),
        (82, 'click', 'MORE'), (83, 'report', 'tutorial_more2'),
        (84, 'click', 'OK'), (85, 'report', 'tutorial_world'),
        (86, 'esc', ''), (88, 'click', 'Leave Battle'), (90, 'click', 'Leave Honorably'),
        (91, 'report', 'tutorial_return'), (92, 'click', 'Options'), (93, 'report', 'volume_options'),
        (94, 'click', 'Music Volume >'), (95, 'report', 'volume_list'),
        (96, 'click', 'Volume 4'), (97, 'report', 'volume_changed'),
        (98, 'click', 'Options'), (99, 'report', 'volume_reopen'),
        (100, 'click', 'Music Volume >'), (101, 'report', 'volume_selected'),
        (102, 'outside', ''), (103, 'report', 'volume_closed'), (104, 'click', 'Quit'),
    ]
    expanded = []
    offset = 0
    # 값 변경 뒤 원본처럼 닫힌 Options를 실제 버튼으로 다시 연다.
    for frame in sorted({step[0] for step in steps}):
        expanded.extend((frame + offset, operation, argument) for original, operation, argument in steps if original == frame)
        if frame in (42, 46, 50, 54):
            expanded.extend([(frame + offset + 1, 'click', 'Options'), (frame + offset + 2, 'report', f'options_reopened{frame}')])
            offset += 2
    steps = expanded
    script = OUTPUT / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    marker = root / 'fullscreenStateFile.dat'
    marker.write_bytes(b'previous startup')
    settings = 'SCREENW=1024;SCREENH=768;maxFPS=75;sound=1;musicVolume=2;autoDemo=0;tellTips=0;DoneTheWarBegins=0;DoneMasterOfWhirligigs=0;DoneTutorial1=0'
    run(executable, ['--run', root, '--window', '--frames', '130', '--set', settings,
                     '--ui-script', script, '--ui-report', OUTPUT / 'states.tsv'])
    states = reports(OUTPUT / 'states.tsv')
    expected_phases = {'main': 0, 'right_ignored': 0, 'campaign': 1, 'overview': 1, 'locked': 1, 'loading': 2,
                       'briefing': 3, 'briefing_esc': 3, 'mission': 4, 'resumed': 4, 'returned': 0,
                       'campaign_back': 1, 'main_back': 0, 'outside_closed': 0,
                       'credits': 1, 'credits_back': 0, 'about': 1, 'about_back': 0,
                       'demo': 1, 'demo_back': 0, 'tutorial_brief': 3, 'tutorial_more': 3,
                       'tutorial_back': 3, 'tutorial_world': 4, 'tutorial_return': 0}
    # 호출 결과가 아니라 프레임 종료 때 관찰된 단계를 비교한다.
    for key, phase in expected_phases.items():
        assert states[key]['phase'] == [str(phase)], (key, states[key])
    assert states['main']['fullscreenMarker'] == ['1'] and states['right_ignored']['fullscreenMarker'] == ['1']
    assert states['campaign']['fullscreenMarker'] == ['0'] and not marker.exists()
    assert states['overview']['controls']['2 Master of Whirligigs'][0] == 0
    assert states['overview']['controls']['1 The War Begins!'][0] == 1
    assert states['locked']['page'] == states['overview']['page']
    assert states['briefing']['paused'] == ['1'] and states['resumed']['paused'] == ['0']
    assert states['mission']['mission'][:2] == ['TheWarBegins', 'Tutorial'] and int(states['mission']['mission'][2]) > 0
    assert states['sound_toggle']['sound'] == ['0']
    # 해상도는 설정 글만 바뀐 것이 아니라 실제 장치 크기와 캡처를 함께 검사한다.
    for key, size in [('size800', (800, 600)), ('size640', (640, 480)), ('size1024', (1024, 768))]:
        assert tuple(map(int, states[key]['size'])) == size
    assert Image.open(OUTPUT / '800.bmp').size == (800, 600) and Image.open(OUTPUT / '640.bmp').size == (640, 480)
    assert states['options']['controls']['Direct Draw / Full Screen'][0] == 0
    assert states['disabled_option']['popup'] == ['options']
    assert states['tutorial_more']['page'] == ['A1.'] and states['tutorial_back']['page'] == ['A.']
    assert states['volume_changed']['musicVolume'] == ['4'] and states['volume_changed']['popup'] == ['']
    assert states['volume_selected']['controls']['Volume 4'][-1] == 1
    assert states['volume_selected']['controls']['Volume 2'][-1] == 0
    restart = OUTPUT / 'restart.tsv'
    restart.write_text('1\treport\trestarted\n', encoding='utf-8')
    run(executable, ['--run', root, '--window', '--frames', '3', '--ui-script', restart, '--ui-report', OUTPUT / 'restart-states.tsv'])
    restarted = reports(OUTPUT / 'restart-states.tsv')['restarted']
    assert restarted['musicVolume'] == ['4'] and restarted['sound'] == ['0'] and restarted['size'] == ['1024', '768']
    return {'observed_states': len(states), 'mission_objects': int(states['mission']['mission'][2]),
            'tutorial_objects': int(states['tutorial_world']['mission'][2]), 'native_window_navigation': True,
            'resolution_changes': [[800, 600], [640, 480], [1024, 768]], 'fullscreen_marker_event': True,
            'options_persist_after_restart': True}


def main():
    """기준 판본의 탐색과 두 판본의 GIF 자료를 검사하고 원본 해시/설정을 복구한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {}
    # CD는 자료 호환만 대조한다. 메뉴 스크립트는 10.78과 달라 완료 기준에 섞지 않는다.
    for edition in ['originals', 'originalCD']:
        root = ROOT / edition
        before = snapshots(root)
        archive = TaffArchive(root / 'netstorm.tarc')
        report[edition] = {'gif_pixels': check_gifs(args.exe, root, archive)}
        if edition == 'originals':
            with preserve_game_settings(root, 'd'):
                report[edition].update(check_window(args.exe, root))
            report[edition]['menu_background_pixels'] = check_title(root, archive, OUTPUT / 'main.bmp')
        assert snapshots(root) == before, f'{edition}: 원본 파일/존재 상태 변경'
        report[edition]['original_files_unchanged'] = True
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
