#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 TEST02의 사제로 실제 제단을 건설하고 메뉴·도움말·회수 수명을 확인한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

# 원본 TEST02 사제의 초기 위치 주변에서 현재 배치 판정이 허용한 칸만 클릭한다.
BUILD_POINT = 'placement-near:145,175'
# 기존 원문 비용/레벨과 실제 건설 메뉴 관측으로 확인한 표시 라벨이다.
BUILD_LABEL = 'Build Level 1 Altar for 500'
# 이번 실행의 원본4:3과 확장16:9 화면비다.
SIZES = ((1280, 720), (1024, 768))


def verify(report, run, width, height):
    """원시 실제 객체·자금·메뉴·자식 삭제와 재건설 가능 상태를 대조한다."""
    observed = states(report)
    initial = observed['initial']['objects']
    assert not any(row[0] in ('altar', 'dais') for row in initial.values())
    altars = [(sid, row) for sid, row in observed['menu']['objects'].items()
              if row[:2] == ['altar', '1']]
    assert len(altars) == 1, altars
    sid, altar = altars[0]
    dais = [(number, row) for number, row in observed['menu']['objects'].items()
            if row[:2] == ['dais', '1'] and row[3:5] == altar[3:5]]
    assert len(dais) == 1, dais
    dais_sid = dais[0][0]
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1 and int(roots[0][1]) == sid, roots
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    assert [row['label'] for row in items] == ['Salvage gains 125', 'About', 'Player'], items
    assert items[0]['command'] == 0x0200005d and items[0]['parameter'] == sid
    assert not any('CHEAT' in row['label'].upper() for row in items)
    assert float(observed['initial']['raw_sp'][1]) == 500000
    for label in ('placed', 'built', 'menu', 'help', 'helpClosed', 'salvageMenu'):  # 실제 건설 비용은 한 번만 차감돼야 한다.
        assert float(observed[label]['raw_sp'][1]) == 499500, label
    fields = [row for row in repeated(report, 'raw_asset_fields')['menu'] if int(row[0]) == sid]
    assert len(fields) == 1 and int(fields[0][3]) == 1500, fields
    help_rows = repeated(report, 'raw_help_body')['help']
    assert len(repeated(report, 'raw_help')['help']) == 1 and any('Altar' in row[-1] for row in help_rows)
    assert not repeated(report, 'raw_help')['helpClosed']
    for label in ('salvaged', 'settled', 'rebuildMenu', 'closed'):  # 제단과 같은 좌표의 다이스가 함께 사라지고 환불이 중복되지 않아야 한다.
        row = observed[label]
        assert sid not in row['objects'] and dais_sid not in row['objects'], label
        assert not any(obj[0] == 'rune' and obj[3:5] == altar[3:5] for obj in row['objects'].values())
        assert float(row['raw_sp'][1]) == 499625, (label, row['raw_sp'])
    rebuilt = [row for row in menu_items(report, 'rebuildMenu') if row['visible'] and row['label'] == BUILD_LABEL]
    assert len(rebuilt) == 1 and not rebuilt[0]['flags'] & 0x200, rebuilt
    for label in ('menu', 'helpClosed', 'salvaged', 'settled', 'rebuildMenu', 'closed'):  # 비정지 일반 게임과 실제 활성 창의 입력만 수락한다.
        row = observed[label]
        assert row['size'] == [str(width), str(height)] and row['phase'] == ['4'] and row['paused'] == ['0']
        assert row['raw_world_modes'] == ['0', '1'] and row['mouse_cursor'][1] == '1'
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    return dict(size=[width, height], altarSid=sid, daisSid=dais_sid, position=altar[3:5],
                cost=500, refund=125, initialSp=500000, finalSp=499625, altarHp=1500,
                menu=items, helpReturned=True, pairAndRunesRemoved=True,
                buildMenuEnabledAgain=True, run=run)


def one(output, source, width, height):
    """같은 원본 입력의 새 사본에서 실제 건설·메뉴 클릭만 실행한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (20, 'report', 'initial'),
             (25, 'right', 'world:priest'), (35, 'click', 'Construct'),
             (45, 'click', BUILD_LABEL), (55, 'report', 'taken'),
             (60, 'up', BUILD_POINT), (70, 'report', 'cursor'), (75, 'click', BUILD_POINT),
             (90, 'report', 'placed'), (600, 'report', 'built'),
             (610, 'right', 'world:altar'), (620, 'report', 'menu'),
             (620, 'snapshot', str(output / 'menu.bmp')), (630, 'click', 'About'),
             (640, 'report', 'help'), (640, 'snapshot', str(output / 'help.bmp')),
             (650, 'outside', ''), (660, 'report', 'helpClosed'),
             (670, 'right', 'world:altar'), (680, 'report', 'salvageMenu'),
             (690, 'click', 'Salvage gains 125'), (700, 'report', 'salvaged'),
             (700, 'snapshot', str(output / 'salvaged.bmp')), (720, 'report', 'settled'),
             (730, 'right', 'world:priest'), (740, 'click', 'Construct'),
             (750, 'report', 'rebuildMenu'), (750, 'snapshot', str(output / 'rebuild.bmp')),
             (760, 'outside', ''), (770, 'report', 'closed')]
    report, run = protected_run(output / 'run', width, height, steps, 780, data_root=data, mission='TEST02')
    result = verify(report, run, width, height)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본 경로를 거부하고 같은 제품의 두 화면비 결과를 확정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    binary = sha(GAME)
    rows = []
    for width, height in SIZES:  # 같은 사제·기술·자금으로 화면비만 바꿔 순차 실행한다.
        rows.append(one(output / f'{width}x{height}', source, width, height))
        assert rows[-1]['run']['binarySha256'] == binary and sha(GAME) == binary
    save(output / 'summary.json', dict(results=rows, binarySha256=binary,
                                     rawObjectMutation=False, attackFlyersNotTested=True))
    print(f'실제 사제 건설·자기 제단 도움말·회수·재건설 메뉴: {len(rows)}개 통과')


if __name__ == '__main__':
    main()
