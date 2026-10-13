#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 Halt 입력 지면을 hash1에서 분리하고 동일864 입력의 전체 원문을 연결한다."""
import argparse
import importlib
import json
from pathlib import Path
from decomp_priesthalt_oracle import ROOT,FIXTURE,REPORT as NATIVE,sha

HISTORY=ROOT/'cpppj/hist/priesthalt-b-f9d20' # 지면/hash1 충돌 발견 전 전체35파일과 바이트 manifest다.
REPORT=ROOT/'cpppj/priesthalt-spots-successor.json' # 최종 Native864와 변경 없는 C++ 검사·생산의 현재 successor다.
SOURCE='tools/decomp_priesthalt_oracle.py' # 지면 주소만 격리하는 현재 제한 실행기다.
PREVIOUS='cpppj/priesthalt-cpp-binding.json' # 초기 C++ 연결과 CD48 입력 교정의 당시 보고서다.

def rows(path):
    """주석 외 입력/반환/사건/전체 부모 raw를 그대로 읽는다."""
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]

def expected():
    """최소 주소 교정과 이전 전체 보고서·검사의 정확 재현을 감사한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));assert len(baseline)==35
    changed_names={SOURCE,'cpppj/priesthalt.json','cpppj/tests/fixtures/priesthalt-x86.tsv'}
    # 원본 PE는 읽기 전용 위치에 유지하고 이전 전체 C++/생산/도구/보고서/본문은 이력에서 감사한다.
    for name,value in baseline.items():
        assert sha(HISTORY/name)==value,name
        if name not in changed_names and not name.startswith('docs/'):assert sha(ROOT/name)==value,name
    value=(ROOT/SOURCE).read_bytes()
    changes=[('HALT_SPOTS=0x11850000 # 기존 Finder hash1과 겹치지 않는 독립 지면 입력 주소다.\n',''),
             ("        self.mu.mem_map(HALT_SPOTS,0x10000);self.put(self.g['spots'],HALT_SPOTS)\n",''),
             ('self.mu.mem_write(HALT_SPOTS,bytes(65536));','self.mu.mem_write(SPOTS,bytes(65536));'),
             ('self.put(HALT_SPOTS+int(y+','self.put(SPOTS+int(y+')]
    # 네 고유 주소 준비 조각만 역변환한다. 호출·레지스터·입력·효과 경계는 그대로다.
    for after,before in changes:
        encoded=after.encode('utf-8');assert value.count(encoded)==1,after;value=value.replace(encoded,before.encode('utf-8'),1)
    assert value==(HISTORY/SOURCE).read_bytes()
    old=importlib.import_module('analyze_priesthalt_cpp_binding');read_bytes,read_text=Path.read_bytes,Path.read_text
    def previous_path(path):
        """첫 연결 감사의 저장된 원문만 역사 경로에서 읽으며 모든 파일 쓰기는 하지 않는다."""
        try:name=path.resolve().relative_to(ROOT).as_posix()
        except ValueError:return path
        candidate=HISTORY/name
        return candidate if candidate.is_file() else path
    def historical_bytes(path):
        """이전 첫 보고서의 전체 바이트를 재현한다."""
        return read_bytes(previous_path(path))
    def historical_text(path,*arguments,**keywords):
        """이전 원본/연결 JSON도 당시 그대로 읽어 첫 보고서를 덮어쓰지 않는다."""
        return read_text(previous_path(path),*arguments,**keywords)
    Path.read_bytes,Path.read_text=historical_bytes,historical_text
    try:assert json.loads(old.REPORT.read_text(encoding='utf-8'))==old.expected()
    finally:Path.read_bytes,Path.read_text=read_bytes,read_text
    current=rows(FIXTURE);previous=rows(HISTORY/'cpppj/tests/fixtures/priesthalt-x86.tsv');assert len(current)==len(previous)==864
    changed,result_changes=0,0
    # 원래864 입력/부모 raw는 전량 불변이며 충돌이 지웠던 운반 후보 요청과 반환만 달라진다.
    for left,right in zip(previous,current):
        assert left[:2]==right[:2] and left[4]==right[4]
        if left!=right:
            case=json.loads(left[1]);assert case['kind']=='Common' and case['neighbor'] in (156,187)
            effect=f'{"C" if case["neighbor"]==156 else "N"}:60:{case["reply"]}'
            assert left[3] in ('-','F:50:0') and right[3]==('' if left[3]=='-' else left[3]+';')+effect
            changed+=1;result_changes+=left[2]!=right[2]
    assert changed==120 and result_changes==50
    native=json.loads(NATIVE.read_text(encoding='utf-8'));assert native['total']==864 and not native['smoke'] and native['game_runs']==0
    # 최종 현재 원본/생산/도구와 독립 fixture를 모두 감사한다.
    for name,digest in native['files'].items():assert sha(ROOT/name)==digest,name
    files={Path(__file__),ROOT/PREVIOUS,NATIVE,FIXTURE,ROOT/SOURCE,ROOT/'cpppj/tests/PriestHaltTests.cpp'}
    files.update(ROOT/name for name in baseline if name.startswith('cpppj/src/') or name=='cpppj/tests/RawSceneSupport.h')
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=864,assert_inputs=0,new_native_inputs=0,
                test_count=1,test_prefix='rawPriestHalt_',fixture_macro='NETSTORM_PRIESTHALT_FIXTURE',whole_previous_oracle_inverse=True,production_and_test_unchanged=True,
                original_inputs_identical=864,entire_parent_raw_identical=864,output_rows_changed=120,return_rows_changed=50,prior_864_recount=0,
                files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='최종864를 원본보고서에서 한 번만 합산한다. 이전 미등록864/초기20smoke/기존CarrierCheck·HP·finder 관찰은 재합산0. hash1과 지면 충돌의 전체35원문을 보존하고 지면주소만 분리한다. 같은864 입력/부모raw는 동일하며 특정후보 호출120행/반환50행만 변경된다. 실제 가상공간/낙하·큰포획/너겟전달은 여전히 명시 외부 입력이고 전체걷기/Arrival/포획/RPC/캠페인 완료는 후속이다.')

def main(check):
    """최종 현재 source/fixture와 이전 전체 바이트 successor를 생성하거나 검증한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Halt 지면분리: 전체35파일/정확역변환/동일864입력·raw/효과120·반환50교정/현재SHA')

# 명시 호출에서만 최종 현재 근거를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true');main(parser.parse_args().verify)
