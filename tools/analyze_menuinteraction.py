#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 메뉴 출력/입력/선택/객체 연결 근거를 실행 없이 기록한다.

사용법: menuinteraction Ghidra 내보내기와 Release 빌드 후
python -X utf8 tools/analyze_menuinteraction.py
게임/GUI/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT, digest, read_ranges

# 계획/결과는 저장소에 기록하고 보호 PE와 같은 PC의 기존 내보내기는 읽기만 한다.
PLAN = ROOT / 'tools/ghidra/menuinteraction-functions.json'
REPORT = ROOT / 'cpppj/menuinteraction.json'
# 실제 가상 표·현재 사건/색/타입/등록표/객체 Form 전역과 식별 BYTE의 두 판본 주소다.
EDITIONS = {
    'originals': {
        'binary': 'originals/Netstorm.exe', 'vtable': 0x50c6f0,
        'menu_identity': 0x540435, 'decoration_identity': 0x540454,
        'identity_stores': [(0x477663, 0x477679), (0x465d58, 0x465d62)],
        'globals': {'registry': 0x563e38, 'request_event': 0x54272c, 'closed_event': 0x5429ec,
            'foreground': 0x5a405c, 'disabled': 0x5c78d0, 'background_type': 0x5ca920,
            'background_frame': 0x542664, 'marker_type': 0x5411c8,
            'object_form_type': 0x54107c, 'form_pool': 0x5c8464},
        'form_stride': 0x32,
        'constants': {'inset': 0x501444, 'half': 0x500ee4, 'line_raise': 0x5012d0,
            'line_trim': 0x502314, 'quarter': 0x501550},
    },
    'originalCD': {
        'binary': 'originalCD/NETSTORM.EXE', 'vtable': 0x501fa0,
        'menu_identity': 0x535f10, 'decoration_identity': 0x535f8c,
        'identity_stores': [(0x451b0d, 0x451b17), (0x494f0f, 0x494f19)],
        'globals': {'registry': 0x56b748, 'request_event': 0x5128b4, 'closed_event': 0x512b74,
            'foreground': 0x55ba90, 'disabled': 0x55bad4, 'background_type': 0x53f608,
            'background_frame': 0x53f614, 'marker_type': 0x51cab4,
            'object_form_type': 0x51c96c, 'form_pool': 0x5395dc},
        'form_stride': 0x24,
        'constants': {'inset': 0x502004},
    },
}


def main():
    """함수 순서/범위·실제 기계어/슬롯/상수·현재 소스/로그 해시와 미완료 경계를 기록한다."""
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    facts = {}
    files = [Path(__file__), PLAN, ROOT / 'tools/analyze_styletext.py']
    # 판본마다 같은 PC의 읽기 전용 내보내기 범위를 원래 PE 주소에서 판독한다.
    for edition, source in EDITIONS.items():
        binary = ROOT / source['binary']
        pe = pefile.PE(str(binary))
        data = pe.get_memory_mapped_image()
        base = pe.OPTIONAL_HEADER.ImageBase
        folder = ROOT / 'extracted/menuinteraction' / edition
        exported, table = folder / 'creation.c', folder / 'functions.tsv'
        functions = read_ranges(table)
        if [entry for entry, _ in functions] != plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 내보낸 함수 주소/순서가 계획과 다름')
        assembly = []
        # 불연속 함수 범위 사이의 타 함수/데이터를 이어 붙이지 않는다.
        for entry, ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra의 inclusive 끝점을 read_ranges가 변환한 half-open 범위로 판독한다.
            for low, high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high - 1:08x}')
                # 실제 주소/바이트/명령/피연산자를 순서대로 보관한다.
                for instruction in decoder.disasm(data[low - base:high - base], low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        stores = []
        # 앞 단계 식별값 오류 수정의 직접 근거: 실제 생성자가 +20에 기록하는 명령이다.
        for low, high in source['identity_stores']:
            stores.append([f'{i.address:08x}: {i.mnemonic} {i.op_str}' for i in decoder.disasm(data[low - base:high - base], low)])
        assembly_path = folder / 'assembly.txt'
        assembly_path.write_text('\n'.join(assembly) + '\n', encoding='utf-8')
        slots = struct.unpack_from('<16I', data, source['vtable'] - base)
        facts[edition] = {
            'binary': source['binary'], 'functions': [entry for entry, _ in functions],
            'vtable': {'address': f'{source["vtable"]:08x}', 'slots': [f'{value:08x}' for value in slots],
                'draw': f'{slots[9]:08x}', 'handle': f'{slots[10]:08x}',
                'broadcast': f'{slots[11]:08x}', 'attach_object': f'{slots[15]:08x}'},
            'identity': {'menu': struct.unpack_from('<b', data, source['menu_identity'] - base)[0],
                'decoration': struct.unpack_from('<b', data, source['decoration_identity'] - base)[0],
                'field': '+20 command, +8 alignFlags가 아님', 'constructor_commands': stores},
            'globals': {name: f'{address:08x}' for name, address in source['globals'].items()},
            'form_stride': source['form_stride'],
            'constants': {name: struct.unpack_from('<f', data, address - base)[0] for name, address in source['constants'].items()},
            'marker_frame': '현재 타입 +154 = I 구간 count, 출력 물리 프레임은 count+2; +150은 I.first',
        }
        files.extend([binary, exported, table, assembly_path])
    # 빌드에 포함된 실제 연결 대상과 수정한 생성자도 현재 근거에 함께 기록한다.
    for name in [
        'MenuGumpInput.h', 'MenuGumpInput.cpp', 'MenuGumpDraw.h', 'MenuGumpDraw.cpp',
        'MenuGumpWidgets.h', 'MenuGumpWidgets.cpp', 'MenuGump.h', 'MenuGump.cpp',
        'MenuGumpLayout.h', 'MenuGumpLayout.cpp', 'GumpDecoration.h', 'GumpDecoration.cpp',
        'GumpEvents.h', 'GumpEvents.cpp', 'GumpInput.h', 'GumpInput.cpp', 'GumpTree.h', 'GumpTree.cpp',
        'GumpLifetime.h', 'GumpLifetime.cpp', 'GumpCollection.h', 'GumpCollection.cpp',
        'GumpDraw.h', 'GumpDraw.cpp', 'GumpBackgroundDraw.h', 'GumpBackgroundDraw.cpp',
        'GumpTextDraw.h', 'GumpTextDraw.cpp', 'StyleText.h', 'StyleText.cpp', 'CMakeLists.txt',
    ]:
        files.append(ROOT / 'cpppj/src/client' / name)
    files.append(ROOT / 'extracted/menuinteraction-build.log')
    report = {
        'last_decompile_host': socket.gethostname(),
        'recorded_at': datetime.datetime.now().astimezone().isoformat(),
        'method': '두 PE/Ghidra/가상 표/명령/필드 정적 판독과 Release 빌드; 게임/GUI/에뮬레이터 실행 없음',
        'cpp_comparison': '미수행: 새 테스트 추가/실행·독립 원본/C++ 사건/콜백/등록표/픽셀 대조 없음',
        'new_original_observations': 0, 'new_cpp_comparisons': 0,
        'implementation': {
            'draw': '같은 배경/글리프/StyleText/측정/장식 SHP 캐시; 자체 클립/강조 배경/세로 구분선 2pass/체크/저장 스타일/하위 표시',
            'broadcast': 'ScreenToLocal/메뉴 자식 가림/가로 절삭/세로 행 탐색/disabled 제외/old-new 강조/행 또는 전체 요청',
            'handle': '확장·release·사건 필터/가상 통지/명령과 sound/자식 닫기/동적 요청/등록 ID/Push-Attach/깊이/확장/부모 통지/팝업 Kill',
            'close': '현재 개수·재귀/Redraw·숨김·동적 Kill/childId와 bit20 지움; 선택 인덱스 유지',
            'attach_object': 'Form(type,2)/owner ID/가상 객체 연결의 명시 경계→flags40/object/form index→현재 가상 Attach; object0은 기존 연결 유지',
            'identity_fix': '앞 단계 Menu/Decoration 생성자의 식별값 쓰기를 alignFlags에서 실제 +20 command로 수정',
            'edition_difference': 'CD unsigned float 비트 하한/패치 ordered 하한·Rect bottom의 패치 Y float32 저장/CD 정수 유지·Form stride/필드 쓰기 순서',
        },
        'boundaries': [
            '독립 PE/C++ 입력/픽셀/콜백/영역/생성/수명 전수 대조와 극단 x87',
            '실제 사건/팔레트/타입 전역·명령 큐·변경 영역·가상 동적 하위 메뉴/allocator·전체 Form 풀/소유/객체 연결 공급',
            '메뉴 제목/머리 포맷·controlled 항목 래퍼·CD 자식 문맥 전달 참고 함수',
            '자동 raw Client UI/OS 큐/폴링/overlay·건설/미션 완주·회전 VFX/그림 색 표',
        ],
        'editions': facts,
        'files': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('메뉴 출력/입력 정적 기록 완료:', ', '.join(f'{edition} {len(value["functions"])}개' for edition, value in facts.items()))


if __name__ == '__main__':
    main()
