#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort Money의현재원본408입력과실제월드장부/저장소C++검사 출처를연결한다."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 공유저장소의절대기준경로다.
REPORT=ROOT/'cpppj/recovery-fortmoney-cpp-binding-evidence.json'  # native보고서를수정하지않는C++연결근거다.
FILES=('cpppj/recovery-fortmoney-evidence.json','cpppj/tests/fixtures/fortmoney-x86.tsv',
       'cpppj/src/o/RawFortMoneyLoad.h','cpppj/src/o/RawFortMoneyLoad.cpp','cpppj/tests/FortMoneyLoadTests.cpp',
       'cpppj/src/client/RawSurfaceWorld.h','cpppj/src/client/RawSurfaceWorld.cpp',
       'cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp','tools/decomp_fortmoney_oracle.py',
       'tools/analyze_fortmoney_cpp_binding.py')  # 실제같은장부·분산저장소/난수참조와원본전수의현재파일이다.


def sha(path):
    """현재파일의전체바이트SHA를읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    """원본408의정상/assert 분리와현C++12열 대조의현재출처를고정한다."""
    subprocess.run([sys.executable,str(ROOT/'tools/decomp_fortmoney_oracle.py'),'--verify'],cwd=ROOT,check=True)
    native=json.loads((ROOT/'cpppj/recovery-fortmoney-evidence.json').read_text(encoding='utf-8'))
    assert native['total']==408
    normal=sum(item['normal_cases'] for item in native['editions'].values())
    assertions=sum(item['assert_cases'] for item in native['editions'].values())
    assert (normal,assertions)==(388,20)
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/fortmoney-x86.tsv').read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==408 and all(len(row)==12 for row in rows)
    counts={edition:sum(row[0]==edition for row in rows) for edition in ('originals','originalCD')}
    assert counts=={'originals':244,'originalCD':164}
    data=dict(host='HJOW-Athlon',last_decompile_host='HJOW-Athlon',last_decompile_date='2026-10-14',
              normal_cases=388,assertion_cases=20,fixture_cases=408,new_cpp_tests=3,new_native_binding_cases=0,
              reused_sp_native_not_added=True,game_runs=0,
              files={name:sha(ROOT/name) for name in FILES},
              scope='실제같은WorldCore.bookkeeping의현재localOwner·같은분산SP/게임난수를3검사에서참조한다. native408의기대값을재계산하지않는다.',
              limitations=['Badges/Technology/전체저장부모와실제GameWorld설치·캠페인완주는미완료다.',
                           'World소비자의미사용타입/프레임/SHP는합성입력이며실제새자산관찰이아니다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """현재원본/전체Cpp/실제소비자의전체바이트를다시감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 각기록파일의전체SHA를확인한다.
    for name, expected in data['files'].items():
        assert sha(ROOT/name)==expected,name
    print('FortMoney C++현재SHA: 원본408·소비자3·새binding native0')


# 기본호출은기록이며 --verify는저장한현재출처의감사다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
