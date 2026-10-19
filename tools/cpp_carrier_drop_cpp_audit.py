#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Cast/Drop와 주문 활성화의 실제 C++ 회귀/독립 기대/중지 인수인계를 읽기 전용 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소와 보존된 검사 근거의 기준이다.
REPORT=ROOT/'cpppj/carrier-drop-cpp.json' # 해당 작업 단위의 실제 소스/검사 해시다.


def sha(path):
    """전체 바이트를 읽어 보존 자료가 바뀌지 않았는지 확인한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """현재 소스와 종료된 검사를 대조하며 UI 미연결이나 전체 목표 미완료를 숨기지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 새 소스와 실제 원문/검사의 전체 해시를 각각 확인한다.
    for group in ('sources','evidence'):
        # 보존된 이전 실행 파일 해시를 현재 바이너리 값으로 바꾸지 않는다.
        for name,wanted in record[group].items():
            assert sha(ROOT/name)==wanted,name
    focused=[json.loads((ROOT/name).read_text('utf-8')) for name in record['validation']['focused']]
    full=json.loads((ROOT/record['validation']['fullAttempt']).read_text('utf-8'))
    interruption=json.loads((ROOT/record['validation']['interruption']).read_text('utf-8'))
    assert len(focused)==2 and sum(row['tests'] for row in focused)==4
    # 실제 종료/실패 수/동일 검사 바이너리를 대조한다.
    for row in focused:
        assert row['returnCode']==row['failed']==0 and row['binarySha256']==record['currentBinaries']['tests']
    assert record['validation']['fullDeferred'] and interruption['userRequested']
    assert interruption['status']=='interrupted' and interruption['resumeFullRegressionFirst']
    assert full['tests'] is None and full['failed'] is None and full['returnCode']==4294967295
    assert full['binarySha256']==record['currentBinaries']['tests']==interruption['binarySha256']
    native=json.loads((ROOT/'cpppj/bomb-activation-native.json').read_text('utf-8'))
    assert native['uniqueInputs']==434 and native['normalReturns']==868
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/bomb-activation-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==434 and all(row[1:3]==row[3:5] for row in rows)
    drop=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/carrier-drop-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(drop)==413 and all(row[1:4]==row[4:7] for row in drop)
    assert not record['sessionIntegrated'] and not record['gameRun'] and not record['goalComplete']
    assert record['pauseAfterCommitRequested'] and record['resumeAt']
    print('Cast/Drop413·활성화434 원문/집중4 통과 및 사용자 요청에 따른 전체 회귀 이월 인수인계 출처 확인')


if __name__=='__main__':
    main()
