#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""1-1의 합법 UI 건설/연구/배치 뒤 실제 덱 반환의 flags100 해제 시각을 관측한다(단위677)."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

ROOT = Path(__file__).resolve().parents[1]


def protection():
    """게임 실행 전후 원본 두 폴더·기존 fixture·AGENTS의 이름/내용을 비교한다."""
    result = {name: snapshots(ROOT/name) for name in ('originals', 'originalCD', 'cpppj/tests/fixtures')}
    result['AGENTS.md'] = hashlib.sha256((ROOT/'AGENTS.md').read_bytes()).hexdigest()
    return result


def observations(path):
    """실제 보고의 시간·전투 옵션·반환 내용물을 프레임별로 읽는다."""
    result, current = [], None
    for line in path.read_text('utf-8').splitlines():
        fields = line.split('\t')
        if fields[0] == 'frame':
            current = {'frame': int(fields[1]), 'label': fields[2], 'contents': []}
            result.append(current)
        elif current is not None:
            if fields[0] == 'time':
                current['time'] = [float(value) for value in fields[1:]]
            elif fields[0] == 'mission_options':
                current['options'] = fields[1:]
            elif fields[0] == 'raw_production_content' and fields[2] == '128' and int(fields[3]) & 8:
                current['contents'].append([int(value) for value in fields[1:]])
    return result


def main():
    """명시 실행 파일의 배치 직후와 복귀 경계를 기록하고 기대 간격을 판정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expected-seconds', type=float, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    steps = [(7, 'right', 'world:priest'), (35, 'click', 'Construct'),
             (65, 'click', 'Workshop'), (95, 'click', ' Build Sun Workshop for 800'),
             (102, 'up', 'cell:107,103'), (110, 'click', 'cell:107,103'),
             (1060, 'right', 'world:sunFactory'), (1070, 'click', 'Put Knowledge into Production'),
             (1080, 'click', 'Sun Cannon'), (1100, 'report', 'researched'),
             (1110, 'click', 'production:128'), (1130, 'up', 'cell:94,109'),
             (1149, 'report', 'beforeDrop'), (1150, 'click', 'cell:94,109'), (1150, 'report', 'dropped')]
    # 약0.17초 간격으로 실제 게임 시각과 같은 반환 SID의 사용 중 비트를 관측한다.
    for frame in range(1160, 1861, 10):
        steps.append((frame, 'report', f'wait{frame}'))
    steps.append((1861, 'snapshot', str(out/'after.bmp')))
    steps = [(frame+200, operation, value) for frame, operation, value in steps]
    # 비동기 Guide/로그인이 실제 브리핑을 연 뒤에만 현재 표시 버튼을 누른다.
    for frame in range(12, 197, 8):
        for label in ('MORE', 'More', 'OK', 'Ok', 'ok', 'Play Mission'):
            condition = r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'
            steps.append((frame, 'when', condition+';;click;;'+label))
    steps.sort(key=lambda step: step[0])
    plan, report = out/'steps.tsv', out/'states.tsv'
    plan.write_text(''.join(f'{frame}\t{operation}\t{value}\n' for frame, operation, value in steps), encoding='utf-8')
    before = protection()
    (out/'protection-before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
    command = [str(args.exe.resolve()), '--run', str(ROOT/'originals'), '--mission', 'thewarbegins',
               '--window', '--no-audio', '--frames', '2063', '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0',
               '--ui-script', str(plan), '--ui-report', str(report)]
    with preserve_game_settings(ROOT/'originals', 'd', out/'settings-backup'):
        run = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=180)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after = protection()
    (out/'protection-after.json').write_text(json.dumps(after, ensure_ascii=False), encoding='utf-8')
    assert before == after, '보호 원본/fixture/AGENTS 변경'
    assert run.returncode == 0, run.stderr.decode('utf-8', errors='replace')[-1500:]
    rows = observations(report)
    dropped = next(row for row in rows if row['label'] == 'dropped')
    assert len(dropped['contents']) == 1 and dropped['contents'][0][2] & 0x100, dropped
    sid = dropped['contents'][0][0]
    ready = next(row for row in rows if row['frame'] > dropped['frame'] and any(c[0] == sid and not c[2] & 0x100 for c in row['contents']))
    last_busy = next(row for row in reversed(rows) if row['frame'] < ready['frame'] and any(c[0] == sid and c[2] & 0x100 for c in row['contents']))
    lower = last_busy['time'][0]-dropped['time'][0]
    upper = ready['time'][0]-dropped['time'][0]
    summary = {'exit': run.returncode, 'protectedUnchanged': True, 'exeSha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
               'returnedSid': sid, 'dropped': dropped, 'lastBusy': last_busy, 'firstReady': ready,
               'observedSecondsInterval': [lower, upper], 'expectedSeconds': args.expected_seconds,
               'limits': '앱 내 명시 UI 입력·한 종류의 반환 시각이다. 물리 입력·공격·포획·희생·승리·전체34 검사는 아니다.'}
    (out/'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False))
    assert lower < args.expected_seconds + 0.2 and upper >= args.expected_seconds - 0.2, summary


if __name__ == '__main__':
    main()
