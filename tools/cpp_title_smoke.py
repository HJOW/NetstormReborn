#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 클론의 메뉴/브리핑 창 제목을 Win32에서 읽는다. 원본 게임은 실행하지 않는다."""
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import time
from cpp_smoke_files import preserve_game_settings

ROOT = Path(__file__).resolve().parent.parent  # 현재 저장소/실행 파일/검사 산출물의 기준이다.
EXE = ROOT/'cpppj/build/bin/Release/Netstorm.exe'  # 이번 Release 빌드의 클론만 실행한다.
OUT = ROOT/'extracted/run27-title-ui.json'  # 실제 창 관찰과 종료 결과를 보관한다.
USER = ctypes.WinDLL('user32', use_last_error=True)  # 현재 검사 프로세스와 같은 데스크톱의 창 조회/종료 경계다.
ENUM = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)  # 보이지 않는 창도 포함하는 Win32 열거 콜백 형식이다.
USER.EnumWindows.argtypes = [ENUM, wintypes.LPARAM]
USER.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
USER.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
USER.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]


def windows(pid):
    """검사에서 시작한 한 프로세스의 현재 제목/핸들을 직접 조회한다."""
    result = []  # 이번 열거에서 찾은 창만 반환하며 다른 사용자 창은 조작하지 않는다.

    @ENUM
    def visit(handle, _):
        """소유 PID를 먼저 확인하고 일치하는 창만 읽는다."""
        owner = wintypes.DWORD()  # Win32가 기록할 실제 창 소유 프로세스 번호다.
        USER.GetWindowThreadProcessId(handle, ctypes.byref(owner))
        if owner.value == pid:
            text = ctypes.create_unicode_buffer(1024)  # 원본 256BYTE 제목을 읽기에 충분한 관찰 버퍼다.
            USER.GetWindowTextW(handle, text, len(text))
            result.append((handle, text.value))
        return True

    USER.EnumWindows(visit, 0)
    return result


def check(name, title, extra):
    """제목 적용을 관찰한 다음 자체 검사 창에 WM_CLOSE를 보내 정상 종료한다."""
    startup = subprocess.STARTUPINFO()  # 검사 시작 시 콘솔/게임 창의 불필요한 전면 표시를 줄인다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    command = [str(EXE), '--run', str(ROOT/'originals'), '--window', '--no-audio', '--frames', '1000000', *extra]  # 정지 브리핑의 빠른 프레임 때문에 제목 적용 전에 종료하지 않게 충분한 상한을 둔다.
    process = subprocess.Popen(command, cwd=ROOT, startupinfo=startup, stdout=subprocess.PIPE, stderr=subprocess.PIPE)  # 이 도구가 소유한 검사 프로세스다.
    seen, matched = [], False  # 실제 관찰 제목 순서와 기대 제목 일치 여부다.
    deadline = time.monotonic()+30  # 초기화 실패나 창 누락 때 검사 자체가 남지 않게 제한한다.
    try:
        # 프로세스가 살아 있는 동안 실제 창을 확인하며 .NET MainWindow의 가시성 필터를 사용하지 않는다.
        while process.poll() is None and time.monotonic() < deadline:
            # 자체 검사 PID에 속한 모든 현재 창의 제목 적용 여부를 확인한다.
            for handle, value in windows(process.pid):
                if value not in seen:
                    seen.append(value)
                if value == title:
                    matched = True
                    USER.PostMessageW(handle, 0x10, 0, 0)
            if matched:
                break
            time.sleep(0.01)
        stdout, stderr = process.communicate(timeout=10)  # WM_CLOSE의 실제 종료 코드도 검사한다.
        assert matched and process.returncode == 0, (name, seen, process.returncode, stderr.decode('utf-8', errors='replace'))
        return dict(case=name, expected=title, observed=seen, exitCode=process.returncode, stdout=stdout.decode('utf-8', errors='replace'))
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()


def main():
    """원본 설정은 허용된 두 파일에 한해 보관/복구하며 두 실제 창 제목을 확인한다."""
    results = []  # 성공한 실제 클론 프로세스의 개별 관찰이다.
    with preserve_game_settings(ROOT/'originals', 'd'):
        # 실제 원본 설정에서 만드는 메뉴/미션 제목을 같은 설정 경로의 CLI로 읽는다.
        for name, key, argument, extra in (
            ('menu', 'blankWindowName', None, []),
            ('mission', 'missionWindowName', 'Bridge the Gap!', ['--mission', 'tutorial1']),
        ):
            command = [str(EXE), '--config-spec', str(ROOT/'originals'), key]  # 설정 인자 NULL과 미션 제목 문자열을 구분한다.
            if argument is not None:
                command.append(argument)
            expected = subprocess.check_output(command, cwd=ROOT).decode('utf-8')  # 독립 원본 fixture와 별도로 실제 설정 치환/창 연결을 확인한다.
            results.append(check(name, expected, extra))
            print(name, repr(expected), 'Win32 제목 PASS', flush=True)
    OUT.write_text(json.dumps(results, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
