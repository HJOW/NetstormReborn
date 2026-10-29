#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단위15 준비 검사의 등록·실행과 단위14 당시 전체 자료를 분리해 보존한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from analyze_campaign_runtime6 import read_log

ROOT = Path(__file__).resolve().parents[1]  # 동일 작업 트리의 원문을 읽는 기준 경로다.
HISTORY = ROOT / 'cpppj/hist/campaign-run-092de'  # 이전 전체 파일을 재귀 복사 없이 보존한다.
PRIOR = ROOT / 'cpppj/run14.json'  # 이미 완료한 단위14 원문은 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/run15.json'  # 등록한 준비7 검사의 결과를 별도로 기록한다.


def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """등록 전 단위14 전체 근거와 준비15 전체 파일을 그대로 보존한다."""
    assert not HISTORY.exists()
    previous = json.loads(PRIOR.read_text(encoding='utf-8'))
    prepared = ROOT / 'cpppj/run15-preparation-handoff.json'
    draft = json.loads(prepared.read_text(encoding='utf-8'))
    names = set(previous['files']) | set(draft['files']) | {PRIOR.relative_to(ROOT).as_posix(), prepared.relative_to(ROOT).as_posix()}
    saved = {}
    # 기존 history 내부는 다시 복사하지 않고 고정된 전체 경로·SHA를 참조한다.
    for name in sorted(names):
        source = ROOT / name
        target = HISTORY / name
        assert not name.startswith('cpppj/hist/')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(source.read_bytes())
        saved[name] = sha(target)
        if name in previous['files']:
            assert saved[name] == previous['files'][name], name
        if name in draft['files']:
            assert saved[name] == draft['files'][name], name
    (HISTORY / 'capture.json').write_text(json.dumps(dict(baseline_commit='6d9589b', host='HJOW-Athlon', files=saved), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위14/준비15 전체 파일 보존:', len(saved))


def audit_history():
    """이전 보고서의 모든 SHA를 당시 전체 파일에서 읽어 검증한다."""
    saved = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))
    # 등록 전 원문·바이너리·로그·fixture를 각각 전체 바이트로 검증한다.
    for name, expected in saved['files'].items():
        assert sha(HISTORY / name) == expected, name
    previous = json.loads((HISTORY / PRIOR.relative_to(ROOT)).read_text(encoding='utf-8'))
    # 단위14의 보고서 본문과 고정한 전체 근거를 수정하지 않고 재현한다.
    for name, expected in previous['files'].items():
        assert sha(HISTORY / name) == expected, name
    assert PRIOR.read_bytes() == (HISTORY / PRIOR.relative_to(ROOT)).read_bytes()
    return saved


def generate():
    """등록7 검사의 실제 성공 로그와 현재 전체 소스·빌드를 고정한다."""
    audit_history()
    names = {Path(__file__).relative_to(ROOT).as_posix(), '.gitattributes'}
    prepared = json.loads((HISTORY / 'cpppj/run15-preparation-handoff.json').read_text(encoding='utf-8'))
    names.update(prepared['files'])
    # 실제 등록·실행한 자료와 이전 전체 보존 manifest를 함께 기록한다.
    names.update({'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
                  'cpppj/build/bin/Release/netstorm_tests.exe', 'cpppj/build/bin/Release/Netstorm.exe',
                  'extracted/run15-build-final.log', 'extracted/run15-new-final.log',
                  'extracted/run15-all-final.log', (HISTORY / 'capture.json').relative_to(ROOT).as_posix(),
                  'tools/analyze_campaign_runtime15_corrections.py', 'cpppj/run15-corrections.json',
                  'extracted/run15-new-first-tests.log', 'extracted/run15-all-first-tests.log',
                  'extracted/run15-build-first.log'})
    focused = read_log(ROOT / 'extracted/run15-new-final.log')
    whole = read_log(ROOT / 'extracted/run15-all-final.log')
    build = read_log(ROOT / 'extracted/run15-build-final.log')
    assert len(re.findall(r'\[ OK \]', focused)) == 7
    assert len(re.findall(r'\[ OK \]', whole)) == 1088
    assert 'FAIL' not in focused and 'FAIL' not in whole
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', build, re.I)
    data = dict(schema=1, host='HJOW-Athlon', last_decompile_host='HJOW-Athlon', last_decompile_date='2026-10-14',
                date='2026-10-14', unit='20261014-07', new_cpp_tests=7, all_tests=1088,
                previously_prepared_native_inputs=1214, normal_unique=1086, assertion_unique=104, unsupported_lower_requests=24,
                new_native_inputs=0, build_warnings=0, build_errors=0, game_runs=0,
                full_campaign_install_completed=False, campaign_completed=False,
                files={name: sha(ROOT / name) for name in sorted(names)},
                scope='준비된1214입력의 C++ 소비를 처음 등록한다. 기존 원본 입력을 새 관찰로 합산하지 않는다. 실제172개 설치·GameWorld 연결·플레이 완주·화면/음향 동일성은 후속이다.')
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위15 새7검사/전체1088·기존준비1214입력 실행 근거 생성')


def verify():
    """이전 전체 자료와 최종 기록의 현재 바이트를 읽기 전용으로 감사한다."""
    audit_history()
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    # 최종 빌드와 검사 로그를 포함해 모든 고정한 파일을 감사한다.
    for name, expected in data['files'].items():
        assert sha(ROOT / name) == expected, name
    print('단위15 최종 전체 SHA/이전 전체 근거 감사 통과')


# 명시 CLI에서만 등록 전 보존·최종 근거 생성·읽기 감사를 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', action='store_true')
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    capture() if arguments.capture else verify() if arguments.verify else generate()
