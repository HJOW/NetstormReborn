#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""모달 뒤 생산 입력 차단·동일 패널 수명·자연 SP600 표시를 실제 Early1 완주로 검증한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_tutorial_harvest_smoke import inspect as inspect_harvest

PLAN = ROOT / 'cpppj/tests/fixtures/modal-production-ui-input.tsv'  # 현재 화면에 맞춘 실제 입력 계획이다.
REPORT = ROOT / 'cpppj/modal-production.json'  # 현재 제품과 자연 완주 관측의 출처 기록이다.


def sha(path):
    """전체 원문 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory, code, unchanged):
    """자연 완주 관측에 실제 패널 식별자/차단된 입력/완료 예산을 더해 확인한다."""
    value = inspect_harvest(directory, code, unchanged)
    observed = states(directory / 'states.tsv')
    initial, blocked, goal = (observed[name] for name in ('initial', 'blocked', 'goal'))
    assert blocked['page'] == ['A.'] and blocked['paused'] == ['1']
    assert blocked['mission_bridges'] == ['0'] and blocked['raw_input'][:2] == ['0', '0']
    assert blocked['raw_cursor'][1:3] == ['0', '0'] and blocked['raw_packets'] == initial['raw_packets']
    history = [current for current in observed.values() if current.get('mission', [''])[0].lower() == 'tutorial1']
    panels = [current['raw_production_gump'] for current in history]
    assert len({tuple(panel[:2]) for panel in panels}) == 1
    assert all(panel[2:4] == ['80', '688'] for panel in panels)
    assert int(goal['raw_production_gump'][4]) == float(goal['raw_sp'][1]) == 600
    value.update(sameProductionPanel=True, modalInputBlocked=True, finalDisplayedBudget=600,
                 productionIdentity=panels[0][:2], panelObservations=len(panels))
    return value


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
        assert report['observation']['naturalTutorialOneComplete'] and report['observation']['sameProductionPanel'] and report['observation']['modalInputBlocked'] and not report['goalComplete']
        print('모달 생산 패널·입력 차단·실제 SP600 표시 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'))
        print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged']), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve()), ensure_ascii=False))


if __name__ == '__main__':
    main()
