#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early1의 숫자/Shift 기호 저장·미니맵·정지/모달·재시작 수명을 확인한다(719)."""
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
    """실제 최초/재시작 Guide가 표시한 계속 버튼만 닫는다."""
    result = []
    for frame in range(first, last, 8):  # 비동기 설치를 고정 성공/대본 상태 주입으로 바꾸지 않는다.
        for label in BUTTONS:  # 승리나 완료 명령을 누르지 않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            result.append((frame, 'when', condition + ';;click;;' + label))
    return result


def plan(out):
    """현재 입력 큐에 실제 ASCII와 상위 수정 비트를 보내고 저장값은 직접 쓰지 않는다."""
    steps = [(230, 'require', r'phase\t4\n'), (235, 'report', 'initial'),
             (240, 'key', '1'), (250, 'report', 'empty'),
             (260, 'key', 'shift+!'), (270, 'report', 'savedOne'),
             (280, 'click', 'minimap:40,37'), (290, 'report', 'panned'),
             (300, 'key', 'shift+)'), (310, 'report', 'savedZero'),
             (320, 'key', '1'), (330, 'report', 'recalledOne'), (330, 'snapshot-presented', str(out / 'recalled.bmp')),
             (340, 'keyup', '1'), (350, 'report', 'released'),
             (360, 'key', 'shift+1'), (370, 'report', 'digitWithShift'),
             (380, 'key', 'ctrl+shift+!'), (390, 'report', 'modifiedSymbol'),
             (400, 'key', 'shift+f9'), (410, 'report', 'paused'),
             (420, 'key', '0'), (430, 'report', 'pausedRecall'),
             (440, 'key', 'shift+!'), (450, 'report', 'pausedSave'),
             (460, 'key', 'shift+f9'), (470, 'report', 'resumed'),
             (480, 'key', '0'), (490, 'report', 'recalledZero'),
             (500, 'esc', ''), (505, 'click', 'Game'), (510, 'click', 'Leave Mission'),
             (520, 'report', 'modal'), (530, 'key', '1'), (540, 'key', 'shift+!'), (550, 'report', 'modalKeys'),
             (560, 'click', 'Continue Mission'), (570, 'report', 'continued'),
             (580, 'esc', ''), (585, 'click', 'Game'), (590, 'click', 'Restart Mission'),
             (810, 'require', r'phase\t4\n'), (820, 'report', 'restarted'),
             (830, 'key', '1'), (840, 'report', 'revisited'), (840, 'snapshot-presented', str(out / 'revisited.bmp'))]
    steps.extend(continue_steps(12, 220))
    steps.extend(continue_steps(600, 800))
    steps.sort(key=lambda row: row[0])
    return steps


def point(words):
    """실제 보고의 원시 단정도 두 좌표를 해석한다."""
    return [struct.unpack('<f', struct.pack('<I', int(word)))[0] for word in words]


def center_pixels(position):
    """같은 기본16/11칸과1024×768의 실제 즉시 중심 결과를 기대한다."""
    return [int(position[0] * 16) - 512, int(position[1] * 11) - 384]


def inspect(out):
    """보호 복원과 각 실제 저장/호출 관측을 각각 판정한다."""
    before = json.loads((out / 'protected-before.json').read_text('utf-8'))
    after = json.loads((out / 'protected-after.json').read_text('utf-8'))
    assert before == after
    run = json.loads((out / 'run/run.json').read_text('utf-8'))
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(out / 'run/states.tsv')
    initial = observed['initial']['saved_camera']
    assert initial == [str(0xc61c4000)] * 20
    assert observed['empty']['saved_camera'] == initial and observed['empty']['camera'] == observed['initial']['camera']
    one = observed['savedOne']['saved_camera']
    one_point = point(one[:2])
    assert all(0 <= value < 256 for value in one_point) and one[2:] == initial[2:]
    assert observed['savedOne']['camera'] == observed['initial']['camera']
    assert observed['panned']['camera'] != observed['savedOne']['camera'] and observed['panned']['saved_camera'] == one
    both = observed['savedZero']['saved_camera']
    zero_point = point(both[18:20])
    assert all(0 <= value < 256 for value in zero_point) and both[:18] == one[:18] and zero_point != one_point
    assert observed['savedZero']['camera'] == observed['panned']['camera']
    for label, position in (('recalledOne', one_point), ('released', one_point), ('digitWithShift', one_point), ('modifiedSymbol', one_point), ('recalledZero', zero_point), ('revisited', one_point)):  #저장 자체는 이동/호출·다른 수정키로 바뀌지 않는다.
        row = observed[label]
        assert row['saved_camera'] == both, (label, row['saved_camera'])
        pixels = center_pixels(position)
        assert list(map(int, row['camera'])) == pixels, (label, row['camera'], pixels)
        assert row['raw_camera_frame'][:4] == list(map(str, pixels + pixels)), (label, row['raw_camera_frame'])
    for first, second in (('paused', 'pausedRecall'), ('paused', 'pausedSave'), ('modal', 'modalKeys')):  #정지/현재 확인창은 읽기와 쓰기를 모두 차단한다.
        assert observed[first]['saved_camera'] == observed[second]['saved_camera'] == both
        assert observed[first]['camera'] == observed[second]['camera']
    assert observed['pausedSave']['paused'] == ['1'] and observed['resumed']['paused'] == ['0']
    assert observed['modal']['page'] == observed['modalKeys']['page'] == ['ABORT']
    assert observed['restarted']['saved_camera'] == both
    assert observed['restarted']['shared_surface_storage'] != observed['initial']['shared_surface_storage']
    for row in observed.values():  #별개 최근 배치 이력은 이번 숫자 저장/호출로 바꾸지 않는다.
        assert row['construction_history'] == observed['initial']['construction_history']
    captures = []
    for label in ('recalled', 'revisited'):  #현재 presented 사본의 실제 크기/색을 읽고 열람용 PNG를 만든다.
        image = Image.open(out / (label + '.bmp')).convert('RGB')
        colors = len(image.getcolors(image.width * image.height))
        assert image.size == (1024, 768) and colors > 16
        image.save(out / (label + '.png'))
        captures.append(dict(label=label, size=list(image.size), rgbColors=colors, sha256=sha(out / (label + '.bmp'))))
    keys = ('frame', 'phase', 'page', 'time', 'paused', 'saved_camera', 'construction_history', 'camera', 'raw_camera_frame', 'mission', 'shared_surface_storage')
    result = dict(unit=719, passed=True, processes=1, exeSha256=run['binarySha256'], seconds=run['seconds'], protectedUnchanged=True,
                  savedOne=one_point, savedZero=zero_point, onePixels=center_pixels(one_point), zeroPixels=center_pixels(zero_point), captures=captures,
                  observations={label: {key: row[key] for key in keys if key in row} for label, row in observed.items()},
                  scope='실제 Early1의 Shift 기호 저장/숫자 호출·미니맵·뗌/다른 수정키·정지/모달·미션 재시작의 같은 Client80BYTE다. 앱 내 입력/음소거 음향 요청이며 물리 키/PCM·원문 전체 픽셀·자연 완주·제외 재현/공중3종 전용·MP/통신 검사는 아니다.')
    save(out / 'summary.json', result)
    return result


def main():
    """현재 실행 파일의 새 게임 한 번을 보호 장부와 함께 관측한다."""
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
            protected_run(out / 'run', 1024, 768, plan(out), 855, mission='tutorial1', timeout=150)
        finally:
            save(out / 'protected-after.json', protection())
    result = inspect(out)
    print(json.dumps({key: value for key, value in result.items() if key != 'observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
