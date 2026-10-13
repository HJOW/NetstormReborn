#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""가이저AI 검사 A5 입력의 정수폭 경고만 교정하고 기존604/전체자료를 보존한다."""
import argparse
import hashlib
import importlib.util
import json
import shutil
from pathlib import Path

# 수정대상은Test literal 한 곳이며 Native/생산/입력바이트는 불변이다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/recovery-history/geyser-ai-before-test-width'
TEST='cpppj/tests/GeyserAiRegistrationTests.cpp'
NATIVE='cpppj/recovery-geyser-ai-registration-evidence.json'
REPORT=ROOT/'cpppj/recovery-geyser-ai-test-width-lineage-evidence.json'


def pin(name):
    """전체파일 길이와 SHA256을 읽는다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def capture():
    """교정전생산/원본604/tool/report/현재9기록 API와실패로그를whole파일로보존한다."""
    target=ROOT/HISTORY
    if target.exists():raise RuntimeError('GeyserAI 교정전전체자료 덮어쓰기 금지')
    proof=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));names=[NATIVE,TEST,'cpppj/src/o/RawGeyserAiRegistration.h','cpppj/src/o/RawGeyserAiRegistration.cpp',
        'cpppj/tests/fixtures/geyser-ai-registration-x86.tsv','tools/decomp_geyser_ai_registration_oracle.py','tools/ghidra/geyser-ai-registration-functions.json',
        'docs/exe/cpp-geyser-ai-registration-20261014.md','cpppj/tests/RawSceneSupport.h','cpppj/src/o/RawPlayerWorld.h','cpppj/src/o/RawPlayerWorld.cpp',
        'cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp','cpppj/recovery-player-postpop-binding-lineage-evidence.json','extracted/campaign-runtime14-build-third.log']
    saved=[]
    # 보호PE는변경없이해시로연결하고그외604관찰출처의현재전체파일도보존한다.
    names.extend(name for name in proof['files'] if not name.startswith(('originals/','originalCD/')))
    for name in sorted(set(names)):
        source=ROOT/name;destination=target/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,destination);saved.append(pin(name))
    (target/'capture.json').write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',native_total=604,normal_unique=168,unrestored_boundary_unique=436,assert_unique=0,files=saved),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def old_native():
    """당시 Test view만whole보존본으로연결해기존Native감사함수를그대로실행한다."""
    spec=importlib.util.spec_from_file_location('geyser_ai_before_test_width',ROOT/'tools/decomp_geyser_ai_registration_oracle.py');observed=importlib.util.module_from_spec(spec);spec.loader.exec_module(observed);previous=observed.digest
    observed.digest=lambda path:previous(ROOT/HISTORY/TEST if Path(path).resolve()==(ROOT/TEST).resolve() else path);observed.verify()


def audit():
    """정확literal역변환과당시전체파일/Native604분류를감사한다."""
    saved=json.loads((ROOT/HISTORY/'capture.json').read_text(encoding='utf-8'))
    # 보존된전체자료의바이트를현재보고서와섞지않고확인한다.
    for item in saved['files']:
        current=pin(f'{HISTORY}/{item["path"]}');assert current['sha256']==item['sha256'] and current['bytes']==item['bytes'],item['path']
    old=(ROOT/HISTORY/TEST).read_bytes();new=(ROOT/TEST).read_bytes();before=b'std::fill(players.begin(),players.end(),0xa5);';after=b'std::fill(players.begin(),players.end(),std::uint8_t{0xa5});'
    assert old.count(before)==new.count(after)==1 and new.replace(after,before)==old
    proof=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));assert (ROOT/NATIVE).read_bytes()==(ROOT/HISTORY/NATIVE).read_bytes()
    assert proof['total']==604 and proof['normal_total']==168 and proof['unrestored_boundary_total']==436 and proof['assert_total']==0;old_native()


def record():
    """새source출처를별도successor로연결하고Native원문 SHA를유지한다."""
    if REPORT.exists():raise RuntimeError('GeyserAI 폭교정 successor 덮어쓰기 금지')
    audit();saved=[pin(path.relative_to(ROOT).as_posix()) for path in sorted((ROOT/HISTORY).rglob('*')) if path.is_file()]
    proof=dict(host='HJOW-Athlon',date='2026-10-14',native_total=604,normal_unique=168,unrestored_boundary_unique=436,assert_unique=0,new_native_unique=0,
        history=saved,current_test=pin(TEST),current_native=pin(NATIVE),tool=pin('tools/analyze_geyser_ai_test_width_lineage.py'),
        cpp_state='세번째공유build C4244 입력literal폭 교정/재빌드대기',limits=['std::fill의int0xa5를동일BYTE값std::uint8_t{0xa5}로한곳만교정한다.','생산/Native604/입력/출력/분류/원본문서와세번째build로그를whole로보존했다.','초기report의Test SHA는보존된당시전체Test를가리키며기존Nativeverify를historical view로연결한다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """604분류/정확역변환과모든보존·현재파일을전수감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));audit()
    # 현재successor와보존whole파일의전체바이트를확인한다.
    for item in [*proof['history'],proof['current_test'],proof['current_native'],proof['tool']]:assert pin(item['path'])==item,item['path']
    print('GeyserAI Test폭: literal1곳정확역변환/Native604불변/추가원본0 감사통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true');parser.add_argument('--record',action='store_true');args=parser.parse_args()
    if args.capture:capture()
    else:
        if args.record:record()
        verify()
