#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원래 BreakingThrough 제단의 실제 메뉴·레벨 조회·Help 왕복을 두 화면비에서 검사한다."""
import argparse
import re
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_solo_entry_smoke import BUTTONS
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 가운데의 사제와 구별되는 원래 제단의 왼쪽 본체 위치다.
ALTAR_POINT = 'cell:163,123'
# 서로 다른 화면비에서 같은 원본 객체를 검사한다.
SIZES = ((1280, 720), (1024, 768))


def one(output, data_root, size):
    """현재 표시된 브리핑만 넘기고 적 소유 제단의 실제 대상·항목·Help 반납을 확인한다."""
    steps = []  # 원문 안내의 존재하는 버튼만 정상 입력으로 누른다.
    for frame in range(12, 72, 8):
        # 화면에 없는 버튼과 이미 시작한 미션에는 클릭을 추가하지 않는다.
        for label in BUTTONS:
            condition = (r'phase\t[13]\n&&(?:control\t' + re.escape(label)
                         + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)')
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.extend([(76, 'report', 'initial'), (80, 'key', 'right'), (94, 'keyup', 'right'),
                  (98, 'report', 'position'), (100, 'right', ALTAR_POINT),
                  (110, 'report', 'menu'), (110, 'snapshot', str(output / 'menu.bmp')),
                  (120, 'click', 'About'), (130, 'report', 'help'),
                  (130, 'snapshot', str(output / 'help.bmp')),
                  (145, 'outside', ''), (155, 'report', 'closed')])
    report, run = protected_run(output, *size, steps, 165, data_root=data_root, mission='breakingthrough')
    observed = states(report)
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1 and int(roots[0][2]), roots
    root = roots[0]
    sid = int(root[1])  # 실제 객체 Form이 참조하는 대상만 검사한다.
    target = observed['menu']['objects'][sid]
    assert target[0] == 'altar' and target[1] == '2' and target[3:5] == ['168', '125'], target
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    assert [row['label'] for row in items] == ['About', 'Player'], items
    assert items[0]['command'] == 0x02000017 and items[0]['parameter'] == 164
    assert not items[0]['flags'] & 0x200
    headings = [row[-1] for row in repeated(report, 'raw_menu_heading')['menu'] if row[0] == root[0]]
    assert headings and any('Level' in row for row in headings), headings
    bodies = repeated(report, 'raw_help_body')['help']
    assert len(repeated(report, 'raw_help')['help']) == 1 and any('Altar' in row[-1] for row in bodies), bodies
    assert not repeated(report, 'raw_help')['closed']
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    for label in ('menu', 'help', 'closed'):
        # 실제 활성 창과 비정지 미션을 확인하며 원격 데스크톱 직사각형은 분석하지 않는다.
        assert observed[label]['mouse_cursor'][1] == '1' and observed[label]['paused'] == ['0']
    result = dict(size=size, selectedSid=sid, target=target, root=root, headings=headings,
                  items=items, helpReturned=True, run=run)
    save(output / 'summary.json', result)
    return result


def one_editor(output, data, size):
    """원본 맵의 제단을 편집기로 열어 메뉴·Help와 실제 Destroy의 다이스 동반 정리를 검사한다."""
    output.mkdir(parents=True, exist_ok=False)
    source = data  # 이전 실행의 별도 편집 저장 경로를 다시 읽지 않도록 사례마다 새 입력 사본을 쓴다.
    data = output / 'data'
    shutil.copytree(source, data)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
             (18, 'click', 'breakingthrough'), (55, 'report', 'editor'),
             (60, 'key', 'right'), (74, 'keyup', 'right'), (78, 'report', 'position'),
             (80, 'right', ALTAR_POINT), (90, 'report', 'menu'),
             (90, 'snapshot', str(output / 'menu.bmp')), (100, 'click', 'About'),
             (110, 'report', 'help'), (110, 'snapshot', str(output / 'help.bmp')),
             (120, 'outside', ''), (125, 'report', 'closed'),
             (130, 'right', ALTAR_POINT), (140, 'report', 'menu-again'),
             (150, 'click', 'Destroy'), (160, 'report', 'deleted')]
    plan, report = output / 'steps.tsv', output / 'states.tsv'
    plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame, op, arg in steps), encoding='utf-8')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', '170', '--set',
               f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(plan), '--ui-report', str(report)]
    before, original, run = snapshots(data), snapshots(ROOT / 'originals'), {}
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 120)
    finally:
        run.update(dataUnchanged=before == snapshots(data), originalsUnchanged=original == snapshots(ROOT / 'originals'))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['dataUnchanged'] and run['originalsUnchanged'], run
    observed = states(report)
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1 and int(roots[0][2]), roots
    root, sid = roots[0], int(roots[0][1])
    assert observed['menu']['objects'][sid][0:2] == ['altar', '2']
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    assert [row['label'] for row in items] == ['Destroy', 'About', 'CHEAT Set Owner', 'Player'], items
    destroy = items[0]
    assert destroy['command'] == 0x03000015 and destroy['parameter'] == sid and not destroy['flags'] & 0x200
    assert len(repeated(report, 'raw_help')['help']) == 1 and not repeated(report, 'raw_help')['closed']
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    dais = [number for number, row in observed['menu']['objects'].items()
            if row[0] == 'dais' and row[3:5] == ['168', '125']]
    assert len(dais) == 1
    assert sid not in observed['deleted']['objects'] and dais[0] not in observed['deleted']['objects']
    assert observed['deleted']['raw_menus'][:3] == ['0', '0', '0']
    for label in ('menu', 'help', 'closed', 'deleted'):
        # 편집기 자체의 정지 정책과 별개로 실제 입력을 받는 활성 창임을 확인한다.
        assert observed[label]['mouse_cursor'][1] == '1'
    result = dict(mode='editor', size=size, selectedSid=sid, daisSid=dais[0], root=root,
                  items=items, helpReturned=True, altarAndDaisDestroyed=True, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본 실행 경로를 거부하고 새 관측 폴더에 같은 제품의 두 사례를 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--editor-data-root', type=Path)
    args = parser.parse_args()
    output, data = args.output.resolve(), args.data_root.resolve()
    assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 실패한 관측을 다른 화면비의 성공으로 덮지 않는다.
    for size in SIZES:
        results.append(one(output / f'{size[0]}x{size[1]}', data, size))
    if args.editor_data_root:
        editor = args.editor_data_root.resolve()
        assert all(editor != (ROOT / name).resolve() and (ROOT / name).resolve() not in editor.parents
                   for name in ('originals', 'originalCD'))
        # 편집 제단도 원래 소유자를 유지하고 같은 현재 게임 제품으로 검사한다.
        for size in SIZES:
            results.append(one_editor(output / f'editor-{size[0]}x{size[1]}', editor, size))
    save(output / 'summary.json', dict(results=results, attackFlyersNotReproduced=True))
    print('원래 적 제단 실제 우클릭/Help 검사 통과: ' + str(len(results)))


if __name__ == '__main__':
    main()
