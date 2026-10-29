#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Demo 메뉴의 자연360초 만료 뒤 같은 미션 재적재와 안내/재개를 관측한다."""
import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_demo_start_smoke import DEMOS
from cpp_fort_smoke import script_value
from cpp_menu_smoke import source
from taff import TaffArchive

FIELDS = {'page', 'phase', 'paused', 'time', 'mission', 'mission_start', 'mission_gate',
          'raw_session', 'raw_uber', 'shared_surface_storage', 'shared_gameplay_globals',
          'shared_kernel', 'raw_brains', 'raw_mission_loading', 'html_window_state', 'mouse_cursor',
          'game_menu_bar', 'raw_menus'}  # 수명 판정에 필요한 실제 보고 필드다.


def observe(path):
    """큰 월드 보고서에서 관측만 추려 원시 파일의 순서/프레임을 유지한다."""
    result = []
    current = None
    with path.open(encoding='utf-8') as stream:
        for line in stream:  # 원시 월드/Player/프로세스 내용을 수정하거나 다시 만들지 않는다.
            fields = line.rstrip('\n').split('\t')
            if fields[0] == 'frame':
                current = {'frame': int(fields[1]), 'label': fields[2]}
                result.append(current)
            elif current is not None and fields[0] in FIELDS:
                current[fields[0]] = fields[1:]
            elif current is not None and fields[0] == 'raw_menu_item' and fields[-2] == '1':
                current.setdefault('visibleMenuCommands', []).append({'command': int(fields[4]), 'parameter': int(fields[5]), 'label': fields[7]})
    return result


def reference(mission):
    """원본 언어파일에서 재시작 기한과 Demo 안내 유무를 독립적으로 읽는다."""
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    data = source(ROOT / 'originals', archive, 'd/' + mission + '.english')
    text = data.decode('cp1252')
    return {'mission': mission, 'seconds': int(script_value(text, 'demoRestartTimer') or '0'),
            'demoSection': '[Demo]' in text, 'englishSha256': hashlib.sha256(data).hexdigest()}


def verify(directory, expected, record, name='result.json'):
    """기존 기한 전후의 서로 다른 실제 Start/HUD·같은 장기 저장소 대여를 대조한다."""
    observations = observe(directory / 'states.tsv')
    save(directory / ('observations-' + name), observations)
    assert record['returnCode'] == 0 and not record['timedOut'], record
    assert record['gameSha256'] == sha(GAME), '관측 뒤 게임 바이너리 변경'
    started_rows = [row for row in observations if row.get('mission_start', [])[:3] == ['1', '4', '1']]  # 실제 Start 완료/순번4/데모1인 관측만 고른다.
    expired = [row for row in started_rows if float(row['mission_start'][4]) == 0]  # 예약 직후 같은 Guide의 기한0도 삭제하지 않고 별도 보존한다.
    active = [row for row in started_rows if float(row['mission_start'][4]) != 0]  # 만료 예약 직후의 기한0은 별도 expired에 남긴다.
    assert active, '실제 Demo Start 누락'
    groups = []
    for row in active:  # 같은 첫 Start와 재시작 뒤 Start를 타이머/새 HUD로 구별한다.
        identity = (row['mission_start'][3], row['mission_start'][4], row['raw_uber'][0])
        if not groups or groups[-1]['identity'] != identity:
            groups.append({'identity': identity, 'rows': []})
        groups[-1]['rows'].append(row)
    assert len(groups) == 2, [(group['identity'], len(group['rows'])) for group in groups]
    first, second = groups
    for group in groups:  # 두 실제 세션 모두 원본 같은 기한과 장기 물리를 대여해야 한다.
        started, deadline = map(float, group['identity'][:2])
        assert abs(deadline - started - expected['seconds']) < 0.05, group['identity']
        for row in group['rows']:  # 모든 관측에서 같은 미션/실제 Session·장기 물리 대여를 확인한다.
            assert row['mission'][0].casefold() == expected['mission'].casefold(), row
            assert row['raw_session'] == ['1', '1'], row
            assert row['shared_surface_storage'][:4] == ['1'] * 4 and row['shared_surface_storage'][5] == '1', row
            assert row['shared_gameplay_globals'] == ['1'] * 5, row
            assert row['shared_kernel'][0] == '1' and row['mouse_cursor'][1] == '1', row
    playing = [[row for row in group['rows'] if row.get('phase') == ['4'] and row.get('paused') == ['0']] for group in groups]  # 각 세션에서 실제 비정지 전투 관측만 추린다.
    assert all(playing), '첫 실행 또는 재시작 뒤 자연 안내/재개 누락'
    guides = [[row for row in group['rows'] if row.get('page') == ['Demo'] and row.get('paused') == ['1'] and row.get('html_window_state') == ['1', '1', '1']] for group in groups]  # 각 세션의 실제 모달 Demo 안내를 별도로 보존한다.
    if expected['demoSection']:
        assert all(guides), '원본 Demo 섹션의 첫 실행/재시작 안내 누락'
    else:
        assert not any(guides), '원본에 없는 Demo 안내 생성'
    before, after = playing[0][-1], playing[1][0]
    first_deadline = float(first['identity'][1])
    assert float(before['time'][0]) < first_deadline, ('기한 뒤 이전 세션 계속 진행', before)
    restart_delay = float(second['identity'][0]) - first_deadline
    assert 0 <= restart_delay < 2.0, ('실제 새 Start의 기한 이전/과도한 지연', restart_delay)
    assert first['identity'][2] != second['identity'][2], '이전 HUD를 같은 재적재로 오인'
    assert int(second['rows'][0]['shared_surface_storage'][4]) == int(first['rows'][0]['shared_surface_storage'][4]) + 1, '월드 재적재 세대 불일치'
    first_wall = float(playing[0][0]['time'][1])
    after_wall = float(second['rows'][0]['time'][1])
    assert 340 < after_wall - first_wall < 390, ('자연 벽시계360초 범위', first_wall, after_wall)
    final = observations[-1]
    assert final.get('phase') == ['0'] and final.get('page') == ['main'] and final.get('html_window_state') == ['0', '0', '0'], final
    assert 'raw_session' not in final and final.get('raw_menus', [])[:3] == ['0', '0', '0'], final
    assert float(playing[1][-1]['time'][0]) - float(second['identity'][0]) > 5, '재시작 뒤 진행 부족'
    # 관측별 모든 표시 항목에서 원문 명령/인자/라벨이 함께 맞는 전용 이탈을 요구한다.
    assert any(item == {'command': 0x0200008c, 'parameter': 1, 'label': 'Exit Demo'}
               for row in observations for item in row.get('visibleMenuCommands', [])), '원문 데모 전용 이탈 명령 누락'
    record.update(passed=True, observations=len(observations), expiredObservations=expired, restartDelaySeconds=restart_delay, reference=expected,
                  firstStart=first['identity'], repeatedStart=second['identity'], guideObservations=[len(rows) for rows in guides],
                  lastBeforeDeadline=before, firstRepeatedPlaying=after,
                  firstRepeatedObservation=second['rows'][0], final=final)
    save(directory / name, record)
    return record


def run(data, directory, mission, label):
    """플레이 명령 없이 메인 Demo 선택 두 번과 자연 프레임만 진행한다."""
    directory.mkdir(parents=True, exist_ok=False)
    expected = reference(mission)
    assert expected['seconds'] == 360, expected
    fps, frames = 15, 15 * 900  # 프레임 수로 벽시각을 예측하지 않으며 실제 재시작/이탈/Quit이 먼저 실행을 끝낸다.
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'report', 'menu'),
             (12, 'click', 'Demo'), (16, 'report', 'choices'), (18, 'click', label), (24, 'report', 'started')]
    samples = set(range(30, frames - 1, fps * 5)) | set(range(fps * 300, frames - 1, fps))
    for frame in sorted(samples):  # 후반은15프레임 간격이다. 실제 초 간격은 종료 후 time 필드에서 읽으며 상태를 쓰지 않는다.
        steps.append((frame, 'report', str(frame)))
    for frame in range(fps * 300, frames - 1, fps):  # 같은 두 번째 월드와 충분한 게임 시각 뒤 실제 메뉴 입력만 사용한다.
        main = r'page\tmain\n&&html_window_state\t0\t0\t0\n'
        leave = r'raw_menu_item[^\n]*\t1\tExit Demo\n'
        bar = r'game_menu_bar\t[0-9]+\t1\t'
        ready = r'phase\t4\n&&shared_surface_storage\t1\t1\t1\t1\t2\t1\n&&time\t(?:[4-9][0-9]{2}|[1-9][0-9]{3,})(?:\.[0-9]+)?\t'
        steps.extend([(frame, 'when', main + ';;click;;Quit'),
                      (frame, 'when', leave + ';;click;;Exit Demo'),
                      (frame, 'when', bar + '&&!'+ leave + ';;click;;Game'),
                      (frame, 'when', ready + '&&game_menu_bar\t[0-9]+\t0\t;;esc;;')])
    for frame in (fps * 25, fps * 330, fps * 360, fps * 390, fps * 420, fps * 450):  # 파일명만 예상 프레임이며 실제 장면은 보고서로 확인한다.
        steps.append((frame, 'snapshot', str(directory / f'sample-{frame}.bmp')))
    steps.extend([(frames - 1, 'report', 'final'), (frames - 1, 'snapshot', str(directory / 'final.bmp'))])
    steps.sort(key=lambda item: item[0])
    plan = directory / 'steps.tsv'
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')  # 정렬된 각 입력/관측을 원래 TSV 폭으로 저장한다.
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', str(frames),
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=15;sleepPerLoop=1;autoDemo=0;tellTips=0',
               '--ui-script', str(plan), '--ui-report', str(directory / 'states.tsv')]
    started = time.monotonic()
    record = {'command': command, 'gameSha256': sha(GAME), 'passed': False}
    launch_tool = directory / 'tool-at-launch.py'  # 실행 뒤 검사 도구가 수정되어도 실제 적재한 버전을 구별한다.
    launch_tool.write_bytes(Path(__file__).read_bytes())
    record['toolSha256'] = sha(launch_tool)
    with (directory / 'stdout.log').open('wb') as stdout, (directory / 'stderr.log').open('wb') as stderr:
        process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        save(directory / 'running.json', dict(pid=process.pid, command=command, gameSha256=record['gameSha256']))
        print(json.dumps({'mission': mission, 'pid': process.pid, 'naturalSeconds': expected['seconds']}), flush=True)
        try:
            record['returnCode'] = process.wait(timeout=1200)
            record['timedOut'] = False
        except subprocess.TimeoutExpired:
            process.terminate()
            record.update(returnCode=process.wait(timeout=20), timedOut=True)
    record['elapsedSeconds'] = time.monotonic() - started
    save(directory / 'result.json', record)
    try:
        return verify(directory, expected, record)
    except Exception as error:
        record['error'] = str(error)
        save(directory / 'result.json', record)
        raise


def main():
    """원본 실행 경로를 거부하고 선택한 한 데모 또는 기존 원시만 처리한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mission', choices=[mission for mission, _ in DEMOS], required=True)  # 원문 메뉴 세 키만 순서대로 허용한다.
    parser.add_argument('--analyze', type=Path)
    args = parser.parse_args()
    if args.analyze:
        directory = args.analyze.resolve()
        assert not args.output.exists(), '기존 재판정 결과 덮어쓰기 거부'
        assert args.output.resolve().parent == directory, '재판정은 같은 원시 폴더에 새 파일로 보존'
        assert not (directory / ('observations-' + args.output.name)).exists(), '기존 재판정 관측 덮어쓰기 거부'
        record = json.loads((directory / 'result.json').read_text(encoding='utf-8'))
        verify(directory, reference(args.mission), record, args.output.name)
    else:
        assert args.data_root, '실행 사본 경로 필요'
        data = args.data_root.resolve()
        assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents for name in ('originals', 'originalCD'))  # 두 보호 루트와 모든 하위를 실행 사본으로 거부한다.
        run(data, args.output.resolve(), args.mission, dict(DEMOS)[args.mission])


if __name__ == '__main__':
    main()
