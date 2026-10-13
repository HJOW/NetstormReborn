#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude414 최초 C++ 실패의 전체자료를 보존하고 x발자국/검사정수폭 최소교정을 감사한다."""
import argparse
import copy
import json
from decomp_owner_oracle import ROOT,digest

# 최초 실제C++대조 직전/실패 때 전체source·414fixture·report·tool·기존lineage·로그다.
HISTORY=ROOT/'cpppj/recovery-history/dudenotice-before-point-correction'
REPORT=ROOT/'cpppj/recovery-dudenotice-evidence.json'
DRAFT=ROOT/'cpppj/recovery-dudenotice-draft-lineage-evidence.json'
LINEAGE=ROOT/'cpppj/recovery-dudenotice-point-lineage-evidence.json'
CPP='cpppj/src/o/RawDudeNotice.cpp'
TEST='cpppj/tests/DudeNoticeTests.cpp'
# decompiler가누락한fild footX→fmul0.5→fsubr x→ftol을그대로반영한다.
POINT_OLD='''    if (!(type.flags2&state_.raisedGenusMask)) y-=0.5f;else if (type.flags2&0x10040000) y+=1.0f;return {static_cast<float>(static_cast<int>(x))+0.5f,y};'''
POINT_NEW='''    const double entranceX=static_cast<double>(x)-static_cast<double>(type.footX)*0.5;
    if (!(type.flags2&state_.raisedGenusMask)) y-=0.5f;else if (type.flags2&0x10040000) y+=1.0f;return {static_cast<float>(static_cast<int>(entranceX))+0.5f,y};'''
# 원본입력의값은동일하며 MSVC tuple생성의암묵정수축소경고만없앤다.
TUPLE_OLD='''for (const auto [sid,type,x,y,owner]:{std::tuple<std::uint16_t,std::uint8_t,float,float,std::uint8_t>{50,70,20.75f,21.9f,1},{60,89,24.75f,24.5f,static_cast<std::uint8_t>(Number(row[6])==2 ? 2 : 1)},{61,89,16.75f,19.3f,1}})'''
TUPLE_NEW='''for (const auto [sid,type,x,y,owner]:{std::tuple{std::uint16_t{50},std::uint8_t{70},20.75f,21.9f,std::uint8_t{1}},std::tuple{std::uint16_t{60},std::uint8_t{89},24.75f,24.5f,static_cast<std::uint8_t>(Number(row[6])==2 ? 2 : 1)},std::tuple{std::uint16_t{61},std::uint8_t{89},16.75f,19.3f,std::uint8_t{1}}})'''


def transform(relative,value):
    """이름교정 후 기존전체몸체에 실제 x/입력정수폭 교정만 적용한다."""
    newline=b'\r\n' if b'\r\n' in value else b'\n'
    changes=((POINT_OLD,POINT_NEW),('일반Gun center와달리x 정수+0.5','일반Gun center와달리x−footX*0.5 절삭+0.5')) if relative==CPP else ((TUPLE_OLD,TUPLE_NEW),) if relative==TEST else ()
    # 각정확한이전본문은한번만있어야 하며 다른소스는바꾸지않는다.
    for before,after in changes:
        before=before.encode('utf-8').replace(b'\n',newline);after=after.encode('utf-8').replace(b'\n',newline)
        if value.count(before)!=1:raise RuntimeError('Dude x/tuple 정확한이전본문 오류 '+relative)
        value=value.replace(before,after)
    return value


def metadata():
    """동일414행과전체이전보고서 출처 및 실제기존endpoint 분기의한계를기록한다."""
    return dict(prior_full_report_sha256=digest(HISTORY/REPORT.relative_to(ROOT)),prior_draft_report_sha256=digest(HISTORY/DRAFT.relative_to(ROOT)),normal_preserved=414,new_native=0,new_unique=0,
        reason='PATCH004505e2/CD004e8a75의x−footX*0.5절삭+0.5를복원하고검사tuple의정수폭만명시한다. 기존414fixture와native도구는불변.',
        endpoint_scope='기존path1endpoint25/22와path2endpoint100/100은실제Snap24/22와둘다다르다. 이414행은같은endpoint 자연반환의원본관찰을포함하지않는다.')


def generate():
    """같은독립원본결과를그대로보존하며 두현재소스/전체역변환을연결한다."""
    for relative in (CPP,TEST):
        if (ROOT/relative).read_bytes()!=transform(relative,(HISTORY/relative).read_bytes()):raise RuntimeError('Dude 현재최소교정 범위 오류 '+relative)
    from analyze_dudenotice_draft_lineage import generate as generate_draft
    generate_draft()
    history=[path for path in HISTORY.rglob('*') if path.is_file()]
    current=(CPP,TEST,REPORT.relative_to(ROOT).as_posix(),DRAFT.relative_to(ROOT).as_posix(),'tools/analyze_dudenotice_draft_lineage.py','tools/decomp_dudenotice_oracle.py','cpppj/tests/fixtures/dudenotice-x86.tsv')
    LINEAGE.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',normal_preserved=414,new_native=0,new_unique=0,
        history_files={path.relative_to(ROOT).as_posix():digest(path) for path in history},current_files={relative:digest(ROOT/relative) for relative in current},tool_sha256=digest(__file__)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """최소교정과전체이전몸체/report/tool/기존lineage 및원본414행 불변을감사한다."""
    old=json.loads((HISTORY/REPORT.relative_to(ROOT)).read_text(encoding='utf-8'));current=json.loads(REPORT.read_text(encoding='utf-8'));lineage=json.loads(LINEAGE.read_text(encoding='utf-8'))
    for relative in (CPP,TEST):
        if (ROOT/relative).read_bytes()!=transform(relative,(HISTORY/relative).read_bytes()):raise RuntimeError('Dude 최소소스교정 역변환 오류 '+relative)
    projected=copy.deepcopy(current);meta=projected.pop('point_lineage')
    for relative in (CPP,TEST):projected['files'][relative]=old['files'][relative]
    if projected!=old or meta!=metadata():raise RuntimeError('Dude 414전체보고서/관찰 불변 오류')
    for relative,sha in old['files'].items():
        before=HISTORY/relative
        if before.exists() and digest(before)!=sha:raise RuntimeError('Dude 이전전체근거SHA 오류 '+relative)
    # 최초실패의전체원문과현재두소스/도구/414fixture/report를별도로감사한다.
    for group in ('history_files','current_files'):
        for relative,sha in lineage[group].items():
            if digest(ROOT/relative)!=sha:raise RuntimeError('Dude x lineage SHA 오류 '+relative)
    if lineage['tool_sha256']!=digest(__file__) or lineage['new_native'] or lineage['new_unique']:raise RuntimeError('Dude x lineage 도구/횟수 오류')
    print(json.dumps(dict(verified=True,normal_preserved=414,new_native=0,new_unique=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
