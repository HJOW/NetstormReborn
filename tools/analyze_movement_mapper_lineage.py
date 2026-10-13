#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 이동 선택 몸체의 공유 함수 이동과718개 회귀 입력의 이전2872개 포함 관계를 감사한다."""
import argparse
import json
from pathlib import Path
from analyze_pathruntime_lineage import ROOT, digest, git, tokens

# 기존 native 자료와 새 공유 함수 이동의 별도 근거다. 이전 native 보고서를 수정하지 않는다.
REPORT=ROOT/'cpppj/movement-mapper-lineage.json'
PRIOR=ROOT/'cpppj/priestworldevents.json'
HEADER='cpppj/src/client/RawPriestWorldBindings.h'
SOURCE='cpppj/src/client/RawPriestWorldBindings.cpp'
PRIOR_FIXTURE=ROOT/'cpppj/tests/fixtures/priestworldevents-x86.tsv'
REGRESSION=ROOT/'cpppj/tests/fixtures/pathprocesstype-x86.tsv'


def section(text,marker,end_marker):
    """정확한 함수 머리와 닫힘 경계로 검토된 한 몸체만 읽는다."""
    first=text.index(marker);last=text.index(end_marker,first)+len(end_marker)
    return first,last,text[first:last]


def inspect(commit,native_commit):
    """기존 전체 소스 보존·인자 이름 치환 뒤 몸체 토큰 동일성·718개 재사용을 확인한다."""
    prior=json.loads(PRIOR.read_text(encoding='utf-8'));result={};texts={}
    assert prior['cases']==2872
    # 이전 native source SHA를 고정 git 객체의 원래 LF/CRLF 바이트와 직접 대응한다.
    for name in (HEADER,SOURCE):
        baseline=git('show',commit+':'+name);current=(ROOT/name).read_bytes();old=baseline.decode('utf-8').replace('\r\n','\n');now=current.decode('utf-8').replace('\r\n','\n')
        native_blob=git('show',native_commit+':'+name);native_text=native_blob.decode('utf-8').replace('\r\n','\n')
        assert tokens(native_text)==tokens(old),name
        expected=prior['files'][name];variants={'git_blob':native_blob,'lf':native_text.encode('utf-8'),'crlf':native_text.replace('\n','\r\n').encode('utf-8')}
        variant=next((key for key,data in variants.items() if digest(data)==expected),None);assert variant,name
        result[name]=dict(native_sha256=expected,native_line_endings=variant,native_git_blob_sha256=digest(native_blob),baseline_git_blob_sha256=digest(baseline),
            baseline_tokens_sha256=digest(tokens(old)),unchanged_tokens_since_native=True,current_sha256=digest(current));texts[name]=(old,now)
    old,now=texts[HEADER];first=now.index('std::uint32_t SelectMovementProcessType(');start=now.rfind('\n',0,first-1)+1;end=now.index(';',first)+2
    restored=now[:start]+now[end:];assert restored==old;result[HEADER]['unchanged_existing_source']=True
    old,now=texts[SOURCE];marker='    std::uint32_t MovementProcessType(std::uint32_t type) const {'
    _,_,baseline_method=section(old,marker,'\n    }\n');first,last,wrapper=section(now,marker,'\n    }\n')
    assert tokens(wrapper)==tokens(marker+'\nreturn SelectMovementProcessType(core.pool.Edition(),core.types,state.movement,type);\n}\n')
    wrapped=now[:first]+baseline_method+now[last:];first,last,shared=section(wrapped,'std::uint32_t SelectMovementProcessType(','\n}\n');start=wrapped.rfind('\n',0,first-1)+1
    assert wrapped[:start]+wrapped[last:]==old
    old_body=baseline_method[baseline_method.index('{')+1:baseline_method.rfind('}')]
    old_body=old_body.replace('const auto& selection=state.movement;','').replace('core.pool.Edition()','edition').replace('core.types','types')
    new_body=shared[shared.index('{')+1:shared.rfind('}')];assert tokens(old_body)==tokens(new_body)
    result[SOURCE].update(unchanged_other_source=True,moved_body_token_sha256=digest(tokens(new_body)),bindings=['state.movement -> selection 인자','core.pool.Edition() -> edition 인자','core.types -> types span 인자'])
    previous={}
    # 기본 전역(profile0/processShift0)의718개는 이미 검증된2872개 입력의 정확한 부분집합이다.
    for line in PRIOR_FIXTURE.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):continue
        row=line.split('\t')
        if row[1:3]==['0','0']:previous[(row[0],row[4],row[3])]=tuple(row[5:7])
    count=0
    # 새 파일의 각 회귀 결과가 이전 보호 PE 관찰과 같음을 확인한다.
    for line in REGRESSION.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):continue
        row=line.split('\t');assert previous[tuple(row[:3])]==tuple(row[3:5]);count+=1
    assert len(previous)==count==718
    return result


def generate():
    """기준 commit과 이전2872개 보고서/fixture 및 현재718개 회귀를 고정한다."""
    commit=git('rev-parse','HEAD').decode('ascii').strip();prior=json.loads(PRIOR.read_text(encoding='utf-8'));native_commit=None
    # 주석 보완 이전 실제 source SHA를 가진 git 객체를 찾고 두 파일 모두 같은 commit인지 확인한다.
    for candidate in git('log','--format=%H','--',HEADER).decode('ascii').split():
        if all(digest(git('show',candidate+':'+name))==prior['files'][name] for name in (HEADER,SOURCE)):native_commit=candidate;break
    assert native_commit
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',baseline_commit=commit,native_source_commit=native_commit,prior_unique_cases=2872,regression_cases=718,new_unique_cases=0,
        files=inspect(commit,native_commit),artifacts={p.relative_to(ROOT).as_posix():digest(p.read_bytes()) for p in (PRIOR,PRIOR_FIXTURE,REGRESSION,Path(__file__),ROOT/'tools/analyze_pathruntime_lineage.py')}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 공유 몸체/기존 wrapper/고정 소스·회귀 포함 관계를 다시 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['prior_unique_cases']==2872 and report['regression_cases']==718 and report['new_unique_cases']==0
    for name,sha in report['artifacts'].items():assert digest((ROOT/name).read_bytes())==sha,name
    assert inspect(report['baseline_commit'],report['native_source_commit'])==report['files']
    print('이동 mapper 기존 몸체 토큰/다른 전체 소스 보존·이전2872개 중718개 회귀 lineage 확인')


# 명시 호출에서만 lineage 생성/감사를 수행한다. 게임과 PE를 실행하지 않는다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');arguments=parser.parse_args();verify() if arguments.verify else generate()
