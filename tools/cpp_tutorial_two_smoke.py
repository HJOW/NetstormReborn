#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early2의 건설·생산·선택·회수·자연 완료와 다음 튜토리얼 진입을 검증한다."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
import pefile
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_tutorial_harvest_smoke import original
from cpp_bridge_ritual_runtime import execute, save

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-two-runtime-ui-input.tsv'  # 실제 사용자 입력과 읽기 전용 관측이다.
REPORT = ROOT / 'cpppj/tutorial-two-runtime.json'  # 2026-10-19의 과거 제품·0 시작 관측 출처이며 새 실행 결과로 덮어쓰지 않는다.


def sha(path):
    """원문 전체 바이트의 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def count(current, name):
    """실제 현재 월드의 로컬 소유 객체만 센다."""
    return sum(row[0].lower() == name and row[1] == '1' for row in current['objects'].values())


def inspect(directory, code, unchanged, *, initial_progress_injected=True, legacy_guide_sequence=False):
    """원본 비용/환불 상수와 실제 단계·객체·돈·회수·완료를 비교한다."""
    assert code == 0 and unchanged
    observed = states(directory / 'states.tsv')
    initial, placed, goal, following = (observed[name] for name in ('initial', 'templePlaced', 'goal', 'nextBriefing'))
    history = [row for row in observed.values() if row.get('mission', [''])[0].lower() == 'tutorial2']
    stages = list(dict.fromkeys(int(row['mission_gate'][1]) for row in history))
    assert stages == list(range(ord('A'), ord('I') + 1))
    done_before = int(initial['mission_progress'][0])  # 현재 실제 초기 완료값을 읽으며 기존1을0으로 바꾸지 않는다.
    assert done_before in (0, 1) and goal['mission_progress'] == ['1']
    if initial_progress_injected:
        assert done_before == 0  # 과거 명시 초기 프로필의0 시작 증거를 별도로 유지한다.
    assert goal['page'] == ['I.'] and goal['paused'] == ['1'] and {'Leave Tutorials', 'Next Tutorial'} <= set(goal['controls'])
    assert all(row['mission_gate'][0] == '2' and row['mission_gate'][5] == '1' for row in history)
    assert all(row['mission_options'] == ['2', '0', '196'] for row in history if row is not initial)
    costs = {name:int(re.search(r'\bcost\s*=\s*(\d+)', original(name + '.type'))[1]) for name in ('windvortex', 'sunfactory', 'sunarcher')}
    image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
    scale = struct.unpack_from('<f', image, 0x501550 - 0x400000)[0]
    assert costs == dict(windvortex=5000, sunfactory=800, sunarcher=300) and scale == .25
    assert float(initial['raw_sp'][1]) == 10000 and float(placed['raw_sp'][1]) == 10000 - costs['windvortex']
    assert count(initial, 'windvortex') == count(initial, 'sunfactory') == count(initial, 'sunarcher') == 0
    assert max(count(row, 'windvortex') for row in history) == count(goal, 'windvortex') == 1
    assert max(count(row, 'sunfactory') for row in history) == count(goal, 'sunfactory') == 1
    assert max(count(row, 'sunarcher') for row in history) == 4 and count(goal, 'sunarcher') == 3
    before = [row for row in history if '84' not in row['raw_packets']][-1]
    after = next(row for row in history if '84' in row['raw_packets'])
    refund = int(costs['sunarcher'] * scale)
    assert goal['raw_packets'].count('84') == 1 and float(after['raw_sp'][1]) - float(before['raw_sp'][1]) == refund == 75
    assert 'Salvage gains 75\n' in (directory / 'states.tsv').read_text('utf-8')
    assert before['mission_rules'][0] == goal['mission_rules'][0] == '0'
    expected = 10000 - costs['windvortex'] - costs['sunfactory'] - 4 * costs['sunarcher'] + refund
    assert float(goal['raw_sp'][1]) == expected == 3075
    following_money = int(re.search(r'\bmyStartMoney\s*=\s*(\d+)', original('tutorial3.english'))[1])
    assert following['mission'][0].lower() == 'tutorial3' and following['phase'] == ['3']
    next_sequence = '1' if legacy_guide_sequence else '4'  # 과거 어댑터1과 실제 Init→Start3→Frame4를 별도 계약으로 검사한다.
    assert following['mission_start'][:3] == ['1', next_sequence, '0'] and float(following['raw_sp'][1]) == following_money == 2000
    if not legacy_guide_sequence:
        assert following['page'] == ['A.'] and following['paused'] == ['1']  # 실제 Tutorial3 첫 안내가 열린 상태여야 한다.
        assert following['mission_gate'][:2] == ['3', str(ord('A'))] and following['raw_session'] == ['1', '1']
    return dict(returnCode=code, originalsUnchanged=unchanged, stages=[chr(stage) for stage in stages],
                startSp=10000, finalSp=expected, templeCount=1, workshopCount=1, maxArchers=4, finalArchers=3,
                salvagePackets=1, refund=refund, doneBefore=done_before, doneAfter=1, nextMission='tutorial3', nextSp=following_money,
                initialProgressInjected=initial_progress_injected, noStateInjection=not initial_progress_injected,
                nextGuideSequence=int(next_sequence), inspectionContract='legacy-20261019' if legacy_guide_sequence else 'native-guide-0-6',
                lastGameTime=float(goal['raw_world'][0]), naturalTutorialTwoComplete=True, goalComplete=False,
                scope='실제 A~I/사제 신전·워크샾/지식·생산/발사기4개/신전 선택/정상 회수·완료값 관측/다음 Tutorial3 진입이다. 영구 진행 저장·Early3 진행·전체 동일성은 별도다.')


def prepare(directory, reset_progress_profile=False):
    """그림 경로만 옮기고 기존 완료값을 보존하는 실제 입력 명령을 먼저 기록한다."""
    rows = []  # 그림 경로만 현재 관측 폴더로 바꾸고 입력/조건은 그대로 사용한다.
    for line in PLAN.read_text('utf-8').splitlines():
        if not line or line.startswith('#'): continue
        frame, operation, argument = line.split('\t')
        assert operation in ('report', 'snapshot', 'click', 'right', 'when')
        if operation == 'snapshot': argument = str(directory / (argument + '.bmp'))
        if operation == 'when':
            condition, action, parameter = argument.split(';;', 2)
            assert condition and action in ('click', 'click-if', 'up-if', 'right', 'esc')
            assert parameter.startswith(('world:', 'placement-', 'production:')) or parameter in ('MORE', 'OK', 'Construct', 'Workshop', ' Build Sun Workshop for 800', 'Put Knowledge into Production', 'Sun Disc Thrower', 'Salvage gains 75', '0')
        rows.append((frame, operation, argument))
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    settings = 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=1'  # 기본 실행은 완료값·SP·단계·HP를 주입하지 않는다.
    if reset_progress_profile:
        settings += ';DoneTutorial2=0'  # 명시 초기 프로필 요청만 과거0 시작 계약을 선택한다.
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'tutorial2', '--window', '--no-audio', '--frames', '4524',
               '--set', settings, '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    profile = dict(command=command, initialProgressInjected=reset_progress_profile, productTimers=1,
                   testStateInjection=False, noStateInjection=not reset_progress_profile,
                   planSha256=sha(PLAN), replaySha256=sha(script), inspectionContract='native-guide-0-6')  # 실제 입력 출처와 검사 전용 쓰기 부재를 실행 전에 고정한다.
    save(directory / 'prepared.json', profile)
    return profile


def run(directory, *, reset_progress_profile=False, timeout=600, prepare_only=False):
    """기본 진행값을 보존하고 종료·상한·예외에도 원시 로그와 원본 복구 결과를 남긴다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    profile = prepare(directory, reset_progress_profile)  # GUI 시작 전에 실행할 명령을 검토 가능하게 저장한다.
    if prepare_only:
        return dict(preparedOnly=True, **profile)
    before = snapshots(ROOT / 'originals')  # 허용 설정까지 포함한 전체 원본 바이트다.
    save(directory / 'originals-before.json', before)
    evidence = dict(profile=profile, completed=False, passed=False)  # 실행이나 판정이 중단되면 성공으로 남지 않는다.
    save(directory / 'execution.json', evidence)
    try:
        with preserve_game_settings(ROOT / 'originals', 'd', directory / 'settings-backup'):
            evidence.update(execute(profile['command'], directory, EXE, timeout))
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        after = snapshots(ROOT / 'originals')  # 설정 복구 뒤 최종 상태를 별도 저장한다.
        save(directory / 'originals-after.json', after)
        evidence['originalsUnchanged'] = before == after
        save(directory / 'execution.json', evidence)
    try:
        assert not evidence['timedOut'], 'Early2 자연 플레이 실행 상한 도달'
        value = inspect(directory, evidence['returnCode'], evidence['originalsUnchanged'], initial_progress_injected=reset_progress_profile)
    except Exception as error:
        evidence['inspectionError'] = repr(error)
        save(directory / 'execution.json', evidence)
        raise
    value.update(binarySha256=evidence['binarySha256'], productTimers=1, testStateInjection=False)
    evidence.update(completed=True, passed=True)
    save(directory / 'execution.json', evidence)
    save(directory / 'results.json', value)
    return value


def main():
    """새 창 실행·저장 관측 재판정·과거 보고서의 현재 작업본 SHA 검사를 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true', help='과거 보고서의 핀과 현재 작업본 SHA를 비교')
    parser.add_argument('--prepare-only', action='store_true', help='게임 없이 실제 입력 계획과 실행 명령만 저장')
    parser.add_argument('--fresh-progress-profile', action='store_true', help='명시 요청한 초기 프로필만 DoneTutorial2=0으로 시작')
    parser.add_argument('--timeout', type=float, default=600, help='실제 실행 상한 초')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):
            for path, wanted in report[group].items(): assert sha(ROOT / path) == wanted, path
        assert report['observation']['naturalTutorialTwoComplete'] and not report['goalComplete']
        print('Early2 실제 자연 완주/다음 Tutorial3 진입 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'))
        injected = evidence.get('profile', {}).get('initialProgressInjected', True)  # 과거 실행은 실제0 주입 계약으로 분류한다.
        legacy = 'profile' not in evidence  # 최초2026-10-19 관측만 옛 Guide1 계약으로 재판정한다.
        print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged'],
                                 initial_progress_injected=injected, legacy_guide_sequence=legacy), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve(), reset_progress_profile=args.fresh_progress_profile,
                             timeout=args.timeout, prepare_only=args.prepare_only), ensure_ascii=False))


if __name__ == '__main__':
    main()
