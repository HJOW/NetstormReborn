#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제타입/좁은의식·알타후속/섬배치·Money·피해반응의단위11 최종근거를기록한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

REPORT=ROOT/'cpppj/recovery-campaign-runtime11-evidence.json'  # 이번단위의최종Release/현재출처를기록한다.
MODULES={'archertype':(42,0,2),'daisregular-core':(668,0,0),'altarsacrifice':(2866,0,0),
         'islandbuilder':(814,342,0),'fortmoney':(388,20,0),'dudenotice':(414,0,0)}  # 정상/assert/접두를별도로센다.
FILTERS={'ArcherTypeData_':4,'rawDaisRegular_':1,'rawAltarSacrifice_':1,'IslandBuilder_':3,
         'FortMoneyLoad_':3,'DudeNotice_':3,'ProcessRetake_':5,'ProcessForm_':4,'canon_type_':4,'SpRotate_':4}  # 새15개와기존17개의고유집중검사다.
AUDITS={'decomp_archertype_oracle':['--verify'],'analyze_archertype_draft_lineage':['--verify'],
        'analyze_archertype_pop_lineage':['--verify'],'decomp_daisregular_oracle':['--verify'],
        'decomp_altarsacrifice_oracle':['--verify'],'analyze_altarsacrifice_successor':['--verify'],
        'analyze_islandbuilder_decoder_lineage':[],'analyze_islandbuilder_comment_lineage':[],
        'decomp_fortmoney_oracle':['--verify'],'analyze_fortmoney_cpp_binding':['--verify'],
        'analyze_fortmoney_cpp_correction':[],'decomp_dudenotice_oracle':['--verify'],
        'analyze_dudenotice_draft_lineage':['--verify'],'analyze_dudenotice_point_lineage':['--verify'],
        'analyze_dude_regular_reset_lineage':['--verify']}  # 이전전체본문/현재SHA를각전용감사기로확인한다.
LINEAGES=('archertype-draft-lineage','archertype-pop-lineage','altarsacrifice-successor',
          'islandbuilder-decoder-lineage','islandbuilder-comment-lineage','islandbuilder-partial-profile-lineage',
          'fortmoney-cpp-binding','fortmoney-cpp-correction','dudenotice-draft-lineage',
          'dudenotice-point-lineage','dude-regular-reset-lineage')  # 역사적보고서SHA를현재값으로덮어쓰지않는다.


def generate():
    """정상/assert/접두분리·전용역변환·최종성공로그로현재단위를기록한다."""
    paths=[Path(__file__).resolve(),ROOT/'tools/analyze_campaign_runtime4.py',ROOT/'tools/analyze_campaign_runtime6.py']
    modules,focused,names={},{},set()
    # 각전체fixture를보고서의고유입력수와비교한다. 두x87반환은다시더하지않는다.
    for name,(normal,assertions,prefix) in MODULES.items():
        report=ROOT/f'cpppj/recovery-{name}-evidence.json'
        data=json.loads(report.read_text(encoding='utf-8'))
        assert data['host']=='HJOW-Athlon' and data['total']==normal+assertions+prefix,name
        fixture=ROOT/f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows=[line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==normal+assertions+prefix,name
        # Island의당시검사SHA는한국어주석successor가연결하며다른보고서는현재SHA까지같다.
        if name!='islandbuilder':audit_files(data)
        modules[name]=dict(normal_cases=normal,assertion_cases=assertions,prefix_cases=prefix,fixture_rows=len(rows))
        paths.extend([report,fixture])
    # 감사기는이전전체자료·정확역변환과현재출처를각자확인한다.
    for name,arguments in AUDITS.items():
        tool=ROOT/f'tools/{name}.py'
        subprocess.run([sys.executable,str(tool),*arguments],cwd=ROOT,check=True)
        paths.append(tool)
    # 별도lineage 전체바이트를이번최종스냅샷에연결한다.
    for name in LINEAGES:
        paths.append(ROOT/f'cpppj/recovery-{name}-evidence.json')
    # 실패/빈필터로그가아닌실제최종성공한고유이름을기록한다.
    for name,expected in FILTERS.items():
        log=ROOT/f'extracted/campaign-runtime11-{name}final-tests.log'
        assert test_count(log)==expected,name
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)',read_log(log)))
        focused[name]=expected
        paths.append(log)
    assert len(names)==32
    whole=ROOT/'extracted/campaign-runtime11-all-final-tests.log'
    assert test_count(whole)==1038
    build=ROOT/'extracted/campaign-runtime11-build-final.log'
    content=read_log(build)
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',content,re.I)
    assert 'netstorm_tests.exe' in content and 'NetstormCpp.exe' in content
    paths.extend([whole,build,ROOT/'cpppj/recovery-campaign-runtime10-evidence.json',ROOT/'cpppj/src/o/CMakeLists.txt',ROOT/'cpppj/tests/CMakeLists.txt'])
    # 새몸체/공유API의전체파일을성공한바이너리에연결한다.
    for name in ('RawArcherTypeData','RawDaisRegular','RawAltarSacrifice','RawDaisLifecycle','RawIslandBuilder','RawFortMoneyLoad','RawDudeNotice','CanonDecoder','SquidProcess'):
        paths.extend([ROOT/f'cpppj/src/o/{name}.h',ROOT/f'cpppj/src/o/{name}.cpp'])
    # 실제전체입력과소비자검사 소스를고정한다.
    for name in ('ArcherTypeDataTests','DaisRegularTests','AltarSacrificeTests','IslandBuilderTests','FortMoneyLoadTests','DudeNoticeTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    # Release 검사/게임 실행파일도이번성공결과의바이트로고정한다.
    for name in ('netstorm_tests.exe','NetstormCpp.exe'):
        paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    data=dict(schema=1,host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',
              date='2026-10-14',unit='20261014-03',new_normal_cases=sum(value[0] for value in MODULES.values()),
              new_assertion_cases=sum(value[1] for value in MODULES.values()),new_prefix_cases=sum(value[2] for value in MODULES.values()),
              modules=modules,new_cpp_tests=15,focused_logs_tests=focused,focused_unique_tests=sorted(names),focused_unique_count=32,
              all_tests=1038,build_warnings=0,build_errors=0,game_runs=0,new_shared_api_native_cases=0,
              same_input_correction_new_cases=0,lifecycle_native_not_added=1928,postdestroy_native_not_added=4412,
              full_gameworld_completed=False,campaign_completed=False,files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
              limitations=['Dais는초기668의좁은진입점이며Rune304/전체Progress 등의완료근거가아니다.',
                           'Altar의마지막전체Damageable는명시경계이며payload9.25F 현재격자와실제0.12F caller의차이는후속이다.',
                           'Lifecycle 실제Pop등록/전체PostDestroy/초기Regular count설정은다음단위자료이며새관찰에더하지않는다.',
                           'Dude의Burning모드0/Path24 하위factory는미완료다.',
                           'Dude fixture의 두 기존 endpoint 입력은 모두 불일치이며 같은 endpoint의 자연반환은 아직 관찰하지 않았다.',
                           'Money 전체SP배열비교는Badges/Technology/전체저장부모·GameWorld설치/캠페인완주완료의근거가아니다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위11 정상5192/assert362/접두2·고유집중32·전체1038 최종근거 생성')


def verify():
    """최종전체SHA와실제전체성공검사 수를감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    assert test_count(ROOT/'extracted/campaign-runtime11-all-final-tests.log')==data['all_tests']
    print('단위11 최종SHA/전체검사 감사 통과')


# 기본호출은최종근거생성이고 --verify는기록한현재바이트감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
