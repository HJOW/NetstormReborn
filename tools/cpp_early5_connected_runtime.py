#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 적 섬 연결·연속 골렘 이동·오벨리스크 획득과 자연F의 출처를 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/early5-connected-runtime.json'  # 실제 자연 진행과 미완료 주문 시전을 구분한다.


def sha(path):
    """전체 바이트의 해시이며 과거 실행 파일 해시는 다시 계산해 덮어쓰지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 이동 시작·첫 F 관측·원래 오벨리스크 삭제를 같은 보고에서 읽는다."""
    result=inspect_combat(directory)
    observed=states(directory/'states.tsv')
    first=next(iter(observed.values()))
    final=next(reversed(observed.values()))
    result['firstMovement']=None
    result['firstF']=None
    initial_points={}  # 같은 SID가 처음 생긴 실제 위치다.
    # 클릭 이름이나 다리 수를 성공으로 세지 않고 실제 객체와 단계의 변화만 읽는다.
    for label,state in observed.items():
        if result['firstF'] is None and state.get('mission_gate',[])[:2]==['5','70']:
            result['firstF']=dict(label=label,frame=state['frame'],gate=state['mission_gate'],page=state.get('page',[]))
        # 저장된 모든 실제 소수 좌표를 보존하는 부모 관측과 이동 시작 경계를 연결한다.
        for sid,row in state['objects'].items():
            if row[:2]!=['sunWalker','1']:
                continue
            point=row[5:7]  # 보고 열3/4의 정수 칸과 구분한 실제 소수 월드 좌표다.
            origin=initial_points.setdefault(sid,point)
            if result['firstMovement'] is None and point!=origin:
                result['firstMovement']=dict(sid=sid,label=label,frame=state['frame'],source=origin,position=point,game=state['raw_world'][0])
    result['originalObeliskPresentInitially']=first['objects'].get(15011,[])[:1]==['buried']
    result['originalObeliskRemoved']=15011 not in final['objects']
    result['secondObeliskRemains']=final['objects'].get(15012,[])[:1]==['buried']
    return result


def main():
    """전체 완주를 주장하지 않고 실제 자연F까지의 입력·결과를 다시 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--inspect',type=Path)
    options=parser.parse_args()
    if options.inspect:
        print(json.dumps(inspect(options.inspect.resolve()),ensure_ascii=False,indent=2))
        return
    if not options.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 실제 실행 원문을 별도로 확인한다.
    for group in ('sources','evidence'):
        # 보존된 이전 실행을 새 실행으로 취급하지 않는다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    # 재계산은 저장된 실제 입력과 상태만 읽으며 게임을 새로 실행하지 않는다.
    for name,value in record['observations'].items():
        assert inspect(ROOT/name)==value,name
        assert value['returnCode']==0 and value['originalsUnchanged'] and value['nextFailure'] is None
        assert value['naturalEObserved'] and value['naturalFObserved'] and value['maximumOwnGolemCount']==1
        assert value['originalObeliskPresentInitially'] and value['originalObeliskRemoved'] and value['secondObeliskRemains']
        assert value['firstMovement'] and value['firstF']['page']==['F.'] and value['lastPage']==['F.']
        golem=next(iter(value['golems'].values()))
        assert golem['positions'][0]==['130','82'] and golem['positions'][-1]==['143','132'] and len(golem['positions'])>=20
        assert golem['lastHp']==50
    previous=json.loads((ROOT/record['validation']['previousFullTests']).read_text('utf-8'))
    assert previous['tests']==1752 and previous['failed']==previous['returnCode']==0 and record['validation']['productUnchanged']
    header=json.loads((ROOT/record['pendingCastStaticHeader']).read_text('utf-8'))
    assert header['bytes']=='50010900' and (header['message'],header['protocol'],header['length'])==(80,1,9)
    assert not header['runtimeExecution'] and not record['castAttemptObserved']
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('Early5 실제 골렘 보행·오벨리스크 획득·자연F 출처 통과')


if __name__=='__main__':
    main()
