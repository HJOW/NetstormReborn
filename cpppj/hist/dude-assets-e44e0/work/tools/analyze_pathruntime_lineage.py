#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PathStart가 추가한 두 API를 제외한 RawPathRuntime의 기존 전체 코드 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

# 저장소와 새 lineage 기록이다. 보호 PE/이전 native 근거를 수정하지 않는다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/pathruntime-lineage.json'
NATIVE=ROOT/'cpppj/pathruntime.json'
SOURCES=('cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp')


def digest(data):
    """파일/코드 토큰의 정확한 바이트 SHA를 계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*arguments):
    """명시 git 인자를 배열로 읽는다. commit/push나 셸 문자열 조립을 하지 않는다."""
    return subprocess.run(['git',*arguments],cwd=ROOT,check=True,capture_output=True).stdout


def existing_source(name,text):
    """새 두 선언/정의와 그 직전 설명 주석만 지워 기존 전체 소스를 복원한다."""
    if name.endswith('.h'):
        lines=text.splitlines(keepends=True);remove=set()
        # 각 새 선언은 설명 한 줄과 함께 제거한다. 다른 멤버는 건드리지 않는다.
        for index,line in enumerate(lines):
            if 'void SetCommandFields(' in line or 'void ReplacePoint(' in line:
                assert index>0 and lines[index-1].lstrip().startswith('//');remove.update((index-1,index))
        assert len(remove)==4
        return ''.join(line for index,line in enumerate(lines) if index not in remove)
    first=text.index('void RawPathRuntime::SetCommandFields(');start=text.rfind('\n',0,first-1)+1
    following=text.index('void RawPathRuntime::SetAttachment(');end=text.rfind('\n',0,following-1)+1
    addition=text[start:end]
    assert addition.count('void RawPathRuntime::')==2 and 'void RawPathRuntime::ReplacePoint(' in addition
    return text[:start]+text[end:]


def tokens(text):
    """문자열 리터럴을 보존하고 주석·공백을 제외한 C++ 토큰을 순서대로 묶는다."""
    pattern=r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]'
    return '\x00'.join(token for token in re.findall(pattern,text) if not token.startswith(('//','/*'))).encode('utf-8')


def observations(commit):
    """기존 native source SHA와 git 객체를 연결하고 새 API를 뺀 전체 소스를 비교한다."""
    native=json.loads(NATIVE.read_text(encoding='utf-8'));output={}
    assert native['cases']==360
    # 두 파일의 모든 기존 선언/정의/상수/주석이 보존됐는지 함께 확인한다.
    for name in SOURCES:
        baseline=git('show',commit+':'+name);current=(ROOT/name).read_bytes()
        old=baseline.decode('utf-8').replace('\r\n','\n');now=current.decode('utf-8').replace('\r\n','\n');core=existing_source(name,now)
        assert core==old,name
        expected=native['files'][name];candidates={'git_blob':baseline,'lf':old.encode('utf-8'),'crlf':old.replace('\n','\r\n').encode('utf-8')}
        variant=next((key for key,data in candidates.items() if digest(data)==expected),None);assert variant,name
        assert tokens(core)==tokens(old)
        output[name]=dict(native_sha256=expected,native_line_endings=variant,baseline_git_blob_sha256=digest(baseline),current_sha256=digest(current),
            baseline_normalized_sha256=digest(old.encode('utf-8')),removed_additions_normalized_sha256=digest(core.encode('utf-8')),existing_token_sha256=digest(tokens(core)),unchanged_existing_source=True)
    return output


def generate():
    """현재 HEAD를 고정해 향후 작업 단위 commit 뒤에도 같은 lineage를 확인할 수 있게 한다."""
    commit=git('rev-parse','HEAD').decode('ascii').strip();files=observations(commit)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',baseline_commit=commit,prior_native_report_sha256=digest(NATIVE.read_bytes()),prior_unique_cases=360,
        new_unique_cases=0,additions=['SetCommandFields','ReplacePoint'],files=files,tool_sha256=digest(Path(__file__).read_bytes()),
        scope='두 새 선언/정의와 해당 설명 주석만 제거하면 기존 전체 소스가 고정 git 객체와 개행 정규화 후 일치한다. 기존360개를 신규 관찰에 합산하지 않는다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """현재 파일·이전 근거·고정 git 객체·모든 기존 코드 토큰의 대응을 다시 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    assert report['prior_unique_cases']==360 and report['new_unique_cases']==0 and report['additions']==['SetCommandFields','ReplacePoint']
    assert digest(NATIVE.read_bytes())==report['prior_native_report_sha256'] and digest(Path(__file__).read_bytes())==report['tool_sha256']
    assert observations(report['baseline_commit'])==report['files']
    print('RawPathRuntime 기존 전체 소스/토큰2파일 보존·이전360개 native SHA lineage 확인')


# 명시 호출에서만 새 lineage를 만들거나 현재 저장 기록을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');arguments=parser.parse_args();verify() if arguments.verify else generate()
