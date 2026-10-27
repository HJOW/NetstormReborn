#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""자기 워크샾의 자금부족·현재생산 도움말·지식생산을 실제 메뉴 클릭으로 관측한다."""
import argparse
import re
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states

SIZES = ((1280, 720), (1024, 768))  # 확장16:9와원본4:3에동일한실제입력열을적용한다.


def one(output, data, width, height):
    """원래 SP0의 Rain워크샾에서 실제 경고·도움말·RainGenerator 연구를 순서대로 누른다."""
    steps = []  # 직접명령/상태변경없이화면에있는버튼과메뉴행만선택한다.
    for frame in range(12, 72, 8):  # 진행중인안내창에실제로존재하는버튼만넘긴다.
        for label in BUTTONS:  # 이미임무가시작됐으면이브리핑분기는실행하지않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.extend([
        (76, 'report', 'initial'), (80, 'key', 'left'), (88, 'keyup', 'left'),
        (100, 'right', 'world:rainFactory'), (110, 'report', 'menu'),
        (120, 'click', 'Upgrade costs 1000'), (130, 'report', 'warning'),
        (130, 'snapshot', str(output / 'warning.bmp')), (140, 'click', 'OK'),
        (150, 'report', 'returned'), (160, 'right', 'world:rainFactory'),
        (170, 'click', 'View Current Production'), (180, 'report', 'production'),
        (190, 'click', 'menu-command:02000017,60000001'), (200, 'report', 'help'),
        (200, 'snapshot', str(output / 'help.bmp')), (220, 'outside', ''),
        (230, 'report', 'helpClosed'), (240, 'right', 'world:rainFactory'),
        (250, 'click', 'Put Knowledge into Production'), (260, 'report', 'knowledge'),
        (270, 'click', 'Rain Generator'), (280, 'report', 'learned'),
        (290, 'right', 'world:rainFactory'), (300, 'click', 'View Current Production'),
        (310, 'report', 'current'), (310, 'snapshot', str(output / 'current.bmp')),
        (320, 'click', 'menu-command:02000017,4c'), (330, 'report', 'typeHelp'),
        (330, 'snapshot', str(output / 'type-help.bmp')), (340, 'outside', ''),
        (350, 'report', 'closed')])
    report, run = protected_run(output, width, height, steps, 365, data_root=data, mission='breakingthrough')
    result = verify(report, run, width, height)
    save(output / 'summary.json', result)
    return result


def verify(report, run, width, height):
    """저장된 실제 보고서에서 원문 분기·같은 워크샾·확정된 내용물·도움말을 대조한다."""
    observed = states(report)  # 반복레코드는별도독립판독기로읽고단일레코드는현재필드만사용한다.
    roots = [row for row in repeated(report, 'raw_menu_object')['menu'] if int(row[1])]
    assert len(roots) == 1, roots
    sid = int(roots[0][1])  # 같은원래워크샾의서버SID다.
    target = observed['menu']['objects'][sid]
    assert target[:2] == ['rainFactory', '1'] and observed['menu']['raw_world_modes'] == ['0', '1'], target
    for label in ('menu', 'returned', 'helpClosed', 'knowledge', 'learned', 'current', 'closed'):  # 경고/도움말외에는원래임무가계속되고대상/HP/레벨은보존된다.
        assert observed[label]['phase'] == ['4'] and observed[label]['paused'] == ['0'], (label, observed[label]['phase'], observed[label]['paused'])
        assert observed[label]['size'] == [str(width), str(height)] and float(observed[label]['raw_sp'][1]) == 0
        assert observed[label]['objects'][sid][:2] == target[:2] and observed[label]['objects'][sid][3:5] == target[3:5]
    fields = repeated(report, 'raw_asset_fields')  # HP와 레벨은 그리기 frame 열과 구별해 원래 상태 BYTE에서 읽는다.
    initial_fields = next(row[1:] for row in fields['menu'] if int(row[0]) == sid)
    for label in ('returned', 'learned', 'closed'):  # 거부된 업그레이드와 무료 지식 등록 뒤 같은 레벨/HP를 확인한다.
        assert next(row[1:] for row in fields[label] if int(row[0]) == sid) == initial_fields
    warning = observed['warning']
    assert warning['page'] == ['upgradeNoBucks'] and warning['phase'] == ['1'] and warning['paused'] == ['1'], warning['page']
    assert 'Storm Power Needed' in ' '.join(warning['raw_html_body'])
    production = menu_items(report, 'production')
    static_help = [row for row in production if row['visible'] and row['command'] == 0x02000017 and row['parameter'] == 0x60000001]
    assert len(static_help) == 1 and static_help[0]['label'] == 'About', static_help
    help_body = ' '.join(observed['help']['raw_help_body'])
    assert 'Knowledge' in help_body and 'Knowledge gives you the ability to build' in help_body, help_body
    contents = repeated(report, 'raw_factory_content')  # 같은 부모의 확정된 Form을 client 예측과 구별한다.
    before = [row for row in contents['knowledge'] if int(row[0]) == sid]
    learned = [row for row in contents['learned'] if int(row[0]) == sid]
    assert not before and len(learned) == 1, (before, learned)
    assert int(learned[0][1]) >= 15000 and learned[0][2:] == ['76', '16'], learned
    assert [row for row in contents['current'] if int(row[0]) == sid] == learned
    current = [row for row in menu_items(report, 'current') if row['visible']]
    assert any(row['label'] == 'Rain Generator' and row['command'] == 0x02000017 and row['parameter'] == 76 for row in current), current
    type_body = ' '.join(observed['typeHelp']['raw_help_body'])
    assert 'Rain Generator' in type_body and 'Generators' in type_body, type_body
    assert observed['closed']['raw_menus'][:3] == ['0', '0', '0']
    for label in ('menu', 'production', 'knowledge', 'current'):  # CHEAT 항목이 일반 플레이의 원시 메뉴에도 없는지 확인한다.
        assert not any('CHEAT' in row['label'].upper() for row in menu_items(report, label))
    return dict(size=[width, height], sid=sid, fields=initial_fields, initialContents=before, confirmedContents=learned,
                production=production, current=current, staticHelp=help_body, typeHelp=type_body, run=run)


def main():
    """준비한 입력 사본에서 새 실행 사본을 만들고 두 화면비를 차례로 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert source != output and source not in output.parents
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'  # 원본 맵을 보존하고 전용 사본을 실행한다.
    shutil.copytree(source, data)
    rows = []  # 각 프로세스의 종료/보호 검사 결과를 따로 보관한다.
    for width, height in SIZES:  # 같은 최종 바이너리로 두 화면비를 확인한다.
        rows.append(one(output / f'{width}x{height}', data, width, height))
    save(output / 'summary.json', dict(results=rows, directObjectMutation=False, attackFlyersNotTested=True))
    print('워크샾 실제 명령 두 화면비 통과')


if __name__ == '__main__':
    main()
