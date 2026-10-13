#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PathStart 저장 근거의 전체 타입 입력·실제 body/경계·두 정밀도 정상 반환을 독립 감사한다."""
import collections
import itertools
import json
from pathlib import Path
from decomp_pathstart_oracle import ROOT, FIXTURE, TYPE_FIXTURE, REPORT, inputs, sha


def rows(path):
    """주석을 제외한 UTF-8 TSV를 원본 순서로 읽는다."""
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]


def verify():
    """출처와 새512개 입력·회귀718개 분기 관찰이 전체 기록과 맞는지 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));start=rows(FIXTURE);types=rows(TYPE_FIXTURE)
    assert report['host']=='HJOW-Athlon' and report['os_calls']==0
    assert report['total']==len(start)==512 and report['type_total']==len(types)==718
    assert report['new_unique_cases']==512 and report['regression_type_cases']==718
    assert len({tuple(row[:2]) for row in start})==512 and len({tuple(row[:3]) for row in types})==718
    assert not report['precision_differences']
    # 기록된 모든 입력·도구·현재 소스·정밀 body·보호 PE의 실제 바이트를 확인한다.
    for name,expected in report['files'].items():assert sha(ROOT/name)==expected,name
    # 판본별 정상 Start와 알려지지 않은 CD 종류 assert를 혼동하지 않는다.
    for edition,count in (('originals',188),('originalCD',171)):
        observed=report['editions'][edition];chosen=[row for row in start if row[0]==edition];type_rows=[row for row in types if row[0]==edition]
        assert [row[1] for row in chosen]==[','.join(map(str,case)) for case in inputs()]
        assert [tuple(map(int,row[1:3])) for row in type_rows]==list(itertools.product(range(count),(0,0x20000)))
        assert len(chosen)==256 and observed['returns']==512 and observed['assertions']==0
        assert sum(int(row[4]) for row in type_rows)==observed['type_assertions']==(0 if edition=='originals' else 163)
        assert all(len(row)==8 for row in chosen) and all(len(row)==5 for row in type_rows)
        begin='00489080' if edition=='originals' else '0047d150'
        assert observed['entries'][begin]==(576 if edition=='originals' else 512)
        assert observed['entries']['0048a660' if edition=='originals' else '0047ec60']==observed['boundaries']['allocate']
        assert observed['boundaries']['attach']==256 and observed['boundaries']['frame']==128 and observed['boundaries']['kill']==384
        if edition=='originals':assert observed['boundaries']['clear']==128
        else:assert 'clear' not in observed['boundaries']
        assert not set(observed['boundaries'])&{'begin','next','init','search','prepare','choose'}
        # 정상 사제는 실제 두 취소 루프를 완료하고 기본 프레임을 지정한 관찰이어야 한다.
        for row in chosen:
            case=tuple(map(int,row[1].split(',')));pending=tuple(map(int,row[7].split(',')))
            if case[0]==158 and row[2]=='1':
                assert pending[:2]==(0,0) and row[6]=='7'
                assert row[3].endswith('M:7') and row[3].count('E:604')==row[3].count('E:605')==case[5]
            else:assert pending[:2]==(case[5],case[5])
    print('PathStart 신규512+종류선택 회귀718 입력·SHA·실제 body·경계·정상반환 감사 통과')


# 명시 호출에서만 현재 근거를 감사하며 PE를 재실행하거나 게임 창을 만들지 않는다.
if __name__=='__main__':verify()
