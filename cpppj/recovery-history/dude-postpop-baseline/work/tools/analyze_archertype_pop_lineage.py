#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ArcherType 첫 실제 Pop 실패의 전체자료를 보존하고 기존 자산 후처리 연결만 감사한다."""
import argparse
import copy
import json
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 최초 공유 C++ 실패 때 실제 읽은 전체 source/test/tool/report/기존lineage 자료다.
HISTORY=ROOT/'cpppj/recovery-history/archertype-before-postpop'
TEST='cpppj/tests/ArcherTypeDataTests.cpp'
REPORT=ROOT/'cpppj/recovery-archertype-evidence.json'
DRAFT=ROOT/'cpppj/recovery-archertype-draft-lineage-evidence.json'
LINEAGE=ROOT/'cpppj/recovery-archertype-pop-lineage-evidence.json'
# 현재 기존 몸체를 소비하므로 전체 출처를 현재 native 보고서에도 기록한다.
NEW_FILES=tuple(f'cpppj/src/o/{name}.{extension}' for name in ('SquidPostPop','RawCarrierPostPop','RawAssetPostPop') for extension in ('cpp','h'))+('cpppj/src/o/PopVtables.inc','tools/analyze_archertype_pop_lineage.py')
# 검사만 실제비전투 후처리를 연결한다. 실행하지 않는 전투/로딩 경계는 명시 예외다.
POP_ADDITION='''SquidUnpop unpop(pool,hash,spots,&display);
        // 비전투 초기등록도 실제71 가상 후처리와 같은 Damageable/공통 장부를 연결해야 한다.
        SquidPostPopState bookkeeping;SquidPostPop post(pool,types,bookkeeping);CarrierPostPopState carrierState;
        RawCarrierPostPop carrier(pool,types,carrierState,MakeCarrierPostPopHooks(pool,post,
            [](Sid) { throw std::logic_error("ArcherType 소비자의 비전투 carrierCheck 조건 오류"); },
            [](float,float,std::uint8_t) { throw std::logic_error("ArcherType 소비자의 비로딩 지면 소유자 조건 오류"); }));
        AssetPostPopState assetState;AssetPostPopHooks assetHooks;
        assetHooks.createGunProcess=[](Sid) { throw std::logic_error("ArcherType 소비자는 비전투 등록 후 같은 Host에서 Gun을 생성한다"); };
        assetHooks.removeAnimation=[](Sid) { throw std::logic_error("ArcherType 소비자의 비전투 애니메이션 제거 조건 오류"); };
        RawAssetPostPop assetPost(pool,types,post,carrier,assetState,std::move(assetHooks));post.SetAssetPostPop(&assetPost);
        SquidPop pop(pool,hash,spots,&display,&post);'''


def expected_test():
    """이전 전체 검사에 명시 include/후처리 연결/깊이와집계 검사만 추가한다."""
    old=(HISTORY/TEST).read_bytes();newline=b'\r\n' if b'\r\n' in old else b'\n'
    include=b'#include "o/RawGunTarget.h"'+newline;new_include=include+b'#include "o/RawAssetPostPop.h"'+newline
    segment=b'SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);'
    suffix=b'==RawPopResult::Registered);auto rawTarget=pool.AllocatedBytes(target);'
    new_suffix=b'==RawPopResult::Registered);CHECK(bookkeeping.globalCounts[71]==1 && bookkeeping.depth==0);auto rawTarget=pool.AllocatedBytes(target);'
    if old.count(include)!=1 or old.count(segment)!=1 or old.count(suffix)!=1:raise RuntimeError('ArcherType 이전 Pop 검사 교정범위 오류')
    return old.replace(include,new_include).replace(segment,POP_ADDITION.encode('utf-8').replace(b'\n',newline)).replace(suffix,new_suffix)


def neutralize_test(current):
    """현재 검사의 전체 바이트가 명시 교정과 같은지 확인한 뒤 이전 전체를 반환한다."""
    if current!=expected_test():raise RuntimeError('ArcherType 실제 Pop 전체검사 교정범위 오류')
    return (HISTORY/TEST).read_bytes()


def update_report():
    """동일42/접두2를 그대로두고 현재 실제소비자/source의 SHA와 추가native0을 연결한다."""
    old=json.loads((HISTORY/REPORT.relative_to(ROOT)).read_text(encoding='utf-8'));report=copy.deepcopy(old)
    report['files'][TEST]=digest(ROOT/TEST)
    # 새로 연결한 기존 몸체의 전체 바이트이며 신규 native 관찰은 아니다.
    for relative in NEW_FILES:report['files'][relative]=digest(ROOT/relative)
    report['pop_lineage']=dict(prior_full_report_sha256=digest(HISTORY/REPORT.relative_to(ROOT)),
        prior_draft_report_sha256=digest(HISTORY/DRAFT.relative_to(ROOT)),normal_preserved=42,prefix_preserved=2,new_native=0,new_unique=0,
        reason='실제71 Pop의누락된Asset→Damageable→공통postPop을비전투등록에연결. 같은Host의Gun명시생성/첫Fire,생산/42fixture/원본tool불변.')
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    return old


def generate():
    """이전 전체자료를 보존한 채 현재 후처리 연결의 정확한 역변환과 SHA를 기록한다."""
    neutralize_test((ROOT/TEST).read_bytes());update_report()
    from analyze_archertype_draft_lineage import generate as generate_draft
    generate_draft()
    paths=[path for path in HISTORY.rglob('*') if path.is_file()]
    LINEAGE.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',normal_preserved=42,prefix_preserved=2,new_native=0,new_unique=0,
        history_files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
        current_files={relative:digest(ROOT/relative) for relative in (TEST,REPORT.relative_to(ROOT).as_posix(),DRAFT.relative_to(ROOT).as_posix(),*NEW_FILES)},
        tool_sha256=digest(__file__)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """최초 실패자료 전체/정확한 검사 교정/기존body와42+접두2 불변을 각각 검사한다."""
    neutralize_test((ROOT/TEST).read_bytes());lineage=json.loads(LINEAGE.read_text(encoding='utf-8'));old=json.loads((HISTORY/REPORT.relative_to(ROOT)).read_text(encoding='utf-8'));current=json.loads(REPORT.read_text(encoding='utf-8'))
    for relative,sha in old['files'].items():
        source=HISTORY/relative
        if source.exists() and digest(source)!=sha:raise RuntimeError('ArcherType 최초실패 전체기존근거 오류 '+relative)
    projected=copy.deepcopy(current);meta=projected.pop('pop_lineage');projected['files'][TEST]=old['files'][TEST]
    # 새dependency만 제거하면 당시 native 전체보고서와 같아야 한다.
    for relative in NEW_FILES:projected['files'].pop(relative)
    if projected!=old or meta['prior_full_report_sha256']!=digest(HISTORY/REPORT.relative_to(ROOT)) or meta['prior_draft_report_sha256']!=digest(HISTORY/DRAFT.relative_to(ROOT)) or meta['new_native'] or meta['new_unique']:raise RuntimeError('ArcherType Pop 전체이전보고서 연결 오류')
    # 당시 모든자료와현재source/검사/보고서의전체원문SHA를검사한다.
    for group in ('history_files','current_files'):
        for relative,sha in lineage[group].items():
            if digest(ROOT/relative)!=sha:raise RuntimeError('ArcherType Pop lineage SHA오류 '+relative)
    if lineage['tool_sha256']!=digest(__file__) or lineage['new_native'] or lineage['new_unique']:raise RuntimeError('ArcherType Pop lineage 도구/횟수 오류')
    print(json.dumps(dict(verified=True,normal_preserved=42,prefix_preserved=2,new_native=0,new_unique=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
