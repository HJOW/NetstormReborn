#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 Halt864 현재 C++ 연결과 CD 타입 입력 교정의 전체 이전 바이트를 감사한다."""
import argparse
import json
from pathlib import Path
from decomp_priesthalt_oracle import ROOT,FIXTURE,REPORT as NATIVE,sha

REPORT=ROOT/'cpppj/recovery-priesthalt-cpp-binding-evidence.json' # 전수 검사와 원본864를 재합산 없이 연결한다.
HISTORY=ROOT/'cpppj/recovery-history/priesthalt-before-cd-input' # CD 비특정 후보 타입을 범위 안170으로 준비하기 전 전체5파일이다.
SMOKE=ROOT/'cpppj/recovery-history/priesthalt-initial-smoke' # 처음20smoke는 전수 입력에 다시 합산하지 않는다.
TEST=ROOT/'cpppj/tests/PriestHaltTests.cpp' # 같은 HP/CarrierCheck/일반 finder를 재사용하는 검사다.

def rows(path):
    """주석 외 모든 입력/정상 반환/사건/전체 raw 행을 그대로 읽는다."""
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]

def expected():
    """현재 출처와 이전 전체 Native/생산/입력 교정의 정확 역변환을 확인한다."""
    native=json.loads(NATIVE.read_text(encoding='utf-8'));assert native['total']==864 and not native['smoke'] and native['game_runs']==0
    # 현재 전체 원본 몸체/PE/도구/생산/fixture SHA를 Native 보고서와 일치시킨다.
    for name,value in native['files'].items():assert sha(ROOT/name)==value,name
    old=json.loads((HISTORY/'cpppj/recovery-priesthalt-evidence.json').read_text(encoding='utf-8'));assert old['total']==864 and not old['smoke']
    # 이전 미등록 전수 자료 전체를 당시 SHA로 보존하며 생산 두파일은 현재도 불변이다.
    for name,value in old['files'].items():
        path=HISTORY/name if (HISTORY/name).is_file() else ROOT/name
        assert sha(path)==value,name
    for suffix in ('.h','.cpp'):
        name=f'cpppj/src/o/RawPriestHalt{suffix}';assert (ROOT/name).read_bytes()==(HISTORY/name).read_bytes()
    source='tools/decomp_priesthalt_oracle.py';value=(ROOT/source).read_bytes()
    before='        for index,value in enumerate(cases):\n'
    after="        for index,input_value in enumerate(cases):\n            # CD에는 타입187이 없으므로 특정 타입과 다른 후보는 실제 범위 끝170으로 준비한다.\n            value=dict(input_value,neighbor=170) if edition=='originalCD' and input_value['neighbor']==187 else input_value\n"
    assert value.count(after.encode('utf-8'))==1
    assert value.replace(after.encode('utf-8'),before.encode('utf-8'))==(HISTORY/source).read_bytes()
    current=rows(FIXTURE);previous=rows(HISTORY/'cpppj/tests/fixtures/priesthalt-x86.tsv');assert len(current)==len(previous)==864
    changed=0
    # CD48개 입력의 비특정 후보 번호187만170으로 바꾸며 모든 정상 반환/사건/부모 raw는 동일하다.
    for left,right in zip(previous,current):
        assert left[0]==right[0] and left[2:]==right[2:]
        one,two=json.loads(left[1]),json.loads(right[1])
        if one!=two:
            assert left[0]=='originalCD' and one['neighbor']==187 and two['neighbor']==170
            assert dict(one,neighbor=170)==two;changed+=1
    assert changed==48
    files={Path(__file__),TEST,NATIVE,FIXTURE,ROOT/source}
    # 실제 소비한 기존 HP/지면/일반 finder의 전체 현재 몸체도 검사 SHA에 연결한다.
    for name in ('RawPriestHalt','RawPriestState','RawCarrierCheck','RawSquidFinder','SquidReward','SidPool','SquidHash'):
        for suffix in ('.h','.cpp'):files.add(ROOT/f'cpppj/src/o/{name}{suffix}')
    files.add(ROOT/'cpppj/tests/RawSceneSupport.h')
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=864,assert_inputs=0,new_native_inputs=0,
                test_count=1,test_prefix='rawPriestHalt_',fixture_macro='NETSTORM_PRIESTHALT_FIXTURE',cd_input_rows_corrected=48,
                initial_smoke_not_added=20,initial_unregistered_native_not_added=864,whole_previous_oracle_inverse=True,entire_original_outputs_identical=True,production_unchanged=True,
                files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},history={path.relative_to(ROOT).as_posix():sha(path) for folder in (HISTORY,SMOKE) for path in sorted(folder.rglob('*')) if path.is_file()},
                scope='원본864는 최종 Native 보고서에서 한 번만 합산한다. 기존CarrierCheck/HP/finder·20smoke·이전 미등록864는 새합산0. 전체사제Halt/공통검색 반환을 대조하되 가상공간/낙하·큰포획/너겟전달은 명시 외부 입력이다. 실제 같은Host 걷기/CarrierArrival/전체포획/RPC/UI/캠페인 완료는 후속이다.')

def main(check):
    """현재 C++ 연결 보고서를 생성하거나 이전 전체 입력 교정과 현재 SHA를 검증한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Halt864: 현재전체SHA/이전Oracle정확역변환/CD48입력·모든출력동일/새합산0')

# 명시 호출에서만 현재 C++ 연결 근거를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true');main(parser.parse_args().verify)
