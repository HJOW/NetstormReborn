#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 상위 관찰을 보존하고 현재 C++ 전용 검사 SHA를 별도 근거로 연결한다."""
import argparse,json
from pathlib import Path
from decomp_daisprogress_oracle import ROOT,FIXTURE,REPORT,sha,verify as native_verify

BINDING=ROOT/'cpppj/recovery-daisprogress-cpp-binding-evidence.json' # 원본 보고서를 수정하지 않는 C++ 검사 연결 근거다.
TEST=ROOT/'cpppj/tests/DaisProgressTests.cpp' # 같은 입력의 현재 상위 몸체 대조 검사다.

def expected():
    """독립 원본 현재 출처/고유 입력을 확인하고 새 검사 원문을 연결한다."""
    native_verify();native=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert all(len(row)==8 for row in rows) and native['total']==2964
    files={Path(__file__),REPORT,FIXTURE,TEST,ROOT/'tools/decomp_daisprogress_oracle.py'}
    # 이 상위 검사에서 호출한 생산 몸체의 전체 원문은 원본 보고서와 동일해야 한다.
    for name in ('cpppj/src/o/RawDaisRegular.h','cpppj/src/o/RawDaisRegular.cpp','cpppj/src/o/RawAltarSacrifice.h','cpppj/src/o/RawAltarSacrifice.cpp'):
        path=ROOT/name;assert sha(path)==native['files'][name];files.add(path)
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=native['total'],new_native_inputs=0,game_runs=0,
        scope='원본 보고서/fixture/생산 전체 원문 불변과 새 전용 C++ 검사 SHA의 연결. C++ 실행 결과는 공유 검사 로그로 별도 기록하며 의식 전체 소비자 완료로 표시하지 않는다.',
        files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)})

def main(check):
    """근거 작성 또는 모든 현재 출처의 일치를 검사한다."""
    data=expected()
    if check:assert json.loads(BINDING.read_text(encoding='utf-8'))==data
    else:BINDING.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 상위 원본 불변·현재 C++ 연결 감사:',data['normal_inputs'],'새 원본 추가:',data['new_native_inputs'])

# 명시 CLI 요청에서만 연결 자료를 작성하거나 현재 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
