#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""포획 알림과 실제 희생 소비자의 입력 준비 교정을 전체 이전 바이트와 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소 루트이며 보호 원본은 읽기만 한다.
HISTORY=ROOT/'cpppj/hist/dais-followu-aeb72' # 첫 후속 C++ 실패와 관련 전체126파일이다.
NOTICE='cpppj/tests/DaisContentNoticeTests.cpp' # 원본 사제 후보의 1×1 발자국을 준비하는 전수 검사다.
CONSUMER='cpppj/tests/DaisRitualConsumerTests.cpp' # 같은 풀의 void 프레임 단계와 최초 Pop 순서를 준비하는 검사다.
REPORTS={NOTICE:'cpppj/daiscontentnotice-cpp-correction.json',CONSUMER:'cpppj/dais-ritual-consumer-cpp-correction.json'} # 두 검사 교정의 현재 출처를 각각 기록한다.

def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def restored(test):
    """허용한 입력 준비만 역제거해 수정 전 검사 전체 바이트를 복원한다."""
    value=(ROOT/test).read_bytes()
    if test==NOTICE:
        change='        types[158].footX=types[158].footY=1; // 같은 원본 입력 체인의 사제 후보는 1×1 발자국이다.\n'.encode('utf-8')
        assert value.count(change)==1
        value=value.replace(change,b'')
    else:
        change='        // void 입력은 실제 StoreLevel로 준비해 Set이 최초 Pop 전에 단계 변경 Repop를 요청하지 않게 한다.\n'.encode('utf-8')
        assert value.count(change)==1 and value.count(b'frame.StoreLevel(daisSid);')==1
        value=value.replace(change,b'').replace(b'frame.StoreLevel(daisSid);',b'')
    assert value==(HISTORY/test).read_bytes(),test
    return value

def expected():
    """수정 전 원본 관찰과 전체 이력/생산을 보존하고 두 현재 검사만 연결한다."""
    history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()}
    assert len(history)==126
    native_name='cpppj/daiscontentnotice.json'
    native=json.loads((HISTORY/native_name).read_text(encoding='utf-8'))
    assert (ROOT/native_name).read_bytes()==(HISTORY/native_name).read_bytes() and native['total']==1444 and not native['smoke'] and native['game_runs']==0
    fixture='cpppj/tests/fixtures/daiscontentnotice-x86.tsv'
    assert (ROOT/fixture).read_bytes()==(HISTORY/fixture).read_bytes()
    rows=[line.split('\t') for line in (ROOT/fixture).read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==1444 and len({(row[0],row[1]) for row in rows})==1444
    # 당시 입력/효과/raw와 x87 정상 반환 수는 새 테스트 SHA와 구분해 그대로 보존한다.
    for edition,item in native['editions'].items():
        assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    original_files={}
    # Native 당시 출처는 보존한 원문을 우선 읽고 PE와 내보내기는 원래 읽기 전용 경로를 읽는다.
    for name,value in native['files'].items():
        path=HISTORY/name if (HISTORY/name).is_file() else ROOT/name
        assert sha(path)==value,name
        if name!=NOTICE: assert sha(ROOT/name)==value,name
        original_files[name]={'sha256':value,'preserved_path':path.relative_to(ROOT).as_posix()}
    binding='cpppj/dais-ritual-consumer-binding.json'
    old_binding=json.loads((HISTORY/binding).read_text(encoding='utf-8'))
    assert (ROOT/binding).read_bytes()==(HISTORY/binding).read_bytes()
    # 같은 풀 소비자에 연결한 기존 전체 생산/선행 보고서/도구의 당시 SHA는 변경하지 않는다.
    for name,value in old_binding['files'].items():
        path=HISTORY/name if (HISTORY/name).is_file() else ROOT/name
        assert sha(path)==value,name
        if name!=CONSUMER: assert sha(ROOT/name)==value,name
    current={}
    # 두 전수 검사 원문만 정확 역변환하고 이전 successor/binding/전체 history는 덮어쓰지 않는다.
    for test in REPORTS:
        restored(test)
        prior_names=(native_name,'cpppj/daiscontentnotice-successor.json') if test==NOTICE else (binding,)
        prior={}
        for name in prior_names:
            assert (ROOT/name).read_bytes()==(HISTORY/name).read_bytes()
            prior[name]=sha(ROOT/name)
        current[test]={
            'host':'HJOW-Athlon','decompile_date':'2026-10-14','normal_inputs':1444 if test==NOTICE else 0,'new_native_inputs':0,
            'entire_previous_test_restored_exactly':True,'previous_test_sha256':sha(HISTORY/test),'current_test_sha256':sha(ROOT/test),
            'production_and_native_fixture_unchanged':True,'prior_reports':prior,'original_native_files':original_files,
            'previous_consumer_binding_sha256':sha(HISTORY/binding),'tool_sha256':sha(Path(__file__)),'history':history,
            'correction':'원본 입력 체인의 사제 158 발자국 1×1을 C++에도 준비한다.' if test==NOTICE else 'void 생성 객체에 실제 StoreLevel을 먼저 실행해 Set의 단계변경 Repop가 최초 Pop 전에 호출되는 구성 오류를 교정한다.',
            'scope':'새 관찰0. 기존 Notice1444/currentLifecycle1928/Progress2964/이전 관찰 보고서는 당시 SHA와 입력/효과/raw를 유지한다. 실제 소비자는 비전투·HP16·genus0 합성 입력의 같은 풀 삭제/반납 사슬이며 전체 캠페인 자산의 드롭/주변 피해/다리/섬정리·전체 Progress/Bridge/사제해방을 주장하지 않는다.'
        }
    return current

def main(check):
    """명시 CLI에서 두 교정 자료를 작성하거나 전체 정확 역변환을 감사한다."""
    data=expected()
    # 두 보고서의 이전 원문과 현재 입력 준비를 각각 독립 파일로 기록한다.
    for test,name in REPORTS.items():
        path=ROOT/name
        if check: assert json.loads(path.read_text(encoding='utf-8'))==data[test],name
        else: path.write_text(json.dumps(data[test],ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('의식 후속 C++ 입력 준비: 전체126파일/두 검사 정확 역변환/생산·Native 불변/새관찰0')

# 명시 요청에서만 현재 교정 자료를 작성하거나 검증한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    main(args.verify)
