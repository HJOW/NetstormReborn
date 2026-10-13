#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ArcherType 최초42/접두2 전체자료를 보존하고 등록 전 include/숫자AST 계약 추가를 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 최초 native 자료와 현재 source/fixture/report의 실제 전체 바이트를 연결한다.
HISTORY=ROOT/'cpppj/hist/archertype-initial'
REPORT=ROOT/'cpppj/archertype.json'
LINEAGE=ROOT/'cpppj/archertype-draft-lineage.json'
PATHS=('cpppj/src/o/RawArcherTypeData.cpp','cpppj/src/o/RawArcherTypeData.h','cpppj/tests/ArcherTypeDataTests.cpp',
       'tools/decomp_archertype_oracle.py','cpppj/tests/fixtures/archertype-x86.tsv','cpppj/archertype.json')
# 숫자 이외의 명시 directions를 누락 속성0으로 받아들이지 않는 정상 계약이다.
PRODUCTION_ADD='''    // 원본은 orientations가 명시된 경우 숫자 AST만 허용한다.
    if (definition.Property("orientations") && !definition.Number("orientations"))
        throw std::invalid_argument("Archer directions 숫자 AST 원본 계약 오류");
    // 누락된 방향수는 원본 초기0이며 숫자는 원본 AST의 float폭으로 읽는다.
'''
# 신규 검사는 C++ 오류 계약만 검사하며 새로운 정상/assert 원본 관찰로 세지 않는다.
TEST_ADD='''// 실제 숫자 AST의 정상 범위 밖 입력을 누락 속성0으로 바꾸지 않는다.
TEST_CASE(ArcherTypeData_InvalidDirectionPropertiesRejected) {
    const auto wrong=RiftTypeDefinition::Parse("typename sunArcher constructor {orientations=\\"many\\";} P00::\\"sarcher.gif\\" #0;");
    const auto large=RiftTypeDefinition::Parse("typename sunArcher constructor {orientations=1e30;} P00::\\"sarcher.gif\\" #0;");
    CHECK(Throws([&] { ReadRawArcherTypeData(wrong); }));CHECK(Throws([&] { ReadRawArcherTypeData(large); }));
}
'''


def check_sources():
    """명시 추가분을 역변환해 최초 전체 바이트와 원본 관찰/fixture 불변을 검사한다."""
    old=json.loads((HISTORY/REPORT.name).read_text(encoding='utf-8'))
    # 당시 report가 기록한 최초 다섯 자료의 전체 SHA를 실제 snapshot으로 검사한다.
    for relative in PATHS[:-1]:
        if digest(HISTORY/Path(relative).name)!=old['files'][relative]:raise RuntimeError('ArcherType 최초 전체snapshot SHA 오류 '+relative)
    for relative in PATHS[:-1]:
        current=(ROOT/relative).read_bytes()
        if relative.endswith('RawArcherTypeData.cpp'):
            addition=PRODUCTION_ADD.encode('utf-8')
            if current.count(addition)!=1:raise RuntimeError('ArcherType 정상계약 추가 범위 오류')
            current=current.replace(addition,b'')
        elif relative.endswith('ArcherTypeDataTests.cpp'):
            newline=b'\r\n' if b'\r\n' in current else b'\n';addition=TEST_ADD.encode('utf-8').replace(b'\n',newline);include=b'#include "o/SquidPop.h"'+newline
            if current.count(addition)!=1 or current.count(include)!=1:raise RuntimeError('ArcherType 검사 추가 범위 오류')
            current=current.replace(addition,b'').replace(include,b'')
        if current!=(HISTORY/Path(relative).name).read_bytes():raise RuntimeError('ArcherType 전체 이전 몸체 역변환 오류 '+relative)
    return old


def generate():
    """42/접두2의 관찰은 그대로 두고 현재 두 파일SHA와 추가native0 출처만 갱신한다."""
    old=check_sources();report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 두 source 변경 이외의 원본 결과와 당시 연결 근거는 그대로 보존한다.
    for relative in PATHS[:3]:report['files'][relative]=digest(ROOT/relative)
    report['draft_lineage']=dict(prior_full_report_sha256=digest(HISTORY/REPORT.name),normal_observations_added=0,prefix_added=0,unique_native_inputs_added=0,reason='등록 전 명시숫자 AST 검사와 직접 SquidPop include/C++ 오류계약검사만 추가. 전체 최초6자료 보존.')
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    LINEAGE.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',normal_preserved=42,prefix_preserved=2,new_native=0,new_unique=0,history_files={str((HISTORY/Path(path).name).relative_to(ROOT)).replace('\\','/'):digest(HISTORY/Path(path).name) for path in PATHS},current_files={path:digest(ROOT/path) for path in PATHS},tool_sha256=digest(__file__)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """역변환/전체출처/현재report와 원본 관찰/fixture 불변을 엄격히 감사한다."""
    old=check_sources();lineage=json.loads(LINEAGE.read_text(encoding='utf-8'));report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 현재 report의 원본 관찰과 기존 파일 목록은 허용한 세 sourceSHA 외에 불변이다.
    projection=json.loads(json.dumps(report));projection.pop('draft_lineage')
    for path in PATHS[:3]:projection['files'][path]=old['files'][path]
    if projection!=old:raise RuntimeError('ArcherType 원본 결과/전체출처 불변 오류')
    for group in ('history_files','current_files'):
        for path,expected in lineage[group].items():
            if digest(ROOT/path)!=expected:raise RuntimeError('ArcherType lineage SHA오류 '+path)
    if lineage['tool_sha256']!=digest(__file__) or lineage['new_native'] or lineage['new_unique']:raise RuntimeError('ArcherType lineage 도구/관찰 오류')
    print(json.dumps(dict(verified=True,normal_preserved=42,prefix_preserved=2,new_native=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
