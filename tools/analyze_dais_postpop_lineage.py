#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스 최초 Pop 등록을 기준11의 전체 기존 본문과 이전 원본 관찰에 연결한다."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 같은 공유 저장소의 절대 기준이다.
REPORT=ROOT/'cpppj/dais-postpop-lineage.json' # 기존11/수명보고서를 덮어쓰지 않는 새 등록 근거다.
BASELINE=ROOT/'cpppj/hist/dais-postpop-a6aad' # 기준11 Git LF와 작업 CRLF 원문을 함께 보존한다.
NATIVE=ROOT/'cpppj/hist/dais-lifecyc-dd023' # 최초1928 관찰의 현재파일당시 SHA를 보존한다.
SOURCES=('cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp','cpppj/src/o/RawDaisLifecycle.h','cpppj/src/o/RawDaisLifecycle.cpp') # 승인된 최소 등록/검사 네 파일만 다룬다.
PRIOR=('cpppj/run11.json','cpppj/daisregular-core.json','cpppj/altarsacrifice.json','cpppj/altarsacrifice-successor.json','cpppj/postpop.json','cpppj/runepostpop.json','cpppj/priestpostpoptail.json','cpppj/assetpostpop.json','cpppj/factorypostpop.json') # 기존 관찰을 새 수명 입력에 더하지 않는다.

def sha(body):
    """파일 전체 또는 기존 본문 토큰의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()

def git(*args):
    """기준 객체를 읽기만 하며 커밋/푸시를 실행하지 않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout

def tokens(body):
    """문자열을 보존하고 주석과 공백만 뺀 전체 기존 토큰을 반환한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',body)
    return '\0'.join(value for value in parts if not value.startswith(('//','/*'))).encode('utf-8')

def cut(body,start,end):
    """새 선언/정의의 고유 경계 사이만 제거한다."""
    assert body.count(start)==body.count(end)==1
    left=body.index(start);right=body.index(end,left);return body[:left]+body[right:]

def restore(name,body):
    """새 등록/검사만 역제거해 기준11의 전체 소스를 정확히 되살린다."""
    if name.endswith('SquidPostPop.h'):
        prefixes=('class RawDaisLifecycle;','    // 다이스/제단의 전체 최초 Pop','    void SetDaisPostPop(','    // 실제 다이스/제단 타입과 생성자','    bool HandlesDais(','    const RawDaisLifecycle* daisPostPop_')
        lines=body.splitlines(keepends=True);removed=[line for line in lines if line.startswith(prefixes)];assert len(removed)==6
        return ''.join(line for line in lines if line not in removed)
    if name.endswith('RawDaisLifecycle.h'):
        lines=body.splitlines(keepends=True);removed=[line for line in lines if line.startswith('    void Validate(Sid sid,')];assert len(removed)==1
        return ''.join(line for line in lines if line not in removed)
    if name.endswith('RawDaisLifecycle.cpp'):
        return cut(body,'// 실제 최초 Pop의 타입과 초기 Player 범위를','// 로딩초기화에서는setter')
    new_include='#include "o/RawDaisLifecycle.h"\n';new_handle=' || HandlesDais(sid)';new_validate='    if (HandlesDais(sid)) daisPostPop_->Validate(sid,flags);\n'
    assert body.count(new_include)==body.count(new_handle)==body.count(new_validate)==1
    body=body.replace(new_include,'').replace(new_handle,'').replace(new_validate,'')
    body=cut(body,'// 실제 다이스/제단 수명 전체몸체를','// 복원한 전용 자산 몸체를')
    return cut(body,'    // 다이스/제단은 자기 전체 몸체 안에서','    // Rune은304예약')

def observe(commit):
    """전체 기존 소스/토큰과 당시 보고서·독립 수명 입력의 불변을 모두 확인한다."""
    sources={};prior={};native={};preserved={}
    # 네 기존 파일의 어느 함수/상수/장부도 새 등록 이외 변경되지 않아야 한다.
    for name in SOURCES:
        git_body=git('show',commit+':'+name);saved_git=(BASELINE/'git'/name).read_bytes();saved_work=(BASELINE/'work'/name).read_bytes();assert git_body==saved_git
        baseline=git_body.decode('utf-8').replace('\r\n','\n');assert saved_work.decode('utf-8').replace('\r\n','\n')==baseline
        current=(ROOT/name).read_text(encoding='utf-8');existing=restore(name,current);assert existing==baseline and tokens(existing)==tokens(baseline),name
        sources[name]=dict(baseline_git_sha256=sha(git_body),baseline_work_sha256=sha(saved_work),current_sha256=sha((ROOT/name).read_bytes()),entire_existing_source_unchanged=True,entire_existing_token_sha256=sha(tokens(existing)))
    # 기준11 통합/전용 보고서의 당시 JSON 원문을 그대로 유지한다.
    for name in PRIOR:
        path=ROOT/name;old=git('show',commit+':'+name);assert path.read_bytes()==old,name
        prior[name]=dict(sha256=sha(old),new_native_inputs=0)
    old_report='cpppj/daislifecycle.json';old=(NATIVE/old_report).read_bytes();assert (ROOT/old_report).read_bytes()==old
    data=json.loads(old.decode('utf-8'));assert data['total']==1928 and not data['smoke'] and not data['game_runs']
    # 당시 보고서의 모든 원본/도구/fixture/소스 출처를 확인한다. 새 API 두 파일만 보존 원문으로 연결한다.
    for name,value in data['files'].items():
        path=NATIVE/name if (NATIVE/name).exists() else ROOT/name;assert sha(path.read_bytes())==value,name
        native[name]=dict(reported_sha256=value,current_or_preserved_path=path.relative_to(ROOT).as_posix())
    # 다른 PC에서도 옛 source/fixture/report/tool/test/spec를 재검사할 수 있도록 전체 역사 파일을 SHA로 묶는다.
    for directory in (BASELINE,NATIVE):
        for path in sorted(directory.rglob('*')):
            if path.is_file():preserved[path.relative_to(ROOT).as_posix()]=sha(path.read_bytes())
    return dict(sources=sources,prior_reports=prior,native1928=dict(total=1928,new_native_inputs=0,report_sha256=sha(old),dependencies=native),history=preserved)

def main(check):
    """기준11 전체 본문 보존과 현재 등록 API SHA를 별도 successor로 기록한다."""
    commit=git('rev-parse','5bdb5be').decode('ascii').strip();data=dict(host='HJOW-Athlon',decompile_date='2026-10-14',baseline_commit=commit,lineage=observe(commit),tool_sha256=sha(Path(__file__).read_bytes()),additional_unique_cases=0,scope='SquidPostPop의 같은Pool 등록·현재VT 분배/검사/실행과 RawDaisLifecycle의 새 prefix Validate만 추가. 기준11 전체 기존 본문·관찰 및 최초 수명1928의 당시 원문을 보존한다.')
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 실제 Pop 등록: 기준11 전체 기존 본문/토큰·기존 관찰·수명1928 보존, 새 원본0')

# 명시 CLI 요청에서만 별도 lineage를 작성하거나 현재 연결을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
