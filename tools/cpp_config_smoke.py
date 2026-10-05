#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 C++ 설정 계층을 실제 두 판본의 설정 파일로 검사한다. 원본 게임은 실행하지 않는다.

- 시작 순서로 만든 설정 버퍼를 Python이 원본 파일에서 독립적으로 구성한 버퍼와 바이트 단위로 비교한다.
- 미션·요새·언어·팔레트 경로 지정값을 계산하고 그 경로의 파일이 실제로 읽히는지 확인한다.
- 값을 쓴 뒤 저장한 파일을 기존 Python 도구(nscfg.py)로 복호화해 내용을 확인한다.
- 원본 폴더에서는 options.cfg만 저장되는지 확인하고, 검사 뒤 설정을 복구한다.

python tools/cpp_config_smoke.py
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from nscfg import decode_file
from taff import TaffArchive, xor_decode
from cpp_smoke_files import preserve_game_settings

# C++ 빌드와 보존된 원본 자산의 공통 루트.
ROOT = Path(__file__).resolve().parent.parent
# 검사 산출물을 두는 Git 제외 폴더.
OUTPUT = ROOT / 'extracted/cpp-config-smoke'
# 시작 순서가 읽는 설정 파일(원본 FUN_00441790이 FUN_00440e00에 넘기는 순서).
STARTUP_FILES = ['user.cfg', 'options.cfg', 'dev.cfg', 'guild.cfg', 'setup.cfg']
# 판본별 (폴더, 데이터 폴더 이름, 아카이브 이름, 부 버전, 명령줄 판본 인자, 확인할 미션 이름).
EDITIONS = [
    ('originals', 'd', 'netstorm.tarc', 78, [], ['TEST01', 'thewarbegins']),
    ('originalCD', 'D', 'NETSTORM.TARC', 72, ['--cd'], ['thewarbegins']),
]


def run(executable, *arguments, check=True):
    """새 C++ 검사 실행 파일만 호출한다. IDENTITY·USER는 지워 사용자별 설정 파일을 읽지 않게 한다."""
    environment = {key: value for key, value in os.environ.items() if key.upper() not in ('IDENTITY', 'USER')}
    result = subprocess.run([str(executable), *map(str, arguments)], capture_output=True, timeout=60, env=environment)
    if check and result.returncode:
        raise RuntimeError(result.stderr.decode('utf-8', errors='replace'))
    return result


def read_game_file(root, data_dir, archive, name):
    """원본 VFS 순서(디스크 → 아카이브)로 설정 파일을 읽는다. 없으면 None."""
    loose = root / data_dir / name
    if loose.exists():
        return loose.read_bytes()
    # 아카이브 이름은 `\d\이름` 꼴이며 대소문자를 구분하지 않는다.
    for entry in archive.entries:
        if entry.name.lower() == '\\d\\' + name.lower():
            return archive.read(entry)
    return None


def expected_buffer(root, data_dir, archive, minor, install_dir):
    """원본 시작 순서의 설정 버퍼를 원본 파일에서 직접 구성한다(C++ 코드와 독립)."""
    parts = ['[ARGS]', 'MAJOR_VERSION=10', f'MINOR_VERSION={minor}', f'version="v10.{minor}"']
    # 파일마다 섹션 줄을 넣고, 있으면 복호화한 내용을 서명째로 붙인다.
    for name in STARTUP_FILES:
        parts.append(f'[{name}]')
        raw = read_game_file(root, data_dir, archive, name)
        if raw is None:
            continue
        plain = xor_decode(raw) if raw[:1] == b'\0' and raw[2:3] == b'\0' else raw
        parts.append(plain.split(b'\0')[0].decode('cp1252').rstrip('\r\n'))
    text = '\r\n'.join(parts) + '\r\n'
    # 시작 순서가 InstallDir·CDDir을 쓸 때 버퍼 전체에서 같은 키의 줄을 지운다(줄 뒤의 빈 줄·들여쓰기 포함).
    # 서명 `mQdsT`가 붙은 options.cfg 첫 줄은 키로 인식되지 않아 남는다.
    text = re.sub(r'(?im)^([ \t]*)(?:InstallDir|CDDir)[ \t]*=[^\n]*\n[\r\n \t]*', r'\1', text)
    text += '\r\n'.join(['[END]', f'InstallDir = "{install_dir}"', f'CDDir = "{install_dir}"']) + '\r\n'
    return text.encode('utf-8')


def check_restore_on_failure():
    """임시 게임 폴더에서 검사 실패 때 설정 두 파일의 바이트·부재 상태가 복구되는지 확인한다."""
    with tempfile.TemporaryDirectory(dir=OUTPUT) as temporary:
        root = Path(temporary)
        (root / 'd').mkdir()
        paths = [root / 'd/options.cfg', root / 'fullscreenStateFile.dat']
        # 기존 파일이 있는 경우와 처음부터 없는 경우를 모두 실패 경로로 검사한다.
        for existed in (True, False):
            # 두 허용 파일의 초기 존재 상태를 맞춘다.
            for path in paths:
                if existed:
                    path.write_bytes(b'original bytes')
                elif path.exists():
                    path.unlink()
            try:
                with preserve_game_settings(root, 'd'):
                    # 파일을 변경한 직후의 예외를 재현한다.
                    for path in paths:
                        path.write_bytes(b'changed bytes')
                    raise RuntimeError('검사 실패를 의도적으로 재현')
            except RuntimeError:
                pass
            # 실패 전 바이트 또는 파일 부재가 두 경로 모두 복구돼야 한다.
            for path in paths:
                if path.exists() != existed or (existed and path.read_bytes() != b'original bytes'):
                    raise AssertionError('검사 실패 뒤 사용자 설정 복구 누락')


def main():
    """두 판본의 설정 버퍼·경로 지정값·저장 왕복을 검사하고 보고서를 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    check_restore_on_failure()
    report = {}
    # 판본마다 같은 검사를 한다.
    for edition, data_dir, archive_name, minor, flag, missions in EDITIONS:
        root = ROOT / edition
        watched = [root / archive_name] + sorted((root / data_dir).glob('*.cfg'))
        before = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in watched}
        archive = TaffArchive(str(root / archive_name))
        # 1) 설정 버퍼 전체.
        actual = run(args.exe, '--config-dump', root, *flag).stdout
        expected = expected_buffer(root, data_dir, archive, minor, str(root))
        if actual != expected:
            (OUTPUT / f'{edition}-actual.txt').write_bytes(actual)
            (OUTPUT / f'{edition}-expected.txt').write_bytes(expected)
            raise AssertionError(f'설정 버퍼 불일치: {edition} (extracted/cpp-config-smoke 에 두 버퍼를 남김)')
        # 2) 경로 지정값과 그 경로의 파일.
        language = run(args.exe, '--config-get', root, 'currentLanguage', *flag).stdout.decode('utf-8')
        palette = run(args.exe, '--config-get', root, 'battlePal', *flag).stdout.decode('utf-8')
        specs = {}
        # 미션마다 스크립트와 요새 파일의 경로를 계산해 읽어 본다.
        for mission in missions:
            # 스크립트와 요새 두 경로를 본다.
            for key, suffix in (('missionSpec', f'.{language}'), ('fortSpec', '.fort')):
                path = run(args.exe, '--config-spec', root, key, mission, *flag).stdout.decode('utf-8')
                if path != f'\\D\\{mission}{suffix}':
                    raise AssertionError(f'경로 지정값 불일치: {edition} {key} {mission}: {path}')
                if not run(args.exe, '--read-game', root, path).stdout:
                    raise AssertionError(f'경로의 파일이 비어 있음: {edition} {path}')
                specs[f'{key}({mission})'] = path
        for key, argument, wanted in (('languageSpec', language, f'\\D\\config.{language}'),
                                      ('GamePalSpec', palette, f'\\D\\{palette}.COL')):
            path = run(args.exe, '--config-spec', root, key, argument, *flag).stdout.decode('utf-8')
            if path != wanted or not run(args.exe, '--read-game', root, path).stdout:
                raise AssertionError(f'경로 지정값 불일치: {edition} {key}: {path}')
            specs[f'{key}({argument})'] = path
        # 3) 값 쓰기 뒤 저장: 기존 Python 도구로 복호화해 확인한다.
        saved = OUTPUT / f'{edition}-options.cfg'
        run(args.exe, '--config-save', root, saved, 'SCREENW=800', 'startInFullScreen=0', 'cloneOnly=yes', *flag)
        lines = decode_file(str(saved)).replace('\r', '').split('\n')
        # 새 값과 InstallDir 줄이 한 번씩 있어야 한다.
        for wanted in ('SCREENW = "800"', 'startInFullScreen = "0"', 'cloneOnly = "yes"', f'InstallDir = "{root}"'):
            if lines.count(wanted) != 1:
                raise AssertionError(f'저장한 설정에 줄이 없거나 중복됨: {edition} {wanted}')
        if any(line.startswith('SCREENW') and line != 'SCREENW = "800"' for line in lines):
            raise AssertionError(f'이전 SCREENW 줄이 남음: {edition}')
        # 4) 사용자 결정에 따라 원본 options.cfg에도 저장하고, 파일 내용·존재 여부는 반드시 되돌린다.
        with preserve_game_settings(root, data_dir) as options_path:
            run(args.exe, '--config-save', root, options_path, 'cloneOnly=inside', *flag)
            if 'cloneOnly = "inside"' not in decode_file(str(options_path)):
                raise AssertionError(f'원본 options.cfg 저장 실패: {edition}')
        # 다른 이름의 원본 폴더 출력은 계속 거부한다.
        # 대소문자·상위 폴더 표기로도 보호 파일을 덮어쓸 수 없어야 한다.
        for output in (root / data_dir / 'cpp-smoke-should-not-exist.cfg', root / data_dir.upper() / 'SETUP.CFG',
                       root / data_dir / '..' / 'Netstorm.exe'):
            refused = run(args.exe, '--config-save', root, output, *flag, check=False)
            if refused.returncode == 0:
                raise AssertionError(f'원본 폴더 안에 쓰기가 거부되지 않음: {output}')
        if (root / data_dir / 'cpp-smoke-should-not-exist.cfg').exists():
            raise AssertionError('거부한 설정 파일이 생성됨')
        after = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in watched}
        if before != after:
            raise AssertionError(f'원본 파일 변경 감지: {edition}')
        report[edition] = {'buffer_bytes': len(actual), 'buffer_sha256': hashlib.sha256(actual).hexdigest(),
                           'specs': specs, 'saved_lines': len([line for line in lines if line]),
                           'options_restored': True,
                           'original_sha256': {Path(path).name: digest for path, digest in before.items()}}
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({edition: {key: value for key, value in data.items() if key != 'original_sha256'}
                      for edition, data in report.items()}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
