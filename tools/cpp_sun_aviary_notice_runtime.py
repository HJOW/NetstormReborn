#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""홈 통지 원문·실제 재출격 회귀와 Early5 입력 관측의 범위를 대조한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_world_smoke import states

REPORT = ROOT/'cpppj/sun-aviary-home-runtime.json'  # 현재 연결/과거 실패/수정 후 실제 입력의 근거다.


def sha(path):
    """파일 전체 바이트를 읽되 과거 실행 해시는 새 바이너리로 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 저장 객체의 HP/소유자·단계와 원래 종료 오류만 읽는다."""
    run=json.loads((directory/'run.json').read_text('utf-8'))
    assert run['originalsUnchanged']
    observed=states(directory/'states.tsv')
    waited=observed.get('forwardGeneratorWaited')  # 준비 초기에 끝난 실행에는 이 이정표가 없을 수 있다.
    generators={str(sid):dict(xy=row[3:5],hp=int(row[12])) for sid,row in waited['objects'].items() if row[:2]==['windBattery','1']} if waited else {}
    cannons={}
    enemies={15006:[],15013:[]}
    flyers=set()
    natural_e=False
    # 보고 이름이 시도라고 해도 실제 객체가 없으면 건설로 세지 않는다.
    for label,state in observed.items():
        # 동일 실제 SID의 HP를 이전 보고와 구별해 보존한다.
        for sid,row in state['objects'].items():
            if row[:2]==['sunCannon','1']:
                record=cannons.setdefault(str(sid),dict(firstLabel=label,xy=row[3:5],firstHp=int(row[12]),maxHp=int(row[12]),lastLabel=label,lastHp=int(row[12])))
                record['maxHp']=max(record['maxHp'],int(row[12]))
                record['lastLabel']=label
                record['lastHp']=int(row[12])
            if sid in enemies and row[0] in ('thunderCannon','sunAviary'):
                enemies[sid].append(dict(label=label,hp=int(row[12])))
            if row[:2]==['sunFlyer','2']:
                flyers.add(sid)
        gate=state.get('mission_gate',[])  # 단계는 검사에서 직접 쓰지 않는다.
        if gate[:2]==['5','69']:
            assert not any(row[0] in ('thunderCannon','sunAviary') and row[1]=='2' for row in state['objects'].values())
            natural_e=True
    last=next(reversed(observed))
    errors=(directory/'stderr.log').read_text('utf-8').splitlines()
    return dict(returnCode=run['returnCode'],binarySha256=run['binarySha256'],originalsUnchanged=True,
        lastLabel=last,gameTime=float(observed[last]['raw_world'][0]),lastGate=observed[last].get('mission_gate'),
        forwardGeneratorsAfterWait=generators,cannons=cannons,sunFlyerSidsObserved=sorted(flyers),
        enemyMinimumHp={str(sid):min(row['hp'] for row in rows) for sid,rows in enemies.items() if rows},
        naturalEObserved=natural_e,combat2Observed='combat2' in observed,nextFailure=errors[-1] if run['returnCode'] else None)


def main():
    """게임을 열지 않고 기존 실행/원본 기대값/현재 연결의 출처를 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--inspect',type=Path)
    args=parser.parse_args()
    if args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()),ensure_ascii=False))
        return
    if not args.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    report=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 저장된 실제 실패/검사를 서로 다른 파일로 확인한다.
    for group in ('sources','evidence'):
        # 이미 보존된 값을 수정해 맞추지 않는다.
        for name,expected in report[group].items():
            assert sha(ROOT/name)==expected,name
    # 각 실행은 저장된 당시의 실제 결과와 대조한다.
    for name,expected in report['observations'].items():
        assert inspect(ROOT/name)==expected,name
    full=json.loads((ROOT/report['validation']['fullTests']).read_text('utf-8'))
    focused=json.loads((ROOT/report['validation']['relaunchTests']).read_text('utf-8'))
    assert full['tests']==1735 and full['failed']==full['returnCode']==0
    assert focused['tests']==1 and focused['failed']==focused['returnCode']==0
    previous=report['observations']['extracted/goal-early5-cannon-20261019-ui1']
    assert previous['returnCode']==1 and '플라이어 홈 가상 통지 미복원' in previous['nextFailure']
    assert len(previous['forwardGeneratorsAfterWait'])==2 and all(row['hp']==700 for row in previous['forwardGeneratorsAfterWait'].values())
    latest=report['observations']['extracted/goal-early5-cannon-20261019-ui3']  # 실제 수정 후 전투와 다음 경계도 원래 값으로 요구한다.
    assert latest['returnCode']==1 and 'RawCampaignSession.cpp:1905:' in latest['nextFailure']
    assert any(row['maxHp']==600 and row['lastHp']==30 for row in latest['cannons'].values())
    assert latest['enemyMinimumHp']['15006']==612 and len(latest['sunFlyerSidsObserved'])==2
    assert not latest['naturalEObserved'] and not latest['combat2Observed']
    assert not report['goalComplete'] and not report['fullFidelityVerified']
    print('훨리베이스 홈 통지128원문·실제 재출격·전체1735검사와 Early5 관측 출처 통과')


if __name__=='__main__':
    main()
