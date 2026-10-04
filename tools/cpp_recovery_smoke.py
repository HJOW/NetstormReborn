#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 C++ 콘솔 모듈을 실제 두 판본 자산과 바이트 단위로 비교한다. 원본 게임은 실행하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import struct
import tempfile
from taff import TaffArchive

# C++ 빌드와 보존된 원본 자산의 공통 루트.
ROOT = Path(__file__).resolve().parent.parent


def run(executable, *arguments):
    """원본 실행 파일 대신 새 C++ 데이터 검사 실행 파일만 호출한다."""
    result = subprocess.run([str(executable), *map(str, arguments)], capture_output=True, timeout=15)
    if result.returncode:
        raise RuntimeError(result.stderr.decode('utf-8', errors='replace'))
    return result.stdout


def parse_xlat(data):
    """기존 문서의 줄 상태 규칙으로 독립적으로 번역표를 읽어 실제 자산의 기대값을 만든다."""
    text = data.decode('cp1252').replace('\r', '')
    table = {}
    original, translated, state = [], [], 0
    # C++ 포인터 범위 코드와 독립적인 줄 목록 방식으로 읽는다.
    for line in text.split('\n'):
        if state == 0 and line.startswith('*'):
            original, translated, state = [], [], 1
        elif state == 1 and line.startswith('-'):
            state = 2
        elif state == 2 and line.startswith('='):
            table['\n'.join(original)] = '\n'.join(translated)
            state = 0
        elif state == 1:
            original.append(line)
        elif state == 2:
            translated.append(line)
    return table


def main():
    """양쪽 아카이브 전체와 번역 키·디스크 우선 읽기·원본 파일 불변을 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    report = {}
    # 두 판본 파일은 읽기만 한다. 코드 구현의 기대값은 기존 Python 추출기에서 얻는다.
    for edition, archive_name in [('originals', 'netstorm.tarc'), ('originalCD', 'NETSTORM.TARC')]:
        path = ROOT / edition / archive_name
        before = hashlib.sha256(path.read_bytes()).hexdigest()
        archive = TaffArchive(str(path))
        payload = run(args.exe, '--dump-archive', path)
        count, = struct.unpack_from('<I', payload)
        if count != len(archive.entries):
            raise AssertionError('아카이브 엔트리 개수 불일치')
        pos = 4
        # 모든 엔트리를 복호화한 뒤 C++ 출력과 바이트 단위로 비교한다.
        for entry in archive.entries:
            name_size, = struct.unpack_from('<I', payload, pos)
            pos += 4
            name = payload[pos:pos+name_size].decode('ascii')
            pos += name_size
            size, = struct.unpack_from('<I', payload, pos)
            pos += 4
            actual = payload[pos:pos+size]
            pos += size
            if name != entry.name:
                raise AssertionError('아카이브 이름 인덱스 순서 불일치')
            if actual != archive.read(entry):
                raise AssertionError(f'아카이브 바이트 불일치: {edition}/{entry.name}')
        if pos != len(payload):
            raise AssertionError('아카이브 출력 끝의 잔여 데이터')
        german = next(entry for entry in archive.entries if entry.name.lower() == '\\d\\xlat.german')
        table = parse_xlat(archive.read(german))
        inspected = run(args.exe, '--inspect-data', ROOT / edition).decode('utf-8')
        if f'German translation keys: {len(table)}' not in inspected:
            raise AssertionError('번역 키 수 불일치')
        translated_count = 0
        # 쉘 argv 인코딩에 독립적인 ASCII 원문과 짧은 번역만 고른다.
        for original, translated in table.items():
            if not original.isascii() or '\n' in original or len(original) > 64:
                continue
            actual = run(args.exe, '--translate-game', ROOT / edition, 3, original).decode('utf-8')
            if actual != translated:
                raise AssertionError(f'번역 불일치: {edition}/{original}')
            translated_count += 1
            if translated_count == 20:
                break
        if before != hashlib.sha256(path.read_bytes()).hexdigest():
            raise AssertionError('원본 아카이브 변경 감지')
        report[edition] = {'archive_entries': len(archive.entries), 'translations_checked': translated_count,
                           'german_keys': len(table), 'archive_sha256': before}
    # 원본에 덮어쓰지 않고 Git 제외 임시 경로에서 디스크 우선 조회를 검증한다.
    temporary_root = ROOT / 'extracted/cpp-recovery-smoke'
    temporary_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=temporary_root) as directory:
        base = Path(directory).resolve()
        if not base.is_relative_to(temporary_root.resolve()):
            raise AssertionError('임시 폴더가 지정한 작업 경로를 벗어남')
        (base / 'netstorm.tarc').write_bytes((ROOT / 'originals/netstorm.tarc').read_bytes())
        archive_value = run(args.exe, '--read-game', base, 'd/altar.type')
        (base / 'd').mkdir()
        (base / 'd/altar.type').write_bytes(b'loose-file-priority\r\n')
        if not archive_value or run(args.exe, '--read-game', base, 'd/altar.type') != b'loose-file-priority\r\n':
            raise AssertionError('디스크 우선 조회 불일치')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
