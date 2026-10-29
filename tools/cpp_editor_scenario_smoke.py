#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Edit와 시험 전투 왕복에서 작업본 시나리오의 SP/AI/동맹/안내를 관측한다."""
import argparse
import json
import struct
from pathlib import Path
from cpp_bridge_ritual_runtime import GAME, execute, save, sha
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated


def main():
    """검사 사본의 실제 버튼만 누르고 Player/AI의 현재 원문을 읽어 시작 설정을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=GAME); parser.add_argument('--map', default='TEST02'); parser.add_argument('--language', default='english')
    parser.add_argument('--map-label'); parser.add_argument('--briefing-title'); parser.add_argument('--mission-spec')
    parser.add_argument('--money', type=int, required=True); parser.add_argument('--ai-name', required=True); parser.add_argument('--ai-money', type=int, required=True); args = parser.parse_args()
    data = args.game_dir.resolve(); binary = args.binary.resolve(); out = args.output.resolve()
    if data.name.lower() in ('originals', 'originalcd'): raise ValueError('시나리오 검사는 보호 원본 대신 자료 사본을 지정해야 합니다')
    out.mkdir(parents=True, exist_ok=False)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (7, 'click-if', 'Main Menu'), (10, 'click', 'Edit'),
             (15, 'report', 'catalog'), (15, 'snapshot', str(out / 'catalog.bmp')), (18, 'click', args.map_label or args.map), (55, 'report', 'editor'), (55, 'snapshot', str(out / 'editor.bmp')),
             (80, 'esc', ''), (85, 'click', 'Game'), (90, 'click', 'Test Battle'), (130, 'report', 'briefing'),
             (140, 'click-if', 'Go!'), (155, 'click-if', 'Go!'), (170, 'report', 'playing'), (170, 'snapshot', str(out / 'playing.bmp')),
             (180, 'esc', ''), (185, 'click', 'Game'), (190, 'click', 'Return to Editing'), (220, 'report', 'returned'),
             (230, 'esc', ''), (235, 'click', 'Game'), (240, 'click', 'Test Battle'), (260, 'report', 'briefing-again'),
             (270, 'click-if', 'Go!'), (280, 'report', 'playing-again'), (290, 'esc', ''), (295, 'click', 'Game'),
             (300, 'click', 'Return to Editing'), (325, 'report', 'final'), (325, 'snapshot', str(out / 'returned.bmp'))]  # 승패나 특정 공격 없이 원문 왕복만 수행한다.
    plan = out / 'steps.tsv'; plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(binary), '--run', str(data), '--window', '--no-audio', '--frames', '332', '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=60;sleepPerLoop=8;autoDemo=0;tellTips=0;currentLanguage=' + args.language + (';missionSpec=' + args.mission_spec if args.mission_spec else ''),
               '--ui-script', str(plan), '--ui-report', str(out / 'states.tsv')]
    run = execute(command, out, binary, 120); save(out / 'run.json', run); assert run['returnCode'] == 0 and not run['timedOut'], run
    observed = states(out / 'states.tsv'); records = repeated(out / 'states.tsv', 'raw_player_record'); settings = repeated(out / 'states.tsv', 'raw_ai_settings'); details = []
    for label in ('editor', 'returned', 'final'):  # 두 번 복귀한 실제 Editor 모드/Session을 확인한다.
        current = observed[label]; assert current['raw_world_modes'] == ['1', '1'] and current['raw_session'][:2] == ['1', '1'], (label, current.get('raw_world_modes'))
        assert current['phase'] == ['4'] and current['paused'] == ['0'], (label, current.get('phase'), current.get('paused'))
    for label in ('playing', 'playing-again'):  # 같은 실제 시작 경로에서 로컬 SP와 AI 본체/Player/동맹을 함께 확인한다.
        current = observed[label]; assert current['phase'] == ['4'] and current['paused'] == ['0'] and current['raw_world_modes'] == ['0', '1'], (label, current.get('phase'))
        assert current['mission'][0].casefold() == args.map.casefold(), (label, current.get('mission'))
        assert float(current['raw_sp'][1]) == args.money, (label, current['raw_sp'])
        assert current['raw_brains'][1] == '0' and int(current['raw_brains'][2]) != 0, (label, current['raw_brains'])
        config = {int(row[0]): bytes.fromhex(row[1]) for row in settings[label]}; assert struct.unpack_from('<I', config[2], 0x22c)[0] == args.ai_money, (label, config[2].hex())
        players = {int(row[0]): bytes.fromhex(row[1]) for row in records[label]}; name = players[2][12:42].split(b'\0', 1)[0].decode('cp1252')
        assert name == args.ai_name, (label, name, args.ai_name)
        alliances = list(map(int, current['raw_alliances'])); assert alliances[1 * 9 + 2] == 1 and alliances[2 * 9 + 1] == 1
        details.append({'label': label, 'sp': current['raw_sp'], 'brains': current['raw_brains'], 'aiMoney': current['raw_ai_money'], 'ai2StartingMoney': struct.unpack_from('<I', config[2], 0x22c)[0], 'ai2Off': struct.unpack_from('<I', config[2], 0x254)[0], 'ai2Name': name, 'ai2PlayerBytes': players[2].hex(), 'alliances': alliances})
    bodies = repeated(out / 'states.tsv', 'raw_html_body')
    for label in ('briefing', 'briefing-again'):  # 실제 HTML 본문과 버튼을 확인하며 Header 제목만으로 표시 성공을 추정하지 않는다.
        assert any('Go!' in str(rows) for rows in observed[label]['html_buttons'].values()), label
        if args.briefing_title: assert any(args.briefing_title in row[3] for row in bodies[label]), (label, bodies[label])
    summary = {'passed': True, 'gameSha256': sha(binary), 'language': args.language, 'missionSpecOverride': args.mission_spec, 'map': args.map, 'run': run, 'details': details,
               'briefing': {label: {'page': observed[label].get('page'), 'buttons': observed[label].get('html_buttons')} for label in ('briefing', 'briefing-again')},
               'actualBriefingBodies': {label: bodies[label] for label in ('briefing', 'briefing-again')},
               'scope': '사본의 사용자 SP/AI 이름·시작돈/방향별 동맹과 Edit 왕복. 포획/희생/특정 공격/승리·공중3종 전용·원격 검사 없음.'}
    save(out / 'summary.json', summary); print(json.dumps({'passed': True, 'seconds': run['seconds'], 'language': args.language, 'money': args.money}, ensure_ascii=False))


if __name__ == '__main__': main()
