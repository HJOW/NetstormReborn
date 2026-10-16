#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표적/서버 Path/미니맵 통지의 현재 연결 단위와 검증 한계를 함께 기록한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 앞선 Path/Factory의 역사적 근거를 덮어쓰지 않는 새 통합 단위다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/campaign-target-path.json'
BUILD='extracted/campaign-target-path-build-final.log'
CTEST='extracted/campaign-target-path-ctest-final.log'
TESTS='extracted/campaign-target-path-tests-final.log'
# 새 고유 관찰과 재사용 관찰을 구별하며 정밀도 반복을 더하지 않는다.
UNITS={'guntarget':('total',3454),'metadisplaydirty':('total',1040),'pathdata':('cases',768),'pathprepare':('cases',48)}
FOCUSED={'rawGunTarget':3,'metadisplayDirty':4,'rawPathData':3,'factoryPostPop':3}
# 같은 풀/가상 분배/좌표 통지와 판본별 시각 전달을 확인할 현재 구현이다.
SOURCES=[
    'cpppj/src/o/RawGunTarget.h','cpppj/src/o/RawGunTarget.cpp',
    'cpppj/src/o/RawMetadisplayDirty.h','cpppj/src/o/RawMetadisplayDirty.cpp',
    'cpppj/src/o/RawPathData.h','cpppj/src/o/RawPathData.cpp',
    'cpppj/src/o/RawPathProcess.h','cpppj/src/o/RawPathProcess.cpp',
    'cpppj/src/o/RawFactoryPostPop.h','cpppj/src/o/RawFactoryPostPop.cpp','cpppj/src/o/RawAssetPostPop.h',
    'cpppj/src/o/SquidPop.h','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidUnpop.h','cpppj/src/o/SquidUnpop.cpp',
    'cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
    'cpppj/tests/GunTargetTests.cpp','cpppj/tests/MetadisplayDirtyTests.cpp','cpppj/tests/RawPathDataTests.cpp',
    'cpppj/tests/FactoryPostPopTests.cpp','tools/analyze_campaign_target_path.py',
]


def sha(name):
    """현재 파일 바이트의 SHA256을 계산한다."""
    return hashlib.sha256((ROOT/name).read_bytes()).hexdigest()


def read(name):
    """UTF-8 자료는 엄격히 읽고 원래 MSBuild 인코딩의 로그만 대체 판독한다."""
    return (ROOT/name).read_text(encoding='utf-8',errors='replace' if name.endswith('.log') else 'strict')


def main():
    """현재 성공 로그/독립 입력이 준비된 단위만 기록하거나 정확한 바이트를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(read(REPORT.relative_to(ROOT).as_posix()))
        # 구현·원본·근거·실제 성공 로그 모두 저장 시점의 바이트인지 확인한다.
        for name,digest in report['sha256'].items():assert sha(name)==digest,name
        assert report['native_cases']==5310 and report['tests']['cases']==946 and report['tests']['failed_checks']==0
        assert not report['campaign_completed'] and not report['directdraw_fullscreen_changed']
        print(json.dumps({'verified':True,'tests':report['tests'],'native_cases':5310},ensure_ascii=False));return
    build=read(BUILD);assert 'netstorm_tests.vcxproj ->' in build and not re.search(r'\b(?:error|warning)\s+[A-Z]+\d+:',build)
    assert '100% tests passed' in read(CTEST)
    result=re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)',read(TESTS));assert result and int(result[1])==946 and int(result[2])==0
    files=SOURCES+[BUILD,CTEST,TESTS];counts={}
    # 고유 fixture 행 수와 native 도구가 기록한 현재 출처 SHA를 확인한다.
    for unit,(key,expected) in UNITS.items():
        name=f'cpppj/{unit}.json';evidence=json.loads(read(name));assert evidence[key]==expected
        fixture=f'cpppj/tests/fixtures/{unit}-x86.tsv';rows=[line for line in read(fixture).splitlines() if line and not line.startswith('#')]
        oracle=f'tools/decomp_{unit}_oracle.py'
        assert len(rows)==expected;counts[unit]=expected;files += [name,fixture,oracle]
        # Path 근거의 구현·내보낸 몸체·보호 PE를 모두 포함하고 fixture SHA도 별도로 확인한다.
        for group in ('files','sources','exports','protected_pe'):
            for path,digest in evidence.get(group,{}).items():assert sha(path)==digest,path;files.append(path)
        if 'fixture' in evidence:assert sha(fixture)==evidence['fixture'],fixture
    # 기존 Factory996개는 명칭 교정 뒤 같은 결과를 재사용했으며 신규 관찰에 더하지 않는다.
    reused='cpppj/factorypostpop.json';evidence=json.loads(read(reused));assert evidence['total']==996;files.append(reused)
    for path,digest in evidence['files'].items():assert sha(path)==digest,path;files.append(path)
    for prefix,count in FOCUSED.items():
        path=f'extracted/campaign-target-path-focused-{prefix}-final.log';assert f'{count} test(s), 0 failed check(s)' in read(path);files.append(path)
    report=dict(host=socket.gethostname(),last_decompile_host='VM-W11-CODEX',date='2026-10-13',tests=dict(cases=int(result[1]),failed_checks=0),
        native_units=counts,native_cases=sum(counts.values()),reused_factory_cases=996,gui_executions=0,
        campaign_completed=False,directdraw_fullscreen_changed=False,sha256={name:sha(name) for name in dict.fromkeys(files)},limits=[
            '표적 native는 실제 finder/각도/MayShoot를 실행하며 사정선 출력만 명시 관찰 경계다.',
            '미니맵 실제 큐/Refresh를 복원하고 Pop/Unpop에 연결했으나 bitmap 표시 재생성·실제 셀 그림은 필수 외부 경계다.',
            'Path 공유 버퍼와 서버 flags0 부착/송신을 연결했고 CD의 override 무시를 교정했다.',
            'Prepare48개는 원본게임 x87 53비트 결과이며 64비트의16개 1ULP 차이는 별도 보존한다.',
            'Factory 파생 wrapper는 아웃포스트126이며 실제 훨리베이스72는 별도 미복원 몸체다.',
            'GameWorld 비표면 이관·실제 지형 도장·Gun 애니메이션/투사체·사제 운반/의식/승패 및 캠페인1-1 완주가 남아 있다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'tests':report['tests'],'native_cases':5310,'campaign_completed':False},ensure_ascii=False))


# 명시 실행에서만 성공 근거를 생성하거나 감사한다.
if __name__=='__main__':main()
