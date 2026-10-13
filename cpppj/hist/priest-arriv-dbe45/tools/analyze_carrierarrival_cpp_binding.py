#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Carrier 도착406과 실제 사제/Path/Host 소비자의 현재 전체 출처를 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_carrierarrival_oracle import ROOT,FIXTURE,REPORT as NATIVE,sha
import analyze_genus_at_public_lineage as genus

REPORT=ROOT/'cpppj/carrierarrival-cpp-binding.json' # 현재 새도착 전수/같은Host 사제 소비자의 별도 C++ 근거다.
HISTORY=ROOT/'cpppj/hist/carrierarriv-91d3e' # 초기10smoke는 새전수406에 다시 합산하지 않는다.
TESTS=('cpppj/tests/CarrierArrivalTests.cpp','cpppj/tests/PriestPathArrivalHostTests.cpp') # 실제 전수와 같은Host 걷기/도착/삭제를 각각 구분한다.

def expected():
    """기존 getter 공개 이동/원본406과 같은 월드의 현재 전체 C++ source를 감사한다."""
    native=json.loads(NATIVE.read_text(encoding='utf-8'));assert native['total']==406 and not native['smoke'] and native['game_runs']==0
    # 원본 당시 전체 도구/부모/정밀내보내기/fixture를 현재 파일과 대조한다.
    for name,digest in native['files'].items():assert sha(ROOT/name)==digest,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==406 and len({(row[0],row[1]) for row in rows})==406
    assert all(item['normal_inputs']==203 and item['returns']==406 for item in native['editions'].values())
    assert json.loads(genus.REPORT.read_text(encoding='utf-8'))==genus.expected()
    initial=json.loads((HISTORY/'cpppj/carrierarrival.json').read_text(encoding='utf-8'));assert initial['total']==10 and initial['smoke']
    history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()};assert len(history)==5
    files={Path(__file__),NATIVE,FIXTURE,genus.REPORT,ROOT/'cpppj/tests/CarrierArrivalSupport.h',*(ROOT/name for name in TESTS)}
    # 실제 같은 풀에서 소비하는 기존/신규 HP/공간/경로/내용물/섬/부모몸체 전체를 현재SHA로 연결한다.
    for name in ('RawCarrierArrival','RawPriestArrival','RawPriestHalt','RawPathProcess','RawPathRuntime','RawPathAnimation','RawPathCleanup','RawCarrierPreDestroy',
                 'RawContainedCreate','RawContainedLookup','RawPlayerPlacementAnchor','RawPriestState','RawCarrierCheck','RawDamageablePostDestroy','RawSquidFinder','RawSquidNeighbors',
                 'RawPriestPostPop','RawPriestPostPopTail','RawPriestForcefield','RawCarrierPostPop','RawCanonPlacementTerrain','RawIslandRuntime','SquidPop','SquidUnpop','SquidPostPop','SquidReward','SquidFactory','SquidProcess','SidPool','SquidHash'):
        for suffix in ('.h','.cpp'):files.add(ROOT/f'cpppj/src/o/{name}{suffix}')
    files.add(ROOT/'cpppj/tests/RawSceneSupport.h')
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=406,assert_inputs=0,new_native_inputs=0,
                test_count=2,test_prefixes=['rawCarrierArrival_','rawPriestArrival_'],fixture_macro='NETSTORM_CARRIERARRIVAL_FIXTURE',initial_smoke_not_added=10,
                previous_genus_native_recount=0,existing_halt_native_recount=0,history=history,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},
                scope='원본406 전수는 최종 Native 보고서에서 한 번만 합산한다. 같은Host 소비자는 실제 사제VT/생성자/Pop/Unpop/사제전체PostPop/보호막조회/HP/Path 보간/실제Halt/사제Arrival→CarrierArrival1000/Path삭제/공간접두/cleanup/프로세스Form반납을 연결한다. 1×1·HP200·owner1·지면6·비포획·AI없음·다이스없음 합성 입력이며 중립섬복귀/임의귀환/포획·운반·경제·RPC·큰회피/의식307/룬304/전체 캠페인 자산·UI/음향 완료로 확장하지 않는다. 기존4412/Halt864/Bridge640/BFS/초기10smoke는 재합산0. 공유 C++ 빌드/실행 결과는 아직 대기다.')

def main(check):
    """현재 C++ 연결 자료를 생성하거나 원본/생산/검사 전체SHA를 감사한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Carrier 도착406/실제사제Host: 현재전체SHA/GenusAt기존4412불변/신규2CPP/추가관찰0')

# 명시 호출에서만 현재 C++ 근거를 기록하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');main(parser.parse_args().verify)
