#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DudeDrop 최초308과 미컴파일 test의 실제 두 진단 줄을 전체 원문으로 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path

# 최초 정상306/assert2의 전체 보고서/출처와 현재 test successor를 분리한다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/recovery-history/dudedrop-before-assert-lines'
REPORT=ROOT/'cpppj/recovery-dudedrop-draft-lineage-evidence.json'
NATIVE='cpppj/recovery-dudedrop-evidence.json'
TEST='cpppj/tests/DudeDropTests.cpp'
OLD='events.push_back("A:0");'
NEW='events.push_back("A:"+std::to_string(Number(row[1]) ? 452 : 469));'


def sha(body):
    """현재/이전 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def observe():
    """test 한 조각을 역변환하고308원본·production·fixture·도구 전체를 보존한다."""
    capture=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'));native=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));history={};dependencies={}
    current=(ROOT/TEST).read_text(encoding='utf-8')
    if current.count(NEW)!=1 or current.replace(NEW,OLD,1)!=(HISTORY/TEST).read_text(encoding='utf-8'):raise RuntimeError('DudeDrop test 전체 역변환 오류')
    # 최초 보고서가 사용한 모든 비보호 출처의 전체 원문과 당시 SHA를 확인한다.
    for name,value in capture['files'].items():
        old=(HISTORY/name).read_bytes()
        if sha(old)!=value:raise RuntimeError('DudeDrop 전체 이전 원문 SHA 오류 '+name)
        if name!=TEST and (ROOT/name).read_bytes()!=old:raise RuntimeError('DudeDrop 원본/source/보고서 변경 '+name)
        history[(HISTORY/name).relative_to(ROOT).as_posix()]=value
    # 보호 PE/TAFF/SHP는 복사하지 않고 현재 동일 바이트만 확인한다.
    for name,value in native['files'].items():
        path=HISTORY/name if (HISTORY/name).exists() else ROOT/name
        if sha(path.read_bytes())!=value:raise RuntimeError('DudeDrop 최초308 출처 오류 '+name)
        dependencies[name]=dict(reported_sha256=value,preserved_or_current_path=path.relative_to(ROOT).as_posix())
    if native['total']!=308 or native['editions']['originals']['returns']!=152 or native['editions']['originals']['assertions']!=2 or native['editions']['originalCD']['returns']!=154 or native['editions']['originalCD']['assertions']:raise RuntimeError('DudeDrop 최초 정상/진단 분류 오류')
    history[(HISTORY/'capture.json').relative_to(ROOT).as_posix()]=sha((HISTORY/'capture.json').read_bytes())
    return dict(history=history,dependencies=dependencies,files={name:sha((ROOT/name).read_bytes()) for name in (NATIVE,TEST,'cpppj/tests/fixtures/dudedrop-x86.tsv','cpppj/src/o/RawDudeDrop.h','cpppj/src/o/RawDudeDrop.cpp','tools/decomp_dudedrop_oracle.py','cpppj/tests/DudeSpawnSupport.h')},whole_old_test_inverse=True)


def main(check):
    """초안 진단 출력만 교정한 현재SHA와 최초308관찰을 별도 보고서에 연결한다."""
    data=dict(schema=1,host='HJOW-Athlon',total=308,normal=306,assertions=2,new_native_inputs=0,cpp_tests_completed=0,lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),scope='미공유 컴파일 test의 sourceSID0 진단 줄을 실제 random 부모452/진입점 부모469로 관찰한 값에 맞춘다. 실제 현재 입력의 부모 선택만 사용하며 최초 전체 원문/308 fixture/도구/production은 보존한다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('DudeDrop 초안 successor SHA 오류')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,total=308,normal=306,assertions=2,new_native_inputs=0,cpp_completed=False),ensure_ascii=False))


# 명시 CLI에서만 새 초안 lineage를 작성하거나 이미 고정된 SHA를 확인한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
