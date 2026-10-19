#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early5 스톤 타워 건설·썬더 피격·자연 B안내와 보호 설정 복구를 확인한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_tutorial_harvest_smoke import original
from cpp_world_smoke import states

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-five-blocker-ui-input.tsv'  # 마우스 입력과 읽기 전용 관측만 담은 고정 계획이다.
REPORT = ROOT / 'cpppj/gun-blocker-runtime.json'  # 제품/독립 원본·전체 회귀·실제 자연 피격의 출처다.


def sha(path):
    """현재 파일의 전체 원문 바이트를 SHA256으로 대조한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory):
    """보호 원문 비용/최대HP와 실제 단계·동일 타워의 건설/피해·SP를 비교한다."""
    run = json.loads((directory / 'run.json').read_text(encoding='utf-8'))
    assert run['returnCode'] == 0 and run['originalsUnchanged']
    observed = states(directory / 'states.tsv')
    initial, placed, completed, goal = (observed[key] for key in ('initial', 'placed', 'completed', 'goal'))
    definition = original('sunblocker.type')  # C++ 보고서와 별개인 보호 TYPE 원문이다.
    cost = int(re.search(r'\bcost\s*=\s*(\d+)', definition)[1])
    maximum = int(re.search(r'\bmaxHitPoints\s*=\s*(\d+)', definition)[1])
    assert cost == 400 and maximum == 2000
    ids = [sid for sid, row in placed['objects'].items() if row[0:2] == ['sunBlocker', '1']]
    assert len(ids) == 1
    sid = ids[0]  # 같은 실제 건물의 HP를 생성부터 피격까지 추적한다.
    assert not any(row[0:2] == ['sunBlocker', '1'] for row in initial['objects'].values())
    assert float(initial['raw_sp'][1]) == 4000 and float(placed['raw_sp'][1]) == 4000-cost
    assert float(goal['raw_sp'][1]) == 4000-cost
    assert completed['objects'][sid][3:5] == goal['objects'][sid][3:5] == ['135', '93']
    # 고정 프레임300은 이미 자연 피격 뒤일 수 있다. 같은 건물의 실제 최대 관측 HP를 따로 기록한다.
    peak = max(int(current['objects'][sid][12]) for current in observed.values() if sid in current['objects'])
    assert int(placed['objects'][sid][12]) == 0 and 0 < peak <= maximum
    remaining = int(goal['objects'][sid][12])  # 실제 완성 HP 이후 감소를 보아 배치/안내만을 피격으로 세지 않는다.
    assert 0 < remaining < maximum
    stages = list(dict.fromkeys(int(current['mission_gate'][1]) for current in observed.values()))
    assert stages == [65, 66]
    assert all(current['mission'][0].lower() == 'tutorial5' and current['mission_gate'][0] == '5' for current in observed.values())
    assert goal['mission_gate'][:6] == ['5', '66', '0', '1', '0', '1']
    assert goal['page'] == ['B.'] and goal['paused'] == ['1'] and 'OK' in goal['controls']
    return dict(naturalHitAndStageB=True, stages=['A', 'B'], cost=cost, sp=4000-cost,
                typeMaxHp=maximum, peakObservedHp=peak, hitHp=remaining, sid=sid, point=[135, 93], gameTime=float(goal['raw_world'][0]),
                originalsUnchanged=True, goalComplete=False, naturalTutorialFiveComplete=False)


def run(directory):
    """현재 실행 파일에 실제 입력만 보내며 새 관측 폴더와 원본 설정을 보존한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    rows = []  # 그림 파일 경로만 이번 출력 폴더로 바꾸며 입력 조건은 바꾸지 않는다.
    # 허용한 마우스/관측 동작만 기존 큐에 전달한다.
    for line in PLAN.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        frame, operation, argument = line.split('\t')
        assert operation in ('click', 'right', 'report', 'snapshot')
        if operation == 'snapshot':
            argument = str(directory / (argument + '.bmp'))
        rows.append(f'{frame}\t{operation}\t{argument}\n')
    script = directory / 'steps.tsv';script.write_text(''.join(rows), encoding='utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT/'originals'), '--mission', 'tutorial5', '--window', '--no-audio', '--frames', '502',
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0', '--ui-script', str(script), '--ui-report', str(directory/'states.tsv')]
    startup = subprocess.STARTUPINFO();startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow = 0
    before = snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals', 'd'):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=80)
        (directory/'stdout.log').write_bytes(result.stdout);(directory/'stderr.log').write_bytes(result.stderr)
    record = dict(command=command, binarySha256=sha(EXE), returnCode=result.returncode, originalsUnchanged=snapshots(ROOT/'originals')==before)
    (directory/'run.json').write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    result = inspect(directory)
    (directory/'results.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))


def main():
    """새 실제 관측과 저장된 검증/출처 확인을 구분한다. --verify는 게임을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path);parser.add_argument('--inspect', type=Path);parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 소스와 과거 실제 관측을 별개 원문 바이트로 검증한다.
        for group in ('sources', 'evidence'):
            # 모든 경로의 해시를 확인하며 실패 이력도 함께 보존한다.
            for name, expected in report[group].items():
                assert sha(ROOT/name) == expected, name
        tested = json.loads((ROOT/report['validation']['fullTests']).read_text(encoding='utf-8'))
        assert tested['tests'] == report['validation']['fullCount'] and tested['failed'] == tested['returnCode'] == 0
        assert inspect(ROOT/report['validation']['naturalRun'])['naturalHitAndStageB']
        assert not report['goalComplete'] and not report['fullFidelityVerified']
        print('장애물 원본 목록/전체 회귀/실제 Early5 A→B 출처 통과')
    elif args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()), ensure_ascii=False))
    elif args.output:
        run(args.output.resolve())
    else:
        parser.error('--output, --inspect 또는 --verify가 필요합니다')


if __name__ == '__main__':
    main()
