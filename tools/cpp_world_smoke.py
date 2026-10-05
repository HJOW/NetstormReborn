#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj의 본섬/초기 객체를 독립 판독기와 대조하고 클론 창에서 선택·이동·정지·카메라·복귀를 검사한다."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image
from cpp_fort_smoke import Catalog, script_value
from cpp_menu_smoke import run, source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from fort import SECTION_NAMES, split_sections, read_chunks
from terrain_mask import generate_mask, territory_chunks
from taff import TaffArchive

# 원본 자료와 분리한 저장소 루트와 검사 출력 위치.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/cpp-world-smoke'
# 저장 미션을 비교하는 서로 다른 지형/소유자/요새 이름의 사례.
MISSIONS = ('thewarbegins', 'savetheisland', 'tutorial1', 'TEST01')


def parse(text):
    """같은 프레임의 객체를 번호로 유지하여 이동 전후 같은 유닛을 비교한다."""
    state = {'objects': {}, 'players': {}, 'controls': {}}
    # TSV의 종류별로 반복 레코드와 단일 레코드를 구분한다.
    for line in text.splitlines():
        fields = line.split('\t')
        if fields[0] == 'object':
            state['objects'][int(fields[1])] = fields[2:]
        elif fields[0] == 'player':
            state['players'][int(fields[1])] = fields[2:]
        elif fields[0] == 'control':
            state['controls'][fields[1]] = fields[2:]
        else:
            state[fields[0]] = fields[1:]
    return state


def states(path):
    """보고 지점마다 독립 스냅샷을 만든다."""
    result = {}
    # 맨 앞 frame 필드는 뒤 내용의 관찰 이름이다.
    for block in path.read_text(encoding='utf-8').split('frame\t')[1:]:
        header, body = block.split('\n', 1)
        frame, label = header.split('\t', 1)
        result[label] = parse(body)
        result[label]['frame'] = int(frame)
    return result


def initial_world(executable, root, name, patterns, catalog, archive, flag=()):
    """65,536칸 마스크와 모든 저장 객체의 좌표·소유자·수량·내용물 수를 Python에서 따로 계산한다."""
    script = source(root, archive, f'd/{name}.english').decode('cp1252')
    fort_name = script_value(script, 'loadFort') or name
    data = source(root, archive, f'd/{fort_name}.fort')
    sections, _ = split_sections(data)
    named = dict(zip(SECTION_NAMES, sections))
    expected = generate_mask(named['Territory'], patterns)
    state = parse(run(executable, ['--dump-world', root, name, *flag]).decode('utf-8'))
    actual = bytes.fromhex(state['land'][0])
    assert actual == expected, f'{name}: 전체 본섬 마스크 불일치'
    coordinates = {}
    # 영역의 y·x 청크 순서가 실제 요새 레코드의 월드 좌표를 정한다.
    for x, y, region, *_ in territory_chunks(named['Territory'], patterns):
        coordinates.setdefault(region, []).append((x, y))
    conversion = catalog.conversion(named['TypeNames'])
    objects = []
    # Chaff 뒤에 Terr00~19를 같은 입력 순서로 읽는다.
    for section_name in ['Chaff', *(f'Terr{region:02}' for region in range(20))]:
        section = named.get(section_name, b'')
        if len(section) <= 1:
            continue
        _, chunks = read_chunks(section, conversion, catalog)
        region = -1 if section_name == 'Chaff' else int(section_name[4:])
        positions = [(i % 16, i // 16) for i in range(256)] if region == -1 else coordinates.get(region, [])
        # 각 청크의 객체는 같은 원본 위치 바이트를 사용한다.
        for index, chunk in enumerate(chunks):
            cx, cy = positions[index]
            # island 레코드만 원본처럼 건너뛴다.
            for obj in chunk:
                if obj.type_name == 'island':
                    continue
                owner = obj.fields.get('owner', 1)
                if owner == 0 or owner > 8:
                    owner = 1
                f2 = catalog.flags[obj.type_name][1]
                if f2 & 0x10002002:
                    owner = 0
                quantity = obj.fields.get('qb', obj.fields.get('qa', 0))
                if 'qb' in obj.fields and quantity >= 32768:
                    quantity -= 65536
                objects.append((obj.type_name, owner, region, cx * 16 + obj.cell[0], cy * 16 + obj.cell[1], quantity, len(obj.fields.get('contents', []))))
    assert len(objects) == len(state['objects'])
    # 런타임 번호가 같은 저장 객체에 해당하는지 모든 레코드를 대조한다.
    for index, obj in enumerate(objects, 1):
        row = state['objects'][index]
        observed = (row[0], int(row[1]), int(row[2]), int(row[3]), int(row[4]), int(row[11]), int(row[13]))
        assert observed == obj, (name, index, observed, obj)
    assert float(state['players'][1][0]) == float(script_value(script, 'myStartMoney') or 6500)
    assert state['players'][1][4] == (script_value(script, 'myTech') or '')
    state['mask'] = expected
    return state, {'land_bytes': len(actual), 'land_cells': sum(value != 255 for value in actual),
                   'sha256': hashlib.sha256(actual).hexdigest(), 'objects_compared': len(objects),
                   'starting_storm_power': float(state['players'][1][0])}


def priest(state):
    """현재 프레임에서 내 사제의 저장/이동 좌표를 찾는다."""
    return next(row for row in state['objects'].values() if row[0] == 'priest' and row[1] == '1')


def window(executable, root, name, steps, frames, direct=False):
    """명령을 직접 호출하지 않고 기존 InputQueue의 마우스/키 사건을 사용한다."""
    script = OUTPUT / f'{name}-steps.tsv'
    report = OUTPUT / f'{name}-states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    arguments = ['--run', root, '--window', '--frames', frames, '--set', 'SCREENW=1024;SCREENH=768;sleepPerLoop=0', '--ui-script', script, '--ui-report', report]
    if direct:
        arguments.extend(['--mission', name])
    run(executable, arguments)
    return states(report)


def check_movement(executable, root):
    """캠페인에서 진입해 몸통 선택·이동·ESC 정지·재개·허공 거부·카메라·같은 미션 재진입을 검사한다."""
    steps = [
        (2, 'click', 'Campaign'), (4, 'click', 'Struggle For Freedom'), (6, 'click', '1 The War Begins!'),
        (9, 'click', 'Play Mission'), (10, 'report', 'initial'), (10, 'snapshot', str(OUTPUT / 'initial.bmp')),
        (11, 'click', 'world:priest'), (12, 'report', 'selected'), (12, 'snapshot', str(OUTPUT / 'selected.bmp')),
        (13, 'click', 'cell:98,108'), (14, 'report', 'moving'), (28, 'report', 'progress'),
        (30, 'esc', ''), (40, 'report', 'paused'), (85, 'report', 'paused_later'),
        (94, 'esc', ''), (105, 'report', 'resumed'), (350, 'report', 'arrived'), (350, 'snapshot', str(OUTPUT / 'arrived.bmp')),
        (351, 'right', 'world:priest'), (353, 'report', 'right_menu'), (355, 'outside', ''), (357, 'report', 'closed_menu'),
        (358, 'click', 'world:priest'), (359, 'click', 'cell:80,90'), (360, 'report', 'void_rejected'),
        (361, 'key', 'left'), (380, 'keyup', 'left'), (381, 'report', 'camera_moved'),
        (382, 'key', 'home'), (383, 'keyup', 'home'), (384, 'report', 'home'),
        (385, 'esc', ''), (387, 'click', 'Leave Battle'), (389, 'click', 'Leave Honorably'), (391, 'report', 'main'),
        (393, 'click', 'Campaign'), (395, 'click', 'Struggle For Freedom'), (397, 'click', '1 The War Begins!'),
        (400, 'click', 'Play Mission'), (402, 'report', 'reloaded'),
    ]
    result = window(executable, root, 'thewarbegins', steps, 405)
    assert result['selected']['selected'][0] == '146'
    assert result['moving']['selected'][0] == '0' and priest(result['moving'])[8] == '1'
    assert 16 <= int(priest(result['moving'])[7]) < 24, '동쪽 C00~C07 걷기 프레임 불일치'
    start, progress = result['moving'], result['progress']
    # 직선 이동의 표시 좌표와 원본 1.8칸/초를 실제 게임 시간으로 비교한다.
    elapsed = float(progress['time'][0]) - float(start['time'][0])
    assert abs(float(priest(progress)[5]) - float(priest(start)[5]) - elapsed * 1.8) < 1e-6
    assert result['paused']['paused'] == ['1'] and result['paused_later']['paused'] == ['1']
    assert priest(result['paused']) == priest(result['paused_later'])
    assert result['paused']['time'][0] == result['paused_later']['time'][0]
    assert result['resumed']['paused'] == ['0'] and float(priest(result['resumed'])[5]) > float(priest(result['paused'])[5])
    assert priest(result['arrived'])[3:7] == ['98', '108', '98', '108'] and priest(result['arrived'])[8] == '0'
    assert priest(result['arrived'])[7] == '17', '도착 뒤 동쪽 C01 정지 프레임 불일치'
    assert result['right_menu']['popup'] == ['object'] and result['right_menu']['controls']['Construct >'][0] == '0'
    assert result['closed_menu']['paused'] == ['0'] and result['void_rejected']['selected'] == ['0']
    assert priest(result['void_rejected'])[3:7] == ['98', '108', '98', '108']
    assert result['camera_moved']['camera'] != result['initial']['camera'] and result['home']['camera'] == result['initial']['camera']
    assert result['main']['phase'] == ['0'] and not result['main']['objects']
    assert priest(result['reloaded'])[3:7] == ['94', '108', '94', '108'] and result['reloaded']['selected'] == ['0']
    assert Image.open(OUTPUT / 'selected.bmp').tobytes() != Image.open(OUTPUT / 'initial.bmp').tobytes()
    return {'observed_states': len(result), 'priest_speed': 1.8, 'arrival': [98, 108], 'pause_freezes_world': True,
            'right_click_menu': True, 'void_rejection': True, 'camera_and_home': True, 'reload_clears_old_world': True}


def check_test01(executable, root):
    """TEST01도 동일 브리핑·월드 경로로 진입하여 50,000 SP와 실제 사제 이동을 검사한다."""
    steps = [(2, 'report', 'briefing'), (3, 'click', 'Go!'), (4, 'report', 'initial'),
             (5, 'click', 'world:priest'), (6, 'report', 'selected'),
             (7, 'click', 'cell:174,202'), (8, 'report', 'moving'), (250, 'report', 'arrived'),
             (250, 'snapshot', str(OUTPUT / 'TEST01.bmp'))]
    result = window(executable, root, 'TEST01', steps, 252, direct=True)
    assert result['briefing']['phase'] == ['3'] and result['initial']['players'][1][0] == '50000'
    assert result['selected']['selected'] == ['1444'] and priest(result['moving'])[8] == '1'
    assert priest(result['arrived'])[3:7] == ['174', '202', '174', '202'] and priest(result['arrived'])[8] == '0'
    assert result['initial']['players'][2][1:3] == ['8', '6'] and result['initial']['players'][3][1:3] == ['2', '8']
    return {'observed_states': len(result), 'objects': len(result['initial']['objects']), 'storm_power': 50000, 'priest_arrival': [174, 202]}


def main():
    """원본 전체 해시를 보존하며 읽기 전용 대조와 새 실행 파일의 조작 검사를 순차 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    root = ROOT / 'originals'
    OUTPUT.mkdir(parents=True, exist_ok=True)
    before = snapshots(root)
    archive = TaffArchive(root / 'netstorm.tarc')
    patterns = json.loads((ROOT / 'dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json').read_text(encoding='utf-8'))
    catalog = Catalog(root, 'd', 116)
    report = {'missions': {}}
    # 자료 보호 경로가 같은 설정을 쓰므로 다른 창 검사와 병렬 실행하지 않는다.
    try:
        with preserve_game_settings(root, 'd'):
            # 서로 다른 네 미션의 전체 마스크와 초기 객체를 대조한다.
            for name in MISSIONS:
                _, report['missions'][name] = initial_world(args.exe, root, name, patterns, catalog, archive)
            report['movement'] = check_movement(args.exe, root)
            report['TEST01_window'] = check_test01(args.exe, root)
    finally:
        assert snapshots(root) == before, '원본 파일의 바이트/존재 상태 변경'
    report['original_files_unchanged'] = True
    cd = ROOT / 'originalCD'
    cd_before = snapshots(cd)
    cd_archive, cd_catalog = TaffArchive(cd / 'NETSTORM.TARC'), Catalog(cd, 'D', 101)
    report['CD_readonly_missions'] = {}
    # CD판의 자산/타입 번호가 달라도 두 같은 맵의 지형과 객체 필드는 일치해야 한다.
    for name in ('thewarbegins', 'savetheisland'):
        _, report['CD_readonly_missions'][name] = initial_world(args.exe, cd, name, patterns, cd_catalog, cd_archive, ('--cd',))
    assert snapshots(cd) == cd_before, 'CD 원본 파일의 바이트/존재 상태 변경'
    report['CD_files_unchanged'] = True
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
