#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort/Gun/Path/ProcessForm 전송·같은 raw 월드 연결 단위의 실제 검증 로그와 SHA를 보존한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 이번 단위의 현재 구현은 앞선 공간/사제 단위 기록과 별도로 저장한다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/campaign-process.json'
UNITS={'fortobjectload':'total','fortfinishowner':'total','gunprocess':'total','pathruntime':'cases','processtransmit':'total'}
# 집중 성공 로그는 최초 실패/교정 전 로그와 구별한다.
FOCUSED={
    'extracted/campaign-process-focused-FortObjectLoad.log':4,
    'extracted/campaign-process-focused-FortFinishOwner-final.log':4,
    'extracted/campaign-process-focused-GunProcess-final.log':5,
    'extracted/campaign-process-focused-rawPath-final.log':7,
    'extracted/campaign-process-focused-ProcessTransmit.log':2,
    'extracted/campaign-process-focused-RawSurfaceWorld.log':6,
    'extracted/campaign-process-focused-ProcessRetake.log':5,
}
# 실제 실행된 구현·연결·검사의 바이트를 동일 revision의 자료와 함께 감사한다.
SOURCES=[
    'cpppj/src/o/SquidProcess.h','cpppj/src/o/SquidProcess.cpp',
    'cpppj/src/o/SquidFormDispatch.h','cpppj/src/o/SquidFormDispatch.cpp',
    'cpppj/src/o/SquidForms.h','cpppj/src/o/SquidForms.cpp',
    'cpppj/src/o/RawFortObjectLoad.h','cpppj/src/o/RawFortObjectLoad.cpp',
    'cpppj/src/o/RawFortFinishOwner.h','cpppj/src/o/RawFortFinishOwner.cpp',
    'cpppj/src/o/RawGunProcess.h','cpppj/src/o/RawGunProcess.cpp',
    'cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathRuntime.cpp',
    'cpppj/src/o/RawPathProcess.h','cpppj/src/o/RawPathProcess.cpp',
    'cpppj/src/client/RawSurfaceWorld.h','cpppj/src/client/RawSurfaceWorld.cpp',
    'cpppj/tests/FortObjectLoadTests.cpp','cpppj/tests/GunProcessTests.cpp',
    'cpppj/tests/FortFinishOwnerTests.cpp',
    'cpppj/tests/RawPathRuntimeTests.cpp','cpppj/tests/ProcessTransmitTests.cpp','cpppj/tests/RawSurfaceWorldTests.cpp',
    'cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt','tools/analyze_campaign_process.py',
]


def sha(path):
    """정규화하지 않은 파일 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()


def main():
    """실제 전체 성공 뒤 생성하고 verify는 저장 구현/PE/근거/로그를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        # 모든 파일은 실제 성공 시점의 정확한 바이트와 같아야 한다.
        for path,digest in report['sha256'].items():assert sha(path)==digest,path
        assert not report['campaign_completed'] and not report['directdraw_fullscreen_changed']
        assert report['tests']['failed_checks']==0 and report['native_cases']==sum(report['native_units'].values())
        print(json.dumps(dict(verified=True,tests=report['tests'],native_cases=report['native_cases']),ensure_ascii=False));return
    build_path='extracted/campaign-process-build-final.log';ctest_path='extracted/campaign-process-ctest-final.log'
    build=(ROOT/build_path).read_text(encoding='utf-8',errors='replace');ctest=(ROOT/ctest_path).read_text(encoding='utf-8',errors='replace')
    assert 'netstorm_tests.vcxproj ->' in build and not re.search(r'\b(?:error|warning)\s+[A-Z]+\d+:',build)
    assert '100% tests passed' in ctest
    last_path='cpppj/build/Testing/Temporary/LastTest.log'
    result=re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)',(ROOT/last_path).read_text(encoding='utf-8',errors='replace'))
    assert result and int(result[2])==0
    report=dict(host=socket.gethostname(),last_decompile_host='VM-W11-CODEX',date='2026-10-13',campaign_completed=False,
        directdraw_fullscreen_changed=False,gui_executions=0,tests=dict(cases=int(result[1]),failed_checks=int(result[2])),
        native_units={},native_cases=0,sha256={},limits=[
            'GameWorld의 비표면 객체/관찰 기반 이동은 아직 실제 core로 이관되지 않음',
            'Fort 단일 무작위 placement·Gun 표적검색/각도/애니메이션/투사체·Path Init/BFS/운반/cleanup/직렬화는 후속',
            'ProcessForm WriteData/공유 버퍼 송신은 명시 경계이며 실제 Gun serializer는 별도 근거와 C++ 연결 검사',
            'native Kernel은 대표 입력 두 정밀도만 실제 실행, 다른 전송 교차 입력은 기존 Kernel 근거/번호 응답 경계와 C++ 실제 등록을 사용',
            '실제 건설/공격/사제 운반·의식/승패·캠페인 완주 및 게임/GUI 대조는 미완료'])
    files=list(SOURCES)+[build_path,ctest_path,last_path,'originals/Netstorm.exe','originalCD/NETSTORM.EXE',
        'cpppj/recovery-gun-lifetime-audit.json','tools/analyze_gun_lifetime.py']
    # 집중 로그도 실제 개수/실패0을 확인하고 기록한다. 전체 검사와 중복 합산하지 않는다.
    for path,count in FOCUSED.items():
        assert f'{count} test(s), 0 failed check(s)' in (ROOT/path).read_text(encoding='utf-8',errors='replace'),path
        files.append(path)
    report['focused']=FOCUSED
    # 각 단위의 고유 입력 행 수만 더하며 재실행·정밀도 반복·setup 내부 호출수는 더하지 않는다.
    for name,key in UNITS.items():
        evidence=f'cpppj/{name}.json';data=json.loads((ROOT/evidence).read_text(encoding='utf-8'))
        count=data[key];assert isinstance(count,int) and count>0;report['native_units'][name]=count
        files.extend((evidence,f'cpppj/tests/fixtures/{name}-x86.tsv',f'tools/decomp_{name}_oracle.py'))
    report['native_cases']=sum(report['native_units'].values())
    # 같은 경로는 한 번만 기록한다.
    for path in dict.fromkeys(files):report['sha256'][path]=sha(path)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(tests=report['tests'],native_cases=report['native_cases'],campaign_completed=False),ensure_ascii=False))


# 명시 실행만 완료 기록/독립 SHA 감사를 시작한다.
if __name__=='__main__':main()
