#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 MCP 입력과 자연 대기로 데모의 두 번째360초 만료·세 번째 월드·메인 반납을 확인한다."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import threading
import time

from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_demo_repeat_smoke import observe, reference
from cpp_demo_start_smoke import DEMOS
from mcp_cpp_smoke import McpClient


def manifest(directory):
    """격리 실행 자료의 경로/내용을 읽어 전후 같은 입력을 사용했는지 확인한다."""
    return {path.relative_to(directory).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(directory.rglob('*')) if path.is_file()}


def verify(directory, expected, record, name='result.json'):
    """원문360초와 실제 세 번의 Start·월드 세대·안내·메인 반납을 독립 기준으로 비교한다."""
    rows = observe(directory / 'states.tsv')
    save(directory / ('observations-' + name), rows)
    assert record['returnCode'] == 0 and not record['timedOut'], record
    assert record['dataUnchanged'], '실행 자료 변경'
    assert record['gameSha256'] == sha(GAME), '실행 중 제품 변경'
    active = [row for row in rows if row.get('mission_start', [])[:3] == ['1', '4', '1'] and float(row['mission_start'][4]) != 0]
    groups = []  # 같은 Start/기한/HUD 식별자만 한 세대로 묶는다.
    for row in active:  # 중간 기한0의 만료 예약은 이전 세대 정상 진행으로 합치지 않는다.
        identity = tuple(row['mission_start'][3:5]) + (row['raw_uber'][0],)
        if not groups or groups[-1]['identity'] != identity:
            groups.append(dict(identity=identity, rows=[]))
        groups[-1]['rows'].append(row)
    assert len(groups) == 3, [(group['identity'], len(group['rows'])) for group in groups]
    playing = []  # 세 번째 월드까지 비정지 실제 진행을 별도로 요구한다.
    guides = []  # 원문 Demo 섹션이 있는 미션만 모달 안내를 요구한다.
    for group in groups:  # 기한을 제품이 만든 다른 값에 맞춰 허용하지 않는다.
        started, deadline = map(float, group['identity'][:2])
        assert abs(deadline - started - expected['seconds']) < 0.05, group['identity']
        for row in group['rows']:  # 같은 장기 owner를 빌린 현재 월드/입력을 끝까지 확인한다.
            assert row['mission'][0].casefold() == expected['mission'].casefold(), row
            assert row['raw_session'] == ['1', '1'], row
            assert row['shared_surface_storage'][:4] == ['1'] * 4 and row['shared_surface_storage'][5] == '1', row
            assert row['shared_gameplay_globals'] == ['1'] * 5 and row['shared_kernel'][0] == '1', row
            # 전면 창은 사용자가 바꿀 수 있다. 실제 활성값은 보존하되 자연 대기의 필수 전제와 섞지 않는다.
            assert row['mouse_cursor'][1] in ('0', '1'), '창 활성 보고 형식 불일치'
        playing.append([row for row in group['rows'] if row.get('phase') == ['4'] and row.get('paused') == ['0']])
        guides.append([row for row in group['rows'] if row.get('page') == ['Demo'] and row.get('paused') == ['1'] and row.get('html_window_state') == ['1', '1', '1']])
    assert all(playing), '세대별 실제 비정지 진행 누락'
    assert all(guides) if expected['demoSection'] else not any(guides), '원문 Demo 안내 유무 불일치'
    transitions = []  # 원문 Reset은 누적 정지만0으로 쓰므로 서로 다른 게임 시계 기준을 직접 빼지 않는다.
    for index in range(2):  # 첫360초와 재시작 뒤 두번째360초에 같은 독립 계약을 적용한다.
        prior, following = groups[index:index + 2]
        deadline = float(prior['identity'][1])
        last = playing[index][-1]
        next_row = following['rows'][0]
        last_game, last_wall = map(float, last['time'])
        next_wall = float(next_row['time'][1])
        pause_offset = last_wall - last_game
        deadline_wall = deadline + pause_offset
        assert 0 <= deadline - last_game < 5, ('만료 직전 관측 누락/기한 뒤 이전 세대', index, last)
        assert 0 <= next_wall - deadline_wall < 5, ('자연 만료 벽시각/재적재 지연', index, next_row, deadline_wall)
        # Start가 Reset 전의 고정 프레임을 읽거나 다음 Capture 뒤 시각을 읽는 양쪽을 원문 범위로 제한한다.
        delay = float(following['identity'][0]) - deadline
        assert 0 <= delay <= max(0, pause_offset) + 5, ('Start 시계 범위', index, delay, pause_offset)
        assert prior['identity'][2] != following['identity'][2], '이전 HUD 재사용'
        assert int(next_row['shared_surface_storage'][4]) == int(prior['rows'][0]['shared_surface_storage'][4]) + 1, '월드 세대 증가 누락'
        seconds = next_wall - float(playing[index][0]['time'][1])
        assert 340 < seconds < 390, ('자연 벽시각360초 범위', index, seconds)
        transitions.append(dict(startDifference=delay, previousPauseOffset=pause_offset, deadlineWall=deadline_wall,
                                observedRestartWall=next_wall, wallDelay=next_wall-deadline_wall, naturalSeconds=seconds))
    assert float(playing[2][-1]['time'][0]) - float(groups[2]['identity'][0]) > 10, '세 번째 월드 게임 진행 부족'
    assert float(playing[2][-1]['time'][1]) - float(playing[2][0]['time'][1]) > 10, '세 번째 월드 실제 진행 부족'
    initial = rows[0]  # Native 메인 자체의 DialogCount를 누수로 오인하지 않도록 진입 전 값을 기준으로 삼는다.
    assert initial.get('phase') == ['0'] and initial.get('page') == ['main'], initial
    assert initial['html_window_state'][0] == '0' and initial['html_window_state'][2] == '0', initial
    final = rows[-1]
    assert final.get('phase') == ['0'] and final.get('page') == ['main'] and final.get('html_window_state') == initial['html_window_state'], final
    assert 'raw_session' not in final and final.get('raw_menus', [])[:3] == ['0', '0', '0'], final
    assert any(item == dict(command=0x0200008c, parameter=1, label='Exit Demo') for row in rows for item in row.get('visibleMenuCommands', [])), '실제 Exit Demo 메뉴 누락'
    record.update(passed=True, observations=len(rows), reference=expected, starts=[group['identity'] for group in groups],
                  transitions=transitions, guideObservations=[len(group) for group in guides], initial=initial, final=final,
                  windowActivity=[dict(active=sum(row['mouse_cursor'][1]=='1' for row in group['rows']),
                                       inactive=sum(row['mouse_cursor'][1]=='0' for row in group['rows'])) for group in groups])
    save(directory / name, record)
    return record


def run(data, directory, mission):
    """실제 Demo 선택 뒤 대기만 하며 게임 시계/기한/AI/승패 값을 변경하지 않는다."""
    directory.mkdir(parents=True, exist_ok=False)
    expected = reference(mission)
    assert expected['seconds'] == 360, expected
    before = manifest(data)
    command = [str(GAME), '--run', str(data), '--mcp', '--window', '--no-audio', '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=15;sleepPerLoop=1;autoDemo=0;tellTips=0;intro=1;currentLanguage=english']
    env = dict(os.environ, NETSTORM_USER_DATA=str(directory / 'user-data'))  # 기존 Saved Games와 분리한 검사 전용 저장 경로다.
    record = dict(mission=mission, command=command, gameSha256=sha(GAME), passed=False, timedOut=False)
    (directory / 'tool-at-launch.py').write_bytes(Path(__file__).read_bytes())
    record['toolSha256'] = sha(directory / 'tool-at-launch.py')
    client = McpClient(command, env)
    watchdog = threading.Timer(1200, client.process.kill)  # 같은 이번 소유 프로세스가 응답하지 않을 경우만 종료한다.
    watchdog.start()
    started = time.monotonic()
    actions = []  # 실제 앱 입력만 저장하며 검사 전용 상태 명령은 사용하지 않는다.
    observations = []  # 완성된 MCP 보고의 순서를 보존한다.
    save(directory / 'running.json', dict(pid=client.process.pid, command=command, gameSha256=record['gameSha256']))
    print(json.dumps(dict(mission=mission, pid=client.process.pid, naturalExpirations=2)), flush=True)

    def call(name, **arguments):
        """MCP 오류 응답은 그대로 실패시키며 정상 단계로 바꾸지 않는다."""
        result = client.call(name, **arguments)
        assert not result.get('isError'), result
        return result

    def capture(label):
        """실제 논리 PNG만 출력 폴더에 저장한다."""
        block = next(block for block in call('netstorm_screenshot')['content'] if block['type'] == 'image')
        (directory / (label + '.png')).write_bytes(base64.b64decode(block['data']))

    def report(label):
        """완성된 한 응답을 append하며 게임이 재작성하는 파일은 실행 중 열지 않는다."""
        text = call('netstorm_report')['content'][0]['text']
        rows = {}
        for line in text.splitlines():  # 중복 필드는 실제 표시 메뉴 판정을 위해 전부 유지한다.
            fields = line.split('\t')
            rows.setdefault(fields[0], []).append(fields[1:])
        frame = int(rows['raw_renderer_frame'][0][0])
        with (directory / 'states.tsv').open('a', encoding='utf-8', newline='\n') as stream:
            stream.write(f'frame\t{frame}\t{label}\n' + text + '\n')
        observations.append(dict(label=label, frame=frame, phase=rows.get('phase'), page=rows.get('page'),
                                 time=rows.get('time'), start=rows.get('mission_start'), storage=rows.get('shared_surface_storage')))
        return rows

    def action(operation, argument=''):
        """기존 MCP가 지원하는 앱 입력만 같은 실제 분배기에 전달한다."""
        actions.append(dict(op=operation, arg=argument))
        call('netstorm_input', actions=[actions[-1]], wait_frames=5, screenshot=False)

    try:
        client.request('initialize', {'protocolVersion': '2024-11-05', 'capabilities': {}, 'clientInfo': {'name': 'demo-second-cycle946', 'version': '2'}})
        client.notify('notifications/initialized')
        call('netstorm_wait_frames', frames=20, screenshot=False)
        assert report('main')['page'] == [['main']]
        action('click', 'Demo')
        report('choices')
        action('click', dict(DEMOS)[mission])
        known = []  # 실제 새 Start의 세대만 세고 임의 목표 시각으로 세 번째 월드를 추정하지 않는다.
        first_playing_wall = {}  # 안내 종료 뒤 세 번째 월드가 실제10초 이상 진행했는지 확인한다.
        last_progress = 0.0
        while True:  # 실제 두 번 만료될 때까지 같은 살아 있는 프로세스에서 자연 프레임만 기다린다.
            call('netstorm_wait_frames', frames=30, screenshot=False)
            rows = report('sample-' + str(len(observations)))
            gate = rows.get('mission_start', [[]])[0]
            if gate[:3] == ['1', '4', '1'] and float(gate[4]) != 0:
                identity = tuple(gate[3:5]) + (rows['raw_uber'][0][0],)
                if identity not in known:
                    known.append(identity)
                    capture('generation-' + str(len(known)))
                    print(json.dumps(dict(generation=len(known), gate=gate, time=rows['time'][0])), flush=True)
                assert len(known) <= 3, '정리 전 네 번째 월드 진입'
                if rows.get('phase') == [['4']] and rows.get('paused') == [['0']]:
                    game_seconds, wall_seconds = map(float, rows['time'][0])
                    first_playing_wall.setdefault(identity, wall_seconds)
                    if len(known) == 3 and game_seconds - float(identity[0]) > 12 and wall_seconds - first_playing_wall[identity] > 12:
                        capture('third-world')
                        break
            elapsed = time.monotonic() - started
            assert elapsed < 1150, '자연 두 만료/세 번째 진행 미완료'
            if elapsed - last_progress > 45:
                print(json.dumps(dict(mission=mission, elapsed=round(elapsed, 1), state=observations[-1])), flush=True)
                last_progress = elapsed
        action('esc')
        action('click', 'Game')
        report('exit-menu')
        action('click', 'Exit Demo')
        call('netstorm_wait_frames', frames=20, screenshot=False)
        final = report('final-main')
        assert final.get('phase') == [['0']] and final.get('page') == [['main']] and 'raw_session' not in final, final.get('page')
        capture('main')
        call('netstorm_quit')
        record['returnCode'] = client.process.wait(timeout=30)
    except Exception as error:
        record['error'] = repr(error)
        raise
    finally:
        watchdog.cancel()
        if client.process.poll() is None:
            client.process.kill()
            client.process.wait(timeout=10)
        record.setdefault('returnCode', client.process.returncode)
        record.update(elapsedSeconds=time.monotonic()-started, dataUnchanged=manifest(data)==before,
                      actions=actions, checkpoints=observations)
        (directory / 'stderr.log').write_text('\n'.join(client.stderr_lines), encoding='utf-8')
        save(directory / 'result.json', record)
    result = verify(directory, expected, record)
    print(json.dumps(dict(mission=mission, passed=result['passed'], seconds=record['elapsedSeconds'], starts=record['starts'])), flush=True)


def main():
    """보호 루트를 거부하고 지정한 한 데모 또는 종료된 원시 관측만 처리한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mission', choices=[item[0] for item in DEMOS], required=True)
    parser.add_argument('--analyze', type=Path)
    args = parser.parse_args()
    if args.analyze:
        directory = args.analyze.resolve()
        assert args.output.resolve().parent == directory and not args.output.exists(), '재판정은 같은 원시 폴더의 새 파일만 허용한다'
        assert not (directory / ('observations-' + args.output.name)).exists(), '최초 재판정 관측 보존'
        record = json.loads((directory / 'result.json').read_text(encoding='utf-8'))
        record.update(analysisToolSha256=sha(Path(__file__)), originalResultSha256=sha(directory / 'result.json'))
        verify(directory, reference(args.mission), record, args.output.name)
        return
    assert args.data_root, '격리 실행 사본 필요'
    data = args.data_root.resolve()
    assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents for name in ('originals', 'originalCD'))
    run(data, args.output.resolve(), args.mission)


if __name__ == '__main__':
    main()
