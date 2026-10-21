#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 State 초기화 관측과 현재 실제 생성 연결/빌드를 구분하여 감사한다."""
import csv
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 모든 근거는 현재 작업 저장소 기준으로 읽는다.
REPORT = ROOT / 'cpppj/player-state-initialization-session.json'  # 과거 session 핀을 덮지 않는 현재 단위 기록이다.
CAPTURE = ROOT / 'extracted/player-state-initialization-20261021-first-capture'  # 제품 변경 전에 동결한 최초 도구/JSON/fixture다.


def digest(path):
    """줄바꿈을 변환하지 않고 전체 파일 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_original():
    """최초2560고유 입력과 보존 영역/부모 생성 경계 관측을 확인한다."""
    native = json.loads((ROOT / 'cpppj/player-state-initialization-native.json').read_text(encoding='utf-8'))
    assert native['rows'] == native['normal_returns'] == 2560
    assert native['entries'] == {'0x4bd550': 2560, '0x44cce0': 1536}
    # 원본·도구·독립 fixture 해시는 제품 수정 전 기록을 유지해야 한다.
    for path, expected in native['hashes'].items():
        assert digest(ROOT / path) == expected, path
    # 최초 세 파일은 재생성하거나 정규화하지 않는다.
    for path in ('tools/decomp_player_state_initialization.py', 'cpppj/player-state-initialization-native.json', 'cpppj/tests/fixtures/player-state-initialization-x86.tsv'):
        assert digest(ROOT / path) == digest(CAPTURE / Path(path).name), path
    with (ROOT / 'cpppj/tests/fixtures/player-state-initialization-x86.tsv').open(encoding='utf-8') as stream:
        rows = list(csv.reader(stream, delimiter='\t'))[1:]
    assert len(rows) == len({row[0] for row in rows}) == 2560
    # 원본이 보존한 미정 바이트는 생성 여부와 무관하게 같은 입력을 유지한다.
    for row in rows:
        before, after, node, buffer = (bytes.fromhex(row[index]) for index in range(1, 5))
        assert len(before) == len(after) == 28 and len(node) == 16 and len(buffer) == 64
        for index in (1, 2, 3, 10, 11, 18, 19, 20, 21, 26, 27):
            assert before[index] == after[index]
        assert row[5] in ('SCE', 'SE')
        if row[5] == 'SCE':
            assert buffer[:28] == after and int.from_bytes(node[4:8], 'little') == 28 and int.from_bytes(node[8:12], 'little') == 28
    return native


def verify_binding():
    """실제 Client 미정 입력과 Session getter/같은 State 섹션 재생성을 확인한다."""
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    client = (ROOT / 'cpppj/src/client/CampaignClientServices.cpp').read_text(encoding='utf-8')
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    state = (ROOT / 'cpppj/src/o/RawPlayerTemplateState.cpp').read_text(encoding='utf-8')
    assert 'services.templateStateBytes=CurrentAllocationBytes;' in client
    assert 'templateState->BindInitialization(' in session and 'templateStorage->InitializeState(owner,bytes)' in session
    assert 'InitializePlayerTemplateStateBytes(bytes)' in state and 'current=&initialization_.replace(slot,bytes)' in state
    assert 'std::erase(streamOrder,handle);state={{},0,0,false};' in storage and 'buffer.assign(0x4000,0)' in storage
    assert 'Put(manager,8,handle);streamOrder.push_back(handle);' in storage
    assert 'for (const auto handle:streamOrder)' in storage


def main():
    """--record에서만 새 최종 근거를 쓰고 나머지는 현재 바이트만 검증한다."""
    native = verify_original();verify_binding()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 소스/세 바이너리는 기록 당시의 같은 바이트여야 한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        assert report['cpp_tests_executed'] is False and report['game_executed'] is False
        print(json.dumps({'original_rows_preserved': native['rows'], 'binding_verified': True, 'cpp_execution': 'deferred'}, ensure_ascii=False));return
    assert not REPORT.exists(), '현재 단위의 최종 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-player-state-initialization-20261021-build2.log'
    result = ROOT / 'extracted/cpp-player-state-initialization-20261021-build2-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8')
    assert not any(text in build for text in ('error C', 'error LNK', 'warning C', 'warning LNK'))
    assert all(text in build for text in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'))
    paths = ['cpppj/src/o/RawPlayerTemplateState.cpp', 'cpppj/src/o/RawPlayerTemplateState.h', 'cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/CampaignClientServices.cpp', 'cpppj/tests/PlayerStateInitializationTests.cpp', 'cpppj/tests/CMakeLists.txt', 'cpppj/player-state-initialization-native.json', 'tools/cpp_player_state_initialization_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/'), str(result.relative_to(ROOT)).replace('\\', '/')]
    # 최종 라이브러리/게임/검사 실행 파일의 위치를 명확하게 고정한다.
    for name in ('netstorm.lib', 'NetstormCpp.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name));assert len(candidates) == 1, (name, candidates)
        paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    compiled_tests = ['PlayerStateInitialization_Native2560', 'CampaignTemplates_EmptyStateRecreatedWithCurrentBytesAndOrder', 'CampaignTemplates_StateRecreationCompactsPreviousTerritoryTail', 'PlayerStateInitialization_RequiredEffectsAndEdition']
    test_binary = (ROOT / paths[-1]).read_bytes()
    assert all(name.encode('ascii') in test_binary for name in compiled_tests), '새 네 검사 등록 문자열이 최종 바이너리에 없음'
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows': native['rows'], 'build_exit_code': 0, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['원본 관측 본문은4bd550/44cce0이며 선택/섹션 생성/확장은 명시 경계다.', '호스트 섹션 재생성/버퍼/목록 어댑터다. 전체 DataManager 생성/파일/오류/힙 동등성의 관측이 아니다.', '미정 State 바이트는 현재 호스트 malloc 입력이다. 원본 실행 당시 스택 바이트와 같다고 주장하지 않는다.', 'C++ 검사는 작성/컴파일했으며 실행 검증과11미션 자연 완주는 미완료다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    report['tests_written_and_compiled'] = compiled_tests
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'recorded': True, 'original_rows': native['rows'], 'build_exit_code': 0, 'cpp_execution': 'deferred'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
