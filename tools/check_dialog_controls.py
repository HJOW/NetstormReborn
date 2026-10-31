#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ESC/HTML 부모의 입력·캡처·경고·모달 수명을 검사한다. 원본 게임은 실행하지 않는다."""
import argparse
import json
import re
import subprocess
from pathlib import Path

from cpp_world_smoke import states as reports
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings

# 원본과 분리한 저장소 루트 및 검사 보고 경로다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/dialog-controls'


def run_scene(executable, name, steps, mission=None, settings=''):
    """현재 클론을 실행하고 단일 필드와 반복 소유자/노드 보고를 함께 보존한다."""
    if not mission:
        from cpp_audio_smoke import after_startup
        steps, _ = after_startup(steps, steps[-1][0] + 2) # UserNotUpdated의 실제 OK를 닫은 뒤 독립 HTML 수명을 시작한다.
    script = OUTPUT / f'{name}.tsv'  # 이번 검사에서 실제 입력 큐에 전달할 프레임 목록이다.
    state_path = OUTPUT / f'{name}-states.tsv'  # 같은 프레임의 현재 장치 관측이다.
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')
    arguments = [str(executable), '--run', str(ROOT / 'originals'), '--window', '--no-audio',
                 '--frames', str(steps[-1][0] + 2), '--set',
                 'SCREENW=640;SCREENH=480;maxFPS=75;autoDemo=0;tellTips=0;'+settings,
                 '--ui-script', str(script), '--ui-report', str(state_path)]  # 자동 안내를 배제하고 실제 UI 수명만 관측한다.
    if mission:
        arguments.extend(['--mission', mission])
    result = subprocess.run(arguments, capture_output=True, timeout=60)  # 새 C++ 실행 파일만 실행하는 기존 경계다.
    (OUTPUT / f'{name}.stdout.log').write_bytes(result.stdout)
    (OUTPUT / f'{name}.stderr.log').write_bytes(result.stderr)
    assert result.returncode == 0, f'{name}: 종료 코드 {result.returncode}: {result.stderr.decode("utf-8", errors="replace")}'
    observed = reports(state_path)  # 객체/버튼의 반복 레코드를 버리지 않는 현재 판독기다.
    # 등록된 실제 노드와 HTML/메뉴 소유자를 같은 보고 블록에 그대로 붙인다.
    for block in re.split(r'(?m)^frame\t', state_path.read_text(encoding='utf-8'))[1:]:
        header, body = block.split('\n', 1)  # 프레임 본문 안의 다른 frame 접미 필드를 분리하지 않는다.
        _, label = header.split('\t', 1)  # 입력 계획이 지정한 실제 보고 이름이다.
        observed[label]['rows'] = [line.split('\t') for line in body.splitlines()]  # 반복 순서도 보존한다.
    return observed


def raw(state):
    """기존 RawButtons의 현재 등록 수·serial·캡처·idle을 정수로 읽는다."""
    return list(map(int, state['RawButtons']))


def rows(state, kind):
    """같은 실제 보고 블록에서 지정한 종류의 반복 레코드를 모두 읽는다."""
    return [row[1:] for row in state['rows'] if row[0] == kind]


def nodes(state):
    """공통 등록표의 토큰과 실제 flags/depth/자식/형제/부모를 연결한다."""
    records = rows(state, 'raw_modal_node')  # 전체 등록표를 순회한 현재 레코드다.
    result = {int(row[0]): tuple(map(int, row[1:])) for row in records}  # 같은 토큰을 중복 등록하지 않아야 한다.
    assert len(result) == len(records), records
    if 'raw_modal_globals' in state:
        assert records, 'HTML 장치가 연결됐지만 공통 등록표 보고가 비어 있음'
        assert len(result) == raw(state)[1], (len(result), raw(state))
    return result


def subtree(state, root):
    """부모 토큰을 따라 현재 HTML 가족 전체를 골라 반납 뒤 잔류를 검사한다."""
    current = nodes(state)  # 보고가 읽은 같은 공통 등록표다.
    result = {root}  # 현재 실제 부모에서 시작한다.
    # 모든 직접/간접 자식을 포함할 때까지 실제 부모 링크를 따라 확장한다.
    while True:
        extended = result | {token for token, row in current.items() if row[4] in result}  # 현재 가족을 부모로 둔 다음 세대의 실제 노드다.
        if extended == result:
            return result
        result = extended


def dialog(state):
    """현재 모달의 HTML 부모·본문·버튼을 실제 공통 노드 소유 관계와 대조한다."""
    root = int(state['current_dialog'][0])  # 동일 GumpInput이 우선 분배할 현재 부모다.
    roots = [row for row in rows(state, 'raw_html_root') if int(row[0]) == root]  # 이전 스택 부모와 구별한다.
    assert root and len(roots) == 1, (root, roots)
    owner = roots[0]  # 토큰/serial/flags/버튼수/본문 토큰이다.
    current = nodes(state)  # 공통 등록표의 실제 부모 링크다.
    buttons = [row for row in rows(state, 'raw_html_button') if int(row[0]) == root]  # 해당 부모가 보유한 버튼만 읽는다.
    assert root in current and not (current[root][0] & 0x10000), (root, current.get(root))
    # Metadata의 $Button/Instant만 AddButton을 호출한다. $Menu/Checked는 별도 menuColumns다.
    # Fit도 controls[] 전체를 stone으로 측정하므로 controlled 메뉴를 버튼 수에 더하지 않는다.
    assert len(buttons) == int(owner[3]), (owner, buttons)
    assert [int(row[1]) for row in buttons] == list(range(int(owner[3]))), (owner, buttons)
    assert int(owner[4]) in current and current[int(owner[4])][4] == root, owner
    # 버튼 토큰/serial/명령/인자가 같은 현재 부모에 속하고 실제 표시되는지 확인한다.
    for row in buttons:
        child = int(row[2])  # 실제 HtmlWindowControl.child 토큰이다.
        assert child in current and current[child][4] == root and row[10] == '1', row
        assert int(row[5]) > 0 and int(row[3]) != 0, row
    return root, owner, buttons


def dialog_menus(state, root):
    """controls[]와 별개인 실제 controlled 메뉴의 등록·부착·명령 행을 대조한다."""
    family = subtree(state, root)  # Fit의 attachMenu가 같은 HTML 부모에 부착한 가족이다.
    current = nodes(state)  # 동일 등록표의 실제 parent를 읽는다.
    records = [row for row in rows(state, 'raw_menu') if int(row[0]) in family]  # 다른 모달의 메뉴는 제외한다.
    objects = {int(row[0]): row for row in rows(state, 'raw_menu_object')}  # 같은 메뉴의 등록 serial이다.
    result = []  # 수명 정체성과 원문 명령만 보관하며 hover selection은 비교하지 않는다.
    # 각 controlled 루트와 실제 모든 명령 행이 같은 HTML 가족에 있어야 한다.
    for row in records:
        token = int(row[0])  # 실제 MenuGump 토큰이다.
        assert token in current and int(row[2]) in family and current[token][4] == int(row[2]), row
        assert not (current[token][0] & 0x10000) and token in objects and int(objects[token][3]) > 0, row
        items = [item for item in rows(state, 'raw_menu_item') if int(item[0]) == token]  # 같은 메뉴가 소유한 순서다.
        assert items and all(item[5] == '1' for item in items), (row, items)
        result.append((row[0], row[1], row[2], row[4], objects[token][3], tuple(tuple(item) for item in items)))
    return tuple(result)


def identity(state):
    """현재 부모/본문과 버튼의 토큰·serial·명령·인자만 묶어 같은 소유자를 증명한다."""
    root, owner, buttons = dialog(state)  # 일시 dirty/깊이 변경과 수명 정체성을 구별한다.
    return (root, owner[1], owner[3], owner[4]), tuple(tuple(row[:6]) for row in buttons), dialog_menus(state, root)


def labels(state):
    """현재 모달 자식의 실제 메뉴와 stone 버튼 또는 메인 호스트 입력 라벨을 읽는다."""
    root = int(state.get('current_dialog', ['0'])[0])  # 모달이 있으면 그 가족만 입력할 수 있다.
    if not root:
        return set(state['controls'])
    _, _, buttons = dialog(state)  # 같은 실제 부모의 버튼 라벨이다.
    family = subtree(state, root)  # 부모에 부착된 controlled 메뉴도 포함한다.
    dialog_menus(state, root)  # 메뉴의 실제 부모/등록 serial을 버튼 배열과 따로 검증한다.
    return {row[11] for row in buttons} | {row[6] for row in rows(state, 'raw_menu_item')
                                         if int(row[0]) in family and row[5] == '1'}


def page(state, expected, expected_labels, phase, paused):
    """라벨·phase/정지와 현재 소유자 노드를 확인하며 전체 UI를 가짜 1+2N으로 세지 않는다."""
    assert state['page'][0].casefold() == expected.casefold(), (expected, state['page'])
    assert labels(state) == set(expected_labels), (expected, labels(state))
    assert state['phase'] == [str(phase)] and state['paused'] == [str(paused)], state['page']
    nodes(state)
    if int(state.get('current_dialog', ['0'])[0]):
        dialog(state)
    elif expected == 'main':
        assert raw(state)[0] == len(expected_labels), raw(state)


def capture_preserved(before, after):
    """경고의 새 노드/serial과 기존 캡처 및 기존 부모 토큰의 보존을 구별한다."""
    focus = raw(before)[3]  # 기존 실제 stone 버튼이 보유한 공통 캡처다.
    assert focus and raw(after)[3] == focus, (raw(before), raw(after))
    initial, current = nodes(before), nodes(after)  # 경고를 추가해도 기존 노드를 반납해서는 안 된다.
    assert initial.keys() <= current.keys() and focus in current
    assert initial[focus][4] == current[focus][4], (initial[focus], current[focus])
    # Warning 종류17은 TextGump 한 노드를 만든다. 비표시 HTML 임시 부모는 이미 공통 Cleanup으로 반납된다.
    assert len(current) - len(initial) == int(after['warning_count'][0]) - int(before['warning_count'][0])
    common_root = initial[int(before['current_dialog'][0])][4] if int(before['current_dialog'][0]) else initial[focus][4]  # Warning은 HTML 자식이 아닌 공통 루트의 직계 자식이다.
    assert common_root and all(current[token][4] == common_root for token in current.keys() - initial.keys())
    assert before['current_dialog'] == after['current_dialog']
    if int(before['current_dialog'][0]):
        assert identity(before) == identity(after)


def check_menu(executable):
    """Kernel Tell의 캡처 취소/경고 보존과 실제 HTML 가족의 반복 생성·반납을 검사한다."""
    main = {'Campaign', 'Multiplayer', 'Demo', 'Help', 'Edit', 'Credits', 'Options', 'Quit'}  # 실제 메인 버튼이다.
    credits = {'Netstorm 10.72 Patch Credits', 'Netstorm Original Credits', 'Cancel'}  # 메뉴 두 행과 stone 버튼이다.
    steps = [
        (5, 'report', 'main'), (10, 'down', 'Campaign'), (12, 'report', 'capture'), (15, 'refresh', ''),
        (20, 'report', 'refreshed'), (25, 'kernel-tell', 'Credits'), (30, 'report', 'credits'),
        (30, 'snapshot', str(OUTPUT / 'credits.bmp')), (35, 'up', 'Cancel'), (40, 'report', 'stale_release'),
        (45, 'click', 'Cancel'), (50, 'report', 'cancel'), (55, 'kernel-tell', 'About'), (60, 'report', 'about'),
        (65, 'click', 'OK'), (70, 'report', 'ok'), (75, 'down', 'Campaign'), (80, 'report', 'warning_before'),
        (85, 'kernel-tell', 'PriestImmobile'), (85, 'report', 'warning_queued'), (90, 'report', 'warning'),
        (95, 'up', 'Campaign'), (100, 'report', 'warning_release'),
        (105, 'kernel-tell', 'Blank'), (110, 'report', 'campaign_closed'),
        (115, 'kernel-tell', 'LeaveNormal'), (120, 'report', 'leave'), (125, 'click', 'Stay'), (130, 'report', 'stay'),
        (135, 'click', 'Options'), (140, 'report', 'popup'), (145, 'kernel-tell', 'About'), (150, 'report', 'popup_replaced'),
        (155, 'click', 'OK'), (160, 'report', 'popup_ok'), (165, 'kernel-tell', 'Credits'),
        (170, 'click', 'Netstorm 10.72 Patch Credits'), (175, 'report', 'menu_command'),
        (180, 'kernel-tell', 'Blank'), (185, 'report', 'blank')]
    # 반복마다 실제 모달 가족을 만든 뒤 Blank의 공통 Kill/Cleanup으로 모두 반납한다.
    for cycle in range(32):
        frame = 190 + cycle * 20  # 각 실제 Kernel/입력/후반 Cleanup 사이에 관측 간격을 둔다.
        steps.extend([(frame, 'kernel-tell', 'About'), (frame + 5, 'report', f'cycle-{cycle}-dialog'),
                      (frame + 10, 'kernel-tell', 'Blank'), (frame + 15, 'report', f'cycle-{cycle}-main')])
    states = run_scene(executable, 'menu', steps)  # 입력과 같은 프레임의 장치 보고다.
    page(states['main'], 'main', main, 0, 0)
    assert raw(states['capture'])[3] and raw(states['capture'])[4] > 0
    assert raw(states['capture']) == raw(states['refreshed']), '표시 갱신이 기존 캡처/등록을 바꿈'
    page(states['credits'], 'Credits', credits, 1, 0)
    assert raw(states['credits'])[3] == 0
    page(states['stale_release'], 'Credits', credits, 1, 0)
    assert identity(states['credits']) == identity(states['stale_release']), '누름 없는 뗌이 새 부모를 실행함'
    page(states['cancel'], 'main', main, 0, 0)
    page(states['about'], 'About', {'OK'}, 1, 0)
    page(states['ok'], 'main', main, 0, 0)
    page(states['warning'], 'main', main, 0, 0)
    capture_preserved(states['warning_before'], states['warning'])
    warning_tokens = nodes(states['warning']).keys() - nodes(states['warning_before']).keys()  # 실제 추가된 한 TextGump 토큰을 저장해 생존/dead 예약/반납을 모두 구별한다.
    assert len(warning_tokens) == 1, warning_tokens
    assert states['warning_queued']['warning_count'] == ['0'] and states['warning']['warning_count'] == ['1']
    assert states['warning_release']['page'] == ['UCampaign'] and int(states['warning_release']['current_dialog'][0])
    assert dialog_menus(states['warning_release'], int(states['warning_release']['current_dialog'][0])), '실제 Campaign 목록 메뉴 부착 누락'
    assert raw(states['warning_release'])[3] == 0, '경고 뒤 기존 Campaign 버튼 명령/캡처 반납 누락'
    page(states['campaign_closed'], 'main', main, 0, 0)
    page(states['leave'], 'LeaveNormal', {'Leave Honorably', 'Stay'}, 1, 0)
    page(states['stay'], 'main', main, 0, 0)
    assert states['popup']['popup'] == ['options']
    page(states['popup_replaced'], 'About', {'OK'}, 1, 0)
    assert states['popup_replaced']['popup'] == ['']
    page(states['popup_ok'], 'main', main, 0, 0)
    assert states['menu_command']['page'] == ['CreditsNew'], '현재 메뉴 행의 실제 명령 누락'
    page(states['blank'], 'main', main, 0, 0)
    baseline = len(nodes(states['blank'])) - len(nodes(states['blank']).keys() & warning_tokens)  # 별도 루트의 경고 토큰은 벽 시각10초 뒤 dead 예약→Cleanup 순서로 반납될 수 있다.
    previous = raw(states['blank'])[2]  # 직전 공통 등록 serial이다.
    # 고정 +2/+18 대신 새 부모/버튼 serial과 이전 가족의 전원 반납을 확인한다.
    for cycle in range(32):
        current, returned = states[f'cycle-{cycle}-dialog'], states[f'cycle-{cycle}-main']  # 같은 반복의 양 끝이다.
        page(current, 'About', {'OK'}, 1, 0)
        root, owner, buttons = dialog(current)  # 실제 새 HTML 가족이다.
        family = subtree(current, root)  # 본문/장식/버튼까지 같은 부모 링크를 따라 찾는다.
        assert int(owner[1]) > previous and all(int(row[5]) > int(owner[1]) for row in buttons)
        page(returned, 'main', main, 0, 0)
        assert not family.intersection(nodes(returned)), 'Blank 뒤 이전 HTML 가족이 등록표에 남음'
        assert not family.intersection(warning_tokens), '공통 루트의 경고 노드가 HTML 가족에 포함됨'
        assert len(nodes(returned)) - len(nodes(returned).keys() & warning_tokens) == baseline, '해당 Warning 토큰의 자연 반납을 제외한 실제 메인 등록 노드 누적'
        assert raw(current)[3] == raw(returned)[3] == 0 and returned['current_dialog'] == ['0']
        previous = raw(returned)[2]
    return {'states': len(states), 'repeat_cycles': 32, 'tick_refresh_capture_preserved': True,
            'kernel_page_capture_cancelled': True, 'stale_release_rejected': True,
            'warning_capture_preserved': True, 'html_family_released': True}


def check_mission(executable):
    """실제 브리핑/ESC/Abort와 HTML 스택의 캡처 취소·경고·Blank 수명을 검사한다."""
    abort = {'Main Menu', 'Replay Mission', 'Continue Mission'}  # 원문 ABORT의 실제 세 버튼이다.
    steps = [
        (5, 'report', 'briefing'), (10, 'down', 'Play Mission'), (15, 'report', 'brief_held'),
        (20, 'up', 'screen:639,479'), (25, 'report', 'brief_cancelled'),
        (30, 'click', 'Play Mission'), (40, 'report', 'mission'),
        (45, 'esc', ''), (50, 'report', 'bar'), (55, 'click', 'Game'), (60, 'report', 'game'),
        (65, 'click', 'menu-command:0200008c,0'), (75, 'report', 'leave'),
        (75, 'snapshot', str(OUTPUT / 'mission-leave.bmp')), (80, 'down', 'Continue Mission'),
        (85, 'report', 'warning_before'), (90, 'kernel-tell', 'PriestImmobile'), (90, 'report', 'warning_queued'),
        (95, 'report', 'warning'), (100, 'refresh', ''), (105, 'report', 'warning_refreshed'),
        (110, 'up', 'screen:639,479'), (115, 'report', 'leave_cancelled'),
        (125, 'click', 'Continue Mission'), (135, 'report', 'resumed'),
        (145, 'kernel-tell', 'About'), (150, 'report', 'modal'),
        (155, 'kernel-tell', 'PriestImmobile'), (155, 'report', 'modal_warning_queued'),
        (160, 'report', 'modal_warning'), (165, 'down', 'OK'), (170, 'report', 'modal_held'),
        (175, 'kernel-tell', 'About'), (180, 'report', 'stacked'), (185, 'up', 'OK'),
        (190, 'report', 'stale_release'), (195, 'click', 'OK'), (200, 'report', 'underlying'),
        (205, 'kernel-tell', 'Blank'), (215, 'report', 'blank')]
    states = run_scene(executable, 'mission', steps, 'thewarbegins')  # 원본/제품 상태를 직접 주입하지 않는다.
    # 실제 stone 버튼을 바깥에서 떼면 같은 부모가 남고 공통 캡처만 반납한다.
    for held, released in (('brief_held', 'brief_cancelled'), ('warning_before', 'leave_cancelled')):
        assert raw(states[held])[3] and raw(states[released])[3] == 0, (held, released)
        assert identity(states[held]) == identity(states[released]), (held, released)
    for label in ('briefing', 'brief_held', 'brief_cancelled'):  # stone 바깥 뗌이 정지 브리핑을 시작하거나 닫지 않아야 한다.
        assert states[label]['phase'] == ['3'] and states[label]['paused'] == ['1'], label
    # ESC 가로 막대와 Game 하위 메뉴를 실제 메뉴 명령으로 열어 Abort를 만든다.
    for label in ('bar', 'game'):  # 가로 막대와 Game 하위 열기 모두 별도 게임 시계 정지를 추가하지 않는다.
        assert states[label]['game_menu_bar'][1] == '1' and states[label]['paused'] == ['0'], label
    assert any(row[6].strip() == 'Game' and row[5] == '1' for row in rows(states['bar'], 'raw_menu_item'))
    assert any(row[3] == str(0x0200008c) and row[4] == '0' and row[5] == '1'
               for row in rows(states['game'], 'raw_menu_item'))
    for label in ('leave', 'warning_before', 'warning_queued', 'warning', 'warning_refreshed', 'leave_cancelled'):  # 캡처/경고 동안 같은 ABORT 부모와 정지를 보존한다.
        page(states[label], 'abort', abort, 1, 1)
    capture_preserved(states['warning_before'], states['warning'])
    capture_preserved(states['warning_before'], states['warning_refreshed'])
    assert states['warning_queued']['warning_count'] == ['0']
    assert states['warning']['warning_count'] == states['warning_refreshed']['warning_count'] == ['1']
    # 빈 본문의 Warning은 기존 About 부모를 교체하지 않고 다음 입력에서 실제 메시지만 추가한다.
    page(states['modal'], 'About', {'OK'}, 1, 1)
    page(states['modal_warning'], 'About', {'OK'}, 1, 1)
    assert identity(states['modal']) == identity(states['modal_warning'])
    assert states['modal_warning_queued']['warning_count'] == states['modal']['warning_count']
    assert int(states['modal_warning']['warning_count'][0]) == int(states['modal']['warning_count'][0]) + 1
    # 같은 About을 새로 열면 새 모달이 쌓이고, Compose의 실제 캡처 취소로 오래된 뗌을 거부한다.
    assert raw(states['modal_held'])[3] and raw(states['stacked'])[3] == 0
    first_root, first_owner, _ = dialog(states['modal_held'])  # 반납 전의 실제 하단 부모다.
    top_root, _, _ = dialog(states['stacked'])  # 새 현재 모달이다.
    for label in ('modal_held', 'stacked', 'stale_release', 'underlying'):  # 누름·새 모달·오래된 뗌·하단 복원 모두 실제 About과 정지 상태다.
        page(states[label], 'About', {'OK'}, 1, 1)
    assert first_root != top_root and states['stacked']['raw_modal_globals'][0] == '2'
    assert any(row[:2] == first_owner[:2] for row in rows(states['stacked'], 'raw_html_root'))
    assert identity(states['stacked']) == identity(states['stale_release'])
    assert dialog(states['underlying'])[0] == first_root and states['underlying']['raw_modal_globals'][0] == '1'
    assert identity(states['underlying']) == identity(states['modal_held'])
    # 모달/Blank 왕복은 같은 실제 월드와 HUD를 유지하고 일반 진행으로 돌아온다.
    for label in ('mission', 'resumed', 'blank'):
        page(states[label], 'mission', set(), 4, 0)
        assert states[label]['state_hud'][0] == '1' and states[label]['raw_world_modes'] == ['0', '1']
        assert raw(states[label])[3] == 0 and states[label]['current_dialog'] == ['0']
        assert states[label]['raw_uber_children'] == states['mission']['raw_uber_children']
        assert states[label]['raw_hud_owners'] == states['mission']['raw_hud_owners']
    assert not subtree(states['stacked'], top_root).intersection(nodes(states['blank']))
    assert not subtree(states['modal_held'], first_root).intersection(nodes(states['blank']))
    return {'states': len(states), 'mission': 'thewarbegins', 'esc_menu': True,
            'outside_release_checks': 2, 'kernel_modal_capture_cancelled': True,
            'modal_stack_restored': True, 'warning_kept_capture_and_page': True,
            'blank_kept_world': True, 'natural_completion_tested': False}


def main():
    """허용된 설정 두 파일을 복원하고 보호 원본 전체의 파일명/해시가 같은지 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)  # 새 C++ 실행 파일만 선택한다.
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/Netstorm.exe')
    arguments = parser.parse_args()  # 기본 원본은 읽기 자료이며 설정 두 파일만 기존 보호 경계에서 복원한다.
    OUTPUT.mkdir(parents=True, exist_ok=True)
    root = ROOT / 'originals'  # 보호할 전체 원본 자료다.
    before = snapshots(root)  # 실행 전 파일명/해시다.
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
