#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Imp4.@3의 명령 태그를 임시 본문에 넣어 전투 없이 실제 HTML/모달 경로를 확인한다(680)."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """커널 검사 입력으로 원본 안내만 열고 실제 창·명령 로그·보호 자료를 보관한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT/'cpppj/build/bin/Release/Netstorm.exe')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    # 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 원문 태그를 실제 파서/큐로 보내는 작은 본문이다.
    probe = out/'probe.english'
    probe.write_text('[Probe]\n<h2>MoveDialog probe</h2>\n<$MoveDialog,tm>\nActual HTML command and current modal.\n$Button=OK,DoNothing,0\n', encoding='utf-8')
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'Main Menu'), (40, 'kernel-tell', 'Probe'),
             (50, 'report', 'opened'), (80, 'report', 'final'), (81, 'snapshot-presented', str(out/'final.bmp'))]
    plan, report = out/'steps.tsv', out/'states.tsv'
    plan.write_text(''.join(f'{frame}\t{operation}\t{value}\n' for frame, operation, value in steps), encoding='utf-8')
    before = protection()
    (out/'protection-before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
    command = [str(args.exe.resolve()), '--run', str(ROOT/'originals'), '--window', '--no-audio', '--frames', '83',
               '--set', f'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;missionSpec={probe.as_posix()}',
               '--ui-script', str(plan), '--ui-report', str(report)]
    with preserve_game_settings(ROOT/'originals', 'd', out/'settings-backup'):
        run = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=90)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after = protection()
    (out/'protection-after.json').write_text(json.dumps(after, ensure_ascii=False), encoding='utf-8')
    assert before == after and run.returncode == 0
    final = states(report)['final']
    text = report.read_text('utf-8')
    assert final['phase'] == ['1'] and int(final['current_dialog'][0]) != 0, final.get('phase')
    assert 'Actual HTML command and current modal.' in text
    assert 'MoveDialog' not in run.stderr.decode('utf-8', errors='replace')
    summary = {'exit': run.returncode, 'protectedUnchanged': True, 'exeSha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
               'phase': final['phase'], 'dialog': final['current_dialog'], 'image': str(out/'final.bmp'),
               'probeSha256': hashlib.sha256(probe.read_bytes()).hexdigest(),
               'limits': '명시 임시 본문/missionSpec/kernel-tell 검사 입력으로 실제 GUI를 연다. 원본 시나리오 진입·전투/AI/공격/포획/희생/승리/물리 입력/PCM 검사는 아니다. 위치는 캡처로 관측한다.'}
    (out/'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__':
    main()
