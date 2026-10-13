#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 실제 Game 버튼의 월드 release·raw 수명·캠페인 재진입을 검사한다. 원본 게임 실행 없음."""
import argparse
import json
from pathlib import Path
from cpp_menu_smoke import reports, run
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings

# 산출물은 기존 원본 바깥에 저장한다. 기본 대상은 새로 빌드한 10.78 C++ 클론이다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-rawbuttons-smoke'


def main():
    """기존 입력 큐/폴링 경로를 사용하고 옵션 예외 두 파일은 성공·실패 모두 복구한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'originals'
    before = snapshots(source)
    steps = [
        (2, 'report', 'briefing'), (3, 'click', 'Play Mission'), (4, 'report', 'mission'),
        (5, 'down', 'Game'), (5, 'report', 'captured'),
        (6, 'up', 'world:priest'), (7, 'report', 'world_released'),
        (8, 'click', 'Game'), (9, 'report', 'game_menu'),
        (10, 'click', 'Leave Battle'), (11, 'click', 'Stay'), (12, 'report', 'stayed'),
        (13, 'down', 'Game'), (13, 'report', 'captured_again'),
        (14, 'up', 'world:priest'), (15, 'report', 'world_released_again'),
        (16, 'esc', ''), (17, 'click', 'Leave Battle'), (18, 'click', 'Leave Honorably'),
        (19, 'report', 'main'), (20, 'click', 'Campaign'), (22, 'click', 'Struggle For Freedom'),
        (24, 'click', '1 The War Begins!'), (27, 'click', 'Play Mission'), (29, 'report', 'reloaded'),
        (29, 'snapshot', str(OUTPUT / 'reloaded.bmp')),
    ]
    script = OUTPUT / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    report = OUTPUT / 'states.tsv'
    with preserve_game_settings(source, 'd'):
        run(args.exe, ['--run', source, '--window', '--no-audio', '--mission', 'TheWarBegins', '--frames', '31', '--set',
            'SCREENW=1024;SCREENH=768;maxFPS=75;autoDemo=0;tellTips=0;DoneTheWarBegins=0', '--ui-script', script, '--ui-report', report])
    state = reports(report)
    assert state['briefing']['phase'] == ['3'] and state['briefing']['paused'] == ['1']
    # Game 입력 트리에는 버튼/배경/루트 세 노드가 있어야 하고 초기 raw serial을 다음 페이지에서 재사용하지 않는다.
    for name in ('mission', 'world_released', 'stayed', 'world_released_again', 'reloaded'):
        current = state[name]
        assert current['phase'] == ['4'] and current['paused'] == ['0'], name
        count, registered, serial, focus, idle, nodes = map(int, current['RawButtons'])
        assert (count, registered, focus, idle, nodes) == (1, 3, 0, 0, 3), (name, current['RawButtons'])
    # 같은 폴링 위치에서 캡처가 유지되며 월드의 사제 위 release가 UI focus/idle을 모두 반납한다.
    for name in ('captured', 'captured_again'):
        current = list(map(int, state[name]['RawButtons']))
        assert current[3] != 0 and current[4] == 1, (name, current)
    assert state['game_menu']['popup'] == ['game']
    assert state['main']['phase'] == ['0'] and list(map(int, state['main']['RawButtons']))[0:2] == [8, 17]
    assert int(state['reloaded']['RawButtons'][2]) > int(state['mission']['RawButtons'][2])
    assert state['mission']['mission'] == state['reloaded']['mission']
    assert snapshots(source) == before, '원본 파일/존재 상태 변경'
    result = dict(states=len(state), world_release_checks=2, campaign_reentry=True, raw_registry_lifetime=True,
        original_files_unchanged=len(before), campaign_completed=False)
    (OUTPUT / 'report.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
