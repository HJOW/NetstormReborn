#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C4457 교정의 cellX/cellY 이름 변경과 이전 native 관찰의 동일성을 연결한다."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

# 이전 보고서/테스트는 별도 읽기 전용 snapshot으로 보존한다. native fixture는 다시 생성하지 않는다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/recovery-geysermined-evidence.json'
OLD_REPORT=ROOT/'extracted/geysermined-before-shadow-rename-evidence.json'
OLD_TEST=ROOT/'extracted/geysermined-before-shadow-rename-tests.cpp'
TEST='cpppj/tests/GeyserMinedTests.cpp'


def digest(path):
    """정확한 바이트 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observations():
    """기존 관찰 모든 출처와 이름 변경만 제거한 전체 테스트 소스를 비교한다."""
    prior=json.loads(OLD_REPORT.read_text(encoding='utf-8'));assert prior['total']==2312 and prior['normal_total']==2304 and prior['assertion_total']==8
    assert digest(OLD_TEST)==prior['files'][TEST]
    old=OLD_TEST.read_text(encoding='utf-8');current=(ROOT/TEST).read_text(encoding='utf-8');restored=current.replace('cellX','x').replace('cellY','y');assert restored==old
    # 다른 기존 관찰 출처/fixture/보호 PE/도구의 바이트는 모두 보존되어야 한다.
    for name,value in prior['files'].items():
        if name!=TEST:assert digest(ROOT/name)==value,name
    return dict(prior_report=OLD_REPORT.relative_to(ROOT).as_posix(),prior_report_sha256=digest(OLD_REPORT),prior_test=OLD_TEST.relative_to(ROOT).as_posix(),prior_test_sha256=digest(OLD_TEST),current_test_sha256=digest(ROOT/TEST),normalized_source_equal_after_rename=True,renames={'cellX':'x','cellY':'y'},new_unique_cases=0)


def generate():
    """이전 관찰 보고서를 보존한 채 현재 SourceSHA와 이름 변경 lineage만 추가한다."""
    prior=json.loads(OLD_REPORT.read_text(encoding='utf-8'));report=copy.deepcopy(prior);report['files'][TEST]=digest(ROOT/TEST);report['files'][Path(__file__).relative_to(ROOT).as_posix()]=digest(Path(__file__));report['source_lineage']=observations()
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 이름 변경/기존 관찰 보고서와 fixture 불변을 다시 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['source_lineage']==observations() and report['files'][TEST]==digest(ROOT/TEST)
    assert report['files'][Path(__file__).relative_to(ROOT).as_posix()]==digest(Path(__file__))
    print('GeyserMined C4457 이름 변경·이전2312개 관찰/fixture 불변 확인')


# 명시 호출에서만 현재 SourceSHA를 갱신하거나 기존 기록을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
