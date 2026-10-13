#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CD 수명 관찰의 표면 입력셀 교정을 같은1928·원문 보존과 연결한다."""
import argparse,json
from pathlib import Path
from decomp_daislifecycle_current_oracle import ROOT,FIXTURE,REPORT,sha,verify as native_verify

HISTORY=ROOT/'cpppj/recovery-history/daislifecycle-initial-cpp-failure' # 최초 실패의 source/test/oracle/1928/lineage/로그/export 전체다.
SUCCESSOR=ROOT/'cpppj/recovery-daislifecycle-current-successor-evidence.json' # 같은 입력의 메모리 위치 교정을 새 현재 근거에 연결한다.

def rows(path):
    """저장된 원본 행을 고유 입력키로 읽는다."""
    values=[line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    result={(row[0],row[1]):row for row in values};assert len(result)==len(values);return result

def expected():
    """현재 전체 SHA·정확 tool 역변환·12행 효과 외 모든 결과 불변을 확인한다."""
    native_verify();old_path=HISTORY/'cpppj/recovery-daislifecycle-evidence.json';old=json.loads(old_path.read_text(encoding='utf-8'));current=json.loads(REPORT.read_text(encoding='utf-8'));assert old['total']==current['total']==1928
    before=(HISTORY/'tools/decomp_daislifecycle_oracle.py').read_text(encoding='utf-8');after=(ROOT/'tools/decomp_daislifecycle_current_oracle.py').read_text(encoding='utf-8')
    restored=after.replace('daislifecycle-current-x86.tsv','daislifecycle-x86.tsv').replace('recovery-daislifecycle-current-evidence.json','recovery-daislifecycle-evidence.json').replace('SURFACE_STORAGE+(42*256+41)*2','SURFACE_STORAGE+(41*256+40)*2');assert restored==before
    previous=rows(HISTORY/'cpppj/tests/fixtures/daislifecycle-x86.tsv');latest=rows(FIXTURE);assert previous.keys()==latest.keys();changed=[]
    # 실제 CD 입력셀만 바뀌어 warning 12행의 가상 조회 효과가 나타난다. raw/player는 전수 같다.
    for key,row in latest.items():
        old_row=previous[key];assert row[3:]==old_row[3:],key
        if row[2]!=old_row[2]:
            value=json.loads(key[1]);assert key[0]=='originalCD' and value['mode']=='dpp' and value['profile'] in (1,2,3,4) and value['saved'] in (0,1,2)
            changed.append(dict(edition=key[0],input=value,previous_effects=old_row[2],current_effects=row[2]))
    assert len(changed)==12;unchanged={};history={}
    # 처음1928과기존PostPop lineage 원문 및현재생산/최초Test를그대로보존한다.
    for name in ('cpppj/recovery-daislifecycle-evidence.json','cpppj/tests/fixtures/daislifecycle-x86.tsv','tools/decomp_daislifecycle_oracle.py','cpppj/recovery-dais-postpop-lineage-evidence.json','tools/analyze_dais_postpop_lineage.py','cpppj/src/o/RawDaisLifecycle.h','cpppj/src/o/RawDaisLifecycle.cpp','cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp','cpppj/tests/DaisLifecycleTests.cpp'):
        assert (ROOT/name).read_bytes()==(HISTORY/name).read_bytes(),name;unchanged[name]=sha(ROOT/name)
    # 보존 당시의 파일 전체 SHA를 기록하여 다른PC에서 실패근거도 재검사한다.
    for path in sorted(HISTORY.rglob('*')):
        if path.is_file():history[path.relative_to(ROOT).as_posix()]=sha(path)
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=1928,new_native_inputs=0,production_and_test_unchanged=True,original_reports_and_fixture_unchanged=True,entire_oracle_restored_exactly=True,
        memory_correction=dict(edition='originalCD',x87_x='0048153b fld x;0048153f fadd DWORD00504030;00481545 __ftol',x87_y='0048154a fld y;0048154e fadd DWORD00504030;00481556 __ftol',previous_cell=[40,41],current_cell=[41,42],changed_rows=changed,unchanged_rows=1916,raw_and_player_fields_unchanged=True),
        previous_report_sha256=sha(old_path),current_report_sha256=sha(REPORT),current_fixture_sha256=sha(FIXTURE),tool_sha256=sha(Path(__file__)),unchanged=unchanged,history=history,
        scope='Ghidra C의 CD __ftol 앞 fadd 생략을 실제 명령으로 확인해 의도한 SID9 입력셀만 바로잡았다. 같은1928 재관찰은 신규 합산0이며전체현재생산/Test/기존보고서·fixture·lineage는불변이다. 별도새current파일로연결한다.')

def main(check):
    """별도 successor 작성 또는 현재 원본/12행 교정의 정확 범위를 확인한다."""
    data=expected()
    if check:assert json.loads(SUCCESSOR.read_text(encoding='utf-8'))==data
    else:SUCCESSOR.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 수명 CD 입력셀: 같은1928/효과12행 교정·나머지1916 및전수raw 불변/추가0')

# 명시 CLI 요청에서만 새 메모리 교정 근거를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
