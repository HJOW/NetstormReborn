#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 원본을 건드리지 않고 설정 복원의 일시 실패·최종 실패·복구 원시 자료를 검사한다."""
from contextlib import ExitStack, redirect_stderr
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from cpp_smoke_files import preserve_game_settings


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


if __name__ == '__main__':
    unittest.main()
