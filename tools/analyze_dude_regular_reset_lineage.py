#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""a349e63의 전체 SquidProcess Git/작업원문과 기존5보고서/fixture를 보존해 ResetTask만 연결한다."""
import argparse
import json
import subprocess
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 단위10 커밋의 LF 원문과 당시 작업트리 바이트를 별도 이름/전체SHA로 보존한다.
BASELINE='a349e63'
HISTORY=ROOT/'cpppj/hist/dude-regular-dc7b9'
REPORT=ROOT/'cpppj/dude-regular-reset-lineage.json'
SHARED=('cpppj/src/o/SquidProcess.cpp','cpppj/src/o/SquidProcess.h')
NAMES=('process','processform','processretake','processtransmit','gunprocess')
ADDITIONS={
 SHARED[0]:'// 패치00450670/CD004e8e70의 기존task Regular 직접쓰기에 대응하며 payload/Form/부착을 보존한다.\nvoid RegularProcess::ResetTask(std::uint32_t event,double time) { event_=event;count_=0;time_=time; }\n',
 SHARED[1]:'    // Dude task 변경의 event/count0/다음double시각 직접쓰기를 같은 기존 Regular 객체에 적용한다.\n    void ResetTask(std::uint32_t event,double time);\n',
}


def paths():
    """기존 전체소스 두 파일과 당시5보고서/5fixture 경로를 돌려준다."""
    return [*SHARED,*[f'cpppj/{part}' for name in NAMES for part in (f'{name}.json',f'tests/fixtures/{name}-x86.tsv')]]


def audit():
    """Git 전체blob과 작업원문을 구분하고 좁은 추가분 외에 모든 기존 바이트를 검사한다."""
    files={}
    # 각 Git LF 원문은 실제 기준커밋에서 읽어 검증하고 작업CRLF는 당시 스냅샷을 그대로 보존한다.
    for relative in paths():
        git=HISTORY/'git'/relative;work=HISTORY/'work'/relative
        if git.read_bytes()!=subprocess.check_output(['git','show',BASELINE+':'+relative],cwd=ROOT):raise RuntimeError('Regular Reset 기준 전체Git 원문 오류 '+relative)
        if work.read_bytes().replace(b'\r\n',b'\n')!=git.read_bytes().replace(b'\r\n',b'\n'):raise RuntimeError('Regular Reset 기준 작업/Git 내용 차이 '+relative)
        files[relative]=dict(git_sha256=digest(git),work_sha256=digest(work),git_bytes=len(git.read_bytes()),work_bytes=len(work.read_bytes()))
    for relative,addition in ADDITIONS.items():
        raw=(ROOT/relative).read_bytes();newline=b'\r\n' if b'\r\n' in raw else b'\n';added=addition.encode('utf-8').replace(b'\n',newline)
        if raw.count(added)!=1:raise RuntimeError('Regular Reset 추가분 범위 오류 '+relative)
        previous=raw.replace(added,b'')
        if previous!=(HISTORY/'work'/relative).read_bytes():raise RuntimeError('Regular Reset 전체작업 이전본문 변경 '+relative)
        if previous.replace(b'\r\n',b'\n')!=(HISTORY/'git'/relative).read_bytes().replace(b'\r\n',b'\n'):raise RuntimeError('Regular Reset 전체Git 이전본문 변경 '+relative)
    return files


def generate():
    """추가native0과 전체기존본문/당시관찰보고서 바이트 보존을 현재API SHA에 연결한다."""
    baseline=audit();history={p.relative_to(ROOT).as_posix():digest(p) for p in HISTORY.rglob('*') if p.is_file()}
    report=dict(schema=1,host='HJOW-Athlon',baseline_commit=BASELINE,new_native=0,new_unique=0,baseline_files=baseline,history_files=history,current_sources={p:digest(ROOT/p) for p in SHARED},tool_sha256=digest(__file__),change='RegularProcess에 event/count0/time 직접쓰기 공개메서드와설명만추가. 기존생성/실행/부착/송신/조회/삭제 몸체와5보고서/fixture 전체불변.',limitations=['기존5보고서의당시전체바이트/관찰을보존하지만missing ignored export의새현재실행감사로세지않는다.','C++의이름있는event/count/time상태를갱신하며32bit원본객체물리배치로주장하지않는다.','DudeNotice원본fixture/같은Host소비자는별도단위에서추가한다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """당시LF/작업바이트/전체기존본문/현재SHA와추가관찰0을 다시 검사한다."""
    baseline=audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['baseline_files']!=baseline or report['new_native'] or report['new_unique'] or report['tool_sha256']!=digest(__file__):raise RuntimeError('Regular Reset lineage 근거 오류')
    for group in ('history_files','current_sources'):
        for path,expected in report[group].items():
            if digest(ROOT/path)!=expected:raise RuntimeError('Regular Reset 전체SHA 오류 '+path)
    print(json.dumps(dict(verified=True,baseline=BASELINE,full_shared_bodies=2,preserved_reports=5,new_native=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
