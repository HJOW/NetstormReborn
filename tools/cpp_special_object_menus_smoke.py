#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""크리스탈과 원래 캠페인 비석의 우클릭/실제 Help 선택을 두 화면비에서 확인한다."""
import argparse
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, save
from cpp_carrier_hover_smoke import variant
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

# 두 자산은 공통 메뉴 진입이지만 크리스탈과 비석의 파생 작성기는 서로 다르다.
CASES = (('crystal', 'TEST02', 'Go!', 'cell:144,185', 'nugget', 154, 'Storm Crystal'),
         ('obelisk', 'dissolvedalliance', 'Play Mission', 'cell:88,119', 'buried', 112, 'Obelisk'))
# 전체화면이나 자연 완주를 이 짧은 창 모드 검사에 합산하지 않는다.
SIZES = ((1024, 768), (1280, 720))


def one(output, data_root, size, case):
    """원시 메뉴의 실제 대상/명령을 확인하고 About을 클릭해 같은 Help 창을 연다."""
    name, mission, start, point, kind, type_number, title = case
    steps = [(5, 'report', 'briefing'), (10, 'click', start), (20, 'report', 'initial')]
    if name == 'obelisk':
        # 원래 왼쪽 덱 뒤에 있는 비석을 실제 방향키로 화면 안에 가져오고 반드시 키를 뗀다.
        steps.extend([(25, 'key', 'left'), (33, 'keyup', 'left')])
    steps.extend([(40, 'report', 'position'), (45, 'right', point), (53, 'report', 'menu'),
                  (53, 'snapshot', str(output / 'menu.bmp')), (60, 'click', 'About'),
                  (68, 'report', 'help'), (68, 'snapshot', str(output / 'help.bmp')),
                  (75, 'esc', ''), (83, 'report', 'escaped'), (90, 'outside', ''),
                  (98, 'report', 'closed')])
    report, run = protected_run(output, *size, steps, 104, data_root=data_root, mission=mission)
    observed = states(report)
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1, roots
    root = roots[0]
    sid = int(root[1])
    assert observed['menu']['objects'][sid][0] == kind and int(root[2]) != 0
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    about = [row for row in items if row['command'] == 0x02000017]
    assert len(about) == 1 and about[0]['parameter'] == type_number, items
    assert {row['command'] for row in items} <= {0x02000017, 0}, items
    assert not any(row['command'] in (0x0200005d, 0x0200002d, 0x030000ce) for row in items)
    if name == 'obelisk':
        assert len(items) == 1, items
    headings = repeated(report, 'raw_menu_heading')['menu']
    assert any(row[0] == root[0] and row[-1] == title for row in headings), headings
    help_rows = repeated(report, 'raw_help')['help']
    bodies = repeated(report, 'raw_help_body')['help']
    assert len(help_rows) == 1 and bodies, (help_rows, bodies)
    assert any(title.casefold() in row[-1].casefold() for row in bodies), bodies
    # Quick Help의 ESC는 공통 부모에 위임된다. 실제 바깥 클릭의 Kill/반납을 닫기 증거로 사용한다.
    assert not repeated(report, 'raw_help')['closed']
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    for label in ('menu', 'help', 'escaped', 'closed'):
        # 비활성 창이나 미션 정지의 관측을 실제 우클릭 성공으로 받아들이지 않는다.
        assert observed[label]['mouse_cursor'][1] == '1' and observed[label]['paused'] == ['0']
    result = dict(name=name, size=size, kind=kind, type=type_number, objectSid=sid,
                  root=root, items=items, helpWindow=help_rows[0],
                  helpWindowsAfterEscape=len(repeated(report, 'raw_help')['escaped']),
                  closedMenus=observed['closed']['raw_menus'][:3], run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """원본 경로를 거부하며 크리스탈 변형은 같은 저장 형식의 사본에서만 잠시 적용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    args = parser.parse_args()
    output, data = args.output.resolve(), args.data_root.resolve()
    assert not any(data == (ROOT / name).resolve() or (ROOT / name).resolve() in data.parents
                   for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 타입 교환을 쓴 크리스탈과 원래 비석 결과를 구별해 기록한다.
    for case in CASES:
        # 공격형 공중3종은 이 도구의 생성/생산/선택 대상에 포함하지 않는다.
        group = output / case[0]
        with variant(data, {'geyser': 'nugget'} if case[0] == 'crystal' else {}, group):
            for size in SIZES:
                # 각 화면비는 새 Session에서 동일한 실제 입력 경로를 사용한다.
                results.append(one(group / f'{size[0]}x{size[1]}', data, size, case))
    save(output / 'summary.json', dict(results=results, crystalUsesExplicitTypeSwap=True,
                                     obeliskUsesOriginalCampaignObject=True, attackFlyersNotTested=True))
    print('크리스탈/비석 실제 우클릭과 Help 왕복4사례 통과')


if __name__ == '__main__':
    main()
