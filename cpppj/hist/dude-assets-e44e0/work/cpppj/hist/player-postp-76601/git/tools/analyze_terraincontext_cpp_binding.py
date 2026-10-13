#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기context 독립 원본 출처를 보존하며 현재 C++ 검사·공유 지도 선언을 연결한다."""
import argparse
import json
from pathlib import Path
from analyze_campaign_runtime4 import ROOT, digest

# 현재224개 입력을 다시 실행하지 않고 C++ 비교 대상과 초기 도장 지도 선언을 고정한다.
REPORT = ROOT / 'cpppj/terraincontext.json'
SOURCES = ('cpppj/tests/TerrainContextTests.cpp', 'cpppj/src/o/RawTerrainStamp.h',
           'cpppj/tests/RawSceneSupport.h', 'tools/analyze_terraincontext_cpp_binding.py')


def bind():
    """현재 native 입력/출처를 직접 감사한 뒤 별도 cpp source 표를 추가한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    original = data.get('native_source_files', data['files'])
    # 원본 PE·실행기·범위·fixture·당시 구현은 이 연결에서 변경되면 안 된다.
    for name, expected in original.items():
        if digest(ROOT/name) != expected:
            raise RuntimeError('초기context native 출처 변경 '+name)
    data['native_source_files'] = dict(original)
    data['cpp_source_files'] = {name: digest(ROOT/name) for name in SOURCES}
    data['files'] = {**original, **data['cpp_source_files']}
    data['cpp_binding_new_native_cases'] = 0
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    verify()


def verify():
    """보존한 native와 현재 cpp/지도 선언의 모든 파일 바이트를 검사한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    if data['cpp_binding_new_native_cases'] or set(data['cpp_source_files']) != set(SOURCES):
        raise RuntimeError('초기context cpp 연결/계수 오류')
    # 고정된 원본과 현재 검사 출처를 각각 모든 파일에서 확인한다.
    for name, expected in data['files'].items():
        if digest(ROOT/name) != expected:
            raise RuntimeError('초기context 연결 SHA 불일치 '+name)
    print('초기context224개 native/현재C++·초기stamp 지도 선언 출처 연결 감사 통과')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else bind()
