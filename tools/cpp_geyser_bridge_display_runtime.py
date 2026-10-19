#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""간헐천 외형과 다리 기준점/소유자 색의 원문·화면·실제 입력 출처를 감사한다."""
import argparse
import collections
import hashlib
import json
from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/geyser-bridge-display-runtime.json'  # 현재 연결과 독립 원본 캡처/과거 관측을 구분한다.


def sha(path):
    """현재 전체 바이트만 해시하며 과거 실행 파일의 해시를 새값으로 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 저장 상태에서 간헐천 프레임/선택과 다리 배치 결과를 읽는다."""
    result=inspect_combat(directory)
    geysers={}
    result['selectedGeyserObserved']=False
    result['bridgeEdgeLabels']=[]
    final=None
    # 조작 시도와 실제 상태 변경을 구분한다.
    for label,state in states(directory/'states.tsv').items():
        final=state
        if label.startswith('bridgeEdge'):
            result['bridgeEdgeLabels'].append(label)
        selected=state.get('selected',[])
        # 같은 실제 SID의 프레임/좌표를 각각 기록한다.
        for sid,row in state['objects'].items():
            if row[0]!='geyser':
                continue
            record=geysers.setdefault(str(sid),dict(frames=[],positions=[]))
            frame=int(row[7])
            if frame not in record['frames']:
                record['frames'].append(frame)
            if row[3:5] not in record['positions']:
                record['positions'].append(row[3:5])
            result['selectedGeyserObserved']|=selected==[str(sid)]
    result['geysers']=geysers
    bridge_groups=collections.defaultdict(list)
    if final:
        # 다리 본체의 실제 중복만 검사하며 연결 조각의 원본 생성을 임의로 제거하지 않는다.
        for sid,row in final['objects'].items():
            if row[0]=='bridge':
                bridge_groups[tuple(row[3:5])].append(sid)
    result['finalBridgeCount']=sum(len(group) for group in bridge_groups.values())
    result['duplicateBridgeGroups']=[values for values in bridge_groups.values() if len(values)>1]
    return result


def main():
    """독립 캡처·수정 전 실패·최종 전체 검사·실제 화면 원문을 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 처음부터 보존한 각 증거를 바이트 단위로 확인한다.
    for group in ('sources','evidence'):
        # 과거 관측을 새 게임 실행으로 바꾸지 않는다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    native=json.loads((ROOT/'cpppj/geyser-capture-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'],native['pixels'])==(459,918,2531340)
    assert native['virtualCalls']=={'0x4acfe0':612,'0x485480':612}
    # 두 결함의 수정 전 실패를 각각 보존한다.
    for key,failed in (('geyserBefore',102),('bridgeBefore',1200)):
        before=json.loads((ROOT/record['validation'][key]).read_text('utf-8'))
        assert before['tests']==1 and before['returnCode']==1 and before['failed']==failed
    full=json.loads((ROOT/record['validation']['fullTests']).read_text('utf-8'))
    assert full['tests']==1750 and full['failed']==full['returnCode']==0
    # 최종 두 집중 검사도 같은 검사 바이너리여야 한다.
    for name in record['validation']['focusedTests']:
        focused=json.loads((ROOT/name).read_text('utf-8'))
        assert focused['tests']==1 and focused['failed']==focused['returnCode']==0 and focused['binarySha256']==full['binarySha256']
    # 실제 UI 진행의 선택/프레임/다리 배치를 저장 상태에서 다시 읽는다.
    for name,value in record['observations'].items():
        assert inspect(ROOT/name)==value,name
        assert value['returnCode']==0 and value['originalsUnchanged'] and value['nextFailure'] is None
        assert value['selectedGeyserObserved'] and value['finalBridgeCount']>0 and not value['duplicateBridgeGroups']
        assert len(value['bridgeEdgeLabels'])==4 and any(len(entry['frames'])>=3 for entry in value['geysers'].values())
    # 최초 입력 오류와 실제 선택 성공의 짧은 실행을 최종 건설 실행과 구분한다.
    for name,value in record['earlierObservations'].items():
        assert inspect(ROOT/name)==value,name
    failed=record['earlierObservations']['extracted/goal-geyser-bridge-display-20261019-ui1']
    assert failed['returnCode']==1 and failed['originalsUnchanged'] and failed['nextFailure']=='UI control not found: world:geyser'
    selected=record['earlierObservations']['extracted/goal-geyser-bridge-display-20261019-ui-select2']
    assert selected['returnCode']==0 and selected['originalsUnchanged'] and selected['selectedGeyserObserved']
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('간헐천51프레임/다리600원문·전체1750검사·실제 선택/건설 화면 출처 통과')


if __name__=='__main__':
    main()
