#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""네 캠페인 하위 몸체의 원본 관찰·집중 검사·전체 Release 검사 근거를 묶는다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

# 작업 루트와 이번 단위의 근거 파일이다. 후속 단위의 관찰을 여기에 합산하지 않는다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/run4.json'
# 원본 입력 수는 fixture의 행 수에서 읽으며 완료·접두·기존 회귀는 별도로 기록한다.
FIXTURES={'pathstart':512,'pathprocesstype':718,'gunanimation':2560,'assetregular':1976,'regionpaint':484}
# 각 집중 검사의 실제 실행 로그와 등록 검사 수다.
FILTERS={'rawPathStart':2,'GunAnimation_':4,'AssetRegular_':4,'RegionPaint_':2,
         'RawPriestWorldBindings_':7,'rawPathRuntime':4,'rawPathProcess':3}

def digest(path):
    """기록된 소스·원본 관찰·빌드 결과의 SHA-256을 구한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def test_count(path):
    """실제 검사 로그의 마지막 요약이 성공일 때 등록 검사 수를 반환한다."""
    # Windows PowerShell의 리디렉션 로그는 BOM 있는 UTF-16이다. 원본 로그 바이트를 보존해 읽기만 한다.
    raw=path.read_bytes();content=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
    summaries=re.findall(r'(\d+) test\(s\), (\d+) failed check\(s\)',content)
    if not summaries or summaries[-1][1]!='0' or '[FAIL]' in content:raise RuntimeError(f'검사 실패/요약 없음: {path}')
    return int(summaries[-1][0])

def generate():
    """이미 완료한 실행의 입력/로그/현재 파일 바이트를 감사하여 단위 근거를 생성한다."""
    paths=[Path(__file__).resolve()];fixture_counts={};focused={}
    # 중복 입력은 하위 전용 감사기가 검증하며 여기서는 판본을 포함한 전체 행 수를 고정한다.
    for name,expected in FIXTURES.items():
        path=ROOT/f'cpppj/tests/fixtures/{name}-x86.tsv';rows=[line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        if len(rows)!=expected:raise RuntimeError(f'fixture 행 수 오류: {name}')
        fixture_counts[name]=len(rows);paths.append(path)
    # 초기 실패 로그와 교정 과정은 별도 보존하며 최종 성공 로그만 완료 근거로 사용한다.
    for name,expected in FILTERS.items():
        path=ROOT/f'extracted/run4-{name}-final.log';count=test_count(path)
        if count!=expected:raise RuntimeError(f'집중 검사 수 오류: {name}')
        focused[name]=count;paths.append(path)
    whole=ROOT/'extracted/run4-all-final.log';total_tests=test_count(whole);paths.append(whole)
    # 현재 하위 보고서의 source/PE/export SHA도 직접 검사한다.
    for name in ('pathstart','gunanimation','assetregular','regionpaint'):
        path=ROOT/f'cpppj/{name}.json';data=json.loads(path.read_text(encoding='utf-8'))
        if data.get('host')!='HJOW-Athlon':raise RuntimeError(f'원본 관찰 PC 불일치: {name}')
        for source,expected in data['files'].items():
            if digest(ROOT/source)!=expected:raise RuntimeError(f'하위 SHA 불일치: {source}')
        paths.append(path)
    # 새 기능/검사 및 기존 몸체를 공유한 lineage의 최신 파일을 같은 결과에 고정한다.
    for name in ('pathruntime','movement-mapper'):
        paths.append(ROOT/f'cpppj/{name}-lineage.json')
    for name in ('RawPathStartTests','GunAnimationTests','AssetRegularTests','RegionPaintTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    for name in ('netstorm_tests.exe','Netstorm.exe'):
        paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    paths.append(ROOT/'extracted/run4-build-final.log')
    data={'schema':1,'host':'HJOW-Athlon','date':'2026-10-13','unit':'20261013-04',
          'new_normal_cases':5482,'missile_constructor_prefix_cases':50,'existing_selector_regression_cases':718,
          'existing_runtime_cases_not_added':360,'fixture_rows':fixture_counts,'focused_tests':focused,'all_tests':total_tests,
          'files':{path.relative_to(ROOT).as_posix():digest(path) for path in paths},
          'limitations':['이는 네 하위 모듈의 실행 근거다. GameWorld 연결·GUI·캠페인 완주 완료 근거가 아니다.',
                         '완료한 원본 입력은5482개다. Missile 접두50개와 기존 종류 선택 회귀718개는 신규 완료 입력으로 더하지 않는다.',
                         '원본/클론 게임 실행과 DirectDraw 전체화면 오류 수정은 이 단위에 포함하지 않는다.']}
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'네 모듈 근거 생성: 새5482·접두50·기존회귀718, 전체{total_tests}검사')

def verify():
    """이 작업 단위의 최종 바이트와 실제 실행 로그 요약이 현재 기록과 같은지 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in data['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'단위 SHA 불일치: {name}')
    if test_count(ROOT/'extracted/run4-all-final.log')!=data['all_tests']:raise RuntimeError('전체 검사 수 불일치')
    print('네 캠페인 하위 몸체 최종 SHA/검사 감사 통과')

def main():
    """--verify는 저장 근거만 검사하고 기본 실행은 성공 로그를 읽어 현재 근거를 생성한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()

if __name__=='__main__':
    main()
