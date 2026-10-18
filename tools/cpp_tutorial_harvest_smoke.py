#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early1의 연결·사제 채취·SP600·완료 기록과 다음 튜토리얼 진입을 검증한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-one-harvest-ui-input.tsv'  # 현재 화면에 맞춘 실제 입력 계획이다.
REPORT = ROOT / 'cpppj/tutorial-one-harvest.json'  # 현재 제품과 자연 완주 관측의 출처 기록이다.


def sha(path):
    """전체 원문 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def original(name):
    """보호 아카이브의 영어 원문을 독립적으로 읽는다."""
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    entry = next(entry for entry in archive.entries if entry.name.lower().endswith(name.lower()))
    return archive.read(entry).decode('cp1252')


def inspect(directory, code, unchanged):
    """원본 수입/다음 미션 설정과 실제 단계·이동·돈·완료를 비교한다."""
    assert code == 0 and unchanged
    observed = states(directory / 'states.tsv')
    initial, goal, following = (observed[name] for name in ('initial', 'goal', 'nextBriefing'))
    assert initial['mission_progress'] == ['0'] and float(initial['raw_sp'][1]) == 0
    assert goal['mission'][0].lower() == 'tutorial1' and goal['page'] == ['G.']
    assert goal['mission_gate'][:6] == ['1', '71', '0', '1', '0', '1'] and goal['paused'] == ['1']
    assert goal['mission_progress'] == ['1'] and {'Leave Tutorials', 'Next Tutorial'} <= set(goal['controls'])
    history = [current for current in observed.values() if current.get('mission', [''])[0].lower() == 'tutorial1']
    stages = [int(current['mission_gate'][1]) for current in history]
    assert list(dict.fromkeys(stages)) == list(range(ord('A'), ord('G') + 1))
    cost = int(re.search(r'\bcost\s*=\s*(\d+)', original('nugget.type'))[1])
    money = list(dict.fromkeys(float(current['raw_sp'][1]) for current in history))
    assert cost == 200 and money == [0, cost, cost * 2, cost * 3]
    priests = [sid for sid, row in initial['objects'].items() if row[0] == 'priest' and row[1] == '1']
    assert len(priests) == 1
    priest = priests[0]
    positions = {tuple(current['objects'][priest][5:7]) for current in history if priest in current['objects']}
    assert len(positions) > 3 and goal['raw_packets'].count('64') == 1
    assert any(current['page'] == ['E.'] for current in history)
    expected = int(re.search(r'\bmyStartMoney\s*=\s*(\d+)', original('tutorial2.english'))[1])
    assert following['mission'][0].lower() == 'tutorial2' and following['phase'] == ['3']
    assert following['mission_start'][:3] == ['1', '1', '0'] and following['raw_session'] == ['1', '1']
    assert float(following['raw_sp'][1]) == expected == 10000 and following['mission_gate'][0] == '2'
    return dict(returnCode=code, originalsUnchanged=unchanged, stages=[chr(stage) for stage in dict.fromkeys(stages)],
                money=money, doneBefore=0, doneAfter=1, priest=priest, distinctPositions=len(positions), movePackets=1,
                lastGameTime=float(goal['raw_world'][0]), nextMission='tutorial2', nextSp=expected,
                naturalTutorialOneComplete=True, goalComplete=False,
                scope='실제 A~G/합법 다리·회전/간헐천 연결/한 번의 사제 명령/세 번 채취·귀환/SP600/완료0→1/다음 튜토리얼 진입이다. Early2 진행과11단계 전체 동일성은 별도다.')


def run(directory):
    """초기 완료 기록0인 검사 프로필에서 실제 UI 입력만 수행하고 보호 설정을 복원한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    rows = []  # 입력 계획의 그림 경로만 현재 관측 폴더로 바꾼다.
    for line in PLAN.read_text('utf-8').splitlines():  # 조건은 읽기 전용이며 입력 외 동작을 허용하지 않는다.
        if not line or line.startswith('#'): continue
        frame, operation, argument = line.split('\t')
        if operation == 'snapshot': argument = str(directory / (argument + '.bmp'))
        if operation == 'when':
            condition, action, parameter = argument.split(';;', 2)
            assert condition and action in ('click', 'click-if', 'up-if', 'up', 'right')
            assert parameter.startswith(('production:', 'placement-', 'world:', 'cell:')) or parameter in ('OK', 'MORE')
        rows.append((frame, operation, argument))
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    startup = subprocess.STARTUPINFO()  # 게임 외 보조 콘솔은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow = 0
    before = snapshots(ROOT / 'originals')
    with preserve_game_settings(ROOT / 'originals', 'd'):
        result = subprocess.run([str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'tutorial1', '--window', '--no-audio', '--frames', '3924',
                                 '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=0;DoneTutorial1=0',
                                 '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')], cwd=ROOT, capture_output=True, startupinfo=startup, timeout=360)
    (directory / 'stdout.log').write_bytes(result.stdout); (directory / 'stderr.log').write_bytes(result.stderr)
    evidence = dict(returnCode=result.returncode, originalsUnchanged=snapshots(ROOT / 'originals') == before)
    (directory / 'execution.json').write_text(json.dumps(evidence, indent=2) + '\n', 'utf-8', newline='\n')
    value = inspect(directory, result.returncode, evidence['originalsUnchanged'])
    (directory / 'results.json').write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    return value


def main():
    """새 창/저장 관측 검사/현재 파일 출처 검증을 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):  # 모든 현재 파일을 확인하며 과거 관측은 덮어쓰지 않는다.
            for path, digest in report[group].items(): assert sha(ROOT / path) == digest, path
        assert report['observation']['naturalTutorialOneComplete'] and not report['goalComplete']
        print('Early1 실제 자연 완주/다음 튜토리얼 진입 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'))
        print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged']), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve()), ensure_ascii=False))


if __name__ == '__main__':
    main()
