#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 브리핑/ESC 메뉴의 캡처 취소·미션 이탈·재진입을 검사한다. 원본 게임은 실행하지 않는다."""
import argparse
import datetime
from pathlib import Path
import cpp_game_menu_smoke as game_menu
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_world_smoke import states


def steps(directory):
    """임시 Game 버튼 대신 실제 브리핑 버튼과 ESC 가로 메뉴에 마우스 사건을 넣는다."""
    return [
        (5, 'report', 'briefing'), (10, 'down', 'Play Mission'), (12, 'report', 'briefHeld'),
        (15, 'up', 'screen:1000,700'), (18, 'report', 'briefCancelled'),
        (20, 'click', 'Play Mission'), (30, 'report', 'initial'),
        (35, 'esc', ''), (40, 'report', 'bar'), (45, 'click', 'Game'),
        (50, 'report', 'game'), (55, 'click', 'menu-command:0200008c,0'),
        (60, 'report', 'leave'), (65, 'down', 'Continue Mission'), (68, 'report', 'cancelHeld'),
        (70, 'up', 'screen:1000,700'), (75, 'report', 'cancelReleased'),
        (80, 'click', 'Continue Mission'), (90, 'report', 'resumed'),
        (95, 'esc', ''), (100, 'click', 'Game'), (105, 'click', 'menu-command:0200008c,0'),
        (110, 'click', 'Main Menu'), (120, 'report', 'main'),
        (130, 'click', 'Campaign'), (140, 'click', 'Struggle For Freedom'),
        (150, 'click', '1 The War Begins!'), (170, 'report', 'reloadedBrief'),
        (180, 'click', 'Play Mission'), (190, 'report', 'reloaded'),
        (190, 'snapshot-presented', str(directory/'reloaded.bmp')),
        (195, 'esc', ''), (200, 'click', 'Game'), (205, 'click', 'menu-command:0200008c,0'),
        (210, 'click', 'Main Menu'), (220, 'report', 'mainFinal'),
        (225, 'click', 'Quit')]


def verify(report):
    """취소가 명령을 실행하지 않고 동일 HUD/캡처가 반납되는지 실제 원시 상태로 판정한다."""
    observed = states(report)
    # 두 실제 stone 버튼은 누른 동안 공통 캡처를 소유하고 바깥 뗌 뒤 반납해야 한다.
    for held, released in (('briefHeld', 'briefCancelled'), ('cancelHeld', 'cancelReleased')):
        assert int(observed[held]['RawButtons'][3]) != 0, (held, observed[held]['RawButtons'])
        assert observed[released]['RawButtons'][3] == '0', (released, observed[released]['RawButtons'])
    # 브리핑 버튼의 바깥 뗌은 미션을 시작하지 않는다.
    for label in ('briefing', 'briefHeld', 'briefCancelled', 'reloadedBrief'):
        assert observed[label]['phase'] == ['3'] and observed[label]['paused'] == ['1'], label
    # 중단 확인의 바깥 뗌도 확인창을 닫지 않고 이후 정상 클릭으로만 복귀한다.
    for label in ('leave', 'cancelHeld', 'cancelReleased'):
        assert observed[label]['page'][0].casefold() == 'abort', (label, observed[label]['page'])
    # 실제 동일 세션 복귀와 새 세션 진입 모두 진행 중이어야 하며 캡처를 남기지 않는다.
    for label in ('initial', 'resumed', 'reloaded'):
        row = observed[label]
        assert row['phase'] == ['4'] and row['paused'] == ['0'], (label, row['phase'], row['paused'])
        assert row['RawButtons'][3] == '0', (label, row['RawButtons'])
        assert row['state_hud'][0] == '1' and row['raw_world_modes'] == ['0', '1'], label
    assert observed['initial']['raw_uber_children'] == observed['resumed']['raw_uber_children']
    assert observed['initial']['raw_hud_owners'] == observed['resumed']['raw_hud_owners']
    # 세션을 나간 뒤 이전 월드가 없고 새 캠페인에서는 등록 serial이 증가해야 한다.
    for label in ('main', 'mainFinal'):
        assert observed[label]['phase'] == ['0'] and observed[label]['state_hud'][0] == '0', label
        assert observed[label]['RawButtons'][3] == '0', label
    assert int(observed['reloaded']['RawButtons'][2]) > int(observed['initial']['RawButtons'][2])
    assert observed['initial']['raw_uber_children'] != observed['reloaded']['raw_uber_children']
    assert observed['initial']['mission'] == observed['reloaded']['mission']
    items = game_menu.menu_items(report, 'bar')
    assert any(row['label'].strip() == 'Game' and row['visible'] for row in items)
    assert not any('CHEAT' in row['label'].upper() for row in items if row['visible'])
    return dict(states=len(observed), outside_release_checks=2, esc_menu=True,
                unchanged_hud_on_cancel=True, campaign_reentry=True,
                natural_completion_tested=False, physical_input_tested=False)


def main():
    """실행 사본과 출력 경로를 선택하고 기존 실행기/설정 복원/원본 보호 검사를 재사용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=GAME)
    parser.add_argument('--data-root', type=Path, default=ROOT/'originals')
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    parser.add_argument('--output', type=Path, default=ROOT/'extracted'/('cpp-rawbuttons-smoke-'+stamp))
    args = parser.parse_args()
    directory = args.output.resolve()
    game_menu.GAME = args.exe.resolve()  # 기존 protected_run이 관측할 실제 클론 실행 파일을 명시한다.
    report, run = game_menu.protected_run(directory, 1024, 768, steps(directory), 235,
                                          data_root=args.data_root, mission='TheWarBegins')
    summary = verify(report)
    summary.update(run=run, binarySha256=sha(game_menu.GAME))
    save(directory/'summary.json', summary)
    print('실제 ESC/브리핑 캡처 취소와 캠페인 재진입 통과')


if __name__ == '__main__':
    main()
