#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기 지형·현재 섬 목록·썬 디스크 애니메이션·신전/룬의 최종 실행 근거를 묶는다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

# 정상/assert/constructor 중단 접두를 서로 다른 완료 범위로 보관한다.
REPORT = ROOT / 'cpppj/recovery-campaign-runtime8-evidence.json'
MODULES = {'gunarcher': (1688, 0, 2), 'islandruntime': (1238, 780, 0),
           'terraincontext': (170, 54, 0), 'templeritual': (6720, 0, 0),
           'runepostpop': (1920, 0, 0)}
# 새15개와 기존18개를 함께 검사하되 실제 등록 이름 집합으로 중복을 제거한다.
FILTERS = {'ArcherAnimation_': 4, 'IslandRuntime_': 3, 'TerrainWorldBinding_': 2,
           'TerrainContext_': 2, 'rawTempleRitual_': 2, 'rawRunePostPop_': 2,
           'RawSurfaceWorld_': 6, 'ConstructionProcess_': 12}
# 기존 전체 소스·공유 HP 몸체·공통 PostPop 추가와 교정 전 관찰을 보존한 연결 보고서다.
LINEAGES = ('world-bridge-state-lineage', 'squidhitpoints-lineage',
            'runepostpop-lineage', 'gunarcher-correction', 'temple-rune-runtime8-lineage')
# 파일 SHA 외에 전체 이전 몸체/교정 역변환을 확인하는 전용 감사기다.
LINEAGE_TOOLS = ('analyze_world_bridge_state_lineage', 'analyze_squidhitpoints_lineage',
                 'analyze_runepostpop_lineage', 'analyze_gunarcher_correction',
                 'analyze_temple_rune_runtime8_lineage')


def native_counts(name, data):
    """판본별 고유 정상/assert/접두 수를 읽으며 반복 반환 횟수는 합산하지 않는다."""
    if name == 'gunarcher':
        return (sum(item['returns'] for item in data['editions'].values()), 0,
                sum(item['prefix'] for item in data['editions'].values()))
    if name == 'islandruntime':
        return (sum(item['normal_cases'] for item in data['editions'].values()),
                sum(item['assert_cases'] for item in data['editions'].values()), 0)
    if name == 'terraincontext':
        return (data['normal_total'], data['assertion_total'], 0)
    if any(item.get('asserts', 0) for item in data['editions'].values()):
        raise RuntimeError(f'정상 관찰에 assert 포함: {name}')
    return (sum(item['cases'] for item in data['editions'].values()), 0, 0)


def generate():
    """현재 원본 출처와 최종 성공 로그·Release 바이너리를 이번 단위 스냅샷에 고정한다."""
    paths = [Path(__file__).resolve(), ROOT / 'tools/analyze_campaign_runtime4.py',
             ROOT / 'tools/analyze_campaign_runtime6.py']
    modules = {}
    focused = {}
    names = set()
    # 모든 원본 입력의 현재 출처와 고유 fixture 행 수를 분리해 확인한다.
    for name, expected in MODULES.items():
        path = ROOT / f'cpppj/recovery-{name}-evidence.json'
        data = json.loads(path.read_text(encoding='utf-8'))
        if data.get('host') != 'HJOW-Athlon' or native_counts(name, data) != expected:
            raise RuntimeError(f'원본 PC/고유 입력 수 오류: {name}')
        audit_files(data)
        fixture = ROOT / f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines()
                if line and not line.startswith('#')]
        if len(rows) != data['total'] or len(rows) != sum(expected):
            raise RuntimeError(f'fixture 행 수 오류: {name}')
        modules[name] = {'normal_cases': expected[0], 'assertion_cases': expected[1],
                         'constructor_prefix_cases': expected[2], 'fixture_rows': len(rows)}
        paths.extend([path, fixture])
    # 이전 단위 출처는 당시의 바이트를 유지하고 별도 연결 보고서를 고정한다.
    for name in LINEAGES:
        path = ROOT / f'cpppj/recovery-{name}-evidence.json'
        lineage = json.loads(path.read_text(encoding='utf-8'))
        if 'files' in lineage:
            audit_files(lineage)
        paths.append(path)
    # 각 감사기에서 부분 SHA뿐 아니라 전체 과거/현재 소스 연결도 재확인한다.
    for name in LINEAGE_TOOLS:
        tool = ROOT / f'tools/{name}.py'
        subprocess.run([sys.executable, str(tool), '--verify'], cwd=ROOT, check=True)
        paths.append(tool)
    # 실제 성공 로그의 이름과 등록 수를 확인하며 빈 필터/첫 실패는 완료로 세지 않는다.
    for name, expected in FILTERS.items():
        path = ROOT / f'extracted/campaign-runtime8-{name}final-tests.log'
        actual = test_count(path)
        if actual != expected:
            raise RuntimeError(f'집중 검사 수 오류: {name}')
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)', read_log(path)))
        focused[name] = actual
        paths.append(path)
    if len(names) != 33:
        raise RuntimeError('집중 검사 고유 이름 수 오류')
    whole = ROOT / 'extracted/campaign-runtime8-all-final-tests.log'
    total = test_count(whole)
    if total != 998:
        raise RuntimeError('최종 전체 검사 수 오류')
    build = ROOT / 'extracted/campaign-runtime8-build-final.log'
    content = read_log(build)
    if re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I):
        raise RuntimeError('최종 빌드 경고/오류 존재')
    if 'netstorm_tests.exe' not in content or 'NetstormCpp.exe' not in content:
        raise RuntimeError('두 Release 타깃 빌드 요약 없음')
    paths.extend([whole, build, ROOT / 'cpppj/recovery-campaign-runtime7-evidence.json',
                  ROOT / 'cpppj/src/o/CMakeLists.txt', ROOT / 'cpppj/src/client/CMakeLists.txt',
                  ROOT / 'cpppj/tests/CMakeLists.txt'])
    # 이번 신규 모듈과 변경한 기존 공통 경로의 전체 소스를 실제 바이너리와 연결한다.
    for name in ('RawArcherAnimation', 'RawIslandRuntime', 'RawTerrainContext',
                 'RawSquidHitPoints', 'RawTempleRitual', 'RawRunePostPop',
                 'RawConstructionProcess', 'SquidPostPop'):
        paths.extend([ROOT / f'cpppj/src/o/{name}.cpp', ROOT / f'cpppj/src/o/{name}.h'])
    # 같은 저장소를 공유하는 월드 참조/소비자도 전체 바이트로 보관한다.
    for name in ('RawSurfaceWorld', 'RawTerrainWorldBinding'):
        paths.extend([ROOT / f'cpppj/src/client/{name}.cpp', ROOT / f'cpppj/src/client/{name}.h'])
    # 새 검사의 입력 준비와 실제 소비자 조립 소스를 현재 버전으로 고정한다.
    for name in ('ArcherAnimationTests', 'IslandRuntimeTests', 'TerrainContextTests',
                 'TerrainWorldBindingTests', 'TempleRitualTests', 'RunePostPopTests'):
        paths.append(ROOT / f'cpppj/tests/{name}.cpp')
    # 원본 관찰/연결 감사기 자체도 이번 결과를 재현하는 출처다.
    for name in ('analyze_terraincontext_cpp_binding',):
        paths.append(ROOT / f'tools/{name}.py')
    # 전체 실행이 실제 이번 빌드의 검사/게임 실행 파일과 연결되게 한다.
    for name in ('netstorm_tests.exe', 'NetstormCpp.exe'):
        paths.append(ROOT / f'cpppj/build/bin/Release/{name}')
    data = {'schema': 1, 'host': 'HJOW-Athlon', 'date': '2026-10-13', 'unit': '20261013-08',
            'new_normal_cases': sum(item[0] for item in MODULES.values()),
            'new_assertion_cases': sum(item[1] for item in MODULES.values()),
            'constructor_prefix_cases_separate': sum(item[2] for item in MODULES.values()),
            'modules': modules, 'focused_logs_tests': focused, 'focused_unique_tests': sorted(names),
            'focused_unique_count': len(names), 'new_cpp_tests': 15, 'all_tests': total,
            'build_warnings': 0, 'prior_construction_native_not_added': 2868,
            'prior_terrain_stamp_native_not_added': 1560, 'repeat_same_input_new_cases': 0,
            'game_runs': 0, 'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
            'limitations': ['실제 GameWorld·GUI·캠페인 완주 완료 근거가 아니다.',
                            'constructor 접두2개는 정상 함수 완료와 구분한다.',
                            '초기 성장/일반·archipelago wrapper/edgeFarm 군집 배치는 후속이다.',
                            '신전305/306/307·Rune304의 전체 희생 진행/사제 사망·미션 승패는 후속이다.',
                            '합성 타입/프레임/SHP 소비자는 실제 전체 게임 자산/화면 대조와 구분한다.']}
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f"단위08 정상{data['new_normal_cases']}/assert{data['new_assertion_cases']}/접두2·전체{total} 근거 생성")


def verify():
    """저장한 최종 스냅샷의 전체 출처 SHA와 실제 성공 검사 수를 감사한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    if test_count(ROOT / 'extracted/campaign-runtime8-all-final-tests.log') != data['all_tests']:
        raise RuntimeError('최종 전체 검사 수 오류')
    print('단위08 최종 SHA/전체 검사 감사 통과')


def main():
    """기본 실행은 근거 생성이며 --verify는 당시 최종 스냅샷의 현재 바이트를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()


if __name__ == '__main__':
    main()
