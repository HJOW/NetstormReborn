#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""승리 섹션 없는 Early1에서 현재 Guide를 유지한 AskEditFort/ReadFort의 State1→7 재설치를 관측한다(714)."""
import argparse
import json
import re
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def brief(scene):
    """결과를 만들지 않고 같은 실제 보고의 수명/상태/현재 플레이어 필드만 추린다."""
    keys = ('phase', 'page', 'time', 'fort_selection', 'fort_names', 'mission', 'mission_start', 'mission_gate', 'mission_init', 'mission_fort_request', 'shared_surface_storage', 'raw_input', 'raw_session', 'raw_world_modes', 'raw_connection_pending', 'state_hud')
    return {key: scene[key] for key in keys if key in scene}


def main():
    """실행 전후 전체 보호 장부와 원시 실패를 남기고 기존 실행은 읽기만 재분석한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output', type=Path, required=True);parser.add_argument('--verify-existing', action='store_true');args = parser.parse_args();out = args.output.resolve()
    if args.verify_existing:
        assert out.is_dir() and not (out / 'summary.json').exists()
    else:
        out.mkdir(parents=True, exist_ok=False);save(out / 'protection-before.json', protection())
        steps = [(5, 'report', 'startup'), (230, 'require', r'phase\t4\n'), (235, 'report', 'initial'), (240, 'script-command', 'AskEditFort,0'), (245, 'report', 'catalog'), (250, 'script-command', 'Tell,Blank'), (255, 'report', 'catalogClosed'), (260, 'script-command', 'ReadFort,BridgeTheGap'), (261, 'report', 'readQueued'), (265, 'report', 'reinstalling'), (290, 'report', 'reinstalled'), (335, 'report', 'running'), (335, 'snapshot-presented', str(out / 'reinstalled.bmp')), (345, 'esc', ''), (355, 'report', 'menu'), (365, 'esc', ''), (375, 'report', 'closed'), (385, 'key', 'H'), (395, 'report', 'home')]
        for frame in range(12, 220, 8):  # 원래 안내의 실제 표시 버튼만 클릭하고 첫 전투 진입을 기다린다.
            for label in BUTTONS:
                condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
                steps.append((frame, 'when', condition + ';;click;;' + label))
        steps.sort(key=lambda row: row[0])
        try:
            protected_run(out / 'run', 1024, 768, steps, 405, mission='tutorial1')
        finally:
            save(out / 'protection-after.json', protection())
    before = json.loads((out / 'protection-before.json').read_text('utf-8'));after = json.loads((out / 'protection-after.json').read_text('utf-8'));assert before == after
    run = json.loads((out / 'run/run.json').read_text('utf-8'));assert run['returnCode'] == 0 and not run['timedOut'], run
    observed = states(out / 'run/states.tsv');labels = ('initial', 'catalog', 'catalogClosed', 'readQueued', 'reinstalling', 'reinstalled', 'running', 'home', 'menu', 'closed')
    save(out / 'observations.json', {label: brief(observed[label]) for label in labels})
    initial, final = observed['initial'], observed['running'];assert initial['phase'] == final['phase'] == ['4']
    assert observed['catalog']['fort_selection'][0] == '2' and final['fort_selection'][:4] == ['2', '14', '0', '0']
    assert final['mission'][:2] == initial['mission'][:2] and final['mission'][0].lower() == 'tutorial1' and final['mission'][1].lower() == 'tutorial'
    assert final['raw_input'][2] == '14' and final['raw_session'][:2] == ['1', '1'] and final['raw_world_modes'] == ['0', '1']
    assert int(final['shared_surface_storage'][4]) > int(initial['shared_surface_storage'][4])
    assert float(final['time'][0]) > float(observed['reinstalled']['time'][0])
    assert observed['menu']['game_menu_bar'][1] == '1' and observed['closed']['game_menu_bar'][1] == '0'
    stdout = (out / 'run/stdout.log').read_text('utf-8', errors='replace');stderr = (out / 'run/stderr.log').read_text('utf-8', errors='replace')
    transition = (stdout + stderr).splitlines();begin = transition.index('State 1');selected = transition[begin:]
    assert selected.index('State 7') < selected.index('State 8') < selected.index('로컬 미션 상태 14')  # 실제 분배기의 순서를 확인한다. 소비 후 반납된 Zone의 임시 이름을 최종 Player 표시 이름으로 추정하지 않는다.
    summary = dict(unit=714, passed=True, processes=1, exeSha256=run['binarySha256'], returnCode=0, protectionUnchanged=True, observations={label: brief(observed[label]) for label in labels}, scope='Early1의 실제 HTML 이름표/문자열 고리/일반 큐와 State1→7 재설치·현재 Guide/Player/HUD·H/ESC를 짧게 관측한다. 앱 내 입력이며 물리 키/전체 픽셀·PCM·자연 완주·포획/희생/특정 적 공격/승리·공중3종 전용/MP/통신 검사는 아니다.')
    save(out / 'summary.json', summary);print(json.dumps({key: value for key, value in summary.items() if key != 'observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
