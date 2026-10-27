#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원래 자금/워크샾으로 실제 업그레이드와 회수 확인·취소·환불을 검사한다."""
import argparse
import re
import shutil
import struct
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states

SIZES = ((1280, 720), (1024, 768))  # 같은최종제품의확장16:9와원래4:3을확인한다.
SALVAGE_POINT = 'menu-command:02000095,60000002'  # 원시메뉴에서확인한SalvageInf문자열행의실제클릭위치다.


def one(output, data, width, height):
    """브리핑부터워크샾선택/업그레이드/실제15초진행/회수까지상태조작없이클릭한다."""
    steps = []  # 메뉴명령직접주입이나게임시각/자금/레벨변경은사용하지않는다.
    for frame in range(12, 72, 8):  # 현재실제브리핑의버튼만처리한다.
        for label in BUTTONS:  # 게임진입후에는같은이름의없는버튼을찾지않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.extend([
        (76, 'report', 'initial'), (100, 'right', 'world:windFactory'),
        (110, 'report', 'menu'), (110, 'snapshot', str(output / 'menu.bmp')),
        (120, 'click', 'Upgrade costs 1000'), (130, 'report', 'requested'),
        (130, 'snapshot', str(output / 'requested.bmp')), (140, 'right', 'world:windFactory'),
        (150, 'click', 'Upgrade costs 1000'), (160, 'report', 'again'),
        (160, 'snapshot', str(output / 'again.bmp')), (170, 'click', 'OK'),
        (180, 'report', 'returned'), (700, 'report', 'complete'),
        (710, 'right', 'world:windFactory'), (720, 'report', 'upgradedMenu'),
        (720, 'snapshot', str(output / 'upgraded.bmp')), (730, 'click', SALVAGE_POINT),
        (740, 'report', 'salvageWarning'), (740, 'snapshot', str(output / 'warning.bmp')),
        (750, 'click', 'Cancel'), (760, 'report', 'salvageCancelled'),
        (770, 'right', 'world:windFactory'), (775, 'report', 'salvageMenu'),
        (780, 'click', SALVAGE_POINT), (790, 'report', 'salvageAgain'),
        (800, 'click', 'Salvage It!'), (810, 'report', 'salvaged'),
        (810, 'snapshot', str(output / 'salvaged.bmp')), (870, 'report', 'settled')])
    report, run = protected_run(output, width, height, steps, 880, data_root=data, mission='dissolvedalliance')
    result = verify(report, run, width, height)
    save(output / 'summary.json', result)
    return result


def f32(value):
    """원본환불계산의명시float저장경계를독립적으로재현한다."""
    return struct.unpack('<f', struct.pack('<f', value))[0]


def verify(report, run, width, height):
    """현재원시관측을원문비용/레벨/예측그림/환불식과대조하며게임을추가실행하지않는다."""
    observed = states(report)
    fields = repeated(report, 'raw_asset_fields')  # frame대신실제raw상태/extra/HP를읽는다.
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1, roots
    sid = int(roots[0][1])  # 원래대상의서버SID를끝까지유지한다.
    target = observed['menu']['raw_objects'][sid]
    assert sid >= 15000 and target[:2] == ['86', '1'] and observed['menu']['raw_world_modes'] == ['0', '1'], target

    def asset(label, selected=sid):
        """같은프레임의정확히한실제객체상태를반환한다."""
        rows = [row[1:] for row in fields[label] if int(row[0]) == selected]
        assert len(rows) == 1, (label, selected, rows)
        return list(map(int, rows[0]))

    for label in ('menu', 'requested', 'returned', 'complete', 'upgradedMenu', 'salvageCancelled', 'salvageMenu'):  # 대화외에는일반임무/같은소유자/위치를확인한다.
        row = observed[label]
        assert row['size'] == [str(width), str(height)] and row['phase'] == ['4'] and row['paused'] == ['0']
        assert row['raw_objects'][sid][:2] == target[:2] and row['raw_objects'][sid][3:] == target[3:]
    assert asset('menu')[0] >> 5 == 1 and asset('requested')[0] >> 5 == 1
    assert float(observed['menu']['raw_sp'][1]) == 4000
    for label in ('requested', 'again', 'returned', 'complete', 'salvageCancelled', 'salvageAgain'):  # 중복 요청과 취소 뒤 비용이 다시 차감되지 않는지 확인한다.
        assert float(observed[label]['raw_sp'][1]) == 3000, (label, observed[label]['raw_sp'])
    again = observed['again']
    assert again['page'] == ['alreadyUpgrading'] and again['phase'] == ['1'] and again['paused'] == ['1']
    assert 'Already Upgrading' in ' '.join(again['raw_html_body'])
    pictures = {key: row for key, row in observed['requested']['raw_objects'].items() if row[:2] == ['86', '1'] and key < 15000}
    assert len(pictures) == 1, pictures
    image, picture = next(iter(pictures.items()))
    assert asset('requested', image)[0] >> 5 == 2 and asset('requested', image)[1] & 8
    assert picture[3] == target[3] and abs(float(picture[4]) - (float(target[4]) + 0.001)) < 0.00001
    assert image not in observed['complete']['raw_objects'] and asset('complete')[0] >> 5 == 2
    assert asset('complete')[2] == 4000 and float(observed['complete']['raw_world'][0]) - float(observed['requested']['raw_world'][0]) >= 15
    warnings = []  # 워크샾 전용 경고와 같은 대상 SID를 확인한다.
    for label in ('salvageWarning', 'salvageAgain'):  # 첫 취소와 두 번째 확정 모두 원본 확인 페이지를 거친다.
        row = observed[label]
        assert row['page'] == ['SalvageInf'] and row['paused'] == ['0'] and sid in row['raw_objects'], (label, row['page'])  # 원본Mission482e20의일반Tell은cf960→cf730만호출하며별도자기Pause를취득하지않는다.
        assert 'lose the ability to produce' in ' '.join(row['raw_html_body'])
        buttons = repeated(report, 'raw_html_button')[label]
        assert any(button[-1] == 'Salvage It!' and int(button[3]) == 0x0200005d and int(button[4]) == sid for button in buttons), buttons
        warnings.append(dict(label=label, buttons=buttons))
    hp = asset('salvageMenu')[2]
    level = asset('salvageMenu')[0] >> 5
    expected_refund = int(f32(hp * f32(1000 * (level + 1) * 0.25)) / 4000)  # 원본의 float 곱 저장과 최종 정수 절삭을 독립 계산한다.
    items = [row for row in menu_items(report, 'salvageMenu') if row['visible']]
    salvage = [row for row in items if row['command'] == 0x02000095 and row['parameter'] == 0x60000002]
    assert len(salvage) == 1 and salvage[0]['label'] == f'Salvage gains {expected_refund}', salvage
    assert asset('salvageAgain')[2] == hp
    for label in ('salvaged', 'settled'):  # 실제54h 처리로 삭제되고 같은 금액만 환불되는지 확인한다.
        row = observed[label]
        assert sid not in row['raw_objects'] and float(row['raw_sp'][1]) == 3000 + expected_refund
        assert row['phase'] == ['4'] and row['paused'] == ['0']
    for label in ('menu', 'upgradedMenu', 'salvageMenu'):  # 일반 플레이에서 CHEAT 항목이 표시되지 않는지 확인한다.
        assert not any('CHEAT' in row['label'].upper() for row in menu_items(report, label))
    return dict(size=[width, height], sid=sid, target=target, predictedSid=image,
                levelBefore=1, levelAfter=level, hpAtMenu=asset('menu')[2], hpCompleted=asset('complete')[2],
                hpAtSalvage=hp, refund=expected_refund, spAfter=3000 + expected_refund,
                salvage=salvage, warnings=warnings, run=run)


def main():
    """새 출력 폴더에 실행 사본을 만들고 차례로 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert source != output and source not in output.parents
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'  # 원본과 이전 관측을 보존하고 전용 사본을 사용한다.
    shutil.copytree(source, data)
    rows = []  # 각 화면비의 종료/보호 검사 결과를 저장한다.
    for width, height in SIZES:  # 같은 제품으로 원본4:3과 확장16:9를 확인한다.
        rows.append(one(output / f'{width}x{height}', data, width, height))
    save(output / 'summary.json', dict(results=rows, directObjectMutation=False, attackFlyersNotTested=True))
    print('워크샾 업그레이드/회수 두 화면비 통과')


if __name__ == '__main__':
    main()
