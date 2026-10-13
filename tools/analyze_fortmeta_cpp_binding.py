#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""950개상위관찰과기존Contents/ContainedCreate의불변전체몸체를새같은풀소비자에연결한다."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

# 새wrapper만추가하며기존전체완료body는11 commit의출처를보존한다.
ROOT=Path(__file__).resolve().parent.parent
BASELINE='5bdb5be'
REPORT=ROOT/'cpppj/recovery-fortmeta-cpp-binding-evidence.json'
NATIVE='cpppj/recovery-fortmeta-evidence.json'
DEPENDENCIES=[f'cpppj/src/o/{name}.{extension}' for name in ('RawFortObjectLoad','RawContainedCreate','ContentFormHost','SquidForms','SquidFactory',
 'SidPool','SquidHash','RawSquidFinder','SquidReward','SpStore','RiftType','Template') for extension in ('h','cpp')]
DEPENDENCIES+=['cpppj/src/o/TypeConstructors.inc','cpppj/src/o/DerivedConstructors.inc','cpppj/tests/RawSceneSupport.h','cpppj/tests/TestSupport.h']
OLD=['cpppj/recovery-fortobjectload-evidence.json','cpppj/recovery-contained-evidence.json','cpppj/recovery-contentlifetime-evidence.json',
 'cpppj/recovery-formlifecycle-evidence.json','cpppj/recovery-factory-world-evidence.json']


def pin(name):
    """한파일의전체바이트출처를고정한다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def native():
    """두PE950개의전체현재SHA·고유입력·분리계수를감사한다."""
    subprocess.run([sys.executable,'-X','utf8',str(ROOT/'tools/decomp_fortmeta_oracle.py'),'--verify'],cwd=ROOT,check=True)
    report=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));assert report['total']==950
    assert sum(item['normal_cases'] for item in report['editions'].values())==394
    assert sum(item['assert_cases'] for item in report['editions'].values())==556
    return report


def record():
    """원본현재보고서와생산/검사및기존전체body의Git/현재SHA를최초연결한다."""
    if REPORT.exists():raise RuntimeError('FortMeta binding 덮어쓰기 금지')
    current=native();dependencies=[]
    # 기존파일은동일commit전체body이며CRLF/LF저장차이는양쪽전체SHA로구별한다.
    for name in [*DEPENDENCIES,*OLD]:
        item=pin(name);committed=subprocess.check_output(['git','show',f'{BASELINE}:{name}'],cwd=ROOT)
        assert committed.replace(b'\r\n',b'\n')==(ROOT/name).read_bytes().replace(b'\r\n',b'\n'),name
        item['git_blob_sha256']=hashlib.sha256(committed).hexdigest();dependencies.append(item)
    proof=dict(host='HJOW-Athlon',date='2026-10-14',baseline_commit=BASELINE,native_report=pin(NATIVE),native_unique=950,normal_unique=394,assert_unique=556,
       binding_new_native=0,existing_observations_recounted=0,existing_production_changed=False,current_cpp=[pin(name) for name in ('cpppj/src/o/RawFortMetaLoad.h','cpppj/src/o/RawFortMetaLoad.cpp','cpppj/tests/FortMetaLoadTests.cpp')],
       dependencies=dependencies,tool=pin('tools/analyze_fortmeta_cpp_binding.py'),cpp_tests=3,cpp_test_result='공유등록/실행대기',
       limitations=['신규950는장부/상위wrapper관찰이며이binding은새원본0이다. 기존완료1218/Contained/Form원본수를다시더하지않는다.',
       'Technology fourth arg1은mapped type0 debug assert를생략하는relaxedConversion이다. 초기화모드가아니다.',
       '현재parsed Technology목록과그원본section identity를확인한뒤기존Contents를호출한다. consumedpos는기존parser자료이며stream.end는보존한다.',
       '현재TypeNames준비/해제·technology부모getter·UI는필수명시경계다. 전체TypeNames/상위Fort/GameWorld완료를주장하지않는다.',
       '실제선택소비자는같은Factory/Pool/ContainedCreate/ContentForm으로두개를생성하며요청되지않은HP/Frame/Pop/Owner등은명시거부한다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재wrapper/전체950출처와기존body전체불변출처를감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));native();assert proof['binding_new_native']==proof['existing_observations_recounted']==0 and not proof['existing_production_changed']
    # 변경없던몸체의이전GitSHA와현재SHA를둘다유지하며현재전체파일을확인한다.
    for item in [proof['native_report'],proof['tool'],*proof['current_cpp'],*proof['dependencies']]:
        current=pin(item['path']);assert all(current[key]==item[key] for key in ('path','bytes','sha256')),item['path']
    print('FortMeta binding 감사: 950=정상394/assert556, 기존몸체불변/추가native0, C++3등록대기')


# --record는첫연결이며기본은현재전체파일감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
