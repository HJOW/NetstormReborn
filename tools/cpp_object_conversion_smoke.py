#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Editor 덱의 Devastation을 비석·주문으로 왕복하고 저장 QA의 재적재를 확인한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import GAME, ROOT, execute, save
from cpp_game_menu_smoke import menu_items
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 원문 비석/주문 작성기가 공유하는 실제 전환 명령이다.
CONVERT = 0x030000ce


def one(output, data, width, height):
    """새 주문 하나의 같은 SID/위치/소유자와 저장 뒤 실제 역전환을 일반 입력으로 검사한다."""
    output.mkdir(parents=True, exist_ok=False)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
             (18, 'click', 'dissolvedalliance'), (55, 'report', 'editor'),
             (60, 'click', 'production:114'), (70, 'up', 'placement:112,118'),
             (80, 'report', 'cursor'), (85, 'click', 'placement:112,118'), (95, 'report', 'created'),
             (110, 'right', 'world:bombExplodeMedium'), (118, 'report', 'menu-spell'),
             (125, 'click', 'Turn To Obelisk!'), (135, 'report', 'buried'),
             (140, 'right', 'world:buried'), (148, 'report', 'menu-buried'),
             (155, 'click', 'Turn To Spell!'), (165, 'report', 'spell'),
             (170, 'right', 'world:bombExplodeMedium'), (178, 'report', 'menu-spell2'),
             (185, 'click', 'Turn To Obelisk!'), (195, 'report', 'saved-form'),
             (195, 'snapshot', str(output / 'before-save.bmp')),
             (205, 'esc', ''), (210, 'click', 'Game'), (215, 'click', 'Main Menu'),
             (224, 'report', 'save-question'), (230, 'click', 'Yes'), (250, 'report', 'main'),
             (260, 'click', 'Edit'), (270, 'click', 'dissolvedalliance'), (310, 'report', 'reopened'),
             (315, 'right', 'cell:112,118'), (323, 'report', 'menu-reopened'),
             (330, 'click', 'Turn To Spell!'), (340, 'report', 'restored-spell'),
             (340, 'snapshot', str(output / 'restored-spell.bmp'))]
    plan, report = output / 'steps.tsv', output / 'states.tsv'
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', '350', '--set',
               f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0;currentLanguage=english',
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
    created = [(sid, row) for sid, row in observed['created']['objects'].items()
               if sid not in observed['editor']['objects'] and row[0] == 'bombExplodeMedium']
    assert len(created) == 1, created
    sid, initial = created[0]
    assert initial[1] == '1' and initial[3:5] == ['112', '118']
    assert observed['cursor']['raw_cursor'][0:3] == ['0', '1', '114'], observed['cursor']['raw_cursor']
    expected = (('buried', 'buried'), ('spell', 'bombExplodeMedium'), ('saved-form', 'buried'))
    # 메뉴가 만드는 일시 range 표시는 제외하고 기존 객체의 TYPE·소유자·좌표를 모두 비교한다.
    other_before = {i: row[:7] for i, row in observed['created']['objects'].items() if i != sid and row[0] != 'range'}
    for label, kind in expected:
        # 매 전환은 재할당 없이 같은 실제 SID·좌표·소유자를 유지해야 한다.
        row = observed[label]['objects'][sid]
        assert row[0] == kind and row[1:7] == initial[1:7], (label, row, initial)
        other_after = {i: row[:7] for i, row in observed[label]['objects'].items() if i != sid and row[0] != 'range'}
        assert other_before == other_after, label
    checked = []  # 메뉴의 표시·활성·SID 인자는 실제 원시 행으로 판정한다.
    for label in ('menu-spell', 'menu-buried', 'menu-spell2', 'menu-reopened'):
        # 재진입 뒤에는 같은 위치 객체의 새 SID를 허용하되 재사용 중의 다른 대상 선택은 거부한다.
        roots = [row for row in repeated(report, 'raw_menu_object')[label] if int(row[1])]
        assert len(roots) == 1 and int(roots[0][2]) != 0, roots
        actual = int(roots[0][1]);items = [row for row in menu_items(report, label) if row['visible'] and row['command'] == CONVERT]
        assert len(items) == 1 and items[0]['parameter'] == actual and not items[0]['flags'] & 0x200, items
        assert observed[label]['objects'][actual][3:5] == initial[3:5]
        assert label == 'menu-reopened' or actual == sid
        checked.append(dict(label=label, sid=actual, text=items[0]['label'], flags=items[0]['flags']))
    assert observed['main']['phase'] == ['0']
    assert observed['reopened']['editor_runtime'][:3] == ['1', '1', '1']
    after_sid = checked[-1]['sid']
    assert observed['reopened']['objects'][after_sid][0] == 'buried'
    assert observed['reopened']['objects'][after_sid][1] == '0'  # 비석은 원본 Fort의 owner 저장 마스크에 포함되지 않는다.
    assert observed['restored-spell']['objects'][after_sid][0] == 'bombExplodeMedium'
    for label in ('created', 'buried', 'spell', 'saved-form', 'reopened', 'restored-spell'):
        # 비활성 창의 화면만으로 입력 성공을 판정하지 않는다.
        assert observed[label]['mouse_cursor'][1] == '1'
    saved = data.with_name(data.name + '-editor') / 'd/dissolvedalliance.fort'
    assert saved.exists()
    shutil.copyfile(saved, output / 'saved.fort')
    result = dict(size=[width, height], createdSid=sid, reloadedSid=after_sid, position=initial[3:5],
                  menus=checked, sameSidPreserved=True, savedQaRestoresDevastation=True,
                  reloadedObeliskOwner=0, unchangedOtherObjects=len(other_before), run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """이전 화면비의 저장 객체가 다음 검사 대상에 섞이지 않도록 원본 동일 사본을 따로 만든다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    args = parser.parse_args();output, source = args.output.resolve(), args.data_root.resolve()
    assert not any(source == (ROOT / name).resolve() or (ROOT / name).resolve() in source.parents
                   for name in ('originals', 'originalCD'))
    assert {path.stem.casefold() for path in (source / 'd').glob('*.fort')} == {'dissolvedalliance'}
    output.mkdir(parents=True, exist_ok=False);results = []
    for width, height in ((1280, 720), (1024, 768)):
        # loose Fort 하나인 동일 입력을 새 namespace로 복사하며 실제 원본/이전 작업본은 바꾸지 않는다.
        data = output / f'data-{width}x{height}';shutil.copytree(source, data)
        assert snapshots(source) == snapshots(data)
        results.append(one(output / f'{width}x{height}', data, width, height))
    save(output / 'summary.json', dict(results=results, actualEditorPalettePlacement=True,
                                     originalCampaignFortCopiedUnchanged=True, attackFlyersNotTested=True))
    print('실제 Editor의 주문/비석 왕복·저장 QA 재적재 두 화면비 통과')


if __name__ == '__main__':
    main()
