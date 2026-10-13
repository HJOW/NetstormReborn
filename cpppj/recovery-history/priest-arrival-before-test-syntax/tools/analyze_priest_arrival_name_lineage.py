#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 Arrival의 로컬 상수 이름만 바꾼 결과를 최초 빌드 전체 원문과 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 workspace에서 전체 바이트를 읽는 기준이다.
SOURCE='cpppj/src/o/RawPriestArrival.cpp' # ChunkMap의 전역 kNoIsland와 충돌한 로컬 선언/사용만 교정한다.
HISTORY=ROOT/'cpppj/recovery-history/priest-arrival-before-name-correction' # 최초 생산/보고서/fixture/검사/도구/로그 전체를 보존한다.
REPORT=ROOT/'cpppj/recovery-priest-arrival-name-lineage-evidence.json' # 기존 Carrier binding을 덮지 않는 현재 successor다.
BINDING='cpppj/recovery-carrierarrival-cpp-binding-evidence.json' # 원본406과 같은 Host 소비자의 최초 전체 출처다.
NATIVE='cpppj/recovery-carrierarrival-evidence.json' # 원본406의 입력/출력/몸체/도구를 그대로 유지한다.
HALT='cpppj/recovery-priesthalt-spots-successor-evidence.json' # 별도 원본864의 입력 공간 교정 출처도 보존한다.
OLD=b'kNoIsland' # 이전 전체 cpp에 정확 두 번 나타나는 로컬 이름이다.
NEW=b'kPriestNoIsland' # 공통 ChunkMap 전역과 구별한 사제 전용 이름이다.

def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def capture(logs):
    """교정 전에 최초 binding의 전체 의존 자료와 최종 실패 로그를 별도 저장한다."""
    assert not HISTORY.exists()
    assert (ROOT/SOURCE).read_bytes().count(OLD)==2 and NEW not in (ROOT/SOURCE).read_bytes()
    prior=importlib.import_module('analyze_carrierarrival_cpp_binding')
    assert json.loads((ROOT/BINDING).read_text(encoding='utf-8'))==prior.expected()
    names={BINDING,NATIVE,HALT,'tools/analyze_carrierarrival_cpp_binding.py',
           'tools/analyze_genus_at_public_lineage.py','cpppj/recovery-genus-at-public-lineage-evidence.json',
           'tools/analyze_priesthalt_spots_successor.py','tools/analyze_priesthalt_cpp_binding.py',
           'cpppj/recovery-priesthalt-cpp-binding-evidence.json','cpppj/recovery-priesthalt-evidence.json',
           'cpppj/tests/PriestHaltTests.cpp','docs/exe/cpp-priest-halt-20261014.md',
           'docs/exe/cpp-carrier-arrival-audit-20261014.md'}
    protected={}
    # 최초 보고서가 참조하는 전체 현재 파일을 저장하며 보호 PE는 원래 경로의 SHA로만 감사한다.
    for report in (BINDING,NATIVE,HALT):
        data=json.loads((ROOT/report).read_text(encoding='utf-8'))
        for name,digest in data['files'].items():
            assert sha(ROOT/name)==digest,name
            if name.startswith(('originals/','originalCD/')):protected[name]=digest
            else:names.add(name)
    # 호출자가 전달한 종료된 빌드/실패 로그도 전체 바이트로 함께 보존한다.
    for name in logs:
        resolved=(ROOT/name).resolve();relative=resolved.relative_to(ROOT).as_posix()
        assert relative.startswith('extracted/') and resolved.is_file(),name
        names.add(relative)
    saved={}
    # 모든 저장 조각은 파일명/전체 SHA와 묶어 다른 PC에서도 최초 교정을 재현하게 한다.
    for name in sorted(names):
        source=ROOT/name;target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(source.read_bytes());saved[name]=sha(target)
    (HISTORY/'baseline.json').write_text(json.dumps(dict(files=saved,protected=protected,logs=list(logs)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Arrival 이름 교정 전 전체 보존:',len(saved))

def expected():
    """이름 두 토큰의 정확 역변환과 최초 binding을 당시 read view에서 검증한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'))
    current=(ROOT/SOURCE).read_bytes();assert current.count(NEW)==2 and OLD not in current
    inverse=current.replace(NEW,OLD);assert inverse==(HISTORY/SOURCE).read_bytes()
    # 같은 이름 교정 외 생산/Native/fixture/전수/Host/지원헤더/도구/기존 보고서는 전체 불변이다.
    for name,digest in baseline['files'].items():
        assert sha(HISTORY/name)==digest,name
        if name==SOURCE:assert hashlib.sha256(inverse).hexdigest()==digest
        elif not name.startswith(('docs/','extracted/campaign-runtime14-')):assert sha(ROOT/name)==digest,name
    for name,digest in baseline['protected'].items():assert sha(ROOT/name)==digest,name
    prior=importlib.import_module('analyze_carrierarrival_cpp_binding');read_bytes=Path.read_bytes
    def historical(path):
        """최초 binding 계산에서 교정된 cpp 하나만 저장한 전체 이전 원문으로 읽는다."""
        return read_bytes(HISTORY/SOURCE if path.resolve()==(ROOT/SOURCE).resolve() else path)
    Path.read_bytes=historical
    try:assert json.loads(prior.REPORT.read_text(encoding='utf-8'))==prior.expected()
    finally:Path.read_bytes=read_bytes
    native=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));assert native['total']==406 and native['game_runs']==0
    halt=json.loads((ROOT/'cpppj/recovery-priesthalt-evidence.json').read_text(encoding='utf-8'));assert halt['total']==864
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,
                preserved_carrier_normal_inputs=406,preserved_halt_normal_inputs=864,existing_genus_normal_recount=0,
                exact_token_rename_count=2,whole_cpp_exact_inverse=True,all_native_fixture_test_body_bytes_unchanged=True,
                historical_binding_reproduced=True,prior_binding_sha256=sha(ROOT/BINDING),original_native_sha256=sha(ROOT/NATIVE),
                old_source_sha256=sha(HISTORY/SOURCE),current_source_sha256=sha(ROOT/SOURCE),tool_sha256=sha(Path(__file__)),
                files={name:sha(ROOT/name) for name in baseline['files'] if not name.startswith(('docs/','extracted/campaign-runtime14-'))},
                history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='ChunkMap의 uint8 전역 kNoIsland와 충돌하는 사제 Arrival uint32 로컬 상수 이름 두 토큰만 kPriestNoIsland로 변경한다. 값127/쓰기폭/조건/호출/계산/기존Native406·Halt864/fixture/전수검사/같은Host소비자는 불변이며 새원본입력0이다. 최초 빌드 실패 로그와 전체 원문을 보존하고 기존 Carrier binding 원문은 당시 cpp read view에서 재현한다.')

def main(arguments):
    """명시 CLI에서 교정 전 저장 또는 현재 최소 이름 교정 successor 감사를 수행한다."""
    if arguments.capture:capture(arguments.log);return
    data=expected()
    if arguments.verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Arrival 이름: 두토큰정확역변환/전체Native·생산몸체·검사불변/기존binding재현/새입력0')

# 명시 호출에서만 전체 보존/현재 보고서 생성/현재 바이트 감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true');parser.add_argument('--log',action='append',default=[])
    parser.add_argument('--verify',action='store_true');main(parser.parse_args())
