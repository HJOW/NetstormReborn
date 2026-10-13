#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dais Bridge 표면 AP 입력 교정/진단 제거를 이전 전체 원문과 정확 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소의 전체 원문을 읽는 기준이다.
HISTORY=ROOT/'cpppj/recovery-history/daisbridge-before-surface-input' # 두 번째 실패와 첫 successor의 전체110파일을 보존한다.
TEST='cpppj/tests/DaisBridgeTests.cpp' # 원본 최종 AP 코드로 실제 BFS 경로/시각을 비교하는 검사다.
REPORT=ROOT/'cpppj/recovery-daisbridge-surface-input-successor-evidence.json' # 현재 검사와 원본640의 별도 successor다.
COMMENT='    // 원본 입력의 surface83/사제158 코드 포인터가 공유되어 최종 표면 코드도 AP가 된다.\n' # 결과를 바꾸는 입력 코드 공유를 설명한다.
DIAGNOSTIC='if (static_cast<std::uint32_t>(current->Runtime().Count())!=Number(row[3]) || BridgePoints(current->Runtime())!=row[5]) std::cerr<<"Bridge 경로 진단 "<<row[0]<<\' \'<<row[1]<<" actual="<<current->Runtime().Count()<<\':\'<<BridgePoints(current->Runtime())<<" expected="<<row[3]<<\':\'<<row[5]<<\'\\n\';' # 이전 실패의 전체 비트 출력 문장이다.

def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def restored():
    """AP 코드/설명과 진단 제거만 되돌려 두 번째 실행의 검사 전체를 복원한다."""
    value=(ROOT/TEST).read_bytes()
    changes=[(COMMENT,''),("frames[83]=RiftTypeFrames({{'A','P',1,0}});","frames[83]=RiftTypeFrames({{'A','A',1,0}});"),
             ('#include "o/SquidUnpop.h"\n','#include "o/SquidUnpop.h"\n#include <iostream>\n'),
             ('if (current) { CHECK(static_cast<std::uint32_t>(current->Runtime().Count())==Number(row[3])',
              'if (current) { '+DIAGNOSTIC+'CHECK(static_cast<std::uint32_t>(current->Runtime().Count())==Number(row[3])')]
    # 네 고유 변경 조각 외 모든 비교 조건/metadata/명령/공간/Host 소비자는 불변이어야 한다.
    for after,before in changes:
        current=after.encode('utf-8');assert value.count(current)==1,after
        value=value.replace(current,before.encode('utf-8'),1)
    assert value==(HISTORY/TEST).read_bytes()
    return value

def expected():
    """원본640과 첫 successor를 덮어쓰지 않고 현재 최소 입력 교정만 연결한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));assert len(baseline)==110
    # 이전 전체 파일을 감사하고 두 번째 실행 당시 생산/Native/보고서/도구 불변을 확인한다.
    for name,value in baseline.items():
        assert sha(HISTORY/name)==value,name
        if name!=TEST and not name.startswith('docs/'):assert sha(ROOT/name)==value,name
    restored()
    old=importlib.import_module('analyze_daisbridge_cpp_correction')
    original=Path.read_bytes
    def historical(path):
        """현재 검사만 정확 복원된 두 번째 실행 원문을 읽고 첫 감사는 그대로 재현한다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:
        assert json.loads(old.REPORT.read_text(encoding='utf-8'))==old.expected()
    finally:
        Path.read_bytes=original
    native_name='cpppj/recovery-daisbridge-evidence.json';native=json.loads((ROOT/native_name).read_text(encoding='utf-8'))
    assert native['total']==640 and len(native['precision_differences'])==18
    source='tools/decomp_daisbridge_oracle.py';text=(ROOT/source).read_text(encoding='utf-8')
    assert "codes=CODES+(typ-158)*0x1000" in text and "self.table(158,'AEKLMNOP'+'P'*24)" in text
    parent=(ROOT/'tools/decomp_pathstart_oracle.py').read_text(encoding='utf-8')
    assert 'TYPES+83*self.type_stride+0x124,CODES' in parent
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=640,assert_inputs=0,new_native_inputs=0,
                whole_previous_test_inverse=True,production_and_native_fixture_unchanged=True,diagnostic_removed=True,
                previous_test_sha256=sha(HISTORY/TEST),current_test_sha256=sha(ROOT/TEST),tool_sha256=sha(Path(__file__)),
                original_native_sha256=sha(ROOT/native_name),prior_correction_sha256=sha(old.REPORT),history={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(HISTORY.rglob('*')) if p.is_file()},
                original_normal_recount=0,x87_different_inputs=18,x87_different_time_words=26,
                changes='원본 합성 입력의 surface83/사제158 프레임 코드 포인터는 CODES를 공유하므로 최종 표면 코드는 AP(41500100)다. C++ 입력만 AA→AP로 교정하고 일시 진단 출력/iostream을 제거한다.',
                scope='원본640/fixture/tool/보고서와 첫 successor 원문 보존. 새원본 입력0. 좌표/방향/double 시각 전체의 목표0x027f 정확 비트 CHECK 유지. 다른0x037f의18입력/26단어 ±1ULP는 별도 전체 관찰이며 허용오차를 추가하지 않는다. 실제 전체 사제/걷기/도착/캠페인 자산은 후속이다.')

def main(check):
    """명시 CLI에서 현재 최소 입력 교정 successor를 생성하거나 전체 바이트를 감사한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Dais Bridge AP 입력: 이전전체110파일/정확역변환/생산·원본640 불변/진단제거/새관찰0')

# 명시 호출에서만 현재 successor를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    main(parser.parse_args().verify)
