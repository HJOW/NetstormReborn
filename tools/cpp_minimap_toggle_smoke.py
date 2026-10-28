#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사용자 시나리오의 원본 ToggleMinimap 버튼으로 삭제·독립 생성·입력·이탈을 확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated

# 원본4:3·확장16:9 전체화면·확장16:10 창을 같은 명령으로 대조한다.
SIZES = ((1024, 768, False), (1280, 720, True), (1280, 800, False))


def verify(report, run, width, height, fullscreen=False):
    """같은 실제 HUD와 세대별 현재 표시·입력·모드/자금·이탈 정리를 확인한다."""
    observed = states(report)
    initial = observed['initial']
    root, production = initial['raw_uber'][0], initial['raw_hud_owners'][2]
    for label in ('initial', 'hidden', 'floating', 'held', 'released', 'hiddenAgain', 'floatingAgain'):
        # 표시를 끄거나 새로 만들어도 전체 HUD·덱·지도 영역과 SP는 유지돼야 한다.
        row = observed[label]
        assert row['phase'] == ['4'] and row['size'] == list(map(str, (width, height)))
        assert row['paused'] == ['0'] if label != 'initial' else row['paused'][0] in ('0', '1')
        assert bool(int(row['screen_mode'][0]) & 1) == fullscreen
        assert row['state_hud'][:2] == ['1', '1'] and row['raw_uber'] == initial['raw_uber']
        assert row['raw_hud_owners'][2] == production and float(row['raw_sp'][1]) == 500000
        assert row['mouse_cursor'][1] == '1' and row['raw_world_modes'] == ['0', '1']
    for label in ('hidden', 'hiddenAgain'):  # 지연 정리 후 현재 표시 포인터와 소유자가 함께 사라져야 한다.
        row = observed[label]
        assert 'minimap' not in row and row['raw_hud_owners'][1] == '0' and row['raw_hud_owners'][3] == '0'
    tokens = [initial['raw_hud_owners'][3]]
    for label in ('floating', 'held', 'released', 'floatingAgain'):  # 원본 재생성은 도킹80×75 대신 독립100,100·128×128이다.
        row = observed[label]
        assert row['minimap'][1:5] == ['100', '100', '128', '128']
        assert row['raw_hud_owners'][1] == '1' and row['state_hud'][2] == row['state_hud'][3] == row['raw_hud_owners'][3]
    tokens.extend(observed[label]['raw_hud_owners'][3] for label in ('floating', 'floatingAgain'))
    assert len(set(tokens)) == 3
    assert observed['held']['minimap'][7] == '1' and observed['released']['minimap'][7] == '0'
    assert observed['held']['camera'] != observed['floating']['camera']
    assert observed['released']['camera'] == observed['held']['camera']
    assert observed['leave']['page'][0].casefold() == 'abort' and observed['leave']['paused'] == ['1']
    assert observed['main']['phase'] == ['0'] and observed['main']['state_hud'][0] == '0'
    assert 'minimap' not in observed['main']
    for label in ('review', 'reviewAgain'):  # 같은 원본 ESC Game→Review 경로로 사용자 시나리오 버튼을 다시 연다.
        assert any(row[-1] == 'Toggle Sky Overview' and int(row[3]) == 0x02000037
                   for row in repeated(report, 'raw_html_button')[label])
        assert not any('CHEAT' in item['label'].upper() for item in menu_items(report, label) if item['visible'])
    return dict(size=[width, height], root=root, productionSerial=production, minimapTokens=tokens,
                cameraBefore=observed['floating']['camera'], cameraAfter=observed['released']['camera'],
                currentGoneWhileHidden=True, independentBounds=[100,100,128,128],
                captureReleased=True, hudAndSpPreserved=True, mainMenuReturned=True,
                initialPause=initial['paused'][0], fullscreen=fullscreen, run=run)


def one(source, output, width, height, fullscreen):
    """새 사본의 A. 섹션에 원본 명령 버튼 한 줄만 넣고 실제 메뉴/클릭을 실행한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    english = data / 'd/TEST02.english'
    before = english.read_bytes()
    needle = b'$Button=Go!,DoNothing,0'
    assert before.count(needle) == 1
    english.write_bytes(before.replace(needle, b'$Button=Toggle Sky Overview,ToggleMinimap,0\r\n' + needle))
    save(output / 'scenario-variant.json', dict(sourceEnglishSha256=sha(source / 'd/TEST02.english'),
         englishSha256=sha(english), onlyChange='A. Go! 앞 ToggleMinimap 버튼 한 줄 추가',
         fortSha256=sha(data / 'd/TEST02.fort'), sourceFortSha256=sha(source / 'd/TEST02.fort')))
    steps = [(5, 'report', 'initial'), (10, 'click', 'Toggle Sky Overview'), (20, 'report', 'hidden'),
             (25, 'esc', ''), (30, 'click', 'Game'), (35, 'click', 'menu-command:02000091,0'),
             (40, 'report', 'review'), (45, 'click', 'Toggle Sky Overview'), (55, 'report', 'floating'),
             (55, 'snapshot', str(output / 'floating.bmp')), (60, 'down', 'minimap:40,40'),
             (65, 'report', 'held'), (70, 'up', 'minimap:40,40'), (75, 'report', 'released'),
             (80, 'esc', ''), (85, 'click', 'Game'), (90, 'click', 'menu-command:02000091,0'),
             (92, 'report', 'reviewAgain'), (95, 'click', 'Toggle Sky Overview'), (105, 'report', 'hiddenAgain'),
             (110, 'esc', ''), (115, 'click', 'Game'), (120, 'click', 'menu-command:02000091,0'),
             (125, 'click', 'Toggle Sky Overview'), (135, 'report', 'floatingAgain'),
             (135, 'snapshot', str(output / 'floating-again.bmp')),
             (145, 'esc', ''), (150, 'click', 'Game'), (155, 'click', 'Leave Mission'),
             (165, 'report', 'leave'), (175, 'click', 'Main Menu'), (210, 'report', 'main'),
             (210, 'snapshot', str(output / 'main.bmp'))]
    report, run = protected_run(output / 'run', width, height, steps, 220, data_root=data, mission='TEST02', fullscreen=fullscreen)
    result = verify(report, run, width, height, fullscreen)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본을 입력에서 거부하고 세 화면비의 실제 수명 전환을 순차 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert all(source != (ROOT / name).resolve() and (ROOT / name).resolve() not in source.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    binary, rows = sha(GAME), []
    for width, height, fullscreen in SIZES:  # 창과 전체화면을 공유하므로 게임은 동시에 실행하지 않는다.
        rows.append(one(source, output / f'{width}x{height}', width, height, fullscreen))
        assert rows[-1]['run']['binarySha256'] == binary and sha(GAME) == binary
    save(output / 'summary.json', dict(results=rows, binarySha256=binary, directWorldMutation=False))
    print(f'독립 미니맵 삭제/생성·입력·이탈: {len(rows)}개 통과')


if __name__ == '__main__':
    main()
