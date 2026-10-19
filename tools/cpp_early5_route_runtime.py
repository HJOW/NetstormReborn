#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early5 저장 지도의 느슨한 연결 상한과 실제 선택/이동 관측의 출처를 감사한다."""
import argparse
import hashlib
import heapq
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/early5-route-runtime.json'  # 실제 진행과 미완료 경계를 함께 보관한다.


def coarse_route(state,start,target=(143,132)):
    """객체 좌표만으로 최소 빈칸 수를 구한다. 방향·점유·원본 Graph를 대체하지 않는다."""
    cells={(int(float(row[3])),int(float(row[4]))) for row in state['objects'].values() if row[0] in {'isle','noIsland','bridge'}}
    distances={start:(0,0)}  # 빈칸 수를 우선하고 같은 수이면 총 길이를 줄인다.
    parents={}  # 결과 경로의 이전 칸을 보관한다.
    queue=[(0,0,start)]  # 정렬 기준과 현재 좌표를 가진 최소 힙이다.
    # 지도 내 빈칸도 비용1로 허용해 실제 통행보다 느슨한 상한을 구한다.
    while queue:
        water,length,point=heapq.heappop(queue)
        if distances[point]!=(water,length):
            continue
        if point==target:
            break
        # 네 방향 이웃을 순서대로 비교한다.
        for neighbor in ((point[0]-1,point[1]),(point[0]+1,point[1]),(point[0],point[1]-1),(point[0],point[1]+1)):
            if not all(0<=value<256 for value in neighbor):
                continue
            cost=(water+(neighbor not in cells),length+1)
            if cost<distances.get(neighbor,(65537,65537)):
                distances[neighbor]=cost
                parents[neighbor]=point
                heapq.heappush(queue,(*cost,neighbor))
    path=[target]  # 목적지에서 출발지까지 역순으로 읽는다.
    # 첫 칸에 도달할 때까지 부모를 따라간다.
    while path[-1]!=start:
        path.append(parents[path[-1]])
    path.reverse()
    return dict(occupiedCells=len(cells),minimumMissingCells=distances[target][0],length=distances[target][1],missing=[list(point) for point in path if point not in cells],path=[list(point) for point in path],nativePathProof=False)


def inspect(directory):
    """같은 골렘의 실제 선택 전/후와 이동 전/후를 저장 상태에서 분리한다."""
    result=inspect_combat(directory)
    observed=states(directory/'states.tsv')
    samples={}  # 입력 경계에서 선택 번호와 같은 SID의 실제 좌표를 보관한다.
    final=None
    # 조작 이름을 성공으로 해석하지 않고 실제 필드만 읽는다.
    for label,state in observed.items():
        final=state
        if not label.startswith(('beforeSelect','afterSelect','afterMove','obeliskWalk')):
            continue
        samples[label]=dict(frame=state['frame'],selected=state.get('selected',[]),rawWorld=state.get('raw_world',[]),
            golems={str(sid):row[3:8] for sid,row in state['objects'].items() if row[:2]==['sunWalker','1']})
    result['selectionSamples']=samples
    result['finalCoarseRoutes']={}
    if final:
        # 출발점은 최종 관측의 실제 골렘 칸이며 위치를 바꾸지 않는다.
        for sid,row in final['objects'].items():
            if row[:2]==['sunWalker','1']:
                result['finalCoarseRoutes'][str(sid)]=coarse_route(final,(int(row[5]),int(row[6])))
    return result


def sha(path):
    """현재 전체 바이트를 해시하고 과거 실행 파일 값을 덮어쓰지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """보존 원문과 재계산 결과를 대조하며 전체 플레이 완료를 주장하지 않는다."""
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
    # 현재 코드와 보존한 실행 자료를 각각 확인한다.
    for group in ('sources','evidence'):
        # 실제 전체 바이트와 기록한 핀을 비교한다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    previous=states(ROOT/record['priorState'])['final']
    assert coarse_route(previous,(130,82))==record['priorCoarseRoute']
    assert record['priorCoarseRoute']['minimumMissingCells']==1
    assert record['priorCoarseRoute']['missing']==[[141,114]]
    # 실제 실행 결과는 단계·객체·종료를 각각 다시 읽는다.
    for name,value in record['observations'].items():
        assert inspect(ROOT/name)==value,name
        assert value['originalsUnchanged'],name
    current=record['observations']['extracted/goal-early5-route-20261019-ui1']
    assert current['returnCode']==0 and current['nextFailure'] is None and current['naturalEObserved']
    assert not current['naturalFObserved'] and current['maximumOwnGolemCount']==1
    assert current['golems']['15153']['positions']==[['130','82']]
    # 실제 선택 성공과 그 뒤의 이동 실패를 구분하는 열두 입력 경계다.
    for number in range(1,13):
        before=current['selectionSamples'][f'beforeSelect{number}']
        selected=current['selectionSamples'][f'afterSelect{number}']
        moved=current['selectionSamples'][f'afterMove{number}']
        assert before['selected']==['0'] and selected['selected']==['15153'] and moved['selected']==['0']
        assert before['golems']==selected['golems']==moved['golems']
    assert current['finalCoarseRoutes']['15153']['minimumMissingCells']==1
    assert current['finalCoarseRoutes']['15153']['missing']==[[135,114]]
    previous_tests=json.loads((ROOT/record['validation']['previousFullTests']).read_text('utf-8'))
    assert previous_tests['tests']==1750 and previous_tests['failed']==previous_tests['returnCode']==0
    assert record['validation']['productUnchanged']
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('Early5 다리 연결 상한과 실제 선택/이동 관측 출처 통과')


if __name__=='__main__':
    main()
