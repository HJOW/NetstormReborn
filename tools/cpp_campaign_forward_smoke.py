#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 1-1 창의 전진 건설과 자연 신전 파괴를 검사한다. 사제 포획/승리는 별도다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

PLAN = ROOT / 'cpppj/tests/fixtures/campaign-forward-ui-input.tsv'  # 결과를 담지 않는 수동 UI 입력이다.
REPORT = ROOT / 'cpppj/campaign-forward-window.json'  # 고정한 기존 창 관측과 현재 파일의 출처다.


def sha(path):
    """검사 대상의 전체 바이트 해시를 반환한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory, code, unchanged):
    """저장된 실제 보고서에서 전진 캐논과 적 신전의 자연 부재를 확인한다."""
    observed = states(directory / 'states.tsv')
    assert code == 0 and unchanged
    prepared = {name: row['raw_cursor'] for name, row in observed.items() if name.startswith('prepare')}
    assert len([name for name in prepared if name.startswith('prepareBridge')]) == 12
    # 공급/전체 배치 판정이 실제로 허용한 모든 건설 입력을 확인한다.
    for name, cursor in prepared.items():
        assert int(cursor[1]) and all(int(value) for value in cursor[8:11]), (name, cursor)
    assert prepared['prepareForwardCannon'][2] == '128'
    assert prepared['prepareForwardCannon'][4:6] == ['129', '126']
    initial = observed['placeCannon']['raw_objects']
    assert initial[15017][:2] == ['123', '2'] and initial[15016][:2] == ['158', '2']
    enemies = {sid: int(raw[0]) for sid, raw in initial.items() if raw[1] == '2'}
    first = {}
    # HP/삭제 결과를 쓰지 않고 관찰된 최초 부재 시각만 기록한다.
    for name, row in observed.items():
        if not name.startswith('combat'):
            continue
        for sid, kind in enemies.items():  # 원래 적 객체만 추적해 새 객체/재사용 SID와 혼합하지 않는다.
            if sid not in row['raw_objects']:
                first.setdefault(str(sid), dict(type=kind, label=name, game=row['raw_world'][0]))
    final = observed['combat20000']
    assert '15017' in first and 15017 not in final['raw_objects']
    assert final['raw_objects'][15016][:2] == ['158', '2']
    assert any(raw[:2] == ['128', '1'] and raw[3:5] == ['129', '126'] for raw in final['raw_objects'].values())
    return dict(returnCode=code, originalsUnchanged=unchanged, maximumGameTime=float(final['raw_world'][0]),
                prepares=prepared, firstEnemyAbsence=first, priestStillAlive=True, goalComplete=False,
                scope='실제 창의 건설/자연 전투/신전 부재다. 자연 포획/희생/미션 승리 증거가 아니다.')


def run(directory):
    """검증한 수동 입력을 새 클론 창에서 재생하고 원본 설정을 복원한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    before = snapshots(ROOT / 'originals')
    rows = []
    # 주석을 제외한 명시 입력만 읽고 절대 경로인 스크린샷 목적지는 새 출력 경로에서 생성한다.
    for line in PLAN.read_text('utf-8').splitlines():
        if line and not line.startswith('#'):
            frame, operation, argument = line.split('\t')
            rows.append((int(frame), operation, argument))
    for frame in range(2100, 20001, 100):  # 같은 최대 프레임까지 자연 전투의 부재를 주기적으로 관찰한다.
        rows.append((frame, 'report', f'combat{frame}'))
    for frame in (4300, 5120, 6300, 7030, 10000, 12000, 14000, 16000, 18000, 20000):  # 실제 렌더러의 화면이다.
        rows.append((frame, 'snapshot', str(directory / f'frame{frame}.bmp')))
    rows.sort(key=lambda row: row[0])
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'thewarbegins', '--window', '--no-audio',
               '--frames', '20003', '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=0;cursorSnapToGrid=1',
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 게임 창 외의 보조 콘솔은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    with preserve_game_settings(ROOT / 'originals', 'd'):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=1300)
    (directory / 'stdout.log').write_bytes(result.stdout)
    (directory / 'stderr.log').write_bytes(result.stderr)
    value = inspect(directory, result.returncode, snapshots(ROOT / 'originals') == before)
    (directory / 'results.json').write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    return value


def main():
    """창 재생·기존 관측 검사·현재 출처 감사 중 명시한 한 작업만 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--output', type=Path, help='새 창을 재생할 새 출력 디렉터리')
    mode.add_argument('--inspect', type=Path, help='이미 저장한 실제 창 결과를 읽기 전용 검사')
    mode.add_argument('--verify', action='store_true', help='고정한 현재 파일과 실제 창 근거의 해시 감사')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):  # 현재 파일과 관측의 모든 해시를 확인하며 과거 값을 고치지 않는다.
            for path, digest in report[group].items():
                assert sha(ROOT / path) == digest, path
        directory = ROOT / report['observationDirectory']
        result = json.loads((directory / 'results.json').read_text('utf-8'))
        value = inspect(directory, result['returnCode'], result['originalsUnchanged'])
        assert value == report['observation'] and not report['goalComplete']
    elif args.inspect:
        result = json.loads((args.inspect / 'results.json').read_text('utf-8'))
        value = inspect(args.inspect, result['returnCode'], result['originalsUnchanged'])
    else:
        value = run(args.output.resolve())
    print(json.dumps(value, ensure_ascii=False))


if __name__ == '__main__':
    main()
