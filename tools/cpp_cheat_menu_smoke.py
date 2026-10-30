#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원래 userCheat 설정의 ESC 메뉴와 실제 입력 SP 가산만 격리 사본에서 확인한다."""
import argparse
import json
import re
from pathlib import Path
from cpp_bridge_ritual_runtime import execute, save, sha
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from cpp_solo_entry_smoke import BUTTONS


def main():
    """SP/의식/승리를 직접 주입하지 않고 실제 메뉴 클릭만 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True); parser.add_argument('--game-dir', type=Path, required=True); parser.add_argument('--binary', type=Path, required=True); parser.add_argument('--enabled', action='store_true'); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False); data = args.game_dir.resolve(); binary = args.binary.resolve()
    if data.name.lower() in ('originals', 'originalcd'): raise ValueError('보호 원본 대신 새 자료 사본을 지정해야 합니다')
    plan = []
    for frame in range(3, 57, 5):  # 같은 초기 안내 버튼으로만 정상 미션 진입을 완료한다.
        for label in BUTTONS: # 기존 원문 미션 진입 도구와 같은 실제 안내 라벨을 사용한다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            plan.append((frame, 'when', condition + ';;click;;' + label))
    plan.extend([(60, 'report', 'ready'), (70, 'esc', ''), (75, 'report', 'bar'), (75, 'snapshot', str(output / 'bar.bmp'))])
    if args.enabled:
        plan.extend([(80, 'click', 'Cheat'), (85, 'report', 'child'), (85, 'snapshot', str(output / 'child.bmp')), (90, 'click', 'Get 10,000'), (105, 'report', 'after-one'),
                     (115, 'esc', ''), (120, 'click', 'Cheat'), (130, 'click', 'Get 10,000'), (145, 'report', 'after-two')])
    else: plan.extend([(80, 'click-if', 'Cheat'), (85, 'report', 'no-cheat'), (90, 'esc', '')])
    plan.extend([(180, 'report', 'final'), (180, 'snapshot', str(output / 'final.bmp')), (180, 'snapshot-output', str(output / 'final-output.bmp'))])
    script = output / 'steps.tsv'; script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in plan), encoding='utf-8', newline='\n')
    settings = 'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;intro=1' + (';userCheat=8675309' if args.enabled else '')
    command = [str(binary), '--run', str(data), '--mission', 'Tutorial1', '--window', '--no-audio', '--frames', '182', '--set', settings, '--ui-script', str(script), '--ui-report', str(output / 'states.tsv')]
    run = execute(command, output, binary, 180); save(output / 'run.json', run); assert run['returnCode'] == 0 and not run['timedOut'], run
    observed = states(output / 'states.tsv'); details = {}
    for label in ('ready', 'final'):  # 같은 실제 Session/미션/비정지 상태를 확인한다.
        current = observed[label]; assert current['phase'] == ['4'] and current['paused'] == ['0'] and current['raw_session'] == ['1', '1'], (label, current.get('phase'))
        assert current['mission'][0].lower() == 'tutorial1'; details[label] = {'money': float(current['raw_sp'][1]), 'time': float(current['time'][0])}
    text = (output / 'states.tsv').read_text(encoding='utf-8')
    if args.enabled:
        before = details['ready']['money']; first = float(observed['after-one']['raw_sp'][1]); second = float(observed['after-two']['raw_sp'][1])
        assert first == before + 10000 and second == first + 10000 and details['final']['money'] == second, (before, first, second)
        assert '\tGet 10,000\n' in text and '\tCheat\n' in text
        details.update(firstMoney=first, secondMoney=second)
    else:
        assert 'Get 10,000' not in text and '\tCheat\n' not in text
        assert details['ready']['money'] == details['final']['money']
    presentation = verify_presentation(output / 'final.bmp', output / 'final-output.bmp', observed['final'], output); assert presentation['same']
    report = {'passed': True, 'enabled': args.enabled, 'run': run, 'gameSha256': sha(binary), 'details': details, 'presentation': presentation,
              'scope': '실제 초기 안내/ESC/Cheat 메뉴 두 번 클릭과 같은 로컬 분산 SP/창RGB. userCheat 원래 magic 설정이며 SP/의식/적 공격/승리 주입 없음.'}
    save(output / 'summary.json', report); print(json.dumps({'passed': True, 'enabled': args.enabled, 'seconds': run['seconds'], 'details': details}))


if __name__ == '__main__':
    main()
