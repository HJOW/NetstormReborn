#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""썬더 캐논의 원본 본체/준비 그림과 실제 전투 화면 출처를 감사한다."""
import argparse
import hashlib
import json
from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/thunder-display-runtime.json'  # 현재 연결과 과거 독립 원본 캡처를 구분한다.


def sha(path):
    """현재 전체 바이트만 해시하며 과거 실행 결과를 새 관측으로 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 저장 상태에서 적 캐논의 현재 그림 변화와 HP/좌표를 읽는다."""
    result=inspect_combat(directory)
    animation={}
    # 실전에서 실제 관측된 캐논만 모으며 클릭 시도를 성공으로 세지 않는다.
    for label,state in states(directory/'states.tsv').items():
        # 같은 실제 SID의 위치/프레임/HP를 원문 그대로 기록한다.
        for sid,row in state['objects'].items():
            if row[0]!='thunderCannon':
                continue
            record=animation.setdefault(str(sid),dict(frames=[],samples=[]))
            frame=int(row[7])
            if frame not in record['frames']:
                record['frames'].append(frame)
            record['samples'].append(dict(label=label,frame=frame,position=row[3:5],hp=int(row[12])))
    result['thunderAnimation']=animation
    return result


def main():
    """원본 픽셀·수정 전 실패·최종 전체 검사와 실제 화면 입력/결과를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 출처와 보존된 최초 관측을 각각 확인한다.
    for group in ('sources','evidence'):
        # 과거 해시와 실제 바이트의 연결만 검사한다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    native=json.loads((ROOT/'cpppj/thunder-cannon-capture-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'])==(333,666)
    before=json.loads((ROOT/record['validation']['beforeTests']).read_text('utf-8'))
    assert before['tests']==1 and before['returnCode']==1 and before['failed']==74
    focused=json.loads((ROOT/record['validation']['focusedTests']).read_text('utf-8'))
    full=json.loads((ROOT/record['validation']['fullTests']).read_text('utf-8'))
    assert focused['tests']==1 and focused['failed']==focused['returnCode']==0
    assert full['tests']==1748 and full['failed']==full['returnCode']==0 and full['binarySha256']==focused['binarySha256']
    # 실전 프레임 변화는 격리74화면의 원본 픽셀 일치와 별도의 관측이다.
    for name,value in record['observations'].items():
        assert inspect(ROOT/name)==value,name
        assert value['returnCode']==0 and value['originalsUnchanged'] and value['nextFailure'] is None
        assert len(value['thunderAnimation']['15006']['frames'])>=3
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('썬더 캐논74화면/462792픽셀·전체1748검사·실전 애니메이션 관측 출처 통과')


if __name__=='__main__':
    main()
