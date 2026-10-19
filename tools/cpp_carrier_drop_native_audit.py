#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Cast/Drop의 독립 원문에서 실제 패킷·거부 관문·효과 순서와 구현 전 경계를 감사한다."""
import argparse
import hashlib
import json
from decomp_carrier_drop import ROOT,FIXTURE,cases

REPORT=ROOT/'cpppj/carrier-drop-analysis.json' # 원본 관측과 아직 미연결인 게임 기능을 구분하는 인수인계 보고서다.


def sha(path):
    """현재 전체 바이트의 SHA256이며 검증 시 기존 원문을 재생성하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect():
    """독립 원문에 직접 기록된 주요 분기만 선별하고 제품의 기대값 계산을 사용하지 않는다."""
    rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert [json.loads(row[0]) for row in rows]==cases()
    assert all(row[1:4]==row[4:7] for row in rows)
    selected={} # 후속 구현에서 유지할 대표 원본 입력과 효과다.
    # 관측된 효과로 입출력의 출처를 고르되 결과를 새로 합성하지 않는다.
    for row in rows:
        c=json.loads(row[0])
        if c['op']==0 and c['genus']==256 and c['afford']==1 and c['immobile']==0 and c['extra']==128 and c['chosen']==60 and c['cost']==100 and c['mutation']==0 and c['local']==1 and c['parentFlags']==0 and c['attached']==1:
            selected.setdefault('spellRequest',row)
        if c['op']==0 and c['chosen']==0 and c['chain']==2 and c['mark']==1:
            selected.setdefault('ordinaryDropSkipsSpell',row)
        if c['op']==1 and c['chain']==2 and c['mark']==0:
            selected.setdefault('castRetainsInitialContent',row)
        if c['op']==2 and c['genus']==256 and c['sender']==257 and c['owner']==1 and c['point']==1 and c['parentFlags']==c['contentFlags']==0 and c['kind']==168:
            selected.setdefault('fullSenderRejected',row)
        if c['op']==2 and c['genus']==256 and c['sender']==c['owner']==1 and c['local']==2 and c['money']==0 and c['point']==1 and c['parentFlags']==c['contentFlags']==0 and c['kind']==168:
            selected.setdefault('insufficientRemoteMoneyContinues',row)
        if c['op']==2 and c['genus']==0 and c['create']==1 and c['flag']==255 and c['carrierGenus']==0x10000 and c['standing']==0:
            selected.setdefault('ordinaryDropNormalizesFlagAndEscapes',row)
    assert len(selected)==6
    assert selected['spellRequest'][2]=='0150090032003c0000'
    assert 'subtract:1:' in selected['spellRequest'][1] and 'notify:3:0:0;select:0:0:1;send:9:' in selected['spellRequest'][1]
    assert selected['ordinaryDropSkipsSpell'][2]=='0150090032003d0000'
    assert selected['castRetainsInitialContent'][2]=='0150090032003c0000'
    assert 'bad-owner:168:257' in selected['fullSenderRejected'][1] and 'create:' not in selected['fullSenderRejected'][1]
    effects=selected['insufficientRemoteMoneyContinues'][1]
    assert 'refund:' in effects and effects.index('refund:')<effects.index('create:')
    assert effects.endswith('local-send:9:015c06004600;broadcast:015c06004600')
    assert 'destroy:60:0:1' in selected['ordinaryDropNormalizesFlagAndEscapes'][1] and 'escape:50:1065353216:4294967295' in selected['ordinaryDropNormalizesFlagAndEscapes'][1]
    return dict(uniqueInputs=len(rows),normalReturns=len(rows)*2,precisionIdentical=True,examples={name:dict(input=json.loads(row[0]),effects=row[1],requestPacket=row[2]) for name,row in selected.items()})


def main():
    """현재 출처와 관측의 요약 일치를 확인하며 실제 게임/주문 효과 완료로 확대하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inspect',action='store_true')
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    actual=inspect()
    if options.inspect:
        print(json.dumps(actual,ensure_ascii=False,indent=2));return
    if not options.verify:
        parser.error('--inspect 또는 --verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 도구와 고정된 실행 근거를 서로 다른 목록으로 읽는다.
    for group in ('sources','evidence'):
        # 동결 원문·실패 로그·보호PE의 전체 바이트를 유지한다.
        for path,wanted in record[group].items():
            assert sha(ROOT/path)==wanted,path
    assert record['observations']==actual and actual['uniqueInputs']==413
    assert not record['productChanged'] and not record['gameRun'] and not record['goalComplete']
    print('Cast/Drop 원본413입력·826반환과 구현 전 인수인계 출처 통과')


if __name__=='__main__':
    main()
