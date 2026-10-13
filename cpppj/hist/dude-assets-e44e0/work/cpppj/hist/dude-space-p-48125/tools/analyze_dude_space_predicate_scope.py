#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Dude의 spot 분기 불일치와 shared Unpop 변경 불필요를 최초48 원문에 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

# 최초 정상48의 source/report/fixture 전체와 현재 별도 분기 범위 보고서다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/hist/dude-space-b-02312'
REPORT=ROOT/'cpppj/dude-space-predicate-scope.json'
NATIVE='cpppj/dude-space.json'


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
    return dict(history=history,native_report_sha256=sha((ROOT/NATIVE).read_bytes()),predicate=inputs,files={name:sha((ROOT/name).read_bytes()) for name in (NATIVE,'cpppj/src/o/SquidUnpop.cpp','cpppj/tests/DudeSpaceTests.cpp','cpppj/tests/DudeSpaceSupport.h','cpppj/tests/fixtures/dude-space-x86.tsv','tools/decomp_dude_space_oracle.py')})


def main(check):
    """처음 초안의 pending shared 경계만 정확한 현재 판정으로 별도 교정한다."""
    data=dict(schema=1,host='HJOW-Athlon',total=48,new_native_inputs=0,cpp_tests_completed=0,requires_shared_unpop_change=False,lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),scope='실제 Dude70의 flags2=02000400은 spot504442ff와 교집합0이고 flags1=28040010의 surface800도0이다. 따라서 Unpop은 해시/상태/표시를 실제 실행하되 foot크기 guard/spot순회에는 들어가지 않는다. 최초48 보고서의 pending_shared_foot0=true는 당시 잘못된 초안이며 이 successor의 false가 현재 소비자 경계다. 기존 보고서/tool/fixture/source 전체 원문은 불변이고 sharedUnpop 수정0이다. 이 자료를 실제 zero-foot spot분기 실행 근거로 세지 않는다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('Dude 공간 분기 successor SHA 오류')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,total=48,new_native_inputs=0,requires_shared_unpop_change=False,cpp_completed=False),ensure_ascii=False))


# 명시 CLI에서 현재 분기 범위 successor를 기록하거나 고정 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
