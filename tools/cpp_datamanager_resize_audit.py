#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 최소 확장/생성/이름 교체 관측과 실제 getter·새 현재 버퍼 연결을 감사한다."""
import csv
import hashlib
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path
from cpp_datamanager_streams_audit import verify_original as verify_streams
from cpp_player_state_initialization_audit import verify_original as verify_initial_state

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소에서만 원문/소스/빌드 근거를 읽는다.
REPORT = ROOT / 'cpppj/datamanager-resize-session.json'  # 과거 감사 핀을 덮지 않는 현재 단위 기록이다.
CAPTURE = ROOT / 'extracted/datamanager-resize-20261021-first-capture'  # 제품 수정 전에 동결한 최초 세 파일이다.


def digest(path):
    """실제 파일 전체 바이트를 줄바꿈 변환 없이 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_original():
    """열 실제 본문의286행·전체796BYTE·반환/경계 자료 보존을 확인한다."""
    native = json.loads((ROOT / 'cpppj/datamanager-resize-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 286
    assert native['entries'] == {'0x44cc20': 314, '0x44cce0': 234, '0x44cd40': 120, '0x44ce10': 120, '0x44d040': 136, '0x44d0f0': 136, '0x44ce60': 478, '0x44d070': 200, '0x44d160': 152, '0x44d290': 174}
    # 원본·공용 기반·정밀 범위·최초 도구·fixture는 같은 바이트를 유지한다.
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    # 동결한 최초 파일을 제품 변경 뒤 다시 생성하지 않는다.
    for path in ('tools/decomp_datamanager_resize.py', 'cpppj/datamanager-resize-native.json', 'cpppj/tests/fixtures/datamanager-resize-x86.tsv'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/datamanager-resize-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    assert len(rows) == len({row[0] for row in rows}) == 286
    assert Counter(row[0][0] for row in rows) == {'G': 174, 'R': 32, 'C': 80}
    writable = set(range(12, 20))
    # 노드 prefix/next와 새 이름 노드 외의 이름·보호 바이트는 변경할 수 없다.
    for index in range(4):
        writable.update(range(36 + index * 76, 52 + index * 76));writable.update(range(92 + index * 76, 96 + index * 76))
    writable.update(range(28 + 3 * 76 + 24, 28 + 3 * 76 + 64))
    # 기존/임시 버퍼의 물리64BYTE만 원본 쓰기를 허용한다.
    for index in range(5):
        writable.update(range(340 + index * 80, 404 + index * 80))
    # 반환 핸들·전체 메모리와 각 경계 직전 스냅샷의 실제 폭을 확인한다.
    for row in rows:
        before, after = bytes.fromhex(row[1]), bytes.fromhex(row[2]);assert len(before) == len(after) == 796
        assert {index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]} <= writable
        assert 0 <= int(row[3]) <= 0xffffffff
        if row[4] != '-':
            for event in row[4].split(';'):
                assert len(bytes.fromhex(event.split(',')[2])) == 796
    return native


def verify_binding():
    """실제 Session getter가 분리 복사 대신 원본 최소 확장 부모의 새 현재 stream을 받는지 확인한다."""
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    state = (ROOT / 'cpppj/src/o/RawPlayerTemplateState.cpp').read_text(encoding='utf-8')
    assert 'templateState->BindMinimumSize(' in session and 'templateStorage->EnsureStateSize(owner,minimum)' in session
    assert 'templateStorage->GrowState(owner,size)' not in session and 'templateStorage->CommitStateGrowth(owner)' not in session
    assert 'current=&ensureMinimum_(slot,28)' in state
    assert 'o::EnsureDataManagerStreamSize(manager,"State",minimum,hooks)' in storage
    assert 'o::ConstructDataManagerStream(handle,std::string_view("State"),0x4000,hooks)' in storage
    assert 'fort.sections.at(StateSection())=std::move(buffer)' in storage and 'streams.emplace(stateHandle,std::move(temporary))' in storage


def main():
    """--record에서만 최종 근거를 쓰고 평소에는 보존 원문과 현재 바이트를 검증한다."""
    native = verify_original();verify_streams();verify_initial_state();verify_binding()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 소스/원문/빌드·세 바이너리는 기록 당시의 같은 전체 바이트여야 한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert not report['cpp_tests_executed'] and not report['game_executed']
        print(json.dumps({'original_rows_preserved': native['rows'], 'binding_verified': True, 'cpp_execution': 'deferred'}));return
    assert not REPORT.exists(), '현재 단위의 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-resize-20261021-build2.log';result = ROOT / 'extracted/cpp-datamanager-resize-20261021-build2-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8');assert not any(value in build for value in ('error C', 'error LNK', 'warning C', 'warning LNK'))
    assert all(value in build for value in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'))
    paths = ['cpppj/src/o/RawDataManagerResize.cpp', 'cpppj/src/o/RawDataManagerResize.h', 'cpppj/src/o/RawPlayerTemplateState.cpp', 'cpppj/src/o/RawPlayerTemplateState.h', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/DataManagerResizeTests.cpp', 'cpppj/datamanager-resize-native.json', 'tools/cpp_datamanager_resize_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/'), str(result.relative_to(ROOT)).replace('\\', '/')]
    # 빌드 산출물 위치를 명확하게 찾고 실제 새 검사 등록 문자열도 확인한다.
    for name in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name));assert len(candidates) == 1, (name, candidates)
        paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    compiled = ['DataManagerResize_Native286', 'CampaignTemplates_MinimumStateUsesTemporarySectionAndSharedData', 'CampaignTemplates_MinimumStateTailCopiesBeforeCompaction', 'DataManagerResize_SufficientStateAndRequiredBinding']
    test_binary = (ROOT / paths[-1]).read_bytes();assert all(name.encode('ascii') in test_binary for name in compiled)
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows': native['rows'], 'build_exit_code': 0, 'tests_written_and_compiled': compiled, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['열 본문을 직접 관측한다. CRT ASCII 비교/new/free/진단은 명시 경계이며 실제 CRT/힙/게임 실행은 아니다.', '실제 저장소는 완료 후 비공개 임시 노드/버퍼 토큰을 State 토큰으로 매핑한다. 원본 힙 주소/외부 노드 포인터 동일성 전체를 주장하지 않는다.', '분리 stream 입력의 기존 확장 어댑터는 유지하며 실제 Session은 전체 최소 확장 부모만 사용한다.', 'C++ 검사는 작성/컴파일했으며 실행 검증과11미션 자연 완주는 미완료다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'recorded': True, 'original_rows': native['rows'], 'build_exit_code': 0, 'cpp_execution': 'deferred'}))


if __name__ == '__main__':
    main()
