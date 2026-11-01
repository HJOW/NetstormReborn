#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-2 초기 장면의 그림자 출력만 짧게 캡처한다. 조작/공격/완주는 수행하지 않는다(678)."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """원본 자료/현재 프레임을 유지한 실제 GUI 진입과 출력 파일을 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    steps = []
    for frame in range(12, 197, 8):  # 현재 표시된 브리핑만 닫고게임월드에는명령을보내지않는다.
        for label in ('MORE', 'More', 'OK', 'Ok', 'ok', 'Play Mission'):
            condition = r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'
            steps.append((frame, 'when', condition+';;click;;'+label))
    steps.extend(((240, 'report', 'final'), (241, 'snapshot-presented', str(out/'final.bmp'))))
    plan, report = out/'steps.tsv', out/'states.tsv'
    plan.write_text(''.join(f'{frame}\t{operation}\t{value}\n' for frame, operation, value in steps), encoding='utf-8')
    before = protection()
    (out/'protection-before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
    command = [str(args.exe.resolve()), '--run', str(ROOT/'originals'), '--mission', 'MasterOfWhirligigs', '--window',
               '--no-audio', '--frames', '243', '--set', 'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;edgeScroll=0;scrollWhileMouseOut=0',
               '--ui-script', str(plan), '--ui-report', str(report)]
    with preserve_game_settings(ROOT/'originals', 'd', out/'settings-backup'):
        run = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=90)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after = protection()
    (out/'protection-after.json').write_text(json.dumps(after, ensure_ascii=False), encoding='utf-8')
    assert before == after and run.returncode == 0
    final = states(report)['final']
    assert final['phase'] == ['4'] and final['raw_session'] == ['1', '1'] and final['paused'] == ['0'], final.get('phase')
    summary = {'exit': run.returncode, 'protectedUnchanged': True, 'exeSha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
               'phase': final['phase'], 'time': final['time'], 'camera': final['camera'], 'image': str(out/'final.bmp'),
               'limits': '실제1-2초기GUI·앱내입력. 원본나란히픽셀/자연완주/물리입력/PCM/공격/포획/희생/승리검사아님.'}
    (out/'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__':
    main()
