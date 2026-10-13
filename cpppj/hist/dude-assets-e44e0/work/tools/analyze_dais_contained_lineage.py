#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dais allowDead 선택 전달과 기준09 Carrier 전체 본문·기존 관찰의 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 같은 공유 저장소의 절대 기준이다.
REPORT=ROOT/'cpppj/dais-contained-lineage.json' # 기존 Carrier/09 보고서를 수정하지 않는 별도 연결 근거다.
SOURCES=('cpppj/src/o/RawCarrierPreDestroy.h','cpppj/src/o/RawCarrierPreDestroy.cpp') # 승인된 선택인수/Begin 전달만 바꾸는 두 파일이다.
PRIOR=('cpppj/run9.json','cpppj/carrierpredestroy.json','cpppj/contained.json') # 기존 정상/회귀/09 원본 입력은 새 Dais에 합산하지 않는다.
COMMENT='    // allowDead=true는 Dais wrapper의 원본 dead-parent assert 허용이다. 기존 호출은 기본false로 같은 동작을 유지한다.\n' # 새 설명 한줄을 역제거할 때 사용한다.


def sha(data):
    """전체 파일과 주석 제외 토큰의 SHA256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """git 기준 객체를 읽기만 하며 커밋/푸시를 하지 않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(text):
    """문자열 리터럴을 보존하고 주석/공백만 빼서 전체 기존 토큰을 비교한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text)
    return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def restore(name,text):
    """선택인수와 전달 및 설명만 되돌려 기존 전체 문자를 복구한다."""
    if name.endswith('.h'):
        assert text.count(COMMENT)==1 and text.count(',bool allowDead=false')==1
        return text.replace(COMMENT,'').replace(',bool allowDead=false','')
    assert text.count(',bool allowDead) const')==1 and text.count('finder.Begin(parent,allowDead)')==1
    return text.replace(',bool allowDead) const',') const').replace('finder.Begin(parent,allowDead)','finder.Begin(parent)')


def observe(commit):
    """기존 Carrier 전체 소스와 당시 보고서·사용 가능한 원본 출처가 보존됨을 증명한다."""
    source={};prior={}
    # 기존 삭제검사/Carrier preDestroy/다른 어댑터 및 선택 loop 전체를 비교한다.
    for name in SOURCES:
        baseline=git('show',commit+':'+name).decode('utf-8').replace('\r\n','\n');current=(ROOT/name).read_text(encoding='utf-8');existing=restore(name,current);assert baseline==existing and tokens(baseline)==tokens(existing),name
        source[name]=dict(baseline_sha256=sha(git('show',commit+':'+name)),current_sha256=sha((ROOT/name).read_bytes()),entire_existing_token_sha256=sha(tokens(existing)),entire_existing_source_unchanged=True)
    # 다른 전담 모듈의 변경과 별개로09 통합 원문은 당시 git 객체와 바이트까지 같다.
    for name in PRIOR:
        path=ROOT/name
        if not path.exists():continue
        assert path.read_bytes()==git('show',commit+':'+name);data=json.loads(path.read_text(encoding='utf-8'));prior[name]=dict(sha256=sha(path.read_bytes()),total=data.get('total'),additional_unique_cases=0,dependencies={})
        if name.endswith('run9.json'):continue
        # 원본 PE/export/도구/fixture만 감사하고 의도된 현재 h/cpp 변경은 위 전체 본문 lineage로 연결한다.
        for file,value in data.get('files',{}).items():
            if not (file.startswith(('tools/','originals/','originalCD/')) or '/fixtures/' in file or file.endswith(('creation.c','functions.tsv'))):continue
            if not (ROOT/file).exists():
                assert file.startswith('extracted/');prior[name]['dependencies'][file]=dict(reported_sha256=value,available=False);continue
            try:baseline_sha=sha(git('show',commit+':'+file))
            except subprocess.CalledProcessError:baseline_sha=value
            assert sha((ROOT/file).read_bytes())==baseline_sha,file;prior[name]['dependencies'][file]=dict(reported_sha256=value,baseline_09_sha256=baseline_sha,current_sha256=baseline_sha)
    return dict(source=source,prior_reports=prior)


def generate():
    """기준09 전체 본문과 현재 선택 전달 API의 SHA를 별도 저장한다."""
    commit=git('rev-parse','5b10ce8').decode('ascii').strip();data=dict(host='HJOW-Athlon',baseline_commit=commit,lineage=observe(commit),tool_sha256=sha(Path(__file__).read_bytes()),additional_unique_cases=0,scope='Dais wrapper00448170/CD00482430은allowDead=true,CanBridge00449710/CD00481910은false다. 기존 네인수 호출은기본false로동일. 선택인수/Begin전달/한줄설명 외 전체기존본문/관찰보존.')
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 선택 전달, 전체 기존 body와 당시 관찰 출처를 다시 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));assert data['additional_unique_cases']==0 and data['lineage']==observe(data['baseline_commit']) and data['tool_sha256']==sha(Path(__file__).read_bytes())
    print('Dais allowDead 공유: 기준09 Carrier 전체 body/이전 관찰 보존, 추가 입력0')


# 명시 호출에서만 새 lineage를 만들거나 현재 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
