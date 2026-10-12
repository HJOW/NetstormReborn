#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 PE의 VFX 변환 커널·보조 함수·각도 0 상수를 정적 복원 근거로 보관한다.

사용법: python -X utf8 tools/analyze_vfxscale.py
Ghidra vfxscale 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 원본은 읽기만 하며 생성 자료와 분석 계획은 저장소 안에 둔다.
PLAN=ROOT/'tools/ghidra/vfxscale-functions.json'
REPORT=ROOT/'cpppj/recovery-vfxscale-evidence.json'
# 판본별 실제 실행 파일과 각도 0의 cos/sin DWORD 주소다.
EDITIONS={
    'originals':('originals/Netstorm.exe',0x40572a,0x40653a),
    'originalCD':('originalCD/NETSTORM.EXE',0x46910a,0x469f1a),
}


def constant(image,base,address):
    """ROM의 실제 signed DWORD 값과 바이트를 읽는다. 기대 출력은 만들지 않는다."""
    raw=image[address-base:address-base+4]
    return {'address':f'{address:08x}','bytes':raw.hex(),'value':struct.unpack('<i',raw)[0]}


def main():
    """실제 함수 범위/명령과 현재 C++ 연결 해시를 실행 없는 분석 보고서로 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 서로 다른 두 원본 PE와 각 정밀 프로젝트 내보내기를 독립적으로 읽는다.
    for edition,(binary,cosine,sine) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/vfxscale'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        assembly=[]
        details=[]
        # 실제 PE 바이트에서 함수마다 호출/반환과 전체 명령을 기록한다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            calls=[]
            returns=[]
            byte_count=0
            # Ghidra의 분리된 몸체 사이에 있는 다른 코드/데이터는 포함하지 않는다.
            for low,high in ranges:
                byte_count+=high-low
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 기계어는 판독만 하며 커널/원본 게임을 실행하지 않는다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
                    if instruction.mnemonic=='call':
                        calls.append({'at':f'{instruction.address:08x}','operand':instruction.op_str})
                    if instruction.mnemonic.startswith('ret'):
                        returns.append({'at':f'{instruction.address:08x}','operand':instruction.op_str})
            details.append({'entry':entry,'body_bytes':byte_count,'calls':calls,'returns':returns})
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        facts[edition]={
            'binary':binary,'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],'functions':details,
            'angle_zero_cosine':constant(image,base,cosine),'angle_zero_sine':constant(image,base,sine),
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 직교 커널과 동일 표면/클립/그림 캐시를 사용하는 현재 연결 파일을 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/VFXTransform.cpp','cpppj/src/client/VFXTransform.h',
        'cpppj/src/client/GumpDraw.cpp','cpppj/src/client/GumpDraw.h',
        'cpppj/src/client/Markup.cpp','cpppj/src/client/Markup.h',
        'cpppj/src/client/VFXDraw.cpp','cpppj/src/client/VFXDraw.h',
        'cpppj/src/client/Renderer.cpp','cpppj/src/client/Renderer.h','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 정적 디컴파일·범위·명령·ROM 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본 픽셀 대조·GUI 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'angle':0,'scratch_reuse':False,'scale_format':'signed 16.16',
            'source_coordinates':'signed WORD, width/height <= 32768',
            'destination_coordinates':'signed WORD, x/y span <= 32767',
            'sampling':'포함 끝점·IDIV 절삭 기울기·16.16 반 픽셀 시작·DWORD 누산',
            'transparency':'일반 분기 opacity 유지; 비단위 배율은 색 치환 후 255도 건너뜀',
        },
        'boundaries':['회전 다각형 커널','임시 표면 재사용 flag 2','고정소수점 범위 밖 원본 버퍼 접근',
            '원본 그림 색 표 공급','대괄호 그림/옵션 파서','클라이언트 자동 raw UI 연결'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('VFX 배율 정적 분석 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
