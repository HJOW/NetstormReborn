#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 시나리오의 실제 Edit 배치·Test Battle과 공격/방어 건물12종 메뉴를 대조한다."""
import argparse
import re
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states
from cpp_solo_entry_smoke import BUTTONS
from shp import load_type_texts
from typefile import parse_type

# 기존 사제·워크샾·신전·제단과 분리한 공격/방어12자산이다. 공중 이동3종은 포함하지 않는다.
CASES = ((71, 'sunArcher'), (78, 'sunBlocker'), (79, 'thunderBlocker'), (128, 'sunCannon'),
         (129, 'rainCannon'), (131, 'thunderCannon'), (136, 'sunFence'), (140, 'windArcher'),
         (142, 'windBlocker'), (145, 'rainFence'), (148, 'thunderArcher'), (149, 'thunderFence'))
# 이 배치의 실제 불투명 몸체를 누른다. 현재 프레임 사각형 중심은 투명하거나 옆 건물과 겹친다.
BODY_POINTS = {'sunArcher': 'cell:119,153', 'sunFence': 'cell:152,162'}


def menu_plan(output, start):
    """각 실제 건물의 우클릭→About→바깥 클릭 반납을 같은 간격으로 구성한다."""
    steps = []
    for index, (_, name) in enumerate(CASES):  # 다른 건물의 메뉴가 다음 대상에 남지 않게 실제 반납을 거친다.
        frame = start + index * 36
        steps.extend([(frame, 'right', BODY_POINTS.get(name, 'world:' + name)), (frame + 8, 'report', name + '-menu'),
                      (frame + 8, 'snapshot', str(output / (name + '-menu.bmp'))),
                      (frame + 12, 'click', 'About'), (frame + 20, 'report', name + '-help'),
                      (frame + 20, 'snapshot', str(output / (name + '-help.bmp'))),
                      (frame + 24, 'outside', ''), (frame + 30, 'report', name + '-closed')])
    return steps


def verify(report, run, size, expected=None):
    """보호 원본의 비용/설명과 실제 대상·항목·HP·Help/반납을 대조한다."""
    observed = states(report)
    raw_items = repeated(report, 'raw_menu_item')
    roots = repeated(report, 'raw_menu_object')
    bodies = repeated(report, 'raw_help_body')
    help_windows = repeated(report, 'raw_help')
    fields = repeated(report, 'raw_asset_fields')
    texts = load_type_texts(ROOT / 'originals')
    rows = []
    for type_number, name in CASES:  # 기대 목록은 현재 메뉴가 아닌 원본 자산과 앞선 실제 배치 좌표로 정한다.
        props = {key.lower(): value for key, value in parse_type(texts[name.lower()]).props.items()}
        menu, help_label, closed = name + '-menu', name + '-help', name + '-closed'
        selected = [row for row in roots[menu] if int(row[1])]
        assert len(selected) == 1 and int(selected[0][2]), (name, selected)
        sid = int(selected[0][1])
        target = observed[menu]['raw_objects'][sid]
        assert target[:2] == [str(type_number), '1'], (name, target)
        assert observed[menu]['objects'][sid][0] == name
        if expected:
            assert target[3:5] == expected[name], (name, target, expected[name])
        items = [row for row in raw_items[menu] if row[5] == '1' and row[0] == selected[0][0]]
        refund = int(float(props['cost']) / 4)
        assert [row[-1] for row in items] == [f'Salvage gains {refund}', 'About', 'Player'], (name, items)
        assert int(items[0][3]) == 0x0200005d and int(items[0][4]) == sid and not int(items[0][2]) & 0x200
        assert int(items[1][3]) == 0x02000017 and int(items[1][4]) == type_number
        assert not any('CHEAT' in row[-1].upper() for row in raw_items[menu] if row[5] == '1')
        hp = [row for row in fields[menu] if int(row[0]) == sid]
        assert len(hp) == 1 and int(hp[0][3]) == int(props['maxhitpoints']), (name, hp)
        assert len(help_windows[help_label]) == 1 and any(str(props['description']) in row[-1] for row in bodies[help_label]), (name, bodies[help_label])
        assert not help_windows[closed] and observed[closed]['raw_menus'][:3] == ['0', '0', '0']
        for label in (menu, help_label, closed):  # 모드/활성·비정지·보존 자금과 같은 실제 대상이 유지돼야 한다.
            current = observed[label]
            assert current['size'] == [str(size[0]), str(size[1])] and current['phase'] == ['4'] and current['paused'] == ['0']
            assert current['raw_world_modes'] == ['0', '1'] and current['mouse_cursor'][1] == '1'
            assert float(current['raw_sp'][1]) == 500000
            assert current['raw_objects'][sid][:2] == target[:2] and current['raw_objects'][sid][3:5] == target[3:5]
        rows.append(dict(type=type_number, asset=name, sid=sid, position=target[3:5],
                         description=props['description'], hp=int(hp[0][3]), refund=refund,
                         items=items, helpReturned=True))
    return dict(size=list(size), buildings=rows, run=run, binarySha256=run['binarySha256'])


def one(output, data, size, prepare, expected=None):
    """새 사본에서 실제 UI만 조작하며 모드 전환/저장 결과와 두 번째 재생을 분리한다."""
    output.mkdir(parents=True, exist_ok=False)
    if prepare:
        steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
                 (18, 'click', 'TEST02'), (55, 'report', 'editor')]
        for index in range(9):  # Edit도 에너지가 필요하므로 실제 세 종류 제네레이터를 각각3개 배치한다.
            frame = 60 + index * 16
            point = f'placement-near:{136 + index % 3 * 4},{150 + index // 3 * 4}'
            steps.extend([(frame, 'click', f'production:{75 + index // 3}'),
                          (frame + 3, 'up', point), (frame + 5, 'click', point)])
        for index, (type_number, _) in enumerate(CASES):  # 공급/공간 판정이 허용한 현재 마우스 위치만 클릭한다.
            frame = 210 + index * 16
            point = f'placement-near:{128 + index % 4 * 8},{147 + index // 4 * 8}'
            steps.extend([(frame, 'click', f'production:{type_number}'),
                          (frame + 3, 'up', point), (frame + 5, 'click', point)])
        steps.extend([(410, 'report', 'placed'), (420, 'outside', ''), (430, 'esc', ''),
                      (435, 'click', 'Game'), (440, 'click', 'Test Battle')])
        for frame in range(450, 546, 8):  # 같은 TEST02의 현재 브리핑 버튼만 정상 입력으로 넘긴다.
            for label in BUTTONS:  # 이미 시작한 일반 게임에는 안내 클릭을 더하지 않는다.
                condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
                steps.append((frame, 'when', condition + ';;click;;' + label))
        steps.extend([(550, 'report', 'briefing'), (560, 'click-if', 'Go!'), (580, 'report', 'initial')])  # TEST02의 A.안내는 phase4 안에도 나타나므로 실제 버튼을 별도로 반납한다.
        start, frames = 600, 1040
    else:
        steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (40, 'report', 'initial')]
        start, frames = 60, 500
    steps.extend(menu_plan(output, start))
    script, report = output / 'steps.tsv', output / 'states.tsv'
    script.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame, op, arg in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', str(frames), '--set',
               f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(script), '--ui-report', str(report)]
    if not prepare:
        command.extend(['--mission', 'TEST02'])
    original, before, run = snapshots(ROOT / 'originals'), snapshots(data), {}
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 180)
    finally:
        run.update(originalsUnchanged=original == snapshots(ROOT / 'originals'), dataUnchanged=before == snapshots(data))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    if prepare:
        placed = states(report)['placed']['raw_objects']
        expected = {}
        for type_number, name in CASES:  # 실제 편집 배치 결과를 다음 프로세스의 위치 기준으로 확정한다.
            objects = [row for row in placed.values() if row[:2] == [str(type_number), '1']]
            assert len(objects) == 1, (name, objects)
            expected[name] = objects[0][3:5]
    result = verify(report, run, size, expected)
    save(output / 'summary.json', result)
    return result


def main():
    """1280 Edit 저장본과 같은 Fort를1024 새 프로세스에서 읽어 두 화면비를 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    binary = sha(GAME)
    first_data = output / 'data-1280x720'
    shutil.copytree(source, first_data)
    first = one(output / '1280x720', first_data, (1280, 720), True)
    saved = first_data.with_name(first_data.name + '-editor') / 'd/TEST02.fort'
    assert saved.exists()
    second_data = output / 'data-1024x768'
    shutil.copytree(source, second_data)
    shutil.copy2(saved, second_data / 'd/TEST02.fort')
    expected = {row['asset']: row['position'] for row in first['buildings']}
    second = one(output / '1024x768', second_data, (1024, 768), False, expected)
    assert first['binarySha256'] == second['binarySha256'] == binary == sha(GAME)
    save(output / 'summary.json', dict(results=[first, second], savedFortSha256=sha(saved),
                                     binarySha256=binary, attackFlyersNotTested=True))
    print('실제 배치·Test Battle·재시작 및 공격/방어12종의 두 화면비 메뉴/Help24사례 통과')


if __name__ == '__main__':
    main()
