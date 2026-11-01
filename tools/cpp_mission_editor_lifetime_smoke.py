#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early1 중단 뒤 Edit·시험 전투·재시작·편집 복귀·메인의 연속 수명을 관측한다(716)."""
import argparse
import json
from pathlib import Path
import re
import shutil
from PIL import Image
from cpp_bridge_ritual_runtime import ROOT, save, sha
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run
from cpp_production_panel_smoke import repeated
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def prepare(out):
    """보호 원본을 읽어 새 실행 사본과 승패 없는 별도 시험 시나리오를 만든다."""
    game = out / 'game'  # 원본과 다른 source/work namespace에만 옵션과 편집 결과를 쓴다.
    shutil.copytree(ROOT / 'originals', game)
    maps = game / 'd/lifetime716'  # 실제 Catalog의 source root 내부 상대 FileSpec을 사용한다.
    maps.mkdir()
    shutil.copyfile(ROOT / 'originals/d/TEST02.fort', maps / 'TEST02.fort')
    original = (ROOT / 'originals/d/TEST02.english').read_bytes()
    scenario = original.split(b'[Succeeded]')[0].replace(b'[Options]', b'ai2Off=1\nai3Off=1\n\n[Options]')
    assert b'[Succeeded]' not in scenario and b'[Failed]' not in scenario
    (game / 'd/TEST02.english').write_bytes(scenario)
    save(out / 'prepared.json', dict(sourceFortSha256=sha(ROOT / 'originals/d/TEST02.fort'), copiedFortSha256=sha(maps / 'TEST02.fort'), sourceScenarioSha256=sha(ROOT / 'originals/d/TEST02.english'), copiedScenarioSha256=sha(game / 'd/TEST02.english'), scenarioChanges='검사 사본만 Succeeded 이후 섹션 제외·ai2Off/ai3Off=1. 기존 게임 시나리오 동일성 증거가 아니다.', gameplayStateInjected=False))
    return game


def plan(out):
    """메뉴의 실제 라벨과 같은 HTML Config/큐만 사용하고 World/승패 상태를 주입하지 않는다."""
    steps = [(230, 'require', r'phase\t4\n'), (235, 'report', 'initial'),
             (240, 'esc', ''), (245, 'click', 'Game'), (250, 'click', 'Leave Mission'),
             (260, 'report', 'abort'), (270, 'click', 'Main Menu'), (300, 'report', 'main'),
             (310, 'script-command', 'Config,SOLOSpec=d/lifetime716/{local.1}.fort'),
             (310, 'script-command', 'Config,fortSpec=d/lifetime716/{local.1}.fort'),
             (320, 'click', 'Edit'), (330, 'report', 'catalog'), (340, 'click', 'TEST02'),
             (390, 'report', 'editor'), (390, 'snapshot-presented', str(out / 'editor.bmp')),
             (400, 'esc', ''), (410, 'click', 'Game'), (420, 'click', 'Test Battle'),
             (450, 'report', 'briefing'), (460, 'click', 'Go!'), (470, 'report', 'playing'),
             (480, 'esc', ''), (490, 'click', 'Game'), (500, 'click', 'Return to Editing'), (540, 'report', 'returned'),
             (550, 'esc', ''), (560, 'click', 'Game'), (570, 'click', 'Test Battle'),
             (600, 'report', 'briefingAgain'), (610, 'click', 'Go!'), (620, 'report', 'playingAgain'),
             (630, 'esc', ''), (640, 'click', 'Game'), (650, 'click', 'Restart Mission'),
             (680, 'report', 'restartBriefing'), (690, 'click', 'Go!'), (700, 'report', 'restarted'),
             (710, 'esc', ''), (720, 'click', 'Game'), (730, 'click', 'Return to Editing'), (770, 'report', 'returnedAgain'),
             (780, 'esc', ''), (790, 'click', 'Game'), (800, 'click', 'Main Menu'),
             (810, 'report', 'leaveEdit'), (820, 'click', 'Cancel'), (830, 'report', 'cancelled'),
             (840, 'esc', ''), (850, 'click', 'Game'), (860, 'click', 'Main Menu'),
             (870, 'report', 'leaveEditAgain'), (880, 'click', 'No'), (920, 'report', 'final')]
    for frame in range(12, 220, 8):  # 비동기 최초 설치가 실제로 표시한 안내만 닫는다.
        for label in BUTTONS:  # 알려진 원문 계속 버튼 이외의 상태/명령은 만들지 않는다.
            condition = r'phase\t[13]\n&&(?:control\t' + re.escape(label) + r'\t|raw_html_button[^\n]*\t' + re.escape(label) + r'\n)'
            steps.append((frame, 'when', condition + ';;click;;' + label))
    steps.sort(key=lambda row: row[0])
    return steps


def inspect(out):
    """이미 종료한 한 프로세스의 현재 보고·전체 보호 장부·실제 창 출력 사본을 판정한다."""
    before = json.loads((out / 'protected-before.json').read_text('utf-8'))
    after = json.loads((out / 'protected-after.json').read_text('utf-8'))
    assert before == after
    run = json.loads((out / 'run/run.json').read_text('utf-8'))
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(out / 'run/states.tsv')
    for label in ('main', 'final'):  # Native cleanup 뒤 Guide와 현재 Session이 실제로 없어야 한다.
        current = observed[label]
        assert current['phase'] == ['0'] and current['page'] == ['main']
        assert 'mission' not in current and 'raw_session' not in current and 'raw_world_modes' not in current
        assert current['fort_selection'][1:4] == ['4', '0', '0']
    for label in ('editor', 'returned', 'returnedAgain', 'cancelled'):  # 같은 Client의 새 설치/복귀는 Editor1과 실제 장치 확정을 유지한다.
        current = observed[label]
        assert current['phase'] == ['4'] and current['paused'] == ['0'] and current['page'] == ['editor']
        assert 'mission' not in current and current['raw_world_modes'] == ['1', '1'] and current['raw_session'][:2] == ['1', '1']
        assert current['editor_runtime'][:3] == ['1', '1', '1'] and current['fort_selection'][-1] == 'TEST02'
    for label in ('playing', 'playingAgain', 'restarted'):  # Test Battle와 Restart 뒤 원래 mode64 복원이 같은 복귀 메뉴를 계속 제공해야 한다.
        current = observed[label]
        assert current['phase'] == ['4'] and current['paused'] == ['0'] and current['mission'][:2] == ['TEST02', 'Tutorial']
        assert current['raw_world_modes'] == ['0', '1'] and current['raw_session'][:2] == ['1', '1']
        assert float(current['raw_sp'][1]) == 500000 and current['raw_brains'][1:4] == ['0', '0', '0']
    for label in ('briefing', 'briefingAgain', 'restartBriefing'):  # 같은 작업 namespace의 실제 본문/버튼을 세 번 확인한다.
        assert observed[label]['phase'] == ['3'] and observed[label]['page'] == ['A.']
        assert any(row[-1] == 'Go!' for row in repeated(out / 'run/states.tsv', 'raw_html_button')[label])
    for label, page in (('abort', 'ABORT'), ('leaveEdit', 'LeaveEditMode'), ('leaveEditAgain', 'LeaveEditMode')):  # 확인창에서 선택 전까지 World를 반납하지 않는다.
        assert observed[label]['phase'] == ['1'] and observed[label]['page'] == [page] and 'raw_session' in observed[label]
    assert all('Succeeded' not in row.get('page', []) and 'Failed' not in row.get('page', []) for row in observed.values())
    image = Image.open(out / 'editor.bmp').convert('RGB')
    colors = len(image.getcolors(image.width * image.height))
    assert image.size == (1024, 768) and colors > 16
    image.save(out / 'editor.png')
    keys = ('frame', 'phase', 'page', 'time', 'mission', 'mission_start', 'mission_gate', 'raw_world_modes', 'raw_session', 'raw_sp', 'raw_brains', 'fort_selection', 'editor_runtime', 'shared_surface_storage', 'html_window_state')
    details = {label: {key: current[key] for key in keys if key in current} for label, current in observed.items()}
    result = dict(unit=716, passed=True, processes=1, binarySha256=run['binarySha256'], seconds=run['seconds'], protectionUnchanged=True, editorCapture=dict(size=list(image.size), rgbColors=colors, sha256=sha(out / 'editor.bmp')), observations=details, scope='앱 내 입력의 Early1 중단→메인→Edit→Test Battle 두 번/Restart 한 번→편집 복귀 두 번→이탈 Cancel/No. 물리 입력·전체 원문 동일성·자연 완주·PCM·승패/포획/희생/특정 적 공격·공중3종 전용·MP/통신 검사는 아니다.')
    save(out / 'summary.json', result)
    return result


def main():
    """새 자료에만 실행하며 verify-existing은 게임을 다시 시작하지 않고 기존 관측을 분석한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify-existing', action='store_true')
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    out = args.output.resolve()
    assert args.timeout > 0
    if args.verify_existing:
        assert out.is_dir() and not (out / 'summary.json').exists()
    else:
        out.mkdir(parents=True, exist_ok=False)
        save(out / 'protected-before.json', protection())
        try:
            game = prepare(out)
            protected_run(out / 'run', 1024, 768, plan(out), 930, data_root=game, mission='tutorial1', timeout=args.timeout)
        finally:
            save(out / 'protected-after.json', protection())
    result = inspect(out)
    print(json.dumps({key: value for key, value in result.items() if key != 'observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
