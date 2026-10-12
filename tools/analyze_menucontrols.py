#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 controlled 메뉴/제목/머리·CD 문맥 helper 근거와 현재 C++ 구현 해시를 정적으로 기록한다.

사용법: menucontrols Ghidra 읽기 전용 내보내기와 Release 빌드 후
python -X utf8 tools/analyze_menucontrols.py
게임/GUI/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT, digest, read_ranges

# 두 보호 PE는 읽기만 하고 계획·기계어/근거 기록은 저장소 안에 만든다.
PLAN = ROOT / 'tools/ghidra/menucontrols-functions.json'
REPORT = ROOT / 'cpppj/recovery-menucontrols-evidence.json'
# 같은 controlled 표/현재 index/메뉴 개수/등록 표와 실제 빈 구분선 문자열 주소다.
EDITIONS = {
    'originals': {'binary': 'originals/Netstorm.exe', 'stack': 0x592fd0, 'index': 0x593020,
        'instances': 0x593024, 'registry': 0x563e38, 'empty_separator': 0x503c8b},
    'originalCD': {'binary': 'originalCD/NETSTORM.EXE', 'stack': 0x51fc28, 'index': 0x51fc78,
        'instances': 0x51fc7c, 'registry': 0x56b748, 'empty_separator': 0x51fddc},
}


def read_string(data, base, address):
    """실제 이미지 안 주소의 최대 256바이트 ASCII 문자열만 판독한다. 원본 메모리를 실행하거나 쓰지 않는다."""
    offset = address - base
    if offset < 0 or offset >= len(data):
        return None
    raw = data[offset:offset + 256]
    end = raw.find(b'\0')
    if end < 0:
        return None
    try:
        return raw[:end].decode('ascii')
    except UnicodeDecodeError:
        return None


def main():
    """함수 순서/범위·명령/실제 ROM 키/서식·현재 연결/빌드 해시와 미검증 경계를 남긴다."""
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    files = [Path(__file__), PLAN, ROOT / 'tools/analyze_styletext.py']
    facts = {}
    # 같은 PC의 읽기 전용 내보내기 범위를 원래 PE 주소에서 각각 판독한다.
    for edition, source in EDITIONS.items():
        binary = ROOT / source['binary']
        pe = pefile.PE(str(binary))
        data = pe.get_memory_mapped_image()
        base = pe.OPTIONAL_HEADER.ImageBase
        folder = ROOT / 'extracted/menucontrols' / edition
        exported, table = folder / 'creation.c', folder / 'functions.tsv'
        functions = read_ranges(table)
        if [entry for entry, _ in functions] != plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 함수 주소/순서가 계획과 다름')
        assembly, strings = [], {}
        # 불연속 범위 사이의 다른 함수/데이터는 이어 붙이지 않는다.
        for entry, ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 실제 주소/바이트/명령/피연산자와 immediate 문자열 참조를 순서대로 기록한다.
            for low, high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high - 1:08x}')
                # 키/서식은 직접 push한 PE 문자열 주소에서 읽으며 임의 문자열을 추정하지 않는다.
                for instruction in decoder.disasm(data[low - base:high - base], low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
                    if instruction.mnemonic == 'push' and instruction.operands and instruction.operands[0].type == capstone.CS_OP_IMM:
                        address = instruction.operands[0].imm
                        value = read_string(data, base, address)
                        if value is not None and value.startswith(('Menu', '~')):
                            strings[f'{address:08x}'] = value
        assembly_path = folder / 'assembly.txt'
        assembly_path.write_bytes(('\n'.join(assembly) + '\n').encode('utf-8'))
        facts[edition] = {
            'binary': source['binary'], 'functions': [entry for entry, _ in functions],
            'globals': {name: f'{source[name]:08x}' for name in ['stack', 'index', 'instances', 'registry']},
            'rom_keys_and_formats': strings,
            'separator': {'address': f'{source["empty_separator"]:08x}',
                'text': read_string(data, base, source['empty_separator'])},
        }
        files.extend([binary, exported, table, assembly_path])
    # 새 래퍼와 실제 항목/제목/설정/문맥 저장소를 함께 기록한다.
    for name in [
        'cpppj/src/client/MenuGumpControls.h', 'cpppj/src/client/MenuGumpControls.cpp',
        'cpppj/src/client/MenuGump.h', 'cpppj/src/client/MenuGump.cpp',
        'cpppj/src/client/MenuGumpLayout.h', 'cpppj/src/client/MenuGumpLayout.cpp',
        'cpppj/src/client/ScrollingTextGumpMarkup.h', 'cpppj/src/client/ScrollingTextGumpMarkup.cpp',
        'cpppj/src/client/StyleTextMarkup.h', 'cpppj/src/client/StyleTextMarkup.cpp',
        'cpppj/src/client/StyleText.h', 'cpppj/src/client/StyleText.cpp',
        'cpppj/src/o/ConfigInterface.h', 'cpppj/src/o/ConfigInterface.cpp',
        'cpppj/src/client/CMakeLists.txt', 'extracted/menucontrols-build.log',
    ]:
        files.append(ROOT / name)
    report = {
        'last_decompile_host': socket.gethostname(),
        'recorded_at': datetime.datetime.now().astimezone().isoformat(),
        'recorded_date': '2026-10-13',
        'method': '두 PE/Ghidra/ROM 키·서식/flags/등록표·문맥 복사 정적 판독과 Release 빌드',
        'cpp_comparison': '미수행: 새 테스트 추가/실행·독립 PE/C++ 항목/태그/콜백/픽셀 대조·게임/GUI 실행 없음',
        'new_original_observations': 0, 'new_cpp_comparisons': 0,
        'implementation': {
            'text_checked': '같은 현재 controlled 칸과 실제 Append/Reflow·include/enabled·명령/명시 인자/체크·패치 선 assert/색 조회·CD null 칸 생략/원문 전달·패치 체크 명령0에도 bit1',
            'title_header': '패치 MenuTitle/MenuHeader 색/~3~E·일반 추가 Menu 색/disabled; CD 제목 ~3%s~./머리 무인자 서식·fancy 실제 다중 줄 Parse만 호출',
            'submenu_separator': '패치 MenuSub 색/항상 styled·CD 첫 NUL 앞 tilde로 styled 결정·실제 자식 Attach/flags·빈 구분선/선두·연속·40개 한도/미작성 필드 유지',
            'format': '고정 %d/%s 접두는 signed 숫자/원문 삽입; 실제 무인자 경로는 literal/%%만 처리·첫 NUL/256 결과 한도·원본의 인자 없는 변환/메모리 초과는 호스트 거부',
            'cd_context': '0x20 자식의 현재 등록 ID·8DWORD 순서 복사·재귀·자신/StyleText/hidden/dead/Redraw/Reflow는 변경/필터하지 않음',
            'cd_selection': '현재 항목 강조를 먼저 지우고 첫 비영 등록/visible 자식에서 조기 반환·뒤 항목 유지',
            'no_menus': '실제 생성/소멸 개수==0이며 controlled 칸/visible/dead·지연 Kill과 구분',
            'binding': '실제 ConfigInterface.ReadInt·MenuGumpLayout.Append·ScrollingTextGumpMarkup.Parse를 사용',
        },
        'boundaries': [
            '독립 PE/C++ 항목/설정/순서/태그/클립/픽셀/문맥/수명 대조와 기존 UI 전체 대조',
            '공용 번호 인자/임의 printf 서식기와 전역 작업표/호스트 포인터 alias·256바이트 원본 오버런',
            '전체 Form 풀/소유/객체 연결·동적 하위 메뉴·실제 전역/명령 큐/변경 영역/allocator 공급',
            'OS 큐/폴링/overlay·자동 raw Client UI/건설/미션 완주·그림 색 표/회전 VFX/flag 2 재사용·극단 x87/생성 EH',
        ],
        'editions': facts,
        'files': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in files},
    }
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print('controlled 메뉴 정적 기록 완료:', ', '.join(f'{edition} {len(value["functions"])}개' for edition, value in facts.items()))


if __name__ == '__main__':
    main()
