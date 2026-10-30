#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""별도 배포본의 세 화면비에서 같은 논리 pan 입력의 창/확대 전체화면 이동량을 대조한다."""
import argparse
import zipfile
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, execute, save, sha
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from nscfg import encode_text

SIZES = ((1024, 768), (1280, 720), (1280, 800))  # 원본4:3과 추가16:9/16:10을 같은 물리 모니터에서 비교한다.


def run(product, output, width, height, fullscreen):
    """일반 폴링/키 큐만 공급하고 두 축 이동·해제·F2 소비·실제 창 RGB를 관측한다."""
    output.mkdir(parents=True, exist_ok=False)
    binary, data = product/'Netstorm.exe', product/'game'  # 원본 실행파일은 자산으로만 읽으며 클론 경로만 실행한다.
    released = f'{0x80010000},{width//2},{height//2}'  # 검사 포인터의 뗌 상태이며 게임 규칙이나 카메라 좌표 쓰기가 아니다.
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Play Mission'), (18, 'pointer', released), (20, 'report', 'baseline'),
        (25, 'pointer', f'{0x120000},{width*3//4},{height//2}'), (26, 'report', 'alt1'), (28, 'key', 'f2'), (36, 'report', 'alt2'),
        (40, 'pointer', released), (41, 'report', 'released'), (48, 'report', 'idle'),
        (50, 'pointer', f'{0x40000},{width//2},{height//4}'), (51, 'report', 'middle1'), (53, 'key', 'f2'), (61, 'report', 'middle2'),
        (65, 'pointer', released), (66, 'report', 'middle-stop'), (68, 'key', 'f2'), (69, 'report', 'key-works'),
        (72, 'key', 'f2'), (73, 'report', 'key-restored'), (80, 'report', 'final'),
        (80, 'snapshot', str(output/'final.bmp')), (80, 'snapshot-output', str(output/'final-output.bmp'))]  # 일반 Alt 가로/가운데 세로 입력을 같은 시점에 공급한다.
    plan = output/'steps.tsv'
    plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame, op, arg in steps), encoding='utf-8', newline='\n')
    command = [str(binary), '--run', str(data), '--mission', 'TheWarBegins', '--no-audio', '--frames', '82', '--set',
        f'SCREENW={width};SCREENH={height};maxFPS=40;sleepPerLoop=1;edgeScroll=0;scrollWhileMouseOut=0;autoDemo=0;tellTips=0;startInFullScreen=1;workingFullScreenFlags=11',
        '--ui-script', str(plan), '--ui-report', str(output/'states.tsv')]  # 시각/월드/승패는 일반 프레임에서 진행한다.
    if not fullscreen:
        command.append('--window')
    with preserve_game_settings(data, 'd', output/'settings-backup'):
        (data/'d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
        result = execute(command, output, binary, 120)
    save(output/'run.json', result)
    assert result['returnCode'] == 0 and not result['timedOut'], result
    observed = states(output/'states.tsv')
    camera = {label: list(map(int, scene['raw_camera_frame'][:7])) for label, scene in observed.items()}  # 현재 위치/목표/요청 카운터는 관측 전용이다.
    for label in ('baseline', 'alt1', 'alt2', 'released', 'idle', 'middle1', 'middle2', 'middle-stop', 'final'):  # 같은 실제 로컬owner/PlayerKnown/활성 창과 비정지를 확인한다.
        scene = observed[label]
        assert scene['phase'] == ['4'] and scene['raw_session'] == ['1', '1'] and scene['paused'] == ['0'] and scene['mouse_cursor'][1] == '1', label
    changes = {}
    for kind in ('alt', 'middle'):  # 두 축의 전체 이동 벡터와 원문 요청10회를 각각 비교할 수 있도록 보존한다.
        first, last = camera[kind+'1'], camera[kind+'2']
        changes[kind] = {'delta': [last[axis]-first[axis] for axis in (0, 1)], 'requests': (last[5]-first[5]) & 0xffffffff}
        assert changes[kind]['requests'] == 10, changes
    assert changes['alt']['delta'][0] > 0 and changes['middle']['delta'][1] < 0, changes
    assert camera['released'][:4] == camera['idle'][:4]
    assert camera['middle-stop'][:4] == camera['final'][:4]
    hide = {name: int(scene['raw_view_keys'][0]) for name, scene in observed.items()}  # 이동 중 일반 F2가 소비되고 해제 후 정상 처리되는지 읽는다.
    assert hide['baseline'] == hide['alt2'] == hide['middle2'] == hide['key-restored'] and hide['key-works'] == int(not hide['baseline'])
    final = observed['final']
    assert final['size'] == [str(width), str(height)] and bool(int(final['screen_mode'][0]) & 1) == fullscreen
    presentation = verify_presentation(output/'final.bmp', output/'final-output.bmp', final, output)
    assert presentation['same'] and float(final['time'][0]) > float(observed['baseline']['time'][0])
    summary = {'passed': True, 'logicalSize': [width, height], 'fullscreen': fullscreen, 'changes': changes, 'cameraFrames': camera,
        'window': final['screen_window'], 'output': final['screen_output'], 'presentation': presentation, 'run': result,
        'scope': '실제 클론 창/검사 폴링이다. 물리 Alt/가운데·실제 앱 포커스 전환·Win11/PCM·포획/희생/특정 공격/승리·신규MP 검사는 아니다.'}
    save(output/'summary.json', summary)
    return summary


def main():
    """새 ZIP의 모든 자료를 원천과 대조하고 여섯 실행 후 원래 배포 바이트 보존까지 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    product = output/'한글 배포본/NetstormReborn-cpppj'  # 한글 설치 경로와 원본 자산 보호를 함께 유지한다.
    with zipfile.ZipFile(args.package) as archive:
        archive.extractall(output/'한글 배포본')
        entries = archive.infolist()
        assets = 0
        for entry in entries:  # 기존 모든 ZIP/추출 바이트와 게임 자료의 원본/CD 바이트를 확인한다.
            assert not entry.is_dir()
            content = archive.read(entry)
            relative = entry.filename.removeprefix('NetstormReborn-cpppj/')
            assert (product/relative).read_bytes() == content
            if relative.startswith('game/'):
                asset = relative.removeprefix('game/')
                source = ROOT/('originalCD' if asset.startswith('MOVIE/') else 'originals')/asset
                assert source.read_bytes() == content, asset
                assets += 1
        assert sha(product/'Netstorm.exe') == sha(ROOT/'cpppj/build/bin/Release/Netstorm.exe')
        results = []
        for width, height in SIZES:  # 한 창의 종료/설정 복구가 끝난 뒤 같은 입력의 다음 모드를 연다.
            pair = [run(product, output/f'{width}x{height}-{mode}', width, height, mode == 'fullscreen') for mode in ('window', 'fullscreen')]
            assert pair[0]['changes'] == pair[1]['changes'], pair
            results.extend(pair)
        for entry in entries:  # 설정 복구를 포함해 실행 후 모든 배포 파일 바이트가 같은지 확인한다.
            assert (output/'한글 배포본'/entry.filename).read_bytes() == archive.read(entry), entry.filename
    save(output/'summary.json', {'passed': True, 'runs': len(results), 'entries': len(entries), 'gameAssets': assets,
        'binarySha256': sha(product/'Netstorm.exe'), 'zipSha256': sha(args.package), 'results': results,
        'windowFullscreenPanEqual': True, 'allArchivedBytesUnchanged': True, 'protectedActionsReproduced': False, 'networkExecuted': False})
    print('세 화면비의 창/확대 전체화면 pan 이동량·해제·키 소비·출력 6회 통과')


if __name__ == '__main__':
    main()
