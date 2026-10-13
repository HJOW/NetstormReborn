#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 삭제 후 Regular의 초기 count 지정 API와 직전 전체 소스/관찰을 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 현재 저장소의 절대 기준이다.
HISTORY=ROOT/'cpppj/hist/regular-init-48bf5'  # 직전 커밋의 전체 작업/Git 바이트를 보존한다.
REPORT=ROOT/'cpppj/regular-initial-count-lineage.json'  # 좁은 API 추가와 기존 자료 보존의 별도 근거다.
SHARED=('cpppj/src/o/SquidProcess.h','cpppj/src/o/SquidProcess.cpp')  # 기존 생성/실행/부착 전체를 보존할 두 파일이다.
FIXTURES=('process','processform','processretake','processtransmit','gunprocess')  # 기존 다섯 원본 관찰 입력을 그대로 보존한다.
ADDITIONS={
    SHARED[0]:'    // Damageable 삭제 후 새 Regular에 원본 +0x20 초기 count를 쓴다. event/payload/time/Form은 보존한다.\n    void SetInitialCount(std::uint32_t count);\n',
    SHARED[1]:'// 패치0044b9e0/CD00461b40의 생성 직후 count 직접쓰기를 같은 실제 Regular에 적용한다.\nvoid RegularProcess::SetInitialCount(std::uint32_t count) { count_=count; }\n'
}  # 이 두 설명/메서드 외에 기존 몸체 변경을 허용하지 않는다.


def sha(path):
    """파일 전체 바이트의 SHA256을 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """API 추가 전에 현재 커밋의 전체 공유 소스/기존 관찰 보고서와 fixture를 보존한다."""
    if HISTORY.exists():raise RuntimeError('초기 count 기준 자료는 이미 존재하므로 덮어쓰지 않습니다.')
    baseline=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    candidates=subprocess.check_output(['git','ls-files','cpppj/*.json'],cwd=ROOT,text=True).splitlines()
    names=[name for name in candidates if any(shared in (ROOT/name).read_text(encoding='utf-8') for shared in SHARED)]
    paths=sorted(set((*SHARED,*names,*(f'cpppj/tests/fixtures/{name}-x86.tsv' for name in FIXTURES))))
    saved={}
    # Git LF 원문과 당시 작업 CRLF를 구분하여 둘 다 전체 바이트로 저장한다.
    for relative in paths:
        source=ROOT/relative
        work=HISTORY/'work'/relative
        git=HISTORY/'git'/relative
        work.parent.mkdir(parents=True,exist_ok=True)
        git.parent.mkdir(parents=True,exist_ok=True)
        work.write_bytes(source.read_bytes())
        git.write_bytes(subprocess.check_output(['git','show',baseline+':'+relative],cwd=ROOT))
        if work.read_bytes().replace(b'\r\n',b'\n')!=git.read_bytes().replace(b'\r\n',b'\n'):
            raise RuntimeError('초기 count 기준 작업/Git 전체 내용 차이 '+relative)
        saved[relative]=dict(work_sha256=sha(work),git_sha256=sha(git))
    (HISTORY/'baseline.json').write_text(json.dumps(dict(baseline_commit=baseline,files=saved),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Regular 초기 count 기준 전체 보존',len(saved),'파일',baseline)


def audit():
    """정확한 추가분 역변환과 전체 기준 Git/작업 바이트 및 기존 관찰 불변을 감사한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'))
    # 기존 보고서/fixture는 수정 없이 유지하며 두 공유 파일만 최소 추가분을 제거해 비교한다.
    for relative,expected in baseline['files'].items():
        work=HISTORY/'work'/relative
        git=HISTORY/'git'/relative
        if sha(work)!=expected['work_sha256'] or sha(git)!=expected['git_sha256']:
            raise RuntimeError('초기 count 전체 기준 SHA 오류 '+relative)
        if git.read_bytes()!=subprocess.check_output(['git','show',baseline['baseline_commit']+':'+relative],cwd=ROOT):
            raise RuntimeError('초기 count 기준 전체 Git 원문 오류 '+relative)
        current=(ROOT/relative).read_bytes()
        if relative in ADDITIONS:
            newline=b'\r\n' if b'\r\n' in current else b'\n'
            addition=ADDITIONS[relative].encode('utf-8').replace(b'\n',newline)
            if current.count(addition)!=1:raise RuntimeError('초기 count 추가 범위 오류 '+relative)
            current=current.replace(addition,b'',1)
        if current!=work.read_bytes():raise RuntimeError('초기 count 이전 전체 몸체/관찰 변경 '+relative)
    return baseline


def record():
    """새 원본 관찰0과 전체 보존 자료를 현재 두 공유 API 파일에 연결한다."""
    baseline=audit()
    histories={path.relative_to(ROOT).as_posix():sha(path) for path in HISTORY.rglob('*') if path.is_file()}
    data=dict(schema=1,host='HJOW-Athlon',baseline_commit=baseline['baseline_commit'],new_native=0,new_unique=0,
              baseline_files=baseline['files'],history_files=histories,current_sources={name:sha(ROOT/name) for name in SHARED},
              tool_sha256=sha(Path(__file__)),change='Regular의 초기 count DWORD만 지정한다. 기존 ResetTask와 생성/실행/부착/송신/삭제 본문 및 전체 기존 관찰을 보존한다.')
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """현재 전체 바이트와 정확 역변환 및 기존 관찰 보고서의 보존을 다시 확인한다."""
    baseline=audit()
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    if data['baseline_files']!=baseline['files'] or data['tool_sha256']!=sha(Path(__file__)) or data['new_native'] or data['new_unique']:
        raise RuntimeError('초기 count lineage 근거 오류')
    # 보존한 전체 스냅샷과 현재 공유 소스의 바이트도 각각 비교한다.
    for group in ('history_files','current_sources'):
        for relative,expected in data[group].items():
            if sha(ROOT/relative)!=expected:raise RuntimeError('초기 count 전체 SHA 오류 '+relative)
    print('Regular 초기 count 전체 역변환/이전 관찰 보존 감사 통과; 새 원본0')


# --capture는 변경 직전 기준 보존, --record는 추가분 최초 기록, 기본 호출은 현재 감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true')
    parser.add_argument('--record',action='store_true')
    arguments=parser.parse_args()
    capture() if arguments.capture else record() if arguments.record else verify()
