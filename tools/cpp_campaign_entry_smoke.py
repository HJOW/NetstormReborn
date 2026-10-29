#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 클론의 튜토리얼/초기 캠페인 브리핑과 raw 세션 진입을 같은 UI 입력으로 확인한다."""
import argparse
import json
from pathlib import Path
import subprocess
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states

ROOT = Path(__file__).resolve().parents[1]  # 보호 자료와 이번 빌드의 기준 경로다.
EXE = ROOT / 'cpppj/build/bin/Release/Netstorm.exe'  # 원본 실행 파일은 시작하지 않는다.
OUT = ROOT / 'extracted/run32-entry'  # 실제 창의 입력·로그·상태·이미지를 보관한다.


def check(mission, play, movement=False, combat=False, output=OUT):
    """현재 MissionBegin→브리핑→Play Mission 경로를 실행하고 같은 raw 월드의 출력을 읽는다."""
    directory = output / (mission + ('-combat' if combat else '-movement' if movement else ''))
    directory.mkdir(parents=True, exist_ok=True)
    steps = [(3, 'report', 'briefing'), (3, 'snapshot', str(directory / 'briefing.bmp'))]
    if play:
        steps += [(5, 'click', 'Play Mission'), (6, 'report', 'mission'),
                  (8, 'snapshot', str(directory / 'mission.bmp'))]
    if movement or combat:
        steps += [(7, 'click', 'world:priest'), (8, 'report', 'selected'),
                  (9, 'click', 'cell:122,114' if combat else 'cell:90,108'), (10, 'report', 'command'),
                  (1999, 'report', 'after'), (1999, 'snapshot', str(directory / 'after.bmp'))]
        if combat:
            steps += [(frame, 'report', f'combat{frame}') for frame in range(600,1900,50)] # 실제 탄체/충돌을 같은 raw 보고서에서 관찰할 시점이다.
        steps.sort(key=lambda step: step[0])  # 같은 프레임의 기존 사건 순서를 유지하며 시간순으로 배치한다.
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--mission', mission, '--window', '--no-audio',
               '--frames', '2000' if movement or combat else '10', '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0',
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 콘솔용 보조 창이 전면에 뜨지 않게 한다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=55)
    (directory / 'stdout.log').write_bytes(result.stdout)
    (directory / 'stderr.log').write_bytes(result.stderr)
    record = {'mission': mission, 'command': command, 'exit_code': result.returncode,
              'stderr': result.stderr.decode('utf-8', errors='replace')}
    (directory / 'result.json').write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    # 긴 이동 실행의 stderr에는 실제 PlayerRegular 로그가 기록된다. 오류는 프로세스 종료 코드로 판정한다.
    # 실제로그인/타이머의확인한상태로그만초기진입에서허용한다. 알수없는stderr는계속실패다.
    login_lines = {'No Central Island.', '로컬 Fort 전송', 'login status timer start',
                   'Seting Priest State', 'Done', 'Moveing Camera', 'Player UI 5 1 0', 'Player UI 3 1 1'}
    login_lines.update(f'로컬 미션 상태 {phase}' for phase in (8, 9, 10, 13, 14))
    stderr_lines = [line for line in record['stderr'].splitlines() if line]
    assert result.returncode == 0 and (movement or combat or all(line in login_lines for line in stderr_lines)), record
    observed = states(directory / 'states.tsv')
    briefing = observed['briefing']
    assert briefing['phase'] == ['3'] and briefing['paused'] == ['1']
    assert briefing['mission'][0] == mission and briefing['mission'][1].lower() == 'tutorial'
    assert briefing['objects'] and briefing['raw_objects']
    assert briefing['raw_world'][2] != '0', '실제 Player/Gun/Regular Kernel이 없는 이전 어댑터'
    if play:
        playing = observed['mission']
        assert playing['phase'] == ['4'] and playing['paused'] == ['0']
        assert any(int(value[0]) >= 70 for value in playing['raw_objects'].values())
        assert any(value[0] == 'priest' and value[1] == '1' for value in playing['objects'].values())
        assert playing['raw_world'][6:9] == ['0', '0', '0']
        record['raw_objects'] = len(playing['raw_objects'])
        record['camera'] = playing['camera']
    if movement:
        selected = int(observed['selected']['selected'][0])
        before = observed['selected']['objects'][selected]
        after = observed['after']['objects'][selected]
        # 같은 실제 선택/강조/로그인 전송이 클릭 전후 원본40h를 소비했는지 확인한다.
        input_before, input_command = observed['selected']['raw_input'], observed['command']['raw_input']
        packets_before = list(map(int, observed['selected']['raw_packets']))
        packets_command = list(map(int, observed['command']['raw_packets']))
        assert input_before[:3] == [str(selected), '5', '14'] and input_command[:3] == ['0', '0', '14']
        assert int(input_before[3]) == len(packets_before) and int(input_command[3]) == len(packets_command)
        assert packets_command == packets_before + [0x40]
        assert before[5:7] != after[5:7] and after[5:7] == ['90', '108']
        assert observed['after']['raw_world'][6:9] == ['0', '0', '0']
        record['movement'] = {'sid': selected, 'from': before[5:7], 'to': after[5:7],
                              'game_before': observed['command']['raw_world'][0], 'game_after': observed['after']['raw_world'][0],
                              'inputPipeline': {'selectionMode': 5, 'loginPhase': 14, 'receivedBefore': len(packets_before),
                                                'receivedAfter': len(packets_command), 'newTypes': [0x40]}}
    if combat:
        selected = int(observed['selected']['selected'][0])
        before = observed['selected']['objects'][selected]
        after = observed['after']['objects'][selected]
        camera = list(map(int, briefing['camera']))
        assert 0 <= 122*16-camera[0] < 1024 and 28 <= 114*11-camera[1] < 768
        assert before[5:7] == ['94', '108'] and after[5:7] == ['122', '114']
        assert before[12] == '100' and after[12] == '1'
        assert observed['after']['raw_world'][6:9] == ['0', '0', '0']
        # 현재 같은 raw 슬롯에서 실제 투사체/충돌 생성도 관찰한다. 모델 상태를 직접 주입하지 않는다.
        effects = sorted({int(raw[0]) for stage in observed.values() for raw in stage['raw_objects'].values() if int(raw[0]) in (83,160)})
        assert effects
        record['combat'] = {'sid': selected, 'from': before[5:7], 'to': after[5:7], 'hp_before': before[12],
                            'hp_after': after[12], 'page_observed': observed['after']['page'][0],
                            'phase_observed': observed['after']['phase'][0], 'paused_observed': observed['after']['paused'][0],
                            'warning_ui_verified': False, 'effects': effects,
                            'game_before': observed['command']['raw_world'][0], 'game_after': observed['after']['raw_world'][0]}
    record['passed'] = True
    (directory / 'result.json').write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(directory.name, 'PASS', flush=True)
    return record


def main():
    """설정 바이트와 존재 상태를 복구하고 다섯 클론 실행 전후 보호 디렉터리 전체를 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--movement', action='store_true', help='실제 1-1 마우스 이동을 2000프레임 동안 추가 확인한다')
    parser.add_argument('--combat', action='store_true', help='실제 1-1 이동/적 공격/HP1 안내를 추가 확인한다')
    parser.add_argument('--output', type=Path, default=OUT, help='기존 관찰을 보존할 별도 결과 폴더')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    before = snapshots(ROOT / 'originals')
    records = []
    with preserve_game_settings(ROOT / 'originals', 'd'):
        # 같은 보호 설정을 쓰므로 창은 한 번에 하나만 시작한다.
        for mission, play in (('thewarbegins', True), ('masterofwhirligigs', True),
                              ('tutorial1', False), ('tutorial2', False), ('tutorial3', False)):
            records.append(check(mission, play, output=args.output))
        if args.movement:
            records.append(check('thewarbegins', True, movement=True, output=args.output))
        if args.combat:
            records.append(check('thewarbegins', True, combat=True, output=args.output))
    assert snapshots(ROOT / 'originals') == before
    (args.output / 'results.json').write_text(json.dumps({'runs': records, 'originals_unchanged': True}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
