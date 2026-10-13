#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""534개부모관찰·초안EOF교정·현재같은월드child소비자의전체출처를연결한다."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

# 새13만고정하며기존12/공통body의변경은하지않는다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/recovery-history/fortworldload-before-static-namespace'
NATIVE='cpppj/recovery-fortworldload-evidence.json'
REPORT=ROOT/'cpppj/recovery-fortworldload-binding-evidence.json'
FIXED=['cpppj/src/o/RawFortWorldLoad.cpp','cpppj/src/client/RawFortWorldLoadBinding.cpp']
SOURCE=['cpppj/src/o/RawFortWorldLoad.h',FIXED[0],'cpppj/src/client/RawFortWorldLoadBinding.h',FIXED[1],'cpppj/tests/FortWorldLoadTests.cpp','cpppj/tests/FortWorldLoadBindingTests.cpp']
# 같은실제생성/지역/게임난수/CRT/장부/Graph와완료child의현재전체파일이다.
DEPENDENCIES=[f'cpppj/src/o/{name}.{extension}' for name in ('RawIslandBuilder','RawIslandRuntime','ChunkMap','CanonDecoder','RiftType','RawTerrainContext','RawTerrainGrowth','RawTerrainStamp',
 'RawFortRandomPlacement','RawFarmCluster','RawFortObjectLoad','RawFortFinishOwner','RawFortMoneyLoad','RawFortMetaLoad','RawGraph','Graph','SidPool','SquidHash','SquidFactory','SquidReward','SpStore','RawCarrierPostPop','RawCanonPlacementPipeline') for extension in ('h','cpp')]
DEPENDENCIES += [f'cpppj/src/client/{name}.{extension}' for name in ('RawSurfaceWorld','RawNormalTerrainWorldBinding','RawTerrainWorldBinding') for extension in ('h','cpp')]
LINKS=['cpppj/recovery-fortmeta-evidence.json','cpppj/recovery-fortmeta-cpp-binding-evidence.json','cpppj/recovery-fortmeta-namespace-lineage-evidence.json',
 'cpppj/recovery-fortmoney-evidence.json','cpppj/recovery-fortmoney-cpp-correction-evidence.json','cpppj/recovery-islandbuilder-evidence.json',
 'cpppj/recovery-islandbuilder-comment-lineage-evidence.json','cpppj/recovery-graphrebuild-evidence.json']


def pin(name):
    """전체바이트의크기/SHA를기록한다."""
    raw=(ROOT/name).read_bytes();return dict(path=name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def native():
    """원래534보고서는보존하며CPP한파일은명시교정전전체snapshot을가리킨다."""
    spec=importlib.util.spec_from_file_location('fortworld_original_native',ROOT/'tools/decomp_fortworldload_oracle.py');observed=importlib.util.module_from_spec(spec);spec.loader.exec_module(observed)
    digest=observed.digest;observed.digest=lambda path:digest(ROOT/f'{HISTORY}/{FIXED[0]}' if Path(path).resolve()==(ROOT/FIXED[0]).resolve() else path)
    observed.verify();report=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));assert report['total']==534 and report['normal_total']==252 and report['assert_total']==282
    return report


def record():
    """전체초안역사/2brace교정/현재6파일과완료child들의출처를최초연결한다."""
    if REPORT.exists():raise RuntimeError('FortWorld binding 덮어쓰기 금지')
    native();old=[pin(path.relative_to(ROOT).as_posix()) for path in sorted((ROOT/HISTORY).rglob('*')) if path.is_file()]
    proof=dict(host='HJOW-Athlon',date='2026-10-14',native_unique=534,normal_unique=252,assert_unique=282,new_binding_native=0,
      preserved_native_report=pin(NATIVE),source=list(map(pin,SOURCE)),dependencies=list(map(pin,DEPENDENCIES)),linked_previous_reports=list(map(pin,LINKS)),
      old_files=old,tool=pin('tools/analyze_fortworldload_binding.py'),static_namespace_fix=dict(files=FIXED,added_each_hex='7d0a'),cpp_tests=3,cpp_test_result='새13공유등록/실행대기',
      limitations=['부모oracle에서새부모/Territory/zero120과기존Builder/Runtime를실제실행한다. 상위순서관찰의child경계를전체하위완료로세지않는다.',
      '현재childbinding은정상지형/객체/Money/Meta/Graph0의완료body를참조한다. 같은풀과현재Template선택효과를필수로받는다.',
      '선택실제소비자는Form없는빈저장객체/빈Technology와loadingDepth1이다. 부모getter/UI는명시관찰경계이고TypeNames/Populate/군도는미복원요청을거부한다.',
      '현재RawGraph의Form<70전체풀거부는보존하며실제Technology/ProcessForm이있는저장월드전체완료를주장하지않는다.',
      '첫smoke의missingTemplate 복구는호출중ECX가아닌진입때의Template this를사용하도록교정했다. 이전whole도구를보존하며표본/실패를534에더하지않는다.',
      'native최초보고서의CPP namespace닫힘누락은EOF brace2BYTE만추가하고전체역변환으로연결한다. native원문/534행/도구 변경0이다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체현재파일과이전초안의정확EOF역변환·기존534출처를감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));native();assert proof['new_binding_native']==0 and proof['native_unique']==534
    # 각CPP본문은보존된초안에정확한2BYTE만더한것이다.
    for name in FIXED:assert (ROOT/name).read_bytes()==(ROOT/f'{HISTORY}/{name}').read_bytes()+b'}\n',name
    # 전체역사/원본보고서/현재source·의존성을하나도생략하지않는다.
    for item in [proof['preserved_native_report'],proof['tool'],*proof['source'],*proof['dependencies'],*proof['linked_previous_reports'],*proof['old_files']]:assert pin(item['path'])==item,item['path']
    print('FortWorld binding 감사: 기존534=정상252/assert282, 새binding0, EOF2파일정확역변환/CPP3대기')


# --record는최초기록이고기본은현재전체출처감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
