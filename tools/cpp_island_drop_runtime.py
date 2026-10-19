#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""섬 정리 원문·실제 월드 삭제 회귀·Early5 저장 관측을 실행 당시 경계로 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_sun_aviary_notice_runtime import inspect as inspect_combat
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/island-drop-runtime.json'  # 원본 기대값과 현재 연결 및 실패 이력을 구분하는 기록이다.


def sha(path):
    """검사 대상의 실제 바이트를 읽고 당시 바이너리 해시는 그대로 둔다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """게임을 실행하지 않고 당시 HP/표면/타워·자연 단계와 마지막 오류를 읽는다."""
    result=inspect_combat(directory)
    observed=states(directory/'states.tsv')
    towers={}
    # 타워 입력 시도와 실제 객체 생성/완공을 구별한다.
    for label,state in observed.items():
        # 동일 실제SID의 완공 전후HP를 같은 기록에 누적한다.
        for sid,row in state['objects'].items():
            if row[:2]!=['sunBlocker','1']:
                continue
            tower=towers.setdefault(str(sid),dict(xy=row[3:5],firstLabel=label,maxHp=int(row[12]),lastHp=int(row[12])))
            tower['maxHp']=max(tower['maxHp'],int(row[12]))
            tower['lastLabel']=label
            tower['lastHp']=int(row[12])
    result['towers']=towers
    return result


def main():
    """현재 소스·고정 원문·실패와 전체검사·수정 후 플레이를 각각 대조한다."""
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
    # 현재 파일 핀과 당시 결과가 일치해야 하며 검증 중 파일을 고치지 않는다.
    for group in ('sources','evidence'):
        # 현재 바이트만 대조하며 이전 실패 파일도 그대로 남겨야 한다.
        for name,value in report[group].items():
            assert sha(ROOT/name)==value,name
    # 당시 저장 객체의 관측을 재해석해 같은 결과인지 검사한다.
    for name,value in report['observations'].items():
        assert inspect(ROOT/name)==value,name
    validation=report['validation']
    full=json.loads((ROOT/validation['fullTests']).read_text('utf-8'))
    assert full['tests']==1740 and full['failed']==full['returnCode']==0
    # 전체 검사와 별도로 실제 삭제/특수 프레임 회귀의 성공을 확인한다.
    for name in validation['focusedTests']:
        test=json.loads((ROOT/name).read_text('utf-8'))
        assert test['tests'] and test['failed']==test['returnCode']==0,name
    packet=json.loads((ROOT/'cpppj/drop-island-packet-native.json').read_text('utf-8'))
    capture=json.loads((ROOT/'cpppj/support-capture-native.json').read_text('utf-8'))
    graph=json.loads((ROOT/'cpppj/island-graph-frame-native.json').read_text('utf-8'))
    assert (packet['inputs'],packet['segmentCompletions'],packet['normalReturns'])==(336,672,0)
    assert (capture['inputs'],capture['normalReturns'],capture['pixels'])==(162,324,328050)
    assert graph['cases']==graph['normalReturns']==24 and not graph['assertions']
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('섬 정리 원문336/162/24·실제 월드 삭제·전체1740검사와 Early5 저장 관측 출처 통과')


if __name__=='__main__':
    main()
