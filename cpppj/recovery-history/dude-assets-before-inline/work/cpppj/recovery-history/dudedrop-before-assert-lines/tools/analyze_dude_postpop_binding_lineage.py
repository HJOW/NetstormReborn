#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 최초 Pop 분배와 발자국0을 기준12 전체 본문·기존288관찰에 연결한다."""
import argparse
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path

# 공유 저장소와 기준12의 Git/작업 바이트 및 새 연결 보고서 경로다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/recovery-history/dude-postpop-baseline'
REPORT=ROOT/'cpppj/recovery-dude-postpop-binding-lineage-evidence.json'
COMMIT='db9b626c0a9c6b9440c6b181dfc401506a07d3e4'
NATIVE='cpppj/recovery-dudespawn-evidence.json'
DRAFT='cpppj/recovery-dudespawn-draft-lineage-evidence.json'
# 기존 공통 몸체에 추가한 선언/분배/빈 순회 검사만 정확히 되돌린다.
HEADER_LINES=(
    'class RawDudePostPop; // Dude70의 실제 idle/population/I 프레임·공통후처리를 연결한다.\n',
    '    // Dude의 실제 최초 등록 전체 몸체를 같은 풀에 등록하며 nullptr은 연결을 해제한다.\n',
    '    void SetDudePostPop(const RawDudePostPop* postPop);\n',
    '    // 현재 Dude 타입과 생성자 가상 표가 등록된 전체 몸체와 일치하는가.\n',
    '    bool HandlesDude(Sid sid) const;\n',
    '    const RawDudePostPop* dudePostPop_{}; // idle/population/I 프레임 뒤 같은 공통 깊이를 한 번 줄이는 전체 몸체다.\n',
)
REGISTRATION='''// 실제 Dude 후처리의 전체 몸체를 같은 풀에만 명시 등록한다.
void SquidPostPop::SetDudePostPop(const RawDudePostPop* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Dude Pop/후처리 풀 불일치");
    dudePostPop_=postPop;
}
// 현재 타입과 생성자 가상 표의 판단은 등록된 실제 몸체를 재사용한다.
bool SquidPostPop::HandlesDude(Sid sid) const { return dudePostPop_ && dudePostPop_->Handles(sid); }
'''
ACTIVATION='''    // Dude는 idle/population/I 프레임 뒤 자기 전체 몸체에서 공통 장부를 한 번 호출한다.
    if (HandlesDude(sid)) {
        dudePostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Dude Activate 공통 깊이 불일치");
        return;
    }
'''
EMPTY_COMMENT='        // 실제 Dude의 발자국0은 left/top이 끝 칸을 넘어 아래 y/x 루프를 빈 순회로 만든다.\n'


def sha(body):
    """전체 바이트 또는 전체 본문 토큰의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def git(name):
    """기준12 파일을 읽기만 하며 저장소를 수정하지 않는다."""
    return subprocess.run(['git','show',COMMIT+':'+name],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(body):
    """문자열을 유지하고 주석/공백만 제외한 전체 기존 토큰을 반환한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',body)
    return '\0'.join(value for value in parts if not value.startswith(('//','/*'))).encode('utf-8')


def once(body,new,old=''):
    """고유한 새 조각 한 개만 정확히 이전 조각으로 바꾼다."""
    if body.count(new)!=1:raise RuntimeError('Dude 연결 역변환 위치 오류 '+new[:70])
    return body.replace(new,old,1)


def restore(name,body):
    """같은 풀 등록과 분배 및0크기 허용만 제거해 이전 전체 소스를 복원한다."""
    if name.endswith('SquidPostPop.h'):
        # 여섯 새 선언/설명/멤버 줄 이외의 전체 헤더는 보존한다.
        for line in HEADER_LINES:body=once(body,line)
    elif name.endswith('SquidPostPop.cpp'):
        # 새 include/판단/검사/실행을 각각 고유한 위치에서만 역제거한다.
        for item in ('#include "o/RawDudeSpawn.h"\n',' || HandlesDude(sid)',REGISTRATION,
                     '    if (HandlesDude(sid)) dudePostPop_->Validate(sid,flags);\n',ACTIVATION):body=once(body,item)
    elif name.endswith('SquidPop.cpp'):
        body=once(body,EMPTY_COMMENT)
        body=once(body,'type.footX<0 || type.footY<0','type.footX<1 || type.footY<1')
    else:raise RuntimeError('Dude 연결 범위 밖 소스 '+name)
    return body


def observe():
    """전체 기존 몸체·당시 보고서·288 fixture와 새 소비자 SHA를 분리해 감사한다."""
    capture=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    if capture['baseline_commit']!=COMMIT:raise RuntimeError('Dude 연결 기준12 오류')
    preserved={};sources={};prior={};dependencies={}
    # 보존한 전체156파일과104 Git 파일의 당시 바이트를 모두 확인한다.
    for name,item in capture['captured'].items():
        saved=HISTORY/'work'/name
        if sha(saved.read_bytes())!=item['work_sha256']:raise RuntimeError('Dude 이전 작업 원문 SHA 오류 '+name)
        preserved[saved.relative_to(ROOT).as_posix()]=item['work_sha256']
        if 'git_sha256' in item:
            saved_git=HISTORY/'git'/name;body=saved_git.read_bytes()
            if sha(body)!=item['git_sha256'] or git(name)!=body:raise RuntimeError('Dude 기준12 Git 원문 오류 '+name)
            preserved[saved_git.relative_to(ROOT).as_posix()]=item['git_sha256']
    preserved[(HISTORY/'capture.json').relative_to(ROOT).as_posix()]=sha((HISTORY/'capture.json').read_bytes())
    # 새 연결만 제거한 전체 본문은 Git LF와 이전 작업 CRLF 원문 양쪽으로 복원돼야 한다.
    for name in capture['shared']:
        current=(ROOT/name).read_text(encoding='utf-8');old=(HISTORY/'work'/name).read_bytes();git_body=(HISTORY/'git'/name).read_bytes()
        restored=restore(name,current);normalized=old.decode('utf-8').replace('\r\n','\n')
        if restored!=normalized or restored!=git_body.decode('utf-8').replace('\r\n','\n'):raise RuntimeError('Dude 전체 기존 본문 역변환 오류 '+name)
        restored_work=restored.replace('\n','\r\n').encode('utf-8') if b'\r\n' in old else restored.encode('utf-8')
        if restored_work!=old:raise RuntimeError('Dude 작업 바이트 전체 역변환 오류 '+name)
        sources[name]=dict(baseline_git_sha256=sha(git_body),baseline_work_sha256=sha(old),current_sha256=sha((ROOT/name).read_bytes()),whole_old_source_inverse=True,whole_old_work_bytes_inverse=True,whole_old_token_sha256=sha(tokens(restored)))
    # 기존27보고서의 당시 전체 JSON과39 fixture를 현재에도 그대로 유지한다.
    for name in capture['prior_reports']:
        old=(HISTORY/'work'/name).read_bytes()
        if (ROOT/name).read_bytes()!=old:raise RuntimeError('Dude 기존 보고서 변경 '+name)
        prior[name]=dict(sha256=sha(old),new_native_inputs=0)
    for name,item in capture['captured'].items():
        if 'fixtures/' in name and not name.startswith('cpppj/recovery-history/') and sha((ROOT/name).read_bytes())!=item['work_sha256']:raise RuntimeError('Dude 기존 fixture 변경 '+name)
    native=json.loads((HISTORY/'work'/NATIVE).read_text(encoding='utf-8'))
    if native['total']!=288 or native['boundaries'] or native['os_calls']:raise RuntimeError('Dude 이전 정상/효과 경계 오류')
    # 최초288보고서가 참조한 소스는 당시 보존 원문으로, 보호 PE/TAFF/SHP는 현재 동일 바이트로 검증한다.
    for name,expected in native['files'].items():
        saved=HISTORY/'work'/name;path=saved if saved.exists() else ROOT/name
        if sha(path.read_bytes())!=expected:raise RuntimeError('Dude 이전288 출처 SHA 오류 '+name)
        if name not in capture['shared'] and (ROOT/name).read_bytes()!=path.read_bytes():raise RuntimeError('Dude 소비자/원본 변경 '+name)
        dependencies[name]=dict(reported_sha256=expected,preserved_or_current_path=path.relative_to(ROOT).as_posix())
    rows=[row for row in csv.reader((ROOT/'cpppj/tests/fixtures/dudespawn-x86.tsv').open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=288:raise RuntimeError('Dude 같은288 격자 오류')
    # 두 판본의 실제 정상 반환144/출생36과 독립 입력7열의 고유성을 보존한다.
    for edition,data in native['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=144 or len({tuple(row[1:8]) for row in selected})!=144 or data['assertions'] or data['births']!=36:raise RuntimeError('Dude 판본별 정상/고유 입력 오류')
    names=tuple(capture['shared'])+('cpppj/src/o/RawDudeSpawn.h','cpppj/src/o/RawDudeSpawn.cpp','cpppj/tests/DudeSpawnTests.cpp','tools/decomp_dudespawn_oracle.py',NATIVE,DRAFT,'tools/analyze_dudespawn_draft_lineage.py','cpppj/tests/fixtures/dudespawn-x86.tsv')
    current={name:sha((ROOT/name).read_bytes()) for name in names}
    return dict(sources=sources,prior_reports=prior,history=preserved,native288=dict(total=288,new_native_inputs=0,report_sha256=sha((ROOT/NATIVE).read_bytes()),dependencies=dependencies),files=current)


def main(check):
    """기준12 정확 역변환과 현재 Dude 소비자를 별도 successor로 기록한다."""
    data=dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',primary_target='10.78',cd_target='10.72',baseline_commit=COMMIT,new_native_inputs=0,cpp_tests_completed=0,lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),scope='SquidPostPop의 같은 풀 Dude 등록/실제 타입·가상표 판단/사전검사/전체 Activate만 추가하고 SquidPop의 foot0은 원래 y/x 루프의 빈 순회로 허용한다. 기존 전체 본문과 당시288·draft보고서는 불변이다. 실제 이동/Unpop·DudePath/Drop 소비자 대조는 별도 후속이다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('Dude 연결 현재 successor SHA 오류')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,baseline_commit=COMMIT,native_inputs=288,new_native_inputs=0,preserved_files=len(data['lineage']['history']),prior_reports=len(data['lineage']['prior_reports']),cpp_completed=False),ensure_ascii=False))


# 명시 CLI 요청에서만 새 연결 근거를 작성하거나 이미 고정된 SHA를 검증한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
