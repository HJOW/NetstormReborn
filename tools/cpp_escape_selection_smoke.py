#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ESC의 선택 정리→메뉴 왕복을 일반/정지 상태에서 짧게 확인한다."""
import argparse
import json
from pathlib import Path
import re
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import protected_run
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def main():
    """공격/생산 없이 현재 사제를 선택하고 ESC가 실제 선택과 메뉴에 준 효과를 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    steps = [(230, 'key', 'P'), (240, 'report', 'selected'), (245, 'esc', ''), (255, 'report', 'escaped'),
             (260, 'esc', ''), (270, 'report', 'closed'), (275, 'key', 'P'), (285, 'report', 'selectedAgain'),
             (290, 'key', 'shift+f9'), (300, 'report', 'pausedSelected'), (305, 'esc', ''), (315, 'report', 'pausedEscaped'),
             (318, 'snapshot-presented', str(out/'paused-menu.bmp')), (320, 'esc', ''), (330, 'report', 'pausedClosed'),
             (335, 'key', 'shift+f9'), (345, 'report', 'resumed'), (350, 'key', 'Q'), (360, 'report', 'held'),
             (365, 'esc', ''), (375, 'report', 'cancelled'), (380, 'esc', ''), (390, 'report', 'openedAfterCancel'),
             (395, 'esc', ''), (405, 'report', 'final')]
    # 실제 안내가 표시됐을 때만 해당 버튼을 클릭한다.
    for frame in range(12, 220, 8):
        # 로딩 중 좌표나 이전 안내의 버튼을 재사용하지 않는다.
        for label in BUTTONS:
            condition = r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'
            steps.append((frame, 'when', condition+';;click;;'+label))
    steps.sort(key=lambda row:row[0])
    report, run = protected_run(out/'run', 1024, 768, steps, 415, mission='thewarbegins')
    observed = states(report)
    labels = ('selected', 'escaped', 'closed', 'selectedAgain', 'pausedSelected', 'pausedEscaped', 'pausedClosed', 'resumed', 'held', 'cancelled', 'openedAfterCancel', 'final')
    compact = {label:{key:observed[label].get(key) for key in ('phase', 'selected', 'game_menu_bar', 'paused', 'time', 'raw_cursor')} for label in labels}
    save(out/'observations.json', compact)
    # 모든 비교는 같은 실제 월드와 선택 SID를 대상으로 한다.
    for label in labels:
        assert observed[label]['phase'] == ['4'], (label, compact[label])
    assert int(observed['selected']['selected'][0]) and int(observed['selectedAgain']['selected'][0])
    # 원문은 정지 여부와 관계없이 선택을 해제한 뒤 메뉴를 토글한다.
    for label in ('escaped', 'pausedEscaped'):
        assert observed[label]['selected'][0] == '0' and observed[label]['game_menu_bar'][1] == '1', (label, compact[label])
    for label in ('closed', 'pausedClosed', 'resumed', 'cancelled', 'final'):
        assert observed[label]['selected'][0] == '0' and observed[label]['game_menu_bar'][1] == '0', (label, compact[label])
    assert observed['pausedSelected']['paused'] == observed['pausedEscaped']['paused'] == observed['pausedClosed']['paused'] == ['1']
    assert observed['pausedSelected']['time'][0] == observed['pausedEscaped']['time'][0] == observed['pausedClosed']['time'][0]
    assert observed['resumed']['paused'] == ['0']
    assert observed['held']['raw_cursor'][0] == '0' and int(observed['held']['raw_cursor'][3]) > 0
    assert observed['cancelled']['raw_cursor'][0] == '1' and observed['cancelled']['raw_cursor'][3] == '0'
    assert observed['openedAfterCancel']['game_menu_bar'][1] == '1'
    summary = dict(unit=742, passed=True, processes=1, exeSha256=run['binarySha256'], returnCode=0, seconds=run['seconds'],
                   originalsUnchanged=run['originalsUnchanged'], dataUnchanged=run['dataUnchanged'], observations=compact,
                   scope='앱 내 선택/ESC/정지·메뉴 입력이다. 물리 키/원본 전체 UI/자연 완주/포획·희생/특정 적 공격/승리/공중3종/MP 검사가 아니다.')
    save(out/'summary.json', summary)
    print(json.dumps({key:value for key,value in summary.items() if key!='observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
