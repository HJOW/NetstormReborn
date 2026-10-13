#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건물 공간·postPop·SP·사제 연결·건설 도착 단위의 실제 성공 로그와 SHA를 기록한다."""
import argparse
import hashlib
import json
import re
import socket
from pathlib import Path

# 이번 작업 단위의 근거를 이전 UI 작업 근거와 별도로 보존한다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/campaign-world.json'
# 실행 당시 실제 구현/호스트 연결/검사의 byte identity를 감사한다.
SOURCES=[
    'cpppj/src/o/SidPool.h','cpppj/src/o/RiftType.h','cpppj/src/o/RiftTypeTable.cpp',
    'cpppj/src/o/RawBuildingSpace.h','cpppj/src/o/RawBuildingSpace.cpp',
    'cpppj/src/o/SquidPop.h','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidUnpop.h','cpppj/src/o/SquidUnpop.cpp',
    'cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp',
    'cpppj/src/o/RawAssetPostPop.h','cpppj/src/o/RawAssetPostPop.cpp','cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp',
    'cpppj/src/o/RawConstructionArrival.h','cpppj/src/o/RawConstructionArrival.cpp',
    'cpppj/src/o/RawConstructionBuilder.h','cpppj/src/o/RawConstructionBuilder.cpp','cpppj/tests/ConstructionBuilderTests.cpp',
    'cpppj/src/client/RawSurfaceWorld.cpp','cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp',
    'cpppj/tests/BuildingSpaceTests.cpp','cpppj/tests/AssetPostPopTests.cpp','cpppj/tests/SpRotateTests.cpp',
    'cpppj/tests/RawPriestWorldBindingsTests.cpp','cpppj/tests/ConstructionArrivalTests.cpp',
    'cpppj/src/o/CMakeLists.txt','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
    'tools/analyze_campaign_world.py',
]
# 각 native 도구의 기록 형식에 맞는 독립 입력수다. 재실행/함수 내부 호출수는 더하지 않는다.
UNITS={'buildingspace':'total_cases','assetpostpop':'total','sprotate':'cases','priestworldevents':'cases','constructionarrival':'total','constructiontimer':'total'}


def sha(path):
    """개행/인코딩을 정규화하지 않고 저장 byte의 SHA256을 계산한다."""
    return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()


def main():
    """실제 전체 성공 뒤에만 저장하며 verify에서는 구현·자료·로그·보호 PE를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        # 모든 기록 파일은 생성 당시 실제 byte와 같아야 한다.
        for path,digest in report['sha256'].items(): assert sha(path)==digest,path
        assert not report['campaign_completed'] and not report['directdraw_fullscreen_changed'] and report['tests']['failed_checks']==0
        assert sum(report['native_units'].values())==report['native_cases']
        print(json.dumps(dict(verified=True,tests=report['tests'],native_cases=report['native_cases']),ensure_ascii=False));return
    build_path='extracted/campaign-world-build-final.log';ctest_path='extracted/campaign-world-ctest-final.log'
    build=(ROOT/build_path).read_text(encoding='utf-8',errors='replace')
    log=(ROOT/ctest_path).read_text(encoding='utf-8',errors='replace')
    assert 'netstorm_tests.vcxproj ->' in build and not re.search(r'\b(?:error|warning)\s+[A-Z]+\d+:',build)
    assert '100% tests passed' in log
    result=re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)',(ROOT/'cpppj/build/Testing/Temporary/LastTest.log').read_text(encoding='utf-8',errors='replace'))
    assert result and int(result[2])==0
    report=dict(host=socket.gethostname(),last_decompile_host='VM-W11-CODEX',date='2026-10-13',
        campaign_completed=False,directdraw_fullscreen_changed=False,tests=dict(cases=int(result[1]),failed_checks=int(result[2])),
        native_units={},native_cases=0,sha256={},gui_executions=0,
        limits=['비표면 GameWorld 객체 이관/원본 Fort side-effect 로더는 후속',
            '건설 도착 몸체 복원은 type23 경로 탐색·이동/입력/UI 연결 완성을 의미하지 않음',
            'GunProcess/타입별 Regular/비zero damageEffect 및 중립 사제 행동 일부 실제 연결은 후속',
            '각 독립 native fixture의 명시 하위 경계 관찰은 게임 종단 완주와 구별'])
    files=list(SOURCES)+[build_path,ctest_path,'originals/Netstorm.exe','originalCD/NETSTORM.EXE']
    # 실제 저장된 독립 입력수를 읽고 모든 도구/fixture 근거의 바이트를 함께 보존한다.
    for name,key in UNITS.items():
        evidence=f'cpppj/{name}.json';data=json.loads((ROOT/evidence).read_text(encoding='utf-8'))
        count=data[key];assert isinstance(count,int) and count>0;report['native_units'][name]=count
        files.extend((evidence,f'cpppj/tests/fixtures/{name}-x86.tsv',f'tools/decomp_{name}_oracle.py'))
    report['native_cases']=sum(report['native_units'].values())
    # 동일 경로는 한 번만 SHA를 계산하며 다른 관찰 입력으로 중복 계산하지 않는다.
    for path in dict.fromkeys(files): report['sha256'][path]=sha(path)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(tests=report['tests'],native_cases=report['native_cases'],campaign_completed=False),ensure_ascii=False))


# 명시 실행에서만 완료 기록 또는 감사 결과를 만든다.
if __name__=='__main__': main()
