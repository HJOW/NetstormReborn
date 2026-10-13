#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 같은 Host 검사의 include/배열 실체화만 정확 역변환해 이전 전체 출처와 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소의 전체 바이트를 읽는 기준이다.
HISTORY=ROOT/'cpppj/recovery-history/priest-arrival-before-test-syntax' # 두 번째 빌드의 전체 현재 생산/관찰/검사를 저장한다.
TEST='cpppj/tests/PriestPathArrivalHostTests.cpp' # 실제 같은 Host 걷기/도착/cleanup 검사의 문법만 바뀐다.
REPORT=ROOT/'cpppj/recovery-priest-arrival-test-syntax-lineage-evidence.json' # 이전 name/Carrier 보고서를 덮지 않는 현재 successor다.
PRIOR='cpppj/recovery-priest-arrival-name-lineage-evidence.json' # 당시 같은 검사 SHA를 포함한 이름 교정의 전수 근거다.
INCLUDE='#include "o/RawContainedLookup.h"\n' # 전방 선언뿐이던 현재 lookup 객체의 실제 정의를 읽는다.
BEFORE='owned->Runtime().SetPath({{20.75F,21.9F},{21.75F,21.9F}},{2,2},{0,0});' # 이전 동일 경로/방향/시각 입력이다.
AFTER='const std::array<RawPathPoint,2> pathPoints{{{20.75F,21.9F},{21.75F,21.9F}}};const std::array<std::int32_t,2> pathDirections{{2,2}};const std::array<double,2> pathTimes{{0,0}};owned->Runtime().SetPath(pathPoints,pathDirections,pathTimes);' # span이 읽을 같은 두 원소 배열을 실체화한다.
COMMENT='        // span 입력의 수명을 유지하는 실제 두 점/방향/시각 배열이며 이전 입력 비트는 그대로다.\n' # 새 배열 세 개의 용도/동일 입력을 설명한다.

def sha(path):
    """파일 전체 바이트 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def capture(logs):
    """검사 교정 전에 최신 name/Carrier/Native/생산/전수/Host/지원헤더와 종료 실패로그를 저장한다."""
    assert not HISTORY.exists()
    old=importlib.import_module('analyze_priest_arrival_name_lineage');data=json.loads((ROOT/PRIOR).read_text(encoding='utf-8'))
    assert data==old.expected()
    names={PRIOR,'tools/analyze_priest_arrival_name_lineage.py',TEST,*data['files']}
    # 이전 전체 문서도 보존하지만 성공 결과를 기록할 현재 문서는 소스 불변 감사와 구별한다.
    for name in ('docs/exe/cpp-priest-halt-20261014.md','docs/exe/cpp-carrier-arrival-audit-20261014.md'):names.add(name)
    for name in logs:
        resolved=(ROOT/name).resolve();relative=resolved.relative_to(ROOT).as_posix();assert relative.startswith('extracted/') and resolved.is_file()
        names.add(relative)
    saved={}
    # 모든 최초 전체 파일은 새 successor의 이름/SHA와 함께 저장한다.
    for name in sorted(names):
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes());saved[name]=sha(target)
    (HISTORY/'baseline.json').write_text(json.dumps(dict(files=saved,logs=list(logs)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Host 문법 교정 전 전체 보존:',len(saved))

def inverse():
    """include 한 줄과 동일 입력 배열 실체화만 되돌려 두 번째 빌드 검사 전체를 복원한다."""
    value=(ROOT/TEST).read_bytes();changes=((INCLUDE,''),(COMMENT,''),(AFTER,BEFORE))
    # 각 변경은 고유한 전체 문장 조각 하나로 제한한다.
    for after,before in changes:
        token=after.encode('utf-8');assert value.count(token)==1,after;value=value.replace(token,before.encode('utf-8'),1)
    assert value==(HISTORY/TEST).read_bytes();return value

def expected():
    """생산/Native/fixture/기존 근거 불변과 이전 name.expected를 당시 검사 read view에서 재현한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));restored=inverse()
    for name,digest in baseline['files'].items():
        assert sha(HISTORY/name)==digest,name
        if name==TEST:assert hashlib.sha256(restored).hexdigest()==digest
        elif not name.startswith(('docs/','extracted/campaign-runtime14-')):assert sha(ROOT/name)==digest,name
    old=importlib.import_module('analyze_priest_arrival_name_lineage');original=Path.read_bytes
    def historical(path):
        """이전 이름 감사에서 변경된 검사 하나만 저장한 전체 이전 파일로 읽는다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:assert json.loads(old.REPORT.read_text(encoding='utf-8'))==old.expected()
    finally:Path.read_bytes=original
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,carrier_normal_recount=0,halt_normal_recount=0,
                preserved_carrier_normal_inputs=406,preserved_halt_normal_inputs=864,whole_test_exact_inverse=True,
                production_and_native_fixture_bytes_unchanged=True,prior_name_and_carrier_binding_reproduced=True,
                current_test_sha256=sha(ROOT/TEST),previous_test_sha256=sha(HISTORY/TEST),prior_name_sha256=sha(ROOT/PRIOR),tool_sha256=sha(Path(__file__)),
                files={name:sha(ROOT/name) for name in baseline['files'] if not name.startswith(('docs/','extracted/campaign-runtime14-'))},
                history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='RawContainedLookup 정의 include 한 줄을 추가하고 기존 두 점 float/방향int32/시각double의 같은 입력을 const std::array로 실체화해 span에 전달한다. 새 계산/입력 변경0, 원본406·864 및 생산body/fixture/기존name·Carrier 보고서는 불변이다. 이전 검사 전체 read view로 name.expected와 그 내부 Carrier binding.expected를 재현한다.')

def main(arguments):
    """교정 전 전체 보존 또는 현재 최소 검사 문법 교정 successor 감사를 수행한다."""
    if arguments.capture:capture(arguments.log);return
    data=expected()
    if arguments.verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Host 문법: include1/동일배열입력/전체검사정확역변환/생산·Native불변/이전name+binding재현')

# 명시 CLI 호출에서만 전체 원문 저장/현재 successor 생성/감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true')
    parser.add_argument('--log',action='append',default=[]);parser.add_argument('--verify',action='store_true');main(parser.parse_args())
