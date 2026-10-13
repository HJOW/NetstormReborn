#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GumpForm의 두 PE/가상 표/내장 타입/종속 체인·소유 UI 알림과 현재 C++ 구현 해시를 정적으로 기록한다.

사용법: gumpform Ghidra 읽기 전용 내보내기와 Release 빌드 후
python -X utf8 tools/analyze_gumpform.py
게임/GUI/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT, digest, read_ranges

# 입력 PE는 읽기만 하고 분석 산출물은 저장소의 extracted/ 및 cpppj/에 기록한다.
PLAN = ROOT / 'tools/ghidra/gumpform-functions.json'
REPORT = ROOT / 'cpppj/recovery-gumpform-evidence.json'
# 실제 GumpForm·기본 Gump·메뉴 가상 표/내장 타입 전역/등록 표와 보호 PE의 기준 SHA-256이다.
EDITIONS = {
    'originals': {'binary': 'originals/Netstorm.exe', 'form_vtable': 0x512188,
        'base_gump_vtable': 0x50ac70, 'menu_vtable': 0x50c6f0, 'type_global': 0x54107c,
        'registry': 0x563e38, 'pool_global': 0x5c8464, 'stride': 0x32,
        'sha256': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'},
    'originalCD': {'binary': 'originalCD/NETSTORM.EXE', 'form_vtable': 0x505a80,
        'base_gump_vtable': 0x5048a8, 'menu_vtable': 0x501fa0, 'type_global': 0x51c96c,
        'registry': 0x56b748, 'pool_global': 0x5395dc, 'stride': 0x24,
        'sha256': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233'},
}
# 원본 가상 표의 역할/바이트 위치다. 생성/Take는 빈 함수, 삭제 전후/Pop은 공통 깊이 감소다.
FORM_SLOTS = {
    'parent': 0x00, 'previous': 0x04, 'set_parent': 0x08, 'set_previous': 0x0c,
    'destroy': 0x10, 'pre_destroy': 0x14, 'post_destroy': 0x18, 'parent_dying': 0x1c,
    'post_pop': 0x20, 'post_create': 0x28, 'post_take': 0x2c, 'unpop': 0x48,
    'pop': 0x4c, 'contained_attach': 0x50, 'attach': 0x74, 'notify': 0x78,
}


def dword(data, base, address):
    """PE 이미지의 실제 주소에서 정렬되지 않은 little endian DWORD를 읽는다."""
    return struct.unpack_from('<I', data, address - base)[0]


def main():
    """실제 내보내기 순서/기계어/표·타입 전역/현재 소스·빌드 해시와 미복원 경계를 기록한다."""
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    files = [Path(__file__), PLAN, ROOT / 'tools/analyze_styletext.py']
    facts = {}
    # 판본별 주소/바이트/가상 표를 다른 바이너리에서 독립적으로 읽는다. 실행 결과를 추정하지 않는다.
    for edition, source in EDITIONS.items():
        binary = ROOT / source['binary']
        if digest(binary) != source['sha256']:
            raise RuntimeError(f'{edition}: 보호 PE의 기준 SHA-256과 다름')
        pe = pefile.PE(str(binary))
        data = pe.get_memory_mapped_image()
        base = pe.OPTIONAL_HEADER.ImageBase
        folder = ROOT / 'extracted/gumpform' / edition
        exported, table = folder / 'creation.c', folder / 'functions.tsv'
        functions = read_ranges(table)
        if [entry for entry, _ in functions] != plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 함수 주소/순서가 계획과 다름')
        assembly = []
        # Ghidra 범위의 실제 PE 바이트만 디스어셈블한다.
        for entry, ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 범위 사이의 다른 함수/데이터는 이어 붙이지 않는다.
            for low, high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high - 1:08x}')
                # 원본 주소/바이트/명령/피연산자를 순서대로 기록한다.
                for instruction in decoder.disasm(data[low - base:high - base], low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path = folder / 'assembly.txt'
        assembly_path.write_bytes(('\n'.join(assembly) + '\n').encode('utf-8'))
        type_value = dword(data, base, source['type_global'])
        if type_value != 3:
            raise RuntimeError(f'{edition}: PE의 내장 GumpForm 타입이 3이 아님')
        facts[edition] = {
            'binary': source['binary'], 'functions': [entry for entry, _ in functions],
            'stride': source['stride'],
            'globals': {name: f'{source[name]:08x}' for name in ['type_global', 'pool_global', 'registry']},
            'gump_form_type_initial_value': type_value,
            'form_vtable': f'{source["form_vtable"]:08x}',
            'form_slots': {name: {'offset': offset, 'function': f'{dword(data, base, source["form_vtable"] + offset):08x}'}
                for name, offset in FORM_SLOTS.items()},
            'gump_virtual_object_release': {
                'base': f'{dword(data, base, source["base_gump_vtable"] + 4):08x}',
                'menu': f'{dword(data, base, source["menu_vtable"] + 4):08x}'},
        }
        files.extend([binary, exported, table, assembly_path])
    # 신규 구현과 실제 SID/삭제/메뉴/등록/지연 수명 저장소의 해시를 함께 남긴다.
    for name in [
        'cpppj/src/o/GumpFormHost.h', 'cpppj/src/o/GumpFormHost.cpp',
        'cpppj/src/o/SquidFactory.h', 'cpppj/src/o/SquidFactory.cpp',
        'cpppj/src/o/SidPool.h', 'cpppj/src/o/SidPool.cpp',
        'cpppj/src/o/RawSquidDestroy.h', 'cpppj/src/o/RawSquidDestroy.cpp',
        'cpppj/src/o/SquidProcess.h', 'cpppj/src/o/SquidProcess.cpp',
        'cpppj/src/client/GumpFormBinding.h', 'cpppj/src/client/GumpFormBinding.cpp',
        'cpppj/src/client/MenuGumpWidgets.h', 'cpppj/src/client/MenuGumpWidgets.cpp',
        'cpppj/src/client/GumpCreate.h', 'cpppj/src/client/GumpLifetime.h', 'cpppj/src/client/GumpLifetime.cpp',
        'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/CMakeLists.txt', 'extracted/gumpform-build.log',
    ]:
        files.append(ROOT / name)
    report = {
        'last_decompile_host': socket.gethostname(),
        'recorded_at': datetime.datetime.now().astimezone().isoformat(),
        'recorded_date': '2026-10-13',
        'method': '두 PE/Ghidra/내장 타입·가상 표/필드 폭·종속 체인/소유 UI 알림 정적 판독과 Release 빌드',
        'cpp_comparison': '미수행: 새 테스트 추가/실행·독립 PE/C++ 풀/필드/알림/중첩 삭제 대조·게임/GUI 실행 없음',
        'new_original_observations': 0, 'new_cpp_comparisons': 0,
        'implementation': {
            'allocation': '기존 SidPool/SquidFactory·flags 2 client 영역·같은 반납 FIFO/삭제 기록·vtable→type만 쓰고 자산 owner/HP/zOrder 생략',
            'ownership': 'Form +12 DWORD Gump 등록 ID·+10 WORD 부모·소유 ID 0 알림 생략·알림 시점 현재 등록 표/가상 +4',
            'attachment': 'GumpForm 가상 +74는 공통 contained 부착·process debug 조건 없음·부모/next/이전 머리 prev/head/자신 prev/contained/void 순서',
            'edition_difference': '패치 부모 0 무동작; CD 부모 0 assert 뒤 계속 경로는 호스트 거부·가상 표/stride는 실제 판본별 값',
            'unpop': 'void 먼저 설정·부모 free/dead 링크 보존·살아 있는 부모의 next/prev/head/contained 정리·raw 부모/next/prev 자체는 남음',
            'notification': '가상 +1c/+78 동일·현재 소유 ID/부모 읽고 콜백 뒤 Form을 다시 읽지 않음·기본/메뉴 가상 +4 실제 Kill',
            'destruction': '기존 RawSquidDestroy dead/pre/post 깊이·종속 next 선저장/알림 뒤 free 재조회·실제 Unpop/반납·Form 자체 삭제에는 UI 알림 없음',
            'binding': 'BindMenuGumpForms/BindGumpFormLifetime·GumpForm 토큰/index에 같은 SID·MakeGumpFormNotice 현재 등록 조회·다른 타입 삭제 분배 보존',
        },
        'boundaries': [
            '독립 PE/C++ SID/쓰기 순서/콜백/중첩 삭제/반납 대조와 기존 전체 UI 동작/픽셀 대조',
            '서버 Form Pop 직렬화/전송·예약/손상 SID·CD 부모0 assert 이후 실행·풀 용량 재구성과 지정 Take',
            'DependForm/ContentForm 전체 가상 수명·다른 파생 Gump 객체 삭제 재정의·파생 처리기 내부의 중첩 Form 재분배',
            '실제 전체 Client/월드 풀과 가상 분배 연결·동적 메뉴·전역/명령 큐/변경 영역/allocator 공급',
            'OS 큐/폴링/overlay·자동 raw Client UI/건설/미션 완주·공용 서식기/그림 색 표/회전 VFX/극단 x87/생성 EH',
        ],
        'editions': facts,
        'files': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files},
    }
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print('GumpForm 정적 기록 완료:', ', '.join(f'{edition} {len(value["functions"])}개' for edition, value in facts.items()))


if __name__ == '__main__':
    main()
