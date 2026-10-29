#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인 설치 연결의 직접 영향 원문과 실제 빌드 결과를 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 저장소의 전체 바이트 기준 경로다.
HISTORY = ROOT / 'cpppj/hist/run18-base'  # 이번에 편집할 기존 파일만 보존한다.
BEFORE = ('.gitattributes', 'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'LEFT_JOBS.md')  # 직접 영향 파일이다.
PROTECTED = {  # 기존 보호 PE의 SHA이며 새 값으로 기준을 덮지 않는다.
    'originals/Netstorm.exe': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
REPORT = ROOT / 'cpppj/run18.json'  # 이 단위의 설치 검사/소스/로그/바이너리 SHA다.
NEW_SOURCES = (  # 전체 현재 원문을 보고서에도 보존하는 이번 신규 C++ 파일이다.
    'cpppj/src/client/RawCampaignWorldRuntime.cpp', 'cpppj/src/client/RawCampaignWorldRuntime.h',
    'cpppj/tests/CampaignWorldRuntimeTests.cpp',
)


def sha(path):
    """파일 전체 바이트를 SHA-256으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """편집 전 원문을 한 번 보존한다. 기존 이력 디렉터리는 복사하지 않는다."""
    assert not (HISTORY / 'capture.json').exists()
    saved = {}  # 원래 경로별 보존 원문 SHA다.
    # 직접 편집하는 네 파일의 원문만 보존한다.
    for name in BEFORE:
        target = HISTORY / name  # 원래 상대 경로를 유지하는 보존본이다.
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / name).read_bytes())
        saved[name] = sha(target)
    (HISTORY / 'capture.json').write_text(json.dumps(dict(baseline='637ad76', host='HJOW-Athlon', files=saved),
                                                    ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위18 편집 전 원문 보존:', len(saved))


def history():
    """보존본과 보호 PE를 전체 바이트로 대조한다."""
    saved = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))  # 편집 전의 불변 기준이다.
    # 과거 원문 보존본을 전체 바이트로 대조한다.
    for name, value in saved['files'].items():
        assert sha(HISTORY / name) == value, name
    # 보호 실행 파일은 읽기만 하고 기준 SHA를 유지한다.
    for name, value in PROTECTED.items():
        assert sha(ROOT / name) == value, name
    return saved


def log(name):
    """PowerShell의 UTF-16 로그와 UTF-8 출력을 원문 보존 상태로 읽는다."""
    data = (ROOT / name).read_bytes()  # 디스크 로그를 덮어쓰지 않는다.
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def finalize():
    """실제 빌드/검사 로그가 통과한 뒤 전체 현재 SHA와 신규 C++ 원문을 고정한다."""
    before = history()  # 보호 PE와 보존 원문이 바뀌지 않았는지 먼저 검사한다.
    build = log('extracted/run18-build8.log')  # 이번 최종 Release 빌드 출력이다.
    focused = log('extracted/run18-focused6.log')  # 실제 보호 미션 설치3개 결과다.
    all_tests = log('extracted/run18-all.log')  # 동일 실행 파일의 전체 CTest 결과다.
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+', build, re.I), '최종 빌드 경고/오류'
    assert '3 test(s), 0 failed check(s)' in focused
    assert '1105 test(s), 0 failed check(s)' in all_tests and '100% tests passed' in all_tests
    paths = set(BEFORE + NEW_SOURCES + ('tools/check_campaign18.py', 'docs/exe/cpp-run18-20261015.md',
                'extracted/run18-build8.log', 'extracted/run18-focused6.log', 'extracted/run18-all.log',
                'cpppj/build/bin/Release/Netstorm.exe', 'cpppj/build/bin/Release/netstorm_tests.exe',
                'cpppj/build/lib/Release/netstorm.lib'))  # 직접 결과 파일이다.
    # 빌드에 사용한 전체 현재 프로젝트 소스의 SHA만 기록한다. 디렉터리를 복사하지 않는다.
    for folder in ('cpppj/src', 'cpppj/tests'):
        for path in (ROOT / folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp', '.h') or path.name == 'CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이번 명시 보존본과 모든 실패 로그의 원문 SHA를 남긴다. 기존 hist는 재귀 보존하지 않는다.
    for folder in ('run18-base', 'run18-fail', 'run18-fail5', 'run18-run1', 'run18-run2', 'run18-run3', 'run18-run4'):
        for path in (ROOT / 'cpppj/hist' / folder).rglob('*'):
            if path.is_file():
                paths.add(path.relative_to(ROOT).as_posix())
    # 이미 존재하는 Native 입력/보고서의 현재 SHA를 고정하며 신규 입력으로 합산하지 않는다.
    for path in (ROOT / 'cpppj/tests/fixtures').glob('*'):
        if path.is_file():
            paths.add(path.relative_to(ROOT).as_posix())
    changes = {}  # 직접 편집한 기존 파일의 전후 전체 SHA와 원문 차이다.
    # 이 단위에서 직접 변경한 네 파일만 이전 전체 보존본과 비교한다.
    for name, old_sha in before['files'].items():
        old = (HISTORY / name).read_bytes().decode('utf-8')
        new = (ROOT / name).read_bytes().decode('utf-8')
        changes[name] = dict(beforeSha=old_sha, afterSha=sha(ROOT / name),
                            diff=''.join(difflib.unified_diff(old.splitlines(keepends=True), new.splitlines(keepends=True), fromfile=name, tofile=name)))
    data = dict(unit='20261015-03', baseline='637ad76', host='HJOW-Athlon', lastDecompileHost='HJOW-Athlon',
                newNativeInputs=0, newTests=3, allTests=1105, failedChecks=0,
                installed=dict(savedAssets=172, activeAssets=1875, kernel=34, contentForms=2, externalRequests=33),
                protected=PROTECTED, changes=changes,
                sourceText={name: (ROOT / name).read_bytes().decode('utf-8') for name in NEW_SOURCES},
                files={name: sha(ROOT / name) for name in sorted(paths)},
                limitations=['GUI/게임 프레임 미실행', 'UI/음향/전송/부모 통지는 명시 검사 경계',
                             '독립 Native 전체 초기메모리 대조0', '실제 플레이 동일성 목표active',
                             '과거 전체 경로/참조/SHA 감사 별도 미완료'])  # 검사의 실제 범위를 기록한다.
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위18 현재 전체 SHA 고정:', len(paths), '파일, 설치172/활성1875/전체1105 실패0')


def verify():
    """보고서의 소스/입력/로그/바이너리와 보존본을 읽기 전용으로 검사한다."""
    history()
    data = json.loads(REPORT.read_text(encoding='utf-8'))  # 이미 고정한 이 단위의 SHA다.
    # 전수 SHA를 확인하여 마감 이후 다른 소스/로그로 결과를 바꾸지 않는다.
    for name, value in data['files'].items():
        assert sha(ROOT / name) == value, name
    # 보고서에 보존한 신규 C++ 전체 원문도 현재 바이트와 대조한다.
    for name, value in data['sourceText'].items():
        assert value.encode('utf-8') == (ROOT / name).read_bytes(), name
    print('단위18 전체 소스/입력/결과 SHA와 보호 원본/보존본 감사 통과')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 보존과 읽기 검사를 명시적으로 선택한다.
    parser.add_argument('action', choices=('capture', 'finalize', 'verify'))
    args = parser.parse_args()  # 요청한 동작만 실행한다.
    capture() if args.action == 'capture' else finalize() if args.action == 'finalize' else verify()
