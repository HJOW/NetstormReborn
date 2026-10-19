#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Cast/Drop 실제 Session 연결과 보존 원문·전송 검사·회귀 기록을 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 현재 소스와 보존 원시 기록을 찾는 저장소 기준이다.
REPORT=ROOT/'cpppj/carrier-drop-session-cpp.json'  # 실제 요청 연결과 미검증 화면 플레이를 구별한다.


def sha(path):
    """파일 전체 바이트를 읽기 전용으로 해시한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """과거 실행을 재생성하지 않고 현재 연결과 실제 종료 기록을 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if not options.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존 실행 증거의 해시를 각자 검증한다.
    for group in ('sources','evidence'):
        for name,wanted in record[group].items():assert sha(ROOT/name)==wanted,name
    native=json.loads((ROOT/'cpppj/carrier-drop-native.json').read_text('utf-8'))
    # 독립 x86 기대값과 수집 도구는 제품 연결에 맞춰 재생성하지 않는다.
    for name,wanted in native['sha256'].items():assert sha(ROOT/name)==wanted,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/carrier-drop-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert native['uniqueInputs']==len(rows)==413 and native['normalReturns']==826 and native['instructions']==107290
    assert all(row[1:4]==row[4:7] for row in rows)
    # 집중 검사와 전체 회귀는 같은 최종 바이너리의 실제 종료 기록이어야 한다.
    for key,count in (('focused',2),('nativeFocused',1),('full',1772)):
        path=ROOT/record['validation'][key];result=json.loads(path.read_text('utf-8'))
        assert result['returnCode']==result['failed']==0 and result['tests']==count
        assert result['binarySha256']==record['binaries']['tests']
        assert sha(path.with_suffix('.stdout'))==result['stdoutSha256'] and sha(path.with_suffix('.stderr'))==result['stderrSha256']
        assert re.search(r'\b'+str(count)+r' test\(s\), 0 failed check\(s\)',path.with_suffix('.stdout').read_text('utf-8'))
    pins=json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'));assert pins['semanticReversalToHead'] and pins['reports']==116 and pins['pins']==297
    # 현재 소스 값은 갱신 이력의 과거 해시와 구별하여 확인한다.
    for pin in pins['changes']:
        report=json.loads((ROOT/pin['report']).read_text('utf-8'));value=report
        for key in pin['pointer']:value=value[key]
        assert value==sha(ROOT/pin['source'])
        group='/'+'/'.join(map(str,pin['pointer'][:-1]))
        assert any(update.get('unit')==record['unit'] and any(item['path']==pin['source'] and item['group']==group and item['previous']==pin['old'] and item['current']==pin['new'] for item in update['pins']) for update in report.get('integration_updates',[]))
    assert record['requestIntegrated'] and record['serverReceiveIntegrated'] and record['menuInputBindingsInstalled'] and record['contentLifetimePreserved']
    assert not record['uiClickVerified'] and not record['targetCombatVerified'] and not record['gameRun'] and not record['goalComplete']
    assert record['campaignSpellScope']=='Devastation' and record['otherSpellsDeferredToMultiplayer']
    legacy=json.loads((ROOT/record['validation']['legacyAudit']).read_text('utf-8'));assert legacy['returnCode']==1
    newline=json.loads((ROOT/record['validation']['legacyNewlines']).read_text('utf-8'))
    assert len(newline['sourceNewlineMismatches'])==5 and len(newline['unavailableHistoricalEvidence'])==11
    assert all(item['workingMatchesHeadText'] and (item['expectedIsHeadLf'] or item['expectedIsHeadCrlf']) for item in newline['sourceNewlineMismatches'])
    print('Cast/Drop 실제Session·원문413·집중2+1/0·전체1772/0 및 현재 소스 핀 감사 통과')


if __name__=='__main__':main()
