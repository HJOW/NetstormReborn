#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DataManager/섹션 소멸의 최초 원문·실제 저장소 연결·빌드와 과거 기록 보존을 읽기 전용으로 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/datamanager-lifetime-session.json'  # 전체 DataManager/State7 완성과 이번 소멸 단위를 구분한다.


def main():
    """제품/게임/C++검사를 실행하지 않고 각 근거·소스·빌드 기록만 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 이번 결과만 현재 소스/바이너리와 대조하고 과거 바이너리의 해시는 유지한다.
    for group in ('sources', 'evidence', 'binaries'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/datamanager-lifetime-native.json').read_text('utf-8'))
    assert native['uniqueInputs'] == native['normalFunctionReturns'] == 392 and native['instructions'] == 20740
    assert native['entryCounts'] == {'004bd330': 192, '0044da60': 384, '0044d040': 664}
    assert native['capturedBeforeProductEdits'] and not native['cppExecuted'] and not native['gameExecuted'] and not native['osExecuted']
    # 최초 관측과 도구는 제품 변경 전 별도 디렉토리에 동결한 바이트 그대로다.
    for name in ('tools/decomp_datamanager_lifetime.py', 'cpppj/tests/fixtures/datamanager-lifetime-x86.tsv', 'cpppj/datamanager-lifetime-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/datamanager-lifetime-20261021-first-capture' / name.split('/')[-1])
    rows = [line.split('\t') for line in (ROOT / 'cpppj/tests/fixtures/datamanager-lifetime-x86.tsv').read_text('utf-8').splitlines() if not line.startswith('#')]
    assert len(rows) == len({row[0] for row in rows}) == 392
    # 모든 정상 반환의 manager/다섯 노드/양쪽 보호 바이트와 free 직전 전체 상태를 확인한다.
    for row in rows:
        assert len(row) == 4 and len(bytes.fromhex(row[1])) == len(bytes.fromhex(row[2])) == 412
        before, after = bytes.fromhex(row[1]), bytes.fromhex(row[2])
        assert before[:8] == after[:8] and before[24:32] == after[24:32]
        for index in range(5):
            at = 32 + index * 76
            assert before[at:at + 8] == after[at:at + 8] and before[at + 68:at + 76] == after[at + 68:at + 76]
        if row[3] != '-':
            for event in row[3].split(';'):
                assert event[0] in 'BN' and len(bytes.fromhex(event.split(',')[2])) == 412
    source = (ROOT / 'cpppj/src/o/RawDataManagerLifetime.cpp').read_text('utf-8')
    assert source.index('Put(manager,4,Read(stream,0x38))') < source.index('DestroyMemStream(stream,hooks.releaseBuffer)') < source.index('hooks.releaseStream(node)') < source.index('head=Read(manager,4);', source.index('while (head)'))
    assert 'Put(stream,0,0);Put(stream,12,0);Put(stream,8,0);Put(stream,4,0)' in source
    assert 'Put(manager,8,' not in source and 'Put(stream,0x38,' not in source
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text('utf-8')
    assert 'o::DestroyDataManager(manager,hooks)' in storage and 'image.Destroy();image.fort=' in storage
    assert 'swap(fort.sections.at(index))' in storage and 'streams.erase(handle)' in storage
    assert storage.index('image.Destroy()') < storage.index('images_.erase(handle)')
    assert record['build']['returnCode'] == record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert record['lastDecompileHost'] == native['lastDecompileHost'] == 'VM-W11-CODEX'
    assert record['dataManagerDestructorConnected'] and not record['wholeRawDataManagerRestored'] and not record['wholeState7Restored']
    verify_pins(record)
    print(f"DataManager 최초392입력·소멸/실제 섹션 반납·Release·{len(record['sourcePinChanges'])}핀 역변환 보존 확인; 제품 실행 이월")


if __name__ == '__main__':
    main()
