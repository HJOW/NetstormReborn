#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인1-1에서 실제 생산·선택한 골렘의 간헐천/지면 커서와 Win32 그림을 확인한다."""
import argparse
import ctypes
import hashlib
from pathlib import Path

from PIL import Image
from cpp_bridge_ritual_runtime import ROOT, save, sha
from cpp_game_menu_smoke import protected_run
from cpp_world_smoke import states
from ns_driver import CursorLogger, cursor_sheet

# 실제 두 화면비의 초기 카메라다. 목표는 같은 월드 픽셀로 입력한다.
VIEWS = ((1280, 720, 848, 817), (1024, 768, 976, 793))
# 원본에서 직접 관측한 간헐천 그림/자유 지면/자기 위치의 커서 번호다.
POINTS = (('geyser', 1611, 1148, 3), ('ground', 1611, 1115, 18),
          ('self', 1496, 1204, 6), ('geyser_again', 1611, 1148, 3))
# 원본19항목 표 가운데 이번 경로에서 실제 관측하는 리소스다.
RESOURCES = {1: 113, 3: 111, 6: 109, 18: 148}


def cursor_images(data, output):
    """실행 없이 PE 리소스를 열어 두 배경의 실제 픽셀과 핫스팟을 보존한다."""
    kernel, user = ctypes.WinDLL('kernel32'), ctypes.WinDLL('user32')
    kernel.LoadLibraryExW.argtypes = [ctypes.c_wchar_p, ctypes.c_void_p, ctypes.c_uint32]
    kernel.LoadLibraryExW.restype = ctypes.c_void_p
    kernel.FreeLibrary.argtypes = [ctypes.c_void_p]
    user.LoadCursorA.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    user.LoadCursorA.restype = ctypes.c_void_p
    module = kernel.LoadLibraryExW(str(data / 'Netstorm.exe'), None, 0x22)
    assert module
    result = {}
    try:
        for index, resource in RESOURCES.items():  # 번호나 핸들만 비교하지 않고 원본 그림 자체를 읽는다.
            handle = user.LoadCursorA(module, resource)
            assert handle
            image, hotspot = cursor_sheet(handle)
            image.save(output / f'expected-cursor-{index}.png')
            result[index] = dict(resource=resource, hotspot=hotspot,
                                 pixelsSha256=hashlib.sha256(image.tobytes()).hexdigest())
    finally:
        kernel.FreeLibrary(module)
    return result


def one(data, output, view, expected):
    """원래 Fort의 골렘 카드를 실제 클릭하며 좌표/Graph/선택과 OS 그림을 함께 기록한다."""
    width, height, cx, cy = view
    output.mkdir(parents=True, exist_ok=False)
    steps = [(10, 'click', 'Play Mission'), (50, 'report', 'initial'),
             (60, 'click', 'production:133'), (70, 'up', 'placement-near:94,112'),
             (80, 'click', 'placement-near:94,112'), (170, 'report', 'built'),
             (190, 'click', 'world:sunWalker'), (198, 'report', 'selected')]
    for index, (label, x, y, _) in enumerate(POINTS):  # 실제 폴링을 충분히 지난 뒤 같은 선택을 관측한다.
        frame = 215 + index * 30
        steps += [(frame, 'pointer', f'0x80010000,{x-cx},{y-cy}'),
                  (frame + 15, 'report', label), (frame + 15, 'snapshot', str(output / f'{label}.bmp'))]
    logger = CursorLogger(output)
    logger.start()
    try:
        report, run = protected_run(output / 'run', width, height, steps, 335,
                                    data_root=data, mission='thewarbegins')
    finally:
        logger.stop()
    observed = states(report)
    assert observed['initial']['camera'] == [str(cx), str(cy)]
    made = [(sid, row) for sid, row in observed['built']['raw_objects'].items() if row[:2] == ['133', '1']]
    assert len(made) == 1
    sid, raw = made[0]
    assert raw[3:5] == ['94', '110'], raw
    cases = []
    for label, _, _, cursor in POINTS:  # 같은 현재 SID와 실제 커서 출력의 세 분기를 대조한다.
        state = observed[label]
        assert state['raw_input'][:2] == [str(sid), '5']
        assert state['mouse_cursor'] == [str(cursor), '1'], (label, state['mouse_cursor'])
        assert state['raw_objects'][sid][:5] == raw[:5]
        assert float(state['raw_sp'][1]) == 2600 and float(state['raw_sp'][2]) == 0
        assert not any(row[:2] == ['133', '2'] for row in state['raw_objects'].values())
        hover = state['raw_carrier_hover']
        if label.startswith('geyser'):
            assert hover[1:3] == ['101', '105'] and hover[4] == '254'
            assert int(hover[6]) and hover[7] == '1', hover
        cases.append(dict(label=label, cursor=cursor, hover=hover))
    actual = {}
    for path in (output / 'cursors').glob('*.png'):  # 실제 GetCursorInfo의 핸들 그림을 PE의 독립 픽셀과 맞춘다.
        pixels = hashlib.sha256(Image.open(path).convert('RGB').tobytes()).hexdigest()
        actual[path.stem] = [index for index, value in expected.items() if value['pixelsSha256'] == pixels]
    assert {3, 6, 18}.issubset({index for indices in actual.values() for index in indices}), actual
    result = dict(view=list(view), golemSid=sid, paidSp=400, enemySp=0, enemyGolems=0,
                  cases=cases, hardwareCursorImages=actual, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """보호 원본 경로를 거부하고 두 실제 화면비의 결과를 새 폴더에만 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data, output = args.data_root.resolve(), args.output.resolve()
    assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents
               for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    expected = cursor_images(data, output)
    save(output / 'expected-cursors.json', expected)
    results = []
    for view in VIEWS:  # 게임이 서로 포커스/클래스 커서를 덮지 않도록 순차 실행한다.
        results.append(one(data, output / f'{view[0]}x{view[1]}', view, expected))
    save(output / 'summary.json', dict(results=results, sourceExeSha256=sha(data / 'Netstorm.exe'),
                                      naturalMissionCompletionVerified=False))
    print('골렘400SP 생산·선택과 두 화면비의 간헐천/지면/자기 위치 커서·OS 그림 확인')


if __name__ == '__main__':
    main()
