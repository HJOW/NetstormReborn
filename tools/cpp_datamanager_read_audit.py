#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""파일 읽기 부모의 최초144행 보존과 같은 초기/재로딩 소비자·현재 빌드 근거를 감사한다."""
import csv
import json
import subprocess
import sys
from pathlib import Path
from decomp_datamanager_read import decode, digest
from cpp_datamanager_create_audit import verify_original as verify_create
from cpp_datamanager_resize_audit import verify_original as verify_resize

ROOT = Path(__file__).resolve().parents[1]  # 원문/현재 소스/실제 빌드 바이트만 읽는다.
REPORT = ROOT / 'cpppj/datamanager-read-session.json'  # 이전 단위의 현재 소스 핀을 수정하지 않는 새 기록이다.
CAPTURE = ROOT / 'extracted/datamanager-read-20261021-first-capture'  # 제품 변경 전에 동결한 최초 세 파일이다.
COMPILED = ['DataManagerRead_Native144', 'DataManagerRead_MarkAssertionUsesCurrentFields', 'DataManagerRead_InitialPositionAndRequiredNames', 'CampaignTemplates_ReadTargetElevenFiles']  # 실행 이월된 새4검사의 실제 등록 문자열이다.


def verify_original():
    """전체1400BYTE/각 경계 직전 상태와 아홉 실제 본문의144행이 그대로인지 확인한다."""
    native = json.loads((ROOT / 'cpppj/datamanager-read-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 144 and native['snapshot_bytes'] == 1400
    assert native['entries'] == {'0x44cc20': 504, '0x44cce0': 72, '0x44cdb0': 144, '0x44d040': 96, '0x44d070': 432, '0x44d0f0': 96, '0x44d380': 144, '0x4a8800': 72, '0x4a8850': 576}
    # 최초 원본/정밀 범위/도구/fixture는 한 바이트라도 변경되면 실패한다.
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    for path in ('tools/decomp_datamanager_read.py', 'cpppj/datamanager-read-native.json', 'cpppj/tests/fixtures/datamanager-read-x86.tsv'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/datamanager-read-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    assert len(rows) == len({row[0] for row in rows}) == 144
    writable = set(range(12, 20)) | set(range(36, 56)) | set(range(1336, 1400))
    # 기존 이름/입력 원문/보호 바이트는 보존하며 새 노드 이름만 쓸 수 있다.
    for index in range(6):
        writable.update(range(72 + index * 76, 88 + index * 76));writable.update(range(128 + index * 76, 132 + index * 76))
        if index >= 3:
            writable.update(range(88 + index * 76, 128 + index * 76))
    for index in range(7):
        writable.update(range(528 + index * 80, 592 + index * 80))
    for row in rows:
        before, after = decode(row[1]), decode(row[2]);assert len(before) == len(after) == 1400
        assert {index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]} <= writable
        for event in row[3].split(';'):
            assert len(decode(event.split(',')[2])) == 1400
    return native


def verify_binding():
    """실제 초기 파일/재로딩은 모두 같은 원문을 읽기 부모의 빈 manager에 전달하는지 확인한다."""
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    start = storage.index('void ReadFile(');end = storage.index('Image(o::FortTemplate', start);reader = storage[start:end]
    assert 'o::ReadDataManagerImage(manager,cursor,hooks)' in reader and 'fort.sections.clear();fort.sections.resize' in reader
    assert 'PublishCursor(current)' in reader and 'streamOrder.push_back(current)' in reader
    assert 'std::make_unique<Image>(std::move(fort),std::move(name),true);image->ReadFile(bytes)' in storage
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    assert 'templateStorage->InstallFile(1,std::move(fort),*services.missionFortImage' in session and 'services.missionFortImage.reset()' in session
    login = (ROOT / 'cpppj/src/client/RawCampaignSessionLogin.inc').read_text(encoding='utf-8')
    assert 's.templateStorage->InstallFile(owner,FortTemplate::Parse(bytes,s.assets.game.TypeTable()),bytes,name)' in login
    ui = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text(encoding='utf-8')
    assert 'o::FortTemplate::Parse(*fortImage, client_.Assets().TypeTable())' in ui and 'services.missionFortImage=fortImage' in ui
    tests = (ROOT / 'cpppj/tests/DataManagerReadTests.cpp').read_text(encoding='utf-8')
    assert all('"'+name+'"' in tests for name in ('bridgethegap', 'secretworkshop', 'capturethepriest', 'tacticalcombat', 'subtledefense', 'rawpower', 'thewarbegins', 'masterofwhirligigs', 'demostormrages', 'guardmyback', 'dissolvedalliance'))


def main():
    """--record에서만 새 현재 근거를 쓰며 과거 session 핀은 다시 만들지 않는다."""
    native = verify_original();verify_create();verify_resize();verify_binding()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert not report['cpp_tests_executed'] and not report['game_executed'];print(json.dumps({'original_rows_preserved': 144, 'initial_and_reload_binding': True, 'cpp_execution': 'deferred'}));return
    assert not REPORT.exists(), '현재 단위의 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-read-20261021-build.log';result = ROOT / 'extracted/cpp-datamanager-read-20261021-build-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8');assert not any(value in build for value in ('error C', 'error LNK', 'warning C', 'warning LNK'));assert all(value in build for value in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'))
    paths = ['cpppj/src/o/RawDataManagerImage.cpp', 'cpppj/src/o/RawDataManagerImage.h', 'cpppj/src/o/RawDataManagerResize.cpp', 'cpppj/src/o/RawDataManagerResize.h', 'cpppj/src/o/RawDataManagerList.cpp', 'cpppj/src/o/RawDataManagerLifetime.cpp', 'cpppj/src/o/RawMemStream.cpp', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSessionLogin.inc', 'cpppj/src/client/UberGump.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/DataManagerReadTests.cpp', 'cpppj/datamanager-read-native.json', 'tools/cpp_datamanager_read_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/'), str(result.relative_to(ROOT)).replace('\\', '/')]
    # 빌드 결과를 실제 위치에서 읽고 작성한4검사 등록 문자열도 확인한다.
    for name in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name));assert len(candidates) == 1, (name, candidates);paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    binary = (ROOT / paths[-1]).read_bytes();assert all(name.encode('ascii') in binary for name in COMPILED)
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows': 144, 'snapshot_bytes': 1400, 'build_exit_code': 0, 'tests_written_and_compiled': COMPILED, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['큰 원본 읽기 부모/하위 아홉 본문을 직접 관측한다. CRT 비교/new/free/가상 이름 표만 명시 경계이며 실제 CRT/힙/OS/게임 실행은 아니다.', '실제 Client 초기/로그인 재로딩은 동일 파일 원문을 같은 새 목록/버퍼의 읽기 부모로 전달한다. 파일 열기/서명 가상 부모/저장/기본 섹션 전체 생성/전체 오류 처리는 미완료다.', '보조 FortTemplate 해석과 명시 입력의 독립 Session 어댑터는 유지한다. 원본 힙 주소/미정 padding/외부 raw 포인터 정체성의 전체 일치를 주장하지 않는다.', '목표11파일 검사는 작성/컴파일했으며 C++ 실행과 실제11미션 자연 완주/동작 동일성은 미검증이다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({'recorded': True, 'original_rows': 144, 'build_exit_code': 0, 'cpp_execution': 'deferred'}))


if __name__ == '__main__':
    main()
