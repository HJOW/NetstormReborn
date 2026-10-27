#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ESC/View와 F9로 같은 중앙 Player List 및 정지 중 창 수명을 관측한다."""
import argparse
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, save
from cpp_game_menu_smoke import protected_run, menu_items, choose, SIZES
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states


def player_root(report, label):
    """현재 제목에 연결된 실제 중앙 목록 토큰만 고르고 하위 Player 창과 구별한다."""
    headings = repeated(report, 'raw_menu_heading')[label]  # 실제 렌더러의 문자열 조각이다.
    roots = {int(row[0]) for row in headings if row[-1] == 'Player List'}
    nodes = [row for row in repeated(report, 'raw_menu')[label] if int(row[0]) in roots]
    if len(nodes) != 1:
        raise RuntimeError('실제 Player List 루트 없음/중복: ' + repr(nodes))
    return nodes[0]


def visible_items(report, label):
    """현재 조상까지 표시된 메뉴 항목만 원시 보고서에서 읽는다."""
    return [row for row in menu_items(report, label) if row['visible']]


def check_center(report, label, width, height):
    """원본 기본 내용 사각형의 중앙 배치를 확인하고 장식 테두리 크기는 별도로 가정하지 않는다."""
    node = player_root(report, label)
    left, top, right, bottom = map(int, node[7:11])  # 실제 공통 Bounds 좌표다.
    assert abs(left + right - width) <= 2 and abs(top + bottom - height) <= 2, node
    return dict(token=int(node[0]), bounds=[left, top, right, bottom])


def one_size(directory, width, height, data_root):
    """같은 제품의 최초 실제 라벨로 두 번째 입력을 작성하고 결과 상태/수명을 확인한다."""
    probe = directory / 'probe'
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Play Mission'), (20, 'report', 'initial'),
             (25, 'esc', ''), (30, 'report', 'bar'), (35, 'esc', ''),
             (40, 'key', 'f9'), (41, 'keyup', 'f9'), (50, 'report', 'players'),
             (50, 'snapshot', str(probe / 'players.bmp'))]
    report, first_run = protected_run(probe, width, height, steps, 56, data_root=data_root)
    bar = menu_items(report, 'bar')  # 게임이 작성한 원문 라벨과 실제 명령만 후속 클릭에 사용한다.
    labels = dict(view=choose(bar, text='View')['label'], entry=choose(bar, command=0x020000f0)['label'])
    players = visible_items(report, 'players')
    assert len(players) == len(states(report)['initial']['players']) > 0
    labels['player'] = players[0]['label']
    labels['find'] = choose(menu_items(report, 'players'), command=0x020000b8, parameter=1)['label']
    check_center(report, 'players', width, height)
    scenario = directory / 'scenario'
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Play Mission'), (20, 'report', 'initial'),
             (25, 'esc', ''), (30, 'click', labels['view']), (35, 'report', 'view'),
             (40, 'click', labels['entry']), (50, 'report', 'menuPlayers'),
             (50, 'snapshot', str(scenario / 'menu-players.bmp')),
             (55, 'click', labels['player']), (60, 'report', 'playerSubmenu'),
             (60, 'snapshot', str(scenario / 'player-submenu.bmp')),
             (65, 'click', labels['find']), (72, 'report', 'findPriest'),
             (80, 'key', 'f9'), (81, 'keyup', 'f9'), (90, 'report', 'keyPlayers'),
             (90, 'snapshot', str(scenario / 'key-players.bmp')),
             (95, 'esc', ''), (102, 'report', 'keyClosed'),
             (104, 'esc', ''), (107, 'report', 'keyBarClosed'),
             (110, 'key', 'shift+f9'), (111, 'keyup', 'shift+f9'), (120, 'report', 'paused'),
             (125, 'key', 'f9'), (126, 'keyup', 'f9'), (135, 'report', 'pausedPlayers'),
             (135, 'snapshot', str(scenario / 'paused-players.bmp')),
             (140, 'esc', ''), (145, 'report', 'pausedClosed'),
             (147, 'esc', ''), (149, 'report', 'pausedBarClosed'),
             (150, 'key', 'shift+f9'), (151, 'keyup', 'shift+f9'), (160, 'report', 'resumed'),
             (170, 'esc', ''), (175, 'click', labels['view']), (180, 'click', labels['entry']),
             (190, 'report', 'reopened'), (195, 'click', f'screen:{width-20},{height-20}'),
             (202, 'report', 'outsideClosed'), (210, 'esc', ''), (215, 'report', 'barAgain'),
             (220, 'esc', ''), (228, 'report', 'barClosed')]
    report, second_run = protected_run(scenario, width, height, steps, 234, data_root=data_root)
    observed = states(report)  # 입력 계획과 별개의 실제 제품 상태다.
    centered = {label: check_center(report, label, width, height)
                for label in ('menuPlayers', 'keyPlayers', 'pausedPlayers', 'reopened')}
    expected = [row['label'] for row in players]
    # 같은 최초 참가자 이름/순서를 메뉴/F9/정지 중/재열기 네 경로에서 대조한다.
    for label in centered:
        assert [row['label'] for row in visible_items(report, label)] == expected
        assert observed[label]['raw_menus'][1] == '0', 'controlled 스택 잔존'
    submenu = [row for row in visible_items(report, 'playerSubmenu') if row['command'] == 0x020000b8]
    assert len(submenu) == 1 and submenu[0]['parameter'] == 1
    # 닫기와 사제 찾기는 전체 공통 메뉴를 반납하며 game clock 정지는 만들지 않는다.
    for label in ('findPriest', 'keyBarClosed', 'pausedBarClosed', 'outsideClosed', 'barClosed'):
        assert not visible_items(report, label), (label, visible_items(report, label))
        assert observed[label]['raw_menus'][:3] == ['0', '0', '0'], (label, observed[label]['raw_menus'])
    # 원본 ESC는 목록 반납 뒤 serial이 없는 가로막대를 연다. 두 번째 ESC로 같은 막대를 닫는다.
    for label in ('keyClosed', 'pausedClosed'):
        assert choose(visible_items(report, label), text='View')
        assert not any(row[-1]=='Player List' for row in repeated(report, 'raw_menu_heading')[label]), '반납한 Player List 제목 잔존'
    # Shift+F9의 정지 중에도 F9/ESC는 동작하며 두 번째 Shift+F9만 시계를 재개한다.
    for label in ('initial', 'menuPlayers', 'keyPlayers', 'keyClosed', 'resumed', 'outsideClosed'):
        assert observed[label]['paused'] == ['0'], label
    for label in ('paused', 'pausedPlayers', 'pausedClosed'):
        assert observed[label]['paused'] == ['1'], label
        assert observed[label]['time'][0] == observed['paused']['time'][0], '정지 중 게임 시계 진행'
    assert float(observed['resumed']['time'][0]) > float(observed['paused']['time'][0])
    assert choose(visible_items(report, 'barAgain'), text='View')
    result = dict(size=[width, height], labels=labels, players=expected, centered=centered,
                  pauseResumeVerified=True, subMenuAndFindPriestVerified=True,
                  escAndOutsideCleanupVerified=True, runs=[first_run, second_run])
    save(directory / 'summary.json', result)
    return result


def main():
    """보호 원본과 분리한 명시 사본에서 두 화면비를 실행하고 모든 원시 결과를 보존한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, default=ROOT / 'originals')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 앞 화면비가 실패하면 후속 실행으로 가리지 않는다.
    for width, height in SIZES:
        results.append(one_size(output / f'{width}x{height}', width, height, args.data_root.resolve()))
    save(output / 'summary.json', dict(results=results))
    print('Player List 실제 두 화면비/키/정지/반납 검사 통과')


if __name__ == '__main__':
    main()
