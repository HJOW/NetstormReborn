#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Player·사제 도착·Burning 소스·픽셀 캡처의 단위14 최종 근거를 기록한다."""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

REPORT = ROOT / 'cpppj/run14.json'  # 실제 최종 소스/바이너리/성공 로그의 출처다.
MODULES = {
    'burningsources': (652, 0, 0, 0),
    'shape-to-buffer': (164, 0, 0, 0),
    'dude-space': (36, 0, 0, 12),
    'geyser-ai-registration': (168, 0, 436, 0),
    'priesthalt': (864, 0, 0, 0),
    'carrierarrival': (406, 0, 0, 0),
    'player-world': (1452, 228, 0, 0),
}  # 고유 정상/assert/미복원 하위 요청/동일입력 반복을 서로 구분한다.
FILTERS = {
    'BurningSources_': 2, 'ShapeToBuffer_': 2, 'DudeSpace_': 2, 'GeyserAiRegistration_': 3,
    'rawPriestHalt_': 1, 'rawCarrierArrival_': 1, 'rawPriestArrival_': 1, 'PlayerWorld_': 3,
    'DudeSpawn_': 2, 'DudeDrop_': 2, 'RawGraph_': 14, 'SquidPostPop_': 6,
    'DamageablePostDestroy_': 3, 'RawPriestWorldBindings_': 7, 'rawPathStart_': 2,
    'rawPathRuntime_': 4, 'rawPathCleanup_': 2, 'CarrierCheck_': 5, 'BurningVisual_': 3,
    'BurningParticle_': 2, 'ContentLifetime_': 4, 'ProcessForm_': 4, 'ProcessRetake_': 5,
}  # 새15와 기존 의존 몸체의 실제 substring 선택 수를 검산한다.
AUDITS = (
    'decomp_burningsources_oracle', 'decomp_shape_to_buffer_oracle',
    'analyze_dude_space_player_successor', 'analyze_geyser_ai_test_width_lineage',
    'analyze_priesthalt_spots_successor', 'decomp_carrierarrival_oracle',
    'check_arrival_cleanup_ln', 'analyze_genus_at_public_lineage',
    'decomp_player_world_oracle', 'analyze_player_world_attach_lineage',
    'analyze_player_postpop_binding_lineage', 'analyze_dude_assets_inline_lineage',
)  # 원본 전수와 현재 공유/소비자 연결의 전용 감사 계약이다.
LINEAGES = ('dude-space-player-successor', 'dude-space-predicate-scope', 'priesthalt-cpp-binding', 'priesthalt-spots-successor',
            'carrierarrival-cpp-binding', 'priest-arrival-name-lineage', 'priest-arrival-test-syntax-lineage', 'priest-arrival-factory-input-lineage', 'priest-arrival-cleanup-binding-lineage', 'genus-at-public-lineage', 'player-world-attach-lineage',
            'player-postpop-binding-lineage', 'dude-assets-inline-lineage', 'geyser-ai-test-width-lineage')  # 초기 전체 원문과 같은입력 보강/좁은 API 연결을 보존한다.


def generate():
    """현재 전수 자료·전체 이전 원문·성공한 실제 로그를 한 상태로 고정한다."""
    paths = [Path(__file__), ROOT / 'tools/analyze_campaign_runtime4.py',
             ROOT / 'tools/analyze_campaign_runtime6.py', ROOT / '.gitattributes']
    modules, focused, names = {}, {}, set()
    # 각 부모의 고유 입력은 한 번만 세고 정밀도 반복/재실행/기존 하위 호출은 더하지 않는다.
    for name, (normal, assertions, unsupported, repeats) in MODULES.items():
        report = ROOT / f'cpppj/{name}.json'
        data = json.loads(report.read_text(encoding='utf-8'))
        fixture = ROOT / f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert data['host'] == 'HJOW-Athlon' and len(rows) == normal + assertions + unsupported + repeats, name
        assert data['total'] == len(rows), name
        if name == 'geyser-ai-registration':
            assert data['normal_total'] == normal and data['unrestored_boundary_total'] == unsupported and data['assert_total'] == 0
        if name == 'player-world':
            assert data['normal_total'] == normal and data['assert_total'] == assertions
        modules[name] = dict(normal_unique=normal, assertion_unique=assertions,
                             unsupported_lower_requests=unsupported, repeated_inputs=repeats, fixture_rows=len(rows))
        paths.extend((report, fixture))
    # 기존 보고서의 SHA를 수정하지 않고 당시 전체 바이트를 현재 좁은 연결에 대조한다.
    for name in AUDITS:
        tool = ROOT / f'tools/{name}.py'
        # 두 기존 CLI는 기본 실행이 감사이며 나머지는 명시 --verify를 사용한다.
        arguments = () if name in ('analyze_player_world_attach_lineage', 'analyze_geyser_ai_test_width_lineage') else ('--verify',)
        subprocess.run([sys.executable, str(tool), *arguments], cwd=ROOT, check=True)
        paths.append(tool)
    for name in LINEAGES:
        paths.append(ROOT / f'cpppj/{name}.json')
    # 각각 실행한 집중 로그의 실제 성공 수와 고유 검사명을 확인한다.
    for name, expected in FILTERS.items():
        log = ROOT / f'extracted/run14-{name}final.log'
        assert test_count(log) == expected, name
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)', read_log(log)))
        focused[name] = expected
        paths.append(log)
    assert len(names) == sum(FILTERS.values())
    whole = ROOT / 'extracted/run14-all-final.log'
    assert test_count(whole) == 1081
    build = ROOT / 'extracted/run14-build-final.log'
    content = read_log(build)
    assert not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I)
    assert 'netstorm_tests.exe' in content and 'Netstorm.exe' in content
    paths.extend((whole, build, ROOT / 'cpppj/run13.json'))
    # 공유 분배/재사용 API 및 등록한 현재 실제 모듈의 전체 원문을 고정한다.
    for name in ('SquidPostPop', 'RawDamageablePostDestroy', 'RawBurningSources',
                 'RawGeyserAiRegistration', 'RawPlayerWorld', 'RawPriestHalt',
                 'RawCarrierArrival', 'RawPriestArrival'):
        for suffix in ('.h', '.cpp'):
            paths.append(ROOT / f'cpppj/src/o/{name}{suffix}')
    for suffix in ('.h', '.cpp'):
        paths.append(ROOT / f'cpppj/src/client/RawShapeToBuffer{suffix}')
    for name in ('BurningSourcesTests', 'ShapeToBufferTests', 'DudeSpaceTests',
                 'GeyserAiRegistrationTests', 'PriestHaltTests', 'CarrierArrivalTests',
                 'PriestPathArrivalHostTests', 'PlayerWorldTests'):
        paths.append(ROOT / f'cpppj/tests/{name}.cpp')
    for name in ('DudeSpaceSupport', 'CarrierArrivalSupport', 'DudeSpawnSupport'):
        paths.append(ROOT / f'cpppj/tests/{name}.h')
    for name in ('src/o', 'src/client', 'tests'):
        paths.append(ROOT / f'cpppj/{name}/CMakeLists.txt')
    for name in ('netstorm_tests.exe', 'Netstorm.exe'):
        paths.append(ROOT / f'cpppj/build/bin/Release/{name}')
    data = dict(schema=1, host='HJOW-Athlon', last_decompile_host='HJOW-Athlon',
                last_decompile_date='2026-10-14', date='2026-10-14', unit='20261014-06',
                new_normal_unique=sum(value[0] for value in MODULES.values()),
                new_assertion_unique=sum(value[1] for value in MODULES.values()),
                unsupported_lower_requests=sum(value[2] for value in MODULES.values()),
                repeated_fixture_inputs=sum(value[3] for value in MODULES.values()),
                new_shared_api_native_inputs=0, new_cpp_tests=15, modules=modules,
                focused_tests=focused, focused_unique_names=sorted(names), focused_unique_count=len(names),
                all_tests=1081, build_warnings=0, build_errors=0, game_runs=0,
                full_campaign_install_completed=False, campaign_completed=False,
                files={path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
                limitations=[
                    'Player 생성/현재9기록/실제Regular/FormAttach이며 전체 캠페인 저장172 설치와 실제 GameWorld 연결은 후속이다.',
                    '사제 Host는 합성1×1/HP200/owner1/지면6이며 실제Path 보간/Halt/도착/cleanup의 선택 범위를 확인한다.',
                    'Carrier의 현재Path와 큰회피 하위 요청, 전체포획/중립귀환/의식의 독립 완료는 미포함이다.',
                    'ShapeToBuffer는 실제 보호Dude의 plain RLE 픽셀이며 모든가상출력/소유자색/전체BurningFrame 완료는 미포함이다.',
                    'Geyser AI nonnull 요청436은 미복원거부이며 정상168과 합치지 않는다. DudeSpace48행은 고유36/반복12다.',
                    '게임 실행/완주·시각/음향·경제/AI/생산/운반·승패/UI 동일성 전체는 미완료다.',
                ])
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위14 새정상3742/assert228·미복원436·반복12·새15·전체1081 근거 생성')


def verify():
    """현재 고정한 파일의 전체 SHA와 실제 전체 성공 수를 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    assert test_count(ROOT / 'extracted/run14-all-final.log') == data['all_tests']
    print('단위14 최종 전체 SHA/검사 수 감사 통과')


# 기본은 최종 성공 자료 생성이며 --verify는 이미 고정한 현재 바이트만 읽는다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()
