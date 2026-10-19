#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""작은 섬의 원본 색표/전체 픽셀과 실제 건설·간헐천 받침 점유의 출처를 감사한다."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/support-color-runtime.json'  # 원본 기대와 실제 입력 결과를 구분한다.


def sha(path):
    """현재 전체 바이트를 확인하며 과거 관측 해시는 재해석하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 같은 SID의 소유자/프레임 변화와 새 받침 생성을 분리해 읽는다."""
    result=inspect_combat(directory)
    observed=states(directory/'states.tsv')
    initial=next(iter(observed.values()))
    initial_ids=set(initial['objects'])  # 최초부터 존재한 받침과 건설로 생긴 받침을 나눈다.
    geyser_points={tuple(row[3:5]) for row in initial['objects'].values() if row[0]=='geyser'}
    supports={}  # 실제 받침과 하단 각각의 SID별 이력이다.
    # 조작 시도 대신 저장 필드의 실제 변화를 기록한다.
    for label,state in observed.items():
        # 윗면과 하단은 서로 다른 원본 색 규칙을 가지므로 별도로 보관한다.
        for sid,row in state['objects'].items():
            if row[0] not in ('island','islandStalag'):
                continue
            entry=supports.setdefault(str(sid),dict(kind=row[0],xy=row[3:5],initial=sid in initial_ids,
                geyserSupport=tuple(row[3:5]) in geyser_points,owners=[],frames=[],firstLabel=label,lastLabel=label,firstOwnedLabel=None))
            owner=int(row[1])
            frame=int(row[7])
            if owner not in entry['owners']:
                entry['owners'].append(owner)
            if frame not in entry['frames']:
                entry['frames'].append(frame)
            if owner and entry['firstOwnedLabel'] is None:
                entry['firstOwnedLabel']=label
            entry['lastLabel']=label
    result['supports']=supports
    result['claimedGeyserSupports']=[sid for sid,entry in supports.items() if entry['kind']=='island' and entry['initial'] and entry['geyserSupport'] and entry['owners'][0]==0 and 1 in entry['owners']]
    result['newOwnedSupports']=[sid for sid,entry in supports.items() if entry['kind']=='island' and not entry['initial'] and 1 in entry['owners']]
    final=next(reversed(observed.values()))  # 마지막 실제 논리 지면 소유자를 읽는다.
    result['supportSurfaceOwners']={}
    # 그림의 색 변화와 실제 아홉 지면 소유권을 각각 보관한다.
    for sid,entry in supports.items():
        if entry['kind']!='island':
            continue
        right,bottom=map(float,entry['xy'])
        counts=collections.Counter(row[1] for row in final['objects'].values() if row[0]=='noIsland' and right-2<=float(row[3])<=right and bottom-2<=float(row[4])<=bottom)
        result['supportSurfaceOwners'][sid]=dict(counts)
    return result


def main():
    """최초 실패와 수정 후 결과·원문·인수인계를 게임 실행 없이 확인한다."""
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
    # 현재 연결부와 보존 원문은 실제 파일 바이트와 같아야 한다.
    for group in ('sources','evidence'):
        # 과거 실행을 새 실행으로 바꾸지 않는다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    native=json.loads((ROOT/'cpppj/support-color-capture-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'],native['pixels'])==(198,396,716562)
    assert native['nativeCalls']=={'0x4daa70':216,'0x4daab0':180}
    # 동일한 최종 검사 소스의 수정 전 실패를 그대로 보존한다.
    for key,failed in (('beforePixels',162),('beforeOwnership',1)):
        previous=json.loads((ROOT/record['validation'][key]).read_text('utf-8'))
        assert previous['tests']==1 and previous['failed']==failed and previous['returnCode']==1
    full=json.loads((ROOT/record['validation']['fullTests']).read_text('utf-8'))
    assert full['tests']==1752 and full['failed']==full['returnCode']==0
    # 집중 검사와 전체 검사는 같은 실행 파일의 결과여야 한다.
    for name in record['validation']['focusedTests']:
        focused=json.loads((ROOT/name).read_text('utf-8'))
        assert focused['tests']==1 and focused['failed']==focused['returnCode']==0 and focused['binarySha256']==full['binarySha256']
    # 실제 건설과 중립 받침의 점유를 별도로 확인한다.
    for name,value in record['observations'].items():
        assert inspect(ROOT/name)==value,name
        assert value['returnCode']==0 and value['originalsUnchanged'] and value['nextFailure'] is None
    # 건설 뒤 잘못된 검사 키 이름으로 종료한 첫 실행은 정상 종료로 세지 않는다.
    for name,value in record['earlierObservations'].items():
        assert inspect(ROOT/name)==value,name
    earlier=record['earlierObservations']['extracted/goal-support-color-20261019-ui1']
    assert earlier['returnCode']==1 and earlier['originalsUnchanged'] and earlier['nextFailure']=='Unknown UI operation: keydown'
    assert earlier['newOwnedSupports']
    current=record['observations']['extracted/goal-support-color-20261019-ui2']
    assert current['claimedGeyserSupports']==['27']
    assert current['supports']['27']['owners']==[0,1] and current['supports']['27']['frames']==[8,0]
    assert current['supports']['27']['firstOwnedLabel']=='geyserBridge5'
    assert current['supports']['28']['owners']==[0] and current['supports']['28']['frames']==[8,0]
    assert current['supportSurfaceOwners']['27']=={'1':9}
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('작은 섬198원문·실제 받침 소유권/표시·전체1752검사 출처 통과')


if __name__=='__main__':
    main()
