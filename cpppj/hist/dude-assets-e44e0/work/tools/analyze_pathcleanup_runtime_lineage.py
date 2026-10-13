#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Cleanup의 helper API 두 개를 뺀 Runtime 전체 소스가 8abd00a와 같은지 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

# 앞 단위 원본/소스 근거는 그대로 읽으며 별도 새 lineage만 기록한다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/pathcleanup-runtime-lineage.json'
SOURCES=('cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp')
PRIOR=('cpppj/pathruntime.json','cpppj/pathruntime-lineage.json','cpppj/pathstart.json')


def sha(data):
    """정확한 바이트/정규화 토큰의 SHA256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """고정 git 객체만 읽으며 commit/push는 실행하지 않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(text):
    """문자열 리터럴은 보존하고 주석/공백만 빼서 모든 C++ 토큰을 비교한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text)
    return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def remove_helpers(name,text):
    """새 Helper/ClearHelper 선언/정의와 각 설명 한 줄만 제거한다."""
    lines=text.splitlines(keepends=True);removed=set()
    patterns=('std::uint32_t Helper() const;','void ClearHelper();') if name.endswith('.h') else ('std::uint32_t RawPathRuntime::Helper() const','void RawPathRuntime::ClearHelper()')
    # 각 새 API의 정확한 줄만 제거하며 SetCommandFields/ReplacePoint를 포함한 기존 본문은 남긴다.
    for index,line in enumerate(lines):
        if any(line.lstrip().startswith(pattern) for pattern in patterns):
            assert index and lines[index-1].lstrip().startswith('//');removed.update((index-1,index))
    assert len(removed)==4
    return ''.join(line for index,line in enumerate(lines) if index not in removed)


def observe(commit):
    """고정 앞 commit의 전체 소스/토큰과 이전 보고서 SHA의 연결을 기록한다."""
    prior=json.loads((ROOT/PRIOR[2]).read_text(encoding='utf-8'));result={}
    # 두 Runtime 파일의 모든 기존 코드와 설명이 앞 단위 상태와 같아야 한다.
    for name in SOURCES:
        baseline=git('show',commit+':'+name);old=baseline.decode('utf-8').replace('\r\n','\n');current=(ROOT/name).read_bytes();now=current.decode('utf-8').replace('\r\n','\n');existing=remove_helpers(name,now)
        assert existing==old and tokens(existing)==tokens(old),name
        assert 'SetCommandFields' in existing and 'ReplacePoint' in existing
        variants={'git_blob':baseline,'lf':old.encode('utf-8'),'crlf':old.replace('\n','\r\n').encode('utf-8')};expected=prior['files'][name]
        match=next((kind for kind,data in variants.items() if sha(data)==expected),None);assert match,name
        result[name]=dict(baseline_blob_sha256=sha(baseline),prior_pathstart_sha256=expected,prior_line_endings=match,current_sha256=sha(current),existing_normalized_sha256=sha(existing.encode('utf-8')),existing_token_sha256=sha(tokens(existing)),unchanged_existing_source=True)
    return result


def generate():
    """완료 단위8abd00a를 고정하고 이전 세 보고서를 수정 없이 참조한다."""
    commit=git('rev-parse','8abd00a').decode('ascii').strip();result=observe(commit)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',baseline_commit=commit,additions=['Helper','ClearHelper'],files=result,prior_reports={name:sha((ROOT/name).read_bytes()) for name in PRIOR},tool_sha256=sha(Path(__file__).read_bytes()),new_unique_cases=0,
        scope='새 helper 조회/0쓰기 API와 설명만 제거하면 8abd00a 전체 소스/토큰과 일치한다. 이전 Runtime360/Start512를 새 관찰에 더하지 않는다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """앞 근거 보존·현재 SHA·기존 모든 본문/토큰의 lineage를 다시 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['new_unique_cases']==0 and report['additions']==['Helper','ClearHelper']
    assert report['files']==observe(report['baseline_commit'])
    assert report['prior_reports']=={name:sha((ROOT/name).read_bytes()) for name in PRIOR}
    assert report['tool_sha256']==sha(Path(__file__).read_bytes())
    print('Runtime 기존8abd00a 전체 소스/토큰2파일·이전3보고서 보존 확인')


# 명시 호출에서만 새 보고서를 생성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
