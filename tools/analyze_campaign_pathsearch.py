#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Path 검색·Init·같은 공간 연결의 실제 검증 자료와 한계를 보존한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 작업 단위의 파일은 앞선 Fort/Gun/ProcessForm 근거와 구별해 보존한다.
ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / 'cpppj/campaign-pathsearch.json'
BUILD = 'extracted/campaign-pathsearch-build-final.log'
CTEST = 'extracted/campaign-pathsearch-ctest-final.log'
TESTS = 'extracted/campaign-pathsearch-tests-final.log'
# 소스 바이트는 현재 구현의 근거이며 과거 주석 보완 전 SHA를 대체하지 않는다.
SOURCES = [
    'cpppj/src/o/RawPathSearch.h', 'cpppj/src/o/RawPathSearch.cpp',
    'cpppj/src/o/RawPathProcess.h', 'cpppj/src/o/RawPathProcess.cpp',
    'cpppj/tests/RawPathSearchTests.cpp', 'cpppj/tests/RawPathRuntimeTests.cpp',
    'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
    'tools/decomp_pathinit_oracle.py', 'tools/decomp_pathsearch_oracle.py',
    'tools/analyze_campaign_pathsearch.py',
]
# 반복 실행은 새 관찰로 더하지 않는다. 기존 Path 시간 360개는 회귀 근거로만 기록한다.
UNITS = {'pathsearch': ('cases', 644), 'pathinit': ('total', 64)}


def sha(name):
    """파일의 실제 바이트를 정규화 없이 해시한다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def read(name):
    """소스와 JSON은 엄격한 UTF-8, 네이티브 빌드 로그는 원래 인코딩을 보존해 읽는다."""
    return (ROOT / name).read_text(encoding='utf-8', errors='replace' if name.endswith('.log') else 'strict')


def main():
    """성공 로그가 있는 현재 단위만 기록하고 저장 근거의 SHA를 다시 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(read(REPORT.relative_to(ROOT).as_posix()))
        # 기록한 실행 로그·구현·원본·입력이 모두 같은 바이트인지 확인한다.
        for name, digest in report['sha256'].items():
            assert sha(name) == digest, name
        assert report['new_native_cases'] == 708 and report['tests']['failed_checks'] == 0
        assert not report['campaign_completed'] and not report['directdraw_fullscreen_changed']
        print(json.dumps({'verified': True, 'tests': report['tests'], 'new_native_cases': 708}, ensure_ascii=False))
        return
    assert 'netstorm_tests.vcxproj ->' in read(BUILD)
    assert not re.search(r'\b(?:error|warning)\s+[A-Z]+\d+:', read(BUILD))
    assert '100% tests passed' in read(CTEST)
    result = re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)', read(TESTS))
    assert result and int(result[1]) == 933 and int(result[2]) == 0
    focused = 'extracted/campaign-pathsearch-focused-rawPathSearch-final.log'
    assert '5 test(s), 0 failed check(s)' in read(focused)
    files = SOURCES + [BUILD, CTEST, TESTS, focused, 'originals/Netstorm.exe', 'originalCD/NETSTORM.EXE']
    report = dict(host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', date='2026-10-13',
                  tests=dict(cases=int(result[1]), failed_checks=int(result[2])), native_units={},
                  new_native_cases=708, reused_pathruntime_cases=360, campaign_completed=False,
                  directdraw_fullscreen_changed=False, gui_executions=0, sha256={}, limits=[
                      'Init의 native 부모 Pop은 명시 좌표/void 관찰 경계이며 C++ 통합 검사는 실제 Pop/Unpop을 사용한다.',
                      'BFS native의 프로파일링 시계는 명시 0 입력이며 실제 OS/게임을 실행하지 않는다.',
                      'Kernel 통합 검사는 client Form과 합성 부모의 Halt/Arrival/소리/cleanup 경계를 사용한다.',
                      '서버 Path 직렬화·실제 사제 운반/의식/도착 효과와 GameWorld 비표면 이관은 미완료다.',
                      '건설/전투/승패 및 캠페인 1-1 완주를 입증하는 자료가 아니다.'])
    # 각 독립 입력 행 수와 내부 실제 함수 근거를 확인한다.
    for unit, (key, expected) in UNITS.items():
        name = f'cpppj/{unit}.json'
        evidence = json.loads(read(name))
        assert evidence[key] == expected
        report['native_units'][unit] = expected
        files += [name, f'cpppj/tests/fixtures/{unit}-x86.tsv']
        # 하위 원본 함수·소스 SHA도 함께 보존한다.
        for name, digest in evidence['files'].items():
            assert sha(name) == digest, name
            files.append(name)
    prior = 'cpppj/pathruntime.json'
    evidence = json.loads(read(prior))
    assert evidence['cases'] == report['reused_pathruntime_cases']
    files += [prior, 'cpppj/tests/fixtures/pathruntime-x86.tsv', 'tools/decomp_pathruntime_oracle.py']
    # 기존 시간/단계 관찰은 새 합계에 더하지 않고 현재 회귀의 바이트 근거만 보존한다.
    for name, digest in evidence['files'].items():
        assert sha(name) == digest, name
        files.append(name)
    report['sha256'] = {name: sha(name) for name in dict.fromkeys(files)}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'tests': report['tests'], 'new_native_cases': 708, 'campaign_completed': False}, ensure_ascii=False))


# 명시적인 실행에서만 성공 기록 또는 독립 SHA 감사를 수행한다.
if __name__ == '__main__':
    main()
