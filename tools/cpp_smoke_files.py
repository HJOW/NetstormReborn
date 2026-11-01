#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 검사 동안 원본 설정을 보관하고 성공·실패 모두 원래 바이트와 존재 상태로 되돌린다."""
import atexit
from contextlib import contextmanager
import ctypes
import os
import sys
import tempfile
import time
from pathlib import Path

# 자동 검사가 소리 장치를 열거나 음악을 내지 않게 한다(NetstormCpp --run이 읽는 환경 변수). 소리를 일부러 검사하는 도구만 이 값을 지운다.
os.environ.setdefault('NETSTORM_CPP_NO_AUDIO', '1')

ERROR_USER_MAPPED_FILE = 1224  # 매핑된 파일을 자를 때의 Windows 오류다. git diff 계열이 수정된 추적 파일을 매핑한 동안 실제로 발생한다(단위675).
replaced_settings = []  # 성공한 교체 중 이전 파일이 아직 매핑돼 지우지 못한 폴더다. collect_replaced_settings가 다시 정리한다(단위675).


def crt_dos_error():
    """현재 스레드 CRT의 _doserrno 저장 위치를 돌려준다. OSError에는 errno 22만 남아 1224를 구별할 수 없기 때문이다(단위675)."""
    location = ctypes.CDLL('ucrtbase').__doserrno
    location.restype = ctypes.POINTER(ctypes.c_ulong)
    return location().contents


def collect_replaced_settings():
    """성공한 교체의 임시 폴더만 지운다. 아직 매핑된 폴더는 목록에 남겨 다음 호출과 프로세스 종료 때 다시 시도한다(단위675)."""
    # 이번 프로세스가 만든 명시 경로만 다루며 다른 폴더를 열거하거나 재귀 삭제하지 않는다.
    for directory in list(replaced_settings):
        try:
            # 교체가 끝난 뒤에는 next가 없고 previous만 남는 것이 정상이다.
            for name in ('next', 'previous'):
                (directory / name).unlink(missing_ok=True)
            directory.rmdir()
            replaced_settings.remove(directory)
        except OSError as error:
            print(f'설정 교체 임시 폴더 정리 보류 {directory}: {error!r}', file=sys.stderr)


atexit.register(collect_replaced_settings)


def replace_mapped_settings(path, data):
    """매핑 때문에 자를 수 없는 파일을 완성된 새 파일로 교체한다. 제품 ClientOptionsDisk의 1224 경로와 같은 방법이다(단위675)."""
    # os.replace는 매핑 중인 대상에서 오류5로 거부되므로 ReplaceFileW를 쓴다.
    directory = Path(tempfile.mkdtemp(prefix=f'{path.name}.restore-', dir=path.parent))
    next_file = directory / 'next'
    with open(next_file, 'wb') as output:
        output.write(data)
        output.flush()
        os.fsync(output.fileno())
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    if not kernel32.ReplaceFileW(str(path), str(next_file), str(directory / 'previous'), 0, None, None):
        error = ctypes.WinError(ctypes.get_last_error())  # 아래 출력이 마지막 오류를 바꾸기 전에 보관한다.
        print(f'설정 교체 실패, 복구 폴더 보존: {directory}', file=sys.stderr)
        raise error
    replaced_settings.append(directory)
    collect_replaced_settings()


@contextmanager
def preserve_game_settings(root, data_dir, backup_dir=None):
    """AGENTS.md가 허용한 options.cfg·fullscreenStateFile.dat만 복구한다. 다른 원본 파일에는 쓰지 않는다."""
    game_root = Path(root).resolve()
    options_path = game_root / data_dir / 'options.cfg'
    paths = [options_path, game_root / 'fullscreenStateFile.dat']
    originals = {}
    # 허용한 두 이름의 내용과 존재 여부를 실행 전에 모두 보관한다.
    for path in paths:
        if path.is_symlink() or path.resolve().parent != path.parent.resolve():
            raise ValueError(f'설정 파일이 원래 폴더 밖을 가리킴: {path.name}')
        originals[path] = path.read_bytes() if path.exists() else None
    if backup_dir is not None:
        backup = Path(backup_dir).resolve()  # 실패 뒤에도 실행 전 원시 바이트를 복구할 수 있는 검사 산출물 경로다.
        backup.mkdir(parents=True, exist_ok=False)
        # 원래 없던 파일은 별도 표식으로 구별해 빈 파일과 혼동하지 않는다.
        for path, original in originals.items():
            if original is None:
                (backup / (path.name + '.absent')).write_bytes(b'')
            else:
                (backup / path.name).write_bytes(original)
    try:
        yield options_path
    finally:
        restore_error = None  # 한 설정의 실패 때문에 다른 허용 설정의 복원까지 생략하지 않는다.
        # 실패·예외에도 복구한다. 원래 없던 파일은 이 두 경로에 한해서 지운다.
        for path, original in originals.items():
            mapped = False  # 직전 실패가 실제 1224였는지 기록한다. 원본 폴더에 임시 폴더를 만드는 교체를 마지막 시도로 미룬다(단위675).
            # 일시적인 OS 쓰기 실패는 기록하고 제한 재시도한다. 최종 실패를 성공으로 숨기지 않는다.
            for attempt in range(5):
                crt_dos_error().value = 0  # 앞선 다른 실패의 값을 이번 오류로 오인하지 않게 지운다.
                try:
                    if mapped and attempt == 4:
                        replace_mapped_settings(path, original)
                    elif original is not None:
                        path.write_bytes(original)
                    elif path.exists():
                        path.unlink()
                    break
                except OSError as error:
                    dos_error = crt_dos_error().value
                    mapped = original is not None and dos_error == ERROR_USER_MAPPED_FILE
                    print(f'설정 복원 실패 {path.name} ({attempt + 1}/5): {error!r} doserrno={dos_error}', file=sys.stderr)
                    if attempt == 4:
                        restore_error = error
                        break
                    time.sleep(0.1)
        if restore_error is not None:
            raise restore_error

