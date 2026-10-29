#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 여섯 본문 관측과 현재 공통 함수/실제 소비자·빌드를 구분해 감사한다."""
import csv
import hashlib
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path
from cpp_player_state_initialization_audit import verify_original as verify_initial_state

ROOT = Path(__file__).resolve().parents[1]  # 현재 작업 저장소에서만 근거를 읽는다.
REPORT = ROOT / 'cpppj/datamanager-streams-session.json'  # 과거 소스/실행 핀을 덮지 않는 현재 단위 기록이다.
CAPTURE = ROOT / 'extracted/datamanager-streams-20261021-first-capture'  # 제품 변경 전에 동결한 최초 세 파일이다.


def digest(path):
    """줄바꿈을 변환하지 않고 실제 전체 파일 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_original():
    """최초404고유 입력·전체716BYTE·허용 쓰기 범위와 모든 하위 진입을 확인한다."""
    native = json.loads((ROOT / 'cpppj/datamanager-streams-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 404
    assert native['entries'] == {'0x44cc20': 114, '0x44cce0': 234, '0x44cd40': 120, '0x44ce10': 108, '0x44d040': 32, '0x44d0f0': 32}
    # 원본 PE/정밀 범위/처음 도구/fixture는 기록 당시의 같은 바이트여야 한다.
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    # 최초 세 파일의 동결 사본도 원문 그대로 유지한다.
    for path in ('tools/decomp_datamanager_streams.py', 'cpppj/datamanager-streams-native.json', 'cpppj/tests/fixtures/datamanager-streams-x86.tsv'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/datamanager-streams-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    assert len(rows) == len({row[0] for row in rows}) == 404
    assert Counter(row[0][0] for row in rows) == {'A': 48, 'W': 168, 'C': 48, 'P': 108, 'R': 32}
    writable = set(range(12, 20))
    # 이름/보호 영역/원문 배열은 허용 쓰기에 포함하지 않는다.
    for index in range(4):
        writable.update(range(36 + index * 76, 52 + index * 76))
        writable.update(range(92 + index * 76, 96 + index * 76))
        writable.update(range(340 + index * 80, 404 + index * 80))
    # 경계 직전 스냅샷과 최종 메모리는 모두 같은 전체716BYTE를 보존한다.
    for row in rows:
        before, after = bytes.fromhex(row[1]), bytes.fromhex(row[2]);assert len(before) == len(after) == 716
        assert {index for index, pair in enumerate(zip(before, after)) if pair[0] != pair[1]} <= writable
        if row[3] != '-':
            for event in row[3].split(';'):
                assert len(bytes.fromhex(event.split(',')[2])) == 716
    return native


def verify_binding():
    """실제 State 재생성이 다섯 공통 함수와 기존 소멸을 사용하도록 연결됐는지 확인한다."""
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    assert 'o::RemoveDataManagerStream(manager,handle,ListEffects())' in storage
    assert 'o::AppendDataManagerStream(manager,handle,ListEffects())' in storage
    assert 'o::AllocateMemStream(streams.at(handle)->bytes,0x4000,hooks)' in storage
    assert 'o::WriteMemStream(streams.at(handle)->bytes,input' in storage
    assert 'o::CompactMemStream(streams.at(handle)->bytes,hooks)' in storage
    assert 'buffer.assign(0x4000,0)' not in storage and 'auto previous=std::move(buffer)' not in storage
    assert 'RawMemStream.cpp RawMemStream.h RawDataManagerList.cpp RawDataManagerList.h' in (ROOT / 'cpppj/src/o/CMakeLists.txt').read_text(encoding='utf-8')
    assert 'DataManagerStreamsTests.cpp' in (ROOT / 'cpppj/tests/CMakeLists.txt').read_text(encoding='utf-8')


def main():
    """--record에서만 새 최종 근거를 쓰고 그 외에는 현재 전체 바이트를 확인한다."""
    native = verify_original();verify_binding();verify_initial_state()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 제품/원문/빌드·실행 파일은 기록 당시 바이트와 같아야 한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert not report['cpp_tests_executed'] and not report['game_executed']
        print(json.dumps({'original_rows_preserved': native['rows'], 'binding_verified': True, 'cpp_execution': 'deferred'}));return
    assert not REPORT.exists(), '현재 단위의 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-streams-20261021-build2.log'
    result = ROOT / 'extracted/cpp-datamanager-streams-20261021-build2-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8');assert not any(value in build for value in ('error C', 'error LNK', 'warning C', 'warning LNK'))
    assert all(value in build for value in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'))
    paths = ['cpppj/src/o/RawMemStream.cpp', 'cpppj/src/o/RawMemStream.h', 'cpppj/src/o/RawDataManagerList.cpp', 'cpppj/src/o/RawDataManagerList.h', 'cpppj/src/o/RawDataManagerLifetime.cpp', 'cpppj/src/o/RawDataManagerLifetime.h', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/DataManagerStreamsTests.cpp', 'cpppj/tests/CMakeLists.txt', 'cpppj/datamanager-streams-native.json', 'tools/decomp_datamanager_streams.py', 'tools/decomp_local_mission_transport.py', 'tools/cpp_datamanager_streams_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/'), str(result.relative_to(ROOT)).replace('\\', '/')]
    # 실행기 공용 기반은 작업 시작 커밋과 같은 소스임을 별도로 확인한다.
    common = 'tools/decomp_local_mission_transport.py';start = subprocess.check_output(['git', 'show', '0429e571:' + common], cwd=ROOT)
    assert start == (ROOT / common).read_bytes(), '공용 실행기 기반이 시작 소스와 달라짐'
    # 세 최종 빌드 산출물을 찾고 실제 새 검사 등록 문자열을 읽기 전용 확인한다.
    for name in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name));assert len(candidates) == 1, (name, candidates)
        paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    compiled = ['DataManagerStreams_Native404', 'DataManagerStreams_PhysicalAndZeroContracts', 'DataManagerStreams_RemoveRequeriesRepairedPredecessor', 'CampaignTemplates_CommonStreamsRecreateAndReleaseEmptyTail']
    test_binary = (ROOT / paths[-1]).read_bytes();assert all(name.encode('ascii') in test_binary for name in compiled)
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows': native['rows'], 'build_exit_code': 0, 'tests_written_and_compiled': compiled, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['여섯 본문은 직접 관측하며 new/free/진단만 명시 경계다. 실제 CRT/OS/게임 실행은 아니다.', '실제 템플릿은 원시 섹션 벡터/임시 버퍼 토큰 어댑터다. 전체 DataManager 생성/파일/오류/힙 주소 동등성 복원은 아니다.', '새404행과 이전 State 초기화2560행은 보존한다. C++ 검사는 실행하지 않았다.', '전체11미션 자연 완주와 원본 외관/음향/동작 동일성은 미완료다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'recorded': True, 'original_rows': native['rows'], 'build_exit_code': 0, 'cpp_execution': 'deferred'}))


if __name__ == '__main__':
    main()
