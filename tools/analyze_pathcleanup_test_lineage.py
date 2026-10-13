#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PathCleanup 테스트의 기존 프레임 빈값 생성 한 줄 교정과 native 출처 보존을 감사한다."""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path

# 최초 원본 관찰/컴파일 전 테스트를 별도로 보존하고 fixture·production은 바꾸지 않는다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/pathcleanup.json'
PRIOR=ROOT/'extracted/pathcleanup-before-default-ctor-evidence.json'
BEFORE=ROOT/'extracted/pathcleanup-before-default-ctor-tests.cpp'
TEST='cpppj/tests/PathCleanupTests.cpp'
POSTPOP_REPORT=ROOT/'extracted/pathcleanup-before-priest-postpop-evidence.json' # 최초 실제 공간 검사의 파생 미연결 실패 전 보고서다.
POSTPOP_TEST=ROOT/'extracted/pathcleanup-before-priest-postpop-tests.cpp' # 실제 사제 후처리 연결 전 전체 테스트를 보존한다.
CONSUMER='TEST_CASE(rawPathCleanup_real_process_spatial_prefix_and_shared)' # 이 후속 소비자 검사만 실제 사제 연결로 교정한다.


def digest(path):
    """정확한 관찰/소스 바이트 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observe():
    """기존 모든 production/fixture/PE 근거와 교정 한 줄을 제거한 전체 테스트 소스를 비교한다."""
    old=json.loads(PRIOR.read_text(encoding='utf-8'));assert old['total']==1732 and old['normal_total']==1716 and old['assertion_total']==16;assert digest(BEFORE)==old['files'][TEST]
    current=(ROOT/TEST).read_text(encoding='utf-8');before_post=POSTPOP_TEST.read_text(encoding='utf-8');post_report=json.loads(POSTPOP_REPORT.read_text(encoding='utf-8'));assert digest(POSTPOP_TEST)==post_report['files'][TEST]
    assert before_post.count('frames(scene.types.size(),RiftTypeFrames({}))')==1
    assert before_post.replace('frames(scene.types.size(),RiftTypeFrames({}))','frames(scene.types.size())')==BEFORE.read_text(encoding='utf-8')
    added_includes='#include "o/RawPriestPostPopTail.h"\n#include "o/RawCarrierCheck.h"\n#include "o/SquidFactory.h"\n';assert current.count(added_includes)==1 and current.count(CONSUMER)==1 and before_post.count(CONSUMER)==1
    # 1732개 원본 입력을 읽는 전체 첫 검사와 조회 준비 코드는 새 include를 빼면 모든 토큰/문자가 같다.
    assert current.replace(added_includes,'').split(CONSUMER)[0]==before_post.split(CONSUMER)[0]
    diff=''.join(difflib.unified_diff(before_post.splitlines(keepends=True),current.splitlines(keepends=True),fromfile=POSTPOP_TEST.relative_to(ROOT).as_posix(),tofile=TEST))
    # 테스트 한 줄 외 모든 기존 source/fixture/보호 PE/최신 정밀 body는 완전히 같아야 한다.
    for name,value in old['files'].items():
        if name!=TEST:assert digest(ROOT/name)==value,name
    return dict(prior_report=PRIOR.relative_to(ROOT).as_posix(),prior_report_sha256=digest(PRIOR),prior_test=BEFORE.relative_to(ROOT).as_posix(),prior_test_sha256=digest(BEFORE),postpop_report=POSTPOP_REPORT.relative_to(ROOT).as_posix(),postpop_report_sha256=digest(POSTPOP_REPORT),postpop_test=POSTPOP_TEST.relative_to(ROOT).as_posix(),postpop_test_sha256=digest(POSTPOP_TEST),current_test_sha256=digest(ROOT/TEST),single_constructor_fix_preserved=True,native_test_body_unchanged=True,consumer_diff=diff,consumer_diff_sha256=hashlib.sha256(diff.encode('utf-8')).hexdigest(),consumer_scope='실제 type158 ctor/전체 PriestPostPopTail→Carrier→SquidPostPopBase, fullHP/spot6 이동 가능 분기와 실제 Forcefield 조회0. 낙하/보호막 생성·삭제/로딩 지면 소유자/표시/Graph는 이 검사에서 실행하지 않는다.',all_production_fixture_pe_unchanged=True,new_unique_cases=0)


def generate():
    """기존 관찰 파일을 보존하고 현재 SourceSHA와 테스트 교정 lineage만 덧붙인다."""
    report=copy.deepcopy(json.loads(PRIOR.read_text(encoding='utf-8')));report['files'][TEST]=digest(ROOT/TEST);report['files'][Path(__file__).relative_to(ROOT).as_posix()]=digest(Path(__file__));report['source_lineage']=observe()
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 테스트/SHA와 고정 전후 출처·production/fixture 불변을 다시 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['source_lineage']==observe() and report['files'][TEST]==digest(ROOT/TEST);assert report['files'][Path(__file__).relative_to(ROOT).as_posix()]==digest(Path(__file__))
    print('PathCleanup 빈값 생성 교정 보존·실제 사제 후처리 소비자 연결·production/1732개 fixture/PE 불변 확인')


# 명시 호출에서만 새 SourceSHA를 연결하거나 현재 기록을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
