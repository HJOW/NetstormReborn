#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early5 캐논 관측과 폭탄 삭제 회귀의 출처·실패·완료 범위를 확인한다."""
import argparse
import hashlib
import json
from pathlib import Path

from cpp_campaign_entry_smoke import ROOT
from cpp_world_smoke import states

REPORT = ROOT/'cpppj/sun-bomb-lifetime-runtime.json'  # 제품/검사/실제 입력 관측의 서로 다른 근거다.


def sha(path):
    """원문 전체 바이트를 읽되 과거 실행 파일의 해시를 현재 값으로 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """성공한 이정표와 다음 실패를 저장된 실제 상태에서 각각 읽는다."""
    run = json.loads((directory/'run.json').read_text('utf-8'))  # 실제 실행 직후 기록이다.
    assert run['originalsUnchanged']
    observed = states(directory/'states.tsv')
    cannons, damage = [], []  # 화면 보고별 실제 완공 HP와 동일 적 SID의 피해다.
    # 보고 이름만으로 완공이나 공격 성공을 가정하지 않는다.
    for label, state in observed.items():
        # 같은 실제 자산의 타입·소유자·HP를 읽는다.
        for sid, row in state['objects'].items():
            if row[:2] == ['sunCannon', '1']:
                cannons.append(dict(label=label, sid=sid, xy=row[3:5], hp=int(row[12])))
            if sid == 15006 and row[0] == 'thunderCannon' and int(row[12]) < 1000:
                damage.append(dict(label=label, sid=sid, hp=int(row[12])))
    errors = (directory/'stderr.log').read_text('utf-8').splitlines()  # 원래 오류를 숨기거나 성공으로 바꾸지 않는다.
    failure = errors[-1] if run['returnCode'] else None
    last = next(reversed(observed))  # 실제 저장된 마지막 관측이다.
    return dict(returnCode=run['returnCode'], binarySha256=run['binarySha256'], originalsUnchanged=True,
        lastLabel=last, gameTime=float(observed[last]['raw_world'][0]), lastGate=observed[last].get('mission_gate'),
        cannonReports=cannons, enemyDamageReports=damage, nextFailure=failure, combat2Observed='combat2' in observed,
        naturalTutorialFiveComplete=False)


def main():
    """게임을 열지 않고 실제 저장 관측·현재 연결 근거를 재검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--inspect', type=Path)
    args = parser.parse_args()
    if args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()), ensure_ascii=False))
        return
    if not args.verify:
        parser.error('--verify 또는 --inspect가 필요합니다')
    report = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존된 관측의 전체 바이트를 별개로 확인한다.
    for group in ('sources', 'evidence'):
        # 역사 실행의 바이트를 현재 바이너리로 덮어쓰지 않는다.
        for name, expected in report[group].items():
            assert sha(ROOT/name) == expected, name
    # 모든 실제 재생은 저장된 원래 결과와 정확하게 대조한다.
    for name, expected in report['observations'].items():
        assert inspect(ROOT/name) == expected, name
    full = json.loads((ROOT/report['validation']['fullTests']).read_text('utf-8'))
    focused = json.loads((ROOT/report['validation']['focusedTests']).read_text('utf-8'))
    assert full['tests'] == 1732 and full['failed'] == full['returnCode'] == 0
    assert focused['tests'] == 1 and focused['failed'] == focused['returnCode'] == 0
    previous = report['observations']['extracted/goal-early5-forward-20261019-ui4']
    assert previous['returnCode'] == 1 and 'TYPE163 SID3629' in previous['nextFailure']
    assert any(row['hp'] == 600 for row in previous['cannonReports'])
    assert any(row['hp'] == 958 for row in previous['enemyDamageReports'])
    assert not report['goalComplete'] and not report['fullFidelityVerified'] and not report['naturalTutorialFiveComplete']
    print('Early5 실제 캐논 피해·폭탄 삭제 집중/전체1732검사와 원래 실패 출처 통과')


if __name__ == '__main__':
    main()
