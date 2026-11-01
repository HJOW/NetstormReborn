#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early1의 C/c·정지/모달·실제 다리 커서 우클릭과 메인 복귀 뒤 같은 옵션을 확인한다(717)."""
import argparse
import json
from pathlib import Path
import re
from PIL import Image
from cpp_bridge_ritual_runtime import save, sha
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def plan(out):
    """월드/덱/시나리오 상태를 주입하지 않고 실제 문자·메뉴·Config만 사용한다."""
    steps = [(230, 'require', r'phase\t4\n'), (235, 'report', 'initial'),
             (240, 'script-command', 'Config,rotmode=0'), (245, 'report', 'zero'),
             (250, 'key', 'C'), (255, 'report', 'counterclockwise'),
             (260, 'keyup', 'C'), (265, 'report', 'released'),
             (270, 'key', 'ctrl+shift+alt+c'), (275, 'report', 'clockwise'),
             (280, 'key', 'shift+f9'), (285, 'report', 'paused'),
             (290, 'key', 'C'), (295, 'report', 'pausedKey'),
             (300, 'key', 'shift+f9'), (305, 'report', 'resumed'),
             (310, 'esc', ''), (315, 'click', 'Game'), (320, 'click', 'Leave Mission'),
             (325, 'report', 'modal'), (330, 'key', 'C'), (335, 'report', 'modalKey'),
             (340, 'click', 'Continue Mission'), (345, 'report', 'continued'),
             (900, 'report', 'stocked'), (910, 'key', 'Q'), (915, 'report', 'cursor'),
             (920, 'right', 'screen:600,400'), (925, 'report', 'rotatedClockwise'),
             (930, 'key', 'C'), (935, 'report', 'flipped'),
             (940, 'right', 'screen:600,400'), (945, 'report', 'rotatedCounterclockwise'),
             (945, 'snapshot-presented', str(out / 'cursor.bmp')), (950, 'esc', ''), (955, 'report', 'cancelled'),
             (960, 'esc', ''), (965, 'click', 'Options'), (970, 'report', 'options'),
             (980, 'esc', ''), (990, 'esc', ''), (995, 'click', 'Game'),
             (1000, 'click', 'Leave Mission'), (1010, 'click', 'Main Menu'),
             (1040, 'report', 'main'), (1050, 'key', 'C'), (1060, 'report', 'mainKey')]
    for frame in range(12, 220, 8):  # 실제 표시한 초기 안내 버튼만 닫고 비동기 설치를 기다린다.
        for label in BUTTONS:  # 알려진 계속 버튼을 승패/미션 전환 명령으로 바꾸지 않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.sort(key=lambda row: row[0])
    return steps


def inspect(out):
    """종료0과 같은 현재 옵션/실제 회전·취소·메뉴 체크·보호 파일만 판정한다."""
    before = json.loads((out / 'protected-before.json').read_text('utf-8'))
    after = json.loads((out / 'protected-after.json').read_text('utf-8'))
    assert before == after
    run = json.loads((out / 'run/run.json').read_text('utf-8'))
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(out / 'run/states.tsv')
    expected = dict(zero=0, counterclockwise=1, released=1, clockwise=0, paused=0, pausedKey=0, resumed=0, modal=0, modalKey=0, continued=0, stocked=0, cursor=0, rotatedClockwise=0, flipped=1, rotatedCounterclockwise=1, cancelled=1, options=1, main=1, mainKey=1)  #메인 부모도 원문과 같은 모달이므로 C를 차단한다.
    for label, mode in expected.items():  # 같은 값을 기대하는 두 실제 관측 대상도 각각 읽는다.
        assert observed[label]['rotation_mode'] == [str(mode), str(mode)], (label, observed[label].get('rotation_mode'))
    assert observed['paused']['paused'] == observed['pausedKey']['paused'] == ['1'] and observed['resumed']['paused'] == ['0']
    assert observed['modal']['page'] == observed['modalKey']['page'] == ['ABORT']
    assert observed['modal']['raw_html_root'] == observed['modalKey']['raw_html_root']
    cursor = observed['cursor']['raw_cursor']
    assert cursor[0] == '0' and cursor[2] == '82', cursor
    direction = int(observed['cursor']['raw_piece'][1])
    assert int(observed['rotatedClockwise']['raw_piece'][1]) == (direction + 2) % 8
    assert int(observed['rotatedCounterclockwise']['raw_piece'][1]) == direction
    assert observed['flipped']['raw_cursor'][3] == cursor[3] == observed['rotatedCounterclockwise']['raw_cursor'][3]
    assert observed['cancelled']['raw_cursor'][0] != '0'
    items = [row for row in menu_items(out / 'run/states.tsv', 'options') if row['visible'] and 'Counterclockwise Piece Rotation' in row['label']]
    assert len(items) == 1 and items[0]['flags'] & 4, items
    assert observed['main']['phase'] == observed['mainKey']['phase'] == ['0'] and 'mission' not in observed['mainKey']
    image = Image.open(out / 'cursor.bmp').convert('RGB')
    colors = len(image.getcolors(image.width * image.height))
    assert image.size == (1024, 768) and colors > 16
    image.save(out / 'cursor.png')
    keys = ('frame', 'phase', 'page', 'time', 'rotation_mode', 'paused', 'raw_cursor', 'raw_piece', 'mission', 'raw_session', 'shared_gameplay_globals')
    result = dict(unit=717, passed=True, processes=1, exeSha256=run['binarySha256'], seconds=run['seconds'], protectionUnchanged=True, cursorSource=int(cursor[3]), directions=[direction, (direction + 2) % 8, direction], menu=items[0], capture=dict(size=list(image.size), rgbColors=colors, sha256=sha(out / 'cursor.bmp')), observations={label: {key: current[key] for key in keys if key in current} for label, current in observed.items()}, scope='앱 내 C/c·수정키/뗌·정지/모달과 같은 실제 다리 커서의 우클릭 양방향 회전/취소·메뉴 체크·메인 모달의 C 차단을 확인한다. 물리 입력/전체 원문 픽셀·자연 완주·PCM·포획/희생/특정 적 공격/승리·공중3종 전용·MP/통신 검사는 아니다.')
    save(out / 'summary.json', result)
    return result


def main():
    """새 경로에서 같은 제품만 실행하며 기존 관측의 재분석은 새 게임 없이 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify-existing', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    if args.verify_existing:
        assert out.is_dir() and not (out / 'summary.json').exists()
    else:
        out.mkdir(parents=True, exist_ok=False)
        save(out / 'protected-before.json', protection())
        try:
            protected_run(out / 'run', 1024, 768, plan(out), 1080, mission='tutorial1', timeout=180)
        finally:
            save(out / 'protected-after.json', protection())
    result = inspect(out)
    print(json.dumps({key: value for key, value in result.items() if key != 'observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
