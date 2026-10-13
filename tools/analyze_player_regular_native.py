#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PlayerRegular/State658의 실제 전체 출처와 정상·진단·미복원경계를 감사한다."""
import hashlib
import json
from pathlib import Path

# 재관찰없이현재동결Native자료만읽는다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/player-regular.json'
FIXTURE=ROOT/'cpppj/tests/fixtures/player-regular-x86.tsv'


def verify():
    """전체각파일SHA·입력고유수·상호배타분류와boundary반환누락을확인한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든원본/도구/생산/fixture를전체파일로연결한다.
    for name,expected in proof['files'].items():assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==expected,name
    assert len(rows)==len({(row[0],row[1]) for row in rows})==proof['total']==658
    # 각판본의정상/assert/하위거부는동일행을다시더하지않는다.
    for edition in ('originals','originalCD'):
        selected=[row for row in rows if row[0]==edition];normal=diagnostic=boundary=0
        for row in selected:
            assert len(row)==19
            if row[2]=='boundary':boundary+=1;assert row[3]=='-' and row[4]!='-'
            elif int(row[-1]):diagnostic+=1;assert row[2]=='return'
            else:normal+=1;assert row[2]=='return'
        assert (normal,diagnostic,boundary)==(283,34,12) and proof['editions'][edition]['classification']==dict(normal=283,**{'assert':34,'boundary':12})
        assert proof['editions'][edition]['returns']==2*(normal+diagnostic)
    print('PlayerRegular658: 정상566/assert68/미복원24·전체출처SHA 감사 통과')


if __name__=='__main__':verify()
