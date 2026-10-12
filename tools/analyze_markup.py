#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Markup 그림 조각의 두 실제 PE·가상 표·배율 상수·내보낸 명령을 정적 분석 자료로 보관한다.

사용법: python -X utf8 tools/analyze_markup.py
게임/에뮬레이터/테스트를 실행하지 않는다. Ghidra markup 내보내기가 먼저 필요하다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 근거 경로는 저장소 안에서만 사용한다. 보호 원본은 읽기 전용이다.
PLAN=ROOT/'tools/ghidra/markup-functions.json'
REPORT=ROOT/'cpppj/recovery-markup-evidence.json'
# 판본별 그림/기본 가상 표, 비교 배율 1 및 16.16 배율 상수의 실제 주소다.
EDITIONS={
    'originals':('originals/Netstorm.exe',0x50c5d8,0x50c5c0,0x5012d0,0x501ad0),
    'originalCD':('originalCD/NETSTORM.EXE',0x501f88,0x501f68,0x501f80,0x501f84),
}


def table(image,base,address):
    """가상 표의 scalar 삭제/기준선/측정/출력/복제 함수 다섯 주소를 읽는다."""
    return [f'{value:08x}' for value in struct.unpack_from('<5I',image,address-base)]


def constant(image,base,address):
    """ROM의 실제 float32 비트와 값을 기록한다. 기대값을 생성하지 않는다."""
    raw=image[address-base:address-base+4]
    return {'address':f'{address:08x}','bits':raw.hex(),'value':struct.unpack('<f',raw)[0]}


def main():
    """두 판본의 실제 명령/가상 표와 현재 구현 해시를 정적 복원 기록으로 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 각 판본의 다른 PE와 정밀 내보내기를 독립적으로 읽는다.
    for edition,(binary,picture_vtable,base_vtable,unit_scale,fixed_scale) in EDITIONS.items():
        binary_path=ROOT/binary
        pe=pefile.PE(str(binary_path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/markup'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        assembly=[]
        # 내보낸 함수의 실제 주소/바이트/명령을 저장한다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra가 분리한 범위 밖의 데이터/함수를 이어 붙이지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 실제 명령을 판독하며 실행하거나 예상 그림/측정값을 만들지 않는다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        facts[edition]={
            'binary':binary,'exported_functions':[entry for entry,_ in functions],
            'requested_functions':plan['editions'][edition],
            'picture_vtable':f'{picture_vtable:08x}','picture_virtual_slots':table(image,base,picture_vtable),
            'base_vtable':f'{base_vtable:08x}','base_virtual_slots':table(image,base,base_vtable),
            'unit_scale':constant(image,base,unit_scale),'fixed_scale':constant(image,base,fixed_scale),
        }
        files.extend([binary_path,exported,ranges_path,assembly_path])
    # 파생 그림과 공통 조각/SHP 헤더의 현재 구현 상태를 같이 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/Markup.cpp','cpppj/src/client/Markup.h',
        'cpppj/src/client/StyleText.cpp','cpppj/src/client/StyleText.h',
        'cpppj/src/client/VFXDraw.cpp','cpppj/src/client/VFXDraw.h','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE의 정적 디컴파일·가상 표·기계어·배율 상수 판독; 원본/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본 동작 대조·GUI 검사는 후속',
        'boundaries':['확대/축소 VFX 픽셀 커널','원본 그림 변환 색 표 공급','대괄호 옵션/그림 파서','클라이언트 자동 UI 연결'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Markup 정적 분석 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
