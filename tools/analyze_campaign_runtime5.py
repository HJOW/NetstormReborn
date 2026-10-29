#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""투사체·총 직접 피해 시각·Ascend 정리의 최종 원본/C++ 실행 근거를 고정한다."""
import argparse
import json
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count

# 이전 완료 단위의 관찰은 별도 값으로 유지하며 이번3850개와 중복 합산하지 않는다.
REPORT=ROOT/'cpppj/run5.json'
MODULES={'gunmissile':296,'gunhit':2058,'vortexascend':1496}
# 실제 등록된 집중 검사와 같은 이름을 사용한다.
FILTERS={'Missile_':4,'GunHit_':3,'VortexAscend_':3}

def generate():
    """현재 원본 관찰·소스·실행 로그에서 성공한 작업 단위의 근거를 생성한다."""
    paths=[Path(__file__).resolve(),ROOT/'tools/analyze_campaign_runtime4.py'];focused={}
    # 동일 고유 입력 재생성과 x87 반복을 관찰 수에 다시 더하지 않는다.
    for name,count in MODULES.items():
        path=ROOT/f'cpppj/{name}.json';data=json.loads(path.read_text(encoding='utf-8'))
        if data['total']!=count or data.get('host')!='HJOW-Athlon':raise RuntimeError(f'원본 입력/PC 오류: {name}')
        # 원본 PE/export·oracle·fixture·현재 production SHA가 모두 같은 근거인지 확인한다.
        for filename,expected in data['files'].items():
            if digest(ROOT/filename)!=expected:raise RuntimeError(f'하위 SHA 불일치: {filename}')
        paths.append(path)
    # 집중 검사의 마지막 성공 요약과 실제 검사 수를 읽는다.
    for name,count in FILTERS.items():
        path=ROOT/f'extracted/run5-{name}-final.log';actual=test_count(path)
        if actual!=count:raise RuntimeError(f'집중 검사 수 오류: {name}')
        focused[name]=actual;paths.append(path)
    whole=ROOT/'extracted/run5-all-final.log';total=test_count(whole);paths.append(whole)
    # 이번 단위가 포함한 실제 수명 검사 소스와 빌드 출력도 같은 스냅샷에 고정한다.
    for name in ('MissileTests','GunHitTests','VortexAscendTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    for name in ('netstorm_tests.exe','Netstorm.exe'):
        paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    paths.extend([ROOT/'extracted/run5-build-final.log',ROOT/'cpppj/run4.json'])
    # 최종 바이너리에 포함된 후속 helper 접근 준비도 이전 전체 몸체 보존 lineage와 함께 고정한다.
    paths.extend([ROOT/'cpppj/src/o/RawPathRuntime.h',ROOT/'cpppj/src/o/RawPathRuntime.cpp',
                  ROOT/'tools/analyze_pathcleanup_runtime_lineage.py',ROOT/'cpppj/pathcleanup-runtime-lineage.json'])
    data={'schema':1,'host':'HJOW-Athlon','date':'2026-10-13','unit':'20261013-05','new_normal_cases':sum(MODULES.values()),
          'modules':MODULES,'focused_tests':focused,'all_tests':total,'prior_unit_new_normal_cases':5482,
          'preparatory_runtime_apis_no_new_observation':['Helper','ClearHelper'],
          'files':{path.relative_to(ROOT).as_posix():digest(path) for path in paths},
          'limitations':['GameWorld·GUI·실제 캠페인 완주 근거가 아니다.',
                         'victim damage/fieryTail 비행 효과/충돌 시각·사운드 하위 몸체는 아직 명시 경계다.',
                         '실제 Factory/공간/Kernel 수명 검사는 effectType0의 원본 자연 분기다. 타입11 구현 완료를 증명하지 않는다.',
                         'Missile 이전148행은 현재296행에 포함된다. 이전 관찰과 반복 실행을 신규 입력으로 다시 더하지 않는다.']}
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'후속 세 몸체 새{sum(MODULES.values())}개·전체{total}검사 근거 생성')

def verify():
    """이 작업 단위의 최종 파일 바이트와 실제 전체 검사 요약을 다시 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in data['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'후속 단위 SHA 불일치: {name}')
    if test_count(ROOT/'extracted/run5-all-final.log')!=data['all_tests']:raise RuntimeError('전체 검사 수 오류')
    print('투사체·직접 피해 시각·Ascend 최종 SHA/검사 감사 통과')

def main():
    """기본 실행은 근거 생성이며 --verify는 저장 근거 감사만 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()

if __name__=='__main__':
    main()
