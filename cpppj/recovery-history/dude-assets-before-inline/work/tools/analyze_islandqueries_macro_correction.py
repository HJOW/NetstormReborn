#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제검사등록매크로TEST_CASE 교정을이전pool lineage/전체소스/native8490과연결한다."""
import argparse,json,hashlib
from pathlib import Path

# 승인된세등록매크로와이전pool교정의현재출처다.
ROOT=Path(__file__).resolve().parent.parent
TEST=ROOT/'cpppj/tests/IslandQueriesTests.cpp'
REPORT=ROOT/'cpppj/recovery-islandqueries-evidence.json'
POOL=ROOT/'cpppj/recovery-islandqueries-pool-correction-evidence.json'
POOL_TOOL=ROOT/'tools/analyze_islandqueries_pool_correction.py'
LINEAGE=ROOT/'cpppj/recovery-islandqueries-macro-correction-evidence.json'


def digest(data):
    """변경전/후바이트의SHA256을반환한다."""
    return hashlib.sha256(data).hexdigest()


def apply():
    """전체교정전자료를내장하고등록identifier 세개만바꾼다."""
    before=TEST.read_bytes();prior_bytes=REPORT.read_bytes();prior=json.loads(prior_bytes)
    if before.count(b'TEST(')!=3 or digest(before)!=prior['files'][TEST.relative_to(ROOT).as_posix()]:raise RuntimeError('Query macro 교정전개수/SHA오류')
    pool_before=(ROOT/'extracted/islandqueries-before-macro-correction-pool-evidence.json').read_bytes()
    tool_before=(ROOT/'extracted/islandqueries-before-macro-correction-pool-tool.py').read_bytes()
    after=before.replace(b'TEST(',b'TEST_CASE(');TEST.write_bytes(after)
    evidence=dict(host='HJOW-Athlon',date='2026-10-14',new_native_observations=0,fixture_cases=8490,macro_changes=3,
        before_test_text=before.decode(),before_test_sha=digest(before),after_test_sha=digest(after),previous_report_text=prior_bytes.decode(),previous_report_sha=digest(prior_bytes),
        previous_pool_evidence_text=pool_before.decode(),previous_pool_evidence_sha=digest(pool_before),previous_pool_tool_text=tool_before.decode(),previous_pool_tool_sha=digest(tool_before),
        current_pool_tool_sha=digest(POOL_TOOL.read_bytes()),tool_sha=digest(Path(__file__).read_bytes()))
    LINEAGE.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    pool=json.loads(pool_before);pool['macro_successor']=dict(evidence=LINEAGE.relative_to(ROOT).as_posix(),sha=digest(LINEAGE.read_bytes()));POOL.write_text(json.dumps(pool,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    # 세변경파일과새후속근거만현재보고서에연결한다.
    for path in (TEST,POOL,POOL_TOOL,LINEAGE,Path(__file__)):prior['files'][path.relative_to(ROOT).as_posix()]=digest(path.read_bytes())
    prior['cpp_macro_correction']=dict(evidence=LINEAGE.relative_to(ROOT).as_posix(),new_native_observations=0,macro_changes=3)
    REPORT.write_text(json.dumps(prior,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """세identifier 역변환/이전전체출처와pool교정원본·현재후속연결을감사한다."""
    evidence=json.loads(LINEAGE.read_text(encoding='utf-8'));current=TEST.read_bytes();prior=json.loads(evidence['previous_report_text']);report=json.loads(REPORT.read_text(encoding='utf-8'))
    if current.count(b'TEST_CASE(')!=3 or current.replace(b'TEST_CASE(',b'TEST(')!=evidence['before_test_text'].encode() or digest(current)!=evidence['after_test_sha'] or digest(evidence['before_test_text'].encode())!=evidence['before_test_sha']:raise RuntimeError('Query macro 역변환오류')
    # 이전보고서/pool근거/실행기전체텍스트와그당시SHA를보존한다.
    for key in ('previous_report','previous_pool_evidence','previous_pool_tool'):
        if digest(evidence[key+'_text'].encode())!=evidence[key+'_sha']:raise RuntimeError('Query macro 이전출처SHA오류')
    changed={path.relative_to(ROOT).as_posix() for path in (TEST,POOL,POOL_TOOL)}
    # 승인된검사/후속연결외모든원본/export/fixture/production을이전바이트로확인한다.
    for name,value in prior['files'].items():
        if name not in changed and (digest((ROOT/name).read_bytes())!=value or report['files'][name]!=value):raise RuntimeError('Query macro 이전전체출처변경 '+name)
    if evidence['new_native_observations'] or evidence['fixture_cases']!=8490 or evidence['tool_sha']!=digest(Path(__file__).read_bytes()) or evidence['current_pool_tool_sha']!=digest(POOL_TOOL.read_bytes()) or report['editions']!=prior['editions']:raise RuntimeError('Query macro 현재실행기/native 계수오류')
    print('IslandQueries TEST_CASE 3등록역변환/이전pool·전체출처·8490fixture보존/새native0 감사 통과')


# root가승인한등록매크로교정과현재출처감사만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--apply',action='store_true');args=parser.parse_args();apply() if args.apply else verify()
