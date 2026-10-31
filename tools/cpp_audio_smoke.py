#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 소리·음악 묶음을 클론 창(NetstormCpp --run)에 실제로 붙여 검사한다. 원본 게임은 실행하지 않는다.

소리 검사는 `--mute-audio`로 초기화한다. 실제 DirectSound 장치와 원본 음악 스트림의 상태를 확인한다.
소리 옵션 off/on은 원문 장치 재초기화에 따라 muteDepth를0으로 되돌릴 수 있으므로 전체 실행의 지속 무음을 보장하지 않는다.
종료 직전의 상태를 `--audio-report`로 받으며 청취/실제 PCM 출력은 이 도구의 검증 범위가 아니다.

1. 메뉴: 장치·음악 스레드가 열리고 ser22.mus가 활성이며 곡 길이가 203.9초 안팎이고 장치 커서가 전진한다.
2. 전투: 미션 진입 시 원소 곡 하나와(천둥 곡이면 천둥 효과음) 전투 장면이 시작된다.
3. 옵션 메뉴의 버튼(실제 마우스 사건)으로 음악 끄기·켜기, 소리 끄기·켜기, 음량 단계 변경이 장치와 곡에 반영된다.
4. 설정 sound=0;music=0이면 곡이 시작되지 않는다. 환경 변수 NETSTORM_CPP_NO_AUDIO이면 소리 묶음 자체가 없다.
5. 원본 파일(옵션·전체화면 표시 파일 제외)이 바뀌지 않았는지 확인한다.
6. 위치 효과음의 화면·카메라가 미션 진입/스크롤/홈/해상도 변경/메뉴 복귀/재진입과 같은 프레임에 일치한다.

창이 몇 초씩 뜬다(시나리오당 3~10초). 설정 파일은 실행 전후로 보관·복원한다.

python tools/cpp_audio_smoke.py
"""
import argparse
import hashlib
import os
from pathlib import Path
from contextlib import nullcontext
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


def after_startup(steps, frames):
    """일반 메인 시작의 실제 UserNotUpdated/Welcome 확인을 버튼으로 닫고 검사 프레임을 옮긴다."""
    offset = 16  # 안내의 누름/뗌과 공통 Cleanup이 끝날 프레임을 확보한다. UserUpdated 전역을 가짜로 쓰지 않는다.
    notices = [(4, 'click-if', 'OK'), (8, 'click-if', 'Ok'), (12, 'click-if', 'ok')]
    # 초기화면이 이미 메인이면 click-if는 효과가 없으며 미션 브리핑에는 이 보조를 사용하지 않는다.
    return notices + [(frame + offset, operation, argument) for frame, operation, argument in steps], frames + offset


def run(executable, root, name, frames, *, extra=(), script=None, settings='', silent_env=False, audio=True,
        screen_size=(1024, 768), preserve_settings=True):
    """한 시나리오의 실제 소리 보고를 읽고 보고가 없으면 None을 돌려준다.

    screen_size=None은 저장된 크기를 읽는 재시작에 사용한다. preserve_settings=False는
    전체 해상도 왕복을 바깥 preserve_game_settings가 보관할 때만 사용한다.
    """
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = OUTPUT / f'{name}.txt'
    if report.exists():
        report.unlink()
    if '--mission' not in extra:
        script, frames = after_startup(script or (), frames) # 첫 메인 안내만 실제 입력으로 닫으며 직접 미션 진입의 본문은 보존한다.
    arguments = [str(executable), '--run', str(root), '--window', '--frames', str(frames), '--audio-report', str(report)]
    if audio:
        arguments.append('--mute-audio')
    # 재시작 관측은 크기 덮어쓰기를 생략하여 실제 SETSIZE가 저장한 두 값을 읽는다.
    size_setting = '' if screen_size is None else f'SCREENW={screen_size[0]};SCREENH={screen_size[1]};'
    arguments += ['--set', size_setting + 'sleepPerLoop=10;autoDemo=0;tellTips=0' + (';' + settings if settings else '')]
    if script:
        path = OUTPUT / f'{name}-steps.tsv'
        # 클릭마다 다음 프레임에 report를 넣는다. 다음 클릭이 찾는 컨트롤 목록은 report가 만든 화면 구성을 기준으로 갱신된다.
        expanded = []
        # 각 단계를 옮기며 클릭 단계 뒤에 report를 붙인다.
        for frame, operation, argument in script:
            expanded.append((frame, operation, argument))
            if operation == 'click':
                expanded.append((frame + 1, 'report', f'after{frame}'))
        expanded.sort(key=lambda step: step[0])  # 같은 프레임 snapshot/report를 클릭 뒤 자동 관측이 추월하지 않게 한다.
        path.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in expanded), encoding='utf-8', newline='\n')
        arguments += ['--ui-script', str(path), '--ui-report', str(OUTPUT / f'{name}-states.tsv')]
    arguments += list(extra)
    environment = dict(os.environ)
    # 이 도구는 소리를 일부러 검사하므로 공용 도우미가 켠 무음 환경 변수를 지운다(silent_env일 때만 다시 켠다).
    environment.pop('NETSTORM_CPP_NO_AUDIO', None)
    if silent_env:
        environment['NETSTORM_CPP_NO_AUDIO'] = '1'
    # 옵션 메뉴가 바꾼 설정은 클라이언트가 options.cfg에 저장하므로, 시나리오마다 보관했다가 되돌려 다음 시나리오가 같은 처음 상태에서 시작하게 한다.
    # 명시 해상도 왕복만 바깥 보관자가 설정 수명을 소유한다. 기존 개별 검사는 종전처럼 매번 복구한다.
    with preserve_game_settings(root, 'd') if preserve_settings else nullcontext():
        result = subprocess.run(arguments, env=environment, capture_output=True, text=True, timeout=180)
    # 실패한 실행도 전체 진단을 이번 폴더에 남기며 마지막400글자의 예외 메시지와 구별한다.
    (OUTPUT / f'{name}.stdout.log').write_text(result.stdout, encoding='utf-8')
    (OUTPUT / f'{name}.stderr.log').write_text(result.stderr, encoding='utf-8')
    assert result.returncode == 0, (name, result.returncode, result.stdout[-400:], result.stderr[-400:])
    if not report.exists():
        return None
    values = {}
    # `이름\t값` 줄을 읽는다.
    for line in report.read_text(encoding='utf-8').splitlines():
        key, _, value = line.partition('\t')
        values[key] = value
    return values


def resolution_roundtrip(executable, root, name, target, *, initial_size=None, settings='', restart_steps=(), restart_frames=50):
    """원본 SETSIZE의 No 보존·Yes 저장/종료 뒤 크기 덮어쓰기 없는 새 프로세스를 관측한다.

    호출자는 전체 왕복을 preserve_game_settings 안에 두어야 한다. 이 함수는 저장 결과를 임의로 쓰지 않는다.
    """
    from cpp_menu_smoke import reports, button_labels
    from PIL import Image
    from nscfg import decode_file
    import re

    def saved_size():
        """암호화된 실제 저장 파일의 첫 SCREENW/H만 독립 reader로 읽는다."""
        text = decode_file(str(root / 'd/options.cfg'))  # C++ 재시작과 별도로 저장 바이트를 읽는 기존 판독기다.
        values = []  # 원본 Config의 첫 일치 두 정수만 모은다.
        for key in ('SCREENW', 'SCREENH'):  # 실제 옵션 순서대로 두 키를 읽는다.
            found = re.search(r'(?im)^[ \t]*' + key + r'[ \t]*=[ \t]*"?([+-]?\d+)"?', text)
            if not found:
                raise RuntimeError('해상도 저장 키 없음: ' + key)
            values.append(int(found[1]))
        return values

    before = saved_size()  # --set 시작 크기와 기존 저장 설정은 다를 수 있으므로 구별한다.
    title = f'{target[0]} by {target[1]}'  # 현재 실제 메뉴에 표시되는 원본 해상도 행이다.
    # 원본 경고를 No로 닫아 크기와 저장 설정이 바뀌지 않는지 먼저 관측한다.
    cancel_steps = [(5, 'report', 'before'), (10, 'click', 'Options'), (15, 'click', 'Resolution >'),
                    (20, 'click', title), (25, 'report', 'confirm'), (30, 'click', 'No'),
                    (40, 'report', 'cancelled')]
    run(executable, root, name + '-no', 42, script=cancel_steps, settings=settings,
        screen_size=initial_size, preserve_settings=False)
    cancelled = reports(OUTPUT / f'{name}-no-states.tsv')  # No는 실제 크기와 저장 설정 모두 보존해야 한다.
    assert cancelled['confirm']['page'][0].casefold() == 'setsize', cancelled['confirm']['page']
    assert {'Yes', 'No'} <= button_labels(cancelled['confirm'])
    assert cancelled['cancelled']['size'] == cancelled['before']['size']
    assert cancelled['cancelled']['phase'] == ['0'] and saved_size() == before
    # 같은 행을 다시 골라 Yes의 저장과 실제 OnExit 종료를 후행 보고 부재로 구별한다.
    accept_steps = [(5, 'report', 'before'), (10, 'click', 'Options'), (15, 'click', 'Resolution >'),
                    (20, 'click', title), (25, 'report', 'confirm'), (30, 'click', 'Yes'),
                    (60, 'report', 'unexpected-alive')]
    run(executable, root, name + '-yes', 65, script=accept_steps, settings=settings,
        screen_size=initial_size, preserve_settings=False)
    accepted = reports(OUTPUT / f'{name}-yes-states.tsv')  # Yes는 OnExit QuitApp99까지 실행하여 후행 프레임에 도달하면 안 된다.
    assert accepted['confirm']['page'][0].casefold() == 'setsize'
    assert 'unexpected-alive' not in accepted and saved_size() == list(target)
    steps = [(5, 'report', 'startup'), (5, 'snapshot', str(OUTPUT / f'{name}-startup.bmp')),
             *restart_steps]  # 재실행의 그림·미션 진입은 호출한 원래 검사의 관측 목적을 유지한다.
    run(executable, root, name + '-restart', restart_frames, script=sorted(steps, key=lambda step: step[0]),
        settings=settings, screen_size=None, preserve_settings=False)
    restarted = reports(OUTPUT / f'{name}-restart-states.tsv')  # 저장값으로 실제 새 장치를 만들었는지 관측한다.
    assert restarted['startup']['size'] == list(map(str, target))
    with Image.open(OUTPUT / f'{name}-startup.bmp') as image:
        assert image.size == tuple(target)
    return restarted


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
    # 실제 State3 cleanup은 Start 뒤00565e68을0으로 쓴다. tail의 같은 ser22 재요청은 즉시 반환해 이0을 보존한다.
    # 곡 길이 덧셈 자체는 SceneMusic의 독립 원문 기대값으로 검사하며 옛 호스트 메뉴의 초기값을 강제하지 않는다.
    assert number(values, 'songEnd') == 0, values
    assert [int(values[key]) for key in ('soundCameraX', 'soundCameraY', 'soundLeft', 'soundTop', 'soundRight', 'soundBottom')] == [0, 0, 0, 0, 1024, 768], values
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


def check_view(executable, root):
    """같은 실제 pan/Home·ESC 이탈·음향 옵션과 SETSIZE 재시작의 카메라/도킹 영역을 대조한다."""
    from cpp_world_smoke import states

    def check_world_view(state, label):
        """위치 음향이 실제 Renderer 카메라와 현재 Uber 도킹 mapRect를 공유하는지 확인한다."""
        camera = list(map(int, state['camera']))  # 보고 전용 현재 픽셀이며 목표 좌표를 대신 기대값으로 쓰지 않는다.
        viewport = list(map(int, state['raw_uber'][3:7]))  # 고정 top28 대신 동일 HUD가 계산한 실제 영역이다.
        assert list(map(int, state['audio_view'])) == camera + viewport, (label, state['audio_view'], camera, viewport)
        assert camera == list(map(int, state['raw_camera_frame'][:2])), (label, state['raw_camera_frame'])

    released = f'{0x80010000},512,384'  # 현재 검사 폴링의 실제 뗌 상태다.
    # Alt pan을 실제 뗌으로 끝낸 뒤 Home·ESC 이탈·음향 옵션을 같은 일반 큐로 진행한다.
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Play Mission'), (18, 'pointer', released),
             (20, 'report', 'initial'), (22, 'key', 'home'), (24, 'report', 'home_initial'),
             (25, 'pointer', f'{0x120000},768,384'),
             (36, 'report', 'scrolled'), (40, 'pointer', released),
             (48, 'key', 'home'), (55, 'report', 'home'),
             (60, 'esc', ''), (65, 'click', 'Game'), (70, 'click', 'menu-command:0200008c,0'),
             (75, 'click', 'Main Menu'), (85, 'report', 'menu'),
             (90, 'click', 'Options'), (95, 'click', 'Sound On'), (100, 'report', 'sound_off'),
             (105, 'click', 'Options'), (110, 'click', 'Sound On'), (115, 'report', 'sound_on')]
    run(executable, root, 'view', 118, extra=['--mission', 'thewarbegins'], script=steps,
        settings='maxFPS=40;scrollWhileMouseOut=0;edgeScroll=0;sound=1')
    observed = states(OUTPUT / 'view-states.tsv')  # 실행한 같은 장면의 관측만 읽는다.
    for label in ('briefing', 'initial', 'home_initial', 'scrolled', 'home'):  # 정지 브리핑과 조작 중 모두 현재 월드 장치를 공유해야 한다.
        check_world_view(observed[label], label)
    assert observed['briefing']['paused'] == ['1'] and observed['home']['paused'] == ['0']
    # 원문 초기 Reset은 사제를, Home은 현재 Player+7c/84의 홈 SID를 고른다. 같은 Home을 pan 전후에 비교한다.
    assert observed['scrolled']['camera'] != observed['home_initial']['camera']
    assert observed['home']['camera'] == observed['home_initial']['camera']
    assert observed['sound_off']['sound'] == ['0'] and observed['sound_on']['sound'] == ['1']
    assert observed['menu']['phase'] == ['0'] and observed['menu']['state_hud'][0] == '0'
    # State3 메인 복귀의 음향 원문 writer는 별도 확인이 필요하다. 현재 제품의 잔존값만 따라 이 단언을 완화하지 않는다.
    assert observed['menu']['audio_view'] == ['0', '0', '0', '0', '1024', '768'] and 'camera' not in observed['menu']
    assert observed['sound_off']['audio_view'] == observed['menu']['audio_view'] == observed['sound_on']['audio_view']
    restarted_views = {}  # 실제 메뉴 저장/종료를 거친 세 새 프로세스의 음향 기준이다.
    with preserve_game_settings(root, 'd'):
        for index, size in enumerate(((800, 600), (640, 480), (1024, 768))):  # 원본 세4:3 해상도를 실제 확인/재시작으로 순환한다.
            name = f'view-{size[0]}'  # 실행별 계획/상태/캡처를 덮어쓰지 않는다.
            restart = [(10, 'click', 'Campaign'), (15, 'click', 'Struggle For Freedom'),
                       (20, 'click', '1 The War Begins!'), (35, 'report', 'briefing'),
                       (40, 'click', 'Play Mission'), (50, 'report', 'world')]
            scenes = resolution_roundtrip(executable, root, name, size,
                initial_size=(1024, 768) if index == 0 else None, restart_steps=restart, restart_frames=55)
            for label in ('briefing', 'world'):  # 새 장치와 실제 Session이 같은 현재 시야를 공급해야 한다.
                check_world_view(scenes[label], name + '/' + label)
            assert scenes['briefing']['paused'] == ['1'] and scenes['world']['paused'] == ['0']
            assert scenes['briefing']['camera'] == scenes['world']['camera']
            restarted_views[str(size[0])] = scenes['world']['audio_view']
    assert restarted_views['1024'][:2] == observed['initial']['camera']
    return {'observed_states': len(observed), 'camera_and_home': True, 'resolution_restart_views': restarted_views,
            'sound_off_on_preserves_view': True, 'menu_resets_camera': True, 'reload_initializes_view': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/Netstorm.exe')
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
            report['view'] = check_view(arguments.exe, root)
    finally:
        assert snapshots(root) == before, '원본 파일의 바이트/존재 상태 변경'
    report['original_files_unchanged'] = len(before)
    import json
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
