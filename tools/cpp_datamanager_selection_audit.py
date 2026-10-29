#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 원문 보존·현재 제품 연결·빌드만 감사하고 C++/게임 실행은 통합 단계로 남긴다."""
import csv
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 모든 경로는 같은 작업 저장소에서만 해석한다.
REPORT = ROOT / 'cpppj/datamanager-selection-session.json'  # 현재 소스/빌드 근거를 과거 결과와 구분한다.
CAPTURE = ROOT / 'extracted/datamanager-selection-20261021-first-capture'  # 제품 수정 전에 동결한 세 최초 파일이다.


def digest(path):
    """전체 파일 바이트를 변경하지 않고 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_native():
    """원문864행의 고유 입력·전체 폭·쓰기 범위·첫 자료 보존을 확인한다."""
    native = json.loads((ROOT / 'cpppj/datamanager-selection-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 864
    assert native['entries'] == {'0x44ce60': 864, '0x44ceb0': 432}
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    # 동결한 원문/도구/보고서는 새 제품 구현에서 다시 생성하지 않는다.
    for path in ('tools/decomp_datamanager_selection.py', 'cpppj/tests/fixtures/datamanager-selection-x86.tsv', 'cpppj/datamanager-selection-native.json'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/datamanager-selection-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    assert len(rows) == len({row['input'] for row in rows}) == 864
    writable = set(range(16, 20))
    # raw 두 함수와 명시 경계 변이는 manager.current/각 노드.pos만 변경할 수 있다.
    for index in range(3):
        writable.update(range(28 + index * 76 + 12, 28 + index * 76 + 16))
    for row in rows:
        before, after = bytes.fromhex(row['before']), bytes.fromhex(row['after'])
        assert len(before) == len(after) == 256
        assert {index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]} <= writable
        if row['events'] != '-':
            for event in row['events'].split(';'):
                assert len(bytes.fromhex(event.split(',')[2])) == 256
    return native


def verify_sources():
    """실제 미션 Stategetter가 현재 manager 선택을 통과하는 소스 연결을 확인한다."""
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    assert 'return templateStorage->SelectState(slot);' in session
    assert 'o::SelectDataManagerSection(manager,"State",1,hooks)' in storage
    assert 'image.SelectState();return image.state;' in storage
    assert 'RawDataManagerSelection.cpp RawDataManagerSelection.h' in (ROOT / 'cpppj/src/o/CMakeLists.txt').read_text(encoding='utf-8')
    assert 'DataManagerSelectionTests.cpp' in (ROOT / 'cpppj/tests/CMakeLists.txt').read_text(encoding='utf-8')


def main():
    """최종 빌드 후 현재 근거를 저장하거나 저장한 동일 근거만 읽기 전용 확인한다."""
    native = verify_native()
    verify_sources()
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert report['cpp_tests_executed'] is False and report['game_executed'] is False
        print(json.dumps({'native_rows_preserved': native['rows'], 'source_binding_verified': True, 'cpp_execution': 'deferred'}, ensure_ascii=False))
        return
    assert not REPORT.exists(), '기존 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-selection-20261021-build2.log'
    build = log.read_text(encoding='utf-8-sig')
    assert 'error C' not in build and 'error LNK' not in build and 'warning C' not in build and 'warning LNK' not in build
    assert 'netstorm_tests.exe' in build and 'Netstorm.exe' in build and 'netstorm.lib' in build
    paths = ['cpppj/src/o/RawDataManagerSelection.h', 'cpppj/src/o/RawDataManagerSelection.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/DataManagerSelectionTests.cpp', 'cpppj/datamanager-selection-native.json', 'tools/cpp_datamanager_selection_audit.py', str(log.relative_to(ROOT)).replace('\\', '/')]
    # 현재 세 바이너리와 컴파일된 소스만 고정하며 과거 감사의 해시는 덮지 않는다.
    for name in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name))
        assert len(candidates) == 1, (name, candidates)
        paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'native_rows': native['rows'], 'build_exit_code': 0, 'cpp_tests_executed': False, 'game_executed': False, 'tests_written_and_compiled': ['DataManagerSelection_Native864', 'DataManagerSelection_PhysicalContractsAndAsciiNames', 'CampaignTemplates_PlayerSelectsCurrentSectionAndSharedCursor'], 'limitations': ['원본 두 본문은 CRT 비교/진단만 명시 경계다. 실제 CRT/게임/OS 실행이 아니다.', 'Stategetter의 실제 검색/선택 연결이다. 전체 DataManager 생성/파일/저장/오류·빈 State 초기화/전체 MemStream은 미완료다.', '기존 현재 소스 핀은 갱신하지 않았다. 과거 빌드/검사 결과는 이번 소스의 검증이 아니다.', '지정11미션 자연 완주와 외관/음향/동작 동일성은 미완료다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'recorded_native_rows': native['rows'], 'build_exit_code': 0, 'cpp_execution': 'deferred'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
