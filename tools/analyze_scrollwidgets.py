#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Slider/화살표/손잡이 생성·실제 Draw·scalar 위임과 VFX 선의 정적 복원 근거를 저장한다.

사용법: python -X utf8 tools/analyze_scrollwidgets.py
scrollwidgets Ghidra 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 생성 자료는 프로젝트 안에 보관한다. 두 보호 바이너리는 읽기만 한다.
PLAN=ROOT/'tools/ghidra/scrollwidgets-functions.json'
REPORT=ROOT/'cpppj/recovery-scrollwidgets-evidence.json'
# 실제 세 파생 가상 표·signed 정렬 BYTE·기본 배경 타입/프레임 전역 주소다.
EDITIONS={
    'originals':('originals/Netstorm.exe',{'slider':0x5116f8,'arrow':0x5116a8,'thumb':0x511668},[0x540442,0x54041c,0x54044e],0x5ca920,0x542664),
    'originalCD':('originalCD/NETSTORM.EXE',{'slider':0x500c78,'arrow':0x500c28,'thumb':0x500be8},[0x535f44,0x535eac,0x535f74],0x53f608,0x53f614),
}
# CD scalar 삭제의 짧은 소멸 보조 몸체다. 반환/단일 tail jump의 실제 바이트만 추가로 기록한다.
CD_HELPERS=[(0x4188f9,8),(0x418951,8),(0x403ca1,8),(0x418849,8),(0x405d41,8),(0x4189b9,8)]


def main():
    """함수별 실제 명령/가상 표/정렬/초기 전역과 현재 생성·픽셀 출력·수명 연결의 해시를 보관한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 두 판본의 같은 PC 내보내기를 독립적으로 읽으며 현재 CPP 입력/픽셀은 실행하지 않는다.
    for edition,(binary,tables,alignment,type_address,frame_address) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/scrollwidgets'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        if [entry for entry,_ in functions]!=plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 내보낸 주소/순서가 계획과 다름')
        assembly=[]
        # Ghidra의 각 실제 범위만 디코드해 다른 코드/데이터를 이어 붙이지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 불연속 함수 몸체의 각 범위를 독립적으로 읽는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 실제 PE의 명령 바이트/피연산자/주소만 기록한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        helpers={}
        if edition=='originalCD':
            # 짧은 소멸 보조 몸체는 첫 tail jump/ret까지만 기록한다. 뒤의 정렬 바이트는 제외한다.
            for address,size in CD_HELPERS:
                lines=[]
                # 실제 지연 소멸 경로에서 기본 Gump 소멸자로 이어지는 지점만 판독한다.
                for instruction in decoder.disasm(image[address-base:address-base+size],address):
                    lines.append(f'{instruction.address:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}')
                    if instruction.mnemonic in ('jmp','ret'): break
                helpers[f'{address:08x}']=lines
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        virtuals={}
        # scalar 삭제/부착/Draw 슬롯을 실제 PE에서 읽어 기존 공통 처리기와 파생 분배를 구분한다.
        for name,address in tables.items():
            slots=struct.unpack_from('<11I',image,address-base)
            virtuals[name]={'address':f'{address:08x}','slots':[f'{slot:08x}' for slot in slots],
                'scalar_delete':f'{slots[0]:08x}','attach':f'{slots[8]:08x}','draw':f'{slots[9]:08x}'}
        facts[edition]={
            'binary':binary,'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],'virtual_tables':virtuals,
            'initial_alignment':dict(zip(['slider','arrow','thumb'],[struct.unpack_from('<b',image,address-base)[0] for address in alignment])),
            'initial_default_type':struct.unpack_from('<I',image,type_address-base)[0],
            'initial_default_frame':struct.unpack_from('<I',image,frame_address-base)[0],
            'scalar_helpers':helpers,
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 같은 등록/버튼/배경/값/클립/픽셀/수명 처리기와 현재 빌드 목록을 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/ScrollGumpWidgets.h','cpppj/src/client/ScrollGumpWidgets.cpp',
        'cpppj/src/client/ScrollGump.h','cpppj/src/client/ScrollGump.cpp',
        'cpppj/src/client/VFXLine.h','cpppj/src/client/VFXLine.cpp',
        'cpppj/src/client/GumpCreate.h','cpppj/src/client/GumpCreate.cpp',
        'cpppj/src/client/GumpDraw.h','cpppj/src/client/GumpDraw.cpp',
        'cpppj/src/client/GumpBackgroundCreate.h','cpppj/src/client/GumpBackgroundCreate.cpp',
        'cpppj/src/client/GumpBackgroundDraw.h','cpppj/src/client/GumpBackgroundDraw.cpp',
        'cpppj/src/client/StoneButtonGump.h','cpppj/src/client/StoneButtonGump.cpp',
        'cpppj/src/client/ButtonGump.h','cpppj/src/client/ButtonGump.cpp',
        'cpppj/src/client/GumpLifetime.h','cpppj/src/client/GumpLifetime.cpp','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 읽기 전용 정적 내보내기·가상 표/생성 상수/기계어 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 PE/C++ 생성/콜백/픽셀 대조·GUI/게임 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'create':'기본 등록→부분 파생 초기화/0 축 40/방향·step→178 두 화살표/판본별 d8/d4 손잡이→세 가상 부착→0..100/10/50 값',
            'arrow':'기존 빈 돌 버튼·명시 8슬롯 문맥·팝업 유지·signed 정렬/방향·실제 base Draw 뒤 3선/기본 색/절삭 중점',
            'thumb':'실제 기본 배경 저장소 상속·기본 전역 타입/프레임·rect 0/이전 좌표 미작성·기본 Draw 뒤 현재 rect/type/frame',
            'line':'mode 0 8비트·주축/방향·min/max 0.32 분수·반 픽셀 곱/시작-끝 역산 차이·원래 시작점 위상·ADD carry·포함 끝점',
            'lifetime':'세 파생 scalar는 기본 소멸/flags bit 0 반납으로 위임·기존 자식 우선 Destroy/Cleanup',
            'editions':'패치 생성 높이 float32 재조회/세로 폭 저장 전 절삭; CD 양 축 FILD 배율/세로 폭 float32 절삭·작은 손잡이 할당',
        },
        'boundaries':['독립 PE/C++ 생성/수명/콜백/선/화살표/손잡이 픽셀 대조',
            '다중 줄 TextGump 전체 생성/소멸·가상 Attach/Input·W 파서·메뉴/Form·자동 raw Client UI·건설/미션 완주',
            'VFX 선 색 치환/사용자 콜백 mode·signed 거리 INT_MAX 초과·DIV 몫 overflow·비정상 클립 수렴',
            '극단 x87 80비트 값·CD Pentium FDIV 보정 경로·생성자 예외 시 원본 EH 정리 전체 대조'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('ScrollGump 생성/표시 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
