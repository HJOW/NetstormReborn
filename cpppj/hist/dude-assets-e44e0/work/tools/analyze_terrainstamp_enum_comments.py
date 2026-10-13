#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기stamp enum 설명만 추가한 전후 헤더 전체 C++토큰과 기존 native/C++ 출처 보존을 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

# 주석 교정 전 자료와 현재 독립 근거 기록이다. 원본 실행기와 관찰 행은 다시 실행하지 않는다.
ROOT=Path(__file__).resolve().parent.parent
HEADER='cpppj/src/o/RawTerrainStamp.h'
BEFORE_HEADER=ROOT/'extracted/terrainstamp-before-enum-comment.h'
BEFORE_REPORT=ROOT/'extracted/terrainstamp-before-enum-comment-evidence.json'
CURRENT_REPORT=ROOT/'cpppj/terrainstamp.json'
REPORT=ROOT/'cpppj/terrainstamp-enum-comments.json'


def sha(data):
    """정확한 파일/토큰 바이트의 SHA를 반환한다."""
    return hashlib.sha256(data).hexdigest()


def tokens(source):
    """설명주석과 공백만 제외한 헤더 전체 C++토큰을 순서대로 반환한다."""
    return re.findall(r'\w+|[^\s]',re.sub(r'/\*.*?\*/|//[^\n]*','',source,flags=re.S))


def audit():
    """전후 헤더 토큰과 나머지 모든 출처를 확인한 뒤 이전 소스 내용/전체SHA를 함께 보존한다."""
    before=BEFORE_HEADER.read_bytes();old=json.loads(BEFORE_REPORT.read_text(encoding='utf-8'));current=json.loads(CURRENT_REPORT.read_text(encoding='utf-8'))
    after=(ROOT/HEADER).read_bytes();prior=tokens(before.decode('utf-8'));now=tokens(after.decode('utf-8'))
    if prior!=now or sha(before)!=old['cpp_source_files'][HEADER] or sha(after)!=current['cpp_source_files'][HEADER]:raise RuntimeError('초기stamp enum 전체토큰/출처 연결 불일치')
    if old['native_source_files']!=current['native_source_files']:raise RuntimeError('초기stamp enum 작업의 native 출처 변경')
    preserved={name:value for name,value in old['files'].items() if name!=HEADER}
    for name,value in preserved.items():
        if sha((ROOT/name).read_bytes())!=value:raise RuntimeError('초기stamp enum 작업의 다른 파일 변경 '+name)
    result=dict(host='HJOW-Athlon',added_native_cases=0,baseline_report_sha256=sha(BEFORE_REPORT.read_bytes()),
        baseline_header_utf8=before.decode('utf-8'),baseline_header_sha256=sha(before),current_header_sha256=sha(after),
        cpp_tokens=len(prior),cpp_tokens_sha256=sha('\n'.join(prior).encode('utf-8')),native_source_files=current['native_source_files'],
        previous_cpp_source_files=old['cpp_source_files'],preserved_files=preserved,
        change='TerrainStampAssertion::MissingRegion/UnpredictableFringe 각각의 한국어 용도주석만 추가했다. 헤더 전체 C++토큰과 나머지 소스/원본/행 출처는 불변이다.',
        files={HEADER:sha(after),'tools/analyze_terrainstamp_enum_comments.py':sha(Path(__file__).read_bytes())})
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'초기stamp enum 주석: 헤더 전체{len(prior)}토큰 불변/native 출처 불변·새 관찰0 감사 통과')


def verify():
    """보존된 이전 헤더와 현재 파일을 재대조해 현재환경에서도 토큰/출처 연결을 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));current=json.loads(CURRENT_REPORT.read_text(encoding='utf-8'))
    before=report['baseline_header_utf8'];now=(ROOT/HEADER).read_bytes();prior=tokens(before)
    if sha(before.encode('utf-8'))!=report['baseline_header_sha256'] or prior!=tokens(now.decode('utf-8')) or sha(now)!=report['current_header_sha256'] or len(prior)!=report['cpp_tokens'] or sha('\n'.join(prior).encode('utf-8'))!=report['cpp_tokens_sha256']:raise RuntimeError('초기stamp enum 전후 토큰 연결 오류')
    for name,value in {**report['files'],**report['preserved_files']}.items():
        if name=='cpppj/src/o/RawTerrainStamp.cpp' and 'mask_successor' in report:
            successor=report['mask_successor'];mask=json.loads((ROOT/successor['path']).read_text(encoding='utf-8'))
            if value!=mask['baseline_cpp_sha256'] or sha((ROOT/successor['path']).read_bytes())!=successor['sha256'] or sha((ROOT/name).read_bytes())!=mask['current_cpp_sha256']:raise RuntimeError('초기stamp enum 후속 mask 출처 연결 오류')
            from analyze_terrainstamp_mask_correction import verify as verify_mask
            verify_mask();continue
        if sha((ROOT/name).read_bytes())!=value:raise RuntimeError('초기stamp enum current SHA 불일치 '+name)
    if report['added_native_cases'] or report['native_source_files']!=current['native_source_files'] or current['cpp_source_files'][HEADER]!=report['current_header_sha256']:raise RuntimeError('초기stamp enum native/현재cpp 연결 오류')
    print('초기stamp enum 전체토큰/이전소스·현재CPP·native 출처 연결 감사 통과')


def link_mask():
    """주석 단계의 당시cpp SHA를 이후 제한된 mask교정 출처로 연결한다. 기존헤더토큰/native는 그대로다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));path='cpppj/terrainstamp-mask-correction.json';mask=json.loads((ROOT/path).read_text(encoding='utf-8'))
    if report['preserved_files']['cpppj/src/o/RawTerrainStamp.cpp']!=mask['baseline_cpp_sha256']:raise RuntimeError('초기stamp enum→mask 이전cpp SHA 불일치')
    report['mask_successor']=dict(path=path,sha256=sha((ROOT/path).read_bytes()),
        reason='주석 단계의 cpp 불변 근거를 그 이후의 제한된 대각선bit 교정에 연결한다. 헤더581토큰/native1560행은 그대로다.')
    tool='tools/analyze_terrainstamp_enum_comments.py';report['previous_enum_tool_sha256']=report['files'][tool];report['files'][tool]=sha(Path(__file__).read_bytes())
    report['previous_enum_evidence_sha256']=sha((ROOT/'extracted/terrainstamp-enum-comments-before-mask-link.json').read_bytes())
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('초기stamp enum581토큰/native불변 및 이후 mask교정 출처 연결 완료. 새native0.')


# 명시한 주석 lineage 생성 또는 기존 기록의 현재 출처 감사만 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--link-mask',action='store_true');args=parser.parse_args()
    if args.link_mask:link_mask()
    elif args.verify:verify()
    else:audit()
