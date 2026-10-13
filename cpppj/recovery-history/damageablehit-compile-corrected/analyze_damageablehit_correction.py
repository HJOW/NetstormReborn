#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 피해 검사 Random→GameRandom 한 줄 교정과 이전 전체5846근거를 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 첫 공유 빌드 전 source/test/oracle/fixture/report와 실패 로그의 실제 전체 snapshot이다.
BEFORE=ROOT/'cpppj/recovery-history/damageablehit-initial'
TEST=ROOT/'cpppj/tests/DamageableHitTests.cpp'
NATIVE=ROOT/'cpppj/recovery-damageablehit-evidence.json'
REPORT=ROOT/'cpppj/recovery-damageablehit-correction-evidence.json'


def audit():
    """생산 코드/원본 입력/관찰은 그대로이며 검사 타입 한 줄만 바뀌었는지 검사한다."""
    previous=json.loads((BEFORE/NATIVE.name).read_text(encoding='utf-8'))
    for name,expected in previous['files'].items():
        saved=BEFORE/Path(name).name;path=saved if saved.exists() else ROOT/name
        if digest(path)!=expected:raise RuntimeError('일반 피해 이전 전체 SHA 불일치 '+name)
    current=TEST.read_text(encoding='utf-8')
    if current.count('    GameRandom random;')!=1 or current.replace('    GameRandom random;','    Random random;',1)!=(BEFORE/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('일반 피해 검사 명칭 외 변경')
    for relative in ('cpppj/src/o/RawDamageableHit.cpp','cpppj/src/o/RawDamageableHit.h','tools/decomp_damageablehit_oracle.py','cpppj/tests/fixtures/damageablehit-x86.tsv'):
        if (ROOT/relative).read_bytes()!=(BEFORE/Path(relative).name).read_bytes():raise RuntimeError('일반 피해 production/oracle/fixture 변경 '+relative)
    return previous


def generate():
    """새 native0에서 동일 관찰 보고서에 현재 검사 SHA만 연결하고 원래 전체 보고서를 보존한다."""
    previous=audit();current=json.loads(json.dumps(previous));current['files'][TEST.relative_to(ROOT).as_posix()]=digest(TEST);current['source_correction']=dict(kind='검사 난수 타입 명칭 한 줄',previous_full_report=(BEFORE/NATIVE.name).relative_to(ROOT).as_posix(),previous_sha256=digest(BEFORE/NATIVE.name),new_native_rows=0,new_unique_rows=0);NATIVE.write_text(json.dumps(current,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    paths=[Path(__file__),TEST,NATIVE,ROOT/'extracted/campaign-runtime9-build-initial.log',*[p for p in BEFORE.iterdir() if p.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',native_rows=5846,new_native_rows=0,new_unique_rows=0,initial_full_snapshot=True,change='검사 Random 형식명을 실제 GameRandom으로 교정',files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=5846,new_native_rows=0),ensure_ascii=False))


def verify():
    """당시 전체 보고서/한 줄 역변환과 현재 native/lineage의 실제 SHA를 감사한다."""
    previous=audit();current=json.loads(NATIVE.read_text(encoding='utf-8'));copy=json.loads(json.dumps(current));copy.pop('source_correction');copy['files'][TEST.relative_to(ROOT).as_posix()]=previous['files'][TEST.relative_to(ROOT).as_posix()]
    if copy!=previous or current['source_correction']['previous_sha256']!=digest(BEFORE/NATIVE.name):raise RuntimeError('일반 피해 원본 관찰 또는 보고서 변경')
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('일반 피해 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=5846,new_native_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
