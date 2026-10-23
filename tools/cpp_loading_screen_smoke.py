#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사본 게임의 시작 직후 client 픽셀을 관측한다. --original을 지정해야 원본 사본도 실행한다.

예: python tools/cpp_loading_screen_smoke.py --output extracted/cpp-loading-20261023-gui1
원본 비교: 위 명령에 --original 추가. 실행은 실제 GUI를 여는 별도 작업이다.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import time

import pefile
from PIL import Image, ImageChops, ImageDraw
from nscfg import decode_file, encode_text, set_value

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 새 extracted 출력의 공통 기준이다.
SOURCE = ROOT / 'originals'  # 읽기·전체 복사·해시만 허용하는 보호 입력이다.
CLONE = ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe'  # 실행 전 별도 출력 폴더에 복사할 클론이다.
WINDOW_CLASS = 'NetstormClient'  # 분석기나 다른 프로세스의 창을 배제하는 실제 게임 클래스다.
BITMAP_ID = 147  # 보호 10.78의 RT_BITMAP 두루마리 번호다.
REFERENCE_SIZE = (328, 464)  # 실제 리소스 크기를 확인하고 다른 그림으로 오인하지 않는다.
SETTINGS = {'sound': '0', 'music': '0', 'playWind': '0', 'SCREENW': '1024', 'SCREENH': '768',
            'startInFullScreen': '0', 'windowScreenFlags': '10', 'autoDemo': '0'}  # 사본에만 인코딩할 창/무음 설정이다.
WM_CLOSE = 0x10  # 이 도구가 시작한 PID의 게임 창에만 보내는 정상 종료 메시지다.
SRCCOPY = 0x00cc0020  # 화면 합성이나 PrintWindow를 요청하지 않고 실제 client DC를 읽는다.
PNG_LIMIT = 6  # 최초 unique6개를 보존하고 뒤늦게 관측한 로딩 최대4개에 우선권을 준다.
UNIQUE_METADATA_LIMIT = 64  # PNG 보존 여부와 별개로 최초64개 unique 화면의 현재 관측을 남긴다.
READ_CHUNK = 1024 * 1024  # 큰 WAV/동영상도 전체 파일을 한 번에 메모리에 올리지 않는다.


def sha(path):
    """보호 파일과 실행 사본의 전체 바이트를 스트리밍 SHA256으로 식별한다."""
    digest = hashlib.sha256()
    with Path(path).open('rb') as source:
        while data := source.read(READ_CHUNK):  # 파일 끝까지 순서대로 읽으며 쓰기는 하지 않는다.
            digest.update(data)
    return digest.hexdigest()


def snapshot(directory):
    """전체 파일 이름/바이트를 보존하고 외부 경로로 이어지는 링크는 사전 거부한다."""
    result = {}
    for path in sorted(directory.rglob('*')):  # 설정 예외 없이 originals 전체를 같은 기준으로 읽는다.
        if path.is_symlink():
            raise RuntimeError('원본 복사 경계의 심볼릭 링크: ' + str(path))
        if path.is_file():
            result[path.relative_to(directory).as_posix()] = sha(path)
    return result


def save(path, value):
    """새 관측 경로에만 UTF-8/LF JSON을 기록한다."""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def scroll_bitmap(executable):
    """보호 PE의 RT_BITMAP147 DIB를 독립 RGB 이미지로 해독하며 실행하지 않는다."""
    pe = pefile.PE(str(executable))
    try:
        resources = [item for kind in pe.DIRECTORY_ENTRY_RESOURCE.entries if kind.id == 2
                     for item in kind.directory.entries if item.id == BITMAP_ID]  # 실제 숫자 리소스만 선택한다.
        if len(resources) != 1:
            raise RuntimeError('RT_BITMAP147 리소스가 유일하지 않음')
        language = resources[0].directory.entries[0].data.struct
        dib = pe.get_data(language.OffsetToData, language.Size)
        header, width, height, planes, depth, compression = struct.unpack_from('<IiiHHI', dib)
        if header < 40 or (width, abs(height)) != REFERENCE_SIZE or planes != 1:
            raise RuntimeError('원본 두루마리 DIB 헤더/크기 변경')
        used = struct.unpack_from('<I', dib, 32)[0]
        palette = used if used else (1 << depth if depth <= 8 else 0)
        masks = 12 if header == 40 and compression == 3 else 0
        offset = 14 + header + masks + palette * 4
        bmp = b'BM' + struct.pack('<IHHI', 14 + len(dib), 0, 0, offset) + dib
        image = Image.open(io.BytesIO(bmp)).convert('RGB')
        image.load()
        return image, {'id': BITMAP_ID, 'size': list(image.size), 'dibSha256': hashlib.sha256(dib).hexdigest(),
                       'rgbSha256': hashlib.sha256(image.tobytes()).hexdigest(), 'peSha256': sha(executable)}
    finally:
        pe.close()


def api():
    """64비트 핸들이 잘리지 않도록 필요한 Win32 함수의 실제 형식을 지정한다."""
    user = ctypes.WinDLL('user32', use_last_error=True)
    gdi = ctypes.WinDLL('gdi32', use_last_error=True)
    timer = ctypes.WinDLL('winmm', use_last_error=True)
    enum_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    signatures = ((user.EnumWindows, [enum_type, wintypes.LPARAM], wintypes.BOOL),
                  (user.GetWindowThreadProcessId, [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD),
                  (user.GetClassNameW, [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int], ctypes.c_int),
                  (user.GetWindowTextW, [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int], ctypes.c_int),
                  (user.GetForegroundWindow, [], wintypes.HWND),
                  (user.GetClientRect, [wintypes.HWND, ctypes.POINTER(wintypes.RECT)], wintypes.BOOL),
                  (user.ClientToScreen, [wintypes.HWND, ctypes.POINTER(wintypes.POINT)], wintypes.BOOL),
                  (user.IsWindowVisible, [wintypes.HWND], wintypes.BOOL),
                  (user.IsIconic, [wintypes.HWND], wintypes.BOOL),
                  (user.GetDC, [wintypes.HWND], wintypes.HDC),
                  (user.ReleaseDC, [wintypes.HWND, wintypes.HDC], ctypes.c_int),
                  (user.PostMessageW, [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.BOOL),
                  (gdi.CreateCompatibleDC, [wintypes.HDC], wintypes.HDC),
                  (gdi.CreateDIBSection, [wintypes.HDC, ctypes.c_void_p, wintypes.UINT, ctypes.POINTER(ctypes.c_void_p), wintypes.HANDLE, wintypes.DWORD], wintypes.HANDLE),
                  (gdi.SelectObject, [wintypes.HDC, wintypes.HANDLE], wintypes.HANDLE),
                  (gdi.BitBlt, [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.DWORD], wintypes.BOOL),
                  (gdi.DeleteObject, [wintypes.HANDLE], wintypes.BOOL),
                  (gdi.DeleteDC, [wintypes.HDC], wintypes.BOOL),
                  (gdi.GdiFlush, [], wintypes.BOOL),
                  (timer.timeBeginPeriod, [wintypes.UINT], wintypes.UINT),
                  (timer.timeEndPeriod, [wintypes.UINT], wintypes.UINT))
    for function, arguments, result in signatures:  # 호출 ABI를 한 번 지정하고 이후 관측에서는 그대로 사용한다.
        function.argtypes, function.restype = arguments, result
    if hasattr(user, 'SetProcessDpiAwarenessContext'):
        user.SetProcessDpiAwarenessContext.argtypes = [ctypes.c_void_p]
        user.SetProcessDpiAwarenessContext.restype = wintypes.BOOL
        user.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
    return user, gdi, timer, enum_type


def owned(user, handle, pid):
    """매 접근 직전에 같은 생존 프로세스 소유 핸들인지 다시 확인한다."""
    owner = wintypes.DWORD()
    user.GetWindowThreadProcessId(handle, ctypes.byref(owner))
    return bool(handle) and owner.value == pid


def game_windows(user, enum_type, pid, for_close=False):
    """PID와 정확한 게임 클래스를 모두 확인하고 0핸들/분석기 창을 반환하지 않는다."""
    result = []

    @enum_type
    def visit(handle, _):
        """소유 PID를 먼저 읽고 게임 클래스인 창만 현재 크기와 함께 기록한다."""
        if not owned(user, handle, pid):
            return True
        name = ctypes.create_unicode_buffer(256)
        user.GetClassNameW(handle, name, len(name))
        if name.value != WINDOW_CLASS:
            return True
        rectangle = wintypes.RECT()
        if user.GetClientRect(handle, ctypes.byref(rectangle)):
            width, height = rectangle.right - rectangle.left, rectangle.bottom - rectangle.top
            iconic = bool(user.IsIconic(handle))
            if for_close or iconic or (0 < width <= 8192 and 0 < height <= 8192):
                title = ctypes.create_unicode_buffer(512)
                user.GetWindowTextW(handle, title, len(title))
                foreground = user.GetForegroundWindow()
                result.append({'handle': int(handle), 'size': [width, height], 'class': name.value,
                               'title': title.value, 'visible': bool(user.IsWindowVisible(handle)),
                               'foreground': foreground == handle, 'foregroundHandle': int(foreground or 0), 'isIconic': iconic})
        return True

    if not user.EnumWindows(visit, 0):
        raise ctypes.WinError(ctypes.get_last_error())
    return result


class ClientPixels:
    """데스크톱DC→client 사각형 BitBlt→top-down32bpp DIB만 사용하고 크기 변화 때 GDI 자원을 재생성한다."""
    def __init__(self, user, gdi):
        """이 도구가 소유하는 메모리 DC/bitmap만 초기화한다."""
        self.user, self.gdi = user, gdi  # Win32 조회/픽셀 읽기에만 사용하는 라이브러리다.
        self.dc = self.bitmap = self.previous = None  # 실제 client DC와 구별하는 소유 GDI 핸들이다.
        self.pointer = ctypes.c_void_p()  # CreateDIBSection이 제공하는 실제 BGRA 바이트 주소다.
        self.size = None  # 같은 크기의 연속 캡처는 하나의 DIB를 재사용한다.

    def close(self):
        """선택 객체를 복구한 뒤 도구 소유 bitmap/DC를 반납한다."""
        if self.dc and self.previous:
            self.gdi.SelectObject(self.dc, self.previous)
        if self.bitmap:
            self.gdi.DeleteObject(self.bitmap)
        if self.dc:
            self.gdi.DeleteDC(self.dc)
        self.dc = self.bitmap = self.previous = self.size = None

    def read(self, handle, size, pid):
        """주어진 소유 게임의 client 영역만 읽고 창을 활성화·이동·다시 그리지 않는다."""
        if not owned(self.user, handle, pid):
            return None
        origin = wintypes.POINT()  # 앱의 CS_OWNDC를 빌리지 않고 실제 데스크톱의 이 client 사각형만 읽는다.
        if not self.user.ClientToScreen(handle, ctypes.byref(origin)):
            return None
        source = self.user.GetDC(None)
        if not source:
            return None
        try:
            width, height = size
            if self.size != tuple(size):
                self.close()
                self.dc = self.gdi.CreateCompatibleDC(source)
                header = ctypes.create_string_buffer(struct.pack('<IiiHHIIiiII', 40, width, -height, 1, 32, 0, width * height * 4, 0, 0, 0, 0))
                self.bitmap = self.gdi.CreateDIBSection(source, header, 0, ctypes.byref(self.pointer), None, 0)
                if not self.dc or not self.bitmap or not self.pointer.value:
                    raise ctypes.WinError(ctypes.get_last_error())
                self.previous = self.gdi.SelectObject(self.dc, self.bitmap)
                if not self.previous or self.previous == ctypes.c_void_p(-1).value:
                    raise ctypes.WinError(ctypes.get_last_error())
                self.size = tuple(size)
            if not self.gdi.BitBlt(self.dc, 0, 0, width, height, source, origin.x, origin.y, SRCCOPY):
                return None
            self.gdi.GdiFlush()
            pixels = ctypes.string_at(self.pointer, width * height * 4)
            return Image.frombytes('RGB', (width, height), pixels, 'raw', 'BGRX', 0, 1)
        finally:
            self.user.ReleaseDC(None, source)


def match_scroll(image, reference):
    """실제 client 중앙과 설정/원본 초기 화면 중앙의 정확 RGB 비율을 각각 보존한다."""
    width, height = reference.size
    candidates = []
    seen = set()
    for basis, size in (('client', image.size), ('configured', (int(SETTINGS['SCREENW']), int(SETTINGS['SCREENH']))), ('nativeInitial640x480', (640, 480))):  # 창 초기화 중 논리 크기와 실제 client 크기가 다를 수 있다.
        x, y = (size[0] - width) // 2, (size[1] - height) // 2
        if (x, y) in seen or x < 0 or y < 0 or x + width > image.width or y + height > image.height:
            continue
        seen.add((x, y))
        difference = ImageChops.difference(image.crop((x, y, x + width, y + height)), reference)
        red, green, blue = difference.split()
        changed = ImageChops.lighter(ImageChops.lighter(red, green), blue)
        count = changed.histogram()[0]
        candidates.append({'basis': basis, 'rectangle': [x, y, x + width, y + height],
                           'equalRgbPixels': count, 'pixels': width * height, 'rgbMatchRatio': count / (width * height)})
    return max(candidates, key=lambda value: value['rgbMatchRatio']) if candidates else None


def text_pixels(image, scroll_rectangle):
    """두루마리를 제외한 우하단 흰 문자 픽셀의 실제 위치·줄 범위·마스크를 보존한다."""
    x0, y0 = max(0, image.width - 500), max(0, image.height - 180)
    crop = image.crop((x0, y0, image.width, image.height))
    red, green, blue = crop.split()
    minimum = ImageChops.darker(ImageChops.darker(red, green), blue)
    maximum = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    mask = ImageChops.multiply(minimum.point(lambda value: 255 if value >= 200 else 0),
                               ImageChops.subtract(maximum, minimum).point(lambda value: 255 if value <= 32 else 0))
    left, top, right, bottom = scroll_rectangle
    overlap = (max(0, left - x0), max(0, top - y0), min(crop.width, right - x0), min(crop.height, bottom - y0))
    if overlap[0] < overlap[2] and overlap[1] < overlap[3]:
        ImageDraw.Draw(mask).rectangle((overlap[0], overlap[1], overlap[2] - 1, overlap[3] - 1), fill=0)
    rectangle = mask.getbbox()
    bands = []
    for row in range(mask.height):  # 원문 글자를 OCR로 추측하지 않고 실제 불투명 픽셀의 연속 행만 묶는다.
        extent = mask.crop((0, row, mask.width, row + 1)).getbbox()
        if extent:
            if bands and bands[-1][3] == y0 + row:
                bands[-1] = [min(bands[-1][0], x0 + extent[0]), bands[-1][1], max(bands[-1][2], x0 + extent[2]), y0 + row + 1]
            else:
                bands.append([x0 + extent[0], y0 + row, x0 + extent[2], y0 + row + 1])
    return {'roi': [x0, y0, image.width, image.height],
            'whitePixelBox': [rectangle[0] + x0, rectangle[1] + y0, rectangle[2] + x0, rectangle[3] + y0] if rectangle else None,
            'rowBands': bands, 'whitePixels': mask.histogram()[255],
            'scope': '우하단 RGB>=200/chroma<=32 실제 픽셀이며 문자열 내용/OCR 확정이 아님'}, mask


def copy_config(path, game):
    """사본의 중복 InstallDir/CDDir까지 모두 바꾸고 외부 절대 저장 경로를 실행 전에 거부한다."""
    text = decode_file(str(path)) if path.exists() else ''
    for key, value in {**SETTINGS, 'InstallDir': str(game), 'CDDir': str(game)}.items():  # 일반 설정은 기존 첫 값 규칙을 유지한다.
        text = set_value(text, key, value)
    for key in ('InstallDir', 'CDDir'):  # 과거 서명 앞/중복 줄도 보호 경로를 남기지 않고 같은 사본으로 돌린다.
        text = re.sub(r'(?im)^([ \t]*)(' + key + r')[ \t]*=[^\r\n]*',
                      lambda match: match[1] + match[2] + ' = "' + str(game) + '"', text)
    roots = set()
    for match in re.finditer(r'(?im)^[ \t]*([^=\r\n]+)[ \t]*=[ \t]*([^\r\n]*)', text):  # 설정 내용은 출력하지 않고 실제 키와 경로 경계만 확인한다.
        key, raw = match[1].strip(), match[2].strip()
        value = raw[1:raw.find('"', 1)] if raw.startswith('"') and '"' in raw[1:] else raw.split('//', 1)[0].strip()
        if key.casefold() in ('installdir', 'cddir'):
            if Path(value).resolve() != game:
                raise RuntimeError('사본 루트 설정 재지정 실패: ' + key)
            roots.add(key.casefold())
        if re.match(r'^[a-zA-Z]:[\\/]|^\\\\', value) and not Path(value).resolve().is_relative_to(game):
            raise RuntimeError('사본 밖 절대 경로 설정은 실행 전에 거부함: ' + key)
    if roots != {'installdir', 'cddir'}:
        raise RuntimeError('사본의 InstallDir/CDDir 설정 누락')
    if str(SOURCE).casefold() in text.casefold() or SOURCE.as_posix().casefold() in text.casefold():
        raise RuntimeError('사본 설정에 보호 originals 절대 경로가 남음')
    if path.exists():
        path.chmod(path.stat().st_mode | stat.S_IWRITE)  # copytree가 물려준 사본의 읽기 전용 속성만 해제한다.
    path.write_bytes(encode_text(text))
    if decode_file(str(path)) != text:
        raise RuntimeError('사본 설정 인코딩 왕복 실패: ' + path.name)
    return {'file': path.name, 'sha256': sha(path), 'settingKeys': [*SETTINGS, 'InstallDir', 'CDDir'],
            'copyRootsVerified': True, 'outsideAbsolutePathsRejected': True}


def prepare_copy(directory, original, before, clone):
    """originals 전체를 각각 copytree한 뒤 사본 설정과 사본 실행 파일만 준비한다."""
    directory.mkdir()
    game = directory / 'game'
    shutil.copytree(SOURCE, game)
    if snapshot(game) != before:
        raise RuntimeError('전체 원본 사본이 실행 전 해시와 다름')
    configs = [copy_config(game / 'd' / name, game) for name in ('options.cfg', 'setup.cfg')]  # SessionStore.cs처럼 두 설정의 저장 루트를 함께 제한한다.
    fullscreen = game / 'fullscreenStateFile.dat'
    removed = fullscreen.exists()
    if removed:
        fullscreen.unlink()  # 이 경로는 새 copytree 하위의 사본이며 보호 원본은 지우지 않는다.
    executable = game / 'Netstorm.exe' if original else directory / 'NetstormCpp.exe'
    if not original:
        shutil.copy2(clone, executable)
    command = [str(executable), 'window'] if original else [str(executable), '--run', str(game), '--window', '--no-audio']
    if not executable.resolve().is_relative_to(directory.resolve()) or not game.resolve().is_relative_to(directory.resolve()):
        raise RuntimeError('게임 실행 파일 또는 자산이 새 사본 경계 밖임')
    return command, executable, game, {'configurations': configs, 'copiedFullscreenMarkerRemoved': removed}


def close_owned(process, user, enum_type):
    """소유 게임에 정상 WM_CLOSE를 먼저 보내고 남은 같은 프로세스만 제한 시간 뒤 종료한다."""
    result = {'closeHandles': [], 'terminateUsed': False, 'killUsed': False}
    if process.poll() is None:
        try:
            windows = game_windows(user, enum_type, process.pid, for_close=True)
        except OSError as error:
            result['closeWindowQueryError'] = repr(error)
            windows = []
        for window in windows:  # 다른 PID/분석기/0핸들에는 종료 메시지를 보내지 않는다.
            handle = window['handle']
            if owned(user, handle, process.pid):
                result['closeHandles'].append(handle)
                user.PostMessageW(handle, WM_CLOSE, 0, 0)
        try:
            process.wait(timeout=1.5)
        except subprocess.TimeoutExpired:
            result['terminateUsed'] = True
            process.terminate()
            try:
                process.wait(timeout=0.5)
            except subprocess.TimeoutExpired:
                result['killUsed'] = True
                process.kill()
                try:
                    process.wait(timeout=0.5)
                except subprocess.TimeoutExpired:
                    result['exitWaitTimedOut'] = True
    result.update(returnCode=process.returncode, normalExit=process.returncode == 0 and not result['terminateUsed'])
    return result


def observe(directory, command, executable, cwd, reference, seconds, interval_ms, threshold, bindings):
    """시작 직후5~10ms 폴링을 시작하고 unique 소수 프레임만 메모리에 보관해 종료 뒤 PNG로 저장한다."""
    user, gdi, timer, enum_type = bindings
    result = {'command': command, 'binarySha256': sha(executable), 'cwd': str(cwd), 'frames': [], 'uniqueFrames': [], 'windowStates': [],
              'requestedPollMs': interval_ms, 'captureSecondsLimit': seconds, 'closeSecondsLimit': 2.5,
              'rgbMatchThreshold': threshold, 'captureApi': 'GetDC(desktop)/client rectangle BitBlt(SRCCOPY)/top-down32bpp DIB',
              'normalCloseRequestedFirst': True, 'windowClass': WINDOW_CLASS,
              'uniqueMetadataLimit': UNIQUE_METADATA_LIMIT, 'nonLoadingRetention': 'first six unique frames; first four loading frames have priority',
              'foregroundRequested': False, 'paintOrPrintWindowRequested': False}
    retained, cache = [], {}
    window_states = {}  # 같은 RGB여도 visible/foreground/최소화 전환은 별도 상태로 보존한다.
    window_state_changes = 0
    count, failed, maximum_gap = 0, 0, 0
    loading_count = 0
    first_loading = previous_poll = None
    pixels = ClientPixels(user, gdi)
    period = timer.timeBeginPeriod(1) == 0
    start = time.perf_counter_ns()
    result['startedUnixNs'] = time.time_ns()
    process = None
    try:
        with (directory / 'stdout.log').open('wb') as stdout, (directory / 'stderr.log').open('wb') as stderr:
            process = subprocess.Popen(command, cwd=cwd, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = process.pid
            save(directory / 'running.json', {key: result[key] for key in ('pid', 'command', 'binarySha256', 'startedUnixNs')})
            deadline = start + int(seconds * 1_000_000_000)
            while process.poll() is None and time.perf_counter_ns() < deadline:  # 메뉴가 준비되어도 상한을 연장하지 않고 프로세스 시작부터 센다.
                polling = time.perf_counter_ns()
                if previous_poll is not None:
                    maximum_gap = max(maximum_gap, polling - previous_poll)
                previous_poll = polling
                for window in game_windows(user, enum_type, process.pid):  # 매 시점 현재 PID 소유 게임만 다시 조회한다.
                    began = time.perf_counter_ns()
                    state_key = (tuple(window['size']), window['visible'], window['foreground'], window['isIconic'])
                    if window_states.get(window['handle']) != state_key:
                        window_states[window['handle']] = state_key
                        window_state_changes += 1
                        if len(result['windowStates']) < UNIQUE_METADATA_LIMIT:
                            result['windowStates'].append(dict(window, observedMs=(began - start) / 1_000_000))
                    if window['isIconic']:
                        continue  # 최소화 상태는 메타데이터만 남기고 유효 화면 픽셀로 오인하지 않는다.
                    image = pixels.read(window['handle'], window['size'], process.pid)
                    ended = time.perf_counter_ns()
                    if image is None:
                        failed += 1
                        continue
                    count += 1
                    identity = hashlib.sha256(image.tobytes()).hexdigest()
                    cache_key = (image.size, identity)
                    if cache_key not in cache:
                        match = match_scroll(image, reference)
                        unique = dict(window, rgbSha256=identity, scrollMatch=match,
                                      firstCaptureBeginMs=(began - start) / 1_000_000,
                                      firstCaptureEndMs=(ended - start) / 1_000_000,
                                      captureCount=0, everVisible=False, everForeground=False)
                        cache[cache_key] = unique
                        if len(result['uniqueFrames']) < UNIQUE_METADATA_LIMIT:
                            result['uniqueFrames'].append(unique)
                        best = result.get('bestScrollFrame')
                        if match and (not best or match['rgbMatchRatio'] > best['scrollMatch']['rgbMatchRatio']):
                            result['bestScrollFrame'] = dict(unique)
                    unique = cache[cache_key]
                    unique.update(captureCount=unique['captureCount'] + 1, lastCaptureEndMs=(ended - start) / 1_000_000,
                                  lastVisible=window['visible'], everVisible=unique['everVisible'] or window['visible'],
                                  lastForeground=window['foreground'], everForeground=unique['everForeground'] or window['foreground'])
                    match = unique['scrollMatch']
                    loading = bool(match and match['rgbMatchRatio'] >= threshold)
                    unique['identifiedLoading'] = loading
                    if loading and first_loading is None:
                        first_loading = ended
                    if not any(frame[1]['rgbSha256'] == identity and frame[1]['size'] == list(image.size) for frame in retained):
                        metadata = dict(window, rgbSha256=identity, identifiedLoading=loading, scrollMatch=match,
                                        firstCaptureBeginMs=unique['firstCaptureBeginMs'], firstCaptureEndMs=unique['firstCaptureEndMs'],
                                        captureBeginMs=(began - start) / 1_000_000, captureEndMs=(ended - start) / 1_000_000)
                        if loading and loading_count < 4:
                            if len(retained) == PNG_LIMIT:
                                replace = next(index for index in range(len(retained) - 1, -1, -1) if not retained[index][1]['identifiedLoading'])  # 나중 비로딩부터 바꿔 최초 visible 구간을 가능한 한 남긴다.
                                retained[replace] = (image, dict(metadata, retentionRole='loading-priority'))
                            else:
                                retained.append((image, dict(metadata, retentionRole='loading-priority')))
                            loading_count += 1
                        elif not loading and len(retained) < PNG_LIMIT:
                            retained.append((image, dict(metadata, retentionRole='startup-unique')))
                    if first_loading and not loading and ended - first_loading >= 250_000_000:
                        deadline = ended  # 실제 로딩을 본 뒤 다른 화면으로 넘어가면 추가 탐험을 하지 않는다.
                remaining = interval_ms / 1000 - (time.perf_counter_ns() - polling) / 1_000_000_000
                if remaining > 0:
                    time.sleep(remaining)
    except Exception as error:
        result['exception'] = repr(error)
    finally:
        if process is not None:
            result.update(close_owned(process, user, enum_type))
        pixels.close()
        if period:
            timer.timeEndPeriod(1)
        result.update(seconds=(time.perf_counter_ns() - start) / 1_000_000_000, captureCount=count,
                      captureFailures=failed, uniqueRgbFrames=len(cache), maximumPollGapMs=maximum_gap / 1_000_000,
                      uniqueMetadataDropped=max(0, len(cache) - UNIQUE_METADATA_LIMIT),
                      windowStateMetadataDropped=max(0, window_state_changes - UNIQUE_METADATA_LIMIT), loadingObserved=first_loading is not None)
        for index, (image, metadata) in enumerate(sorted(retained, key=lambda frame: frame[1]['captureBeginMs'])):  # PNG 압축은 종료 뒤에 수행하며 교체한 후반 프레임도 실제 캡처 시간순으로 저장한다.
            name = f'{index:02d}-' + ('loading' if metadata['identifiedLoading'] else 'other')
            png = directory / (name + '.png')
            image.save(png)
            metadata.update(png=png.name, pngSha256=sha(png))
            if metadata['identifiedLoading']:
                metrics, mask = text_pixels(image, metadata['scrollMatch']['rectangle'])
                mask_path = directory / (name + '-text-mask.png')
                mask.save(mask_path)
                metadata['textPixels'] = dict(metrics, maskPng=mask_path.name, maskSha256=sha(mask_path))
            result['frames'].append(metadata)
        result.update(stdoutSha256=sha(directory / 'stdout.log'), stderrSha256=sha(directory / 'stderr.log'))
        save(directory / 'run.json', result)
    if len(retained) > PNG_LIMIT:
        raise RuntimeError('프레임 저장 상한 초과')
    return result


def main():
    """새 extracted 경로와 사본만 사용하며 실제 원본 실행은 --original로 명시한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='아직 없는 extracted 하위 출력 폴더')
    parser.add_argument('--exe', type=Path, default=CLONE, help='실행 전 사본을 만들 클론 exe')
    parser.add_argument('--original', action='store_true', help='클론 뒤 보호 원본의 전체 사본도 실행')
    parser.add_argument('--width', type=int, default=1024, help='사본 설정의 내부 화면 가로 크기')
    parser.add_argument('--height', type=int, default=768, help='사본 설정의 내부 화면 세로 크기')
    parser.add_argument('--seconds', type=float, default=10, help='프로세스별 캡처 상한5~12초; 종료 대기 최대2.5초 추가')
    parser.add_argument('--poll-ms', type=float, default=5, help='PID/게임 창 조회 요청 간격5~10ms')
    parser.add_argument('--match-ratio', type=float, default=0.97, help='두루마리 전체 정확 RGB 일치율')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('이 실제 client DC 관측은 Windows에서만 실행할 수 있습니다')
    if not 5 <= args.seconds <= 12 or not 5 <= args.poll_ms <= 10 or not 0.9 <= args.match_ratio <= 1:
        parser.error('seconds=5..12, poll-ms=5..10, match-ratio=0.9..1 범위가 필요합니다')
    if not 640 <= args.width <= 4096 or not 480 <= args.height <= 2160:
        parser.error('내부 화면 크기는 가로640..4096·세로480..2160이어야 합니다')
    SETTINGS.update(SCREENW=str(args.width), SCREENH=str(args.height))  # 새 사본의 설정과 독립 중앙 판정을 같은 요청 크기로 제한한다.
    directory, clone = args.output.resolve(), args.exe.resolve()
    if not directory.is_relative_to((ROOT / 'extracted').resolve()) or directory == (ROOT / 'extracted').resolve():
        parser.error('출력은 저장소 extracted의 새 하위 폴더여야 합니다')
    if not clone.is_file():
        parser.error('클론 실행 파일 없음: ' + str(clone))
    directory.mkdir(parents=True, exist_ok=False)
    before = snapshot(SOURCE)
    save(directory / 'originals-before.json', before)
    summary = {'originalRequested': args.original, 'runs': [], 'toolSha256': sha(Path(__file__)),
               'scope': '실제 시작 로딩 픽셀 관측이며 전체 GUI/텍스트 내용/원본 동일성 완료 판정이 아님'}
    try:
        reference, summary['reference'] = scroll_bitmap(SOURCE / 'Netstorm.exe')
        reference.save(directory / 'reference-147.png')
        bindings = api()
        for original in ((False, True) if args.original else (False,)):  # 원본 사본은 명시 옵션이 있을 때만 생성·실행한다.
            case = directory / ('original' if original else 'clone')
            command, executable, game, settings = prepare_copy(case, original, before, clone)
            result = observe(case, command, executable, game, reference, args.seconds, args.poll_ms, args.match_ratio, bindings)
            result['copySettings'] = settings
            save(case / 'run.json', result)
            summary['runs'].append(result)
    except Exception as error:
        summary['exception'] = repr(error)
    finally:
        after = snapshot(SOURCE)
        save(directory / 'originals-after.json', after)
        summary['originalsUnchanged'] = before == after
        summary['captureSucceeded'] = len(summary['runs']) == (2 if args.original else 1) and all(row['loadingObserved'] and not row.get('exception') and not row.get('exitWaitTimedOut') and (row['returnCode'] == 0 or row['terminateUsed']) for row in summary['runs'])
        save(directory / 'summary.json', summary)
    print(json.dumps({'output': str(directory), 'loadingObserved': summary['captureSucceeded'],
                      'originalsUnchanged': summary['originalsUnchanged']}, ensure_ascii=False))
    return 0 if summary['captureSucceeded'] and summary['originalsUnchanged'] and not summary.get('exception') else 1


if __name__ == '__main__':
    raise SystemExit(main())
