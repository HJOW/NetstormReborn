#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""첫 캠페인의 실제 덱 생산물 배치→미니맵→Tab·정지/모달·재시작 뒤 같은 최근 위치를 확인한다(718)."""
import argparse
import json
from pathlib import Path
import re
import struct
from PIL import Image
from cpp_bridge_ritual_runtime import save, sha
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def continue_steps(first, last):
    """원래 브리핑이 실제 표시한 계속 버튼만 닫는다."""
    result = []
    for frame in range(first, last, 8):  # 최초/재시작의 비동기 Guide 완료를 같은 일반 입력으로 기다린다.
        for label in BUTTONS:  # 알려진 계속 버튼만 사용하고 승패/미션 완료를 만들지 않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            result.append((frame, 'when', condition + ';;click;;' + label))
    return result


def plan(out):
    """실제 사제 메뉴의 일반 건설과 카메라 키만 사용하고 이력/월드 상태를 주입하지 않는다."""
    steps = [(230, 'require', r'phase\t4\n'), (235, 'report', 'initial'),
             (240, 'key', 'tab'), (250, 'report', 'empty'),
             (260, 'right', 'world:priest'), (280, 'click', 'Construct'),
             (300, 'click', 'Workshop'), (320, 'click', ' Build Sun Workshop for 800'),
             (330, 'up', 'cell:107,103'), (340, 'report', 'cursor'),
             (350, 'click', 'cell:107,103'), (380, 'report', 'workshop'),
             (1300, 'right', 'world:sunFactory'), (1310, 'click', 'Put Knowledge into Production'),
             (1320, 'click', 'Sun Cannon'), (1340, 'report', 'researched'),
             (1350, 'click', 'production:128'), (1370, 'up', 'cell:94,109'),
             (1380, 'click', 'cell:94,109'), (1390, 'report', 'placed'),
             (1400, 'click', 'minimap:40,37'), (1410, 'report', 'panned'),
             (440, 'key', 'tab'), (450, 'report', 'visited'), (450, 'snapshot-presented', str(out / 'visited.bmp')),
             (460, 'keyup', 'tab'), (470, 'report', 'released'),
             (480, 'key', 'ctrl+shift+alt+tab'), (490, 'report', 'wrapped'),
             (500, 'key', 'shift+f9'), (510, 'report', 'paused'),
             (520, 'key', 'tab'), (530, 'report', 'pausedTab'),
             (540, 'key', 'shift+f9'), (550, 'report', 'resumed'),
             (560, 'esc', ''), (565, 'click', 'Game'), (570, 'click', 'Leave Mission'),
             (580, 'report', 'modal'), (590, 'key', 'tab'), (600, 'report', 'modalTab'),
             (610, 'click', 'Continue Mission'), (620, 'report', 'continued'),
             (630, 'esc', ''), (635, 'click', 'Game'), (640, 'click', 'Restart Mission'),
             (860, 'require', r'phase\t4\n'), (870, 'report', 'restarted'),
             (880, 'key', 'tab'), (900, 'report', 'revisited'), (900, 'snapshot-presented', str(out / 'revisited.bmp'))]
    # 워크숍 완공/연구/덱 배치를 기존 합법 UI 경로로 기다린 뒤 나머지 키·재시작을 실행한다.
    steps = [(frame + 1000 if frame >= 440 and frame < 1000 else frame, operation, value) for frame, operation, value in steps]
    steps.extend(continue_steps(12, 220))
    steps.extend(continue_steps(1650, 1850))
    steps.sort(key=lambda row: row[0])
    return steps


def point(bits):
    """보고된 원시 DWORD 두 개를 화면 이동 계산용 단정도로 해석한다."""
    return [struct.unpack('<f', struct.pack('<I', int(word)))[0] for word in bits]


def inspect(out):
    """같은 실제 보고의 이력/카메라·정지/모달·재시작 수명을 판정한다."""
    before = json.loads((out / 'protected-before.json').read_text('utf-8'))
    after = json.loads((out / 'protected-after.json').read_text('utf-8'))
    assert before == after
    run = json.loads((out / 'run/run.json').read_text('utf-8'))
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(out / 'run/states.tsv')
    initial = observed['initial']['construction_history']
    assert initial == ['0'] + [str(0xc61c4000)] * 10, initial
    assert observed['empty']['construction_history'] == initial and observed['empty']['camera'] == observed['initial']['camera']
    assert observed['workshop']['construction_history'] == initial  # 실제 TYPE85의 group10은 원문 이력 기록에서 제외된다.
    placed = observed['placed']['construction_history']
    position = point(placed[1:3])
    assert placed[0] == '0' and all(0 < value < 256 for value in position) and placed[3:] == initial[3:], placed
    assert observed['panned']['camera'] != observed['placed']['camera']
    pixels = [int(position[0] * 16) - 512, int(position[1] * 11) - 384]
    for label in ('visited', 'released', 'wrapped', 'revisited'):  #같은 기록된 위치를 즉시 중심으로 적용하고 다음 순번1을 유지해야 한다.
        current = observed[label]
        assert current['construction_history'] == ['1'] + placed[1:], (label, current.get('construction_history'))
        assert list(map(int, current['camera'])) == pixels, (label, current.get('camera'), pixels)
        assert current['raw_camera_frame'][:4] == list(map(str, pixels + pixels)), (label, current['raw_camera_frame'])
    for first, second in (('paused', 'pausedTab'), ('modal', 'modalTab')):  #현재 정지/모달 관문은 순번과 카메라를 바꾸지 않는다.
        assert observed[first]['construction_history'] == observed[second]['construction_history']
        assert observed[first]['camera'] == observed[second]['camera']
    assert observed['pausedTab']['paused'] == ['1'] and observed['resumed']['paused'] == ['0']
    assert observed['modalTab']['page'] == ['ABORT'] and observed['modal']['raw_html_root'] == observed['modalTab']['raw_html_root']
    assert observed['restarted']['construction_history'] == ['1'] + placed[1:] and observed['restarted']['phase'] == ['4']
    assert observed['restarted']['shared_surface_storage'][4] != observed['initial']['shared_surface_storage'][4]
    captures = []
    for label in ('visited', 'revisited'):  #실제 presented 출력의 크기/색을 읽고 열람용 PNG를 만든다.
        image = Image.open(out / (label + '.bmp')).convert('RGB')
        colors = len(image.getcolors(image.width * image.height))
        assert image.size == (1024, 768) and colors > 16
        image.save(out / (label + '.png'))
        captures.append(dict(label=label, size=list(image.size), rgbColors=colors, sha256=sha(out / (label + '.bmp'))))
    keys = ('frame', 'phase', 'page', 'time', 'paused', 'construction_history', 'camera', 'raw_camera_frame', 'mission', 'raw_session', 'shared_surface_storage')
    result = dict(unit=718, passed=True, processes=1, exeSha256=run['binarySha256'], seconds=run['seconds'], protectionUnchanged=True, constructedPosition=position, centerPixels=pixels, captures=captures, observations={label: {key: current[key] for key in keys if key in current} for label, current in observed.items()}, scope='첫 캠페인의 실제 워크숍 group10 제외/연구/덱 생산물 배치/로컬 통지→미니맵→Tab·수정키/뗌·정지/모달·재시작 뒤 같은 Client 이력 재방문이다. 앱 내 입력이며 물리 키/전체 원문 픽셀·PCM·자연 완주·포획/희생/특정 적 공격/승리·공중3종 전용·MP/통신 검사는 아니다.')
    save(out / 'summary.json', result)
    return result


def main():
    """새 실행의 전체 보호 장부를 보존하며 기존 관측은 새 게임 없이 분석한다."""
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
            protected_run(out / 'run', 1024, 768, plan(out), 1910, mission='thewarbegins', timeout=180)
        finally:
            save(out / 'protected-after.json', protection())
    result = inspect(out)
    print(json.dumps({key: value for key, value in result.items() if key != 'observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
