#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 사제 건설로 네 워크샾의 건설 중 메뉴 차단과 완공·회수 전환을 검사한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_factory_upgrade_smoke import SALVAGE_POINT
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states
from shp import load_type_texts
from typefile import parse_type

# 네 원소의 실제 원본 TYPE/자산 이름이며 이동 공중 유닛은 생성하지 않는다.
CASES = ((85, 'sunFactory'), (86, 'windFactory'), (87, 'rainFactory'), (88, 'thunderFactory'))
# 원본4:3과 확장16:9에 같은 건설·메뉴 입력을 적용한다.
SIZES = ((1280, 720), (1024, 768))
# 사제 주변에서 현재 게임 배치 규칙이 허용한 지점을 조회해 실제 클릭한다.
BUILD_POINT = 'placement-near:145,175'
# 원래 LevelIII 워크샾과 구별되는 새 배치의 실제 몸체를 월드 칸으로 누른다.
BODY_POINT = 'cell:150,176'


def properties(name):
    """보호 원본의 타입 텍스트에서 설명·비용·최대HP를 독립 추출한다."""
    text = load_type_texts(ROOT / 'originals')[name.lower()]
    return {key.lower(): value for key, value in parse_type(text).props.items()}


def verify(report, run, size, number, name):
    """현재 관측의 동일 서버 객체·건설 비트·자금과 일반 메뉴/실제 삭제를 대조한다."""
    props = properties(name)
    description, cost, maximum = str(props['description']), int(props['cost']), int(props['maxhitpoints'])
    observed, fields = states(report), repeated(report, 'raw_asset_fields')
    initial = observed['initial']['raw_objects']
    original_factories = {sid: row for sid, row in initial.items()
                          if sid >= 15000 and row[0] in ('85', '86', '87', '88') and row[1] == '1'}
    targets = [(sid, row) for sid, row in observed['placed']['raw_objects'].items()
               if sid >= 15000 and sid not in initial and row[:2] == [str(number), '1']]
    assert len(targets) == 1, targets
    sid, target = targets[0]

    def asset(label):
        """같은 실제 보고서에서 대상의 상태/extra/HP 한 행만 읽는다."""
        rows = [row[1:] for row in fields[label] if int(row[0]) == sid]
        assert len(rows) == 1, (label, rows)
        return list(map(int, rows[0]))

    for label in ('placed', 'constructionDenied'):  # 생성 직후 HP0도 유효하며 건설 중 비트가 켜져 있어야 한다.
        raw = asset(label)
        assert raw[1] & 1 and 0 <= raw[2] < maximum, (label, raw)
    assert observed['constructionDenied']['raw_menus'][:3] == ['0', '0', '0']
    assert not repeated(report, 'raw_menu_object')['constructionDenied']
    for label in ('built', 'menu', 'help', 'helpClosed', 'production', 'productionClosed', 'salvageCancelled', 'salvageMenu'):
        # 같은 실제 LevelI 객체와 완공 HP·위치를 유지하는지 확인한다.
        raw = asset(label)
        assert not raw[1] & 1 and raw[0] >> 5 == 0 and raw[2] == maximum, (label, raw)
        assert observed[label]['raw_objects'][sid][:2] == target[:2]
        assert observed[label]['raw_objects'][sid][3:5] == target[3:5]
    root = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(root) == 1 and int(root[0][1]) == sid, root
    items = [row for row in menu_items(report, 'menu') if row['visible']]
    assert all(any(item['label'] == text for item in items) for text in
               ('Put Knowledge into Production', 'View Current Production', 'About', 'Player')), items
    assert len(repeated(report, 'raw_help')['help']) == 1
    assert any(item['label'] == 'About' and item['parameter'] == number for item in items)
    assert any('Workshops build your' in row[-1] for row in repeated(report, 'raw_help_body')['help'])
    assert not repeated(report, 'raw_help')['helpClosed']
    production = [row for row in menu_items(report, 'production') if row['visible']]
    assert any(row['label'] == 'About' and row['parameter'] == 0x60000001 for row in production), production
    assert not [row for row in repeated(report, 'raw_factory_content')['production'] if int(row[0]) == sid]
    refund = cost // 4
    for label in ('salvageWarning', 'salvageAgain'):  # 원본의 회수 경고와 같은 대상 명령이 취소/확정 양쪽에서 유지돼야 한다.
        row = observed[label]
        assert row['page'] == ['SalvageInf'] and row['paused'] == ['0']
        buttons = repeated(report, 'raw_html_button')[label]
        assert any(button[-1] == 'Salvage It!' and int(button[3]) == 0x0200005d and int(button[4]) == sid for button in buttons), buttons
    salvage = [row for row in menu_items(report, 'salvageMenu') if row['visible'] and
               row['command'] == 0x02000095 and row['parameter'] == 0x60000002]
    assert len(salvage) == 1 and salvage[0]['label'] == f'Salvage gains {refund}', salvage
    assert float(observed['initial']['raw_sp'][1]) == 500000
    for label, row in observed.items():  # 실제 건설/환불 이외에 비용이나 중복 지급이 없는지 모든 관측에서 확인한다.
        expected = 500000 if label in ('briefing', 'initial', 'taken', 'cursor') else 500000 - cost
        if label in ('salvaged', 'settled', 'closed'):
            expected += refund
            assert sid not in row['raw_objects'], label
        assert float(row['raw_sp'][1]) == expected, (label, row['raw_sp'])
        if label == 'briefing':
            continue
        assert row['size'] == list(map(str, size)) and row['raw_world_modes'] == ['0', '1']
        assert row['paused'] == ['0'] and row['mouse_cursor'][1] == '1'
        if label not in ('salvageWarning', 'salvageAgain'):
            assert row['phase'] == ['4'], (label, row['phase'])
        assert not any('CHEAT' in item['label'].upper() for item in menu_items(report, label) if item['visible'])
        for existing, original in original_factories.items():  # 기존 워크샾을 새 건물 대신 선택·회수하지 않았는지 별도로 확인한다.
            assert existing in row['raw_objects'], (label, existing)
            assert row['raw_objects'][existing][:2] == original[:2]
            assert row['raw_objects'][existing][3:5] == original[3:5]
    for label in ('helpClosed', 'productionClosed', 'salvageCancelled', 'closed'):  # 원본 메뉴/경고를 반납한 뒤 잔여 루트가 없어야 한다.
        assert observed[label]['raw_menus'][:3] == ['0', '0', '0']
    return dict(size=list(size), type=number, asset=name, sid=sid, target=target,
                cost=cost, maximumHp=maximum, refund=refund, finalSp=500000 - cost + refund,
                constructionMenuBlocked=True, helpReturned=True, emptyProduction=True,
                salvageCancelledThenConfirmed=True, originalFactoriesPreserved=sorted(original_factories),
                items=items, production=production, run=run)


def one(output, source, size, number, name):
    """새 입력 사본에서 실제 사제 메뉴·배치·우클릭·확인 버튼만 누른다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    props = properties(name)
    label = f" Build {props['description']} for {int(props['cost'])}"
    point = BODY_POINT
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (20, 'report', 'initial'),
             (25, 'right', 'world:priest'), (35, 'click', 'Construct'), (40, 'click', 'Workshop'),
             (45, 'click', label), (55, 'report', 'taken'), (60, 'up', BUILD_POINT),
             (70, 'report', 'cursor'), (75, 'click', BUILD_POINT), (90, 'report', 'placed'),
             (100, 'right', point), (110, 'report', 'constructionDenied'),
             (110, 'snapshot', str(output / 'construction-denied.bmp')), (750, 'report', 'built'),
             (760, 'right', point), (770, 'report', 'menu'), (770, 'snapshot', str(output / 'menu.bmp')),
             (780, 'click', 'About'), (790, 'report', 'help'), (790, 'snapshot', str(output / 'help.bmp')),
             (800, 'outside', ''), (810, 'report', 'helpClosed'), (820, 'right', point),
             (830, 'click', 'View Current Production'), (840, 'report', 'production'),
             (840, 'snapshot', str(output / 'production.bmp')), (850, 'outside', ''),
             (860, 'report', 'productionClosed'), (870, 'right', point), (880, 'click', SALVAGE_POINT),
             (890, 'report', 'salvageWarning'), (900, 'click', 'Cancel'),
             (910, 'report', 'salvageCancelled'), (920, 'right', point), (930, 'report', 'salvageMenu'),
             (940, 'click', SALVAGE_POINT), (950, 'report', 'salvageAgain'), (960, 'click', 'Salvage It!'),
             (970, 'report', 'salvaged'), (970, 'snapshot', str(output / 'salvaged.bmp')),
             (1000, 'report', 'settled'), (1010, 'report', 'closed')]
    report, run = protected_run(output / 'run', *size, steps, 1020, data_root=data, mission='TEST02')
    result = verify(report, run, size, number, name)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본 밖의 준비본을 복제하고 지정한 원소·화면비를 순서대로 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--asset', choices=[row[1] for row in CASES])
    parser.add_argument('--size', choices=['1280x720', '1024x768'])
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    binary, results = sha(GAME), []
    for number, name in CASES:  # 원소별 새 게임 사본으로 객체·기술·예약을 공유하지 않는다.
        if args.asset and name != args.asset:
            continue
        for size in SIZES:  # 같은 제품에서 원본4:3과 확장16:9만 바꾼다.
            if args.size and args.size != f'{size[0]}x{size[1]}':
                continue
            directory = output / f'{name}-{size[0]}x{size[1]}'
            results.append(one(directory, source, size, number, name))
            assert results[-1]['run']['binarySha256'] == binary and sha(GAME) == binary
    save(output / 'summary.json', dict(results=results, binarySha256=binary,
                                     directObjectMutation=False, attackFlyersNotTested=True))
    print(f'워크샾 실제 건설 중 차단·완공·회수 전환: {len(results)}개 통과')


if __name__ == '__main__':
    main()
