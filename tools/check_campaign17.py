#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 신전 소유자·캠페인 지역 연결의 기존 전체 원문과 새 실행 결과를 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 작업 저장소의 원문 기준 경로다.
HISTORY = ROOT / 'cpppj/hist/run17-base'  # 직접 영향 파일만 보존하며 이력을 재귀 복사하지 않는다.
REPORT = ROOT / 'cpppj/run17.json'  # 이번 연결의 현재 소스와 실제 검사 SHA를 고정한다.
PROTECTED = {  # 이전에 확인한 두 보호 PE의 전체 SHA다. 새 값으로 기준을 바꾸지 않는다.
    'originals/Netstorm.exe': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
BEFORE = (  # 공유 지역 필드와 빌드 등록에 직접 영향을 받는 이전 파일이다.
    '.gitattributes', 'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
    'cpppj/src/o/RawIslandRuntime.h', 'cpppj/src/o/RawIslandRuntime.cpp',
    'cpppj/src/o/RawFortFinishOwner.h', 'cpppj/src/o/RawFortFinishOwner.cpp',
    'cpppj/tests/IslandRuntimeTests.cpp', 'cpppj/tests/FortFinishOwnerTests.cpp',
    'cpppj/islandruntime.json', 'cpppj/tests/fixtures/islandruntime-x86.tsv',
    'cpppj/fortfinishowner.json', 'cpppj/tests/fixtures/fortfinishowner-x86.tsv',
    'cpppj/run16.json', 'tools/check_campaign16.py',
)


def sha(path):
    """전체 원문 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """편집 전 파일을 한 번 보존하고 이전 바이트와 기준 커밋을 기록한다."""
    assert not (HISTORY / 'capture.json').exists()
    assert all((ROOT / name).is_file() for name in BEFORE), '보존 입력 파일이 누락됐습니다'
    saved = {}  # 원래 경로별 전체 보존 SHA다.
    # 명시한 기존 파일만 복사한다. 보호 바이너리와 기존 history는 복사하지 않는다.
    for name in BEFORE:
        target = HISTORY / name  # 현재 파일의 원래 상대경로를 유지한다.
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert target.read_bytes() == (ROOT / name).read_bytes(), name
        else:
            target.write_bytes((ROOT / name).read_bytes())
        saved[name] = sha(target)
    (HISTORY / 'capture.json').write_text(json.dumps(dict(baseline='cfa283a', host='HJOW-Athlon', files=saved),
                                                    ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위17 편집 전 전체 파일 보존:', len(saved))


def history():
    """보존 원문과 기존 원본 관찰·기대값의 불변을 검사한다."""
    saved = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))  # 당시 전체 파일 SHA다.
    # 원본 실행 파일을 읽기만 하여 보호 기준과 대조한다.
    for name, expected in PROTECTED.items():
        assert sha(ROOT / name) == expected, name
    # 보존본의 전체 바이트를 검증한다.
    for name, value in saved['files'].items():
        assert sha(HISTORY / name) == value, name
    # 기존 관찰 fixture와 보고서는 현재도 덮지 않는다.
    for name in BEFORE:
        if name.endswith('.json') or '/fixtures/' in name or name == 'tools/check_campaign16.py':
            assert sha(ROOT / name) == saved['files'][name], name
    return saved


def log(path):
    """PowerShell 출력의 UTF-16/UTF-8 인코딩을 구별해 읽는다."""
    data = path.read_bytes()  # 실제 실행 로그 원문이다.
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def finalize():
    """현재 연결 소스·실제 빌드/검사·변경 전 전체 원문을 한 보고서에 기록한다."""
    saved = history()  # 보존된 직접 영향 원문이다.
    names = set(BEFORE) | {  # 새 구현과 실행 자료의 전체 SHA 목록이다.
        'tools/check_campaign17.py', 'cpppj/hist/run17-base/capture.json',
        'cpppj/src/o/RawVortexOwner.h', 'cpppj/src/o/RawVortexOwner.cpp',
        'cpppj/src/client/RawCampaignOwnership.h', 'cpppj/src/client/RawCampaignOwnership.cpp',
        'cpppj/tests/VortexOwnerTests.cpp', 'tools/decomp_vortex_owner.py',
        'tools/ghidra/vortex-owner-functions.json', 'cpppj/vortex-owner.json',
        'cpppj/tests/fixtures/vortex-owner-x86.tsv', 'extracted/run17-build.log', 'extracted/run17-all.log',
        'cpppj/build/bin/Release/NetstormCpp.exe', 'cpppj/build/bin/Release/netstorm_tests.exe',
        'extracted/run17-vortex.log', 'extracted/run17-islands.log', 'extracted/run17-finish.log',
        'cpppj/hist/run17-fail/cpppj/src/client/RawCampaignOwnership.h',
        'cpppj/hist/run17-fail/cpppj/src/client/RawCampaignOwnership.cpp',
        'cpppj/hist/run17-fail/cpppj/tests/VortexOwnerTests.cpp',
        'cpppj/hist/run17-fail/cpppj/tests/IslandRuntimeTests.cpp',
        'cpppj/hist/run17-fail/extracted/run17-build.log',
        'cpppj/hist/run17-link/cpppj/tests/VortexOwnerTests.cpp',
        'cpppj/hist/run17-link/extracted/run17-build.log',
    }
    whole = log(ROOT / 'extracted/run17-all.log')  # 전체 실행기 출력의 실제 검사 수다.
    summary = re.findall(r'(\d+) test\(s\), (\d+) failed check\(s\)', whole)
    assert len(summary) == 1 and summary[0][1] == '0'
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', log(ROOT / 'extracted/run17-build.log'), re.I)
    focused = ''.join(log(ROOT / f'extracted/run17-{name}.log') for name in ('vortex', 'islands', 'finish'))  # 실제 집중 실행 출력이다.
    assert len(re.findall(r'\[ OK \]', focused)) == 15 and '[FAIL]' not in focused
    delta = {}  # 변경 전/후 전체 원문을 대조할 정확 줄 차이다.
    # 보존한 원문 전체와 현재 파일 전체를 비교한다. 동일 파일도 해시로 남긴다.
    for name in BEFORE:
        old = (HISTORY / name).read_text(encoding='utf-8')  # 이전 코드/문서의 전체 텍스트다.
        new = (ROOT / name).read_text(encoding='utf-8')  # 현재 코드/문서의 전체 텍스트다.
        delta[name] = dict(before_sha256=saved['files'][name], after_sha256=sha(ROOT / name),
                           patch=''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), fromfile='before', tofile='after')))
    data = dict(schema=1, host='HJOW-Athlon', date='2026-10-15', unit='20261015-02', all_tests=int(summary[0][0]),
                new_cpp_tests=8, focused_tests=15, new_native_inputs=1728, prior_native_recount=0,
                failed_checks=0, campaign_completed=False, changes=delta, protected_pe_sha256=PROTECTED,
                files={name: sha(ROOT / name) for name in sorted(names)})
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위17 실제 빌드/검사와 전체 SHA 마감:', data['all_tests'])


def verify():
    """고정한 모든 현재 파일의 바이트를 읽기 전용으로 검증한다."""
    history()
    data = json.loads(REPORT.read_text(encoding='utf-8'))  # 이번 단위의 최종 원문 기록이다.
    # 기록한 소스·원본 관찰·로그·빌드 전체 SHA를 확인한다.
    for name, expected in data['files'].items():
        assert sha(ROOT / name) == expected, name
    print('단위17 최종 전체 SHA 감사 통과')


# 요청한 모드에서만 보존·마감·읽기 검사를 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 작업 단위의 실행 모드다.
    parser.add_argument('--capture', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()  # 호출자가 지정한 보존/감사 방식이다.
    capture() if args.capture else verify() if args.verify else finalize()
