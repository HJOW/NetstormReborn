#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 실제 캠페인 사제 경로 연결 준비 소스와 기존 원본 몸체 출처를 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path
import analyze_priest_arrival_test_syntax_lineage as prior

ROOT=Path(__file__).resolve().parents[1] # 같은 workspace의 준비 소스/보고서 바이트를 읽는다.
REPORT=ROOT/'cpppj/campaign-path-bindings.json' # 새 원본0/CPP미실행의 준비 연결 자료다.

def sha(path):
    """파일 전체 SHA256을 계산하며 토큰 일부만으로 현재 출처를 대신하지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def expected():
    """새 연결부와 기존 소비 모듈의 전체 현재 SHA 및 이전 최소 교정 근거를 감사한다."""
    assert json.loads(prior.REPORT.read_text(encoding='utf-8'))==prior.expected()
    names={Path(__file__).relative_to(ROOT).as_posix(),prior.REPORT.relative_to(ROOT).as_posix(),
           'cpppj/src/client/RawCampaignPathBindings.h','cpppj/src/client/RawCampaignPathBindings.cpp',
           'cpppj/src/client/RawSurfaceWorld.h','cpppj/src/client/RawSurfaceWorld.cpp',
           'cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp'}
    # 실제 callback/시계/자료를 소비하는 기존 모듈 전체를 각각 기록한다.
    for name in ('RawPathStart','RawPathSearch','RawPathProcess','RawPathRuntime','RawPathData','RawPathCleanup',
                 'RawPriestHalt','RawPriestArrival','RawPriestState','RawContainedLookup','SquidProcess','SquidFrame','SidPool','SquidHash'):
        for suffix in ('.h','.cpp'):names.add(f'cpppj/src/o/{name}{suffix}')
    header=(ROOT/'cpppj/src/client/RawCampaignPathBindings.h').read_text(encoding='utf-8')
    source=(ROOT/'cpppj/src/client/RawCampaignPathBindings.cpp').read_text(encoding='utf-8')
    assert 'o::RawPathStartState& startState_' in header and 'o::RawPathSearch search_' in header
    # 조립 경계를 확인하며 정적 문자열 검사 자체를 원본 실행 관찰로 세지 않는다.
    for token in ('o::MakePathSearchHooks','start_->Start(request)','halt_.Halt(sid)','arrival_.Arrival(sid,first,second)',
                  'cleanup_.AfterSpatial(runtime,parent)','lookup_.Find(parent,kActualPriestProcess)','o::MakePathFormTransmission'):
        assert token in source,token
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,new_cpp_tests=0,
                existing_native_recount=0,existing_production_edits=0,cpp_build_and_consumer_pending=True,
                prior_syntax_sha256=sha(prior.REPORT),tool_sha256=sha(Path(__file__)),files={name:sha(ROOT/name) for name in sorted(names)},
                scope='새 client 경로 연결부만 준비한다. 같은 실제 Core/Host/Kernel/BFS/PathStart/Process/Halt/Arrival/cleanup/전송을 조립하며 RawPathStartState는 root의 동일 상태를 직접 참조한다. 기존 원본/CPP 관찰을 재합산하지 않으며 단위15 공유 빌드/실제172 설치 및 사제 명령 소비자 검사는 아직 필요하다. 중립귀환/포획/큰운반/겹침/음향/다른 이동 부모는 명시 caller 경계다.')

def main(check):
    """준비 소스 자료를 작성하거나 이전 전체 교정/현재 SHA를 검증한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('캠페인 사제 경로 준비: 동일Core/기존몸체조립/현재전체SHA/새Native0/CPP소비자대기')

# 명시 CLI에서만 준비 소스의 전체 출처를 기록하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');main(parser.parse_args().verify)
