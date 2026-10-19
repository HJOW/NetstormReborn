#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리기그 방송 연결과 실제 Early5 전진 관측의 성공 범위·다음 실패를 구별한다."""
import argparse
import hashlib
import json
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT
from cpp_world_smoke import states

REPORT = ROOT/'cpppj/flyer-command-runtime.json'  # 현재 소스/원본 패킷/전체 회귀와 부분 플레이의 출처다.


def sha(path):
    """전체 원문 바이트의 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory):
    """실패한 실제 실행을 성공으로 바꾸지 않고 그 전에 도달한 건설·자원만 검사한다."""
    run = json.loads((directory/'run.json').read_text(encoding='utf-8'))  # 최종 실제 종료/원본 복구 결과다.
    assert run['returnCode']==1 and run['originalsUnchanged']
    observed = states(directory/'states.tsv')  # 저장된 실제 보고를 그대로 읽는다.
    generator, archer, bridge, tower = (observed[name] for name in ('generator','defenseArcher','bridge12','forwardTower'))
    assert float(generator['raw_sp'][1])==2400 and float(archer['raw_sp'][1])==2100 and float(tower['raw_sp'][1])==1700
    assert any(row[:2]==['windBattery','1'] and row[12]=='700' for row in bridge['objects'].values())
    assert any(row[:2]==['sunArcher','1'] and row[12]=='300' for row in bridge['objects'].values())
    assert int(bridge['mission_bridges'][0])>=40
    initial_towers = {sid for sid,row in bridge['objects'].items() if row[:2]==['sunBlocker','1']}  # 이미 존재하던 타워와 새 타워를 구별한다.
    new_towers = {sid for sid,row in tower['objects'].items() if row[:2]==['sunBlocker','1']}-initial_towers
    assert len(new_towers)==1
    new_sid = next(iter(new_towers))  # 배치 시점의 같은 새 원본 SID다.
    assert tower['objects'][new_sid][12]=='0' and tower['mission_gate'][1]=='68'
    error = (directory/'stderr.log').read_text(encoding='utf-8')  # 다음 배치 탐색 실패를 숨기지 않는다.
    assert 'UI control not found: placement-fire:136,110,15006' in error and 'legal28 fire0' in error
    assert '실제 세션 하위 소비자 미연결' not in error
    assert 'forwardCannon' not in observed and 'combat2' not in observed
    return dict(returnCode=1,originalsUnchanged=True,naturalConstructionObserved=True,gameplayRunPassed=False,
                bridgesBuilt=int(bridge['mission_bridges'][0]),generatorHp=700,archerHp=300,sp=1700,
                newTower=new_sid,point=tower['objects'][new_sid][3:5],towerHp=0,gameTime=float(tower['raw_world'][0]),
                nextFailure='합법28/사격0으로 캐논 배치 좌표 탐색 실패',priorMissingConsumerReproduced=False,
                nativeCommandInvocationCountObserved=False,combatVerified=False,naturalTutorialFiveComplete=False,goalComplete=False)


def main():
    """저장 관측 검사와 현재 근거 검증만 수행하며 게임은 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inspect',type=Path)
    parser.add_argument('--verify',action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 실제 실패/이전 미완성 표본과 현재 소스를 모두 보존한다.
        for group in ('sources','evidence'):
            # 파일마다 전체 원문 바이트를 대조한다.
            for name,expected in report[group].items():
                assert sha(ROOT/name)==expected,name
        tests = json.loads((ROOT/report['validation']['fullTests']).read_text(encoding='utf-8'))
        assert tests['tests']==1731 and tests['failed']==tests['returnCode']==0
        assert inspect(ROOT/report['validation']['naturalRun'])['naturalConstructionObserved']
        assert not report['goalComplete'] and not report['fullFidelityVerified'] and not report['combatVerified']
        print('훨리기그 방송 원본/전체1731검사/실제 전진 관측과 다음 실패 출처 통과')
    elif args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()),ensure_ascii=False))
    else:
        parser.error('--inspect 또는 --verify가 필요합니다')


if __name__=='__main__':
    main()
