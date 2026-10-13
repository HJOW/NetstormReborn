#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체4412 원본/이전CPP를 보존하며 새 실제 Host count 소비자의 출처만 후속 연결한다."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

# 고정11 commit의완료몸체와새12 최소직접count API를연결한다.
ROOT=Path(__file__).resolve().parent.parent
BASELINE='5bdb5be'
HISTORY='cpppj/recovery-history/damageablepost-host-baseline'
REPORT=ROOT/'cpppj/recovery-damageablepost-host-binding-evidence.json'
FROZEN=['cpppj/src/o/RawDamageablePostDestroy.h','cpppj/src/o/RawDamageablePostDestroy.cpp','cpppj/tests/DamageablePostDestroyTests.cpp',
 'cpppj/recovery-damageablepostdestroy-evidence.json','cpppj/recovery-damageablepostdestroy-lineage-evidence.json','tools/decomp_damageablepostdestroy_oracle.py',
 'tools/analyze_damageablepostdestroy.py','cpppj/tests/fixtures/damageablepostdestroy-x86.tsv']
# 실제consumer가재사용한단일풀/Kernel/Factory/Regular/HasHP/중심/해시/삭제몸체의전체파일이다.
DEPENDENCIES=[f'cpppj/src/o/{name}.{extension}' for name in ('SidPool','SquidFactory','SquidForms','ContentFormHost','Kernel','BaseProcess','SquidProcess',
 'RawSquidDestroy','SquidUnpop','SquidHash','SquidReward','RawSquidFinder','RawSquidNeighbors','Bridge','RiftType','SpStore') for extension in ('h','cpp')]


def pin(path):
    """파일전체크기/SHA를고정한다."""
    raw=(ROOT/path).read_bytes();return dict(path=path,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())


def native():
    """변경하지않은부모4412전체관찰과앞의owner한줄교정을그대로감사한다."""
    subprocess.run([sys.executable,str(ROOT/'tools/analyze_damageablepostdestroy.py')],cwd=ROOT,check=True)
    subprocess.run([sys.executable,str(ROOT/'tools/analyze_regular_initial_count_lineage.py')],cwd=ROOT,check=True)


def record():
    """이전전체8파일을보존하고새Host검사/현재count API와전체의존성을최초기록한다."""
    if REPORT.exists():raise RuntimeError('Post Host binding 덮어쓰기 금지')
    native();snapshots=[];dependencies=[]
    # 원본/생산/CPP기존2검사/fixture는현재와보존된전체바이트가동일하다.
    for name in FROZEN:
        old=f'{HISTORY}/{name}';assert (ROOT/name).read_bytes()==(ROOT/old).read_bytes();snapshots.append(dict(current=pin(name),saved=pin(old)))
    # SquidProcess는root의정확API lineage가담당하며나머지는완료11전체파일과동일해야한다.
    for name in DEPENDENCIES:
        value=pin(name)
        if not name.endswith('/SquidProcess.h') and not name.endswith('/SquidProcess.cpp'):
            committed=subprocess.check_output(['git','show',f'{BASELINE}:{name}'],cwd=ROOT)
            assert committed.replace(b'\r\n',b'\n')==(ROOT/name).read_bytes().replace(b'\r\n',b'\n'),name
            value['baseline_git_blob_sha256']=hashlib.sha256(committed).hexdigest()
        dependencies.append(value)
    proof=dict(host='HJOW-Athlon',date='2026-10-14',baseline_commit=BASELINE,new_native_unique=0,preserved_native_unique=4412,
      preserved_normal_unique=4412,preserved_assert_unique=0,production_parent_changed=False,actual_host_test_count=1,
      frozen_files=snapshots,dependencies=dependencies,root_count_lineage=pin('cpppj/recovery-regular-initial-count-lineage-evidence.json'),
      consumer=pin('cpppj/tests/DamageablePostDestroyHostTests.cpp'),tool=pin('tools/analyze_damageablepost_host_binding.py'),
      cpp_test_result='공유등록/실행대기',limitations=['새consumer는원본packed count/event0x1a2b/payload0→실제Host Form/Kernel/초기double시각→첫handler인자를확인한다.',
      '최대HP100/131073과두판본의같은실제Factory/풀을사용하며+20 count를SetInitialCount로직접쓴다.',
      'handler는명시관찰후0반환으로실제ProcessForm정리를검사한다. 실제DamageableHandle/지면피해/가상HP전체를완료로세지않는다.',
      'Dude생성/DropIsland/전체공통post/다리출력은선택범위밖이다. 요청되는미복원효과는명시거부한다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """원본전체바이트불변/현재전체의존성/후속API provenance를감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));native();assert proof['new_native_unique']==0 and proof['preserved_native_unique']==4412 and not proof['production_parent_changed']
    items=[proof['root_count_lineage'],proof['consumer'],proof['tool'],*proof['dependencies']]
    # 저장원문과현재원문은출처를잃지않고모두감사한다.
    for pair in proof['frozen_files']:items.extend((pair['current'],pair['saved']))
    for item in items:
        current=pin(item['path']);assert all(current[key]==item[key] for key in ('path','bytes','sha256')),item['path']
    print('Post Host binding 감사: 기존4412/생산원문불변, 새원본0, 별도실제Host검사1')


# --record는첫후속연결이며기본실행은전체출처감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
