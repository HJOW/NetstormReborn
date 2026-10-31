#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 창의 메뉴→캠페인→원본 브리핑→미션 진입/복귀와 GIF 번호를 검사한다. 이동은 cpp_world_smoke.py가 검사한다."""
import argparse
import io
import json
import struct
import subprocess
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops
from taff import TaffArchive
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

# 원본 자료를 건드리지 않는 검사 결과 폴더.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-menu-smoke'


def run(executable, arguments):
    """새 C++ 실행 파일만 구동하고 실패 메시지를 보존한다."""
    result = subprocess.run([str(executable), *map(str, arguments)], capture_output=True, timeout=60)
    if result.returncode:
        raise AssertionError(result.stderr.decode('utf-8', errors='replace') + result.stdout.decode('utf-8', errors='replace'))
    return result.stdout


def source(root, archive, path):
    """독립 Python VFS로 디스크 우선, 없으면 TAFF의 원본 바이트를 읽는다."""
    disk = root / path
    if disk.exists():
        return disk.read_bytes()
    entry = next(e for e in archive.entries if e.name.replace('\\', '/').lstrip('/').lower() == path.lower())
    return archive.read(entry)


def check_gifs(executable, root, archive):
    """Pillow의 해독 결과와 C++ 색 번호/투명을 대조한다. 실제 자료가 LZW 사전 폭 증가도 검사한다."""
    count = 0
    # 메뉴와 캠페인 배경을 모두 읽어 타이틀 하나만 맞는 경우를 막는다.
    for path in ['d/titleMenu.gif', 'd/gifcloud.gif', 'd/gifcloud2.gif']:
        expected = Image.open(io.BytesIO(source(root, archive, path)))
        assert expected.mode == 'P'
        data = run(executable, ['--dump-gif', root, path])
        width, height = struct.unpack_from('<2I', data)
        size = width * height
        assert (width, height) == expected.size and len(data) == 8 + size * 2
        assert data[8:8 + size] == expected.tobytes(), path
        transparent = expected.info.get('transparency', -1)
        opacity = bytes(0 if value == transparent else 255 for value in expected.tobytes())
        assert data[8 + size:] == opacity, path
        count += size
    return count


def reports(path):
    """프레임마다 기록된 실제 상태와 판정 영역을 읽는다."""
    result = {}
    current = None
    # report 사건마다 새 스냅샷을 시작한다.
    for line in path.read_text(encoding='utf-8').splitlines():
        fields = line.split('\t')
        if fields[0] == 'frame':
            current = {'frame': int(fields[1]), 'controls': {}, '_rows': {}}
            result[fields[2]] = current
        elif fields[0] == 'control':
            current['controls'][fields[1]] = list(map(int, fields[2:]))
        else:
            # Native HTML/메뉴는 같은 종류가 여러 번 나오므로 마지막 값과 별도로 모든 행을 보존한다.
            current.setdefault('_rows', {}).setdefault(fields[0], []).append(fields[1:])
            current[fields[0]] = fields[1:]
    return result


def button_labels(state):
    """같은 실제 화면의 host/Native HTML 표시 버튼을 읽고 가상 Game 버튼을 만들지 않는다."""
    # 실제 HTML 버튼 행의 조상 표시 비트가 켜진 라벨만 기존 host 입력 표와 합친다.
    return set(state['controls']) | {row[11] for row in state['_rows'].get('raw_html_button', []) if len(row) == 12 and row[10] == '1'}


def menu_rows(state):
    """같은 보고서의 표시된 실제 메뉴 행을 숫자 command/parameter와 함께 읽는다."""
    # 원문 메뉴의 일곱 필드 중 실제 표시된 부모 아래에 있는 행만 순서대로 유지한다.
    return [row for row in state['_rows'].get('raw_menu_item', []) if len(row) == 7 and row[5] == '1']


def check_title(root, archive, image_path):
    """돌 버튼 영역을 제외한 실제 창 전체를 독립 GIF/게임 팔레트로 합성해 비교한다."""
    data = source(root, archive, 'd/gifcloud.col')
    colors = list(data[8:8 + 768])
    colors[:3] = [0, 0, 0]
    colors[-3:] = [255, 255, 255]
    cloud = Image.open(io.BytesIO(source(root, archive, 'd/gifcloud.gif')))
    title = Image.open(io.BytesIO(source(root, archive, 'd/titleMenu.gif')))
    cloud.putpalette(colors)
    title.putpalette(colors)
    expected = Image.new('RGB', (1024, 768))
    # 원본 크기의 구름 타일을 화면 전체에 반복한다.
    for y in range(0, 768, cloud.height):
        # 오른쪽 경계는 Pillow paste가 잘라 낸다.
        for x in range(0, 1024, cloud.width):
            expected.paste(cloud.convert('RGB'), (x, y))
    expected.paste(title.convert('RGB'), (192, 144))
    actual = Image.open(image_path).convert('RGB')
    assert actual.size == expected.size
    mask = Image.new('L', expected.size, 255)
    draw = ImageDraw.Draw(mask)
    # 버튼은 별도 입력/좌표 검사 대상으로 두고 GIF 픽셀 대조에서는 제외한다.
    for i in range(8):
        x, y = 356 + i % 4 * 79, 311 + i // 4 * 23
        draw.rectangle((x, y, x + 74, y + 19), fill=0)
    expected.paste((0, 0, 0), mask=ImageChops.invert(mask))
    actual.paste((0, 0, 0), mask=ImageChops.invert(mask))
    assert ImageChops.difference(actual, expected).getbbox() is None, 'GIF 색 번호/화면 합성 불일치'
    return 1024 * 768 - 8 * 75 * 20


def check_window(executable, root):
    """실제 메뉴/브리핑/ESC와 전체화면 왕복 및 원문 해상도 저장·종료·재시작을 검사한다."""
    from cpp_audio_smoke import resolution_roundtrip, after_startup

    # 메뉴 탐색·미션 이탈/취소·두 모드 전환을 현재 화면의 실제 라벨 입력으로만 진행한다.
    steps = [(5,'report','main'),(5,'snapshot',str(OUTPUT/'main.bmp')),
        (10,'right','Campaign'),(15,'report','right_ignored'),(20,'click','Campaign'),(30,'report','campaign'),
        (30,'snapshot',str(OUTPUT/'campaign.bmp')),(35,'click','Struggle For Freedom'),(45,'report','overview'),
        (50,'click','2 Master of Whirligigs'),(55,'report','locked'),(60,'click','1 The War Begins!'),
        (80,'report','briefing'),(80,'snapshot',str(OUTPUT/'briefing.bmp')),(85,'esc',''),(90,'report','briefing_esc'),
        (95,'click','Play Mission'),(105,'report','mission'),(105,'snapshot',str(OUTPUT/'mission.bmp')),
        (110,'esc',''),(115,'report','bar'),(120,'click','Game'),(125,'report','game'),
        (130,'click','menu-command:0200008c,0'),(140,'report','leave'),(145,'click','Continue Mission'),
        (155,'report','resumed'),(160,'esc',''),(165,'click','Game'),(170,'click','menu-command:0200008c,0'),
        (180,'report','leave2'),(185,'click','Main Menu'),(200,'report','returned'),
        (205,'click','Campaign'),(210,'report','campaign2'),(215,'click','Struggle For Freedom'),(220,'report','overview2'),
        (225,'click','Back'),(230,'report','campaign_back'),(235,'click','Back'),(240,'report','main_back'),
        (245,'click','Options'),(250,'report','options'),(255,'click','Direct Draw / Full Screen'),
        (270,'report','fullscreen'),(275,'click','Options'),(280,'click','Window Mode'),(295,'report','window'),
        (300,'click','Options'),(305,'click','Sound On'),(310,'report','sound_toggle'),
        (315,'click','Credits'),(320,'report','credits'),(325,'click','Cancel'),(330,'report','credits_back'),
        (335,'click','Help'),(340,'report','help'),(345,'click','About NetStorm'),(350,'report','about'),
        (355,'click','OK'),(360,'report','about_back'),(365,'click','Demo'),(370,'report','demo'),
        (375,'click','Cancel'),(380,'report','demo_back'),(385,'click','Campaign'),(390,'click','Early Missions'),
        (400,'report','early'),(405,'click','1 Bridge the Gap'),(425,'report','tutorial_brief'),
        (430,'click','MORE'),(435,'report','tutorial_more'),(440,'click','BACK'),(445,'report','tutorial_back'),
        (450,'click','MORE'),(455,'click','OK'),(465,'report','tutorial_world'),
        (470,'esc',''),(475,'click','Game'),(480,'click','menu-command:0200008c,0'),(485,'click','Main Menu'),
        (500,'report','tutorial_return'),(505,'click','Options'),(510,'click','Music Volume >'),
        (515,'click','Volume 4'),(520,'report','volume_changed'),(525,'click','Options'),(530,'click','Music Volume >'),
        (535,'report','volume_selected'),(540,'outside',''),(545,'report','volume_closed'),(550,'click','Quit')]
    steps, frames = after_startup(steps, 555) # 실제 첫 안내를 닫고 메인 기준 프레임을 뒤로 옮긴다.
    script = OUTPUT/'steps.tsv'  # 입력은 상태/명령 직접 쓰기 없이 실제 라벨과 관측된 command 행 좌표를 쓴다.
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    marker = root/'fullscreenStateFile.dat'  # 허용된 옛 실패 표식만 생성하고 바깥 설정 보관자가 원상 복구한다.
    marker.write_bytes(b'previous startup')
    # 정적 GIF 픽셀 검사의 초기 캐시만 cloudSpeed0으로 고정한다. 동적 구름은 별도 원문 Kencloud 검사 범위다.
    settings = 'SCREENW=1024;SCREENH=768;windowScreenFlags=10;workingFullScreenFlags=11;startInFullScreen=0;maxFPS=60;sleepPerLoop=8;sound=1;musicVolume=2;autoDemo=0;tellTips=0;cloudSpeed=0;DoneTheWarBegins=0;DoneMasterOfWhirligigs=0;DoneTutorial1=0'
    run(executable,['--run',root,'--window','--mute-audio','--frames',str(frames),'--set',settings,
                    '--ui-script',script,'--ui-report',OUTPUT/'states.tsv'])
    states = reports(OUTPUT/'states.tsv')  # 현재 Native HTML/공통 메뉴의 실제 반복 행을 보존한다.
    # 각 실제 페이지/미션의 완료 프레임 단계이며 버튼 선택 요청 자체를 성공으로 세지 않는다.
    phases = {'main':0,'right_ignored':0,'campaign':1,'overview':1,'locked':1,'briefing':3,'briefing_esc':3,
              'mission':4,'bar':4,'game':4,'leave':1,'resumed':4,'leave2':1,'returned':0,'campaign_back':1,
              'main_back':0,'credits':1,'credits_back':0,'about':1,'about_back':0,'demo':1,'demo_back':0,
              'tutorial_brief':3,'tutorial_more':1,'tutorial_back':1,'tutorial_world':4,'tutorial_return':0}
    for label,phase in phases.items():  # 요청이 아닌 프레임 종료의 실제 단계를 판정한다.
        assert states[label]['phase']==[str(phase)],(label,states[label]['phase'])
    # Windows10/11 DIB 호환 경로는 원본과 달리 시작 모드 설정 전에 옛 DirectDraw 실패 표식을 지운다.
    for label in ('main','right_ignored','campaign'):  # 최초 메뉴부터 캠페인까지 표식 부재가 계속 유지되어야 한다.
        assert states[label]['fullscreenMarker']==['0'],(label,states[label]['fullscreenMarker'])
    assert states['main']['kencloud'][4]=='0', '정적 GIF 픽셀 검사의 명시 cloudSpeed0이 적용되지 않음'
    assert not marker.exists()
    assert any(row[6]=='1 The War Begins!' for row in menu_rows(states['overview'])) # 미션 목록은 stone 버튼 배열과 별개인 실제 controlled 메뉴 행이다.
    assert states['locked']['page']==states['overview']['page'] and 'mission' not in states['locked']
    assert states['briefing']['paused']==['1'] and states['resumed']['paused']==['0']
    assert states['mission']['mission'][:2]==['TheWarBegins','Tutorial'] and int(states['mission']['mission'][2])>0
    assert states['bar']['game_menu_bar'][1]=='1' and states['bar']['paused']==['0']
    # Game 하위의 실제 command8c 행이 원본 캠페인 이탈 라벨과 함께 표시되는지 찾는다.
    assert any(int(row[3])==0x0200008c and row[6].strip()=='Leave Mission' for row in menu_rows(states['game']))
    for label in ('leave','leave2'):  # 캠페인의 실제 ABORT 확인과 일반 Battle LeaveNormal을 구별한다.
        assert states[label]['page'][0].casefold()=='abort' and states[label]['paused']==['1']
        assert {'Main Menu','Replay Mission','Continue Mission'}<=button_labels(states[label])
    assert states['mission']['raw_uber_children']==states['resumed']['raw_uber_children']
    assert states['options']['controls']['Direct Draw / Full Screen'][0]==1
    assert int(states['fullscreen']['screen_mode'][0])&1 and not (int(states['window']['screen_mode'][0])&1)
    assert states['fullscreen']['size']==states['window']['size']==['1024','768']
    assert states['sound_toggle']['sound']==['0']
    assert states['tutorial_more']['page']==['A1.'] and states['tutorial_back']['page']==['A.']
    assert states['tutorial_more']['paused']==states['tutorial_back']['paused']==['1'] # MORE/BACK의 일반 Tell은 Dialog1이지만 같은 Tutorial 자기 정지를 보존한다.
    assert states['volume_changed']['musicVolume']==['4'] and states['volume_changed']['popup']==['']
    assert states['volume_selected']['controls']['Volume 4'][-1]==1 and states['volume_selected']['controls']['Volume 2'][-1]==0
    restart = OUTPUT/'restart.tsv'  # 저장 설정을 크기/소리/음량 덮어쓰기 없이 새 프로세스가 읽는다.
    restart_steps, restart_frames = after_startup([(5, 'report', 'restarted')], 8) # 저장된 옵션을 읽는 새 프로세스에서도 실제 초기 안내를 닫는다.
    restart.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in restart_steps),encoding='utf-8')
    run(executable,['--run',root,'--window','--mute-audio','--frames',str(restart_frames),'--ui-script',restart,'--ui-report',OUTPUT/'restart-states.tsv'])
    restarted = reports(OUTPUT/'restart-states.tsv')['restarted']
    assert restarted['musicVolume']==['4'] and restarted['sound']==['0'] and restarted['size']==['1024','768']
    resolutions = []  # 실제 Yes 저장/종료와 재실행 크기/픽셀을 확인한 값만 기록한다.
    for index,size in enumerate(((800,600),(640,480),(1024,768))):  # 원본3개 해상도를 같은 저장 결과에서 연속 재시작한다.
        observed=resolution_roundtrip(executable,root,f'menu-{size[0]}',size,initial_size=(1024,768) if index==0 else None)
        assert observed['startup']['size']==list(map(str,size))
        resolutions.append(list(size))
    return {'observed_states':len(states),'mission_objects':int(states['mission']['mission'][2]),
            'tutorial_objects':int(states['tutorial_world']['mission'][2]),'native_window_navigation':True,
            'resolution_changes':resolutions,'fullscreen_marker_event':True,'fullscreen_window_roundtrip':True,
            'options_persist_after_restart':True}


def main():
    """기준 판본의 탐색과 두 판본의 GIF 자료를 검사하고 원본 해시/설정을 복구한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/Netstorm.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {}
    # CD는 자료 호환만 대조한다. 메뉴 스크립트는 10.78과 달라 완료 기준에 섞지 않는다.
    for edition in ['originals', 'originalCD']:
        root = ROOT / edition
        before = snapshots(root)
        archive = TaffArchive(root / 'netstorm.tarc')
        report[edition] = {'gif_pixels': check_gifs(args.exe, root, archive)}
        if edition == 'originals':
            with preserve_game_settings(root, 'd'):
                report[edition].update(check_window(args.exe, root))
            report[edition]['menu_background_pixels'] = check_title(root, archive, OUTPUT / 'main.bmp')
        assert snapshots(root) == before, f'{edition}: 원본 파일/존재 상태 변경'
        report[edition]['original_files_unchanged'] = True
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
