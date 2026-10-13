#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""내부 Form 복원 후의 기존 거부 검사를 범위 밖 타입 입력으로 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 현재 저장소 전체 원문을 읽는 기준이다.
HISTORY=ROOT/'cpppj/recovery-history/graph-forms-before-test-scope'  # 첫 공유 실행과 이전 검사/원본 결과를 전체 보존한다.
REPORT=ROOT/'cpppj/recovery-graph-forms-test-lineage-evidence.json'  # 기존 보호 검사의 입력 범위 교정 출처다.
TEST='cpppj/tests/RawGraphTests.cpp'  # 기존 그래프 원본 전수와 손상 입력 보호 검사 전체다.
CHANGES=(
    ('// 전체 풀 복구를 선택한 Pop도 다른 void 표면의 손상/내부 타입을 공간 쓰기 전에 거부한다.',
     '// 전체 풀 복구를 선택한 Pop도 다른 void 표면의 손상 프레임/범위 밖 타입을 쓰기 전에 거부한다.'),
    ('RawGraph_RecoveryPreflightRejectsOtherVoidAndInternalTypeBeforePop',
     'RawGraph_RecoveryPreflightRejectsOtherVoidAndOutOfRangeTypeBeforePop'),
    ('// 처음에는 손상 프레임, 다음에는 미복원 내부 타입을 전체 순회에서 검출한다.',
     '// 처음에는 손상 프레임, 다음에는 범위 밖 타입을 전체 순회에서 검출한다.'),
    ('if (scenario) { Put(bytes,36,0); bytes[10]=5; }',
     'if (scenario) { Put(bytes,36,0); bytes[10]=255; }'),
)  # 표면 계산과 검사 조건은 보존하고 복원된 내부 타입 생략과 손상 입력을 구분한다.
SAVED=(TEST,'cpppj/src/o/RawGraph.cpp','tools/analyze_graph_forms_lineage.py',
       'cpppj/recovery-graph-forms-lineage-evidence.json','tools/decomp_graphforms_oracle.py',
       'cpppj/recovery-graphforms-evidence.json','cpppj/tests/fixtures/graphforms-x86.tsv',
       'extracted/campaign-runtime13-build-initial.log','extracted/campaign-runtime13-RawGraph_initial-tests.log')  # 최초 전체 결과/소스/로그다.


def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """첫 공유 실패 뒤 검사 교정 전에 당시 전체 파일을 보존한다."""
    assert not HISTORY.exists()
    values={}
    # 현재 전체 생산/원본 결과와 검사/실패 로그를 그대로 저장한다.
    for name in SAVED:
        target=HISTORY/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes())
        values[name]=sha(target)
    (HISTORY/'baseline.json').write_text(json.dumps(values,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('그래프 기존 거부 입력 교정 전 전체9파일 보존')


def expected():
    """한 입력/설명/검사명을 정확 역변환하고 기존 원본64와 보호 몸체를 보존한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'))
    # 전체 이전 바이트와 수정하지 않은 생산/원본 결과를 각각 확인한다.
    for name,value in baseline.items():
        assert sha(HISTORY/name)==value,name
        if name!=TEST:assert (ROOT/name).read_bytes()==(HISTORY/name).read_bytes(),name
    current=(ROOT/TEST).read_bytes()
    restored=current
    # 설명/이름과 입력의 네 고유 조각만 이전 값으로 되돌린다.
    for before,after in CHANGES:
        old,new=before.encode('utf-8'),after.encode('utf-8')
        assert restored.count(new)==1
        restored=restored.replace(new,old,1)
    assert restored==(HISTORY/TEST).read_bytes()
    graph=importlib.import_module('analyze_graph_forms_lineage')
    original=Path.read_bytes
    def historical(path):
        """정확 역변환이 확인된 기존 검사만 당시 바이트를 읽게 하고 파일 쓰기는 하지 않는다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:
        assert json.loads(graph.REPORT.read_text(encoding='utf-8'))==graph.expected()
    finally:
        Path.read_bytes=original
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,
                original_normal_inputs=64,production_and_native_fixture_unchanged=True,whole_previous_test_inverse=True,
                current_test_sha256=sha(ROOT/TEST),tool_sha256=sha(Path(__file__)),
                history={p.relative_to(ROOT).as_posix():sha(p) for p in HISTORY.rglob('*') if p.is_file()},
                change='기존 내부타입5 미복원 거부 입력은 범위 밖 타입255로 바꾼다. 손상프레임/범위밖타입의 실패 전 전체쓰기 보호 검사는 유지한다.')


def main(arguments):
    """첫 전체 자료 저장 또는 현재 검사 범위 교정의 생성/검증을 수행한다."""
    if arguments.capture:
        capture()
        return
    data=expected()
    if arguments.verify:
        assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:
        REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('그래프 기존 거부 입력: 전체9파일/정확역변환/생산·원본64 불변/추가0')


# 실패 로그를 보존한 뒤 현재 보호 검사 입력을 명시 교정하고 별도 출처를 기록한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true')
    parser.add_argument('--verify',action='store_true')
    main(parser.parse_args())
