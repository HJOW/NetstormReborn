#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 raw 표면의 실제 자산 로드·정지 중 예약·재개 뒤 끝 칸/연결 정리·화면 변화를 검사한다."""
import argparse
import json
from pathlib import Path

from PIL import Image, ImageChops
from cpp_menu_smoke import run
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import parse, states

# 원본 실행 파일 대신 클론만 실행하며 결과는 Git 제외 경로에 남긴다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-surface-world-smoke'
# 10.78과 CD 10.72의 실제 표면/부착 타입 번호다.
BRIDGE, TERRAIN, ISLAND, STALAG, CONNECTOR, SURFACE = 82, 97, 94, 95, 155, 157


def at(state, kind, x, y):
    """현재 raw 슬롯에서 특정 위치/종류를 찾는다. 옛 object 행은 읽지 않는다."""
    return {sid: row for sid, row in state['raw_objects'].items()
            if int(row[0]) == kind and float(row[3]) == x and float(row[4]) == y}


def counts(state):
    """실제 raw 종류별 개수를 계산한다."""
    result = {}
    # 살아 있는 렌더링 입력만 센다.
    for row in state['raw_objects'].values():
        kind = int(row[0])
        result[kind] = result.get(kind, 0) + 1
    return result


def load(executable, root, mission, flags):
    """전체 raw 표면 수와 저장 다리/표면의 좌표를 실제 두 판본 자산에서 대조한다."""
    state = parse(run(executable, ['--dump-world', root, mission, *flags]).decode('utf-8'))
    tally = counts(state)
    assert int(state['raw_world'][3]) == sum(tally.get(kind, 0) for kind in (TERRAIN, BRIDGE, SURFACE))
    assert tally.get(ISLAND, 0) == tally.get(STALAG, 0)
    assert state['raw_world'][2] == '0' and state['raw_world'][6:9] == ['0', '0', '0']
    # 초기 저장 다리와 noIsland가 같은 위치/소유자로 실제 Pop되어 있어야 한다.
    for row in state['objects'].values():
        if row[0] not in ('bridge', 'noIsland'):
            continue
        kind = BRIDGE if row[0] == 'bridge' else SURFACE
        actual = at(state, kind, float(row[5]), float(row[6]))
        assert len(actual) == 1
        if kind == BRIDGE:
            value = next(iter(actual.values()))
            assert value[1:3] == [row[1], row[7]]
    return {'raw_objects': len(state['raw_objects']), 'surface_count': int(state['raw_world'][3]),
            'graph_count': int(state['raw_world'][4]), 'connectors': tally.get(CONNECTOR, 0)}


def window(executable, root):
    """첫 미션에서 받침 삭제를 검사 경계로 주입한 뒤 실제 게임 루프/렌더러의 결과를 확인한다."""
    script, report = OUTPUT / 'steps.tsv', OUTPUT / 'states.tsv'
    before_image, after_image = OUTPUT / 'before.bmp', OUTPUT / 'after.bmp'
    steps = [(2, 'click', 'Play Mission'), (4, 'key', 'right'), (44, 'keyup', 'right'),
             (46, 'report', 'before'), (46, 'snapshot', before_image),
             (48, 'esc', ''), (50, 'surface-delete', 'island:135,138'), (51, 'report', 'paused_delete'),
             (70, 'report', 'paused_later'), (72, 'esc', ''), (76, 'report', 'after'), (76, 'snapshot', after_image),
             (78, 'esc', ''), (80, 'click', 'Leave Battle'), (82, 'click', 'Leave Honorably'),
             (84, 'report', 'main'), (86, 'click', 'Campaign'), (88, 'click', 'Struggle For Freedom'),
             (90, 'click', '1 The War Begins!'), (93, 'click', 'Play Mission'), (95, 'report', 'reloaded')]
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    run(executable, ['--run', root, '--mission', 'thewarbegins', '--window', '--frames', '97',
                     '--set', 'SCREENW=1024;SCREENH=768;sleepPerLoop=0', '--ui-script', script, '--ui-report', report])
    result = states(report)
    before, paused, later, after = (result[name] for name in ('before', 'paused_delete', 'paused_later', 'after'))
    assert len(at(before, ISLAND, 135, 138)) == 1 and len(at(before, CONNECTOR, 133, 136)) == 1
    old = next(iter(at(before, BRIDGE, 133, 135)))
    assert paused['paused'] == later['paused'] == ['1'] and paused['raw_world'][2] == later['raw_world'][2] == '1'
    assert paused['raw_objects'] == later['raw_objects'] and paused['raw_world'][0] == later['raw_world'][0]
    assert old in paused['raw_objects'] and at(paused, CONNECTOR, 133, 136) and not at(paused, ISLAND, 135, 138)
    assert after['paused'] == ['0'] and after['raw_world'][2] == '0' and old not in after['raw_objects']
    assert len(at(after, BRIDGE, 133, 135)) == 1 and not at(after, CONNECTOR, 133, 136)
    assert counts(after)[BRIDGE] == counts(before)[BRIDGE] and counts(after)[CONNECTOR] == counts(before)[CONNECTOR] - 1
    assert after['raw_world'][6:9] == ['0', '0', '0'] and int(after['raw_world'][9]) > 0
    # 같은 화면 위치의 받침/다리 영역에서 실제 픽셀이 바뀌어야 한다. 메뉴나 게임 시각 차이로 대체하지 않는다.
    camera_x, camera_y = map(int, before['camera'])
    assert before['camera'] == after['camera']
    x, y = 135 * 16 - camera_x, 138 * 11 - camera_y
    region = (x - 70, y - 75, min(x + 70, 1024), min(y + 75, 768))
    assert region[0] >= 0 and region[1] >= 28 and region[0] < region[2]
    assert ImageChops.difference(Image.open(before_image).convert('RGB').crop(region),
                                Image.open(after_image).convert('RGB').crop(region)).getbbox() is not None
    assert not result['main']['raw_objects'] and result['main']['phase'] == ['0']
    reloaded = result['reloaded']
    assert reloaded['raw_world'][2] == '0' and len(at(reloaded, ISLAND, 135, 138)) == 1 and len(at(reloaded, CONNECTOR, 133, 136)) == 1
    assert counts(reloaded) == counts(before)
    return {'pause_keeps_scheduled_regular': True, 'resume_replaces_bridge_and_removes_connector': True,
            'surface_pixels_changed': True, 'reload_restores_new_world': True, 'image_region': region}


def main():
    """자료 해시와 허용 설정 파일의 바이트/존재 상태를 보존하며 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {}
    # 두 판본 로드는 읽기 전용이며 클론 GUI만 설정 보존 범위 안에서 실행한다.
    for directory, flags in (('originals', ()), ('originalCD', ('--cd',))):
        root = ROOT / directory
        before = snapshots(root)
        try:
            with preserve_game_settings(root, 'd' if directory == 'originals' else 'D'):
                missions = ('thewarbegins', 'savetheisland', 'TEST01') if directory == 'originals' else ('thewarbegins', 'savetheisland')
                report[directory] = {mission: load(args.exe, root, mission, flags) for mission in missions}
                if directory == 'originals':
                    report['window'] = window(args.exe, root)
        finally:
            assert snapshots(root) == before, f'{directory}: 원본 파일 상태 변경'
    report['original_files_unchanged'] = True
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
