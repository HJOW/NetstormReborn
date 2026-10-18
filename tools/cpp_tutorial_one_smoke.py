#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early1 창의 A~D 안내·정지 재개·홈/다리 건설·간헐천 생성을 검사한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

PLAN = ROOT / 'cpppj/tests/fixtures/tutorial-one-runtime-ui-input.tsv'  # 결과를 넣지 않는 실제 UI 입력이다.
REPORT = ROOT / 'cpppj/tutorial-one-runtime.json'  # 현재 제품과 실제 창 관측의 출처 기록이다.


def sha(path):
    """원문 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory, code, unchanged):
    """같은 실제 상태에서 안내 순서·자기 정지·누적 다리·생성을 검증한다."""
    observed = states(directory / 'states.tsv')
    assert code == 0 and unchanged
    initial, closing, play = (observed[name] for name in ('initial', 'closing', 'play'))
    assert initial['mission_gate'][:6] == ['1', '65', '6', '0', '0', '1']
    assert closing['phase'] == ['4'] and closing['paused'] == ['1'] and closing['mission_gate'][3] == '1'
    assert play['phase'] == ['4'] and play['paused'] == ['0'] and play['mission_gate'][:6] == ['1', '65', '0', '0', '0', '1']
    assert observed['A1']['page'] == ['A1.'] and observed['scheduled']['mission_gate'][1:4] == ['66', '10', '0']
    records = []  # 각 안내에서 직접 관측한 현재 단계와 살아 있는 객체 수다.
    for name in ('A', 'B', 'C', 'D'):  # 같은 화면의 실제 단계/정지/자금을 한 번씩 검증한다.
        current = observed[name]
        assert current['page'] == [name + '.'] and current['phase'] == ['3'] and current['paused'] == ['1']
        assert current['mission_gate'][:6] == ['1', str(ord(name)), '0', '1', '0', '1']
        assert float(current['raw_sp'][1]) == 0 and current['mission_progress'] == initial['mission_progress']
        raw = current['raw_objects']  # 서버가 실제 배치한 타입82 다리 칸과122 간헐천만 센다.
        records.append(dict(stage=name, built=int(current['mission_bridges'][0]), bridgeCells=sum(row[0] == '82' for row in raw.values()),
                            geysers=sum(row[0] == '122' for row in raw.values())))
    assert [record['built'] for record in records[:2]] == [0, 0]
    assert records[2]['built'] >= 8 and records[3]['built'] >= 19 #원본 누적 판정과 동일하며 붕괴한 다리도 누적 집계에는 남는다.
    assert [record['geysers'] for record in records] == [0, 0, 0, 3]
    return dict(returnCode=code, originalsUnchanged=unchanged, stages=records,
                lastGameTime=float(observed['D']['raw_world'][0]), initialDoneFlag=int(initial['mission_progress'][0]), goalComplete=False, naturalHarvestVerified=False,
                scope='실제 Early1의 A~D 안내/자기 정지/홈·다리 건설/간헐천 생성이다. 연결·수확·E~G/완주는 별도다.')


def run(directory):
    """보호 설정을 복원하면서 새 클론 창에 같은 사용자 입력만 재생한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    rows = []  # 고정 수동 계획의 경로만 현재 관측 폴더로 바꾼다.
    for line in PLAN.read_text('utf-8').splitlines():  # 단계/게임 값은 전달하지 않는다.
        if not line or line.startswith('#'): continue
        frame, operation, argument = line.split('\t')
        if operation == 'snapshot': argument = str(directory / (argument + '.bmp'))
        rows.append((frame, operation, argument))
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'tutorial1', '--window', '--no-audio', '--frames', '704',
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=0',
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 게임 이외의 보조 콘솔은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow = 0
    before = snapshots(ROOT / 'originals')
    with preserve_game_settings(ROOT / 'originals', 'd'):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=150)
    (directory / 'stdout.log').write_bytes(result.stdout); (directory / 'stderr.log').write_bytes(result.stderr)
    evidence = dict(returnCode=result.returncode, originalsUnchanged=snapshots(ROOT / 'originals') == before)
    (directory / 'execution.json').write_text(json.dumps(evidence, indent=2) + '\n', 'utf-8', newline='\n')
    value = inspect(directory, result.returncode, evidence['originalsUnchanged'])
    (directory / 'results.json').write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    return value


def main():
    """새 창 재생/기존 관측 검사/현재 파일 출처 감사를 구별해 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):  # 과거 관측을 현재 자료로 덮어쓰지 않고 각 바이트를 검증한다.
            for path, digest in report[group].items(): assert sha(ROOT / path) == digest, path
        print('Early1 실제 A~D 프레임/입력/건설·생성 출처 통과')
    elif args.inspect:
        evidence = json.loads((args.inspect / 'execution.json').read_text('utf-8'))
        print(json.dumps(inspect(args.inspect, evidence['returnCode'], evidence['originalsUnchanged']), ensure_ascii=False))
    else:
        if not args.output: parser.error('새 관측 폴더 --output이 필요합니다')
        print(json.dumps(run(args.output.resolve()), ensure_ascii=False))


if __name__ == '__main__':
    main()
