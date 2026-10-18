#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 1-1 건설 뒤 캐논 우클릭 회수의 환불/삭제/붕괴 출력을 검사한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

PLAN = ROOT / 'cpppj/tests/fixtures/campaign-forward-ui-input.tsv'  # 검증한 초기 실제 건설 입력이다.
REPORT = ROOT / 'cpppj/campaign-salvage-command.json'  # 현재 제품/원본/실제 창 근거의 기록이다.


def sha(path):
    """전체 관측 바이트를 그대로 해시한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory, code, unchanged):
    """기존 실제 보고서를 읽고 같은 캐논 부재와 정확한 환불을 확인한다."""
    observed = states(directory / 'states.tsv')
    assert code == 0 and unchanged
    before, after, settled = (observed[name] for name in ('beforeSalvage', 'afterSalvage', 'settled'))
    cannons = [sid for sid, raw in before['raw_objects'].items() if raw[:2] == ['128', '1']]
    assert len(cannons) == 1
    sid = cannons[0]
    assert sid not in after['raw_objects'] and sid not in settled['raw_objects']
    money = float(before['raw_sp'][1]); paid = float(after['raw_sp'][1])
    assert paid == money + 100
    return dict(returnCode=code, originalsUnchanged=unchanged, cannon=sid, spBefore=money, spAfter=paid,
                lastGameTime=float(settled['raw_world'][0]), goalComplete=False,
                scope='실제 건설/우클릭 메뉴/54h 회수 환불·삭제와 후속 붕괴 표시다. 자연 사제 포획/미션 승리는 별도다.')


def run(directory):
    """보호 설정을 복원하며 새 창에서 사용자 메뉴 클릭만 재생한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True); before = snapshots(ROOT / 'originals'); rows = []
    for line in PLAN.read_text('utf-8').splitlines():  # 결과값 없이 같은 최초 건설 입력만 재생한다.
        if not line or line.startswith('#'): continue
        frame, operation, argument = line.split('\t')
        if int(frame) <= 2640: rows.append((int(frame), operation, argument))
    rows.extend(((2800, 'report', 'beforeSalvage'), (2805, 'right', 'world:sunCannon'),
                 (2806, 'report', 'salvageMenu'), (2807, 'snapshot', str(directory / 'menu.bmp')),
                 (2810, 'click', 'Salvage gains 100'), (2812, 'report', 'afterSalvage'),
                 (2813, 'snapshot', str(directory / 'collapse.bmp')), (3400, 'report', 'settled'),
                 (3400, 'snapshot', str(directory / 'settled.bmp'))))
    rows.sort(key=lambda row: row[0]); script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), 'utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', 'thewarbegins', '--window', '--no-audio', '--frames', '3403',
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0;useProductTimers=0;cursorSnapToGrid=1',
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 게임 이외의 보조 콘솔은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow = 0
    with preserve_game_settings(ROOT / 'originals', 'd'):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=360)
    (directory / 'stdout.log').write_bytes(result.stdout); (directory / 'stderr.log').write_bytes(result.stderr)
    evidence = dict(returnCode=result.returncode, originalsUnchanged=snapshots(ROOT / 'originals') == before)
    (directory / 'execution.json').write_text(json.dumps(evidence, indent=2) + '\n', 'utf-8', newline='\n')
    value = inspect(directory, result.returncode, evidence['originalsUnchanged'])
    (directory / 'results.json').write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    return value


def main():
    """새 실제 창·기존 관측 검사·현재 출처 감사를 명시적으로 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__); mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--output', type=Path); mode.add_argument('--inspect', type=Path); mode.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for group in ('sources', 'evidence'):  # 현재 제품/도구와 관측의 원문 핀만 감사한다.
            for path, digest in report[group].items(): assert sha(ROOT / path) == digest, path
        directory = ROOT / report['observationDirectory']; execution = json.loads((directory / 'execution.json').read_text('utf-8'))
        value = inspect(directory, execution['returnCode'], execution['originalsUnchanged'])
        assert value == report['observation'] and not report['goalComplete']
    elif args.inspect:
        execution = json.loads((args.inspect / 'execution.json').read_text('utf-8')); value = inspect(args.inspect, execution['returnCode'], execution['originalsUnchanged'])
    else: value = run(args.output.resolve())
    print(json.dumps(value, ensure_ascii=False))


if __name__ == '__main__':
    main()
