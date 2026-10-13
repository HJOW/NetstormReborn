#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 Archer 전체 바이트 snapshot과 target0 90도/Host group0 교정을 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_owner_oracle import ROOT, digest

# 최초1690행/전체 보고서·source/test/oracle를 수정 전에 보존한 경로다.
BEFORE=ROOT/'cpppj/hist/gunarcher-initial'
REPORT=ROOT/'cpppj/gunarcher-correction.json'
CPP=ROOT/'cpppj/src/o/RawArcherAnimation.cpp'
TEST=ROOT/'cpppj/tests/ArcherAnimationTests.cpp'
# 검증 가능한 현재 source 두 수정 및 설명과 입력 수정의 역변환이다.
CHANGES=(
    ('// target0 방향 BYTE는 실제 0050afd8/CD00505354의 90도 계수로 변환한다.\nconstexpr float kAimAngleScale=90.0f;\n',''),
    ('angle=hooks_.aimDirection(fields.Parent())*kAimAngleScale;','angle=hooks_.aimDirection(fields.Parent())*45.0f;'),
)


def audit():
    """이전 전체 SHA를 실제 snapshot 또는 불변 파일과 검사하고 명시 수정만 허용한다."""
    previous=json.loads((BEFORE/'gunarcher.json').read_text(encoding='utf-8'))
    for name,expected in previous['files'].items():
        saved=BEFORE/Path(name).name;path=saved if saved.exists() else ROOT/name
        if digest(path)!=expected:raise RuntimeError('Archer 이전 근거 SHA 불일치 '+name)
    if (BEFORE/'gunarcher-x86.tsv').read_bytes()!=(ROOT/'cpppj/tests/fixtures/gunarcher-x86.tsv').read_bytes():raise RuntimeError('Archer 같은1690개 fixture 바이트 변경')
    current=CPP.read_text(encoding='utf-8')
    # target0 계수 수정과 그 설명 이외의 source 변경은 거부한다.
    for now,old in CHANGES:
        if current.count(now)!=1:raise RuntimeError('Archer 교정 source 분기 누락')
        current=current.replace(now,old,1)
    if current!=(BEFORE/CPP.name).read_text(encoding='utf-8'):raise RuntimeError('Archer 명시 계수 외 source 변경')
    current_test=TEST.read_text(encoding='utf-8')
    if current_test.count('types[71].group=0;')!=1 or current_test.replace('types[71].group=0;','',1)!=(BEFORE/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('Archer Host group 입력 외 test 변경')


def generate():
    """현재 source/report/로그와 전체 초기 snapshot을 새 관찰0 교정 근거로 저장한다."""
    audit();paths=[Path(__file__),CPP,TEST,ROOT/'cpppj/gunarcher.json',ROOT/'cpppj/tests/fixtures/gunarcher-x86.tsv',ROOT/'extracted/run8-ArcherAnimation_-first.log',*[path for path in BEFORE.iterdir() if path.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',native_rows=1690,normal=1688,prefix=2,new_unique_rows=0,changes=['실제 target0 방향 계수90을 적용(초안45 교정)','실제Host 검사 입력 type71.group=0 공급'],initial_full_snapshot=True,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=1690,new_unique_rows=0),ensure_ascii=False))


def verify():
    """전체 초기 snapshot/명시 수정/현재 SHA와 동일 입력을 다시 감사한다."""
    audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('Archer 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=1690,new_unique_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
