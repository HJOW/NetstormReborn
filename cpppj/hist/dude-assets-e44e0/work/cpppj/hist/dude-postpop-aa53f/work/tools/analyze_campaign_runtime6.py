#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""가이저 채집·사제 피격과 후보 선택 공유의 원본/C++ 실행 근거를 고정한다."""
import argparse
import json
import re
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest, test_count

# 이번 단위의 정상 입력·assert 입력·이전 회귀는 서로 구분해 기록한다.
REPORT = ROOT / 'cpppj/run6.json'
MODULES = {'geysermined': (2304, 8), 'priestdamage': (3424, 0)}
# 최종 집중 로그에 실제 등록된 검사 수다.
FILTERS = {'GeyserMined_': 2, 'PriestDamage_': 4, 'RegionPaint_': 2}


def read_log(path):
    """PowerShell UTF-16 또는 UTF-8 로그를 원본 바이트 변경 없이 읽는다."""
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def audit_files(data):
    """보고서에 포함된 모든 파일의 현재 SHA가 저장된 출처와 같은지 확인한다."""
    # 이전 관찰 출처와 현재 구현/검사의 연결을 파일마다 직접 확인한다.
    for filename, expected in data['files'].items():
        if digest(ROOT / filename) != expected:
            raise RuntimeError(f'단위 SHA 불일치: {filename}')


def generate():
    """완료한 native 관찰과 최종 Release 검사 로그에서 이번 단위 근거를 생성한다."""
    paths = [Path(__file__).resolve(), ROOT / 'tools/analyze_campaign_runtime4.py']
    counts = {}
    focused = {}
    # 두 판본의 고유 입력만 세며 x87 반복·이전 입력 재사용은 다시 더하지 않는다.
    for name, (normal, assertions) in MODULES.items():
        report = ROOT / f'cpppj/{name}.json'
        data = json.loads(report.read_text(encoding='utf-8'))
        if data['total'] != normal + assertions or data.get('host') != 'HJOW-Athlon':
            raise RuntimeError(f'원본 입력/PC 오류: {name}')
        if data.get('normal_total', data['total']) != normal or data.get('assertion_total', 0) != assertions:
            raise RuntimeError(f'정상/assert 구분 오류: {name}')
        audit_files(data)
        fixture = ROOT / f'cpppj/tests/fixtures/{name}-x86.tsv'
        rows = [line for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        if len(rows) != normal + assertions:
            raise RuntimeError(f'fixture 행 수 오류: {name}')
        counts[name] = {'normal_cases': normal, 'assertion_cases': assertions, 'fixture_rows': len(rows)}
        paths.extend([report, fixture])
    # 공유 후보 선택의484개 이전 근거와 나머지 전체 소스 보존을 별도 lineage로 연결한다.
    lineage = ROOT / 'cpppj/framechoose-lineage.json'
    data = json.loads(lineage.read_text(encoding='utf-8'))
    if data['prior_unique_cases'] != 484 or data['new_unique_cases'] != 0 or not data['remaining_header_and_implementation_identical']:
        raise RuntimeError('후보 선택 공유 lineage 오류')
    audit_files(data)
    paths.extend([lineage, ROOT / 'tools/analyze_framechoose_lineage.py', ROOT / 'tools/analyze_geysermined_shadow_lineage.py'])
    # 초기 실패/교정 로그는 보존하며 최종 성공 로그의 검사 수만 완료로 기록한다.
    for name, expected in FILTERS.items():
        path = ROOT / f'extracted/run6-{name}-final.log'
        count = test_count(path)
        if count != expected:
            raise RuntimeError(f'집중 검사 수 오류: {name}')
        focused[name] = count
        paths.append(path)
    whole = ROOT / 'extracted/run6-all-final.log'
    total = test_count(whole)
    if total != 974:
        raise RuntimeError('최종 전체 검사 수 오류')
    build = ROOT / 'extracted/run6-build-final.log'
    content = read_log(build)
    if re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:', content, re.I):
        raise RuntimeError('최종 빌드 경고/오류 존재')
    if 'netstorm_tests.exe' not in content or 'NetstormCpp.exe' not in content:
        raise RuntimeError('두 Release 타깃 빌드 요약 없음')
    paths.extend([whole, build, ROOT / 'cpppj/run5.json'])
    # 이 실행에 등록된 구현·검사·바이너리도 같은 스냅샷에 고정한다.
    for name in ('RawFrameChoose', 'RawRegionPaint', 'RawGeyserMined', 'RawPriestDamage', 'RawDamageableBurning'):
        paths.extend([ROOT / f'cpppj/src/o/{name}.cpp', ROOT / f'cpppj/src/o/{name}.h'])
    for name in ('GeyserMinedTests', 'PriestDamageTests', 'RegionPaintTests'):
        paths.append(ROOT / f'cpppj/tests/{name}.cpp')
    for name in ('netstorm_tests.exe', 'NetstormCpp.exe'):
        paths.append(ROOT / f'cpppj/build/bin/Release/{name}')
    paths.extend([ROOT / 'cpppj/src/o/CMakeLists.txt', ROOT / 'cpppj/tests/CMakeLists.txt'])
    result = {'schema': 1, 'host': 'HJOW-Athlon', 'date': '2026-10-13', 'unit': '20261013-06',
              'new_normal_cases': 5728, 'new_assertion_cases': 8, 'modules': counts,
              'prior_regionpaint_regression_cases_not_added': 484, 'shared_choose_new_cases': 0,
              'focused_tests': focused, 'all_tests': total, 'build_warnings': 0,
              'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in paths},
              'limitations': ['GameWorld·GUI·실제 캠페인 완주 근거가 아니다.',
                              'nonzero burning 생성/애니메이션과 일반 표적 피해는 별도 미완료다.',
                              '가이저 상위 CarrierArrival·채굴 통지 작성/송신·실제 음향 재생은 미연결이다.',
                              '이전 지형 후보484개는 기존 몸체 공유의 회귀이며 신규 입력으로 더하지 않는다.']}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'채집·사제 피해 정상5728/assert8·기존 후보 회귀484·전체{total}검사 근거 생성')


def verify():
    """저장된 최종 단위의 파일 SHA와 실제 검사 성공 요약을 다시 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_files(data)
    if test_count(ROOT / 'extracted/run6-all-final.log') != data['all_tests']:
        raise RuntimeError('전체 검사 수 오류')
    print('채집·사제 피해·후보 선택 공유 최종 SHA/검사 감사 통과')


def main():
    """기본 실행은 근거 생성이며 --verify는 저장 근거의 현재 바이트만 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()


if __name__ == '__main__':
    main()
