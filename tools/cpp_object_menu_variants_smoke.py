#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TEST02의 실제 우클릭으로 전용 메뉴와 기존 이동/건물형 메뉴를 함께 확인한다."""
import argparse
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

# 화면 안의 원래 객체만 클릭한다. cell은 실제 월드 좌표로부터 기존 Point API가 구한 정상 클릭이다.
CASES = (('generator', 'world:windBattery', 'windBattery'),
         ('temple', 'world:thunderVortex', 'thunderVortex'),
         ('geyser', 'cell:144,185', 'geyser'),
         ('bridge', 'cell:119,146', 'bridge'),
         ('foliage', 'cell:123,160', 'edgeFarm'),
         ('workshop', 'world:windFactory', 'windFactory'),
         ('priest', 'world:priest', 'priest'),
         ('walker', 'world:windWalker', 'windWalker'),
         ('airship', 'world:windBalloon', 'windBalloon'),
         ('whirlibase', 'world:sunAviary', 'sunAviary'))


def one(output, data_root, name, point, kind):
    """선택 대상과 공통 메뉴의 실제 Form 참조/표시 항목·닫기를 확인한다."""
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (20, 'report', 'initial'),
             (25, 'right', point), (35, 'report', 'menu'), (35, 'snapshot', str(output / 'menu.bmp'))]
    if name == 'temple':
        steps.extend([(40, 'click', 'View Netstorm Knowledge'), (47, 'report', 'knowledge'),
                      (47, 'snapshot', str(output / 'knowledge.bmp')), (50, 'esc', ''), (57, 'report', 'escaped'),
                      (60, 'esc', ''), (67, 'report', 'closed')])
    else:
        steps.extend([(40, 'esc', ''), (45, 'report', 'escaped'), (50, 'esc', ''), (57, 'report', 'closed')])
    report, run = protected_run(output, 1280, 720, steps, 72 if name=='temple' else 62, data_root=data_root, mission='TEST02')
    observed = states(report)
    menu = observed['menu']
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1]) != 0]
    assert len(roots) == 1, (name, roots)
    root = roots[0]
    sid = int(root[1])
    assert menu['objects'][sid][0] == kind, (name, menu['objects'][sid])
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    assert items and menu['mouse_cursor'][1] == '1' and menu['paused'] == ['0']
    assert int(root[2]) != 0, '실제 객체 메뉴의 Form 참조 누락'
    labels = [row['label'] for row in items]
    if name == 'generator':
        assert 'Meltdown!' not in labels, '미션 전용 원본 관문 누락'
    if name == 'temple':
        assert any(row['command'] == 0x020000a0 for row in items)
        assert len(repeated(report, 'raw_menu')['knowledge']) > len(repeated(report, 'raw_menu')['menu'])
        assert observed['escaped']['game_menu_bar'][1]=='1'  # 원본 ESC는 지식 팝업을 반납한 뒤 가로막대를 연다.
    if name == 'foliage':
        assert len(items) == 1 and items[0]['command'] == 0x02000017, labels
    if name == 'bridge':
        assert not any(row['command'] in (0x02000125, 0x02000126, 0x02000127) for row in items), '일반 플레이에 편집 전용 항목 노출'
    if name == 'geyser':
        headings = [row[-1] for row in repeated(report, 'raw_menu_heading')['menu'] if row[0] == root[0]]
        assert any('Storm Power:' in label for label in headings), headings
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    result = dict(name=name, kind=kind, selectedSid=sid, labels=labels, root=root,
                  menusAfterEscape=observed['closed']['raw_menus'][:3], run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """새 출력 폴더에 현재 제품의 실제 메뉴 결과를 보존한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--only', nargs='*')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 앞 사례 실패를 뒤 성공으로 덮지 않는다.
    for name, point, kind in CASES:  # 원래 맵의 이동/건물/환경 객체를 종류별로 확인한다.
        if args.only and name not in args.only:
            continue
        results.append(one(output / name, args.data_root.resolve(), name, point, kind))
    if not results:
        raise RuntimeError('선택한 실제 우클릭 검사 사례 없음')
    save(output / 'summary.json', dict(results=results))
    print('실제 우클릭 메뉴 검사 통과: ' + str(len(results)))


if __name__ == '__main__':
    main()
