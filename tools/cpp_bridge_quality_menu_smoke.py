#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 TEST02 Edit에서 개별 다리3품질의 실제 우클릭·저장·재진입을 확인한다."""
import argparse
from pathlib import Path

from cpp_bridge_ritual_runtime import GAME, ROOT, execute, save
from cpp_game_menu_smoke import menu_items
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from shp import load_type_texts
from typefile import parse_type

# 원본 다리 작성기가 보내는 세 명령/라벨/품질 플래그다.
QUALITIES = (('cracked', 'Set as Cracked', 0x02000125),
             ('normal', 'Set as Normal', 0x02000126),
             ('hard', 'Set as Hard', 0x02000127))


def one(output, data, width, height):
    """게임 상태를 주입하지 않고 Editor 설치→우클릭3회→저장→같은 맵 재진입을 실행한다."""
    output.mkdir(parents=True, exist_ok=False)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
             (18, 'click', 'TEST02'), (55, 'report', 'editor')]
    for index, (name, label, _) in enumerate(QUALITIES):
        # 매번 새 팝업의 실제 라벨을 일반 InputQueue 클릭으로 선택한다.
        frame = 60 + index * 30
        steps.extend([(frame, 'right', 'cell:119,146'), (frame + 8, 'report', 'menu-' + name),
                      (frame + 15, 'click', label), (frame + 25, 'report', name)])
    steps.extend([(145, 'snapshot', str(output / 'hard.bmp')), (155, 'esc', ''),
                  (160, 'click', 'Game'), (165, 'click', 'Main Menu'), (174, 'report', 'save-question'),
                  (180, 'click', 'Yes'), (200, 'report', 'main'), (210, 'click', 'Edit'),
                  (220, 'click', 'TEST02'), (260, 'report', 'reopened'),
                  (260, 'snapshot', str(output / 'reopened.bmp'))])
    plan = output / 'steps.tsv'
    report = output / 'states.tsv'
    plan.write_text(''.join(f'{frame}\t{op}\t{value}\n' for frame, op, value in steps), encoding='utf-8')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', '270', '--set',
               f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0;currentLanguage=english',
               '--ui-script', str(plan), '--ui-report', str(report)]
    original, before = snapshots(ROOT / 'originals'), snapshots(data)
    run = {}  # 실행 실패도 복구/보호 결과와 함께 기록한다.
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 120)
    finally:
        run.update(originalsUnchanged=original == snapshots(ROOT / 'originals'),
                   dataUnchanged=before == snapshots(data))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(report)
    assert observed['editor']['editor_runtime'][:3] == ['1', '1', '1']
    clusters = parse_type(load_type_texts(str(data))['bridge']).clusters
    target = None  # 세 메뉴가 같은 실제 SID를 대상으로 하는지 첫 루트에서 확정한다.
    changed = []
    for name, label, code in QUALITIES:
        # 현재 실제 루트/Form·활성·명령 인자를 확인하고 다른 다리의 프레임도 비교한다.
        roots = [row for row in repeated(report, 'raw_menu_object')['menu-' + name] if int(row[1])]
        assert len(roots) == 1 and int(roots[0][2]) != 0, roots
        sid = int(roots[0][1])
        target = sid if target is None else target
        assert sid == target and observed[name]['objects'][sid][0] == 'bridge'
        entry = [row for row in menu_items(report, 'menu-' + name) if row['visible'] and row['command'] == code]
        assert len(entry) == 1 and entry[0]['parameter'] == sid and entry[0]['label'] == label, entry
        frame = int(observed[name]['objects'][sid][7])
        tag, flags, pictures = clusters[frame]
        assert len(pictures) == 1 and (name in flags.split() if name != 'normal' else not {'cracked', 'hard'} & set(flags.split())), (name, frame, tag, flags)
        other_before = {i: row[7] for i, row in observed['editor']['objects'].items() if row[0] == 'bridge' and i != sid}
        other_after = {i: row[7] for i, row in observed[name]['objects'].items() if row[0] == 'bridge' and i != sid}
        assert other_before == other_after, name
        assert observed[name]['mouse_cursor'][1] == '1'
        changed.append(dict(quality=name, sid=sid, frame=frame, code=tag, flags=flags, unchangedOtherBridges=len(other_after)))
    assert observed['main']['phase'] == ['0']
    assert observed['reopened']['editor_runtime'][:3] == ['1', '1', '1']
    position = observed['hard']['objects'][target][3:5]
    reopened = [row for row in observed['reopened']['objects'].values() if row[0] == 'bridge' and row[3:5] == position]
    assert len(reopened) == 1 and int(reopened[0][7]) == changed[-1]['frame'], reopened
    result = dict(size=[width, height], changes=changed, savedHardFrame=int(reopened[0][7]),
                  actualMenuSaveReload=True, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """원본50개 목록 상한에 맞춰 loose Fort가 TEST02 하나인 명시 사본만 허용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, required=True)
    args = parser.parse_args()
    output, data = args.output.resolve(), args.data_root.resolve()
    assert not any(data == (ROOT / name).resolve() or (ROOT / name).resolve() in data.parents
                   for name in ('originals', 'originalCD'))
    assert {p.stem.casefold() for p in (data / 'd').glob('*.fort')} == {'test02'}
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 두 화면비는 새 프로세스로 독립 설치한다.
    for width, height in ((1280, 720), (1024, 768)):
        # 첫 실행이 저장한 실제 형제 Editor 작업본을 다음 실행도 원문 reader로 읽는다.
        results.append(one(output / f'{width}x{height}', data, width, height))
    save(output / 'summary.json', dict(results=results, targetInput='원본 TEST02의 다리119,146',
                                     onlyWorkingEditorCopySaved=True, attackFlyersNotTested=True))
    print('개별 다리3품질 우클릭·저장·재진입 두 화면비 통과')


if __name__ == '__main__':
    main()
