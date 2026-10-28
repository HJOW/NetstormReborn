#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 사제 건설의 진행 중 메뉴 차단과 완료·회수 뒤 메뉴 전환을 대조한다."""
import argparse
import shutil
from pathlib import Path

from cpp_altar_salvage_smoke import BUILD_LABEL, BUILD_POINT, SIZES, verify
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states


def enabled_item(report, label, text):
    """현재 보이는 실제 항목 하나와 원본 비활성 비트를 함께 읽는다."""
    rows = [row for row in menu_items(report, label) if row['visible'] and row['label'] == text]
    assert len(rows) == 1, (label, text, rows)
    return not bool(rows[0]['flags'] & 0x200)


def check_transition(report):
    """현재 슬롯의 건설 비트·HP와 실제 메뉴 관측을 확인하며 시계나 상태를 바꾸지 않는다."""
    observed = states(report)
    dais = [(sid, row) for sid, row in observed['placed']['objects'].items()
            if row[:2] == ['dais', '1']]
    assert len(dais) == 1, dais
    sid, _ = dais[0]
    for label in ('placed', 'constructionDenied', 'priestBuilding'):  # 실제 진행 중 슬롯과 비용을 매번 조회한다.
        row = observed[label]
        fields = [value for value in repeated(report, 'raw_asset_fields')[label] if int(value[0]) == sid]
        assert len(fields) == 1 and int(fields[0][2]) & 1 and 0 <= int(fields[0][3]) < 1500, fields
        assert not any(value[:2] == ['altar', '1'] for value in row['objects'].values())
        assert float(row['raw_sp'][1]) == 499500
        assert row['phase'] == ['4'] and row['paused'] == ['0'] and row['raw_world_modes'] == ['0', '1']
        assert row['mouse_cursor'][1] == '1'
    assert observed['constructionDenied']['raw_menus'][:3] == ['0', '0', '0']
    assert not repeated(report, 'raw_menu_object')['constructionDenied']
    assert enabled_item(report, 'priestBuilding', 'Construct')
    assert not enabled_item(report, 'pendingBuildMenu', BUILD_LABEL)
    assert enabled_item(report, 'priestBuilt', 'Construct')
    assert not enabled_item(report, 'duplicateMenu', BUILD_LABEL)
    assert enabled_item(report, 'rebuildMenu', BUILD_LABEL)
    assert float(observed['closed']['time'][0]) > float(observed['placed']['time'][0])
    return dict(daisSid=sid, constructionMenuBlocked=True, idlePriestConstructAvailable=True,
                pendingAltarBuildBlocked=True, constructAvailableAfterCompletion=True, duplicateAltarBlocked=True,
                altarBuildEnabledAfterSalvage=True)


def one(output, source, width, height):
    """원본을 보존하는 새 사본에서 같은 실제 메뉴와 합법적인 배치만 사용한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (20, 'report', 'initial'),
             (25, 'right', 'world:priest'), (35, 'click', 'Construct'), (45, 'click', BUILD_LABEL),
             (55, 'report', 'taken'), (60, 'up', BUILD_POINT), (70, 'report', 'cursor'),
             (75, 'click', BUILD_POINT), (90, 'report', 'placed'),
             (100, 'right', 'world:dais'), (110, 'report', 'constructionDenied'),
             (110, 'snapshot', str(output / 'construction-denied.bmp')),
             (120, 'right', 'world:priest'), (130, 'report', 'priestBuilding'),
             (130, 'snapshot', str(output / 'priest-building.bmp')), (132, 'click', 'Construct'),
             (136, 'report', 'pendingBuildMenu'), (136, 'snapshot', str(output / 'pending-build.bmp')),
             (140, 'outside', ''),
             (600, 'report', 'built'), (610, 'right', 'world:priest'), (620, 'report', 'priestBuilt'),
             (630, 'click', 'Construct'), (640, 'report', 'duplicateMenu'),
             (640, 'snapshot', str(output / 'duplicate.bmp')), (650, 'outside', ''),
             (660, 'right', 'world:altar'), (670, 'report', 'menu'),
             (670, 'snapshot', str(output / 'menu.bmp')), (680, 'click', 'About'),
             (690, 'report', 'help'), (690, 'snapshot', str(output / 'help.bmp')),
             (700, 'outside', ''), (710, 'report', 'helpClosed'),
             (720, 'right', 'world:altar'), (730, 'report', 'salvageMenu'),
             (740, 'click', 'Salvage gains 125'), (750, 'report', 'salvaged'),
             (750, 'snapshot', str(output / 'salvaged.bmp')), (770, 'report', 'settled'),
             (780, 'right', 'world:priest'), (790, 'click', 'Construct'),
             (800, 'report', 'rebuildMenu'), (800, 'snapshot', str(output / 'rebuild.bmp')),
             (810, 'outside', ''), (820, 'report', 'closed')]
    report, run = protected_run(output / 'run', width, height, steps, 830, data_root=data, mission='TEST02')
    result = verify(report, run, width, height)
    result['constructionTransition'] = check_transition(report)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본을 입력에서 거부하고 두 화면비의 같은 제품을 순서대로 실행한다."""
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
    for width, height in SIZES:  # 타이밍·입력은 같게 두고 두 화면비를 순차 검증한다.
        rows.append(one(output / f'{width}x{height}', source, width, height))
        assert rows[-1]['run']['binarySha256'] == binary and sha(GAME) == binary
    save(output / 'summary.json', dict(results=rows, binarySha256=binary,
                                     rawObjectMutation=False, attackFlyersNotTested=True))
    print(f'제단 건설 중 차단·완료·회수 전환: {len(rows)}개 통과')


if __name__ == '__main__':
    main()
