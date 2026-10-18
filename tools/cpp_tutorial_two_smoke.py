#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early2의 건설·생산·선택·회수·자연 완료와 다음 튜토리얼 진입을 검증한다."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path
import pefile
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_tutorial_harvest_smoke import original

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-two-runtime-ui-input.tsv'  # 실제 사용자 입력과 읽기 전용 관측이다.
REPORT = ROOT / 'cpppj/tutorial-two-runtime.json'  # 현재 제품/실제 자연 완주의 출처다.


def sha(path):
    """원문 전체 바이트의 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def count(current, name):
    """실제 현재 월드의 로컬 소유 객체만 센다."""
    return sum(row[0].lower() == name and row[1] == '1' for row in current['objects'].values())


def inspect(directory, code, unchanged):
    """원본 비용/환불 상수와 실제 단계·객체·돈·회수·완료를 비교한다."""
    assert code == 0 and unchanged
    observed = states(directory / 'states.tsv')
    initial, placed, goal, following = (observed[name] for name in ('initial', 'templePlaced', 'goal', 'nextBriefing'))
    history = [row for row in observed.values() if row.get('mission', [''])[0].lower() == 'tutorial2']
    stages = list(dict.fromkeys(int(row['mission_gate'][1]) for row in history))
    assert stages == list(range(ord('A'), ord('I') + 1))
    assert initial['mission_progress'] == ['0'] and goal['mission_progress'] == ['1']
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
    assert following['mission_start'][:3] == ['1', '1', '0'] and float(following['raw_sp'][1]) == following_money == 2000
    return dict(returnCode=code, originalsUnchanged=unchanged, stages=[chr(stage) for stage in stages],
                startSp=10000, finalSp=expected, templeCount=1, workshopCount=1, maxArchers=4, finalArchers=3,
                salvagePackets=1, refund=refund, doneBefore=0, doneAfter=1, nextMission='tutorial3', nextSp=following_money,
                lastGameTime=float(goal['raw_world'][0]), naturalTutorialTwoComplete=True, goalComplete=False,
                scope='실제 A~I/사제 신전·워크샾/지식·생산/발사기4개/신전 선택/정상 회수·완료0→1/다음 Tutorial3 진입이다. Early3 진행과 전체11단계 동일성은 별도다.')


def run(directory):
    """새 검사 프로필에서 실제 UI 입력만 수행하고 모든 원본 설정을 복원한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
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
    startup = subprocess.STARTUPINFO();startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow = 0
    before = snapshots(ROOT / 'originals')
    with preserve_game_settings(ROOT / 'originals', 'd'):
        result = subprocess.run([str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'tutorial2', '--window', '--no-audio', '--frames', '4524',
                                 '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=1;DoneTutorial2=0',
                                 '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')], cwd=ROOT, capture_output=True, startupinfo=startup, timeout=420)
    (directory / 'stdout.log').write_bytes(result.stdout); (directory / 'stderr.log').write_bytes(result.stderr)
    evidence = dict(returnCode=result.returncode, originalsUnchanged=snapshots(ROOT / 'originals') == before)
    (directory / 'execution.json').write_text(json.dumps(evidence, indent=2) + '\n', 'utf-8', newline='\n')
    value = inspect(directory, result.returncode, evidence['originalsUnchanged'])
    (directory / 'results.json').write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    return value


def main():
    """새 창 실행/저장 관측 검사/현재 출처 검사를 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output', type=Path);parser.add_argument('--inspect', type=Path);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):
            for path, wanted in report[group].items(): assert sha(ROOT / path) == wanted, path
        assert report['observation']['naturalTutorialTwoComplete'] and not report['goalComplete']
        print('Early2 실제 자연 완주/다음 Tutorial3 진입 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'));print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged']), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve()), ensure_ascii=False))


if __name__ == '__main__':
    main()
