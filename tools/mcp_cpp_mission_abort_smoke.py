#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""격리 저장 폴더와 실제 cpppj 창에서 Early1 중단/계속/재시작/메인 복귀를 확인한다."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import threading

from mcp_cpp_smoke import McpClient

SAFE_BUTTONS = ('MORE', 'More', 'OK', 'Ok', 'ok', 'Play Mission')  # 시작 안내만 닫고 승패/다음 미션 버튼은 선택하지 않는다.


def file_manifest(directory):
    """실행 자료의 경로/내용 지문을 읽어 게임 실행이 자료를 쓰지 않았는지 비교한다."""
    return {path.relative_to(directory).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(directory.rglob('*')) if path.is_file()}


def main():
    """한 프로세스의 실제 메뉴 입력과 단계/보고서/화면만 관측하며 상태를 주입하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--user-data', type=Path, help='같은 격리 프로필을 재사용해 캐시/설정 재시작을 확인할 때만 지정한다')
    parser.add_argument('--check-saved-state-delete', action='store_true', help='중단 뒤 사용자 저장 상태 삭제와 설치 파일 보존을 검사한다')
    args = parser.parse_args()
    output = args.output.resolve()
    game = args.game.resolve()
    if args.check_saved_state_delete:
        root = Path(__file__).resolve().parents[1]
        for name in ('originals', 'originalCD'):  # 추가 검사 자료를 보호 원본 아래에 만들거나 지우지 않는다.
            protected = (root / name).resolve()
            assert all(path != protected and protected not in path.parents for path in (game, output)), '격리 실행 자료와 출력 폴더 필요'
    output.mkdir(parents=True, exist_ok=False)
    (output / 'tool-at-launch.py').write_bytes(Path(__file__).read_bytes())
    legacy = game / 'd' / '!state.dat' if args.check_saved_state_delete else None
    if legacy is not None and not legacy.exists():
        legacy.write_bytes(b'949-install-saved-state-must-survive')  # 검사 사본에 없는 파일만 새로 만들고 기존 파일은 읽기만 한다.
    legacy_sha = hashlib.sha256(legacy.read_bytes()).hexdigest() if legacy is not None else None
    before = file_manifest(game)
    user_data = (args.user_data or output / 'user-data').resolve()  # 기본은 새 격리 루트이며 명시 재시작 검사만 같은 기존 검사 루트를 빌린다.
    env = dict(os.environ, NETSTORM_USER_DATA=str(user_data))
    command = [str(args.exe.resolve()), '--run', str(game), '--mcp', '--window', '--no-audio', '--set', 'currentLanguage=english']  # 파일 확장자 설정은 UI 언어 번호가 아닌 원문 언어 이름이다.
    client = McpClient(command, env)
    watchdog = threading.Timer(180, client.process.kill)  # 응답이 멈춰도 이번에 연 게임 프로세스만 제한 시간 뒤 종료한다.
    watchdog.start()
    observations = []  # 실제 보고의 단계/월드 존재 여부를 순서대로 보관한다.
    actions = []  # 실행한 앱 내부 메뉴 입력이며 물리 입력으로 세지 않는다.
    saved_checks = []  # 실제 중단 명령 전후의 사용자 파일과 설치 파일을 구별한다.
    user_state = user_data / 'Saves' / '!state.dat'  # 실제 Client의 사용자 정책과 같은 검사 전용 위치다.

    def call(name, **arguments):
        """MCP 오류 응답을 정상 게임 단계로 오인하지 않는다."""
        reply = client.call(name, **arguments)
        if reply.get('isError'):
            raise RuntimeError(str(reply))
        return reply

    def observe(label):
        """같은 프레임 보고서의 전체 원문과 필드별 목록을 보관한다."""
        report = call('netstorm_report')['content'][0]['text']
        (output / (label + '.tsv')).write_text(report, encoding='utf-8')
        rows = {}
        for line in report.splitlines():  # 같은 이름의 버튼/컨트롤도 모두 남긴다.
            fields = line.split('\t')
            rows.setdefault(fields[0], []).append(fields[1:])
        observations.append(dict(label=label, phase=rows.get('phase'), page=rows.get('page'), mission=rows.get('mission'), session=rows.get('raw_session'), rendererFrame=rows.get('raw_renderer_frame', [[]])[0][:1]))
        return rows

    def action(op, argument=''):
        """검사 명령이나 좌표 주입 대신 기존 MCP의 앱 입력 경로만 사용한다."""
        actions.append(dict(op=op, arg=argument))
        call('netstorm_input', actions=[dict(op=op, arg=argument)], wait_frames=5, screenshot=False)

    def ready(label):
        """현재 표시된 시작 안내만 닫고 실제 월드의 입력 가능 단계까지 기다린다."""
        for attempt in range(40):  # 최대 약400프레임이며 승패나 생산/공격을 기다리지 않는다.
            call('netstorm_wait_frames', frames=10, screenshot=False)
            rows = observe(label + '-' + str(attempt))
            if rows.get('phase') == [['4']] and 'raw_session' in rows and rows.get('paused') == [['0']]:
                return rows
            labels = {row[-1] for row in rows.get('raw_html_button', []) if row}
            labels.update(row[0] for row in rows.get('control', []) if row)
            selected = next((item for item in SAFE_BUTTONS if item in labels), None)
            if selected:
                action('click', selected)
        raise RuntimeError('시작 안내 뒤 미션 입력 단계에 도달하지 못함')

    def screenshot(label):
        """서버가 반환한 실제 논리 PNG를 검사 산출물 폴더에만 저장한다."""
        result = call('netstorm_screenshot')
        block = next(block for block in result['content'] if block['type'] == 'image')
        (output / (label + '.png')).write_bytes(base64.b64decode(block['data']))

    def abort(label):
        """현재 게임 메뉴의 실제 Leave Mission 버튼으로 ABORT 창을 연다."""
        action('esc')
        action('click', 'Game')
        action('click', 'Leave Mission')
        rows = observe(label)
        assert rows.get('page') == [['ABORT']] and 'raw_session' in rows, observations[-1]

    def main_menu(label):
        """첫 사용자 Welcome/Tips 안내를 실제 버튼으로 닫고 월드 없는 메인 메뉴를 확인한다."""
        for attempt in range(8):  # 새 프로필 안내 외의 임의 창을 성공 상태로 취급하지 않는다.
            rows = observe(label + '-' + str(attempt))
            if rows.get('page') == [['main']] and 'mission' not in rows and 'raw_session' not in rows:
                return rows
            if rows.get('page') in ([['Welcome']], [['Tips']]) and 'mission' not in rows and 'raw_session' not in rows:
                assert any(row and row[-1] == 'OK' for row in rows.get('raw_html_button', []))
                action('click', 'OK')
            elif rows.get('page') == [['Welcome2']] and 'mission' not in rows and 'raw_session' not in rows:
                assert any(row and row[-1] == 'Main Menu' for row in rows.get('raw_html_button', []))
                action('click', 'Main Menu')
            else:
                call('netstorm_wait_frames', frames=10, screenshot=False)
        raise RuntimeError('월드 없는 실제 메인 메뉴에 도달하지 못함')

    try:
        client.request('initialize', {'protocolVersion': '2024-11-05', 'capabilities': {}, 'clientInfo': {'name': 'mission-abort945', 'version': '1'}})
        client.notify('notifications/initialized')
        call('netstorm_wait_frames', frames=20, screenshot=False)
        main_menu('startup')
        for cycle in range(2):  # 같은 프로세스에서 두 번 새 진입하고 각각 실제 Replay를 거친다.
            action('script-command', 'MissionBegin,tutorial1')
            ready('entry-' + str(cycle))
            screenshot('mission-' + str(cycle))
            abort('abort-continue-' + str(cycle))
            action('click', 'Continue Mission')
            continued = observe('continued-' + str(cycle))
            assert continued.get('phase') == [['4']] and 'raw_session' in continued
            abort('abort-replay-' + str(cycle))
            action('click', 'Replay Mission')
            ready('replayed-' + str(cycle))
            abort('abort-main-' + str(cycle))
            if args.check_saved_state_delete:
                assert not user_state.exists(), '검사 전에 실제 사용자 저장 상태가 이미 있음'
                user_state.parent.mkdir(parents=True, exist_ok=True)
                user_state.write_bytes(('949-user-saved-state-cycle-' + str(cycle)).encode('utf-8'))  # 초기 Startup reader 뒤에만 삭제 검사용 파일을 준비한다.
            action('click', 'Main Menu')
            if args.check_saved_state_delete:
                check = dict(cycle=cycle, userRemoved=not user_state.exists(), installExists=legacy.exists(),
                             installSha256=hashlib.sha256(legacy.read_bytes()).hexdigest() if legacy.exists() else None)
                saved_checks.append(check)
                assert check['userRemoved'] and check['installSha256'] == legacy_sha, check
            call('netstorm_wait_frames', frames=20, screenshot=False)
            main_menu('main-' + str(cycle))
        screenshot('final-main')
        call('netstorm_quit')
        code = client.process.wait(timeout=30)
        assert code == 0, code
        assert file_manifest(game) == before, '실행 자료가 변경됨'
        result = dict(passed=True, unit=949 if args.check_saved_state_delete else 945, processes=1, exitCode=code, cycles=2, entries=4, actions=actions, observations=observations,
                      savedStateChecks=saved_checks,
                      dataUnchanged=True, exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(), userData=str(user_data))
        (output / 'summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(dict(passed=True, cycles=2, entries=4, exitCode=code, observations=len(observations)), ensure_ascii=False))
    finally:
        watchdog.cancel()
        if client.process.poll() is None:
            client.process.kill()
            client.process.wait(timeout=10)
        (output / 'stderr.log').write_text('\n'.join(client.stderr_lines), encoding='utf-8')
        (output / 'actions.json').write_text(json.dumps(actions, ensure_ascii=False, indent=2), encoding='utf-8')
        if args.check_saved_state_delete:
            audit = dict(checks=saved_checks, userStateExists=user_state.exists(), installStateExists=legacy.exists(),
                         expectedInstallSha256=legacy_sha, installSha256=hashlib.sha256(legacy.read_bytes()).hexdigest() if legacy.exists() else None,
                         dataUnchanged=file_manifest(game)==before, exitCode=client.process.returncode)
            (output / 'saved-state-audit.json').write_text(json.dumps(audit, ensure_ascii=False, indent=2), encoding='utf-8')  # 실패/종료 예외에서도 실제 파일 상태를 남긴다.


if __name__ == '__main__':
    main()
