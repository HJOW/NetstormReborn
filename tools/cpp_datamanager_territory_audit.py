#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 섹션 선택 근거를 재사용하여 실제 지형 getter의 연결·동일 버퍼·현재 빌드를 감사한다."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from cpp_datamanager_selection_audit import verify_native

ROOT = Path(__file__).resolve().parents[1]  # 동일 작업 저장소의 현재 소스만 검사한다.
REPORT = ROOT / 'cpppj/datamanager-territory-session.json'  # 단위263의 과거 핀을 덮지 않는 현재 기록이다.


def digest(path):
    """파일 전체 바이트의 SHA-256을 읽으며 줄바꿈을 정규화하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_binding():
    """실제 Session의 두 선택과 원시 Territory 버퍼 연결을 확인한다."""
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8')
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text(encoding='utf-8')
    tests = (ROOT / 'cpppj/tests/DataManagerSelectionTests.cpp').read_text(encoding='utf-8')
    assert 'return templateStorage->SelectTerritory(slot);' in session
    assert 'return templateStorage->SelectState(slot);' in session
    assert 'o::SelectDataManagerSection(manager,"Territory",0,hooks)' in storage
    assert 'std::span<const std::uint8_t>(fort.sections[territoryAt])' in storage
    assert 'territory={fort.territory' not in storage
    assert 'CampaignTemplates_TerritoryAndStateSelectSameManager' in tests
    assert 'before.data()==territory.bytes.data()' in tests


def main():
    """보존 원문/연결을 검사하고 --record일 때만 새 최종 근거를 작성한다."""
    native = verify_native()
    verify_binding()
    if '--record' not in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 소스/바이너리와 각 보존 자료는 기록 당시와 같아야 한다.
        for path, value in report['hashes'].items():
            assert digest(ROOT / path) == value, path
        assert report['cpp_tests_executed'] is False and report['game_executed'] is False
        print(json.dumps({'original_rows_preserved': native['rows'], 'territory_binding_verified': True, 'cpp_execution': 'deferred'}, ensure_ascii=False))
        return
    assert not REPORT.exists(), '기존 단위264 근거 덮어쓰기 금지'
    log = ROOT / 'extracted/cpp-datamanager-territory-20261021-build3.log'
    result = ROOT / 'extracted/cpp-datamanager-territory-20261021-build3-result.json'
    assert json.loads(result.read_text(encoding='utf-8'))['exit_code'] == 0
    build = log.read_text(encoding='utf-8')
    assert 'error C' not in build and 'error LNK' not in build and 'warning C' not in build and 'warning LNK' not in build
    assert 'netstorm.lib' in build and 'Netstorm.exe' in build and 'netstorm_tests.exe' in build
    paths = ['cpppj/src/client/CampaignTemplateStorage.cpp', 'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/RawDataManagerSelection.cpp', 'cpppj/src/o/RawDataManagerSelection.h', 'cpppj/tests/DataManagerSelectionTests.cpp', 'cpppj/datamanager-selection-native.json', 'cpppj/tests/fixtures/datamanager-selection-x86.tsv', 'tools/cpp_datamanager_selection_audit.py', 'tools/cpp_datamanager_territory_audit.py', '.gitattributes', str(log.relative_to(ROOT)).replace('\\', '/')]
    paths.append(str(result.relative_to(ROOT)).replace('\\', '/'))
    # 세 최종 빌드 산출물의 위치가 모호하면 기록하지 않는다.
    for name in ('netstorm.lib', 'Netstorm.exe', 'netstorm_tests.exe'):
        candidates = list((ROOT / 'cpppj/build').rglob(name))
        assert len(candidates) == 1, (name, candidates)
        paths.append(str(candidates[0].relative_to(ROOT)).replace('\\', '/'))
    report = {'start_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'host': native['host'], 'last_decompile_host': 'VM-W11-CODEX', 'last_decompile_date': '2026-10-21', 'original_rows_reused': native['rows'], 'new_original_capture': False, 'build_exit_code': 0, 'cpp_tests_executed': False, 'game_executed': False, 'limitations': ['실제 Territory 섹션 선택/원시 버퍼 연결이며 전체 DataManager/파일/오류/전체 MemStream은 미완료다.', '단위263의 선택864행을 보존해 재사용한다. 새 원본 관측 수로 합산하지 않는다.', '검사는 작성/컴파일했으며 실제 C++/게임 실행과11미션 자연 완주는 미완료다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'recorded': True, 'build_exit_code': 0, 'cpp_execution': 'deferred'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
