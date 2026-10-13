#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 성장의 완료 native 출력에 전체 배열 C++ 대조/공유 context 소비자 출처를 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_terraingrowth_oracle import ROOT, REPORT, digest

# native 고유 입력을 다시 실행하거나 늘리지 않고 C++ 검사 소스만 추가 연결한다.
CPP_FILES = ('cpppj/tests/TerrainGrowthTests.cpp', 'cpppj/src/o/RawTerrainContext.h',
             'cpppj/src/o/TerrainBuilder.h', 'cpppj/src/o/TerrainBuilder.cpp',
             'cpppj/tests/RawSceneSupport.h', 'tools/analyze_terraingrowth_cpp_binding.py')


def audit_native(data):
    """완료한108개 native 입력의 전체 기존 출처 바이트를 보존한다."""
    if data['total'] != 108 or data['normal_total'] != 108 or data['smoke']:
        raise RuntimeError('일반 성장 native 전수 완료 범위 오류')
    # 원본 PE/도구/fixture/구현을 당시 전체 파일 바이트와 비교한다.
    for name, expected in data['files'].items():
        if digest(ROOT/name) != expected:
            raise RuntimeError(f'일반 성장 기존 출처 SHA 차이: {name}')


def main():
    """기본 실행은 검사 출처 추가, --verify는 저장한 native/C++ 출처의 현재 바이트 감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    audit_native(data)
    if args.verify:
        if data.get('cpp_binding_new_native_cases') != 0 or data.get('cpp_source_files') != list(CPP_FILES):
            raise RuntimeError('일반 성장 C++ 연결 범위/새 원본 계수 오류')
        print('일반 성장108개 native/전체 배열 검사/공유 context 출처 감사 통과')
        return
    data.setdefault('native_source_files', list(data['files']))
    # 동일 저장소 검사/기존 로컬 PRNG 출처를 native 보고서에 별도 목록으로 보관한다.
    for name in CPP_FILES:
        data['files'][name] = digest(ROOT/name)
    data['cpp_source_files'] = list(CPP_FILES)
    data['cpp_binding_new_native_cases'] = 0
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('일반 성장108개 native 보존·C++ 검사 출처 추가 완료')


if __name__ == '__main__':
    main()
