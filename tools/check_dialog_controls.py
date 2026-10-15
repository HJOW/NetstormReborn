#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 클론 Kernel의 Tell 뒤 페이지/명령/입력 트리 교체와 Warning의 캡처 보존을 검사한다."""
import argparse
import json
import subprocess
from pathlib import Path

from cpp_menu_smoke import reports
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings

# 원본과 분리한 저장소 루트 및 검사 보고 경로다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/dialog-controls'


def run_scene(executable, name, steps, mission=None):
    """실제 Win32 클론을 무음 실행하고 프레임/실패 출력을 검사 폴더에 보존한다."""
    script = OUTPUT / f'{name}.tsv'
    state_path = OUTPUT / f'{name}-states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')
    arguments = [str(executable), '--run', str(ROOT / 'originals'), '--window', '--no-audio',
                 '--frames', str(steps[-1][0] + 2), '--set', 'SCREENW=640;SCREENH=480;maxFPS=75',
                 '--ui-script', str(script), '--ui-report', str(state_path)]
    if mission:
        arguments.extend(['--mission', mission])
    result = subprocess.run(arguments, capture_output=True, timeout=60)
    (OUTPUT / f'{name}.stdout.log').write_bytes(result.stdout)
    (OUTPUT / f'{name}.stderr.log').write_bytes(result.stderr)
    assert result.returncode == 0, f'{name}: 종료 코드 {result.returncode}: {result.stderr.decode("utf-8", errors="replace")}'
    return reports(state_path)


def raw(state):
    """보고의 버튼 수·등록 수·serial·캡처·idle·실제 노드 수를 정수로 읽는다."""
    return list(map(int, state['RawButtons']))


def page(state, expected, labels, phase, paused):
    """실제 그리기와 같은 입력 영역/라벨 및 페이지 정지를 함께 확인한다."""
    assert state['page'] == [expected], (expected, state['page'])
    assert set(state['controls']) == set(labels), (expected, state['controls'])
    assert state['phase'] == [str(phase)] and state['paused'] == [str(paused)]
    buttons = sum(not control[1] for control in state['controls'].values())
    assert raw(state)[0] == buttons and raw(state)[1] == 1 + buttons * 2
    assert raw(state)[5] == 1 + buttons * 2


def check_menu(executable):
    """메뉴/목록/모달과 반복 페이지 교체가 이전 캡처·명령·등록을 재사용하지 않는지 검사한다."""
    main = {'Campaign', 'Multiplayer', 'Demo', 'Help', 'Edit', 'Credits', 'Options', 'Quit'}
    steps = [
        (1, 'report', 'main'), (2, 'down', 'Campaign'), (2, 'report', 'capture'), (2, 'refresh', ''),
        (3, 'report', 'refreshed'), (4, 'kernel-tell', 'Credits'), (4, 'report', 'credits'),
        (4, 'snapshot', str(OUTPUT / 'credits.bmp')), (5, 'up', 'Cancel'), (6, 'report', 'stale_release'),
        (7, 'click', 'Cancel'), (8, 'report', 'cancel'), (9, 'kernel-tell', 'About'), (9, 'report', 'about'),
        (10, 'click', 'OK'), (11, 'report', 'ok'), (12, 'down', 'Campaign'), (12, 'report', 'warning_before'),
        (13, 'kernel-tell', 'PriestImmobile'), (13, 'report', 'warning'), (14, 'up', 'Campaign'),
        (15, 'report', 'warning_release'), (16, 'kernel-tell', 'LeaveNormal'), (16, 'report', 'leave'),
        (17, 'click', 'Stay'), (18, 'report', 'stay'), (19, 'click', 'Options'), (20, 'report', 'popup'),
        (21, 'kernel-tell', 'About'), (21, 'report', 'popup_replaced'), (22, 'click', 'OK'),
        (23, 'report', 'popup_ok'), (24, 'kernel-tell', 'Credits'),
        (25, 'click', 'Netstorm 10.72 Patch Credits'), (26, 'report', 'menu_command'),
        (27, 'kernel-tell', 'Blank'), (27, 'report', 'blank')]
    # 다수의 실제 Kernel 페이지 교체에서 등록 누적/오래된 serial 재사용을 검사한다.
    for cycle in range(32):
        frame = 28 + cycle * 2
        steps.extend([(frame, 'kernel-tell', 'About'), (frame, 'report', f'cycle-{cycle}-dialog'),
                      (frame + 1, 'kernel-tell', 'Blank'), (frame + 1, 'report', f'cycle-{cycle}-main')])
    states = run_scene(executable, 'menu', steps)
    page(states['main'], 'main', main, 0, 0)
    assert raw(states['capture'])[3] and raw(states['capture'])[4] == 1
    assert raw(states['capture']) == raw(states['refreshed']), 'Tick 표시 갱신이 캡처/등록을 바꿈'
    credits = {'Netstorm 10.72 Patch Credits', 'Netstorm Original Credits', 'Cancel'}
    page(states['credits'], 'Credits', credits, 1, 0)
    assert raw(states['credits'])[3:5] == [0, 0]
    page(states['stale_release'], 'Credits', credits, 1, 0)
    page(states['cancel'], 'main', main, 0, 0)
    page(states['about'], 'About', {'OK'}, 1, 0)
    page(states['ok'], 'main', main, 0, 0)
    page(states['warning'], 'main', main, 0, 0)
    assert raw(states['warning_before']) == raw(states['warning']), 'Warning이 기존 버튼 캡처/등록을 바꿈'
    assert states['warning']['warning_count'][0] == '1'
    assert states['warning_release']['page'] == ['UCampaign'], 'Warning 뒤 캡처의 원래 명령이 실행되지 않음'
    page(states['leave'], 'LeaveNormal', {'Leave Honorably', 'Stay'}, 1, 0)
    page(states['stay'], 'main', main, 0, 0)
    assert states['popup']['popup'] == ['options']
    page(states['popup_replaced'], 'About', {'OK'}, 1, 0)
    assert states['popup_replaced']['popup'] == ['']
    page(states['popup_ok'], 'main', main, 0, 0)
    assert states['menu_command']['page'] == ['CreditsNew'], '새 목록의 실제 명령이 실행되지 않음'
    page(states['blank'], 'main', main, 0, 0)
    previous = raw(states['blank'])[2]
    # 각 페이지에서 살아 있는 노드 수와 새로 생성한 serial 증가량을 직접 확인한다.
    for cycle in range(32):
        dialog, menu = states[f'cycle-{cycle}-dialog'], states[f'cycle-{cycle}-main']
        page(dialog, 'About', {'OK'}, 1, 0)
        page(menu, 'main', main, 0, 0)
        assert raw(dialog)[2] == previous + 2 and raw(menu)[2] == previous + 18
        assert raw(dialog)[3:5] == [0, 0] and raw(menu)[3:5] == [0, 0]
        previous = raw(menu)[2]
    return {'states': len(states), 'repeat_cycles': 32, 'tick_refresh_capture_preserved': True,
            'kernel_page_replaced': True, 'stale_release_rejected': True, 'warning_capture_preserved': True}


def check_mission(executable):
    """실제 1-1 월드의 Game 버튼 캡처·Kernel 대화상자·Warning·Blank 복귀를 검사한다."""
    steps = [(3, 'report', 'briefing'), (4, 'click', 'Play Mission'), (5, 'report', 'mission'),
             (6, 'down', 'Game'), (6, 'report', 'capture'), (7, 'kernel-tell', 'LeaveNormal'),
             (7, 'report', 'leave'), (7, 'snapshot', str(OUTPUT / 'mission-leave.bmp')),
             (8, 'up', 'Stay'), (9, 'report', 'stale_release'), (10, 'click', 'Stay'),
             (11, 'report', 'resumed'), (12, 'kernel-tell', 'About'),
             (12, 'kernel-tell', 'PriestImmobile'), (12, 'report', 'modal_warning'),
             (13, 'click', 'OK'), (14, 'report', 'modal_ok'), (15, 'down', 'Game'),
             (15, 'report', 'warning_before'), (16, 'kernel-tell', 'PriestImmobile'),
             (16, 'report', 'warning'), (16, 'refresh', ''), (17, 'report', 'warning_refreshed'),
             (18, 'up', 'Game'), (19, 'report', 'warning_release'),
             (20, 'kernel-tell', 'Blank'), (20, 'report', 'blank')]
    states = run_scene(executable, 'mission', steps, 'thewarbegins')
    page(states['mission'], 'mission', {'Game'}, 4, 0)
    assert raw(states['capture'])[3] and raw(states['capture'])[4] == 1
    page(states['leave'], 'LeaveNormal', {'Leave Honorably', 'Stay'}, 1, 1)
    assert raw(states['leave'])[3:5] == [0, 0]
    page(states['stale_release'], 'LeaveNormal', {'Leave Honorably', 'Stay'}, 1, 1)
    page(states['resumed'], 'mission', {'Game'}, 4, 0)
    page(states['modal_warning'], 'About', {'OK'}, 1, 1)
    assert states['modal_warning']['warning_count'][0] == '1'
    page(states['modal_ok'], 'mission', {'Game'}, 4, 0)
    page(states['warning'], 'mission', {'Game'}, 4, 0)
    assert raw(states['warning_before']) == raw(states['warning']) == raw(states['warning_refreshed'])
    assert states['warning_release']['popup'] == ['game']
    page(states['blank'], 'mission', {'Game'}, 4, 0)
    return {'states': len(states), 'mission': 'thewarbegins', 'kernel_modal_paused': True,
            'modal_warning_kept_page': True, 'warning_kept_capture_and_running': True, 'blank_resumed': True}


def main():
    """허용된 설정 두 파일을 복원하고 보호 원본 전체의 파일명/해시가 같은지 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    arguments = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    root = ROOT / 'originals'
    before = snapshots(root)
    try:
        with preserve_game_settings(root, 'd'):
            result = {'menu': check_menu(arguments.exe), 'mission': check_mission(arguments.exe)}
    finally:
        assert snapshots(root) == before, '보호 원본의 파일명/내용이 바뀜'
    result['original_files_unchanged'] = True
    result['original_game_executions'] = 0
    (OUTPUT / 'report.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
