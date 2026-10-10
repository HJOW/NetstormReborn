#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 소리·음악 묶음을 클론 창(NetstormCpp --run)에 실제로 붙여 검사한다. 원본 게임은 실행하지 않는다.

모든 실행은 `--mute-audio`다: 실제 DirectSound 장치를 열고 원본 음악 파일을 실제로 스트리밍하지만 음량은 -10000(무음)이다.
종료 직전의 상태를 `--audio-report`로 받아 청취 없이 확인한다.

1. 메뉴: 장치·음악 스레드가 열리고 ser22.mus가 활성이며 곡 길이가 203.9초 안팎이고 장치 커서가 전진한다.
2. 전투: 미션 진입 시 원소 곡 하나와(천둥 곡이면 천둥 효과음) 전투 장면이 시작된다.
3. 옵션 메뉴의 버튼(실제 마우스 사건)으로 음악 끄기·켜기, 소리 끄기·켜기, 음량 단계 변경이 장치와 곡에 반영된다.
4. 설정 sound=0;music=0이면 곡이 시작되지 않는다. 환경 변수 NETSTORM_CPP_NO_AUDIO이면 소리 묶음 자체가 없다.
5. 원본 파일(옵션·전체화면 표시 파일 제외)이 바뀌지 않았는지 확인한다.

창이 몇 초씩 뜬다(시나리오당 3~10초). 설정 파일은 실행 전후로 보관·복원한다.

python tools/cpp_audio_smoke.py
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

from cpp_smoke_files import preserve_game_settings

# 저장소 루트와 검사 산출물 폴더(Git 제외).
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-audio-smoke'
# 음량 단계 4·1에 대응하는 장치 음량(원본 점프 표 004359d0). 메뉴 한 칸을 고른 결과를 비교한다.
STEP_VOLUME = {1: -4000, 2: -2000, 3: -1000, 4: -500}
# ser22.mus의 곡 길이(초)와 허용 오차. 데이터 17,988,096바이트 ÷ 88,200바이트/초.
MENU_SONG_SECONDS = 203.947
# 링 버퍼 한 번 분량(바이트). 읽은 위치가 이 값을 넘으면 스레드가 장치 커서를 따라 다시 채웠다는 뜻이다.
RING_BYTES = 0x2af80


def snapshots(root):
    """원본 폴더의 모든 파일 해시. 허용된 두 설정 파일은 제외한다."""
    skipped = {'options.cfg', 'fullscreenStateFile.dat', 'fullscreenstatefile.dat'}
    result = {}
    # 하위 폴더까지 모든 파일을 읽어 해시한다.
    for path in sorted(root.rglob('*')):
        if path.is_file() and path.name.lower() not in {name.lower() for name in skipped}:
            result[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return result


def run(executable, root, name, frames, *, extra=(), script=None, settings='', silent_env=False, audio=True):
    """한 시나리오를 실행하고 소리 보고를 사전으로 돌려준다. 보고가 없으면 None이다."""
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = OUTPUT / f'{name}.txt'
    if report.exists():
        report.unlink()
    arguments = [str(executable), '--run', str(root), '--window', '--frames', str(frames), '--audio-report', str(report)]
    if audio:
        arguments.append('--mute-audio')
    arguments += ['--set', 'SCREENW=1024;SCREENH=768;sleepPerLoop=10;autoDemo=0;tellTips=0' + (';' + settings if settings else '')]
    if script:
        path = OUTPUT / f'{name}-steps.tsv'
        # 클릭마다 다음 프레임에 report를 넣는다. 다음 클릭이 찾는 컨트롤 목록은 report가 만든 화면 구성을 기준으로 갱신된다.
        expanded = []
        # 각 단계를 옮기며 클릭 단계 뒤에 report를 붙인다.
        for frame, operation, argument in script:
            expanded.append((frame, operation, argument))
            if operation == 'click':
                expanded.append((frame + 1, 'report', f'after{frame}'))
        path.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in expanded), encoding='utf-8', newline='\n')
        arguments += ['--ui-script', str(path), '--ui-report', str(OUTPUT / f'{name}-states.tsv')]
    arguments += list(extra)
    environment = dict(os.environ)
    # 이 도구는 소리를 일부러 검사하므로 공용 도우미가 켠 무음 환경 변수를 지운다(silent_env일 때만 다시 켠다).
    environment.pop('NETSTORM_CPP_NO_AUDIO', None)
    if silent_env:
        environment['NETSTORM_CPP_NO_AUDIO'] = '1'
    # 옵션 메뉴가 바꾼 설정은 클라이언트가 options.cfg에 저장하므로, 시나리오마다 보관했다가 되돌려 다음 시나리오가 같은 처음 상태에서 시작하게 한다.
    with preserve_game_settings(root, 'd'):
        result = subprocess.run(arguments, env=environment, capture_output=True, text=True, timeout=180)
    assert result.returncode == 0, (name, result.returncode, result.stdout[-400:], result.stderr[-400:])
    if not report.exists():
        return None
    values = {}
    # `이름\t값` 줄을 읽는다.
    for line in report.read_text(encoding='utf-8').splitlines():
        key, _, value = line.partition('\t')
        values[key] = value
    return values


def number(values, key):
    """보고 값을 실수로 읽는다."""
    return float(values[key])


def check_menu(executable, root):
    """메뉴에서 장치·스트림이 실제로 돌아가는지 검사한다."""
    # 약 10초: 링 버퍼(약 2초)를 여러 번 비우고 채울 만큼 돌린다.
    values = run(executable, root, 'menu', 700)
    assert values['deviceReady'] == '1' and values['soundEnabled'] == '1' and values['musicRuntime'] == '1', values
    assert values['musicName'] == 'ser22.mus' and values['musicActive'] == '1', values
    assert abs(number(values, 'musicDuration') - MENU_SONG_SECONDS) < 0.01, values['musicDuration']
    assert values['sceneBattle'] == '0' and values['sceneIndex'] == '3', values
    # 음소거: 효과음·음악 음량이 모두 -10000이고 깊이가 1이다.
    assert values['masterVolume'] == '-10000' and values['musicVolume'] == '-10000' and values['muteDepth'] == '1', values
    # 장치가 재생 중(비트 1)이고 반복 재생(비트 4)이다.
    assert int(values['musicBufferStatus']) & 5 == 5, values['musicBufferStatus']
    # 읽은 위치가 링 버퍼 한 번 분량을 넘어야 장치 커서를 따라 스레드가 다시 채운 것이다.
    assert number(values, 'musicReadOffset') > RING_BYTES * 2, values['musicReadOffset']
    assert number(values, 'musicPlayCursor') > 0 and values['workerFailed'] == '0', values
    # 곡 끝 시각은 시작 시각 + 곡 길이(30초보다 긴 곡)다.
    assert abs(number(values, 'songEnd') - number(values, 'musicDuration')) < 3, values
    return {key: values[key] for key in ('musicName', 'musicDuration', 'musicReadOffset', 'musicBufferStatus', 'songEnd')}


def check_battle(executable, root):
    """미션 진입 시 전투 장면과 원소 곡이 시작되는지 검사한다."""
    # 게임 시계가 멈춘 구간은 프레임 제한이 풀리므로 프레임당 대기(sleepPerLoop)로 시간을 확보한다.
    values = run(executable, root, 'battle', 800, extra=['--mission', 'thewarbegins'])
    assert values['sceneBattle'] == '1' and values['musicActive'] == '1' and values['workerFailed'] == '0', values
    index = int(values['sceneIndex'])
    names = ['wind22.mus', 'rain22.mus', 'thu22.mus', 'sun22.mus']
    # 시작 직후 Frame이 곡 끝을 보지 않으므로 고른 색인의 다음 곡(색인은 이미 한 번 증가)이다.
    assert 0 <= index <= 3 and values['musicName'] == names[index], values
    assert int(values['musicBufferStatus']) & 5 == 5 and number(values, 'musicReadOffset') >= RING_BYTES, values
    # 천둥 곡이면 천둥 효과음을 한 번 냈다(짧은 소리라 보고 시점에는 이미 끝났을 수 있으므로 재생 횟수로 본다). 다른 곡이면 효과음이 없다.
    assert (int(values['soundSerial']) > 0) == (index == 2), values
    return {key: values[key] for key in ('musicName', 'sceneIndex', 'musicDuration', 'musicReadOffset', 'soundSerial')}


def check_options(executable, root):
    """옵션 메뉴의 실제 버튼으로 음악·소리·음량을 바꾼다."""
    result = {}
    # 음악 끄기: 곡 이름은 남고 채널은 멈춘다.
    off = run(executable, root, 'music-off', 250, script=[(3, 'click', 'Options'), (5, 'click', 'Play Music')])
    assert off['musicOption'] == '0' and off['musicActive'] == '0' and off['musicName'] == 'ser22.mus', off
    assert off['deviceReady'] == '1' and off['musicRuntime'] == '1', off
    result['music_off'] = {'musicOption': off['musicOption'], 'musicActive': off['musicActive']}
    # 음악 끄고 켜기: 같은 곡이 다시 시작해 스트림이 전진한다. 클릭은 두 프레임 간격으로 둔다(누름과 뗌이 각각 한 프레임이다).
    on = run(executable, root, 'music-on', 450,
             script=[(3, 'click', 'Options'), (5, 'click', 'Play Music'), (7, 'click', 'Options'), (9, 'click', 'Play Music')])
    assert on['musicOption'] == '1' and on['musicActive'] == '1' and on['musicName'] == 'ser22.mus', on
    assert int(on['musicBufferStatus']) & 5 == 5 and number(on, 'musicReadOffset') >= RING_BYTES, on
    result['music_on_again'] = {'musicReadOffset': on['musicReadOffset'], 'musicBufferStatus': on['musicBufferStatus']}
    # 소리 끄기: 음악 스레드를 합류하고 장치를 닫는다.
    sound_off = run(executable, root, 'sound-off', 250, script=[(3, 'click', 'Options'), (5, 'click', 'Sound On')])
    assert sound_off['soundEnabled'] == '0' and sound_off['deviceReady'] == '0' and sound_off['musicRuntime'] == '0', sound_off
    assert sound_off['musicActive'] == '0' and sound_off['workerFailed'] == '0', sound_off
    result['sound_off'] = {key: sound_off[key] for key in ('soundEnabled', 'deviceReady', 'musicRuntime', 'musicActive')}
    # 소리 끄고 켜기: 장치를 다시 열고 같은 곡을 다시 시작한다.
    sound_on = run(executable, root, 'sound-on', 450,
                   script=[(3, 'click', 'Options'), (5, 'click', 'Sound On'), (7, 'click', 'Options'), (9, 'click', 'Sound On')])
    assert sound_on['soundEnabled'] == '1' and sound_on['deviceReady'] == '1' and sound_on['musicRuntime'] == '1', sound_on
    assert sound_on['musicActive'] == '1' and int(sound_on['musicBufferStatus']) & 5 == 5, sound_on
    assert number(sound_on, 'musicReadOffset') >= RING_BYTES and sound_on['workerFailed'] == '0', sound_on
    result['sound_on_again'] = {key: sound_on[key] for key in ('deviceReady', 'musicRuntime', 'musicActive', 'musicReadOffset')}
    # 음량 단계: 음소거 중이므로 현재 음량은 그대로이고 예약값이 단계 표 값으로 바뀐다.
    volume = run(executable, root, 'volume', 200, script=[
        (3, 'click', 'Options'), (5, 'click', 'Music Volume >'), (7, 'click', 'Volume 4'),
        (9, 'click', 'Options'), (11, 'click', 'Sound Effect Volume >'), (13, 'click', 'Volume 1')])
    assert volume['masterVolume'] == '-10000' and volume['musicVolume'] == '-10000' and volume['muteDepth'] == '1', volume
    assert int(volume['pendingMusicVolume']) == STEP_VOLUME[4] and int(volume['pendingMasterVolume']) == STEP_VOLUME[1], volume
    result['volume'] = {key: volume[key] for key in ('pendingMusicVolume', 'pendingMasterVolume', 'muteDepth')}
    return result


def check_disabled(executable, root):
    """설정으로 소리·음악이 꺼진 실행과 무음 환경 변수 실행을 검사한다."""
    result = {}
    # 원본은 소리 옵션과 무관하게 장치를 연다. 곡은 음악 옵션이 켜져 있을 때만 시작한다.
    disabled = run(executable, root, 'disabled', 120, settings='sound=0;music=0')
    assert disabled['soundEnabled'] == '0' and disabled['musicOption'] == '0' and disabled['musicActive'] == '0', disabled
    assert disabled['deviceReady'] == '1' and disabled['workerFailed'] == '0', disabled
    result['sound0_music0'] = {key: disabled[key] for key in ('soundEnabled', 'musicOption', 'musicActive', 'deviceReady')}
    # 무음 환경 변수: 소리 묶음을 만들지 않으므로 보고가 없다(실행은 정상 종료).
    silent = run(executable, root, 'silent-env', 30, silent_env=True, audio=False)
    assert silent is None
    result['silent_env'] = 'no report, exit 0'
    # --no-audio 옵션도 같다.
    no_audio = run(executable, root, 'no-audio', 30, extra=['--no-audio'], audio=False)
    assert no_audio is None
    result['no_audio_option'] = 'no report, exit 0'
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--root', type=Path, default=ROOT / 'originals')
    arguments = parser.parse_args()
    root = arguments.root
    before = snapshots(root)
    report = {}
    try:
        with preserve_game_settings(root, 'd'):
            report['menu'] = check_menu(arguments.exe, root)
            report['battle'] = check_battle(arguments.exe, root)
            report['options'] = check_options(arguments.exe, root)
            report['disabled'] = check_disabled(arguments.exe, root)
    finally:
        assert snapshots(root) == before, '원본 파일의 바이트/존재 상태 변경'
    report['original_files_unchanged'] = len(before)
    import json
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
