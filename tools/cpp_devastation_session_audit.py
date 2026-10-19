#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Devastation Session 연결의 실제 검사와 독립 길이/그림 원문을 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 보호 자료·현재 소스와 보존 실행 기록의 기준이다.
REPORT=ROOT/'cpppj/devastation-session-cpp.json'  # 활성화/주기/효과 연결과 미완료 요청/UI를 구별한다.


def sha(path):
    """파일 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """원문과 과거 실행을 덮어쓰지 않고 현재 구현/실제 종료 기록을 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if not options.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존 증거를 구별하여 각각 해시한다.
    for group in ('sources','evidence'):
        for name,wanted in record[group].items():assert sha(ROOT/name)==wanted,name
    # 길이909/그림32의 기대값은 제품 계산으로 재생성하지 않는다.
    for stem,count,instructions in (('bomb-packet',909,21210),('devastation-draw',32,1472)):
        native=json.loads((ROOT/f'cpppj/{stem}-native.json').read_text('utf-8'))
        for name,wanted in native['sha256'].items():assert sha(ROOT/name)==wanted,name
        rows=[line.split('\t') for line in (ROOT/f'cpppj/tests/fixtures/{stem}-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert native['uniqueInputs']==len(rows)==count and native['normalReturns']==count*2 and native['instructions']==instructions
        assert all(row[1]==row[2] for row in rows)
    # 같은 최종 바이너리에서 원문/실제 Session와 전체 회귀가 모두 통과해야 한다.
    for key,count in (('focused',3),('full',1770)):
        path=ROOT/record['validation'][key];result=json.loads(path.read_text('utf-8'))
        assert result['returnCode']==result['failed']==0 and result['tests']==count
        assert result['binarySha256']==record['binaries']['tests']
        assert re.search(r'\b'+str(count)+r' test\(s\), 0 failed check\(s\)',path.with_suffix('.stdout').read_text('utf-8'))
    pins=json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'));assert pins['semanticReversalToHead'] and pins['reports']==95 and pins['pins']==160
    # 현재 경로의 해시와 당시 갱신 이력을 서로 다른 증거로 확인한다.
    for pin in pins['changes']:
        report=json.loads((ROOT/pin['report']).read_text('utf-8'));value=report
        for key in pin['pointer']:value=value[key]
        assert value==sha(ROOT/pin['source'])
        group='/'+'/'.join(map(str,pin['pointer'][:-1]))
        assert any(update.get('unit')==record['unit'] and any(item['path']==pin['source'] and item['group']==group and item['previous']==pin['old'] and item['current']==pin['new'] for item in update['pins']) for update in report.get('integration_updates',[]))
    assert record['activationEffectIntegrated'] and record['regularIntegrated'] and record['castStopVerified']
    assert not record['requestIntegrated'] and not record['uiIntegrated'] and not record['targetCombatVerified'] and not record['gameRun'] and not record['goalComplete']
    assert record['campaignSpellScope']=='Devastation' and record['otherSpellsDeferredToMultiplayer']
    print('Devastation 실제Session·원문909+32·집중3/0·전체1770/0 및 현재 소스 핀 감사 통과')


if __name__=='__main__':main()
