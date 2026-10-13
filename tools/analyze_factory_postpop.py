#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Factory/아비어리 전체 분배·실제 프레임 연결의 성공 로그와 SHA를 보존한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 다른 작업 단위의 역사적 SHA를 덮어쓰지 않는 현재 단위 근거다.
ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT/'cpppj/recovery-factory-world-evidence.json'
BUILD = 'extracted/factorypostpop-build-final.log'
CTEST = 'extracted/factorypostpop-ctest-final.log'
TESTS = 'extracted/factorypostpop-tests-final.log'
FOCUSED = 'extracted/factorypostpop-focused-final.log'
# 현재 가상 분배와 공유 목록 및 검사·CMake·독립 도구의 바이트를 기록한다.
SOURCES = [
    'cpppj/src/o/RawFactoryPostPop.h','cpppj/src/o/RawFactoryPostPop.cpp',
    'cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp','cpppj/src/o/RawAssetPostPop.h',
    'cpppj/tests/FactoryPostPopTests.cpp','cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
    'tools/analyze_factory_postpop.py','docs/exe/cpp-factory-postpop.md',
]


def sha(name):
    """정규화 없이 저장된 파일 바이트의 SHA256을 반환한다."""
    return hashlib.sha256((ROOT/name).read_bytes()).hexdigest()


def read(name):
    """UTF-8 소스·JSON을 엄격히 읽고 원래 인코딩을 보존한 빌드 로그만 대체 판독한다."""
    return (ROOT/name).read_text(encoding='utf-8',errors='replace' if name.endswith('.log') else 'strict')


def main():
    """실제 성공한 단위의 자료만 생성하거나 이전 성공 시점의 SHA를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(read(REPORT.relative_to(ROOT).as_posix()))
        # 구현·원본·입력·로그 모두 현재 바이트가 같은지 확인한다.
        for name,digest in report['sha256'].items():assert sha(name)==digest,name
        assert report['native_cases']==996 and report['tests']['cases']==936 and report['tests']['failed_checks']==0
        assert not report['campaign_completed'] and not report['directdraw_fullscreen_changed']
        print(json.dumps({'verified':True,'tests':report['tests'],'native_cases':996},ensure_ascii=False));return
    assert 'netstorm_tests.vcxproj ->' in read(BUILD) and not re.search(r'\b(?:error|warning)\s+[A-Z]+\d+:',read(BUILD))
    assert '100% tests passed' in read(CTEST) and '3 test(s), 0 failed check(s)' in read(FOCUSED)
    result=re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)',read(TESTS));assert result and int(result[1])==936 and int(result[2])==0
    native='cpppj/recovery-factorypostpop-evidence.json';evidence=json.loads(read(native));assert evidence['total']==996
    files=SOURCES+[BUILD,CTEST,TESTS,FOCUSED,native]
    # 독립 PE 실제 실행의 source/body/fixture SHA를 함께 확인한다.
    for name,digest in evidence['files'].items():assert sha(name)==digest,name;files.append(name)
    report=dict(host=socket.gethostname(),last_decompile_host='VM-W11-CODEX',date='2026-10-13',
        tests=dict(cases=int(result[1]),failed_checks=int(result[2])),native_cases=996,gui_executions=0,
        campaign_completed=False,directdraw_fullscreen_changed=False,sha256={name:sha(name) for name in dict.fromkeys(files)},limits=[
            'native는 실제 일반 finder·상태 getter·목록·프레임 구간 판단·후처리 몸체를 두 x87 정밀도로 실행한다.',
            '예약 할당/Regular·Damageable·SquidFrame 효과·아비어리 소유권/경고/삭제는 native 명시 관찰 경계다.',
            'C++ 직접 Damageable→공통 장부와 실제 SquidFrame의 dirty/프레임 쓰기 연결을 추가 검사한다.',
            '아비어리 섬 소유권 재계산·실제 Regular/건물 삭제·GameWorld 비표면 이관은 후속이며 캠페인1-1은 미완료다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'tests':report['tests'],'native_cases':996,'campaign_completed':False},ensure_ascii=False))


# 명시 실행에서만 완료 근거를 생성하거나 감사한다.
if __name__=='__main__':main()
