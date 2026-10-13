#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 생성/드롭·Fort 부모·내부 Form 그래프·의식 경로 연결의 최종 근거를 기록한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT,digest,test_count
from analyze_campaign_runtime6 import read_log,audit_files

REPORT=ROOT/'cpppj/recovery-campaign-runtime13-evidence.json'  # 현재 성공한 전체 소스/바이너리/로그의 출처다.
MODULES={'dudespawn':(288,0),'dudedrop':(306,2),'fortworldload':(252,282),'graphforms':(64,0),'daisbridge':(640,0)}  # 고유 정상/진단을 정밀도 반복과 구분한다.
FILTERS={'DudeSpawn_':2,'DudeDrop_':2,'FortWorldLoad_':2,'FortWorldLoadBinding_':1,'GraphForms_':2,'rawDaisBridge_':1,
         'RawGraph_':14,'rawPathStart_':2,'DudeTask_':3,'DudeNotice_':3,'DamageablePostDestroy_':3,
         'ContentLifetime_':4,'ProcessRetake_':5,'ProcessForm_':4,'SquidPostPop_':6,
         'rawDaisProgress_':1,'rawDaisLifecycle_':1,'rawDaisRitual_':1,'rawDaisRegular_':1,'rawAltarSacrifice_':1}  # 새10/기존49의 중복 없는 실제 이름을 검산한다.
AUDITS={'analyze_graph_forms_test_lineage':['--verify'],'decomp_graphforms_oracle':['--verify'],
        'analyze_dude_postpop_binding_lineage':['--verify'],'analyze_dudespawn_draft_lineage':['--verify'],
        'analyze_dudedrop_draft_lineage':['--verify'],'decomp_daisbridge_oracle':['--verify'],
        'analyze_daisbridge_surface_input_successor':['--verify'],'decomp_fortworld_builder_consumer_oracle':['--verify']}  # 현재 SHA와 당시 전체 원문을 전용 CLI 계약으로 확인한다.
LINEAGES=('graph-forms-lineage','graph-forms-test-lineage','dude-postpop-binding-lineage','dudespawn-draft-lineage',
          'dudedrop-draft-lineage','fortworldload-binding','fortworldload-consumer-lineage',
          'fortworldload-builder-consumer','daisbridge-cpp-binding','daisbridge-cpp-correction',
          'daisbridge-surface-input-successor')  # 이전 전체 원문과 현재 좁은 연결을 보존한다.


def generate():
    """전수 원본 자료·이전 전체몸체·현재 성공한 실제 로그를 한 상태로 고정한다."""
    paths=[Path(__file__).resolve(),ROOT/'tools/analyze_campaign_runtime4.py',ROOT/'tools/analyze_campaign_runtime6.py',ROOT/'.gitattributes']
    modules,focused,names={},{},set()
    # 각 새 전용 원본 입력은 서로 다른 부모 진입점이며 선행 호출 횟수는 다시 더하지 않는다.
    for name,(normal,assertions) in MODULES.items():
        report=ROOT/f'cpppj/recovery-{name}-evidence.json'
        data=json.loads(report.read_text(encoding='utf-8'))
        assert data['host']=='HJOW-Athlon' and data['total']==normal+assertions,name
        fixture=ROOT/f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows=[line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==normal+assertions,name
        # Dude의 공유 연결과 Fort의 EOF 교정 전 SHA는 전체 successor가 현재에 연결한다.
        if name in ('graphforms','daisbridge'):audit_files(data)
        modules[name]=dict(normal_cases=normal,assertion_cases=assertions,fixture_rows=len(rows))
        paths.extend([report,fixture])
    # 공통 소스의 정확 역변환과 이전 관찰 불변 및 현재 소비자 SHA를 각각 감사한다.
    for name,arguments in AUDITS.items():
        tool=ROOT/f'tools/{name}.py'
        subprocess.run([sys.executable,str(tool),*arguments],cwd=ROOT,check=True)
        paths.append(tool)
    # 원래 보고서와 바이트 단위 교정/연결 근거의 원문을 각각 보존한다.
    for name in LINEAGES:paths.append(ROOT/f'cpppj/recovery-{name}-evidence.json')
    # 실제 substring이 선택한 성공 검사 수와 고유 이름을 함께 확인한다.
    for name,expected in FILTERS.items():
        log=ROOT/f'extracted/campaign-runtime13-{name}final-tests.log'
        assert test_count(log)==expected,name
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)',read_log(log)))
        focused[name]=expected
        paths.append(log)
    assert len(names)==59
    whole=ROOT/'extracted/campaign-runtime13-all-final-tests.log'
    assert test_count(whole)==1066
    build=ROOT/'extracted/campaign-runtime13-build-final.log'
    content=read_log(build)
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',content,re.I)
    assert 'netstorm_tests.exe' in content and 'NetstormCpp.exe' in content
    paths.extend([whole,build,ROOT/'cpppj/recovery-campaign-runtime12-evidence.json',ROOT/'cpppj/src/o/CMakeLists.txt',
                  ROOT/'cpppj/src/client/CMakeLists.txt',ROOT/'cpppj/tests/CMakeLists.txt'])
    # 최소 공유 분배/공간 변경과 새 실제 부모/생성 몸체의 전체 파일을 현재 바이너리에 연결한다.
    for name in ('SquidPostPop','SquidPop','RawGraph','RawDudeSpawn','RawDudeDrop','RawFortWorldLoad'):
        paths.extend([ROOT/f'cpppj/src/o/{name}.h',ROOT/f'cpppj/src/o/{name}.cpp'])
    paths.extend([ROOT/'cpppj/src/client/RawFortWorldLoadBinding.h',ROOT/'cpppj/src/client/RawFortWorldLoadBinding.cpp'])
    # 기존 실패전 보호와 새 원본 전수/같은 Host 소비자의 전체 검사 source를 고정한다.
    for name in ('RawGraphTests','DudeSpawnTests','DudeDropTests','FortWorldLoadTests','FortWorldLoadBindingTests','GraphFormsTests','DaisBridgeTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    paths.append(ROOT/'cpppj/tests/DudeSpawnSupport.h')
    # 실행한 두 Release 파일의 바이트를 실제 성공 로그에 연결한다.
    for name in ('netstorm_tests.exe','NetstormCpp.exe'):paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    data=dict(schema=1,host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',
              date='2026-10-14',unit='20261014-05',new_normal_cases=sum(value[0] for value in MODULES.values()),
              new_assertion_cases=sum(value[1] for value in MODULES.values()),additional_existing_body_consumer_golden=2,
              new_shared_api_native_cases=0,new_cpp_tests=10,modules=modules,focused_logs_tests=focused,
              focused_unique_tests=sorted(names),focused_unique_count=59,all_tests=1066,build_warnings=0,build_errors=0,
              game_runs=0,full_gameworld_completed=False,campaign_completed=False,
              files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
              limitations=['Dude의 생성/드롭/첫 I/작업등록이며 실제 걷기·도착·전체source삭제는 후속이다.',
                           'Fort 부모의 하위 요청 경계와 C++ 같은월드 선택범위 소비자를 구분한다. Builder 골든2는 기존몸체 추가소비자 입력이다.',
                           'Farm 연결 범위는 최종Retype의 독립 동일성 완료 근거가 아니며 TypeNames/getter/UI/전체 저장월드는 후속이다.',
                           'Graph 입력은 비표면 내부Form6과 표면1이며 실제 복수자산 전체월드의 초기설치는 후속이다.',
                           'Bridge는 실제 Path 생성/BFS/두번째Prepare/깊이이며 전체 걷기/도착/의식307·룬304까지 완료한 것이 아니다.',
                           '실제 GameWorld/경제/AI/생산/운반·시각/음향·승패/UI·캠페인1-1 완주 동일성은 미완료다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위13 새정상1550/assert284·기존Builder소비자2·새10·집중59·전체1066 근거 생성')


def verify():
    """실제 전체 성공 수와 최종 고정한 현재 파일의 전체 SHA를 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    assert test_count(ROOT/'extracted/campaign-runtime13-all-final-tests.log')==data['all_tests']
    print('단위13 최종 전체 SHA/검사 수 감사 통과')


# 기본은 최종 성공 근거 생성이며 --verify는 이미 고정한 현재 바이트만 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
