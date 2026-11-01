#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전투 없는 임시 HTML의 PlaySound를 실제 큐/음향 이름표까지 확인한다(682)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """명시 검사 본문을 사용하며 원본 시나리오·PCM·물리 입력 완료로 합산하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect-missing', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    probe = out/'probe.english'
    probe.write_text('[Probe]\n<h2>Script sound probe</h2>\n<$PlaySound,somethingCreated.wav>\nOriginal script command to current sound list.\n$Button=OK,DoNothing,0\n', encoding='utf-8', newline='\n')
    plan, report = out/'steps.tsv', out/'states.tsv'
    steps = ((3, 'click-if', 'OK'), (5, 'click-if', 'Main Menu'), (30, 'report', 'before'),
             (40, 'kernel-tell', 'Probe'), (60, 'report', 'final'))
    plan.write_text(''.join(f'{frame}\t{operation}\t{value}\n' for frame, operation, value in steps), encoding='utf-8', newline='\n')
    before = protection()
    (out/'protection-before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
    command = [str(args.exe.resolve()), '--run', str(ROOT/'originals'), '--window', '--no-audio', '--frames', '63',
               '--set', f'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;missionSpec={probe.as_posix()}',
               '--ui-script', str(plan), '--ui-report', str(report)]
    with preserve_game_settings(ROOT/'originals', 'd', out/'settings-backup'):
        run = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=90)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after = protection()
    (out/'protection-after.json').write_text(json.dumps(after, ensure_ascii=False), encoding='utf-8')
    assert before == after and run.returncode == 0, run.returncode
    observed = states(report)
    final = observed['final']
    assert final['phase'] == ['1'] and int(final['current_dialog'][0]) != 0
    missing = 'PlaySound' in run.stderr.decode('utf-8', errors='replace')
    assert missing == args.expect_missing, missing
    counts = None
    if not args.expect_missing:
        counts = [int(observed[label]['sound_first_free'][0]) for label in ('before', 'final')]
        assert counts[1] > counts[0], counts
    result = {'exit': run.returncode, 'expectedMissing': args.expect_missing, 'missingCommandDiagnostic': missing,
              'soundFirstFreeBeforeAfter': counts, 'phase': final['phase'], 'protectedUnchanged': True,
              'exeSha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
              'limits': '명시 임시 HTML/missionSpec/kernel-tell로 실제 GUI 큐와 같은 음향 이름표까지만 확인한다. 음향 장치/PCM/청취·원본 시나리오·전투·포획/희생·공격·승리·공중3종·MP 실행이 아니다.'}
    (out/'summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
