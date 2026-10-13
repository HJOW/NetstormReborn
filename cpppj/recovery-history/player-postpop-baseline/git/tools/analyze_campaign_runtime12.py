#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""의식 수명/피해 후처리/Burning/저장 후처리와 작업 설정의 단위12 최종 근거를 기록한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT,digest,test_count
from analyze_campaign_runtime6 import read_log,audit_files

REPORT=ROOT/'cpppj/recovery-campaign-runtime12-evidence.json'  # 실제 최종 성공 로그와 현재 전체 바이트의 기록이다.
MODULES={'daislifecycle-current':(1928,0),'daisprogress':(2964,0),'damageablepostdestroy':(4412,0),
         'burningprocess-constructor':(256,0),'burningparticle':(564,0),'fortmeta':(394,556),
         'dudetask':(684,96),'daiscontentnotice':(1444,0)}  # 정상/진단 고유 입력이며 x87 반복 반환은 다시 더하지 않는다.
FILTERS={'rawDaisLifecycle_':1,'rawDaisProgress_':1,'DamageablePostDestroy_':3,'BurningVisual_':3,
         'BurningParticle_':2,'FortMetaLoad_':3,'DudeTask_':3,'rawDaisContentNotice_':1,'rawDaisRitual_':1,
         'ContentLifetime_':4,'ProcessRetake_':5,'ProcessForm_':4,'DudeNotice_':3,
         'SquidPostPop_':6,'rawAltarSacrifice_':1,'rawDaisRegular_':1}  # 새18개와 기존24개의 실제 고유 이름을 확인한다.
AUDITS={'analyze_regular_initial_count_lineage':[],'analyze_dais_postpop_lineage':['--verify'],
        'decomp_daislifecycle_current_oracle':['--verify'],'analyze_daislifecycle_current_successor':['--verify'],
        'analyze_dais_followups_cpp_correction':['--verify'],
        'analyze_daisprogress_cpp_correction':['--verify'],'analyze_damageablepostdestroy':[],
        'analyze_damageablepost_host_group_lineage':[],'decomp_burningprocess_oracle':['--verify'],
        'decomp_burningparticle_oracle':['--verify'],'analyze_fortmeta_namespace_lineage':[],
        'decomp_dudetask_oracle':['--verify']}  # 교정 감사가 Notice/의식의 이전 원문과 현재 Test를 연결한다.
LINEAGES=('regular-initial-count-lineage','dais-postpop-lineage','daisprogress-cpp-binding',
          'daislifecycle-current-successor','dais-ritual-consumer-binding',
          'daisprogress-cpp-correction','damageablepostdestroy-lineage','damageablepost-host-binding',
          'damageablepost-host-group-lineage',
          'fortmeta-cpp-binding','fortmeta-namespace-lineage','daiscontentnotice-successor',
          'daiscontentnotice-cpp-correction','dais-ritual-consumer-cpp-correction')  # 이전 전체 출처와 현재 최소 교정을 함께 보존한다.


def generate():
    """원본 입력 분리·전체 이전 몸체 보존·실제 성공한 바이너리와 로그를 현재 출처로 묶는다."""
    paths=[Path(__file__).resolve(),ROOT/'tools/analyze_campaign_runtime4.py',ROOT/'tools/analyze_campaign_runtime6.py',ROOT/'.gitattributes']
    modules,focused,names={},{},set()
    # 수명1928의 원래 자료는 Pop 등록/current 입력 successor가 연결하며 현재1928만 고유 수에 넣는다.
    for name,(normal,assertions) in MODULES.items():
        report=ROOT/f'cpppj/recovery-{name}-evidence.json'
        data=json.loads(report.read_text(encoding='utf-8'))
        assert data['host']=='HJOW-Athlon' and data['total']==normal+assertions,name
        fixture=ROOT/f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows=[line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==normal+assertions,name
        # Meta namespace와 Notice 입력 교정 전 SHA는 전체 역변환 감사가 현재 몸체에 연결한다.
        if name not in ('fortmeta','daiscontentnotice'):audit_files(data)
        modules[name]=dict(normal_cases=normal,assertion_cases=assertions,fixture_rows=len(rows))
        paths.extend([report,fixture])
    # 도구마다 정확 역변환/기준 Git·작업 전체 바이트/현재 source를 검사한다.
    for name,arguments in AUDITS.items():
        tool=ROOT/f'tools/{name}.py'
        subprocess.run([sys.executable,str(tool),*arguments],cwd=ROOT,check=True)
        paths.append(tool)
    # 별도 successor/소비자 근거의 전체 바이트도 성공한 이번 상태에 고정한다.
    for name in LINEAGES:paths.append(ROOT/f'cpppj/recovery-{name}-evidence.json')
    # substring이 선택한 실제 검사 수와 고유 이름을 모두 검산한다.
    for name,expected in FILTERS.items():
        log=ROOT/f'extracted/campaign-runtime12-{name}final-tests.log'
        assert test_count(log)==expected,name
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)',read_log(log)))
        focused[name]=expected
        paths.append(log)
    assert len(names)==42
    whole=ROOT/'extracted/campaign-runtime12-all-final-tests.log'
    assert test_count(whole)==1056
    build=ROOT/'extracted/campaign-runtime12-build-final.log'
    content=read_log(build)
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',content,re.I)
    assert 'netstorm_tests.exe' in content and 'NetstormCpp.exe' in content
    paths.extend([whole,build,ROOT/'cpppj/recovery-campaign-runtime11-evidence.json',ROOT/'cpppj/src/o/CMakeLists.txt',ROOT/'cpppj/tests/CMakeLists.txt'])
    # 생성/실행 전체를 보존한 공유 API와 새 생산 몸체를 현재 바이너리에 연결한다.
    for name in ('SquidProcess','SquidPostPop','RawDaisLifecycle','RawDamageablePostDestroy','RawBurningVisual','RawFortMetaLoad','RawDudeTask','RawDaisContentNotice'):
        paths.extend([ROOT/f'cpppj/src/o/{name}.h',ROOT/f'cpppj/src/o/{name}.cpp'])
    # 실제 소비자/독립 원본 전수 검사 소스 전체를 고정한다.
    for name in ('DaisLifecycleTests','DaisProgressTests','DamageablePostDestroyTests','DamageablePostDestroyHostTests','BurningVisualTests','BurningParticleTests','FortMetaLoadTests','DudeTaskTests','DaisContentNoticeTests','DaisRitualConsumerTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    # 실행한 두 Release 산출물의 전체 바이트를 성공 로그에 연결한다.
    for name in ('netstorm_tests.exe','NetstormCpp.exe'):paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    data=dict(schema=1,host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',
              date='2026-10-14',unit='20261014-04',new_normal_cases=sum(value[0] for value in MODULES.values()),
              new_assertion_cases=sum(value[1] for value in MODULES.values()),new_prefix_cases=0,modules=modules,
              new_cpp_tests=18,focused_logs_tests=focused,focused_unique_tests=sorted(names),focused_unique_count=42,
              all_tests=1056,build_warnings=0,build_errors=0,game_runs=0,new_shared_api_native_cases=0,
              reused_old_native_cases_not_added=0,same_input_correction_new_cases=0,
              full_gameworld_completed=False,campaign_completed=False,
              files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
              limitations=['의식 소비자는 비전투/HP16/genus0 합성 입력이며 실제 캠페인 자산/전체 Progress·Bridge·해방·Path 분기는 후속이다.',
                           'Post Host는 packed count/실제 Regular와 첫 Kernel 호출이며 실제 Dude 드롭/피해 지면/전체 가상 HP는 후속이다.',
                           'Burning은 cache가 있는 생성자와 append이며 첫 팔레트/전체 frame/표시/삭제/Host는 후속이다.',
                           'FortMeta의 TypeNames 전신·현재 부모 getter·UI·전체 Fort 부모 설치는 미완료다.',
                           'DudeTask는 실제 Dude 생성/Pop/Path 및 전체 드롭 완료의 근거가 아니다.',
                           '실제 GameWorld/경제/AI/생산/운반·시각/음향·승패/UI·캠페인1-1 완주 동일성은 미완료다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위12 정상12646/assert652·새18·고유집중42·전체1056 최종근거 생성')


def verify():
    """실제 성공한 전체 검사 수와 현재 기록의 전체 SHA를 다시 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    assert test_count(ROOT/'extracted/campaign-runtime12-all-final-tests.log')==data['all_tests']
    print('단위12 최종 SHA/전체 검사 감사 통과')


# 기본 호출은 최종 자료 생성이며 --verify는 이미 기록한 현재 바이트 감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
