#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""내용물 알림 검사의 VT 상수 헤더 한줄을 이전1444와 정확히 연결한다."""
import argparse,json
from pathlib import Path
from decomp_daiscontentnotice_oracle import ROOT,REPORT,FIXTURE,sha,verify as native_verify

HISTORY=ROOT/'cpppj/hist/daiscontentn-b4878' # 수정 전1444 자료와 검사 원문 전체다.
SUCCESSOR=ROOT/'cpppj/daiscontentnotice-successor.json' # 같은 입력의 재관찰과 헤더 한줄 교정을 분리한 근거다.
TEST='cpppj/tests/DaisContentNoticeTests.cpp' # Altar VT 상수 선언을 실제 헤더에서 읽도록 한줄을 추가했다.

def expected():
    """현재 출처·정확 역변환·모든 기존 결과/생산 불변을 확인한다."""
    native_verify();old=json.loads((HISTORY/'cpppj/daiscontentnotice.json').read_text(encoding='utf-8'));current=json.loads(REPORT.read_text(encoding='utf-8'));assert old['total']==current['total']==1444
    include='#include "o/RawAltarSacrifice.h"\n';test=(ROOT/TEST).read_text(encoding='utf-8');assert test.count(include)==1 and test.replace(include,'')==(HISTORY/TEST).read_text(encoding='utf-8')
    assert FIXTURE.read_bytes()==(HISTORY/'cpppj/tests/fixtures/daiscontentnotice-x86.tsv').read_bytes()
    assert old['editions']==current['editions'];dependencies={};history={}
    # 생산/도구/PE/export/모든 입력·출력은 같은 원문이며 새 Test include만 SHA가 다르다.
    for name,value in old['files'].items():
        path=HISTORY/name if (HISTORY/name).exists() else ROOT/name;assert sha(path)==value,name
        if name!=TEST:assert current['files'][name]==value
        dependencies[name]=dict(previous_sha256=value,current_sha256=current['files'][name],preserved_or_current=path.relative_to(ROOT).as_posix())
    # 이전 자료의 전체 파일은 다른 PC에서도 당시 SHA로 재현할 수 있게 보존한다.
    for path in sorted(HISTORY.rglob('*')):
        if path.is_file():history[path.relative_to(ROOT).as_posix()]=sha(path)
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=1444,new_native_inputs=0,entire_test_restored_exactly=True,production_and_fixture_unchanged=True,
        previous_report_sha256=sha(HISTORY/'cpppj/daiscontentnotice.json'),current_report_sha256=sha(REPORT),tool_sha256=sha(Path(__file__)),dependencies=dependencies,history=history,
        scope='현재 검사의 명시 Altar 헤더 한줄만 추가. 이전1444/전체생산·도구·fixture·반환/효과/입력은 보존하고 같은1444 재관찰을 새 고유 입력에 더하지 않는다.')

def main(check):
    """새 successor 작성 또는 현재 한줄 교정과 전체 원본 불변을 감사한다."""
    data=expected()
    if check:assert json.loads(SUCCESSOR.read_text(encoding='utf-8'))==data
    else:SUCCESSOR.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 내용물 알림 헤더 한줄: 정확 역변환/원본1444 불변/추가0')

# 명시 CLI 요청에서만 연결 자료를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
