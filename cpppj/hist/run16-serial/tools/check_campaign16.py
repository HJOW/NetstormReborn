#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인 공유 Player·로딩 종료 연결의 변경 전 원문과 실제 빌드 근거를 보존한다."""
import argparse
import base64
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 파일을 읽는 기준 경로다.
HISTORY = ROOT / 'cpppj/hist/run16-base'  # 직접 변경하는 원문만 저장하며 기존 이력을 복사하지 않는다.
REPORT = ROOT / 'cpppj/run16.json'  # 이번 공유 연결 단위의 최종 전체 파일 해시를 기록한다.
DELTA = ROOT / 'cpppj/run16-delta.json'  # 변경 전/후 전체 소스를 정확히 재구성하는 줄 단위 차이다.
BEFORE = (  # 공유 API·등록·검사에 영향을 받는 기존 파일과 독립 원본 근거다.
    'cpppj/src/o/RawAssetPostPop.h', 'cpppj/src/o/RawAssetPostPop.cpp',
    'cpppj/src/client/RawSurfaceWorld.h', 'cpppj/src/client/RawSurfaceWorld.cpp',
    'cpppj/src/client/RawCampaignAssetBindings.h', 'cpppj/src/client/RawCampaignAssetBindings.cpp',
    'cpppj/src/client/RawCampaignPathBindings.h', 'cpppj/src/client/RawCampaignPathBindings.cpp',
    'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
    'cpppj/tests/AssetPostPopTests.cpp', 'cpppj/tests/RawSurfaceWorldTests.cpp',
    'cpppj/tests/fixtures/assetpostpop-x86.tsv', 'cpppj/assetpostpop.json',
    'tools/decomp_assetpostpop_oracle.py', 'cpppj/campaign-path-bindings.json',
    'tools/analyze_campaign_path_bindings.py', 'cpppj/run15.json',
)
PROTECTED = {  # 원본 판본의 불변 SHA이며 분석 후에도 같은 값이어야 한다.
    'originals/Netstorm.exe': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}


def sha(path):
    """파일 전체 원문 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def protect():
    """보호 실행 파일을 읽기만 하여 이전 기준과 비교한다."""
    # 두 판본을 각각 검증하며 새 출력으로 기준값을 바꾸지 않는다.
    for name, expected in PROTECTED.items():
        assert sha(ROOT / name) == expected, name


def capture():
    """최초 편집 전에 현재 원문을 한 번 보존하며 같은 이력을 덮지 않는다."""
    protect()
    assert not HISTORY.exists(), '기존 이력은 덮지 않습니다'
    saved = {}  # 각 보존본의 전체 SHA를 원래 경로로 조회하는 장부다.
    # 명시한 파일만 저장해 중첩 이력과 빌드 디렉터리의 재귀 복사를 피한다.
    for name in BEFORE:
        target = HISTORY / name  # 원래 상대경로를 그대로 유지하는 보존 파일이다.
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / name).read_bytes())
        saved[name] = sha(target)
    manifest = dict(host='HJOW-Athlon', baseline_commit='c241c6f', files=saved)  # 변경 전 PC·Git·원문 기준이다.
    (HISTORY / 'capture.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('캠페인16 변경 전 전체 원문 보존:', len(saved))


def history():
    """보존본과 기존 원본 입력·기대값의 불변을 읽기 전용으로 검사한다."""
    protect()
    saved = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))  # 변경 전 파일 해시 장부다.
    # 저장한 모든 원문을 전체 바이트로 확인한다.
    for name, expected in saved['files'].items():
        assert sha(HISTORY / name) == expected, name
    # 기존 원본 관찰·보고서는 현재도 같은 바이트여야 한다.
    for name in ('cpppj/tests/fixtures/assetpostpop-x86.tsv', 'cpppj/assetpostpop.json',
                 'tools/decomp_assetpostpop_oracle.py', 'cpppj/run15.json',
                 'cpppj/campaign-path-bindings.json', 'tools/analyze_campaign_path_bindings.py'):
        assert sha(ROOT / name) == saved['files'][name], name
    return saved


def log(path):
    """PowerShell UTF-16 또는 UTF-8 빌드 로그를 실제 인코딩으로 읽는다."""
    data = path.read_bytes()  # 콘솔 리다이렉션의 원문 바이트다.
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def source_delta(check=False):
    """변경 전 원문과 현재 전체 파일을 양방향 재구성해 줄바꿈을 포함한 정확 출처를 연결한다."""
    saved = history()  # 원래 직접 영향 파일의 전체 SHA다.
    result = {}  # 변경한 파일별 원문 해시와 정확 교체 범위다.
    # 명시 보존 파일만 비교하며 변경하지 않은 원본 fixture는 차이 대상에 넣지 않는다.
    for name in BEFORE:
        before = (HISTORY / name).read_bytes()  # 변경 전 전체 원문이다.
        after = (ROOT / name).read_bytes()  # 현재 전체 원문이다.
        if before == after:
            continue
        old_lines, new_lines = before.splitlines(keepends=True), after.splitlines(keepends=True)
        matcher = difflib.SequenceMatcher(None, old_lines, new_lines, autojunk=False)  # 동일 줄의 위치를 비교한다.
        edits = []  # 양방향 변환에 필요한 실제 교체 바이트다.
        # 변경된 모든 구간을 원래/현재 줄 인덱스와 함께 저장한다.
        for kind, old_start, old_end, new_start, new_end in matcher.get_opcodes():
            if kind != 'equal':
                edits.append(dict(old=[old_start, old_end], new=[new_start, new_end],
                                  removed=base64.b64encode(b''.join(old_lines[old_start:old_end])).decode('ascii'),
                                  inserted=base64.b64encode(b''.join(new_lines[new_start:new_end])).decode('ascii')))
        forward, reverse = list(old_lines), list(new_lines)  # 각 전체 파일을 재구성할 별도 줄 배열이다.
        # 뒤쪽부터 적용해 앞 구간의 원래 인덱스를 보존한다.
        for edit in reversed(edits):
            forward[edit['old'][0]:edit['old'][1]] = base64.b64decode(edit['inserted']).splitlines(keepends=True)
            reverse[edit['new'][0]:edit['new'][1]] = base64.b64decode(edit['removed']).splitlines(keepends=True)
        assert b''.join(forward) == after and b''.join(reverse) == before, name
        result[name] = dict(before_sha256=saved['files'][name], after_sha256=sha(ROOT / name), edits=edits)
    if check:
        assert json.loads(DELTA.read_text(encoding='utf-8')) == result
    else:
        DELTA.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return len(result)


def finalize():
    """성공한 실제 빌드·전체 검사와 현재 연결 소스의 전체 SHA를 새 보고서로 기록한다."""
    history()
    source_delta()
    names = set(BEFORE) | {  # 보존 원문과 새 실행 근거를 함께 기록한다.
        'tools/check_campaign16.py', 'cpppj/hist/run16-base/capture.json',
        'cpppj/build/bin/Release/NetstormCpp.exe', 'cpppj/build/bin/Release/netstorm_tests.exe',
        'extracted/run16-build.log', 'extracted/run16-focused.log', 'extracted/run16-all.log',
        'tools/decomp_asset_players.py', 'cpppj/asset-player.json',
        'cpppj/tests/fixtures/asset-player-x86.tsv',
        'cpppj/run16-delta.json', 'extracted/run16-assets.log', 'extracted/run16-world.log',
        'cpppj/hist/run16-fail/cpppj/tests/RawSurfaceWorldTests.cpp',
        'cpppj/hist/run16-fail/extracted/run16-world.log',
    }
    build = log(ROOT / 'extracted/run16-build.log')  # 실제 Release 빌드 출력이다.
    whole = log(ROOT / 'extracted/run16-all.log')  # 전체 C++ 검사 실행 출력이다.
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', build, re.I)
    summary = re.findall(r'(\d+) test\(s\), (\d+) failed check\(s\)', whole)  # 실제 실행기가 출력한 총계다.
    assert len(summary) == 1 and summary[0][1] == '0'
    focused = log(ROOT / 'extracted/run16-focused.log')  # 실제 공유기록·로딩 종료의 집중 검사 출력이다.
    assert '[FAIL]' not in focused and len(re.findall(r'\[ OK \]', focused)) == 16
    data = dict(schema=1, host='HJOW-Athlon', last_decompile_date='2026-10-14',
                validated_date='2026-10-15', unit='20261015-01',
                original_inputs_unchanged=True, campaign_completed=False,
                new_native_inputs=576, new_cpp_tests=6, focused_tests=16,
                all_tests=int(summary[0][0]), failed_checks=0,
                files={name: sha(ROOT / name) for name in sorted(names)})  # 이번 단위의 고정된 최종 근거다.
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('캠페인16 전체 원문·보호 PE·실제 빌드/검사 근거 기록:', data['all_tests'])


def verify():
    """최종 보고서의 모든 파일을 전체 바이트로 재확인한다."""
    history()
    source_delta(check=True)
    data = json.loads(REPORT.read_text(encoding='utf-8'))  # 최종 기록의 각 파일 기대 SHA다.
    # 고정한 모든 소스·입력·로그·실행 파일을 대조한다.
    for name, expected in data['files'].items():
        assert sha(ROOT / name) == expected, name
    print('캠페인16 최종 전체 SHA 감사 통과')


# 명시 CLI에서만 보존·마감·읽기 검사를 실행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 작업 단위의 실행 모드를 받는다.
    parser.add_argument('--capture', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()  # 실행할 보존/감사 모드다.
    capture() if args.capture else verify() if args.verify else finalize()
