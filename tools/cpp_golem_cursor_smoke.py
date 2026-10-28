#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인1-1에서 실제 생산·선택한 골렘의 간헐천/지면 커서와 Win32 그림을 확인한다."""
import argparse
import ctypes
import hashlib
import json
import threading
from ctypes import wintypes
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


def position_hardware_pointer(output, stopped, result):
    """자기 게임의 실제 활성 HWND 안에 OS 포인터를 두어 창 밖 기본 화살표를 검사하지 않는다."""
    user = ctypes.WinDLL('user32')
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.IsWindowVisible.argtypes = [wintypes.HWND]
    user.GetForegroundWindow.restype = wintypes.HWND
    user.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
    user.ClientToScreen.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
    user.GetCursorPos.argtypes = [ctypes.POINTER(wintypes.POINT)]
    user.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
    original = wintypes.POINT()
    assert user.GetCursorPos(ctypes.byref(original))
    result['originalPosition'] = [original.x, original.y]
    try:
        while not stopped.wait(0.02):  # execute가 기록한 자기 PID와 실제 활성 게임 창이 생길 때까지만 기다린다.
            running = output / 'run/running.json'
            if not running.exists():
                continue
            try:
                pid = json.loads(running.read_text('utf-8'))['pid']
            except (ValueError, KeyError):
                continue
            found = []

            def visit(window, _):
                """다른 앱은 건드리지 않고 실행 중인 자기 PID의 보이는 활성 창만 고른다."""
                owner = wintypes.DWORD()
                user.GetWindowThreadProcessId(window, ctypes.byref(owner))
                if owner.value == pid and user.IsWindowVisible(window) and user.GetForegroundWindow() == window:
                    found.append(window)
                    return False
                return True

            user.EnumWindows(callback_type(visit), 0)
            if not found:
                continue
            bounds, point = wintypes.RECT(), wintypes.POINT()
            assert user.GetClientRect(found[0], ctypes.byref(bounds))
            point.x, point.y = (bounds.right - bounds.left) // 2, (bounds.bottom - bounds.top) // 2
            if point.x < 1 or point.y < 1:
                continue
            assert user.ClientToScreen(found[0], ctypes.byref(point)) and user.SetCursorPos(point.x, point.y)
            result.update(pid=pid, window=int(found[0]), positioned=[point.x, point.y])
            stopped.wait()  # 같은 논리 포인터 입력을 유지하며 OS 포인터를 반복 이동하지 않는다.
            break
    finally:
        result['restored'] = bool(user.SetCursorPos(original.x, original.y))


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
    pointer = {}  # 실제 OS 포인터의 검사 전제와 복구를 게임 결과와 구별해 남긴다.
    positioner = threading.Thread(target=position_hardware_pointer, args=(output, logger.stop_flag, pointer), daemon=True)
    logger.start()
    positioner.start()
    try:
        report, run = protected_run(output / 'run', width, height, steps, 335,
                                    data_root=data, mission='thewarbegins')
    finally:
        logger.stop()
        positioner.join(timeout=3)
        save(output / 'hardware-pointer.json', pointer)
    assert pointer.get('positioned') and pointer.get('restored'), pointer
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
