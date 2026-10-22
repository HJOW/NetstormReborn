#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TEST02 원본 실행의 저장된 메모리·작업본·종료 기록을 대조한다. 게임을 실행하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct

import fort

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 저장 관찰을 읽는 저장소 루트다.
WORK = ROOT / 'extracted/test02-live-20261022'  # 이미 종료한 원본 세션의 원문과 조작 기록이다.
OPTIONS_WORK = ROOT / 'extracted/test02-options-20261022'  # 별도 재실행에서 현재 옵션과 TYPE를 읽은 원문이다.
REPORT = ROOT / 'cpppj/test02-runtime-20261022.json'  # 원문 SHA와 전체 관찰 슬롯을 보존하는 보고서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/test02-runtime/d/TEST02.fort'  # 원본 편집기가 저장한 독립 작업본이다.
TABLE = ROOT / 'cpppj/tests/fixtures/test02-original-runtime.tsv'  # 원본 14자산의 선택 필드를 보존한다.


def digest(path):
    """인코딩이나 줄바꿈을 바꾸지 않고 실제 파일의 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect():
    """저장 풀과 JSON을 독립적으로 대조하고 관찰 입력·종료·보호 파일의 출처를 보존한다."""
    scene = json.loads((WORK / 'scene.json').read_text(encoding='utf-8'))
    pool = (WORK / 'pool.bin').read_bytes()
    assert len(pool) == 23000 * 50
    actions = [json.loads(line) for line in (WORK / 'actions.jsonl').read_text(encoding='utf-8').splitlines()]
    first, last = actions[0], actions[-1]
    assert first['op'] == 'start' and first['sessionId'] == scene['session']
    assert last['op'] == 'end' and last['closed'] and not last['isError']
    assert last['automaticRecording']['state'] == 'stopped' and last['automaticRecording']['error'] is None
    session_root = ROOT / 'extracted/analyzeManager' / scene['session']
    session = json.loads((session_root / 'session.json').read_text(encoding='utf-8'))
    assert session['processId'] == scene['pid']
    assert FIXTURE.read_bytes() == (session_root / 'game/d/TEST02.fort').read_bytes()
    assert digest(session_root / 'game/Netstorm.exe') == digest(ROOT / 'originals/Netstorm.exe')
    assert digest(session_root / 'game/d/TEST02.english') == digest(ROOT / 'originals/d/TEST02.english')
    expected = []  # 원본 풀의 실제 슬롯에서만 TSV 선택 필드를 다시 읽는다.
    # 자산 목록 전체를 원문 50바이트 및 실제 내용물 링크와 대조한다.
    for entry in scene['objects']:
        sid = entry['sid']
        raw = pool[sid * 50:(sid + 1) * 50]
        assert raw.hex() == entry['raw'] and not raw[11] & 3
        values = [sid, raw[10], raw[34], struct.unpack_from('<H', raw, 12)[0],
                  *struct.unpack_from('<II', raw, 14), struct.unpack_from('<I', raw, 26)[0]]
        assert values[1:4] == [entry['type'], entry['owner'], entry['qa']]
        assert struct.unpack_from('<ff', raw, 14) == (entry['x'], entry['y']) and values[-1] == entry['hp']
        expected.append(values)
        child = struct.unpack_from('<H', raw, 6)[0]
        # 일반 Form과 ContentForm을 같은 의미로 해석하지 않고 링크·원문만 비교한다.
        for saved in entry['children']:
            assert child == saved['sid'] and 0 < child < 23000
            body = pool[child * 50:(child + 1) * 50]
            assert body.hex() == saved['raw']
            child = struct.unpack_from('<H', body, 4)[0]
        assert child == 0
    assert len(expected) == 14 and sum(row[1] == 135 for row in expected) == 1
    rows = [list(map(int, line.split())) for line in TABLE.read_text(encoding='utf-8').splitlines()
            if line and not line.startswith('#')]
    assert rows == expected
    assert not next(entry for entry in scene['objects'] if entry['type'] == 135)['children']
    option_scene = json.loads((OPTIONS_WORK / 'scene.json').read_text(encoding='utf-8'))
    option_actions = [json.loads(line) for line in (OPTIONS_WORK / 'actions.jsonl').read_text(encoding='utf-8').splitlines()]
    option_session_root = ROOT / 'extracted/analyzeManager' / option_scene['session']
    option_session = json.loads((option_session_root / 'session.json').read_text(encoding='utf-8'))
    assert option_actions[0]['sessionId'] == option_scene['session'] and option_session['processId'] == option_scene['pid']
    assert option_actions[-1]['op'] == 'end' and option_actions[-1]['closed'] and not option_actions[-1]['isError']
    assert option_actions[-1]['automaticRecording']['state'] == 'stopped'
    options = bytes.fromhex(option_scene['options'])
    type122 = bytes.fromhex(option_scene['type122'])
    assert len(options) == 87 and len(type122) == 500
    assert options[16] == 2 and struct.unpack_from('<f', type122, 0xc4)[0] == 3000.0
    assert option_scene['modes'] == {'0x594fb8': 0, '0x594fbc': 1, '0x594fa4': 1, '0x5c85a4': 0, '0x540c70': 1}
    assert digest(option_session_root / 'game/Netstorm.exe') == digest(ROOT / 'originals/Netstorm.exe')
    assert digest(option_session_root / 'game/netstorm.tarc') == digest(ROOT / 'originals/netstorm.tarc')
    assert digest(option_session_root / 'game/d/TEST02.english') == digest(ROOT / 'originals/d/TEST02.english')
    original, end = fort.split_sections((ROOT / 'originals/d/TEST02.fort').read_bytes())
    edited, edited_end = fort.split_sections(FIXTURE.read_bytes())
    assert end == edited_end == 2150
    changed = [fort.SECTION_NAMES[index] for index in range(len(original)) if original[index] != edited[index]]
    files = [ROOT / 'originals/Netstorm.exe', ROOT / 'originalCD/NETSTORM.EXE',
             ROOT / 'originals/d/TEST02.fort', ROOT / 'originals/d/TEST02.english', FIXTURE, TABLE,
             WORK / 'read_scene.py', WORK / 'scene.json', WORK / 'pool.bin', WORK / 'actions.jsonl',
             OPTIONS_WORK / 'read_scene.py', OPTIONS_WORK / 'scene.json', OPTIONS_WORK / 'pool.bin',
             OPTIONS_WORK / 'actions.jsonl', option_session_root / 'session.json',
             session_root / 'session.json', ROOT / 'tools/analyze_test02_runtime.py',
             ROOT / 'cpppj/tests/CampaignSessionTests.cpp', ROOT / 'cpppj/src/o/RawAssetPostPop.h',
             ROOT / 'cpppj/src/o/RawAssetPostPop.cpp', ROOT / 'originals/netstorm.tarc',
             ROOT / 'extracted/refined/originals/Netstorm.c']
    return dict(host=socket.gethostname(), lastFullDecompileHost='VM-W11-Codex',
                session=scene['session'], pid=scene['pid'], poolAddress=scene['poolAddress'],
                started=first['started'], ended=last['finished'], frames=last['automaticRecording']['frames'],
                closed=last['closed'], editorChangedSections=changed, objects=scene['objects'],
                executedReaderSource=(WORK / 'read_scene.py').read_text(encoding='utf-8'),
                optionsObservation=option_scene, optionsClosed=option_actions[-1]['closed'],
                executedOptionsReaderSource=(OPTIONS_WORK / 'read_scene.py').read_text(encoding='utf-8'),
                hashes={str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files})


def main():
    """저장 증거의 보고서 생성 또는 동일성 검사만 선택하며 실제 게임은 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    evidence = inspect()
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        assert saved['evidence'] == evidence
        # 뒤에서 추가한 빌드·검사 로그도 원문 그대로 확인한다.
        for path, sha in saved.get('validationHashes', {}).items():
            assert digest(ROOT / path) == sha, path
    else:
        REPORT.write_bytes((json.dumps(dict(evidence=evidence), ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print(json.dumps(dict(session=evidence['session'], objects=len(evidence['objects']), closed=evidence['closed'])))


if __name__ == '__main__':
    main()
