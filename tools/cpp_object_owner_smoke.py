#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제단·윈드 타워·워크샾·신전의 Edit 선택/Core·SetOwner 대화와 취소/변경을 검사한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_game_menu_smoke import menu_items
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 원래 BreakingThrough의 제단 외곽이며 겹친 사제를 선택하지 않는 위치다.
POINT = 'cell:163,123'
# 다른 일반 건물형도 원래 맵의 본체 안쪽을 클릭한다. 사제의0번 owner 진단 규칙과 합치지 않는다.
POINTS = {'altar': POINT, 'windBlocker': 'cell:170,123',
          'windFactory': 'cell:180,118', 'windVortex': 'cell:180,125'}
# 같은 원문 객체를 서로 다른 화면비에서 조작한다.
SIZES = ((1280, 720), (1024, 768))


def one(output, source, size, kind='altar'):
    """새 사본에서 원래 대화만 조작하고 현재 대상·소유자·Player Core 선택을 검사한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'  # 앞 사례의 편집 저장 경로와 분리한 실제 입력 사본이다.
    shutil.copytree(source, data)
    point = POINTS[kind]  # 확인한 원래 타입/위치 조합만 사용한다.
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
             (18, 'click', 'breakingthrough'), (55, 'report', 'editor'),
             (60, 'key', 'right'), (74, 'keyup', 'right'), (80, 'right', point),
             (90, 'report', 'menu'), (95, 'click', 'CHEAT Set Owner'),
             (100, 'report', 'dialog-cancel'), (105, 'click', 'Cancel'), (115, 'report', 'cancelled'),
             (120, 'right', point), (130, 'report', 'menu-human'), (135, 'click', 'CHEAT Set Owner'),
             (140, 'report', 'dialog-human'), (140, 'snapshot', str(output / 'dialog.bmp')),
             (145, 'click', 'Human Player'), (155, 'report', 'human'),
             (160, 'right', point), (170, 'report', 'menu-computer'), (180, 'click', 'CHEAT Set Owner'),
             (190, 'report', 'dialog-computer'), (200, 'click', 'Computer #1'), (210, 'report', 'computer'),
             (220, 'right', point), (230, 'report', 'menu-neutral'), (240, 'click', 'CHEAT Set Owner'),
             (250, 'report', 'dialog-neutral'), (260, 'click', 'No Owner'), (270, 'report', 'neutral'),
             (280, 'right', point), (290, 'report', 'menu-final'),
             (290, 'snapshot', str(output / 'neutral-menu.bmp'))]
    plan, report = output / 'steps.tsv', output / 'states.tsv'
    plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame, op, arg in steps), encoding='utf-8')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', '300', '--set',
               f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(plan), '--ui-report', str(report)]
    original, before, run = snapshots(ROOT / 'originals'), snapshots(data), {}
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 120)
    finally:
        run.update(originalsUnchanged=original == snapshots(ROOT / 'originals'), dataUnchanged=before == snapshots(data))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(report)
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1 and int(roots[0][2]), roots
    sid = int(roots[0][1])  # 실제 객체 Form이 참조한 대상과 현재 선택 SID를 대조한다.
    initial = observed['editor']['objects'][sid]
    assert initial[0:2] == [kind, '2'], initial
    if kind == 'altar':
        assert initial[3:5] == ['168', '125']
    assert observed['editor']['selected'] == ['0']
    assert observed['menu']['selected'] == [str(sid)] and observed['menu']['raw_session'] == ['2', '1']
    expected = (('cancelled', '2', '2'), ('human', '1', '1'), ('computer', '2', '2'), ('neutral', '0', '2'))
    transitions = []  # 중립 변경 뒤에는 원문처럼 마지막 비영 현재 Player를 유지한다.
    for label, owner, current in expected:
        row = observed[label]['objects'][sid]
        assert row[0] == initial[0] and row[1] == owner and row[2:7] == initial[2:7], (label, row)
        assert observed[label]['selected'] == [str(sid)] and observed[label]['raw_session'] == [current, '1']
        assert observed[label]['phase'] == ['4'] and observed[label]['mouse_cursor'][1] == '1'
        transitions.append(dict(label=label, owner=int(owner), currentPlayer=int(current), selected=sid))
    for label in ('dialog-cancel', 'dialog-human', 'dialog-computer', 'dialog-neutral'):
        # 원래 Tell.SetOwner 본문과 실제 스크립트 번호를 사용했는지 확인한다.
        bodies = repeated(report, 'raw_html_body')[label]
        assert any('Make this unit owned by which player?' in row[-1] for row in bodies), bodies
        buttons = repeated(report, 'raw_html_button')[label]
        assert any(row[-1] == 'No Owner' and int(row[3]) == 0x020000cc for row in buttons)
        assert observed[label]['selected'] == [str(sid)] and observed[label]['mouse_cursor'][1] == '1'
    final_items = [row for row in menu_items(report, 'menu-final') if row['visible']]
    assert [row['label'] for row in final_items] == ['Destroy', 'About', 'CHEAT Set Owner'], final_items
    if kind in ('windFactory', 'windVortex'):
        for label in ('menu', 'menu-human', 'menu-computer', 'menu-neutral'):
            # 편집 우클릭은 비영 소유자의 현재 Core를 선택하므로 소유 전용 메뉴도 같은 현재 자료를 읽는다.
            labels = [row['label'] for row in menu_items(report, label) if row['visible']]
            assert not any(text.startswith('Salvage') for text in labels), (kind, label, labels)
            if kind == 'windFactory':
                assert 'View Current Production' in labels and 'Put Knowledge into Production' in labels, (label, labels)
            else:
                assert 'View Netstorm Knowledge' in labels, (label, labels)
    result = dict(kind=kind, size=size, selectedSid=sid, root=roots[0], transitions=transitions,
                  neutralMenu=final_items, run=run, attackFlyersNotReproduced=True)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본을 입력 사본 생성의 원천으로도 직접 지정하지 않고 새 관측만 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--kind', choices=tuple(POINTS), default='altar')
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 첫 화면비 실패를 다음 성공으로 덮지 않는다.
    for size in SIZES:
        results.append(one(output / f'{size[0]}x{size[1]}', source, size, args.kind))
    save(output / 'summary.json', dict(results=results))
    print('실제 SetOwner 취소/변경 검사 통과: ' + str(len(results)))


if __name__ == '__main__':
    main()
