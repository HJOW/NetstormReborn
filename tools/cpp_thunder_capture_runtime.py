#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 썬더 캡처·최초 실패 대상·수정후 실제 전투와 전체 검사 출처를 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_island_drop_runtime import inspect as inspect_saved
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/thunder-cannon-capture-runtime.json'  # 현재 연결과 당시 실제 결과를 함께 보존한다.


def sha(path):
    """현재 파일 전체 바이트를 고정하며 과거 바이너리 해시는 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """적HP와건설뿐아니라신전파괴안내를구별해프로세스정상종료를승리로세지않는다."""
    result=inspect_saved(directory)
    observed=states(directory/'states.tsv')
    result['templeDestroyedObserved']=False
    result['maximumOwnCannonCount']=0
    # 같은 저장 상태의 실제 미션 페이지와 살아 있는 캐논 행만 읽는다.
    for state in observed.values():
        result['templeDestroyedObserved']|=state.get('page')==['VortexDestroyed']
        result['maximumOwnCannonCount']=max(result['maximumOwnCannonCount'],sum(row[:2]==['sunCannon','1'] for row in state['objects'].values()))
    last=next(reversed(observed.values()))
    result['lastPage']=last.get('page')
    result['enemyThunderPresentAtLastReport']=15006 in last['objects']
    result['enemyAviaryPresentAtLastReport']=15013 in last['objects']
    return result


def main():
    """게임을 열지 않고 원본 근거·실제 관측·현재 소스와 검사 기록을 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--inspect',type=Path)
    options=parser.parse_args()
    if options.inspect:
        print(json.dumps(inspect(options.inspect.resolve()),ensure_ascii=False))
        return
    if not options.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    report=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 원본 관측의 근거를 과거 이력과 구별하여 검증한다.
    for group in ('sources','evidence'):
        # 모든 현재 파일은 저장된 실제 바이트와 같아야 한다.
        for name,value in report[group].items():
            assert sha(ROOT/name)==value,name
    # 당시 실행을 저장 상태에서 다시 해석하며 재실행으로 세지 않는다.
    for name,value in report['observations'].items():
        assert inspect(ROOT/name)==value,name
    validation=report['validation']
    full=json.loads((ROOT/validation['fullTests']).read_text('utf-8'))
    focused=json.loads((ROOT/validation['focusedTests']).read_text('utf-8'))
    assert full['tests']==1742 and full['failed']==full['returnCode']==0
    assert focused['tests']==3 and focused['failed']==focused['returnCode']==0
    assert full['binarySha256']==focused['binarySha256']
    native=json.loads((ROOT/'cpppj/thunder-cannon-capture-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'],native['pixels'])==(333,666,4165128)
    assert native['virtualCalls']=={'0x46ae10':444,'0x46ae30':444}
    before=report['observations']['extracted/goal-thunder-capture-20261019-ui1']
    assert before['returnCode']==1 and 'SID=15006 TYPE=131 VT=5290704' in before['nextFailure']
    lost=report['observations']['extracted/goal-thunder-capture-20261019-ui2']
    assert lost['returnCode']==0 and lost['templeDestroyedObserved'] and not lost['naturalEObserved']
    after=report['observations']['extracted/goal-thunder-capture-20261019-ui3']
    assert after['enemyMinimumHp']['15006']<500
    assert not after['nextFailure'] or 'Factory 그림의 실제 가상 표/TYPE 미복원' not in after['nextFailure']
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('썬더 캡처333원문·실제131실패 진단/후속전투·전체1742검사 출처 통과')


if __name__=='__main__':
    main()
