#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소스·문서·분석 이력의 경로 길이를 검사하여 Windows 길이 문제의 재발을 막는다."""
import argparse
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 검사하는 프로젝트 루트이며 보호 원본은 대상에서 제외한다.
MAP = ROOT / 'docs/path-map.json'  # 기존 이름 정리 때 남긴 파일별 대응표다.
RELATIVE_LIMIT = 160  # 체크아웃 루트에 최소40자 여유를 남기는 저장소 상대경로 상한이다.
ABSOLUTE_LIMIT = 200  # 실제 현재 Windows 경로에 적용하는 여유 있는 상한이다.
NEW_NAME_LIMIT = 40  # 새 소스·도구·문서·보고서의 확장자를 포함한 파일명 상한이다.
DIRECTORY_LIMIT = 24  # 새 경로 구성요소의 최대 길이이며 반복 이력 복사를 허용하지 않는다.


def units(value):
    """Windows 경로의 UTF-16 코드 단위 수를 세어 이모지도 정확히 검사한다."""
    return len(str(value).encode('utf-16-le')) // 2


def inspect(candidates=None):
    """실제 파일과 선택 신규경로를 검사하고 길이·중첩 이력 위반을 반환한다."""
    saved = json.loads(MAP.read_text(encoding='utf-8')) if MAP.is_file() else {'files': {}}
    legacy = {entry['target'] for entry in saved['files'].values()}  # 기존 정리된 파일만48/64자 예외를 유지한다.
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'], cwd=ROOT).decode('utf-8').split('\0')
    paths = {name for name in names if name and (ROOT / name).is_file()}
    # 이력 내부의 ignored 바이트도 조사하되 원본게임·빌드·녹화파일은 건드리지 않는다.
    if (ROOT / 'cpppj/hist').exists():
        paths.update(path.relative_to(ROOT).as_posix() for path in (ROOT / 'cpppj/hist').rglob('*') if path.is_file())
    paths.update(candidates or ())
    errors, maximum, checked = [], 0, 0
    # 신규 파일은 기존 대응표의 예외를 적용하지 않고40자 기준을 사용한다.
    for name in sorted(paths):
        path = Path(name)
        if path.parts[0].startswith('original') or path.parts[0] in {'playingVideos', 'fonts', '.git'}:
            continue
        if any(part in {'build', 'bin', 'obj', '__pycache__', 'node_modules', '.vs'} for part in path.parts) and 'hist' not in path.parts:
            continue
        checked += 1
        absolute = units(ROOT / path)  # Windows의 UTF-16 문자수 기준이다.
        maximum = max(maximum, absolute)
        if units(path.as_posix()) > RELATIVE_LIMIT or (os.name == 'nt' and absolute > ABSOLUTE_LIMIT):
            errors.append((name, f'전체경로 {absolute}자/상대경로 {len(path.as_posix())}자'))
        existing = name in legacy and name not in (candidates or ())  # 명시 신규 후보에는 과거 대응표의 예외를 적용하지 않는다.
        name_limit = (48 if path.suffix in {'.py', '.cpp', '.h', '.json', '.md'} else 64) if existing else NEW_NAME_LIMIT
        if units(path.name) > name_limit:
            errors.append((name, f'파일명 {units(path.name)}자/상한 {name_limit}자'))
        if not existing and any(units(part) > DIRECTORY_LIMIT for part in path.parts[:-1]):
            errors.append((name, '신규디렉토리 이름24자 초과'))
        if path.parts.count('hist') > 1:
            # 과거 중첩 복사는 대응표로만 허용하고 같은 구조의 신규 생성은 거부한다.
            if not existing:
                errors.append((name, '분석이력의 재귀 복사'))
    return checked, maximum, errors


# 기본은 실제 파일 검사이며 --candidate는 파일 생성 전에 예상 경로를 함께 확인한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', action='append', default=[])
    arguments = parser.parse_args()
    count, maximum, problems = inspect(arguments.candidate)
    # 각 위반의 실제 경로와 기준을 출력해 생성 전에 이름을 교정할 수 있게 한다.
    for name, message in problems:
        print(message, name)
    print(f'경로 {count}개, 최장 {maximum}자, 위반 {len(problems)}개')
    raise SystemExit(1 if problems else 0)
