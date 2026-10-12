#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메뉴 정상 생성/수명/부착과 장식 배경의 두 실제 PE·가상 표·정렬/여백/전역을 정적으로 기록한다.

사용법: python -X utf8 tools/analyze_menuwidgets.py
menuwidgets Ghidra 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 계획과 기록은 저장소 안에 생성한다. 두 보호 PE와 디컴파일 결과는 읽기만 한다.
PLAN=ROOT/'tools/ghidra/menuwidgets-functions.json'
REPORT=ROOT/'cpppj/recovery-menuwidgets-evidence.json'
# 두 실제 메뉴/장식 가상 표·signed 정렬·변경 영역 여백과 생성/스택 전역의 주소다.
EDITIONS={
    'originals':{
        'binary':'originals/Netstorm.exe','menu_vtable':0x50c6f0,'decoration_vtable':0x50ae7c,
        'menu_alignment':0x540435,'decoration_alignment':0x540454,'redraw_padding':0x502314,
        'globals':{'stack':0x592fd0,'index':0x593020,'instances':0x593024,'background_type':0x5ca920,
            'menu_background_frame':0x542664,'decoration_background_frame':0x5ca924,
            'corner_type':0x5411c8,'foreground':0x5a405c,'shadow':0x5c78d0},
    },
    'originalCD':{
        'binary':'originalCD/NETSTORM.EXE','menu_vtable':0x501fa0,'decoration_vtable':0x504928,
        'menu_alignment':0x535f10,'decoration_alignment':0x535f8c,'redraw_padding':0x5048a0,
        'globals':{'stack':0x51fc28,'index':0x51fc78,'instances':0x51fc7c,'background_type':0x53f608,
            'menu_background_frame':0x53f614,'decoration_background_frame':0x53f60c,
            'corner_type':0x51cab4,'foreground':0x55ba90,'shadow':0x55bad4},
    },
}


def main():
    """정상 몸체/보조 소멸/실제 명령과 같은 메뉴/제목/장식/기본 저장소 연결 해시를 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),PLAN,ROOT/'tools/analyze_styletext.py'];facts={}
    # 각 판본은 같은 PC 정밀 프로젝트에서 읽기 전용으로 내보낸 함수를 판독한다.
    for edition,source in EDITIONS.items():
        path=ROOT/source['binary'];pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/menuwidgets'/edition
        exported=folder/'creation.c';ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        if [entry for entry,_ in functions]!=plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 내보낸 함수 주소/순서가 계획과 다름')
        assembly=[]
        # 원본 결과를 실행하지 않고 실제 함수의 기계어 범위만 보관한다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 불연속 범위 사이의 다른 함수/데이터는 이어 붙이지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 실제 주소/바이트/명령/피연산자를 기록한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt';assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        menu=struct.unpack_from('<16I',image,source['menu_vtable']-base)
        decoration=struct.unpack_from('<15I',image,source['decoration_vtable']-base)
        facts[edition]={
            'binary':source['binary'],'functions':[entry for entry,_ in functions],
            'menu_vtable':{'address':f'{source["menu_vtable"]:08x}','slots':[f'{value:08x}' for value in menu],
                'scalar':f'{menu[0]:08x}','attach':f'{menu[8]:08x}','draw_pending':f'{menu[9]:08x}',
                'handle_pending':f'{menu[10]:08x}','broadcast_pending':f'{menu[11]:08x}'},
            'decoration_vtable':{'address':f'{source["decoration_vtable"]:08x}','slots':[f'{value:08x}' for value in decoration],
                'scalar':f'{decoration[0]:08x}','redraw':f'{decoration[2]:08x}','draw':f'{decoration[9]:08x}'},
            'menu_alignment':struct.unpack_from('<b',image,source['menu_alignment']-base)[0],
            'decoration_alignment':struct.unpack_from('<b',image,source['decoration_alignment']-base)[0],
            'decoration_redraw_padding':struct.unpack_from('<f',image,source['redraw_padding']-base)[0],
            'globals':{name:f'{value:08x}' for name,value in source['globals'].items()},
            'runtime_values':'타입/프레임/팔레트 번호는 실제 초기화 전역을 공급하며 PE 미초기화 값을 대체값으로 쓰지 않음',
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 기존 제목/메뉴 크기·글꼴/StyleText·트리/등록/수명과 새 실제 픽셀 분배를 함께 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/GumpDecoration.h','cpppj/src/client/GumpDecoration.cpp',
        'cpppj/src/client/MenuGumpWidgets.h','cpppj/src/client/MenuGumpWidgets.cpp',
        'cpppj/src/client/MenuGump.h','cpppj/src/client/MenuGump.cpp',
        'cpppj/src/client/MenuGumpLayout.h','cpppj/src/client/MenuGumpLayout.cpp',
        'cpppj/src/client/ScrollingTextGumpWidgets.h','cpppj/src/client/ScrollingTextGumpWidgets.cpp',
        'cpppj/src/client/TextGump.h','cpppj/src/client/TextGump.cpp',
        'cpppj/src/client/StyleText.h','cpppj/src/client/StyleText.cpp',
        'cpppj/src/client/GumpBackgroundCreate.h','cpppj/src/client/GumpBackgroundCreate.cpp',
        'cpppj/src/client/GumpBackgroundDraw.h','cpppj/src/client/GumpBackgroundDraw.cpp',
        'cpppj/src/client/GumpTree.h','cpppj/src/client/GumpTree.cpp',
        'cpppj/src/client/GumpLifetime.h','cpppj/src/client/GumpLifetime.cpp',
        'cpppj/src/client/GumpCollection.h','cpppj/src/client/GumpCollection.cpp',
        'cpppj/src/client/CMakeLists.txt','extracted/menuwidgets-build.log',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 읽기 전용 정적 내보내기·가상 표/기계어/정렬/여백 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본/C++ 생성/수명/부착/모서리/변경 영역/픽셀 대조·게임/GUI 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'constructor':'원본 varargs를 문자열/자식·flags/command/parameter/condition 값 목록으로 표현·부분 필드 초기화·조건 검사 전 명령/인자 쓰기·이전 항목 자식 연결·최종 flags/Fancy/Reflow/notify',
            'fancy':'0xc4 실제 장식 배경(1,1)/부착/secondaryDepth 1→0xd8 실제 다중 줄 제목/자식 버퍼·Slider/overlay/16,16/secondaryDepth -1',
            'title_layout':'기존 제목 Reflow의 RequestScroll(0)/FitContent(0,0)를 실제 TextGump에 연결',
            'controlled':'index 증가/빈 칸→0xb90 할당/생성→현재 칸 저장→가로/세로 idle·세로 배경 flags OR; 장식은 idle 생략·콜백 뒤 현재 index 재조회',
            'attach':'root sticky 4000·parent 0 재사용·plain/fancy 전체 화면 상한/장식 0 하한·DWORD 로컬 차·기존 기본 Attach; 패치 float32 요청 저장/CD 미저장과 fancy Y 합 순서',
            'lifetime':'파생 표/개수 감소→현재 선택 소멸 통지→40개 전체 항목의 StyleText 역순 Clear→기본 scalar·기존 자식 우선 Destroy',
            'decoration':'현재 배경/type/I 첫 프레임/SHP 캐시·실제 배경→Position→TL/TR/BR/BL 현재 필드 원색 픽셀·보이는 부모 확인→x-1/y-1 및 폭/높이+4 변경 영역·기본 scalar',
            'binding':'같은 메뉴/장식/제목/배경·트리/Draw/Redraw/수명 분배와 실제 GameAssets/SHP/GumpPixelTarget 사용',
        },
        'boundaries':['독립 PE/C++ 생성/항목/조건/카운터/콜백/부착/클립/영역/수명/픽셀 대조',
            '메뉴 실제 Draw/Handle/Broadcast/AttachObject·제목/머리 포맷·controlled 항목 래퍼·전체 Form 객체',
            '그림 색 표/회전 VFX/flag 2 재사용·OS 큐/폴링/overlay·자동 raw Client UI·건설/미션 완주',
            '현재 실제 사건/타입/프레임/팔레트 전역·allocator/변경 영역 공급·생성 EH 정리/극단 x87·원본 varargs 포인터 alias'],
        'editions':facts,'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('메뉴 생성/장식 정적 기록 완료:',', '.join(f'{edition} {len(value["functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
