#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""BurningFrame 최초426을 보존하고 미관찰 Dais extra 읽기의 한 조각을 정확 역변환한다."""
import argparse
import hashlib
import json
from pathlib import Path

# 최초Frame426의 전체원문과현재생산몸체를 별도successor로 연결한다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/hist/burningframe-d5cb6'
REPORT=ROOT/'cpppj/burningframe-draft-lineage.json'
NATIVE='cpppj/burningframe.json'
SOURCE='cpppj/src/o/RawBurningFrame.cpp'
TEST='cpppj/tests/BurningFrameTests.cpp'
PREVIOUS=ROOT/'cpppj/hist/burningframe-99a53'
OLD='raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]&1U'
NEW='raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1U'
OLD_TEST='RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState visualState;RawBurningVisual visual(pool,registry,visualState,{});'
NEW_TEST='const auto rejectReserve=registryHooks.reserve;RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState visualState;BurningVisualHooks visualHooks;visualHooks.reserve=rejectReserve;RawBurningVisual visual(pool,registry,visualState,std::move(visualHooks));'


def sha(body):
    """전체원문바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def observe():
    """원본격자·fixture·도구와모든이전전체소스를 보존하고한조각역변환을확인한다."""
    capture=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'));native=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));current=(ROOT/SOURCE).read_bytes();old=(HISTORY/SOURCE).read_bytes();history={};dependencies={}
    if current.count(NEW.encode())!=1 or current.replace(NEW.encode(),OLD.encode(),1)!=old:raise RuntimeError('BurningFrame 전체body 정확역변환 오류')
    test=(ROOT/TEST).read_bytes()
    if test.count(NEW_TEST.encode())!=1 or test.replace(NEW_TEST.encode(),OLD_TEST.encode(),1)!=(HISTORY/TEST).read_bytes():raise RuntimeError('BurningFrame 전체test 정확역변환 오류')
    # 이미기록한426자료는전체원문을그대로보존한다. current/source만두숫자필드가다르다.
    for name,value in capture['files'].items():
        body=(HISTORY/name).read_bytes()
        if sha(body)!=value:raise RuntimeError('BurningFrame 이전전체원문SHA 오류 '+name)
        if name not in (SOURCE,TEST) and (ROOT/name).read_bytes()!=body:raise RuntimeError('BurningFrame 최초fixture/tool/report 변경 '+name)
        history[(HISTORY/name).relative_to(ROOT).as_posix()]=value
    # 보호PE/TAFF/SHP는복사하지않으며당시SHA와현재읽기전용바이트를확인한다.
    for name,value in native['files'].items():
        path=HISTORY/name if (HISTORY/name).exists() else ROOT/name
        if sha(path.read_bytes())!=value:raise RuntimeError('BurningFrame 최초426출처SHA 오류 '+name)
        dependencies[name]=dict(reported_sha256=value,preserved_or_current_path=path.relative_to(ROOT).as_posix())
    if native['total']!=426 or any(item['returns']!=213 or item['assertions'] for item in native['editions'].values()):raise RuntimeError('BurningFrame 최초426 정상분류 오류')
    history[(HISTORY/'capture.json').relative_to(ROOT).as_posix()]=sha((HISTORY/'capture.json').read_bytes())
    prior=json.loads((PREVIOUS/'capture.json').read_text(encoding='utf-8'))
    # 첫successor의전체도구/보고서와같은당시test원문도별도보존한다.
    for name,value in prior['files'].items():
        if sha((PREVIOUS/name).read_bytes())!=value:raise RuntimeError('BurningFrame 첫successor 전체SHA 오류 '+name)
        history[(PREVIOUS/name).relative_to(ROOT).as_posix()]=value
    history[(PREVIOUS/'capture.json').relative_to(ROOT).as_posix()]=sha((PREVIOUS/'capture.json').read_bytes())
    previous=json.loads((PREVIOUS/'cpppj/burningframe-draft-lineage.json').read_text(encoding='utf-8'))
    if previous['schema']!=1 or previous['total']!=426 or previous['new_native_inputs'] or previous['lineage']['files'][TEST]!=sha((PREVIOUS/TEST).read_bytes()):raise RuntimeError('BurningFrame 첫successor 당시test연결 오류')
    return dict(history=history,dependencies=dependencies,files={name:sha((ROOT/name).read_bytes()) for name in (SOURCE,NATIVE,'cpppj/src/o/RawBurningFrame.h','cpppj/src/client/BurningFrameCapture.h','cpppj/src/client/BurningFrameCapture.cpp',TEST,'cpppj/tests/fixtures/burningframe-x86.tsv','tools/decomp_burningframe_oracle.py')},whole_old_production_inverse=True,whole_old_test_inverse=True,previous_draft_preserved=True)


def main(check):
    """새원본입력0의초안교정과당시426을현재SHA에연결한다."""
    data=dict(schema=2,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=426,normal=426,assertions=0,new_native_inputs=0,cpp_tests_completed=0,lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),scope='Dais extra bit1은ownerBYTE34/32가아닌실제extraBYTE40/35다. 기존RawBurningSources와같은HasHP몸체로초안의두offset숫자만교정한다. 최초426은실제Dude70이라이분기를관찰하지않았으며fixture/Native출력/도구/보고서전체를그대로보존한다. Dais분기를새실행한것으로세지않는다. 미컴파일test의RawBurningVisual에는실제생성자계약이요구하는reserve훅을설치하되기존명시미관찰reserve거부callback을재사용한다. 실제Frame중reserve호출은없다. 첫successor전체도별도보존한다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('BurningFrame 초안successor SHA 오류')
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,total=426,normal=426,new_native_inputs=0,cpp_completed=False),ensure_ascii=False))


# 명시CLI로만이전전체원문과현재소비자의successor를생성하거나확인한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
