#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 캠페인 실행 기반 단위의 실제 로그·독립 자료·소스 SHA를 저장/감사한다. 게임은 실행하지 않는다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 완료한 작업 단위의 보고서 위치다. 캠페인 전체 완료 상태와 구별한다.
ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / 'cpppj/recovery-campaign-runtime-evidence.json'
# 검증 시 변경을 검출할 실제 구현/검사/호스트 연결 파일이다.
SOURCES = [
    'cpppj/src/client/RawButtonControls.h', 'cpppj/src/client/RawButtonControls.cpp',
    'cpppj/src/client/UberGump.h', 'cpppj/src/client/UberGump.cpp',
    'cpppj/src/client/ClientMain.h', 'cpppj/src/client/ClientMain.cpp',
    'cpppj/src/client/StoneButtonGump.cpp', 'cpppj/src/client/GumpLifetime.h', 'cpppj/src/client/GumpLifetime.cpp',
    'cpppj/src/o/Kernel.h', 'cpppj/src/o/Kernel.cpp', 'cpppj/src/o/SquidProcess.h', 'cpppj/src/o/SquidProcess.cpp',
    'cpppj/src/o/RawMissionOutcome.h', 'cpppj/src/o/RawMissionOutcome.cpp',
    'cpppj/tests/RawButtonControlsTests.cpp', 'cpppj/tests/GumpTextTests.cpp',
    'cpppj/tests/ProcessRetakeTests.cpp', 'cpppj/tests/GumpResetTests.cpp', 'cpppj/tests/MissionOutcomeTests.cpp',
    'tools/cpp_rawbuttons_smoke.py', 'tools/cpp_gumpvisual_smoke.py', 'tools/cpp_gumpbackground_smoke.py',
    'tools/analyze_campaign_runtime.py',
]


def sha(path):
    """실제 저장 바이트의 SHA-256을 계산한다. 개행/UTF-8을 다시 정규화하지 않는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def main():
    """실제 최종 성공 로그가 있을 때만 기록하며 감사 모드에서는 저장 SHA와 원본 PE를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        # 자료/소스/로그가 기록 시점과 같아야 단위 완료 근거를 재사용할 수 있다.
        for path, expected in saved['sha256'].items():
            assert sha(path) == expected, path
        assert saved['tests']['failed_checks'] == 0 and saved['campaign_completed'] is False
        print(f"실행 기반 SHA 감사 통과: {saved['tests']['cases']}개 검사, 원본/C++ {saved['native_cases']}개")
        return
    log = (ROOT / 'extracted/campaign-runtime-ctest-final.log').read_text(encoding='utf-8')
    assert '100% tests passed' in log, '전체 CTest 최종 성공이 필요함'
    match = re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)',
        (ROOT / 'cpppj/build/Testing/Temporary/LastTest.log').read_text(encoding='utf-8', errors='replace'))
    assert match and match[2] == '0', '실제 전체 C++ 결과가 필요함'
    saved = dict(host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', date='2026-10-13',
        campaign_completed=False, directdraw_fullscreen_changed=False, native_cases=1358,
        tests=dict(cases=int(match[1]), failed_checks=int(match[2])), native_units={}, gui={}, sha256={})
    files = list(SOURCES) + ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE',
        'extracted/campaign-runtime-ctest-final.log', 'extracted/campaign-runtime-build-final-pixel-expectations.log']
    # 실제 세 독립 보고서/fixture의 입력 개수를 읽고 중복 재실행/기존 시각 fixture를 새 입력에 더하지 않는다.
    for name in ('processretake', 'uireset', 'missionoutcome'):
        path = f'cpppj/recovery-{name}-evidence.json'
        data = json.loads((ROOT / path).read_text(encoding='utf-8'))
        count = sum(value['rows'] for value in data['editions'].values()) if name == 'processretake' else data['total']
        saved['native_units'][name] = count
        files.extend((path, f'cpppj/tests/fixtures/{name}-x86.tsv'))
    assert sum(saved['native_units'].values()) == saved['native_cases']
    # GUI 기대값은 기존 원본 함수/독립 자산 도구가 만들었으며 완료/원본 보존 결과를 그대로 보관한다.
    for name in ('menu', 'rawbuttons', 'gumpvisual', 'gumpbackground'):
        path = f'extracted/cpp-{name}-smoke/report.json'
        saved['gui'][name] = json.loads((ROOT / path).read_text(encoding='utf-8'))
        files.append(path)
    # 같은 자료의 개행/파일 수정도 최종 단위 재감사에서 검출한다.
    for path in files:
        saved['sha256'][path] = sha(path)
    REPORT.write_text(json.dumps(saved, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f"실행 기반 기록: {saved['tests']['cases']}개 검사, 원본/C++ {saved['native_cases']}개, 캠페인 완주 미완료")


if __name__ == '__main__':
    main()
