#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 섬 검색·일반 피해·edgeFarm 군집·초기 성장의 최종 실행 근거를 고정한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

# 최종 정상/원본 assertion 입력과 전체 fixture를 분리한다.
REPORT = ROOT/'cpppj/run9.json'
MODULES = {'islandqueries': (8490, 0), 'damageablehit': (5846, 0),
           'farmcluster': (205, 3), 'terraingrowth': (108, 0)}
# 새12개와 공유 API의 기존13개 검사 이름을 구분하여 실제 성공 로그를 기록한다.
FILTERS = {'IslandQueries_': 3, 'DamageableHit_': 4, 'FarmCluster_': 3, 'TerrainGrowth_': 2,
           'rawRunePostPop_': 2, 'rawPathRuntime_': 4, 'rawPathProcess_': 3,
           'rawPathStart_': 2, 'rawPathCleanup_': 2}
# 기본 실행이 감사인 Query 교정 도구와 --verify 도구의 계약을 따로 보관한다.
AUDITS = {'analyze_islandqueries_pool_correction': [], 'analyze_islandqueries_macro_correction': [],
          'analyze_damageablehit_correction': ['--verify'], 'analyze_farmcluster_cpp_correction': [],
          'analyze_terraingrowth_cpp_binding': ['--verify'], 'analyze_dais_shared_lineage': ['--verify']}
# 당시 보고서/전체 source를 보존한 연결 보고서만 이번 최종 스냅샷에 연결한다.
LINEAGES = ('islandqueries-pool-correction', 'islandqueries-macro-correction',
            'damageablehit-correction', 'farmcluster-cpp-correction', 'dais-shared-lineage')


def generate():
    """현재 모듈 출처·전용 역변환 감사와 최종 성공 로그에서 이번 단위를 만든다."""
    paths = [Path(__file__).resolve(), ROOT/'tools/analyze_campaign_runtime4.py',
             ROOT/'tools/analyze_campaign_runtime6.py']
    modules = {}
    focused = {}
    names = set()
    # 원본 고유 입력 수를 판본별로 확인하며 두x87 반복 반환은 합산하지 않는다.
    for name, (normal, assertions) in MODULES.items():
        path = ROOT/f'cpppj/{name}.json'
        data = json.loads(path.read_text(encoding='utf-8'))
        if data.get('host') != 'HJOW-Athlon' or data['total'] != normal+assertions:
            raise RuntimeError(f'원본 PC/고유 입력 수 오류: {name}')
        audit_files(data)
        if name == 'damageablehit':
            actual = sum(item['returns'] for item in data['editions'].values())
            if actual != normal or any(item['assertions'] for item in data['editions'].values()):
                raise RuntimeError('일반 피해 정상 완료 범위 오류')
        else:
            actual_normal = sum(item['normal_cases'] for item in data['editions'].values())
            actual_assertions = sum(item.get('assert_cases', 0) for item in data['editions'].values())
            if (actual_normal, actual_assertions) != (normal, assertions):
                raise RuntimeError(f'정상/assert 구분 오류: {name}')
        fixture = ROOT/f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        if len(rows) != normal+assertions:
            raise RuntimeError(f'fixture 행 수 오류: {name}')
        modules[name] = {'normal_cases': normal, 'assertion_cases': assertions, 'fixture_rows': len(rows)}
        paths.extend([path, fixture])
    # 각 전용 감사기에서 현재 파일 SHA와 전체 이전/현재 소스의 역변환을 함께 확인한다.
    for name, arguments in AUDITS.items():
        path = ROOT/f'tools/{name}.py'
        subprocess.run([sys.executable, str(path), *arguments], cwd=ROOT, check=True)
        paths.append(path)
    # lineage는 역사적 원본 SHA를 유지하므로 전용 감사 결과와 보고서 바이트를 고정한다.
    for name in LINEAGES:
        paths.append(ROOT/f'cpppj/{name}.json')
    # 빈 필터/초기 실패를 제외하고 최종 성공 로그의 실제 고유 이름만 기록한다.
    for name, expected in FILTERS.items():
        path = ROOT/f'extracted/run9-{name}final.log'
        actual = test_count(path)
        if actual != expected:
            raise RuntimeError(f'집중 검사 수 오류: {name}')
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)', read_log(path)))
        focused[name] = actual
        paths.append(path)
    if len(names) != 25:
        raise RuntimeError('집중 검사 고유 이름 수 오류')
    whole = ROOT/'extracted/run9-all-final.log'
    total = test_count(whole)
    if total != 1010:
        raise RuntimeError('최종 전체 검사 수 오류')
    build = ROOT/'extracted/run9-build-final.log'
    content = read_log(build)
    if re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I):
        raise RuntimeError('최종 빌드 경고/오류 존재')
    if 'netstorm_tests.exe' not in content or 'NetstormCpp.exe' not in content:
        raise RuntimeError('두 Release 타깃 빌드 요약 없음')
    paths.extend([whole, build, ROOT/'cpppj/run8.json',
                  ROOT/'cpppj/src/o/CMakeLists.txt', ROOT/'cpppj/tests/CMakeLists.txt'])
    # 새 모듈과 처음 등록한 기존Place·공유 API의 전체 소스를 현재 바이너리에 연결한다.
    for name in ('RawIslandQueries', 'RawDamageableHit', 'RawFarmCluster', 'RawTerrainGrowth',
                 'RawFortRandomPlacement', 'RawRunePostPop', 'RawPathRuntime'):
        paths.extend([ROOT/f'cpppj/src/o/{name}.cpp', ROOT/f'cpppj/src/o/{name}.h'])
    # 새 입력 준비/실제 소비자 코드도 최종 바이트로 보관한다.
    for name in ('IslandQueriesTests', 'DamageableHitTests', 'FarmClusterTests', 'TerrainGrowthTests'):
        paths.append(ROOT/f'cpppj/tests/{name}.cpp')
    # 검사와 실제 게임 실행 파일이 이번 성공 빌드의 결과인지 SHA로 고정한다.
    for name in ('netstorm_tests.exe', 'NetstormCpp.exe'):
        paths.append(ROOT/f'cpppj/build/bin/Release/{name}')
    data = {'schema': 1, 'host': 'HJOW-Athlon', 'date': '2026-10-14', 'unit': '20261014-01',
            'new_normal_cases': sum(value[0] for value in MODULES.values()),
            'new_assertion_cases': sum(value[1] for value in MODULES.values()), 'modules': modules,
            'focused_logs_tests': focused, 'focused_unique_tests': sorted(names),
            'focused_unique_count': len(names), 'new_cpp_tests': 12, 'all_tests': total,
            'build_warnings': 0, 'prior_rune_native_not_added': 1920, 'new_shared_api_native_cases': 0,
            'new_existing_placement_native_cases': 0, 'same_input_rerun_new_cases': 0, 'game_runs': 0,
            'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
            'limitations': ['실제 GameWorld·GUI·캠페인 완주 완료 근거가 아니다.',
                            '지형 성장의 전체 배열 대조는 일반 분기이며 archipelago/전체wrapper는 후속이다.',
                            'Farm의 실제단일배치와 외부 legalDrop/공간·owner/할당·진단 효과 경계를 구분한다.',
                            'Dais 공유API 준비만 포함하며 희생 진행/Rune304/305/306/307 전체는 후속이다.',
                            '과거 경로 ignored 내보내기12파일 부재를 별도lineage에 명시하며 현재SHA감사성공으로 세지 않는다.']}
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(f"단위09 정상{data['new_normal_cases']}/assert{data['new_assertion_cases']}·전체{total} 최종 근거 생성")


def verify():
    """최종 당시 스냅샷의 전체 SHA와 실제 전체 검사 성공 요약을 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    if test_count(ROOT/'extracted/run9-all-final.log') != data['all_tests']:
        raise RuntimeError('최종 전체 검사 수 오류')
    print('단위09 최종 SHA/전체 검사 감사 통과')


def main():
    """기본 실행은 최종 근거 생성이고 --verify는 저장한 성공 스냅샷의 현재 바이트 감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()


if __name__ == '__main__':
    main()
