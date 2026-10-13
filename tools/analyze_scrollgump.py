#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Slider/손잡이의 실제 가상 표·정밀 함수/명령과 C++ 연결 범위를 정적 근거로 저장한다.

사용법: python -X utf8 tools/analyze_scrollgump.py
scrollgump Ghidra 내보내기를 먼저 수행한다. 게임·에뮬레이터·테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 보호 원본은 읽기만 하며 계획/보고서/명령 자료는 프로젝트 안에서 생성한다.
PLAN=ROOT/'tools/ghidra/scrollgump-functions.json'
REPORT=ROOT/'cpppj/scrollgump.json'
# 실제 두 PE의 Slider/손잡이 가상 표 주소다. 생성/픽셀 Draw는 참고 내보내기이며 이번 구현 범위와 구분한다.
EDITIONS={
    'originals':('originals/Netstorm.exe',{'slider':0x5116f8,'thumb':0x511668}),
    'originalCD':('originalCD/NETSTORM.EXE',{'slider':0x500c78,'thumb':0x500be8}),
}


def main():
    """실제 PE/함수 범위/가상 슬롯을 기록하고 현재 공유 저장소·TextGump 연결 구현의 해시를 보관한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 두 판본의 같은 PC Ghidra 내보내기를 독립적으로 읽는다.
    for edition,(binary,tables) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/scrollgump'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        if [entry for entry,_ in functions]!=plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 함수 내보내기 주소/순서가 계획과 다름')
        assembly=[]
        # Ghidra가 판독한 실제 함수 범위만 디코드한다. 명령/호스트 입력을 실행하지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 범위 사이의 다른 코드나 데이터를 합치지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 원본 x86 바이트/명령/피연산자를 주소별로 보관한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        virtuals={}
        # 부모/손잡이의 실제 12슬롯에서 Push/Attach/Draw/입력/프레임 통지의 주소를 확인한다.
        for name,address in tables.items():
            slots=struct.unpack_from('<12I',image,address-base)
            virtuals[name]={'address':f'{address:08x}','slots':[f'{slot:08x}' for slot in slots],
                'push':f'{slots[6]:08x}','attach':f'{slots[8]:08x}','draw':f'{slots[9]:08x}',
                'input':f'{slots[10]:08x}','broadcast':f'{slots[11]:08x}'}
        facts[edition]={
            'binary':binary,'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],'virtual_tables':virtuals,
            'implemented_functions':plan['editions'][edition][:8],
            'reference_only_functions':plan['editions'][edition][8:],
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 실제 저장소/범위/부모 통지/캡처/트리 부착/TextGump 연결과 빌드 목록을 함께 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/ScrollGump.h','cpppj/src/client/ScrollGump.cpp',
        'cpppj/src/client/TextGump.h','cpppj/src/client/TextGump.cpp',
        'cpppj/src/client/GumpTree.h','cpppj/src/client/GumpTree.cpp',
        'cpppj/src/client/GumpCreate.h','cpppj/src/client/GumpCreate.cpp',
        'cpppj/src/client/GumpEvents.h','cpppj/src/client/GumpEvents.cpp',
        'cpppj/src/client/GumpInput.h','cpppj/src/client/GumpInput.cpp',
        'cpppj/src/client/GumpFrame.h','cpppj/src/client/GumpFrame.cpp',
        'cpppj/src/client/GumpCollection.h','cpppj/src/client/GumpCollection.cpp','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 읽기 전용 정적 내보내기·가상 표/기계어 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 PE/C++ 동작·GUI/게임 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'values':'signed 범위 정렬/동률 +1·page 상한만 제한·위치 제한·범위 변경에서 위치 두 번 적용·DWORD 감김',
            'callbacks':'값 저장 후 현재 void·Push 뒤 현재 x/y→Attach 또는 Redraw(false)·다음 적용의 현재 position',
            'attach':'기본 Attach→현재 세 자식 Push→화살표/트랙 크기/부착→양 끝 포함 손잡이 rect·절삭 두께/저장 전 배율',
            'geometry':'page/position×가용 길이÷범위; position-minimum 보정 없음·CD 두 번째 화살표 두께의 float32 좁힘',
            'notifications':'click/repeat→step 2/1 부모 통지→미소비 시 step 증감→change; 손잡이 step 전달·항상 0 반환',
            'thumb':'포함 rect/캡처·page 방향 4/3·release 통지 플래그 해제·poll release/전달 y·세로 DWORD 곱/절삭·NaN 세로 분기',
            'editions':'패치 가로 캡처/이전 x 저장·CD 가로 무동작; 두 판본 세로만 위치 변환·min 보정 없음',
            'binding':'같은 트리/이벤트/직접 캡처/프레임에 분배·TextGump 네 slider 경계를 실제 처리기로 연결',
        },
        'boundaries':['Slider/손잡이/화살표 전체 생성·파생 FrameGump/화살표/손잡이 실제 픽셀 Draw',
            '다중 줄 TextGump 전체 생성/소멸·가상 Attach/Input·W 명령/전체 스타일 파서',
            '자동 raw Client UI·메뉴/Form·건설/미션 완주',
            '극단 x87 80비트 중간값·CD Pentium FDIV 보정 경로·독립 PE/C++ 동작 대조'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('ScrollGump 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
