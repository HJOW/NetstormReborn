#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Edit 아이스 타워 배치와 일반 제네레이터3종/타워의 Help·회수를 확인한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_game_menu_smoke import menu_items
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from shp import load_type_texts
from typefile import parse_type

# 성장형150은 실제 Edit 덱에 없으므로 강제 생성하지 않는다.
CASES = ((75, 'windBattery'), (76, 'rainBattery'), (77, 'thunderBattery'), (147, 'rainBlocker'))
# 원래 세 공급자가 닿는 본섬의 실제 허용 위치만 선택한다.
ICE_POINT = 'placement-near:148,174'


def one(output, data, size, prepare):
    """준비는 실제 Edit 저장, 검증은 일반 게임의 현재 메뉴와 SP/삭제를 사용한다."""
    output.mkdir(parents=True, exist_ok=False)
    if prepare:
        steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'),
                 (18, 'click', 'TEST02'), (55, 'report', 'editor'),
                 (60, 'click', 'production:147'), (63, 'up', ICE_POINT), (65, 'click', ICE_POINT),
                 (75, 'report', 'placed'), (80, 'outside', ''), (90, 'esc', ''),
                 (95, 'click', 'Game'), (100, 'click', 'Test Battle'),
                 (150, 'click', 'Go!'), (160, 'report', 'initial')]
        start = 180
    else:
        steps = [(10, 'click', 'Go!'), (30, 'report', 'initial')]
        start = 40
    specs, texts = {}, load_type_texts(ROOT / 'originals')
    for index, (number, name) in enumerate(CASES):  # 실제 원본 비용/HP/설명을 메뉴 기대값으로 사용한다.
        props = {key.lower(): value for key, value in parse_type(texts[name.lower()]).props.items()}
        refund, frame = int(float(props['cost']) / 4), start + index * 64
        specs[name] = (number, refund, int(props['maxhitpoints']), props['description'])
        steps.extend([(frame, 'right', 'world:' + name), (frame + 6, 'report', name + '-menu'),
                      (frame + 6, 'snapshot', str(output / (name + '-menu.bmp'))),
                      (frame + 10, 'click', 'About'), (frame + 18, 'report', name + '-help'),
                      (frame + 18, 'snapshot', str(output / (name + '-help.bmp'))),
                      (frame + 24, 'outside', ''), (frame + 30, 'right', 'world:' + name),
                      (frame + 36, 'report', name + '-again'),
                      (frame + 42, 'click', f'Salvage gains {refund}'),
                      (frame + 54, 'report', name + '-removed')])
    steps.extend([(start + 256, 'report', 'settled'),
                  (start + 256, 'snapshot', str(output / 'settled.bmp'))])
    script, report = output / 'steps.tsv', output / 'states.tsv'
    script.write_text(''.join(f'{f}\t{op}\t{arg}\n' for f, op, arg in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', str(start + 270),
               '--set', f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(script), '--ui-report', str(report)]
    if not prepare:
        command.extend(['--mission', 'TEST02'])
    original, before, run = snapshots(ROOT / 'originals'), snapshots(data), {}
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 120)
    finally:
        run.update(originalsUnchanged=original == snapshots(ROOT / 'originals'), dataUnchanged=before == snapshots(data))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed, roots, fields, bodies = states(report), repeated(report, 'raw_menu_object'), repeated(report, 'raw_asset_fields'), repeated(report, 'raw_help_body')
    initial, removed, rows, sp = observed['initial'], set(), [], 500000
    for _, name in CASES:  # 같은 대상의 도움말 반납 뒤 재선택도 SID로 대조한다.
        number, refund, hp, description = specs[name]
        label = name + '-menu'
        selected = [row for row in roots[label] if int(row[1])]
        assert len(selected) == 1, (name, selected)
        sid = int(selected[0][1])
        target = observed[label]['raw_objects'][sid]
        assert target[:2] == [str(number), '1'] and initial['raw_objects'][sid][3:5] == target[3:5], (name, target)
        items = [row for row in menu_items(report, label) if row['visible'] and row['token'] == int(selected[0][0])]
        assert [row['label'] for row in items] == [f'Salvage gains {refund}', 'About', 'Player'], (name, items)
        assert items[0]['command'] == 0x0200005d and items[0]['parameter'] == sid and not items[0]['flags'] & 0x200
        assert not any('CHEAT' in row['label'].upper() for row in menu_items(report, label) if row['visible'])
        current_hp = [row for row in fields[label] if int(row[0]) == sid]
        assert len(current_hp) == 1 and int(current_hp[0][3]) == hp
        assert any(description in row[-1] for row in bodies[name + '-help'])
        again = [row for row in roots[name + '-again'] if int(row[1])]
        assert len(again) == 1 and int(again[0][1]) == sid
        assert float(observed[name + '-again']['raw_sp'][1]) == sp
        removed.add(sid)
        sp += refund
        after = observed[name + '-removed']
        assert not removed.intersection(after['raw_objects']) and float(after['raw_sp'][1]) == sp
        assert after['raw_menus'][:3] == ['0', '0', '0']
        assert after['phase'] == ['4'] and after['raw_world_modes'] == ['0', '1'] and after['mouse_cursor'][1] == '1'
        rows.append(dict(type=number, asset=name, sid=sid, position=target[3:5], hp=hp, refund=refund, spAfter=sp, menu=items))
    final = observed['settled']
    assert float(final['raw_sp'][1]) == sp and not removed.intersection(final['raw_objects'])
    ice = rows[-1]
    assert not any(row[0] == '150' and row[3:5] == ice['position'] for row in final['raw_objects'].values())
    result = dict(size=list(size), buildings=rows, finalSp=sp, noRegrowthAfterSalvage=True, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """사본 하나를 편집한 뒤 저장 Fort를 명시 복사해 두 번째 일반 진입을 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    binary = sha(GAME)
    first = one(output / '1280x720', data, (1280, 720), True)
    saved = output / 'data-editor/d/TEST02.fort'
    assert saved.exists()
    shutil.copy2(saved, data / 'd/TEST02.fort')
    second = one(output / '1024x768', data, (1024, 768), False)
    assert first['run']['binarySha256'] == second['run']['binarySha256'] == binary == sha(GAME)
    assert first['buildings'][-1]['position'] == second['buildings'][-1]['position']
    save(output / 'summary.json', dict(results=[first, second], binarySha256=binary, savedFortSha256=sha(saved),
                                     growthLifecycleTested=False, attackFlyersNotTested=True))
    print('제네레이터3종·아이스 타워의 실제 메뉴/Help/회수8사례 통과')


if __name__ == '__main__':
    main()
