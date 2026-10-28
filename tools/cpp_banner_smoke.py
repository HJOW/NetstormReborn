#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 F7·View 옵션의 공통 배너/지면 표와 Edit 표시를 실제 입력으로 확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_game_menu_smoke import muted_device_environment
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 세 화면비는 같은 실행파일로 순차 확인하며16:9만 전체화면을 사용한다.
SIZES = {'1024x768': (1024, 768, False), '1280x720': (1280, 720, True), '1280x800': (1280, 800, False)}
OWNER_ITEM = 'Hide Island Ownership - F7'  # 원본 스타일 종료표시~.를 제외한 실제 View 행 문자열이다.


def run(output, data, width, height, full, steps, frames, mission=None):
    """CLI 초기 설정과 정상 UI 입력만 사용하며 원본·설정·현재 파일을 전후 비교한다."""
    output.mkdir(parents=True, exist_ok=False)
    script, report = output/'steps.tsv', output/'states.tsv'
    script.write_bytes(''.join(f'{f}\t{op}\t{arg}\n' for f, op, arg in steps).encode('utf-8'))
    command = [str(GAME), '--run', str(data), '--mute-audio', '--frames', str(frames), '--set',
               f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0;useBanners=1;music=0;playWind=0',
               '--ui-script', str(script), '--ui-report', str(report)]
    if mission:
        command.extend(['--mission', mission])
    if full:
        command[command.index('--set')+1] += ';startInFullScreen=1'
    else:
        command.append('--window')
    original, before = snapshots(ROOT/'originals'), snapshots(data)
    save(output/'originals-before.json', original)
    save(output/'data-before.json', before)
    result = {'command': command, 'binarySha256': sha(GAME), 'scriptSha256': sha(script)}
    try:
        with preserve_game_settings(data, 'd', output/'settings-backup'), muted_device_environment():
            result.update(execute(command, output, GAME, 120))
    except Exception as error:
        result['exception'] = repr(error)
        raise
    finally:
        result.update(originalsUnchanged=snapshots(ROOT/'originals') == original, dataUnchanged=snapshots(data) == before)
        save(output/'run.json', result)
    assert result['returnCode'] == 0 and not result['timedOut'], result
    assert result['originalsUnchanged'] and result['dataUnchanged']
    return states(report), result


def tables(row, enabled):
    """실제5319c0/557938과 다섯 표/고정 세 표의 현재 존재를 확인한다."""
    values = list(map(int, row['raw_owner_tables']))
    assert values[:2] == [enabled, 1], values
    assert values[2:5] == [0, 0, 0], values
    assert all(values[7:10]), values
    assert all(values[5:7]) if enabled == 0 else values[5:7] == [0, 0], values


def mission(output, width, height, full, source):
    """캠페인1-1의 모달·수정키·정지·같은 View 행을 실제 버튼/키로 확인한다."""
    data = output/'data'
    shutil.copytree(source, data)  # 다른 읽기 검사와 options.cfg 매핑을 공유하지 않는 실행 사본이다.
    steps = [(5, 'report', 'brief'), (10, 'key', 'f7'), (20, 'report', 'blocked'),
             (25, 'click', 'Play Mission'), (35, 'report', 'initial'),
             (40, 'snapshot', str(output/'on.bmp')), (45, 'key', 'f7'), (55, 'report', 'off'),
             (55, 'snapshot', str(output/'off.bmp')), (60, 'keyup', 'f7'), (70, 'report', 'released'),
             (75, 'key', 'ctrl+f7'), (85, 'report', 'control'), (90, 'key', 'alt+f7'), (100, 'report', 'alt'),
             (105, 'key', 'f7'), (115, 'report', 'restored'), (120, 'key', 'shift+f9'), (130, 'report', 'paused'),
             (135, 'key', 'f7'), (145, 'report', 'pausedF7'), (150, 'keyup', 'f7'), (155, 'key', 'shift+f9'),
             (165, 'report', 'resumed'), (170, 'esc', ''), (180, 'click', 'View'), (185, 'report', 'view'),
             (190, 'click', OWNER_ITEM), (200, 'report', 'menuOff'), (205, 'esc', ''), (210, 'click', 'View'),
             (215, 'click', OWNER_ITEM), (225, 'report', 'menuOn'), (235, 'key', 'f8'),
             (245, 'report', 'review'), (250, 'key', 'f7'), (260, 'report', 'reviewF7'),
             (265, 'click', 'Play Mission'), (275, 'report', 'continued'), (280, 'esc', ''), (285, 'click', 'Game'),
             (290, 'click', 'Leave Mission'), (300, 'report', 'leave'), (310, 'click', 'Main Menu'), (320, 'report', 'main')]
    observed, result = run(output/'run', data, width, height, full, steps, 330, 'thewarbegins')
    for label in ('brief', 'blocked', 'initial', 'control', 'restored', 'paused', 'pausedF7', 'resumed', 'menuOn', 'review', 'reviewF7', 'continued'):
        # 모달·뗌·정지 거부와 정상 두 번의 논리 반전 뒤에는 같은 현재 값1이다.
        tables(observed[label], 1)
    for label in ('off', 'released', 'alt', 'menuOff'):
        # 일반 F7과 같은 메뉴 쓰기 모두 같은94/97표 두 개를 등록한다.
        tables(observed[label], 0)
    for first, second in (('brief', 'blocked'), ('review', 'reviewF7')):
        # 열린 안내를 새 모달로 겹치지 않으며 같은 serial과 자기 정지를 보존한다.
        assert observed[first]['raw_html_root'][:2] == observed[second]['raw_html_root'][:2]
        assert observed[second]['paused'] == ['1']
    assert observed['paused']['time'][0] == observed['pausedF7']['time'][0]
    assert observed['main']['phase'] == ['0'] and observed['main']['state_hud'][0] == '0'
    for label, row in observed.items():
        # 같은 창 크기·현재 Session/HUD로 전환하며 직접 상태 변경은 하지 않는다.
        assert row['size'] == [str(width), str(height)] and bool(int(row['screen_mode'][0]) & 1) == full
        if label not in ('leave', 'main'):
            assert row['raw_session'] == ['1', '1'] and row['raw_uber'][:3] == observed['initial']['raw_uber'][:3]
    summary = {'size': [width, height], 'fullscreen': full, 'normalControlAltF7': True,
               'sameViewOption': True, 'pausedAndModalGates': True, 'currentSurfaceMaps': True, 'run': result}
    save(output/'summary.json', summary)
    return summary


def editor(output, source, width, height, full):
    """사본 TEST02를 새로 열고 전투 배너 없이 지면 표/옵션을 정상 전환한다."""
    data = output/'data'
    shutil.copytree(source, data)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (10, 'click', 'Edit'), (20, 'click', 'TEST02'), (55, 'report', 'initial'),
             (60, 'snapshot', str(output/'editor-on.bmp')), (65, 'key', 'f7'), (75, 'report', 'off'),
             (75, 'snapshot', str(output/'editor-off.bmp')), (80, 'keyup', 'f7'), (90, 'report', 'released'),
             (95, 'key', 'ctrl+f7'), (105, 'report', 'restored')]
    observed, result = run(output/'run', data, width, height, full, steps, 115)
    tables(observed['initial'], 1)
    tables(observed['off'], 0)
    tables(observed['released'], 0)
    tables(observed['restored'], 1)
    for row in observed.values():
        # 같은 Edit mode/Session/HUD를 유지하고 실제 데이터 파일을 저장하지 않는다.
        assert row['raw_world_modes'] == ['1', '1'] and row['raw_session'] == ['1', '1'] and row['paused'] == ['0']
        assert row['raw_uber'][:3] == observed['initial']['raw_uber'][:3]
        assert row['size'] == [str(width), str(height)] and bool(int(row['screen_mode'][0]) & 1) == full
    summary = {'size': [width, height], 'fullscreen': full, 'surfaceMapsToggle': True,
               'sameEditorHudAndSession': True, 'run': result}
    save(output/'summary.json', summary)
    return summary


def main():
    """같은 최종 제품의 세 화면비 캠페인과 선택한 사본 Editor를 순차 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', choices=SIZES, action='append')
    parser.add_argument('--data-source', type=Path, default=ROOT/'originals')
    parser.add_argument('--editor-data-root', type=Path)
    parser.add_argument('--editor-only', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    binary, games, editors = sha(GAME), [], []
    if args.editor_only and not args.editor_data_root:
        parser.error('--editor-only는 --editor-data-root가 필요합니다')
    if not args.editor_only:
        for name in args.case or SIZES:
            # 각 실행이 끝난 뒤에만 다음 게임을 시작하고 같은 바이너리를 유지한다.
            games.append(mission(output/name, *SIZES[name], args.data_source.resolve()))
            assert sha(GAME) == binary
    if args.editor_data_root:
        for name in args.case or SIZES:
            # 편집도 원본과 독립된 지정 샘플의 새 사본만 사용한다.
            editors.append(editor(output/('editor-'+name), args.editor_data_root.resolve(), *SIZES[name]))
            assert sha(GAME) == binary
    save(output/'summary.json', {'results': games, 'editors': editors, 'binarySha256': binary, 'directWorldMutation': False})
    print('배너/F7 실제 확인:', len(games)+len(editors), '게임 통과')


if __name__ == '__main__':
    main()
