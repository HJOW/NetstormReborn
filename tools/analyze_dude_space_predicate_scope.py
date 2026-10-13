#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Dude의 spot 분기 불일치와 shared Unpop 변경 불필요를 최초48 원문에 연결한다."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys

# 최초 정상48의 source/report/fixture 전체와 현재 별도 분기 범위 보고서다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/hist/dude-space-b-02312'
REPORT=ROOT/'cpppj/dude-space-predicate-scope.json'
NATIVE='cpppj/dude-space.json'
COUNT_HISTORY=ROOT/'cpppj/hist/dude-space-p-48125' # 앞 분기 보고서/도구/48전체와 잘못된 고유수 문서를 보존한다.


def sha(body):
    """전체 원문 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def observe():
    """기존48/전체source 불변과 두 PE의 실제 spot마스크를 함께 감사한다."""
    capture=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'));native=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));history={};inputs={}
    # 최초 전체 원문은 수정하지 않으며 새 입력/재생성도 추가하지 않는다.
    for name,value in capture['files'].items():
        old=(HISTORY/name).read_bytes()
        if sha(old)!=value or (ROOT/name).read_bytes()!=old:raise RuntimeError('Dude 공간 최초 전체 원문 변경 '+name)
        history[(HISTORY/name).relative_to(ROOT).as_posix()]=value
    sys.path.insert(0,str(ROOT/'extracted/oracle-python'));import pefile
    # 두 PE의 실제 building 마스크와 TYPE 명시 입력의 비트 교집합을 확인한다.
    for edition,address in (('originals',0x5424b8),('originalCD',0x5395cc)):
        binary=ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE');pe=pefile.PE(str(binary));value=int.from_bytes(pe.get_data(address-pe.OPTIONAL_HEADER.ImageBase,4),'little');mask=value|0x400ff;flags1=0x28040010;flags2=0x02000400
        if value!=0x50444200 or (flags2&mask) or (flags1&0x800):raise RuntimeError('Dude 실제 spot 분기 교집합 오류')
        inputs[edition]=dict(pe_sha256=sha(binary.read_bytes()),building_mask_address=f'{address:08x}',building_mask=value,spot_mask=mask,type_flags1=flags1,type_flags2=flags2,genus_intersection=flags2&mask,surface_intersection=flags1&0x800,writes_spots=False)
    if native['total']!=48:raise RuntimeError('Dude 공간 정상48 불변 오류')
    rows=[row for row in csv.reader((ROOT/'cpppj/tests/fixtures/dude-space-x86.tsv').open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')];effective={}
    # Unpop-only의 목적지 label은 PE 메모리/인자로 전혀 공급하지 않으므로 고유 입력에서 제외한다.
    for edition in inputs:
        selected=[row for row in rows if row[0]==edition];groups={}
        for row in selected:
            key=tuple(row[1:4])+tuple(row[6:7])+(tuple(row[4:6]) if row[6]=='1' else ())
            if key in groups and groups[key]!=row[7:]:raise RuntimeError('Dude 같은 실제 입력의 반복 출력 불일치')
            groups[key]=row[7:]
        if len(selected)!=24 or len(groups)!=18:raise RuntimeError('Dude 유효 입력/반복 수 오류')
        effective[edition]=dict(normal_executions=24,unique_native_inputs=18,duplicate_executions=6,unpop_only_unique=6,actual_repop_unique=12)
    # 고유수 교정 전 전체8자료는 보존하며 원래 native/source/fixture 바이트는 변경하지 않는다.
    count_capture=json.loads((COUNT_HISTORY/'capture.json').read_text(encoding='utf-8'))
    for name,value in count_capture['files'].items():
        saved=COUNT_HISTORY/name
        if sha(saved.read_bytes())!=value:raise RuntimeError('Dude 고유수 교정 전 원문 SHA 오류 '+name)
        history[saved.relative_to(ROOT).as_posix()]=value
    return dict(history=history,native_report_sha256=sha((ROOT/NATIVE).read_bytes()),predicate=inputs,effective_inputs=effective,files={name:sha((ROOT/name).read_bytes()) for name in (NATIVE,'cpppj/src/o/SquidUnpop.cpp','cpppj/tests/DudeSpaceTests.cpp','cpppj/tests/DudeSpaceSupport.h','cpppj/tests/fixtures/dude-space-x86.tsv','tools/decomp_dude_space_oracle.py')})


def main(check):
    """처음 초안의 pending shared 경계만 정확한 현재 판정으로 별도 교정한다."""
    data=dict(schema=2,host='HJOW-Athlon',total=48,unique_native_inputs=36,duplicate_executions=12,new_native_inputs=0,cpp_tests_completed=0,requires_shared_unpop_change=False,lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),scope='실제 Dude70의 flags2=02000400은 spot504442ff와 교집합0이고 flags1=28040010의 surface800도0이다. 따라서 Unpop은 해시/상태/표시를 실제 실행하되 foot크기 guard/spot순회에는 들어가지 않는다. 최초48 보고서의 pending_shared_foot0=true는 당시 잘못된 초안이며 이 successor의 false가 현재 소비자 경계다. 정상실행48 중 Unpop-only의 목적지label만 다른12반복은 PE에 공급되지 않아 새고유로 세지 않으며 실제고유는36이다. 기존 보고서/tool/fixture/source 전체 원문은 불변이고 sharedUnpop 수정0이다. 이 자료를 실제 zero-foot spot분기 실행 근거로 세지 않는다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('Dude 공간 분기 successor SHA 오류')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,total=48,unique_native_inputs=36,duplicate_executions=12,new_native_inputs=0,requires_shared_unpop_change=False,cpp_completed=False),ensure_ascii=False))


# 명시 CLI에서 현재 분기 범위 successor를 기록하거나 고정 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
