#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""골렘 표시 수정의 원본 픽셀 회귀와 실제 다리 왕복 보행 자료를 읽기 전용 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT
from cpp_world_smoke import states

REPORT=ROOT/'cpppj/golem-display-runtime.json' # 현재 소스와 과거 미션 실행의 경계를 보존하는 단위 보고서다.


def sha(path):
    """전체 파일 바이트의 SHA256이며 원본 관측을 재생성하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """이름만 붙인 이동 입력 대신 실제 소수 월드 좌표와 골렘 생존을 읽는다."""
    run=json.loads((directory/'run.json').read_text('utf-8'))
    observed=states(directory/'states.tsv')
    golems={} # 실제 SID별 첫 위치/모든 위치/마지막 체력이다.
    # 각 보고 지점의 객체 열5/6은 정수 칸 열3/4와 다른 실제 소수 좌표다.
    for label,state in observed.items():
        # 현재 실제 소유 골렘만 모으며 중립 오벨리스크 등을 골렘으로 세지 않는다.
        for sid,row in state['objects'].items():
            if row[:2]!=['sunWalker','1']:
                continue
            item=golems.setdefault(str(sid),dict(positions=[],north=[],south=[]))
            point=row[5:7]
            if point not in item['positions']:
                item['positions'].append(point)
            item['lastHp']=float(row[12])
            phase='north' if label.startswith('north') else 'south' if label.startswith('south') else None
            if phase and point not in item[phase]:
                item[phase].append(point)
    return dict(returnCode=run['returnCode'],originalsUnchanged=run['originalsUnchanged'],binarySha256=run['binarySha256'],golems=golems,
        finalGate=next(reversed(observed.values())).get('mission_gate'),finalPage=next(reversed(observed.values())).get('page'))


def main():
    """현재 핀/최종 검사/실제 이동을 감사하며 전체 미션 완주로 확대하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inspect',type=Path)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.inspect:
        print(json.dumps(inspect(options.inspect.resolve()),ensure_ascii=False,indent=2))
        return
    if not options.verify:
        parser.error('--inspect 또는 --verify가 필요합니다')
    report=json.loads(REPORT.read_text('utf-8'))
    # 새 소스/실행 근거의 전체 바이트를 각각 확인한다.
    for group in ('sources','evidence'):
        # 각 핀은 실제 실행 시점의 바이너리 해시와 혼동하지 않는다.
        for name,wanted in report[group].items():
            assert sha(ROOT/name)==wanted,name
    before=json.loads((ROOT/report['validation']['before']).read_text('utf-8'))
    focused=json.loads((ROOT/report['validation']['focused']).read_text('utf-8'))
    full=json.loads((ROOT/report['validation']['full']).read_text('utf-8'))
    assert before['failed']>0 and focused['tests']==1 and focused['failed']==focused['returnCode']==0
    assert full['tests']==1753 and full['failed']==full['returnCode']==0
    # 성공 입력의 실제 북쪽/남쪽 이동과 생존을 저장된 원문에서 다시 대조한다.
    for name,expected in report['observations'].items():
        actual=inspect(ROOT/name)
        assert actual==expected and actual['returnCode']==0 and actual['originalsUnchanged']
        assert len(actual['golems'])==1
        golem=next(iter(actual['golems'].values()))
        assert len(golem['north'])>3 and len(golem['south'])>3 and golem['lastHp']==50
        assert float(golem['north'][-1][1])<float(golem['north'][0][1])
        assert float(golem['south'][-1][1])>float(golem['south'][0][1])
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('골렘 발 기준점/64보행프레임/실제 다리 왕복 보행 출처 통과')


if __name__=='__main__':
    main()
