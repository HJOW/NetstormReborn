#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""의식 동작 테스트의 Factory include/실제 사제 ctor 입력 교정과 기존 native 출처를 연결한다."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

# 최초 관찰/컴파일 실패 입력은 별도 snapshot으로 보존한다. production/fixture는 그대로다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/recovery-priestritualactions-evidence.json'
PRIOR=ROOT/'extracted/priestritualactions-before-factory-include-evidence.json'
BEFORE=ROOT/'extracted/priestritualactions-before-factory-include-tests.cpp'
TEST='cpppj/tests/PriestRitualActionsTests.cpp'
INTERMEDIATE=ROOT/'extracted/priestritualactions-before-number-correction-evidence.json' # 없는 number를 추가한 중간 실패 근거를 별도로 보존한다.
INTERMEDIATE_TEST=ROOT/'extracted/priestritualactions-before-number-correction-tests.cpp' # 타입 정의 오독으로 실패한 테스트의 정확한 바이트를 보존한다.
TAKE_REPORT=ROOT/'extracted/priestritualactions-before-take-correction-evidence.json' # client SID에 서버 전용 Take를 호출한 실패 전 근거다.
TAKE_TEST=ROOT/'extracted/priestritualactions-before-take-correction-tests.cpp' # 서버/client 입력 교정 직전 전체 테스트 바이트다.


def digest(path):
    """정확한 관찰/소스 바이트 SHA를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observe():
    """두 테스트 입력 교정 외 전체 소스와 모든 기존 native 근거의 불변을 확인한다."""
    old=json.loads(PRIOR.read_text(encoding='utf-8'));assert old['total']==1752 and digest(BEFORE)==old['files'][TEST]
    current=(ROOT/TEST).read_text(encoding='utf-8');added_include='#include "o/SquidFactory.h"\n';added_input='types[158].constructorAddress=TypeConstructorAddress(edition,158);';assert current.count(added_include)==1 and current.count(added_input)==1
    take_report=json.loads(TAKE_REPORT.read_text(encoding='utf-8'));assert digest(TAKE_TEST)==take_report['files'][TEST]
    changed=';\n        // SID5는 client 목록에서 생성하고, 서버 영역 SID23999만 실제 수신 Take로 확보한다.\n        CHECK((dataSid==Sid{5} ? factory.Create(type,2) : factory.Take(type,dataSid))==dataSid);'
    original=';CHECK(factory.Take(type,dataSid)==dataSid);';assert current.count(changed)==1
    restored=current.replace(changed,original);assert restored==TAKE_TEST.read_text(encoding='utf-8')
    assert restored.replace(added_include,'').replace(added_input,'')==BEFORE.read_text(encoding='utf-8')
    intermediate=json.loads(INTERMEDIATE.read_text(encoding='utf-8'));assert digest(INTERMEDIATE_TEST)==intermediate['files'][TEST]
    assert INTERMEDIATE_TEST.read_text(encoding='utf-8').replace('types[158].number=158;','')==restored
    # 기존 모든 production/fixture/보호 PE/정밀 body/도구는 동일 바이트다.
    for name,value in old['files'].items():
        if name!=TEST:assert digest(ROOT/name)==value,name
    return dict(prior_report=PRIOR.relative_to(ROOT).as_posix(),prior_report_sha256=digest(PRIOR),prior_test=BEFORE.relative_to(ROOT).as_posix(),prior_test_sha256=digest(BEFORE),intermediate_report=INTERMEDIATE.relative_to(ROOT).as_posix(),intermediate_report_sha256=digest(INTERMEDIATE),intermediate_test=INTERMEDIATE_TEST.relative_to(ROOT).as_posix(),intermediate_test_sha256=digest(INTERMEDIATE_TEST),take_report=TAKE_REPORT.relative_to(ROOT).as_posix(),take_report_sha256=digest(TAKE_REPORT),take_test=TAKE_TEST.relative_to(ROOT).as_posix(),take_test_sha256=digest(TAKE_TEST),current_test_sha256=digest(ROOT/TEST),changes=['SquidFactory 명시 include','실제 사제 type158 TypeConstructorAddress 입력','없는 number 필드 교정 및 중간 실패 입력 보존','client SID5는 Create(flags2),서버 SID23999만 Take; native 입력/raw/fixture 불변'],all_production_fixture_pe_unchanged=True,new_unique_cases=0)


def generate():
    """이전 원본 관찰을 보존하고 현재 테스트 SourceSHA/교정 lineage만 추가한다."""
    report=copy.deepcopy(json.loads(PRIOR.read_text(encoding='utf-8')));report['files'][TEST]=digest(ROOT/TEST);report['files'][Path(__file__).relative_to(ROOT).as_posix()]=digest(Path(__file__));report['source_lineage']=observe()
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 교정·기존 출처 snapshot·production과1752개 fixture 불변을 다시 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['source_lineage']==observe() and report['files'][TEST]==digest(ROOT/TEST);assert report['files'][Path(__file__).relative_to(ROOT).as_posix()]==digest(Path(__file__))
    print('의식 동작 Factory include/사제 ctor 입력 교정·production/1752개 fixture 불변 확인')


# 명시 호출에서만 SourceSHA를 갱신하거나 저장 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
