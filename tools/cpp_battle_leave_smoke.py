#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ESC→Game→Leave Mission→Main Menu 입력으로 Native State3 복귀를 확인한다."""
import argparse
import json
from pathlib import Path
from cpp_bridge_ritual_runtime import GAME, execute, save, sha
from cpp_world_smoke import states


def main():
    """보호 원본 대신 지정한 사본에서 실제 버튼과 원시 보고/화면/종료를 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=GAME); parser.add_argument('--mission', default='TheWarBegins'); parser.add_argument('--start-button', default='Play Mission'); parser.add_argument('--fullscreen', action='store_true'); args = parser.parse_args()
    data = args.game_dir.resolve(); binary = args.binary.resolve(); out = args.output.resolve()
    if data.name.lower() in ('originals', 'originalcd'): raise ValueError('종료 검사는 별도 게임 자료 사본을 지정해야 합니다')
    out.mkdir(parents=True, exist_ok=False)
    steps = [(5, 'report', 'briefing'), (10, 'click', args.start_button), (20, 'report', 'playing'),
             (25, 'esc', ''), (28, 'report', 'menu'), (30, 'click', 'Game'), (34, 'report', 'game-menu'),
             (36, 'click', 'Leave Mission'), (43, 'report', 'confirm'), (46, 'click', 'Main Menu'),
             (49, 'report', 'returned'), (50, 'click-if', 'OK'), (53, 'click-if', 'OK'), (56, 'click-if', 'Main Menu'),
             (70, 'report', 'final'), (70, 'snapshot', str(out / 'main-menu.bmp'))]  # 환영 안내가 실제로 열리면 같은 버튼으로만 닫는다.
    plan = out / 'steps.tsv'; plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(binary), '--run', str(data), '--mission', args.mission, '--no-audio', '--frames', '76', '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=60;sleepPerLoop=8;autoDemo=0;tellTips=0;startInFullScreen=' + ('1' if args.fullscreen else '0'),
               '--ui-script', str(plan), '--ui-report', str(out / 'states.tsv')]
    if not args.fullscreen: command.append('--window')
    run = execute(command, out, binary, 90); save(out / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'], run
    observed = states(out / 'states.tsv'); playing = observed['playing']; final = observed['final']
    assert playing['phase'] == ['4'] and playing['raw_session'][:2] == ['1', '1'], playing.get('phase')
    assert final['phase'] == ['0'] and final['page'] == ['main'] and final['paused'] == ['0'], {key: final.get(key) for key in ('phase', 'page', 'paused')}
    assert 'raw_session' not in final and 'mission' not in final
    assert any('Main Menu' in str(rows) for rows in observed['confirm']['html_buttons'].values())
    log = (out / 'stderr.log').read_text(encoding='utf-8'); assert 'State 3\n' in log and 'State 4\n' in log
    summary = {'passed': True, 'gameSha256': sha(binary), 'mission': args.mission, 'fullscreen': args.fullscreen, 'run': run,
               'playing': {key: playing.get(key) for key in ('phase', 'paused', 'mission', 'raw_session')},
               'returned': {key: observed['returned'].get(key) for key in ('phase', 'page', 'paused')},
               'final': {key: final.get(key) for key in ('phase', 'page', 'paused')},
               'scope': '일반 중단 확인과 실제 Native 메인 복귀. 포획/희생/특정 공격/승리·원격 플레이 실행 없음.'}
    save(out / 'summary.json', summary); print(json.dumps({'passed': True, 'seconds': run['seconds'], 'mission': args.mission}, ensure_ascii=False))


if __name__ == '__main__': main()
