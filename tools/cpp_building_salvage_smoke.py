#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 편집 저장본의 일반 건물12종을 순서대로 회수해 SP·삭제·중복 지급을 확인한다."""
import argparse
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_building_matrix_smoke import CASES, BODY_POINTS
from cpp_game_menu_smoke import menu_items, protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states
from shp import load_type_texts
from typefile import parse_type

# 앞선 실제 Edit 저장본을 두 화면비에서 각각 새 일반 게임으로 적재한다.
SIZES = ((1280, 720), (1024, 768))


def one(output, data, width, height):
    """메뉴 라벨·기대액은 보호 TYPE에서 정하고 실제 클릭·수신·삭제를 기다린다."""
    output.mkdir(parents=True, exist_ok=False)
    texts = load_type_texts(ROOT / 'originals')
    specs = {}
    steps = [(10, 'click', 'Go!'), (30, 'report', 'initial')]
    for index, (number, name) in enumerate(CASES):  # 같은 저장본의12자산을 한 번씩 회수한다.
        props = {key.lower(): value for key, value in parse_type(texts[name.lower()]).props.items()}
        refund = int(float(props['cost']) / 4)
        specs[name] = (number, refund, int(props['maxhitpoints']))
        frame = 40 + index * 64
        steps.extend([(frame, 'right', BODY_POINTS.get(name, 'world:' + name)),
                      (frame + 6, 'report', name + '-menu'),
                      (frame + 6, 'snapshot', str(output / (name + '-menu.bmp'))),
                      (frame + 10, 'click', f'Salvage gains {refund}'),
                      (frame + 24, 'report', name + '-removed'),
                      (frame + 50, 'report', name + '-settled'),
                      (frame + 50, 'snapshot', str(output / (name + '-settled.bmp')))])
    report, run = protected_run(output / 'run', width, height, steps, 820, data_root=data, mission='TEST02')
    observed = states(report)
    roots = repeated(report, 'raw_menu_object')
    fields = repeated(report, 'raw_asset_fields')
    initial = observed['initial']
    assert float(initial['raw_sp'][1]) == 500000
    sp, removed, rows = 500000, set(), []
    for _, name in CASES:  # 앞서 제거한 대상과 누적SP도 매 단계 다시 확인한다.
        number, refund, hp = specs[name]
        label = name + '-menu'
        menu = observed[label]
        selected = [row for row in roots[label] if int(row[1])]
        assert len(selected) == 1, (name, selected)
        sid = int(selected[0][1])
        target = menu['raw_objects'][sid]
        assert target[:2] == [str(number), '1'], (name, target)
        assert float(menu['raw_sp'][1]) == sp, (name, menu['raw_sp'], sp)
        original = initial['raw_objects'][sid]
        assert target[:2] == original[:2] and target[3:5] == original[3:5]
        current_hp = [row for row in fields[label] if int(row[0]) == sid]
        assert len(current_hp) == 1 and int(current_hp[0][3]) == hp, (name, current_hp)
        items = [row for row in menu_items(report, label) if row['visible'] and row['token'] == int(selected[0][0])]
        assert [row['label'] for row in items] == [f'Salvage gains {refund}', 'About', 'Player'], (name, items)
        assert items[0]['command'] == 0x0200005d and items[0]['parameter'] == sid and not items[0]['flags'] & 0x200
        assert not any('CHEAT' in row['label'].upper() for row in menu_items(report, label) if row['visible'])
        sp += refund
        removed.add(sid)
        for suffix in ('removed', 'settled'):  # 실제 삭제 뒤의 추가 시간에도 같은 환불 한 번만 유지돼야 한다.
            after = observed[name + '-' + suffix]
            assert not removed.intersection(after['raw_objects']), (name, suffix, removed)
            assert float(after['raw_sp'][1]) == sp, (name, suffix, after['raw_sp'], sp)
            assert after['raw_menus'][:3] == ['0', '0', '0']
            assert after['size'] == [str(width), str(height)] and after['phase'] == ['4'] and after['paused'] == ['0']
            assert after['raw_world_modes'] == ['0', '1'] and after['mouse_cursor'][1] == '1'
        rows.append(dict(type=number, asset=name, sid=sid, position=target[3:5], hp=hp,
                         refund=refund, spAfter=sp, menu=items, removed=True, paidOnce=True))
    result = dict(size=[width, height], buildings=rows, initialSp=500000, finalSp=sp, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """원본 경로를 거부하고 같은 Fort·제품의 두 화면비 실제 회수를 순차 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data, output = args.data_root.resolve(), args.output.resolve()
    assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    binary, fort = sha(GAME), sha(data / 'd/TEST02.fort')
    rows = []
    for width, height in SIZES:  # 서로 다른 실제 창에서 같은 저장 입력을 새로 시작한다.
        rows.append(one(output / f'{width}x{height}', data, width, height))
        assert rows[-1]['run']['binarySha256'] == binary == sha(GAME)
        assert sha(data / 'd/TEST02.fort') == fort
    save(output / 'summary.json', dict(results=rows, binarySha256=binary, fortSha256=fort,
                                     rawObjectMutation=False, attackFlyersNotTested=True))
    print('일반 건물12종의 두 화면비 실제 회수·환불·중복 없음24사례 통과')


if __name__ == '__main__':
    main()
