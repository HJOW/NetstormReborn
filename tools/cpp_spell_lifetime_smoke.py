#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새주문·같은세션전환·저장비석전환·저장주문직접적재의실제예약/프레임수명을구별한다."""
import argparse
import re
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_object_conversion_smoke import one as convert
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states
from cpp_game_menu_smoke import menu_items

SIZES = ((1280, 720), (1024, 768))  # 같은입력의확장16:9/원본4:3을차례로확인한다.
LATE = (360, 400, 450, 500, 550, 600, 650, 700, 750, 800)  # 기존예약의실제호출/삭제가끝날때까지상태만읽는다.


def regulars(report, label, sid):
    """현재부모의실제등록Form과예약원문열을필터링한다."""
    return [row for row in repeated(report, 'raw_object_regular')[label] if int(row[0]) == sid]


def check_converted(report, run, width, height):
    """저장전후의두예약정체성과원래Bomb후속삭제를같은시각열에서확인한다."""
    observed = states(report)
    initial = observed['editor']['raw_objects']
    added = [(sid, row) for sid, row in observed['created']['raw_objects'].items() if sid not in initial and row[0] == '114']
    assert len(added) == 1, added
    fresh, original = added[0]
    for label in ('created', 'buried', 'spell'):  # 새로놓은주문과Repop0의비석은최초Pop예약을추가하지않는다.
        assert not regulars(report, label, fresh)
    assert original[:3] == ['114', '1', '1'] and observed['spell']['raw_objects'][fresh][:3] == ['114', '1', '1']
    roots = [row for row in repeated(report, 'raw_menu_object')['menu-reopened'] if int(row[1])]
    assert len(roots) == 1
    sid = int(roots[0][1])  # 재적재한같은위치비석은새SID이며owner0이다.
    assert observed['reopened']['raw_objects'][sid][:3] == ['112', '0', '0']
    before = regulars(report, 'menu-reopened', sid)
    after = regulars(report, 'restored-spell', sid)
    assert len(before) == len(after) == 2 and {int(row[3]) for row in before} == {0, 1}
    assert {row[1] for row in before} == {row[1] for row in after}, (before, after)
    assert observed['restored-spell']['raw_objects'][sid][:3] == ['114', '0', '2']
    assert any(int(row[4]) > 0 for row in after)  # 유지된예약의양수count가현재Bomb애니메이션에전달된다.
    late = []  # 재현된일반입력없이실제effect/가상삭제뒤부모가없어지는지읽는다.
    for frame in LATE:
        label = f'late{frame}'
        assert observed[label]['size'] == [str(width), str(height)]
        late.append(dict(frame=frame, time=observed[label]['raw_world'][0], object=observed[label]['raw_objects'].get(sid), regulars=regulars(report, label, sid)))
    assert late[0]['object'] and not late[-1]['object'] and not late[-1]['regulars'], late
    return dict(case='converted', size=[width, height], freshSid=fresh, reloadedSid=sid, regularsBefore=before, regularsAfter=after, late=late, run=run)


def stored(output, data, width, height):
    """Edit에서실제주문을배치하고TestBattle의저장/적재와같은칸우클릭을확인한다."""
    output.mkdir(parents=True, exist_ok=False)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
             (18, 'click', 'dissolvedalliance'), (55, 'report', 'editor'),
             (60, 'click', 'production:114'), (70, 'up', 'placement:112,118'),
             (85, 'click', 'placement:112,118'), (95, 'report', 'created'),
             (100, 'esc', ''), (105, 'click', 'Game'), (110, 'click', 'Test Battle')]
    for frame in range(120, 224, 8):  # 재개된같은미션의현재안내버튼만넘긴다.
        for label in BUTTONS:  # 일반게임진입뒤에는없는버튼을추가선택하지않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.extend([(230, 'report', 'loaded'), (230, 'snapshot', str(output / 'loaded.bmp')),
                  (240, 'right', 'cell:112,118'), (250, 'report', 'pointed'),
                  (260, 'outside', ''), (390, 'report', 'settled'),
                  (390, 'snapshot', str(output / 'settled.bmp'))])
    script, report = output / 'steps.tsv', output / 'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), 'utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--mute-audio', '--frames', '400', '--set',
               f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(script), '--ui-report', str(report)]
    before, original = snapshots(data), snapshots(ROOT / 'originals')
    run = dict(command=command, binarySha256=sha(GAME), scriptSha256=sha(script))
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run.update(execute(command, output, GAME, 120))
    finally:
        run.update(dataUnchanged=before == snapshots(data), originalsUnchanged=original == snapshots(ROOT / 'originals'))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['dataUnchanged'] and run['originalsUnchanged'], run
    result = check_stored(report, run, width, height)
    save(output / 'summary.json', result)
    return result


def check_stored(report, run, width, height):
    """실제로 저장한 주문의 frame0/예약0과 투명 영역 클릭을 각각 대조한다."""
    observed = states(report)
    spell = [(sid, row) for sid, row in observed['loaded']['raw_objects'].items() if row[0] == '114' and row[3:] == ['112', '118']]
    assert len(spell) == 1, spell
    sid, target = spell[0]
    assert target[:3] == ['114', '1', '0'], target
    for label in ('loaded', 'pointed', 'settled'):  # 저장 프레임을 강제로 바꾸지 않고 같은 원래 임무를 짧게 진행한다.
        row = observed[label]
        assert row['size'] == [str(width), str(height)] and row['phase'] == ['4'] and row['paused'] == ['0']
        assert row['raw_world_modes'] == ['0', '1'] and row['raw_objects'][sid] == target
        assert not regulars(report, label, sid)
    assert not any(row['visible'] and row['label'] == 'Detonate!' for row in menu_items(report, 'pointed'))
    assert all(int(row[1]) != sid for row in repeated(report, 'raw_menu_object')['pointed'])
    return dict(case='stored', size=[width, height], sid=sid, target=target, noRegulars=True, blankSpellNotSelected=True, run=run)


def main():
    """각 사례에 새 사본을 제공하고 원본 Fort/영어 파일을 보존하며 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert source != output and source not in output.parents
    assert {path.stem.lower() for path in (source / 'd').glob('*.fort')} == {'dissolvedalliance'}
    output.mkdir(parents=True, exist_ok=False)
    rows = []  # 같은 최종 제품의 실행만 실제 수락 결과에 포함한다.
    for width, height in SIZES:  # 원래4:3과 확장16:9에 같은 입력을 적용한다.
        for case in ('converted', 'stored'):  # 앞 사례가 저장한 편집 내용을 다음 입력 사본에 섞지 않는다.
            data = output / f'{case}-{width}x{height}-data'
            shutil.copytree(source, data)
            directory = output / f'{case}-{width}x{height}'
            if case == 'converted':
                binary = sha(GAME)  # 실제실행전후같은제품인지확인하고기록한다.
                result = convert(directory, data, width, height, LATE)
                run = result['run']
                assert sha(GAME) == binary
                run['binarySha256'] = binary
                checked = check_converted(directory / 'states.tsv', run, width, height)
                save(directory / 'lifetime-summary.json', checked)
            else:
                checked = stored(directory, data, width, height)
            rows.append(checked)
    save(output / 'summary.json', dict(results=rows, directObjectMutation=False, attackFlyersNotTested=True))
    print('주문 프레임/예약/후속 삭제 네 사례 통과')


if __name__ == '__main__':
    main()
