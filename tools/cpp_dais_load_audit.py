#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""저장 다이스의 최초 Pop 연결·빌드·기존 근거 보존을 게임 실행 없이 확인한다."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]  # 저장소와 보존한 보고서의 기준 경로다.
REPORT = ROOT / 'cpppj/dais-load-session.json'  # 이번 연결·빌드와 실행 검증 이월을 구분하는 기록이다.


def sha(path):
    """지정 파일의 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def at(document, pointer):
    """JSON의 실제 키/배열 위치를 따라 현재 값을 읽는다."""
    # 기록에 보존한 경로만 순서대로 방문한다.
    for key in pointer:
        document = document[key]
    return document


def main():
    """현재 소스와 빌드·핀 역변환을 확인하며 게임/검사 실행 파일은 시작하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 새 제품과 원본·기존 fixture를 각자의 실제 바이트로 검사한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    source = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    load = source.split('void RawCampaignSession::Load() {', 1)[1].split('\n}', 1)[0]
    assert load.index('ConnectBuilding()') < load.index('ConnectMotion()') < load.index('runtime->Load()')
    assert 'if (daisState) PrepareRitual();' in source
    assert 'core.postPop.SetDaisPostPop(nullptr);' in source
    build = record['build']
    assert build['returnCode'] == 0
    assert sha(ROOT / build['log']) == build['logSha256']
    # 과거 원본/검사 결과는 새 통합 성공으로 갱신하지 않는다.
    assert record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted']
    assert not record['goalComplete']
    changes = record['sourcePinChanges']
    # 이번 새 핀만 되돌리면 시작 커밋의 보고서 내용 전체와 같아야 한다.
    for name in sorted({item['report'] for item in changes}):
        report = json.loads((ROOT / name).read_text('utf-8'))
        for item in changes:
            if item['report'] != name:
                continue
            assert at(report, item['pointer']) == item['new'] == sha(ROOT / item['source'])
            parent = at(report, item['pointer'][:-1])
            parent[item['pointer'][-1]] = item['old']
        update = report['integration_updates'].pop()
        assert update['unit'] == record['unit']
        baseline_bytes = subprocess.check_output(['git', 'show', f"{record['baseline']}:{name}"], cwd=ROOT)
        baseline = json.loads(baseline_bytes)
        if 'integration_updates' not in baseline and not report['integration_updates']:
            del report['integration_updates']
        assert report == baseline, name
    print(f"저장 다이스: 초기 등록 순서·Release 빌드·{len(changes)}개 현재 핀 보존 확인, 실행 검증 이월")


if __name__ == '__main__':
    main()
