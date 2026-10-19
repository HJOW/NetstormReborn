#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""골렘 실제 배치와 기존 전투 뒤 단계 관측의 출처를 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_sun_aviary_capture_runtime import inspect as inspect_combat

REPORT=ROOT/'cpppj/early5-spells-runtime.json'  # 실제 성공과 조작 시도를 구별하는 기록이다.


def sha(path):
    """현재 파일 전체 바이트를 읽으며 기존 실행 해시를 재해석하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """같은 원문에서 관측을 다시 읽고 전체 목표의 미완료 상태를 유지한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--inspect',type=Path)
    options=parser.parse_args()
    if options.inspect:
        print(json.dumps(inspect_combat(options.inspect.resolve()),ensure_ascii=False))
        return
    if not options.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 출처만 대조하며 과거 관측을 새 실행으로 세지 않는다.
    for group in ('sources','evidence'):
        # 원본/입력/결과의 전체 현재 바이트를 비교한다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    # 각각 독립 실행의 종료 코드와 실제 객체/단계를 다시 읽는다.
    for name,value in record['observations'].items():
        assert inspect_combat(ROOT/name)==value,name
    first=record['observations']['extracted/goal-early5-spells-20261019-ui1']
    assert first['returnCode']==0 and first['originalsUnchanged'] and first['maximumOwnGolemCount']==1
    assert not first['naturalEObserved'] and not first['naturalFObserved']
    assert len(first['golems'])==1 and next(iter(first['golems'].values()))['positions']==[['130','83']]
    second=record['observations']['extracted/goal-early5-spells-20261019-ui2']
    assert second['returnCode']==0 and second['originalsUnchanged'] and second['naturalEObserved']
    assert second['maximumOwnCannonCount']==2 and second['maximumOwnGolemCount']==1 and not second['naturalFObserved']
    assert next(iter(second['golems'].values()))['positions']==[['130','82']]
    third=record['observations']['extracted/goal-early5-spells-20261019-ui3']
    assert third['returnCode']==0 and third['originalsUnchanged'] and third['naturalEObserved']
    assert third['maximumOwnGolemCount']==1 and not third['naturalFObserved']
    assert next(iter(third['golems'].values()))['positions']==[['130','82']]
    previous=json.loads((ROOT/record['validation']['previousFullTests']).read_text('utf-8'))
    assert previous['tests']==1746 and previous['failed']==previous['returnCode']==0
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('골렘 실제 배치와 전투 후 단계 관측 출처 통과')


if __name__=='__main__':
    main()
