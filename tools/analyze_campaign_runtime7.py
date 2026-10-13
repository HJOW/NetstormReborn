#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""경로 정리·초기 도장·사제 의식 동작·썬 디스크 투사체의 최종 실행 근거를 묶는다."""
import argparse
import json
import re
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count
from analyze_campaign_runtime6 import read_log, audit_files

# 이전 입력/반복 재생성은 합산하지 않으며 정상과 assert 입력을 분리한다.
REPORT = ROOT / 'cpppj/recovery-campaign-runtime7-evidence.json'
MODULES = {'pathcleanup': (1716, 16), 'terrainstamp': (1560, 0),
           'priestritualactions': (1752, 0), 'archermissile': (1888, 0)}
# substring 필터는 일부 Archer 검사를 다시 포함하므로 실제 검사 이름 집합으로 중복을 제거한다.
FILTERS = {'rawPathCleanup': 2, 'TerrainStamp_': 3, 'rawPriestRitualActions': 2, 'ArcherMissile_': 2,
           'Missile_PatchNativeCtorAndFrames': 2, 'Missile_CdNativeCtorAndFrames': 2,
           'Missile_ActualFactorySpaceKernelAndDelete': 1, 'Missile_MissingRealFlightEffectRejected': 1}
# 현재 소스 교정/실제 소비자/기존 근거 보존을 증명하는 별도 연결 보고서다.
LINEAGES = ('terrainstamp-enum-comments', 'terrainstamp-mask-correction', 'archermissile-correction')


def generate():
    """최종 정상 원본 입력·현재 출처·빌드와 검사 성공에서 이번 단위 근거를 만든다."""
    paths = [Path(__file__).resolve(), ROOT / 'tools/analyze_campaign_runtime4.py',
             ROOT / 'tools/analyze_campaign_runtime6.py']
    counts = {}
    focused = {}
    names = set()
    # 각 모듈의 실제 고유 입력/정상과 assert 및 현재 모든 출처 바이트를 확인한다.
    for name, (normal, assertions) in MODULES.items():
        path = ROOT / f'cpppj/recovery-{name}-evidence.json'
        data = json.loads(path.read_text(encoding='utf-8'))
        if data.get('host') != 'HJOW-Athlon' or data['total'] != normal + assertions:
            raise RuntimeError(f'원본 PC/행 수 오류: {name}')
        if data.get('normal_total', data['total']) != normal or data.get('assertion_total', 0) != assertions:
            raise RuntimeError(f'정상/assert 구분 오류: {name}')
        audit_files(data)
        fixture = ROOT / f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        if len(rows) != normal + assertions:
            raise RuntimeError(f'fixture 행 수 오류: {name}')
        counts[name] = {'normal_cases': normal, 'assertion_cases': assertions, 'fixture_rows': len(rows)}
        paths.extend([path, fixture])
    # 교정된 구현과 보존된 원본 입력의 연결도 최종 스냅샷에 고정한다.
    for name in LINEAGES:
        path = ROOT / f'cpppj/recovery-{name}-evidence.json'
        data = json.loads(path.read_text(encoding='utf-8'))
        # 주석 교정의 당시 cpp SHA는 mask_successor가 보존하므로 전용 감사기의 과거/현재 연결을 사용한다.
        if 'files' in data and name != 'terrainstamp-enum-comments':
            audit_files(data)
        paths.append(path)
    paths.extend([ROOT / 'tools/analyze_pathcleanup_test_lineage.py',
                  ROOT / 'tools/analyze_priestritualactions_test_lineage.py',
                  ROOT / 'tools/analyze_terrainstamp_enum_comments.py',
                  ROOT / 'tools/analyze_terrainstamp_mask_correction.py',
                  ROOT / 'tools/analyze_terrainstamp_cpp_binding.py',
                  ROOT / 'tools/analyze_archermissile_correction.py'])
    # 각 성공 로그를 확인한 뒤 등록 검사 이름의 집합으로 반복된 substring 실행을 제거한다.
    for name, expected in FILTERS.items():
        path = ROOT / f'extracted/campaign-runtime7-{name}-final-tests.log'
        actual = test_count(path)
        if actual != expected:
            raise RuntimeError(f'집중 검사 수 오류: {name}')
        names.update(re.findall(r'\[ OK \] ([^\r\n]+)', read_log(path)))
        focused[name] = actual
        paths.append(path)
    if len(names) != 13:
        raise RuntimeError('집중 검사 고유 이름 수 오류')
    whole = ROOT / 'extracted/campaign-runtime7-all-final-tests.log'
    total = test_count(whole)
    if total != 983:
        raise RuntimeError('최종 전체 검사 수 오류')
    build = ROOT / 'extracted/campaign-runtime7-build-final.log'
    content = read_log(build)
    if re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I):
        raise RuntimeError('최종 빌드 경고/오류 존재')
    if 'netstorm_tests.exe' not in content or 'NetstormCpp.exe' not in content:
        raise RuntimeError('두 Release 타깃 빌드 요약 없음')
    paths.extend([whole, build, ROOT / 'cpppj/recovery-campaign-runtime6-evidence.json',
                  ROOT / 'cpppj/src/o/CMakeLists.txt', ROOT / 'cpppj/tests/CMakeLists.txt'])
    # 신규 구현·기존 Missile 변경 및 실제 소비자 검사 소스를 현재 바이너리와 함께 고정한다.
    for name in ('RawPathCleanup', 'RawTerrainStamp', 'RawSquidRegion', 'RawPriestRitualActions', 'RawMissile'):
        paths.extend([ROOT / f'cpppj/src/o/{name}.cpp', ROOT / f'cpppj/src/o/{name}.h'])
    for name in ('PathCleanupTests', 'TerrainStampTests', 'PriestRitualActionsTests', 'ArcherMissileTests', 'MissileTests'):
        paths.append(ROOT / f'cpppj/tests/{name}.cpp')
    for name in ('netstorm_tests.exe', 'NetstormCpp.exe'):
        paths.append(ROOT / f'cpppj/build/bin/Release/{name}')
    data = {'schema': 1, 'host': 'HJOW-Athlon', 'date': '2026-10-13', 'unit': '20261013-07',
            'new_normal_cases': 6916, 'new_assertion_cases': 16, 'modules': counts,
            'focused_logs_tests_including_repeats': focused, 'focused_unique_tests': sorted(names),
            'focused_unique_count': len(names), 'all_tests': total, 'build_warnings': 0,
            'prior_sun_cannon_native_regression_not_added': 296,
            'archer_same_input_rerun_new_cases': 0,
            'archer_before_full_report_snapshot_available': False,
            'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
            'limitations': ['GameWorld·GUI·실제 캠페인 완주 완료 근거가 아니다.',
                            '전체 신전 의식/Archer 각도 애니메이션·실제 일반 표적 피해·미션 승패 상위 사슬은 별도 미완료다.',
                            'Archer 최초 전체 보고서 snapshot 부재는 교정 보고서에 명시했고 root 사전 관찰 SHA/제한 역변환으로 연결했다.',
                            '지형 factory/가상 owner/Pop 경계와 후속 context/군집 배치의 실제 월드 연결은 별도다.']}
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'후속 정상6916/assert16·고유집중{len(names)}·전체{total}검사 근거 생성')


def verify():
    """저장한 최종 파일 바이트와 실제 전체 검사 성공 요약을 감사한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    if test_count(ROOT / 'extracted/campaign-runtime7-all-final-tests.log') != data['all_tests']:
        raise RuntimeError('최종 전체 검사 수 오류')
    print('경로 정리·초기 도장·사제 의식 동작·투사체 최종 SHA/검사 감사 통과')


def main():
    """기본 실행은 최종 근거 생성이며 --verify는 저장 스냅샷의 현재 바이트를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()


if __name__ == '__main__':
    main()
