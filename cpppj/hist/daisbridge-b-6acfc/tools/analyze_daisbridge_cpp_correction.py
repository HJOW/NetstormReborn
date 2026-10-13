#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dais Bridge 첫 C++ 구성 교정과 경로 진단을 전체 이전 원문에 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소 전체 바이트를 읽는 기준이다.
HISTORY=ROOT/'cpppj/hist/daisbridge-i-42bda' # 첫 실행의 원본640/검사/생산/실패 로그 전체106파일이다.
TEST='cpppj/tests/DaisBridgeTests.cpp' # 실제 PathStart/Prepare/Host/cleanup을 정확 비트로 비교하는 소비자 검사다.
BINDING='cpppj/daisbridge-cpp-binding.json' # 첫 실행 당시 검사 SHA를 보존하는 연결 보고서다.
NATIVE='cpppj/daisbridge.json' # 원본640과 두 x87 결과 전체를 보존하는 보고서다.
REPORT=ROOT/'cpppj/daisbridge-cpp-correction.json' # 첫 최소 구성 교정/진단의 독립 successor다.
COMMENT='        // Native 합성 VT는 C++ 표시 분배에서 판본별 일반 VT로 준비하고 실제 Dais VT는 아래에서 유지한다.\n' # 일반 사제 합성 표시 입력의 범위를 설명한다.

def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def restored():
    """판본별 일반 VT와 진단 문장만 역제거해 첫 검사 전체 원문을 복원한다."""
    value=(ROOT/TEST).read_bytes()
    changes=[
        ('#include <iostream>\n',''),
        (COMMENT,''),
        ('Put(raw,0,patch ? 0x501dd0U : 0x5058d8U);','Put(raw,0,kVtable);'),
        ('if (static_cast<std::uint32_t>(current->Runtime().Count())!=Number(row[3]) || BridgePoints(current->Runtime())!=row[5]) std::cerr<<"Bridge 경로 진단 "<<row[0]<<\' \'<<row[1]<<" actual="<<current->Runtime().Count()<<\':\'<<BridgePoints(current->Runtime())<<" expected="<<row[3]<<\':\'<<row[5]<<\'\\n\';',''),
    ]
    # 네 고유 교정 조각의 정확한 이전 값만 되돌린다.
    for after,before in changes:
        current=after.encode('utf-8')
        assert value.count(current)==1,after
        value=value.replace(current,before.encode('utf-8'),1)
    assert value==(HISTORY/TEST).read_bytes()
    return value

def expected():
    """이전640/x87/생산 바이트 불변과 현재 진단 검사만 별도 연결한다."""
    history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()}
    assert len(history)==106,len(history)
    restored()
    old=json.loads((HISTORY/BINDING).read_text(encoding='utf-8'))
    assert (ROOT/BINDING).read_bytes()==(HISTORY/BINDING).read_bytes()
    native=json.loads((HISTORY/NATIVE).read_text(encoding='utf-8'))
    assert native['total']==640 and not native['smoke'] and native['game_runs']==0
    assert (ROOT/NATIVE).read_bytes()==(HISTORY/NATIVE).read_bytes()
    unchanged={}
    # 첫 보고서의 전체 생산/도구/관찰/fixture는 당시 원문과 현재가 일치해야 한다.
    for name,value in old['files'].items():
        assert sha(HISTORY/name)==value,name
        if name!=TEST:
            assert sha(ROOT/name)==value,name
            unchanged[name]=value
    binding=importlib.import_module('analyze_daisbridge_cpp_binding')
    original=Path.read_bytes
    def historical(path):
        """정확 역변환한 검사만 당시 원문을 읽고 원본640과 생산은 현재에서 감사한다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:
        assert json.loads(binding.REPORT.read_text(encoding='utf-8'))==binding.expected()
    finally:
        Path.read_bytes=original
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=640,assert_inputs=0,new_native_inputs=0,
                x87_different_inputs=18,x87_different_time_words=26,whole_previous_test_inverse=True,production_and_native_fixture_unchanged=True,
                previous_test_sha256=sha(HISTORY/TEST),current_test_sha256=sha(ROOT/TEST),tool_sha256=sha(Path(__file__)),
                previous_binding_sha256=sha(ROOT/BINDING),original_native_sha256=sha(ROOT/NATIVE),history=history,unchanged_files=unchanged,
                changes='C++ 표시 입력의 Native 합성 VT를 실제 판본 일반 VT로 준비한다. 실제 Dais VT/일반 사제 합성 범위는 유지한다. 정확 비교 실패의 count/좌표/방향/시각 비트를 출력하는 진단만 추가한다.',
                scope='새 관찰0. 원본640/fixture/tool과 전체 x87 결과는 불변이다. 목표0x027f의 double 시각 비트 CHECK를 유지하며 0x037f의18입력/26단어 차이를 허용오차로 성공 처리하지 않는다. 실제 전체 사제/걷기/가상도착/캠페인 metadata는 후속이다.')

def main(check):
    """명시 CLI에서 전체 이전 바이트와 현재 검사 교정 보고서를 생성하거나 검증한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Dais Bridge C++ 교정: 전체106파일/정확역변환/원본640·생산불변/현재진단/추가0')

# 명시 호출에서만 현재 successor를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    main(parser.parse_args().verify)
