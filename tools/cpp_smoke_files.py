#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 검사 동안 원본 설정을 보관하고 성공·실패 모두 원래 바이트와 존재 상태로 되돌린다."""
from contextlib import contextmanager
from pathlib import Path


@contextmanager
def preserve_game_settings(root, data_dir):
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
    try:
        yield options_path
    finally:
        # 실패·예외에도 복구한다. 원래 없던 파일은 이 두 경로에 한해서 지운다.
        for path, original in originals.items():
            if original is not None:
                path.write_bytes(original)
            elif path.exists():
                path.unlink()

