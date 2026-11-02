#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Edit의 SetOwner 취소·0/2/8 소유자·저장 재적재를 작업 사본에서 확인한다."""
import argparse
from pathlib import Path
import shutil
import socket

from cpp_bridge_ritual_runtime import ROOT, save, sha
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_world_smoke import states


def prepare(out):
    """원본을 읽어 별도 실행 사본과 목록 한 개의 FileSpec을 준비하며 원문을 고치지 않는다."""
    game = out / 'game'
    shutil.copytree(ROOT / 'originals', game)
    maps = game / 'd/owner726'
    maps.mkdir()
    shutil.copyfile(ROOT / 'originals/d/TEST02.fort', maps / 'TEST02.fort')
    return game


def plan(out):
    """오브젝트 상태를 주입하지 않고 실제 우클릭·HTML·ESC 저장과 재진입만 요청한다."""
    steps = [(5, 'script-command', 'Config,SOLOSpec=d/owner726/{local.1}.fort'),
             (5, 'script-command', 'Config,fortSpec=d/owner726/{local.1}.fort'),
             (10, 'click', 'Edit'), (25, 'click', 'TEST02'), (100, 'report', 'loaded')]
    cases = [('cancel1', 'Cancel'), ('owned2', 'Computer #1'), ('unowned', 'No Owner'),
             ('owned8', 'Computer #7'), ('cancel8', 'Cancel')]
    for index, (label, choice) in enumerate(cases):  # 같은 원본 건물 한 개에 각 실제 버튼을 순서대로 적용한다.
        frame = 110 + index * 70
        steps.extend([(frame, 'right', 'cell:137,166'), (frame + 10, 'report', label + 'Object'),
                      (frame + 20, 'click', 'CHEAT Set Owner'), (frame + 30, 'report', label + 'Dialog'),
                      (frame + 40, 'click', choice), (frame + 55, 'report', label)])
    steps.extend([(470, 'esc', ''), (480, 'click', 'Game'), (490, 'click', 'Main Menu'),
                  (500, 'report', 'saveQuestion'), (510, 'click', 'Yes'), (550, 'report', 'main'),
                  (560, 'click', 'Edit'), (575, 'click', 'TEST02'), (650, 'report', 'reloaded'),
                  (650, 'snapshot-presented', str(out / 'run/reloaded.bmp'))])
    return steps


def inspect(out, report, run):
    """보고의 실제 Pool 소유자·현재 Core·HTML 명령과 새 파일의 별도 재적재를 대조한다."""
    observed = states(report)
    expected = [('loaded', 1, 1), ('cancel1', 1, 1), ('owned2', 2, 2),
                ('unowned', 0, 2), ('owned8', 8, 8), ('cancel8', 8, 8), ('reloaded', 8, 1)]
    rows = []
    for label, owner, current in expected:  # SID가 바뀌어도 실제 타입·좌표로 같은 저장 대상을 식별한다.
        state = observed[label]
        matches = [(sid, value) for sid, value in state['raw_objects'].items()
                   if value[0] == '86' and value[3:] == ['140', '174']]
        assert len(matches) == 1 and int(matches[0][1][1]) == owner, (label, matches)
        assert state['page'] == ['editor'] and state['phase'] == ['4'] and state['paused'] == ['0'], label
        assert state['raw_session'][:2] == [str(current), '1'], (label, state['raw_session'])
        assert int(state['player_core'][4]) == 108 + 180 * current, (label, state['player_core'])
        neighbors = [value for value in state['raw_objects'].values()
                     if value[0] == '75' and value[3:] == ['137', '166']]
        assert len(neighbors) == 1 and neighbors[0][1] == '1', (label, neighbors)
        rows.append(dict(label=label, sid=matches[0][0], owner=owner, currentOwner=current,
                         coreOffset=int(state['player_core'][4])))
    for label in ('cancel1', 'owned2', 'unowned', 'owned8', 'cancel8'):  # 다섯 창의 원본 여덟 선택과 두 버튼을 별도로 확인한다.
        selected = observed[label + 'Object']['selected']
        selected_object = observed[label + 'Object']['raw_objects'][int(selected[0])]
        assert selected_object[0] == '86' and selected_object[3:] == ['140', '174'], (label, selected_object)
        state = observed[label + 'Dialog']
        assert state['page'] == ['SetOwner'] and state['phase'] == ['1'], label
        assert state['selected'] == selected and observed[label]['selected'] == selected
        items = [row for row in menu_items(report, label + 'Dialog') if row['visible']]
        owners = [row for row in items if row['command'] == 0x020000cc]
        assert [row['parameter'] for row in owners] == list(range(1, 9)), owners
        buttons = [row for values in state['html_buttons'].values() for row in values if row[-2] == '1']
        assert {(int(row[2]), int(row[3]), row[-1]) for row in buttons} == {
            (0x020000cc, 0, 'No Owner'), (0x02000089, 0, 'Cancel')}, buttons
    assert observed['main']['phase'] == ['0'] and 'raw_session' not in observed['main']
    saved = out / 'game-editor/d/owner726/TEST02.fort'
    source = out / 'game/d/owner726/TEST02.fort'
    assert saved.is_file() and sha(saved) != sha(source)
    return dict(unit=726, host=socket.gethostname(), product=725, productVersion='11.01',
                observations=rows, sourceFortSha256=sha(source), savedFortSha256=sha(saved), run=run,
                limits=['앱 내 InputQueue이며 물리 입력 검사가 아니다', '창 모드1024×768·일반 건물 한 개의 소유자 경로다',
                        '게임 제품 변경·전체34·자연 완주·MP·공중3종 전용 검사는 없다'])


def main():
    """새 출력 폴더에서 한 프로세스만 실행하고 실패해도 원본 보호 판정을 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    before = protection()
    save(out / 'protected-before.json', before)
    try:
        game = prepare(out)
        report, run = protected_run(out / 'run', 1024, 768, plan(out), 675,
                                    data_root=game, mission='', timeout=180)
        save(out / 'summary.json', inspect(out, report, run))
    finally:
        after = protection()
        save(out / 'protected-after.json', after)
        assert before == after, 'AGENTS·원본·CD·기존 fixture 변경'


if __name__ == '__main__':
    main()
