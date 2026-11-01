#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 원본을 건드리지 않고 설정 복원의 일시 실패·최종 실패·복구 원시 자료를 검사한다."""
from contextlib import ExitStack, redirect_stderr
import ctypes
from ctypes import wintypes
import io
import msvcrt
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from cpp_smoke_files import collect_replaced_settings, preserve_game_settings

VOLUMES = (tempfile.gettempdir(), str(Path(__file__).resolve().parents[1] / 'extracted'))  # C:(NTFS)와 Git 제외 H:(exFAT)의 실제 쓰기 경계다.
kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)  # 아래 형식 선언이 다른 모듈의 kernel32에 번지지 않는 전용 인스턴스다.
kernel32.CreateFileMappingW.restype = wintypes.HANDLE
kernel32.CreateFileMappingW.argtypes = (wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.LPCWSTR)
kernel32.MapViewOfFile.restype = wintypes.LPVOID
kernel32.MapViewOfFile.argtypes = (wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t)
kernel32.UnmapViewOfFile.argtypes = (wintypes.LPCVOID,)
kernel32.CloseHandle.argtypes = (wintypes.HANDLE,)


class FileView:
    """git의 mmap처럼 파일 핸들 없이 읽기 전용 보기만 남긴다. mmap 모듈은 복제 핸들을 유지해 공유 조건이 달라지므로 쓰지 않는다."""

    def __init__(self, path):
        """파일을 잠깐 열어 매핑한 뒤 핸들을 모두 닫는다."""
        with open(path, 'rb') as stream:
            mapping = kernel32.CreateFileMappingW(msvcrt.get_osfhandle(stream.fileno()), None, 2, 0, 0, None)  # 2=PAGE_READONLY
            self.address = kernel32.MapViewOfFile(mapping, 4, 0, 0, 0)  # 4=FILE_MAP_READ
        kernel32.CloseHandle(mapping)

    def read(self, length):
        """매핑이 계속 보는 이전 바이트를 돌려준다."""
        return ctypes.string_at(self.address, length)

    def close(self):
        """보기를 반납한다. 두 번 불러도 안전해야 finally에서 쓸 수 있다."""
        if self.address:
            kernel32.UnmapViewOfFile(self.address)
            self.address = None


class RealMappingTests(unittest.TestCase):
    """모의 예외가 아닌 실제 매핑의 1224로 재시도·교체·지연 정리를 두 볼륨에서 검증한다(단위675)."""

    def setUp(self):
        """시험 중 만든 매핑을 끝에 모두 반납하도록 등록한다."""
        self.views = []  # 예외 경로에서도 매핑을 반납해 임시 폴더 삭제가 막히지 않게 한다.
        self.addCleanup(lambda: [view.close() for view in self.views])

    def view(self, path):
        """시험 끝에 반드시 반납되는 실제 매핑을 만든다."""
        view = FileView(path)
        self.views.append(view)
        return view

    def test_mapping_held_to_the_end_is_replaced(self):
        """매핑이 끝까지 남으면 마지막 시도에서 교체하고, 이전 파일은 매핑 반납 뒤에 정리한다."""
        # 두 볼륨을 각각 실제로 시험한다.
        for volume in VOLUMES:
            with self.subTest(volume=volume), tempfile.TemporaryDirectory(dir=volume) as directory:
                root = Path(directory)
                (root / 'd').mkdir()
                options = root / 'd/options.cfg'
                options.write_bytes(b'original\x00\xff')
                messages = io.StringIO()
                with redirect_stderr(messages), patch('cpp_smoke_files.time.sleep'):
                    with preserve_game_settings(root, 'd', root / 'backup'):
                        options.write_bytes(b'changed!!!')
                        view = self.view(options)
                self.assertEqual(options.read_bytes(), b'original\x00\xff')
                self.assertEqual(view.read(10), b'changed!!!')  # 교체가 매핑을 깨뜨리지 않았다.
                self.assertEqual(messages.getvalue().count('doserrno=1224'), 4)
                folders = list((root / 'd').glob('options.cfg.restore-*'))
                self.assertEqual(len(folders), 1)
                self.assertEqual((folders[0] / 'previous').read_bytes(), b'changed!!!')  # 매핑이 살아 있는 동안 삭제가 거부된 이전 바이트다.
                view.close()
                with redirect_stderr(io.StringIO()):
                    collect_replaced_settings()
                self.assertEqual(list((root / 'd').glob('options.cfg.restore-*')), [])

    def test_mapping_released_during_retry_needs_no_replacement(self):
        """재시도 중 매핑이 풀리면 일반 쓰기로 복원하고 원본 폴더에 임시 폴더를 만들지 않는다."""
        # 두 볼륨을 각각 실제로 시험한다.
        for volume in VOLUMES:
            with self.subTest(volume=volume), tempfile.TemporaryDirectory(dir=volume) as directory:
                root = Path(directory)
                (root / 'd').mkdir()
                options = root / 'd/options.cfg'
                options.write_bytes(b'original')
                messages = io.StringIO()
                released = []  # 두 번째 실패 뒤의 대기에서 매핑을 푼다.

                def release(_seconds):
                    """git 프로세스가 끝나는 시점을 대기 호출에 맞춘다."""
                    released.append(True)
                    if len(released) == 2:
                        view.close()

                with redirect_stderr(messages), patch('cpp_smoke_files.time.sleep', release):
                    with preserve_game_settings(root, 'd', root / 'backup'):
                        options.write_bytes(b'changed!')
                        view = self.view(options)
                self.assertEqual(options.read_bytes(), b'original')
                self.assertEqual(messages.getvalue().count('doserrno=1224'), 2)
                self.assertEqual(list((root / 'd').glob('options.cfg.restore-*')), [])

    def test_other_error_after_mapping_does_not_replace(self):
        """실제 1224 뒤에 다른 errno 22가 이어지면 낡은 _doserrno로 교체하지 않고 오류를 전달한다."""
        # 두 볼륨을 각각 실제로 시험한다.
        for volume in VOLUMES:
            with self.subTest(volume=volume), tempfile.TemporaryDirectory(dir=volume) as directory:
                root = Path(directory)
                (root / 'd').mkdir()
                options = root / 'd/options.cfg'
                options.write_bytes(b'original')
                write = Path.write_bytes
                calls = []  # 첫 요청만 실제 매핑 거부를 받고 나머지는 CRT를 거치지 않는 다른 실패다.

                def real_then_other(path, data):
                    """CRT 파일 호출이 _doserrno를 지울 기회 없이 다른 실패로 이어지는 순서를 만든다."""
                    calls.append(path)
                    if len(calls) == 1:
                        return write(path, data)
                    raise OSError(22, 'fixture other')

                with redirect_stderr(io.StringIO()), patch('cpp_smoke_files.time.sleep'), ExitStack() as mocks:
                    with self.assertRaisesRegex(OSError, 'fixture other'):
                        with preserve_game_settings(root, 'd', root / 'backup'):
                            options.write_bytes(b'changed!')
                            self.view(options)
                            mocks.enter_context(patch.object(Path, 'write_bytes', real_then_other))
                self.assertEqual(options.read_bytes(), b'changed!')
                self.assertEqual(list((root / 'd').glob('options.cfg.restore-*')), [])
                self.views.pop().close()

    def test_refused_replacement_keeps_recovery_folder(self):
        """공유 거부로 교체까지 실패하면 오류를 전달하고 변경된 파일과 원래 바이트를 복구 폴더에 보존한다."""
        # 두 볼륨을 각각 실제로 시험한다.
        for volume in VOLUMES:
            with self.subTest(volume=volume), tempfile.TemporaryDirectory(dir=volume) as directory:
                root = Path(directory)
                (root / 'd').mkdir()
                options = root / 'd/options.cfg'
                options.write_bytes(b'original')
                messages = io.StringIO()
                with redirect_stderr(messages), patch('cpp_smoke_files.time.sleep'):
                    with self.assertRaises(OSError):
                        with preserve_game_settings(root, 'd', root / 'backup'):
                            options.write_bytes(b'changed!')
                            self.view(options)
                            refusing = open(options, 'rb')  # FILE_SHARE_DELETE 없는 핸들이 ReplaceFileW를 거부시킨다.
                            self.addCleanup(refusing.close)
                self.assertEqual(options.read_bytes(), b'changed!')
                folders = list((root / 'd').glob('options.cfg.restore-*'))
                self.assertEqual(len(folders), 1)
                self.assertEqual((folders[0] / 'next').read_bytes(), b'original')
                self.assertIn('복구 폴더 보존', messages.getvalue())
                self.assertEqual((root / 'backup/options.cfg').read_bytes(), b'original')
                refusing.close()  # 임시 폴더를 지울 수 있게 다음 볼륨 전에 반납한다.
                self.views.pop().close()


class SettingsRecoveryTests(unittest.TestCase):
    """격리된 임시 두 파일만 사용해 게임 실행 없는 실패 경계를 검증한다."""

    def test_transient_failure_restores_original_and_absence(self):
        """첫 복원 쓰기 실패를 기록하고 재시도해 원래 바이트와 없는 파일 상태를 되돌린다."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'd').mkdir()
            options = root / 'd/options.cfg'
            options.write_bytes(b'original\x00\xff')
            fullscreen = root / 'fullscreenStateFile.dat'
            write = Path.write_bytes
            failures = []  # 복원 첫 요청 하나에만 명시 OS 실패를 주입한다.

            def transient(path, data):
                """원시 백업 쓰기는 허용하고 실제 복원 대상의 첫 쓰기만 거부한다."""
                if path == options and not failures:
                    failures.append(path)
                    raise OSError(22, 'fixture transient')
                return write(path, data)

            messages = io.StringIO()
            with redirect_stderr(messages), patch('cpp_smoke_files.time.sleep'), ExitStack() as mocks:
                with preserve_game_settings(root, 'd', root / 'backup'):
                    options.write_bytes(b'changed')
                    fullscreen.write_bytes(b'created')
                    mocks.enter_context(patch.object(Path, 'write_bytes', transient))
            self.assertEqual(options.read_bytes(), b'original\x00\xff')
            self.assertFalse(fullscreen.exists())
            self.assertEqual((root / 'backup/options.cfg').read_bytes(), b'original\x00\xff')
            self.assertTrue((root / 'backup/fullscreenStateFile.dat.absent').exists())
            self.assertEqual(len(failures), 1)
            self.assertIn('fixture transient', messages.getvalue())

    def test_final_failure_keeps_backup_and_restores_other_file(self):
        """상한까지 실패하면 오류를 유지하면서 다른 설정을 복구하고 원시 백업을 남긴다."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'd').mkdir()
            options = root / 'd/options.cfg'
            options.write_bytes(b'original')
            fullscreen = root / 'fullscreenStateFile.dat'
            fullscreen.write_bytes(b'fullscreen original')
            write = Path.write_bytes

            def permanent(path, data):
                """두 설정 중 options 복원만 실패시켜 다른 파일의 finally 실행을 확인한다."""
                if path == options:
                    raise OSError(22, 'fixture permanent')
                return write(path, data)

            with redirect_stderr(io.StringIO()), patch('cpp_smoke_files.time.sleep'), ExitStack() as mocks:
                with self.assertRaisesRegex(OSError, 'fixture permanent'):
                    with preserve_game_settings(root, 'd', root / 'backup'):
                        options.write_bytes(b'changed')
                        fullscreen.write_bytes(b'changed')
                        mocks.enter_context(patch.object(Path, 'write_bytes', permanent))
            self.assertEqual(fullscreen.read_bytes(), b'fullscreen original')
            self.assertEqual((root / 'backup/options.cfg').read_bytes(), b'original')
            self.assertEqual(list((root / 'd').glob('options.cfg.restore-*')), [])  # 1224가 아닌 errno 22는 교체로 넘기지 않는다.


if __name__ == '__main__':
    unittest.main()
