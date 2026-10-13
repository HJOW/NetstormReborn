#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 지형 도장의 후보 선택 몸체를 공유 함수로 옮긴 소스/토큰 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

# 후보 공유 직전의 완료 commit과484개 원본 근거를 고정한다.
ROOT=Path(__file__).resolve().parent.parent
BASELINE='4e532fd'
PRIOR=ROOT/'cpppj/recovery-regionpaint-evidence.json'
REPORT=ROOT/'cpppj/recovery-framechoose-lineage-evidence.json'

def digest(data):
    """원본 파일 또는 정규화된 코드 문자열의 SHA-256을 계산한다."""
    return hashlib.sha256(data).hexdigest()

def source(name,baseline=False):
    """기준 git blob 또는 현재 파일을 UTF-8로 읽고 LF로만 정규화한다."""
    data=subprocess.check_output(['git','show',f'{BASELINE}:{name}'],cwd=ROOT) if baseline else (ROOT/name).read_bytes()
    return data.decode('utf-8').replace('\r\n','\n'),data

def tokens(text):
    """설명 주석과 공백을 제외한 전체 C++ 코드 토큰을 비교용으로 반환한다."""
    return re.findall(r'\w+|[^\s]',re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S))

def audit():
    """옮긴 몸체 및 나머지 헤더/구현 전체가 기준과 같음을 직접 재구성한다."""
    old,old_bytes=source('cpppj/src/o/RawRegionPaint.cpp',True);current,current_bytes=source('cpppj/src/o/RawRegionPaint.cpp')
    pattern=r'// 원본 최대256개 후보와 실패 -1을 유지한다\.\nstd::int32_t RawRegionPaint::Choose\([^\n]+\) \{\n(.*?)\n\}\n'
    found=re.search(pattern,old,re.S)
    if not found:raise RuntimeError('기준 후보 선택 몸체 없음')
    moved,_=source('cpppj/src/o/RawFrameChoose.cpp');start=moved.index('    std::vector<std::int32_t> candidates;');end=moved.rindex('\n}\n}')
    body=moved[start:end]
    if tokens(body)!=tokens(found.group(1)):raise RuntimeError('옮긴 후보 몸체 토큰 차이')
    reconstructed=current.replace('#include "o/RawFrameChoose.h"\n','').replace('ChooseTerrainFrame(frames_[type],code,state_.theme,random_)','Choose(frames_[type],code,state_.theme)')
    retained=old[:found.start()]+old[found.end():]
    if reconstructed!=retained:raise RuntimeError('기존 지형 도장 나머지 전체 소스 차이')
    old_header,old_header_bytes=source('cpppj/src/o/RawRegionPaint.h',True);header,_=source('cpppj/src/o/RawRegionPaint.h')
    declaration='    // 0049aa90의 최대256 후보·정규화/방향 폴백·quartile 선택이다.\n    std::int32_t Choose(const RiftTypeFrames& frames,FrameCode code,std::int32_t theme);\n'
    if old_header.replace(declaration,'')!=header:raise RuntimeError('기존 지형 도장 나머지 헤더 차이')
    prior=json.loads(PRIOR.read_text(encoding='utf-8'))
    # 이전 native 전체 source SHA를 기준 commit의 실제 LF/CRLF 바이트와 연결한다.
    for name,data in (('cpppj/src/o/RawRegionPaint.cpp',old_bytes),('cpppj/src/o/RawRegionPaint.h',old_header_bytes)):
        variants={digest(data),digest(data.replace(b'\n',b'\r\n'))}
        if prior['files'][name] not in variants:raise RuntimeError(f'이전 native source SHA 연결 실패: {name}')
    # 구현 이동으로 변하지 않아야 하는 PE/export/oracle/fixture 출처도 현재 바이트와 대조한다.
    for name,expected in prior['files'].items():
        if name.startswith('cpppj/src/o/RawRegionPaint'):continue
        if digest((ROOT/name).read_bytes())!=expected:raise RuntimeError(f'기존 원본 출처 차이: {name}')
    files=['tools/analyze_framechoose_lineage.py','cpppj/src/o/RawFrameChoose.cpp','cpppj/src/o/RawFrameChoose.h',
           'cpppj/src/o/RawRegionPaint.cpp','cpppj/src/o/RawRegionPaint.h','cpppj/recovery-regionpaint-evidence.json']
    return {'host':'HJOW-Athlon','baseline_commit':subprocess.check_output(['git','rev-parse',BASELINE],cwd=ROOT,text=True).strip(),
            'prior_unique_cases':484,'new_unique_cases':0,'moved_body_token_sha256':digest(json.dumps(tokens(body),ensure_ascii=False).encode('utf-8')),
            'remaining_header_and_implementation_identical':True,'files':{name:digest((ROOT/name).read_bytes()) for name in files}}

def main():
    """기본 실행은 연결 근거 생성이며 --verify는 저장된 근거와 현재 재구성을 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();data=audit()
    if args.verify:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('후보 선택 lineage 현재 SHA 차이')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('기존 후보 몸체 토큰·나머지 지형 도장 전체 소스·484개 원본 출처 보존 확인')

if __name__=='__main__':
    main()
