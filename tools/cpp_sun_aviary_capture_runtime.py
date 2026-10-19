#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리베이스 캡처 원문·실제 자연E와 골렘 후속 조작·전체 검사 출처를 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_thunder_capture_runtime import inspect as inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/sun-aviary-capture-runtime.json'  # 현재 연결과 각 실행 당시의 결과를 구별하는 기록이다.


def sha(path):
    """현재 파일의 전체 바이트를 읽고 과거 실행 파일 해시는 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """저장 상태의 자연 단계와 실제 골렘 위치를 읽고 조작 시도를 생성 성공으로 세지 않는다."""
    result=inspect_combat(directory)
    observed=states(directory/'states.tsv')
    result['naturalFObserved']=False
    result['maximumOwnGolemCount']=0
    golems={}
    # 단계 변경과 실제 객체 생성은 원본 보고서에서 각각 확인한다.
    for label,state in observed.items():
        result['naturalFObserved']|=state.get('mission_gate',[])[:2]==['5','70']
        count=0
        # 같은 실제 SID의 위치와 HP를 기록하며 화면 애니메이션 품질의 증거로 확대하지 않는다.
        for sid,row in state['objects'].items():
            if row[:2]!=['sunWalker','1']:
                continue
            count+=1
            record=golems.setdefault(str(sid),dict(firstLabel=label,lastLabel=label,positions=[],lastHp=int(row[12])))
            point=row[5:7]
            if point not in record['positions']:
                record['positions'].append(point)
            record['lastLabel']=label
            record['lastHp']=int(row[12])
        result['maximumOwnGolemCount']=max(result['maximumOwnGolemCount'],count)
    result['golems']=golems
    return result


def main():
    """원본 기대값·현재 코드·단계/객체 관측과 검사 결과를 게임 없이 검증한다."""
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
    # 현재 파일 핀은 실제 바이트와 같아야 하며 과거 이력을 다시 해석하지 않는다.
    for group in ('sources','evidence'):
        # 소스와 증거의 모든 현재 항목을 각각 검사한다.
        for name,value in report[group].items():
            assert sha(ROOT/name)==value,name
    # 각 실행의 종료 오류와 마지막 관측 상태를 저장 파일에서 다시 읽는다.
    for name,value in report['observations'].items():
        assert inspect(ROOT/name)==value,name
    full=json.loads((ROOT/report['validation']['fullTests']).read_text('utf-8'))
    focused=json.loads((ROOT/report['validation']['focusedTests']).read_text('utf-8'))
    assert full['tests']==1744 and full['failed']==full['returnCode']==0
    assert focused['tests']==9 and focused['failed']==focused['returnCode']==0
    assert full['binarySha256']==focused['binarySha256']
    native=json.loads((ROOT/'cpppj/sun-aviary-capture-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'],native['pixels'])==(384,768,3449976)
    assert native['virtualCalls']=={'0x45b530':576,'0x45b560':576}
    before=report['observations']['extracted/goal-thunder-capture-20261019-ui3']
    assert before['returnCode']==1 and 'TYPE72' in before['nextFailure'] and not before['naturalEObserved']
    first=report['observations']['extracted/goal-aviary-capture-20261019-ui1']
    assert first['naturalEObserved'] and not first['enemyThunderPresentAtLastReport'] and not first['enemyAviaryPresentAtLastReport']
    assert first['nextFailure']=='UI script ended before all steps ran'
    second=report['observations']['extracted/goal-aviary-capture-20261019-ui2']
    assert second['returnCode']==0 and second['originalsUnchanged'] and second['naturalEObserved']
    assert not second['enemyThunderPresentAtLastReport'] and not second['enemyAviaryPresentAtLastReport']
    assert not second['naturalFObserved'] and second['maximumOwnGolemCount']==0
    closed=json.loads((ROOT/'extracted/goal-aviary-capture-20261019-ui1-close.json').read_text('utf-8'))
    assert closed['requestedClose']
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('훨리베이스384원문·실제 적두건물 소멸/자연E·전체1744검사 출처 통과')


if __name__=='__main__':
    main()
