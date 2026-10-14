#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""과거 Git 객체를 읽는 분석 도구에 경로 이관 전 이름과 보존된 이관 뷰를 제공한다."""
import hashlib
import json
import re
import runpy
import sys
import inspect
import subprocess as native
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 분석 도구와 이관 대응표가 있는 저장소 루트다.
MAP = ROOT / 'docs/path-map.json'  # 원래 Git blob의 SHA와 현재 보존 파일의 대응을 읽는다.
NATIVE_RUN = native.run  # 호환 래퍼를 설치한 뒤에도 일반 프로세스 실행은 이 원래 함수를 사용한다.
NATIVE_CHECK_OUTPUT = native.check_output  # Git 이외 출력 조회의 원래 호출 규약을 유지한다.


@lru_cache(maxsize=1)
def mapping():
    """한 분석 프로세스 안에서는 이관 대응표를 한 번만 읽는다."""
    return json.loads(MAP.read_text(encoding='utf-8')) if MAP.exists() else {'files': {}, 'aliases': {}}


def renamed(value):
    """이관 전 경로 문자열을 같은 전역 이름 규칙으로 현재 경로로 바꾼다."""
    data = mapping()
    aliases = data['aliases']
    if not aliases:
        return value
    pattern = re.compile('|'.join(re.escape(name) for name in sorted(aliases, key=len, reverse=True)))
    value = re.sub(r'recovery-([A-Za-z0-9_{}.*-]+)-evidence\.json', r'\1.json', value)
    return pattern.sub(lambda match: aliases[match.group()], value)


def old_spec(spec):
    """현재 파일명을 과거 커밋에 저장된 실제 파일명으로 역조회한다."""
    revision, name = spec.split(':', 1)
    # 같은 현재 이름을 가리키는 최초 경로만 Git 조회의 후보로 사용한다.
    for previous, entry in mapping()['files'].items():
        if entry['target'] == name:
            return revision + ':' + previous
    return spec


def migrated_blob(body, spec):
    """원래 Git SHA와 일치하는 보존 파일로 경로 이관 후의 당시 뷰를 읽는다."""
    if isinstance(body, str):
        return migrated_blob(body.encode('utf-8'), spec).decode('utf-8')
    name = renamed(spec.split(':', 1)[1])
    digest = hashlib.sha256(body).hexdigest()
    matches = []
    frame = inspect.currentframe()
    # 실제 감사 호출자의 HISTORY/git 경로가 있으면 같은 원래 blob인지 확인한 뒤 우선한다.
    while frame is not None:
        history = frame.f_globals.get('HISTORY')
        if isinstance(history, Path) and history.is_relative_to(ROOT / 'cpppj/hist'):
            candidate = (history / 'git' / name).relative_to(ROOT).as_posix()
            entry = next((item for item in mapping()['files'].values() if item['target'] == candidate), None)
            if entry and digest in {entry['before'], entry['current_before'], *entry.get('eol_hashes', ())}:
                return (ROOT / candidate).read_bytes()
        frame = frame.f_back
    # 현재 의미가 바뀐 파일 대신 같은 원래 blob을 가진 당시 Git/작업 보존본을 우선한다.
    for previous, entry in mapping()['files'].items():
        target = entry['target']
        if digest not in {entry['before'], entry['current_before'], *entry.get('eol_hashes', ())}:
            continue
        if not (target == name or target.endswith('/' + name) or Path(target).name == Path(name).name):
            continue
        score = (0 if '/git/' in target else 1 if 'hist' in Path(target).parts else 2, len(target))
        matches.append((score, target))
    # 원문으로 식별한 보존본만 반환하고 임의로 다른 의미의 최신 소스를 선택하지 않는다.
    for _, target in sorted(matches):
        path = ROOT / target
        if path.is_file():
            return path.read_bytes()
    return body


def run(args, *positional, **kwargs):
    """일반 subprocess.run은 유지하고 읽기 전용 Git show/ls-tree에만 이관을 적용한다."""
    if not isinstance(args, (list, tuple)) or len(args) < 3 or args[0] != 'git' or args[1] not in {'show', 'ls-tree'}:
        return NATIVE_RUN(args, *positional, **kwargs)
    checking = kwargs.pop('check', False)
    effective = args
    if args[1] == 'show' and ':' in str(args[2]):
        spec = old_spec(args[2])
        if spec != args[2]:
            probe = NATIVE_RUN(['git', 'cat-file', '-e', spec], cwd=kwargs.get('cwd', ROOT), stdout=native.DEVNULL, stderr=native.DEVNULL)
            if probe.returncode == 0:
                effective = [*args[:2], spec, *args[3:]]
    result = NATIVE_RUN(effective, *positional, check=False, **kwargs)
    if args[1] == 'show' and ':' in str(args[2]):
        spec = effective[2]
        if result.returncode and spec != args[2]:
            result = NATIVE_RUN([*args[:2], spec, *args[3:]], *positional, check=False, **kwargs)
        if result.returncode == 0 and result.stdout is not None:
            result.stdout = migrated_blob(result.stdout, spec)
    elif args[1] == 'ls-tree' and result.returncode == 0 and result.stdout is not None:
        # ls-tree 목록의 파일명도 이후 ROOT 조회와 같은 현재 이름을 사용한다.
        result.stdout = renamed(result.stdout) if isinstance(result.stdout, str) else renamed(result.stdout.decode('utf-8')).encode('utf-8')
    if checking and result.returncode:
        raise native.CalledProcessError(result.returncode, args, output=result.stdout, stderr=result.stderr)
    return result


def check_output(args, *positional, **kwargs):
    """Git 출력의 호환 뷰를 check_output 호출 규약으로 반환한다."""
    if not isinstance(args, (list, tuple)) or len(args) < 3 or args[0] != 'git' or args[1] not in {'show', 'ls-tree'}:
        return NATIVE_CHECK_OUTPUT(args, *positional, **kwargs)
    return run(args, *positional, stdout=native.PIPE, check=True, **kwargs).stdout


def __getattr__(name):
    """PIPE·Popen·예외 등 나머지 subprocess API는 표준 라이브러리에 그대로 위임한다."""
    return getattr(native, name)


def main():
    """현재 감사 도구를 고정된 원문 그대로 실행하되 Git 조회에 이관 호환 뷰를 설치한다."""
    if len(sys.argv) < 2:
        raise SystemExit('사용법: python tools/path_compat.py tools/감사도구.py --verify')
    path = (ROOT / sys.argv[1]).resolve()
    assert path.is_relative_to(ROOT / 'tools') and path.is_file()
    native.run = run
    native.check_output = check_output
    sys.argv = [str(path), *sys.argv[2:]]
    runpy.run_path(str(path), run_name='__main__')


# 보고서가 고정한 기존 도구의 SHA를 바꾸지 않고 과거 Git 조회를 실행한다.
if __name__ == '__main__':
    main()
