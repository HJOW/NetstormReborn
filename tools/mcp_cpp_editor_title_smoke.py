#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 State1/Test Battle/편집 복귀의 Win32 제목을 원문 설정 키로 구별한다."""
import argparse
import base64
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import threading

from mcp_cpp_smoke import McpClient
from mcp_cpp_mission_abort_smoke import SAFE_BUTTONS, file_manifest


def window_title(pid):
    """이번에 실행한 프로세스의 보이는 최상위 창 제목만 읽고 활성화/입력은 하지 않는다."""
    api = ctypes.WinDLL('user32', use_last_error=True)
    api.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    api.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    api.IsWindowVisible.argtypes = [wintypes.HWND]
    titles = []  # 다른 프로세스/숨은 창의 제목은 수집하지 않는다.
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def visit(window, parameter):
        """소유 PID와 표시 상태를 모두 확인한 창에서 현재 UTF-16 제목을 읽는다."""
        owner = wintypes.DWORD()
        api.GetWindowThreadProcessId(window, ctypes.byref(owner))
        if owner.value == pid and api.IsWindowVisible(window):
            buffer = ctypes.create_unicode_buffer(1024)
            api.GetWindowTextW(window, buffer, len(buffer))
            titles.append(buffer.value)
        return True

    callback = callback_type(visit)
    api.EnumWindows(callback, 0)
    assert len(titles) == 1, titles
    return titles[0]


def main():
    """실제 메뉴/시작 안내만 조작하고 제목·현재 World·자료 불변을 관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    game, output = args.game.resolve(), args.output.resolve()
    root = Path(__file__).resolve().parents[1]
    for name in ('originals', 'originalCD'):  # 실제 편집 저장을 보호 원본에 보내지 않는다.
        protected = (root / name).resolve()
        assert all(path != protected and protected not in path.parents for path in (game, output))
    output.mkdir(parents=True, exist_ok=False)
    (output / 'tool-at-launch.py').write_bytes(Path(__file__).read_bytes())
    before = file_manifest(game)
    command = [str(args.exe.resolve()), '--run', str(game), '--mcp', '--window', '--no-audio', '--set',
               'intro=1;tellTips=0;autoDemo=0;currentLanguage=english;mayEditMissions=1;fortSpec=d/title950/{local.1}.fort;SOLOSpec=d/title950/{local.1}.fort;blankWindowName=MAIN;EditorWindowName=EDITOR:{local.1};missionWindowName=MISSION:{local.1};testBattleWindowName=TEST:{local.1};demoWindowName=DEMO:{local.1}']
    client = McpClient(command, dict(os.environ, NETSTORM_USER_DATA=str(output / 'user-data')))
    watchdog = threading.Timer(180, client.process.kill)  # 응답 중단 때도 이번 소유 프로세스만 종료한다.
    watchdog.start()
    actions, observations = [], []  # 실제 앱 입력과 같은 시점 보고/Win32 제목의 원문을 보존한다.
    result = dict(passed=False, exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(), command=command)

    def call(name, **arguments):
        """MCP 입력 오류를 정상 제목/월드 관측으로 바꾸지 않는다."""
        reply = client.call(name, **arguments)
        assert not reply.get('isError'), reply
        return reply

    def action(op, argument=''):
        """원문 앱 입력만 보내며 Player/시계/승패 상태를 설정하지 않는다."""
        actions.append(dict(op=op, arg=argument))
        call('netstorm_input', actions=[actions[-1]], wait_frames=5, screenshot=False)

    def observe(label):
        """완성된 MCP 보고와 실제 OS 창 제목을 별도로 기록한다."""
        report = call('netstorm_report')['content'][0]['text']
        report_file = f'{len(observations):03d}-{label}.tsv'  # 같은 상태를 재방문해도 처음 보고를 덮어쓰지 않는다.
        (output / report_file).write_text(report, encoding='utf-8')
        rows = {}
        for line in report.splitlines():  # 중복 버튼과 메뉴 항목도 원문 그대로 보존한다.
            fields = line.split('\t')
            rows.setdefault(fields[0], []).append(fields[1:])
        observations.append(dict(label=label, reportFile=report_file, title=window_title(client.process.pid), phase=rows.get('phase'), page=rows.get('page'),
                                 mission=rows.get('mission'), session=rows.get('raw_session'), modes=rows.get('raw_world_modes')))
        return rows

    def ready(kind):
        """실제 시작 안내만 닫고 지정한 편집/미션/메인 상태까지 기다린다."""
        for attempt in range(60):  # 승패나 공격 결과를 기다리지 않는다.
            call('netstorm_wait_frames', frames=10, screenshot=False)
            rows = observe(kind + '-' + str(attempt))
            if kind == 'main' and rows.get('phase') == [['0']] and rows.get('page') == [['main']] and 'raw_session' not in rows:
                return rows
            if kind != 'main' and rows.get('phase') == [['4']] and rows.get('paused') == [['0']] and rows.get('page') == [[kind]]:
                return rows
            labels = {row[-1] for row in rows.get('raw_html_button', []) if row}
            labels.update(row[0] for row in rows.get('control', []) if row)
            button = next((item for item in (*SAFE_BUTTONS, 'Go!', 'Main Menu') if item in labels), None)
            if button:
                action('click', button)
        raise AssertionError('현재 입력 가능한 ' + kind + ' 상태에 도달하지 못함')

    def capture(label):
        """MCP가 반환한 실제 논리 PNG를 검사 폴더에만 저장한다."""
        block = next(block for block in call('netstorm_screenshot')['content'] if block['type'] == 'image')
        (output / (label + '.png')).write_bytes(base64.b64decode(block['data']))

    try:
        client.request('initialize', {'protocolVersion': '2024-11-05', 'capabilities': {}, 'clientInfo': {'name': 'editor-title950', 'version': '1'}})
        client.notify('notifications/initialized')
        ready('main')
        action('click', 'Edit')
        observe('map-list')
        action('click', 'TEST01')
        ready('editor')
        assert observations[-1]['title'] == 'EDITOR:TEST01', observations[-1]
        capture('editor')
        action('esc');action('click', 'Game');action('click', 'Test Battle')
        ready('mission')
        capture('test-battle')
        assert observations[-1]['title'] == 'TEST:TEST01', observations[-1]
        action('esc');action('click', 'Game');action('click', 'Return to Editing')
        ready('editor')
        assert observations[-1]['title'] == 'EDITOR:TEST01', observations[-1]
        capture('returned-editor')
        action('esc');action('click', 'Game');action('click', 'Main Menu')
        rows = observe('leave-editor')
        assert rows.get('page') == [['LeaveEditMode']], rows.get('page')
        action('click', 'No')
        ready('main')
        assert observations[-1]['title'] == 'MAIN'
        capture('main')
        result['passed'] = True
    except Exception as error:
        result['error'] = repr(error)
    finally:
        if client.process.poll() is None:
            try:
                call('netstorm_quit')
                client.process.wait(timeout=20)
            except Exception:
                client.process.kill();client.process.wait(timeout=10)
        watchdog.cancel()
        result.update(exitCode=client.process.returncode, dataUnchanged=file_manifest(game)==before, actions=actions, observations=observations)
        (output / 'stderr.log').write_text('\n'.join(client.stderr_lines), encoding='utf-8')
        (output / 'summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    assert result['passed'] and result['exitCode']==0 and result['dataUnchanged'], result.get('error', result)
    print(json.dumps(dict(passed=True, observations=len(observations), exitCode=0)))


if __name__ == '__main__':
    main()
