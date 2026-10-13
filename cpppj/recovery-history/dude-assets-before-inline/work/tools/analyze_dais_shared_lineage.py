#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dais 소비자의 AltarAt/MoveAwayDepth 공개를 기준08 전체 소스·기존 원본 출처와 연결한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 공유 저장소의 절대 기준이다.
REPORT=ROOT/'cpppj/recovery-dais-shared-lineage-evidence.json' # 기존08/원본 보고서를 수정하지 않는 새 API 연결 근거다.
SOURCES=('cpppj/src/o/RawRunePostPop.h','cpppj/src/o/RawRunePostPop.cpp','cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp') # 승인된 네 기존 파일만 비교한다.
PRIOR=('cpppj/recovery-campaign-runtime8-evidence.json','cpppj/recovery-runepostpop-evidence.json','cpppj/recovery-runepostpop-lineage-evidence.json','cpppj/recovery-temple-rune-runtime8-lineage-evidence.json','cpppj/recovery-pathruntime-evidence.json','cpppj/recovery-pathstart-evidence.json','cpppj/recovery-pathcleanup-evidence.json') # 이전 정상/회귀/교정 관찰은 새 입력에 합산하지 않는다.


def sha(data):
    """전체 파일과 주석 제외 토큰의 SHA256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """기준 git 객체를 읽기만 하며 커밋/푸시를 실행하지 않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(text):
    """문자열 리터럴을 보존하고 주석/공백만 제거한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text)
    return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def restore(name,text):
    """새 두 API와 공유 조회의 위임만 역제거해 기존 전체 문자를 되살린다."""
    if name.endswith('RawPathRuntime.h'):
        prefixes=('    // Dais 다음 목표 이동이','    std::int32_t MoveAwayDepth()','    // 같은 필드 DWORD만 쓴다.','    void SetMoveAwayDepth(')
        lines=text.splitlines(keepends=True);removed=[line for line in lines if any(line.startswith(prefix) for prefix in prefixes)];assert len(removed)==4
        return ''.join(line for line in lines if line not in removed)
    if name.endswith('RawPathRuntime.cpp'):
        prefixes=('// 실제 Moveaway getter','std::int32_t RawPathRuntime::MoveAwayDepth()','// 실제 BridgeCommand00449900','void RawPathRuntime::SetMoveAwayDepth(')
        lines=text.splitlines(keepends=True);removed=[line for line in lines if any(line.startswith(prefix) for prefix in prefixes)];assert len(removed)==4
        return ''.join(line for line in lines if line not in removed)
    if name.endswith('RawRunePostPop.h'):
        old='    // Rune기준점의첫altar원래ownerWORD/playerFlags와현재전역으로7/10/25초를읽는다.\n'
        prefixes=('    // 전달한 객체 기준점의 첫 altar를','    Sid AltarAt(','    // 전달한 객체 기준점의 첫 altar 원래ownerWORD')
        lines=text.splitlines(keepends=True);removed=[line for line in lines if any(line.startswith(prefix) for prefix in prefixes)];assert len(removed)==3
        return ''.join(old if line==removed[-1] else '' if line in removed else line for line in lines)
    start=text.index('// 기존 시간 helper의 typed altar 조회');end=text.index('// patch의char7/10/25',start);method=text[start:end]
    declaration='Sid RawRunePostPop::AltarAt(Sid rune) const {\n';assert declaration in method and method.endswith('    return altar;\n}\n')
    body=method.split(declaration,1)[1].removesuffix('    return altar;\n}\n')
    text=text[:start]+text[end:];replacement='    const auto altar=AltarAt(rune);\n';assert text.count(replacement)==1
    return text.replace(replacement,body)


def observe(commit):
    """전체 이전 body와 옮긴 조회 prefix, 당시 보고서 및 PE/export/tool/fixture 불변을 감사한다."""
    sources={};prior={}
    # 신규 선언/정의/위임 외 모든 기존 함수·상수·프레임/HP/경로 필드는 정확히 보존한다.
    for name in SOURCES:
        baseline=git('show',commit+':'+name).decode('utf-8').replace('\r\n','\n');current=(ROOT/name).read_text(encoding='utf-8');restored=restore(name,current);assert baseline==restored and tokens(baseline)==tokens(restored),name
        sources[name]=dict(baseline_sha256=sha(git('show',commit+':'+name)),current_sha256=sha((ROOT/name).read_bytes()),entire_existing_token_sha256=sha(tokens(restored)),entire_existing_source_unchanged=True)
    # 원래08/선행 보고서는 바이트까지 그대로며 원본 관찰의 공통 출처도 그대로다.
    for name in PRIOR:
        path=ROOT/name
        if not path.exists():continue
        assert path.read_bytes()==git('show',commit+':'+name),name;data=json.loads(path.read_text(encoding='utf-8'));prior[name]=dict(sha256=sha(path.read_bytes()),total=data.get('total'),additional_unique_cases=0,dependencies={})
        # 다른 전담 모듈의 후속 관찰은 root가 연결한다. 이번 API는08 통합 보고서 원문 SHA를 보존한다.
        if name.endswith('recovery-campaign-runtime8-evidence.json'):continue
        for file,value in data.get('files',{}).items():
            if '/src/' in file or '/tests/' in file and '/fixtures/' not in file:continue
            if not (file.startswith('tools/') or '/fixtures/' in file or file.startswith(('originals/','originalCD/')) or file.endswith(('creation.c','functions.tsv'))):continue
            # 과거 다른 PC의 Git 제외 내보내기가 없는 경우 원보고서 SHA를 보존하고 재검증 결손을 명시한다.
            if not (ROOT/file).exists():
                assert file.startswith('extracted/'),file
                prior[name]['dependencies'][file]=dict(reported_sha256=value,available=False,limitation='당시 ignored export가 현재 호스트에 없다. 바이트 재검증/새 관찰 합산을 하지 않으며 원보고서·fixture·기존 전체 소스 lineage를 보존한다.')
                continue
            current_sha=sha((ROOT/file).read_bytes())
            try:baseline_sha=sha(git('show',commit+':'+file))
            except subprocess.CalledProcessError:baseline_sha=value
            assert current_sha==baseline_sha,file
            prior[name]['dependencies'][file]=dict(reported_sha256=value,baseline_08_sha256=baseline_sha,current_sha256=current_sha)
    return dict(sources=sources,prior_reports=prior)


def generate():
    """08 당시 전체 소스와 현재 API SHA를 별도 근거로 저장한다."""
    commit=git('rev-parse','552656c').decode('ascii').strip();data=dict(host='HJOW-Athlon',baseline_commit=commit,lineage=observe(commit),tool_sha256=sha(Path(__file__).read_bytes()),additional_unique_cases=0,scope='AltarAt은 기존 ConstructionSeconds typed 조회 prefix를 그대로 이동하고 시간 선택 본문은 보존한다. MoveAwayDepth는 helper+4 signedDWORD 조회/쓰기만 추가한다. 이전 Rune1920/Runtime/Path/08 관찰을 새 Dais 고유 입력에 합산하지 않는다.')
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 API와 기준08 본문/이전 출처 불변을 다시 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));assert data['additional_unique_cases']==0 and data['lineage']==observe(data['baseline_commit']) and data['tool_sha256']==sha(Path(__file__).read_bytes())
    print('Dais 공유 API: 기준08 전체 기존 소스/토큰·이전 관찰 보존, 추가 입력0')


# 명시 호출에서만 새 lineage를 만들거나 현재 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
