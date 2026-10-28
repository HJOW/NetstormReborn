#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Shift F7의 실제 저장·파일 충돌·겹친 안내·수동 정지·메인 복귀를 짧게 확인한다."""
import argparse
import json
import shutil
from pathlib import Path
from PIL import Image
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_game_menu_smoke import muted_device_environment
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 같은 최종 실행파일로 네이티브4:3·16:9전체화면·16:10창을 순차 실행한다.
SIZES = {'1024x768': (1024, 768, False), '1280x720': (1280, 720, True), '1280x800': (1280, 800, False)}


def one(output, size):
    """원본은 읽기 전용으로 보존하고 각 실행의 데이터와 저장 경로를 분리한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output/'data'
    shutil.copytree(ROOT/'originals', data)
    pictures = data/'ScreenShot'  # 원본 설치 기본 이름공간을 실제로 사용하며 드라이브 루트에 저장하지 않는다.
    pictures.mkdir()
    occupied = pictures/'pic1.gif'
    occupied.write_bytes(b'original occupied screenshot sentinel')
    original, initial_data = snapshots(ROOT/'originals'), snapshots(data)
    width, height, fullscreen = SIZES[size]
    steps = [(5, 'report', 'brief'), (10, 'key', 'shift+f7'), (20, 'report', 'briefShot'),
             (25, 'click', 'OK'), (35, 'report', 'briefRestored'), (40, 'click', 'Play Mission'),
             (50, 'report', 'initial'), (55, 'snapshot', str(output/'before.bmp')), (55, 'key', 'shift+f7'),
             (65, 'report', 'shot2'), (70, 'keyup', 'shift+f7'), (75, 'key', 'shift+f7'),
             (85, 'report', 'nested'), (85, 'snapshot', str(output/'modal.bmp')),
             (90, 'click', 'OK'), (100, 'report', 'shotRestored'), (105, 'click', 'OK'),
             (115, 'report', 'back'), (120, 'key', 'shift+f9'), (130, 'report', 'paused'),
             (135, 'key', 'shift+f7'), (145, 'report', 'pausedShot'), (150, 'click', 'OK'),
             (160, 'report', 'stillPaused'), (165, 'key', 'shift+f9'), (175, 'report', 'resumed'),
             (180, 'key', 'alt+shift+f7'), (190, 'report', 'altShot'), (195, 'click', 'OK'),
             (205, 'report', 'continued'), (210, 'esc', ''), (215, 'click', 'Game'),
             (220, 'click', 'Leave Mission'), (230, 'report', 'leave'), (235, 'click', 'Main Menu'),
             (245, 'report', 'main'), (250, 'key', 'shift+f7'), (260, 'report', 'mainShot'),
             (265, 'click', 'OK'), (275, 'report', 'mainBack')]
    script, report = output/'steps.tsv', output/'states.tsv'
    script.write_text(''.join(f'{frame}\t{action}\t{argument}\n' for frame, action, argument in steps), encoding='utf-8', newline='\n')
    settings = (f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;'
                'useProductTimers=0;useBanners=1;music=0;playWind=0')
    if fullscreen:
        settings += ';startInFullScreen=1'
    command = [str(GAME), '--run', str(data), '--mute-audio', '--frames', '285', '--set', settings,
               '--mission', 'thewarbegins', '--ui-script', str(script), '--ui-report', str(report)]
    if not fullscreen:
        command.append('--window')
    result = dict(command=command, binarySha256=sha(GAME), scriptSha256=sha(script), screenshotDirectory=str(pictures))
    try:
        with preserve_game_settings(data, 'd', output/'settings-backup'), muted_device_environment():
            result.update(execute(command, output, GAME, 120))
    finally:
        after_data = snapshots(data)
        created = set(after_data)-set(initial_data)
        allowed = {str(Path('ScreenShot')/f'pic{number}.gif') for number in (0, 2, 3, 4, 5, 6)}
        result.update(originalsUnchanged=original == snapshots(ROOT/'originals'),
                      dataUnchanged=all(after_data.get(name) == digest for name, digest in initial_data.items()) and created.issubset(allowed),
                      createdFiles=sorted(created))
        save(output/'run.json', result)
    assert result['returnCode'] == 0 and not result['timedOut'], result
    assert result['originalsUnchanged'] and result['dataUnchanged']
    return verify(output, size, result)


def verify(output, size, result):
    """보존된 실제 보고서와 파일만 재판독하며 입력·기대값·게임 상태를 바꾸지 않는다."""
    width, height, fullscreen = SIZES[size]
    pictures, report = Path(result.get('screenshotDirectory', str(output/'pictures'))), output/'states.tsv'
    assert result['returnCode'] == 0 and not result['timedOut'] and result['originalsUnchanged'] and result['dataUnchanged']
    observed = states(report)
    assert observed['brief']['raw_html_root'][:2] == observed['briefRestored']['raw_html_root'][:2]
    assert observed['shot2']['raw_html_root'][:2] == observed['shotRestored']['raw_html_root'][:2]
    for label in ('briefShot', 'shot2', 'nested', 'pausedShot', 'altShot'):
        # 저장 완료 안내는 console 관문보다 앞이며 같은 배너 설정을 유지한다.
        assert observed[label]['paused'] == ['1'], (label, observed[label].get('paused'))
        assert observed[label]['raw_owner_tables'][0] == '1'
    assert observed['back']['paused'] == ['0'] and observed['continued']['paused'] == ['0']
    assert observed['paused']['time'][0] == observed['pausedShot']['time'][0] == observed['stillPaused']['time'][0]
    assert observed['stillPaused']['paused'] == ['1'] and observed['resumed']['paused'] == ['0']
    assert observed['main']['phase'] == ['0'] and observed['mainBack']['phase'] == ['0']
    assert observed['mainShot']['page'] == ['TellGifWritten'] and observed['mainShot']['paused'] == ['0']
    assert (pictures/'pic1.gif').read_bytes() == b'original occupied screenshot sentinel'
    names = sorted(path.name for path in pictures.glob('*.gif'))
    assert names == [f'pic{index}.gif' for index in range(7)], names
    images = []
    for number in (0, 2, 3, 4, 5, 6):
        # 별도 Pillow GIF 해독기로 실제 치수·단일 프레임·8비트 인덱스·색표를 검사한다.
        path = pictures/f'pic{number}.gif'
        with Image.open(path) as image:
            image.load()
            assert image.format == 'GIF' and image.mode == 'P' and image.size == (width, height) and image.n_frames == 1
            assert len(image.getpalette()) == 768
            image.convert('RGB').save(output/f'pic{number}.png')
            images.append(dict(name=path.name, sha256=sha(path), bytes=path.stat().st_size, colors=len(set(image.tobytes()))))
    summary = dict(size=[width, height], fullscreen=fullscreen, run=result, files=images,
                   collisionPreserved=True, nestedModalRestored=True, manualPausePreserved=True,
                   mainMenuCounterPreserved=True, originalGameRuns=0, naturalClears=0, multiplayerTests=0)
    save(output/'summary.json', summary)
    return summary


def main():
    """선택한 화면비를 같은 최종 바이너리에서 순차 실행하고 실패 원시를 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', choices=SIZES, action='append')
    parser.add_argument('--verify-existing', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    if not args.verify_existing:
        output.mkdir(parents=True, exist_ok=False)
    results = []
    for size in args.case or SIZES:
        # 앞 게임을 정상 종료한 뒤 다음 독립 사본으로 이동한다.
        if args.verify_existing:
            results.append(verify(output/size, size, json.loads((output/size/'run.json').read_text(encoding='utf-8'))))
        else:
            results.append(one(output/size, size))
    save(output/'summary.json', dict(results=results, binarySha256=sha(GAME)))
    print('Shift F7 저장/안내/충돌/정지/메인 복귀 확인 완료')


if __name__ == '__main__':
    main()
