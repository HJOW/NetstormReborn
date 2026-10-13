#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기stamp 이웃bit표의 C++ 교정만 제한하고 기존1560개 원본 행/출처 보존을 감사한다."""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

# 교정 전 body/report와 현재 source 연결을 보관한다. 기존 fixture/native 도구는 수정하지 않는다.
ROOT=Path(__file__).resolve().parent.parent
CPP='cpppj/src/o/RawTerrainStamp.cpp'
BEFORE_CPP=ROOT/'extracted/terrainstamp-before-mask-correction.cpp'
BEFORE_REPORT=ROOT/'extracted/terrainstamp-before-mask-correction-evidence.json'
CURRENT_REPORT=ROOT/'cpppj/terrainstamp.json'
REPORT=ROOT/'cpppj/terrainstamp-mask-correction.json'
BIT_TABLE=(1,2,2,4,4,8,8,1)


def sha(data):
    """원본/소스/관찰 파일의 정확한 바이트SHA를 계산한다."""
    return hashlib.sha256(data).hexdigest()


def expected(before):
    """전체 cpp에서 명시한 bit표 추가와 해당mask대입/설명 세 곳만 교정해 기대소스를 재구성한다."""
    replacements=(
        ('constexpr std::uint32_t kCrtMultiplier=0x343fdU,kCrtIncrement=0x269ec3U,kCrtMask=0x7fffU;',
         'constexpr std::uint32_t kCrtMultiplier=0x343fdU,kCrtIncrement=0x269ec3U,kCrtMask=0x7fffU;\n// 실제 양판본의 북부터 시계방향 bit표다. 대각선은 북서1/북동2/남동4/남서8이다.\nconstexpr std::array<int,8> kNeighborBits{1,2,2,4,4,8,8,1};'),
        ('// 표의 짝수 이웃은 직선, 홀수 이웃은 대각선이며 두 mask의 bit위치가 같다.',
         '// 짝수는 직선/홀수는 대각선이며 실제 이웃별 bit표로 두 mask를 각각 누적한다.'),
        ('(neighbor%2 ? diagonal : straight)|=1<<(neighbor/2);','(neighbor%2 ? diagonal : straight)|=kNeighborBits[neighbor];'))
    # 지정한 교정 위치가 전체 파일에 각각 한 곳뿐인지 확인한다.
    for old,new in replacements:
        if before.count(old)!=1:raise RuntimeError('초기stamp mask의 교정 전 위치 불일치')
        before=before.replace(old,new)
    return before


def native_tables():
    """보호 두PE의 실제8DWORD bit표를 읽는다. 새 명령 실행은0이다."""
    sys.path.insert(0,str(ROOT/'extracted/oracle-python'));import pefile
    tables={}
    for name,address in (('originals/Netstorm.exe',0x52f8dc),('originalCD/NETSTORM.EXE',0x51c3d0)):
        pe=pefile.PE(str(ROOT/name));offset=address-pe.OPTIONAL_HEADER.ImageBase
        values=tuple(struct.unpack('<8I',pe.get_memory_mapped_image()[offset:offset+32]))
        if values!=BIT_TABLE:raise RuntimeError('초기stamp 보호PE bit표 불일치 '+name)
        tables[name]=dict(address=f'{address:08x}',values=list(values),sha256=sha((ROOT/name).read_bytes()))
    return tables


def audit():
    """기존 전체source/행/PE SHA를 보존하고 bit표 교정만 있는 전체 현재cpp를 연결한다."""
    old=json.loads(BEFORE_REPORT.read_text(encoding='utf-8'));current=json.loads(CURRENT_REPORT.read_text(encoding='utf-8'));before=BEFORE_CPP.read_bytes();after=(ROOT/CPP).read_bytes()
    if sha(before)!=old['cpp_source_files'][CPP] or sha(after)!=current['cpp_source_files'][CPP] or expected(before.decode('utf-8')).encode('utf-8')!=after:raise RuntimeError('초기stamp cpp 교정/출처 연결 불일치')
    if old['native_source_files']!=current['native_source_files']:raise RuntimeError('초기stamp mask교정의 원본 출처 변경')
    preserved={name:value for name,value in old['files'].items() if name!=CPP}
    for name,value in preserved.items():
        if sha((ROOT/name).read_bytes())!=value:raise RuntimeError('초기stamp mask의 나머지출처 변경 '+name)
    result=dict(host='HJOW-Athlon',added_native_cases=0,total=1560,baseline_report_sha256=sha(BEFORE_REPORT.read_bytes()),
        baseline_cpp_utf8=before.decode('utf-8'),baseline_cpp_sha256=sha(before),current_cpp_sha256=sha(after),
        native_tables=native_tables(),native_source_files=current['native_source_files'],previous_cpp_source_files=old['cpp_source_files'],preserved_files=preserved,
        change='직선 N/E/S/W=1/2/4/8, 대각선 NW/NE/SE/SW=1/2/4/8의 실제표를 사용한다. 이전 C++의NE=1 해석만 교정하고 native1560개 행/실행기/PE/범위는 불변이다.',
        files={CPP:sha(after),'tools/analyze_terrainstamp_mask_correction.py':sha(Path(__file__).read_bytes())})
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('초기stamp 실제8bit표/제한된C++교정·native1560행출처 보존감사 통과. 새native0.')


def verify():
    """이전 cpp와 제한 교정으로 현재전체소스를 재구성하고 원본표/기록 출처를 재감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));current=json.loads(CURRENT_REPORT.read_text(encoding='utf-8'));before=report['baseline_cpp_utf8'];after=(ROOT/CPP).read_bytes()
    if sha(before.encode('utf-8'))!=report['baseline_cpp_sha256'] or expected(before).encode('utf-8')!=after or sha(after)!=report['current_cpp_sha256']:raise RuntimeError('초기stamp mask 전체source 교정 연결 오류')
    for name,value in {**report['files'],**report['preserved_files']}.items():
        if sha((ROOT/name).read_bytes())!=value:raise RuntimeError('초기stamp mask current SHA 불일치 '+name)
    if report['added_native_cases'] or report['native_source_files']!=current['native_source_files'] or current['cpp_source_files'][CPP]!=report['current_cpp_sha256'] or report['native_tables']!=native_tables():raise RuntimeError('초기stamp mask native/현재cpp 연결 오류')
    print('초기stamp mask 실제PE표/제한cpp교정·native1560행 불변감사 통과')


# 명시한 mask교정 lineage 생성 또는 현재출처 감사만 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:audit()
