#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""동일 캠페인 월드의 사제 이동 연결 원문과 검사 결과를 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 전체 바이트 기준이다.
HISTORY = ROOT / 'cpppj/hist/run19-base'  # 직접 편집하는 기존 파일만 보존한다.
BEFORE = ('.gitattributes', 'LEFT_JOBS.md', 'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
          'cpppj/src/client/RawPriestWorldBindings.h', 'cpppj/src/client/RawPriestWorldBindings.cpp',
          'cpppj/tests/CampaignWorldRuntimeTests.cpp')  # 변경 전 원문 목록이다.
REPORT = ROOT / 'cpppj/run19.json'  # 이번 실제 실행과 현재 전체 SHA 보고서다.
NEW_SOURCES = ('cpppj/src/client/RawCampaignMotionBindings.cpp', 'cpppj/src/client/RawCampaignMotionBindings.h',
               'cpppj/src/o/RawSquidLevel.cpp', 'cpppj/src/o/RawSquidLevel.h',
               'cpppj/tests/CampaignInstallSupport.h', 'cpppj/tests/CampaignMotionTests.cpp')  # 전체 현재 원문을 보존할 신규 C++이다.
PROTECTED = {  # 이전에 확인한 보호 PE의 불변 기준이다.
    'originals/Netstorm.exe': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}


def sha(path):
    """전체 파일 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """첫 편집 전에 직접 영향 원문을 한 번 저장한다."""
    assert not (HISTORY / 'capture.json').exists()
    files = {}  # 원래 경로별 보존 SHA다.
    # 지정한 이전 파일만 보존하며 기존 hist를 재귀 복사하지 않는다.
    for name in BEFORE:
        target = HISTORY / name  # 원래 상대 경로의 원문 보존본이다.
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / name).read_bytes())
        files[name] = sha(target)
    (HISTORY / 'capture.json').write_text(json.dumps(dict(baseline='2ed0b3e', host='HJOW-Athlon', files=files),
                                                    ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위19 편집 전 원문 보존:', len(files))


def history():
    """추가 영향 파일까지 포함한 보존 원문과 보호 PE를 검사한다."""
    data = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))  # 이번15개 이전 원문이다.
    # 보존 원문이 현재 편집 때문에 달라지지 않았는지 전수 대조한다.
    for name, value in data['files'].items():
        assert sha(HISTORY / name) == value, name
    # 두 보호 PE는 읽기만 하고 불변 기준을 확인한다.
    for name, value in PROTECTED.items():
        assert sha(ROOT / name) == value, name
    return data


def log(name):
    """PowerShell의 UTF-16 또는 UTF-8 로그를 원문 변경 없이 읽는다."""
    value = (ROOT / name).read_bytes()  # 실행 당시 로그 바이트다.
    return value.decode('utf-16' if value.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def finalize():
    """검사가 완료된 뒤 새 원문/바이너리/이전 근거의 SHA를 별도 고정한다."""
    saved = history()  # 실제 보존본과 보호 PE부터 확인한다.
    assert len(saved['files']) == 15
    assert '1110 test(s), 0 failed check(s)' in log('extracted/run19-all.log')
    assert '100% tests passed' in log('extracted/run19-all.log')
    assert '5 test(s), 0 failed check(s)' in log('extracted/run19-focused9.log')
    assert '4 test(s), 0 failed check(s)' in log('extracted/run19-fort.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+', log('extracted/run19-build8.log'), re.I)
    native = json.loads((ROOT / 'cpppj/squid-level.json').read_text(encoding='utf-8'))  # 새96입력만 합산한다.
    assert native['uniqueInputs'] == 96 and native['toolSha'] == sha(ROOT / 'tools/decomp_squid_level.py')
    assert native['fixtureSha'] == sha(ROOT / 'cpppj/tests/fixtures/squid-level-x86.tsv')
    # 실제 실행에 사용한 함수 export 원문도 전수 검사한다.
    for edition in native['editions'].values():
        assert edition['samples'] == 48 and edition['assertions'] == 24
        for name, value in edition['exports'].items():
            assert sha(ROOT / name) == value, name
    paths = set(saved['files']) | set(NEW_SOURCES) | {
        'tools/check_campaign19.py', 'tools/decomp_squid_level.py', 'cpppj/squid-level.json',
        'docs/exe/cpp-run19-20261015.md', 'cpppj/run18.json', 'tools/check_campaign18.py',
        'extracted/run19-build8.log', 'extracted/run19-focused9.log', 'extracted/run19-fort.log', 'extracted/run19-all.log',
        'extracted/run19-native2.log', 'extracted/run19-export.log', 'extracted/run19-export2.log', 'extracted/run19-export3.log',
        'cpppj/build/bin/Release/Netstorm.exe', 'cpppj/build/bin/Release/netstorm_tests.exe', 'cpppj/build/lib/Release/netstorm.lib',
    }  # 결과와 직접 의존 원문이다.
    # 현재 전체 C++ 소스 및 원본 입력을 복사하지 않고 SHA만 남긴다.
    for folder in ('cpppj/src', 'cpppj/tests'):
        for path in (ROOT / folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp', '.h', '.tsv') or path.name == 'CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이 단위의 명시 보존본만 읽으며 이전 hist는 재귀 복사하지 않는다.
    for folder in ('run19-base', 'run19-fail1', 'run19-fail2', 'run19-fail8', 'run19-native1'):
        for path in (ROOT / 'cpppj/hist' / folder).rglob('*'):
            if path.is_file():
                paths.add(path.relative_to(ROOT).as_posix())
    # 새 export와 새 계획만 현재 보고서에 묶는다.
    for pattern in ('extracted/squid-level*/**/*', 'tools/ghidra/squid-level*-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():
                paths.add(path.relative_to(ROOT).as_posix())
    changes = {}  # 직접 영향15개 원문의 실제 전후 차이다.
    # 전체 이전 보존본과 현재 원문을 UTF-8로 비교하며 줄바꿈도 그대로 기록한다.
    for name, value in saved['files'].items():
        before = (HISTORY / name).read_bytes().decode('utf-8')
        after = (ROOT / name).read_bytes().decode('utf-8')
        changes[name] = dict(beforeSha=value, afterSha=sha(ROOT / name), diff=''.join(difflib.unified_diff(
            before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile=name, tofile=name)))
    data = dict(unit='20261015-04', baseline='2ed0b3e', host='HJOW-Athlon', lastDecompileHost='HJOW-Athlon',
                newNativeInputs=96, oldInputsRecounted=0, newTests=5, allTests=1110, failedChecks=0,
                movementFrames=dict(warBeginsEnemy=189, masterLocal=148), native=native, protected=PROTECTED,
                changes=changes, sourceText={name: (ROOT / name).read_bytes().decode('utf-8') for name in NEW_SOURCES},
                files={name: sha(ROOT / name) for name in sorted(paths)},
                limitations=['GameWorld의 비표면 배열/관찰 기반 이동 유지', 'UI/AI/전체Kernel/전투 미실행',
                             '소리0토큰/전송은 검사 외부 경계', '실제 플레이 동일성 목표active',
                             '이전 경로/참조/과거SHA 최종 감사 별도 미완료'])  # 실제 범위만 기록한다.
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('단위19 전체 SHA 고정:', len(paths), '파일, Native96/전체1110 실패0')


def verify():
    """마감한 원문/입력/로그/바이너리를 읽기 전용으로 대조한다."""
    history()
    data = json.loads(REPORT.read_text(encoding='utf-8'))  # 이미 고정한 이번 기준이다.
    # 실제 전체 파일 바이트가 마감 기준과 일치하는지 확인한다.
    for name, value in data['files'].items():
        assert sha(ROOT / name) == value, name
    # 신규 C++의 보고서 전체 원문도 현재 바이트와 직접 대조한다.
    for name, value in data['sourceText'].items():
        assert value.encode('utf-8') == (ROOT / name).read_bytes(), name
    print('단위19 전체 원문/입력/실행 SHA와 보호 원본/보존본 불변 확인')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 요청한 보존만 실행한다.
    parser.add_argument('action', choices=('capture', 'finalize', 'verify'))
    args = parser.parse_args()  # 작업 단위의 명시 실행 요청이다.
    capture() if args.action == 'capture' else finalize() if args.action == 'finalize' else verify()
