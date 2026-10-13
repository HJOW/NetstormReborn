#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""명시빈프레임생성두교정과native실행기의기존의존성SHA추가를208원본과역변환연결한다."""
import argparse,hashlib,json
from pathlib import Path

# 신규군집fixture/native보고서와허용된검사·메타데이터교정범위다.
ROOT=Path(__file__).resolve().parent.parent
TEST=ROOT/'cpppj/tests/FarmClusterTests.cpp'
TOOL=ROOT/'tools/decomp_farmcluster_oracle.py'
REPORT=ROOT/'cpppj/farmcluster.json'
LINEAGE=ROOT/'cpppj/farmcluster-cpp-correction.json'
REPLACEMENTS=(('frames(types.size()),','frames(types.size(),RiftTypeFrames({})),') ,('frames.assign(frames.size(),RiftTypeFrames{});','frames.assign(frames.size(),RiftTypeFrames({}));'))
TOOL_OLD="'cpppj/src/o/DerivedConstructors.inc'):"
TOOL_NEW="'cpppj/src/o/DerivedConstructors.inc','tools/decomp_constructionarrival_oracle.py','tools/decomp_damageablepredestroy_oracle.py'):"


def digest(data):
    """전체교정전/후바이트를SHA256으로고정한다."""
    return hashlib.sha256(data).hexdigest()


def inverse(data,replacements):
    """허용된문자열한곳씩만역변환해그외전체토큰/바이트를보존한다."""
    for before,after in replacements:
        if data.count(after.encode())!=1:raise RuntimeError('FarmCluster 허용교정위치개수오류')
        data=data.replace(after.encode(),before.encode())
    return data


def apply():
    """검사/실행기/보고서전체snapshot을내장하고현재SHA와기존의존성을명시연결한다."""
    before_test=(ROOT/'extracted/farmcluster-before-cpp-correction-tests.cpp').read_bytes();before_tool=(ROOT/'extracted/farmcluster-before-cpp-correction-oracle.py').read_bytes();before_report=(ROOT/'extracted/farmcluster-before-cpp-correction-evidence.json').read_bytes();report=json.loads(before_report)
    if report['files'][TEST.relative_to(ROOT).as_posix()]!=digest(before_test) or report['files'][TOOL.relative_to(ROOT).as_posix()]!=digest(before_tool):raise RuntimeError('FarmCluster 교정전SourceSHA오류')
    evidence=dict(host='HJOW-Athlon',date='2026-10-14',new_native_observations=0,fixture_cases=208,cpp_replacements=list(REPLACEMENTS),tool_replacements=[[TOOL_OLD,TOOL_NEW]],
        before_test_text=before_test.decode(),before_test_sha=digest(before_test),after_test_sha=digest(TEST.read_bytes()),before_tool_text=before_tool.decode(),before_tool_sha=digest(before_tool),after_tool_sha=digest(TOOL.read_bytes()),before_report_text=before_report.decode(),before_report_sha=digest(before_report),tool_sha=digest(Path(__file__).read_bytes()),
        reason='기본생성자가없는RiftTypeFrames를벡터최초/재초기화둘다명시빈생성으로교정;실행에이미사용한두기존Python의존성SHA추가')
    LINEAGE.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    # 새게임동작관찰은추가하지않고현재검사/메타데이터/근거SHA만연결한다.
    for path in (TEST,TOOL,LINEAGE,Path(__file__),ROOT/'tools/decomp_constructionarrival_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py'):report['files'][path.relative_to(ROOT).as_posix()]=digest(path.read_bytes())
    report['cpp_correction']=dict(evidence=LINEAGE.relative_to(ROOT).as_posix(),new_native_observations=0,explicit_empty_frame_constructions=2,existing_python_dependency_hashes_added=2)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """두명시빈생성/메타데이터역변환과208fixture·원본/전체나머지출처불변을감사한다."""
    evidence=json.loads(LINEAGE.read_text(encoding='utf-8'));prior=json.loads(evidence['before_report_text']);report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,path,replacements in (('test',TEST,evidence['cpp_replacements']),('tool',TOOL,evidence['tool_replacements'])):
        current=path.read_bytes()
        if digest(current)!=evidence['after_'+name+'_sha'] or inverse(current,replacements)!=evidence['before_'+name+'_text'].encode() or digest(evidence['before_'+name+'_text'].encode())!=evidence['before_'+name+'_sha']:raise RuntimeError('FarmCluster 전체역변환오류 '+name)
    if digest(evidence['before_report_text'].encode())!=evidence['before_report_sha'] or evidence['tool_sha']!=digest(Path(__file__).read_bytes()) or evidence['new_native_observations'] or evidence['fixture_cases']!=208 or report['editions']!=prior['editions'] or report['total']!=prior['total']:raise RuntimeError('FarmCluster 교정/native 분류오류')
    changed={TEST.relative_to(ROOT).as_posix(),TOOL.relative_to(ROOT).as_posix()}
    # 두교정파일외모든production/PE/export/fixture는이전SHA를그대로유지한다.
    for name,value in prior['files'].items():
        if name not in changed and (digest((ROOT/name).read_bytes())!=value or report['files'][name]!=value):raise RuntimeError('FarmCluster 이전전체출처변경 '+name)
    print('FarmCluster 명시빈프레임2곳/metadata역변환·208fixture전체출처보존/새native0 감사 통과')


# root가승인한빌드교정의현재출처연결/감사만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--apply',action='store_true');args=parser.parse_args();apply() if args.apply else verify()
