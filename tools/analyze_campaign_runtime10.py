#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""충돌 효과·기본방어·실제 일반 지형/섬 연결의 단위10 최종 실행 근거를 기록한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

REPORT = ROOT / 'cpppj/run10.json'  # 이번 단위의 실제 최종 바이트/실행 결과다.
MODULES = {'gunimpact': 2274, 'squiddefense': 48}  # 새 정상 원본 입력이며 재사용 지형/Count를 더하지 않는다.
FILTERS = {'GunImpact_': 4, 'SquidDefense_': 4, 'NormalTerrainWorldBinding_': 3, 'VortexIslandBinding_': 2,
           'VortexAscend_': 3, 'carrier_predestroy_': 4, 'TerrainStamp_': 3, 'TerrainContext_': 2,
           'TerrainWorldBinding_': 5, 'TerrainGrowth_': 2, 'FarmCluster_': 3, 'IslandRuntime_': 3}  # 이substring은Normal3개도 포함한다. 고유집중은35개다.
AUDITS = {'decomp_gunimpact_oracle': ['--verify'], 'decomp_squiddefense_oracle': ['--verify'],
          'analyze_gunimpact_correction': ['--verify'], 'analyze_normalterrain_world_binding': ['--verify'],
          'analyze_normalterrain_binding_cpp_correction': [], 'check_terrain_profile_fix': [],
          'check_terrain_frame_fix': [],
          'analyze_dais_contained_lineage': ['--verify'], 'analyze_vortex_live_islands_lineage': ['--verify']}  # 기본감사/명시감사 계약을 따른다.
LINEAGES = ('gunimpact-correction', 'normalterrain-world-binding', 'normalterrain-binding-cpp-correction',
            'normalterrain-binding-profile-correction', 'normalterrain-binding-frame-correction',
            'dais-contained-lineage', 'vortex-live-islands-lineage')  # 역사적 SHA는 전용 역변환으로 연결한다.


def generate():
    """모든 전용감사와 실제 최종 성공 로그를 통과한 바이트만 근거에 저장한다."""
    paths = [Path(__file__).resolve(), ROOT / 'tools/analyze_campaign_runtime4.py', ROOT / 'tools/analyze_campaign_runtime6.py']
    modules, focused, names = {}, {}, set()
    # 각 판본의 실제 정상 반환과 고유fixture 행 수를 함께 확인한다.
    for name, expected in MODULES.items():
        report = ROOT / f'cpppj/{name}.json'
        data = json.loads(report.read_text(encoding='utf-8'))
        audit_files(data)
        assert data['host'] == 'HJOW-Athlon' and data['total'] == expected
        assert sum(item['returns'] for item in data['editions'].values()) == expected
        assert sum(item.get('assertions', 0) for item in data['editions'].values()) == 0
        fixture = ROOT / f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == expected
        modules[name] = dict(normal_cases=expected, assertion_cases=0, fixture_rows=len(rows))
        paths.extend([report, fixture])
    # 기존 전체 본문의 역변환과 현재SHA를 각 전용도구에서 감사한다.
    for name, arguments in AUDITS.items():
        tool = ROOT / f'tools/{name}.py'
        subprocess.run([sys.executable, str(tool), *arguments], cwd=ROOT, check=True)
        paths.append(tool)
    # 역사적 보고서를 보존하며 별도 대응표 전체 바이트를 고정한다.
    for name in LINEAGES:
        paths.append(ROOT / f'cpppj/{name}.json')
    # 빈필터와 최초실패 로그를 제외하고 실제 성공한 고유검사 이름을 기록한다.
    for name, expected in FILTERS.items():
        log = ROOT / f'extracted/run10-{name}final.log'
        assert test_count(log) == expected, name
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)', read_log(log)))
        focused[name] = expected
        paths.append(log)
    assert len(names) == 35
    whole = ROOT / 'extracted/run10-all-final.log'
    assert test_count(whole) == 1023
    build = ROOT / 'extracted/run10-build-final.log'
    content = read_log(build)
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I)
    assert 'netstorm_tests.exe' in content and 'Netstorm.exe' in content
    paths.extend([whole, build, ROOT / 'cpppj/run9.json',
                  ROOT / 'cpppj/src/o/CMakeLists.txt', ROOT / 'cpppj/src/client/CMakeLists.txt', ROOT / 'cpppj/tests/CMakeLists.txt'])
    # 새구현과 공유API의 전체소스를 이번 Release에 연결한다.
    for name in ('RawGunImpact', 'RawSquidDefense', 'RawCarrierPreDestroy', 'RawVortexAscend'):
        paths.extend([ROOT / f'cpppj/src/o/{name}.h', ROOT / f'cpppj/src/o/{name}.cpp'])
    paths.extend([ROOT / 'cpppj/src/client/RawNormalTerrainWorldBinding.h', ROOT / 'cpppj/src/client/RawNormalTerrainWorldBinding.cpp'])
    # 실제입력/소비자 검사도 고정하며 이전fixture는 새고유관찰에 합산하지 않는다.
    for name in ('GunImpactTests', 'SquidDefenseTests', 'NormalTerrainWorldBindingTests', 'VortexIslandBindingTests'):
        paths.append(ROOT / f'cpppj/tests/{name}.cpp')
    # 이번에 성공한 검사/게임 실행파일의 전체바이트를 연결한다.
    for name in ('netstorm_tests.exe', 'Netstorm.exe'):
        paths.append(ROOT / f'cpppj/build/bin/Release/{name}')
    data = dict(schema=1, host='HJOW-Athlon', last_decompile_host='HJOW-Athlon', last_decompile_date='2026-10-14',
                date='2026-10-14', unit='20261014-02', new_normal_cases=sum(MODULES.values()), new_assertion_cases=0,
                modules=modules, new_cpp_tests=13, focused_logs_tests=focused, focused_unique_tests=sorted(names),
                focused_unique_count=len(names), all_tests=1023, build_warnings=0, build_errors=0, game_runs=0,
                reused_vortex_count_cases_not_added=40, new_world_binding_native_cases=0, new_shared_api_native_cases=0,
                same_input_correction_new_cases=0, full_gameworld_completed=False, campaign_completed=False,
                files={path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
                limitations=['실제GameWorld 설치/GUI/캠페인 완주 동일성은 미완료다.',
                            'DudeNoticeAround의 실제 하위몸체와 파생방어는 명시적 외부경계다.',
                            '일반지형 소비자는 같은pool/record를 공유하며 전체 저장부모/군도는 미복원이다.',
                            'Dais/Rune/Altar진행 및 ArcherType은 다음단위 자료이며 이번 관찰에 합산하지 않는다.'])
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'단위10 정상{sum(MODULES.values())}/assert0·집중35·전체1023 최종근거 생성')


def verify():
    """기록한 최종SHA와 전체 성공검사 수를 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    assert test_count(ROOT / 'extracted/run10-all-final.log') == data['all_tests']
    print('단위10 최종SHA/전체검사 감사 통과')


# 기본실행은 실제 최종근거 생성이며 --verify는 기록한 현재바이트 감사다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    verify() if arguments.verify else generate()
