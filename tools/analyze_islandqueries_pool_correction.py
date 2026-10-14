#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Query 검사풀의단일capacity 교정을이전전체소스/8490 native fixture와역변환으로연결한다."""
import argparse,json,hashlib
from pathlib import Path

# 기존native보고서와오직한검사소스의교정범위다.
ROOT=Path(__file__).resolve().parent.parent
TEST=ROOT/'cpppj/tests/IslandQueriesTests.cpp'
NATIVE=ROOT/'cpppj/islandqueries.json'
LINEAGE=ROOT/'cpppj/islandqueries-pool-correction.json'
OLD=':pool(edition,128,false),runtime('
NEW=':pool(edition,kCapacity,false),runtime('


def digest(data):
    """바이트SHA를고정한다."""
    return hashlib.sha256(data).hexdigest()


def apply():
    """교정전전체검사/원본보고서를보존한뒤허용된capacity 한줄만바꾼다."""
    before=TEST.read_bytes();old_report=NATIVE.read_bytes();report=json.loads(old_report)
    if before.count(OLD.encode())!=1:raise RuntimeError('Query capacity 교정원문개수오류')
    if digest(before)!=report['files'][TEST.relative_to(ROOT).as_posix()]:raise RuntimeError('Query 교정전검사SHA불일치')
    (ROOT/'extracted/islandqueries-before-pool-correction-tests.cpp').write_bytes(before)
    (ROOT/'extracted/islandqueries-before-pool-correction-evidence.json').write_bytes(old_report)
    after=before.replace(OLD.encode(),NEW.encode());TEST.write_bytes(after)
    evidence=dict(host='HJOW-Athlon',new_native_observations=0,fixture_cases=report['total'],before_test_text=before.decode('utf-8'),before_test_sha=digest(before),after_test_sha=digest(after),
        inverse_from=NEW,inverse_to=OLD,previous_report_sha=digest(old_report),previous_report_text=old_report.decode('utf-8'),tool_sha=digest(Path(__file__).read_bytes()))
    LINEAGE.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    report['files'][TEST.relative_to(ROOT).as_posix()]=digest(after)
    report['files'][LINEAGE.relative_to(ROOT).as_posix()]=digest(LINEAGE.read_bytes())
    report['files'][Path(__file__).relative_to(ROOT).as_posix()]=digest(Path(__file__).read_bytes())
    report['cpp_pool_correction']=dict(evidence=LINEAGE.relative_to(ROOT).as_posix(),new_native_observations=0,reason='검사pool capacity128을원본예측SID영역보다큰기존kCapacity24000으로교정')
    NATIVE.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """한줄역변환과이전전체출처/fixture·현재SHA를연결하고새native0을확인한다."""
    evidence=json.loads(LINEAGE.read_text(encoding='utf-8'));current=TEST.read_bytes();prior=json.loads(evidence['previous_report_text']);report=json.loads(NATIVE.read_text(encoding='utf-8'))
    successor=evidence.get('macro_successor')
    current_tool_sha=digest(Path(__file__).read_bytes());expected_tool_sha=evidence['tool_sha']
    if successor:
        macro_bytes=(ROOT/successor['evidence']).read_bytes();macro=json.loads(macro_bytes)
        if digest(macro_bytes)!=successor['sha'] or digest(current)!=macro['after_test_sha'] or current.replace(b'TEST_CASE(',b'TEST(')!=macro['before_test_text'].encode():raise RuntimeError('Query pool macro후속연결오류')
        if digest(macro['previous_pool_tool_text'].encode())!=evidence['tool_sha']:raise RuntimeError('Query pool 이전실행기출처오류')
        current=macro['before_test_text'].encode();expected_tool_sha=macro['current_pool_tool_sha']
    if digest(evidence['previous_report_text'].encode())!=evidence['previous_report_sha'] or digest(current)!=evidence['after_test_sha'] or current.count(NEW.encode())!=1 or current.replace(NEW.encode(),OLD.encode())!=evidence['before_test_text'].encode():raise RuntimeError('Query pool 한줄역변환오류')
    if digest(evidence['before_test_text'].encode())!=evidence['before_test_sha'] or expected_tool_sha!=current_tool_sha or evidence['new_native_observations'] or evidence['fixture_cases']!=8490:raise RuntimeError('Query pool lineage 계수/SHA오류')
    # 이전검사외모든소스/원본PE/export/fixture는이전바이트SHA를그대로유지한다.
    for name,value in prior['files'].items():
        if name!=TEST.relative_to(ROOT).as_posix() and (digest((ROOT/name).read_bytes())!=value or report['files'][name]!=value):raise RuntimeError('Query 이전출처변경 '+name)
    if report['editions']!=prior['editions'] or report['total']!=prior['total'] or report['cpp_pool_correction']['new_native_observations']:raise RuntimeError('Query native 관찰변경오류')
    print('IslandQueries pool 한줄역변환/8490 이전fixture·전체출처보존/새native0 감사 통과')


# root가승인한한줄교정과현재보존근거감사만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--apply',action='store_true');args=parser.parse_args();apply() if args.apply else verify()
