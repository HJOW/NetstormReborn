#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""독립 초기stamp 원본 보고서를 변경하지 않은 실행기 출처에 묶고 현재 C++/공유Choose 출처를 추가한다."""
import argparse
import hashlib
import json
from pathlib import Path

# 원본 보고서와 초기stamp의 현재 C++/공유 후보선택 의존성을 명시한다.
ROOT=Path(__file__).resolve().parent.parent
REPORT=ROOT/'cpppj/recovery-terrainstamp-evidence.json'
SOURCES=('cpppj/src/o/RawTerrainStamp.h','cpppj/src/o/RawTerrainStamp.cpp','cpppj/tests/TerrainStampTests.cpp',
    'cpppj/tests/RawSceneSupport.h','cpppj/src/o/RawFrameChoose.h','cpppj/src/o/RawFrameChoose.cpp',
    'cpppj/src/o/RawSquidRegion.h','cpppj/src/o/RawSquidRegion.cpp',
    'cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp','cpppj/src/o/RiftType.h','cpppj/src/o/RiftType.cpp',
    'cpppj/src/o/TerrainBuilder.h','cpppj/src/o/TerrainBuilder.cpp','cpppj/recovery-framechoose-lineage-evidence.json',
    'tools/analyze_framechoose_lineage.py','tools/analyze_terrainstamp_cpp_binding.py')


def sha(path):
    """현재 파일의 정확한 UTF-8/바이너리 바이트 SHA를 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bind():
    """기존 native 도구/PE/범위/fixture의 SHA를 검증한 뒤 별도 C++ 출처를 연결한다. 새 native 관찰은0이다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    original=report.get('native_source_files',report['files'])
    for name,value in original.items():
        if sha(ROOT/name)!=value:raise RuntimeError('초기stamp 원본 관찰 출처 변경 '+name)
    report['native_source_files']=dict(original)
    report['cpp_source_files']={name:sha(ROOT/name) for name in SOURCES}
    report['files']={**original,**report['cpp_source_files']}
    report['cpp_binding']={
        'native_cases':report['total'],'added_native_cases':0,
        'scope':'두 판본 FrameAt/Stamp와 공유 ChooseTerrainFrame/CRT99cache를 같은 pool/두 난수에서 비교한다.',
        'shared_choose_lineage':'4e532fd의 RawRegionPaint::Choose 원래 토큰과484개 관찰을 recovery-framechoose-lineage-evidence.json에서 연결한다.',
        'external_cpp_boundaries':'factory/가상 owner/Pop/region setter/region127 chunk 조회/assert는 필수 hooks다. 검사에서는 명시 원본 인자/슬롯 효과를 관찰하며 월드 production 결합 완료로 세지 않는다.',
        'allocator_scope':'native의 실제 allocation/free는 external 계수에 기록한다. C++ std배열/후보vector의 호스트 allocator 호출순서는 게임 효과 대조 항목이 아니다.'}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('초기stamp 기존 native 출처 SHA와 현재 C++/공유Choose lineage 연결 완료. 신규 native0.')


def verify():
    """native/현재C++ 출처를 모두 확인하고 공유Choose lineage 출처가 포함됐는지 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('초기stamp 연결 출처 SHA 불일치 '+name)
    if set(report['files'])!=set(report['native_source_files'])|set(report['cpp_source_files']) or report['cpp_binding']['added_native_cases']:
        raise RuntimeError('초기stamp 출처 분리/계수 연결 오류')
    if set(report['cpp_source_files'])!=set(SOURCES):raise RuntimeError('초기stamp 현재 C++ 의존성 누락')
    print('초기stamp native/현재C++·공유Choose lineage SHA 연결 감사 통과')


# 명시한 출처 연결 또는 현재 기록 감사만 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:bind()
