#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 생성/복제 원문 보존과 현재 실제 State 생성 부모·빌드 근거를 정적으로 감사한다."""
import csv
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path
from decomp_datamanager_create import decode, digest
from cpp_datamanager_resize_audit import verify_original as verify_resize
from cpp_datamanager_streams_audit import verify_original as verify_streams
from cpp_player_state_initialization_audit import verify_original as verify_initial_state

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소의 보존 원문과 최종 바이트만 읽는다.
REPORT = ROOT / 'cpppj/datamanager-create-session.json'  # 이전 단위의 소스/빌드 핀은 덮지 않는다.
CAPTURE = ROOT / 'extracted/datamanager-create-20261021-first-capture'  # 제품 수정 전에 동결한 최초 세 파일이다.
COMPILED = ['DataManagerCreate_Native220', 'DataManagerCreate_EmptyNullAllocationAndRequiredEffects', 'MemStreamCopy_SelfAssignmentResetsBeforeReadingLength', 'CampaignTemplates_StateCreationParentPreservesSharedOrderAndTail']  # 컴파일됐지만 실행 이월된 새 검사 이름이다.


def verify_original():
    """전체17380BYTE·반환·각 경계 상태와 열한 실제 본문의 최초220행을 확인한다."""
    native = json.loads((ROOT / 'cpppj/datamanager-create-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 220 and native['snapshot_bytes'] == 17380
    assert native['entries'] == {'0x44cc20': 522, '0x44cc80': 180, '0x44cce0': 152, '0x44cd40': 152, '0x44ce10': 190, '0x44ce60': 350, '0x44d040': 114, '0x44d070': 190, '0x44d0f0': 114, '0x44d1b0': 190, '0x44d230': 160}
    # 최초 도구/정밀 범위/원본/전체fixture의 바이트가 바뀌면 실패한다.
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    for path in ('tools/decomp_datamanager_create.py', 'cpppj/datamanager-create-native.json', 'cpppj/tests/fixtures/datamanager-create-x86.tsv'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/datamanager-create-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    assert len(rows) == len({row[0] for row in rows}) == 220 and Counter(row[0][0] for row in rows) == {'N': 30, 'C': 160, 'M': 30}
    writable = set(range(12, 20)) | set(range(40, 48))
    # 기존 이름과 양쪽 보호 바이트는 그대로이며 새 노드 이름만 쓸 수 있다.
    for index in range(5):
        writable.update(range(64 + index * 76, 80 + index * 76));writable.update(range(120 + index * 76, 124 + index * 76))
    writable.update(range(56 + 4 * 76 + 24, 56 + 4 * 76 + 64))
    # 작은64BYTE 할당과 실제16384BYTE 전체만 제품 변경 전에 쓰기를 허용했다.
    for index in range(6):
        writable.update(range(444 + index * 80, 508 + index * 80))
    writable.update(range(924, 924 + 0x4000))
    for row in rows:
        before, after = decode(row[1]), decode(row[2]);assert len(before) == len(after) == 17380
        assert {index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]} <= writable
        assert row[3] == '-' if row[0][0] == 'M' else 0 <= int(row[3]) <= 0xffffffff
        if row[4] != '-':
            for event in row[4].split(';'):
                assert len(decode(event.split(',')[2])) == 17380
    return native


def verify_binding():
    """현재 State 초기화가 수동 remove/append 조합 대신 같은 전체 생성 부모를 호출하는지 확인한다."""
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    start = storage.index('void InitializeState(');end = storage.index('void EnsureStateSize(', start);creation = storage[start:end]
    assert 'o::CreateDataManagerStream(manager,"State",hooks)' in creation
    assert 'o::RemoveDataManagerStream(' not in creation and 'o::AppendDataManagerStream(' not in creation and 'o::ConstructDataManagerStream(' not in creation
    assert 'streams.emplace(handle,std::make_unique<Stream>())' in creation and 'fort.sections.at(StateSection())=std::move(allocation)' in creation
    assert 'streamOrder.push_back(handle);PublishCursor(handle);WriteStream(handle,input)' in creation
    resize = (ROOT / 'cpppj/src/o/RawDataManagerResize.cpp').read_text(encoding='utf-8')
    assert 'const auto stream=CreateDataManagerStream(manager,name,hooks)' in resize and 'if (Read(source,0)) CopyMemStream(target,source,hooks.memory)' in resize
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    assert 'templateStorage->InitializeState(owner,bytes)' in session and 'templateStorage->EnsureStateSize(owner,minimum)' in session


def main():
    """새 감사에서만 현재 근거를 기록하고 과거 핀은 최초 원문 보존 확인에만 사용한다."""
    native = verify_original();verify_resize();verify_streams();verify_initial_state();verify_binding()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert not report['cpp_tests_executed'] and not report['game_executed']
        print(json.dumps({'original_rows_preserved': native['rows'], 'binding_verified': True, 'cpp_execution': 'deferred'}));return
    assert not REPORT.exists(), '현재 단위의 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-create-20261021-build2.log';result = ROOT / 'extracted/cpp-datamanager-create-20261021-build2-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8');assert not any(value in build for value in ('error C', 'error LNK', 'warning C', 'warning LNK'))
    assert all(value in build for value in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'))
    paths = ['cpppj/src/o/RawDataManagerResize.cpp', 'cpppj/src/o/RawDataManagerResize.h', 'cpppj/src/o/RawMemStream.cpp', 'cpppj/src/o/RawMemStream.h', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/RawPlayerTemplateState.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/DataManagerCreateTests.cpp', 'cpppj/datamanager-create-native.json', 'tools/cpp_datamanager_create_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/'), str(result.relative_to(ROOT)).replace('\\', '/')]
    # 빌드 산출물을 실제로 찾고 새 검사가 등록된 바이너리를 읽기만 한다.
    for name in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name));assert len(candidates) == 1, (name, candidates);paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    test_binary = (ROOT / paths[-1]).read_bytes();assert all(name.encode('ascii') in test_binary for name in COMPILED)
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows': native['rows'], 'snapshot_bytes': 17380, 'build_exit_code': 0, 'tests_written_and_compiled': COMPILED, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['열한 원본 본문을 직접 관측했다. CRT ASCII 비교/new/free/진단은 명시 경계이며 실제 CRT/힙/게임 실행은 아니다.', '실제 State 생성 소비자만 전체 생성 부모에 연결했다. 복제 공통 몸체는 별도 컴파일 검사이며 파일 읽기/전체 기본 섹션 생성 부모/전송 직렬화 연결은 남는다.', '실제 호스트는 완료한 같은 새 할당을 소유하지만 원본 힙 주소/미정 padding/외부 raw 노드 포인터 정체성 전체의 일치를 주장하지 않는다.', 'C++ 검사 실행과11미션 자연 완주는 미완료이며 기존 실행 이월 지시를 유지한다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'recorded': True, 'original_rows': native['rows'], 'build_exit_code': 0, 'cpp_execution': 'deferred'}))


if __name__ == '__main__':
    main()
