#!/usr/bin/env python3
"""원본 exe 바이트에서 특정 함수를 부르는 곳을 찾는 읽기 전용 스캔 도구.

전체 디컴파일(extracted/decomp/Netstorm.c)에 없는 함수(가상 함수 표·함수 포인터로만 호출되거나
디컴파일에서 빠진 함수)가 어디서 불리는지 찾을 때 쓴다. 디컴파일·원본 실행 없이 바이트만 읽는다.

사용법 (저장소 루트에서):
    python tools/exe_callscan.py 0x460df0 0x460e30        # 직접 call(E8) 호출 지점
    python tools/exe_callscan.py --refs 0x4c2a40          # 4바이트 값으로 들어 있는 위치(가상 함수 표·push·mov)
    python tools/exe_callscan.py --table 0x5149ec 12      # 가상 함수 표 덤프(시작 주소, 슬롯 수)
"""
import argparse
import re
import struct
import sys
from pathlib import Path

# 기본 exe 경로: 이 파일(tools/) 기준 저장소 루트의 originals/Netstorm.exe
DEFAULT_EXE = Path(__file__).resolve().parent.parent / "originals" / "Netstorm.exe"
# 섹션 특성 플래그: 코드 포함(IMAGE_SCN_CNT_CODE)
SCN_CODE = 0x20
# E8 명령 길이(opcode 1바이트 + rel32 4바이트)
CALL_LENGTH = 5


def load_sections(data):
    """PE 헤더를 읽어 (이름, 가상 주소, 원시 크기, 파일 오프셋, 특성) 목록을 만든다."""
    pe = struct.unpack_from("<I", data, 0x3C)[0]                 # PE 헤더 오프셋
    count = struct.unpack_from("<H", data, pe + 6)[0]            # 섹션 수
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]   # 선택 헤더 크기
    image_base = struct.unpack_from("<I", data, pe + 24 + 28)[0]  # 이미지 기준 주소
    sections = []
    # 섹션 헤더(40바이트)를 차례로 읽는다
    for i in range(count):
        o = pe + 24 + optional_size + i * 40
        name = data[o:o + 8].rstrip(b"\0").decode()
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, o + 8)
        characteristics = struct.unpack_from("<I", data, o + 36)[0]
        sections.append((name, image_base + rva, raw_size, raw_offset, characteristics))
    return sections


def offset_to_va(sections, offset):
    """파일 오프셋을 (섹션 이름, 가상 주소)로 바꾼다. 섹션 밖이면 (None, None)."""
    for name, va, raw_size, raw_offset, _ in sections:
        if raw_offset <= offset < raw_offset + raw_size:
            return name, va + offset - raw_offset
    return None, None


def va_to_offset(sections, va):
    """가상 주소를 파일 오프셋으로 바꾼다. 섹션 밖이면 None."""
    for _, sva, raw_size, raw_offset, _ in sections:
        if sva <= va < sva + raw_size:
            return raw_offset + va - sva
    return None


def scan_calls(data, sections, target):
    """목적지가 target인 E8 rel32 호출 명령의 가상 주소 목록(코드 섹션만 검사)."""
    found = []
    for _, va, raw_size, raw_offset, characteristics in sections:
        if not characteristics & SCN_CODE:
            continue
        segment = data[raw_offset:raw_offset + raw_size]
        # 모든 E8 바이트를 후보로 보고 rel32를 해석한다
        for match in re.finditer(b"\xe8", segment):
            i = match.start()
            if i + CALL_LENGTH > raw_size:
                continue
            rel = struct.unpack_from("<i", segment, i + 1)[0]
            if (va + i + CALL_LENGTH + rel) & 0xFFFFFFFF == target:
                found.append(va + i)
    return found


def scan_refs(data, sections, value):
    """value가 4바이트 리틀엔디언 값으로 들어 있는 모든 위치: (섹션, 가상 주소, 앞뒤 바이트)."""
    needle = struct.pack("<I", value)
    found = []
    for match in re.finditer(re.escape(needle), data):
        name, va = offset_to_va(sections, match.start())
        context = data[max(match.start() - 4, 0):match.start() + 8].hex(" ")
        found.append((name, va, context))
    return found


def dump_table(data, sections, start, slots):
    """가상 함수 표를 4바이트 슬롯 단위로 읽어 (슬롯 번호, 슬롯 주소, 값) 목록으로 돌려준다."""
    offset = va_to_offset(sections, start)
    return [(i, start + 4 * i, struct.unpack_from("<I", data, offset + 4 * i)[0]) for i in range(slots)]


def main():
    """명령줄 인자를 읽어 요청한 스캔을 실행하고 결과를 출력한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("addresses", nargs="*", help="함수 가상 주소(16진수)")
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE, help="분석할 exe (기본: originals/Netstorm.exe)")
    parser.add_argument("--refs", action="store_true", help="E8 호출 대신 4바이트 값 참조 위치를 찾는다")
    parser.add_argument("--table", nargs=2, metavar=("START", "SLOTS"), help="가상 함수 표 덤프")
    args = parser.parse_args()
    data = args.exe.read_bytes()
    sections = load_sections(data)
    if args.table:
        for slot, address, value in dump_table(data, sections, int(args.table[0], 16), int(args.table[1])):
            print(f"슬롯 {slot:2d} (+0x{4 * slot:02x}) @0x{address:x} -> 0x{value:08x}")
        return 0
    if not args.addresses:
        parser.print_usage()
        return 2
    # 주소마다 호출 지점 또는 값 참조 위치를 출력한다
    for text in args.addresses:
        target = int(text, 16)
        if args.refs:
            for name, va, context in scan_refs(data, sections, target):
                print(f"0x{target:x} 값 위치 {name} 0x{va:x}  [{context}]")
        else:
            calls = scan_calls(data, sections, target)
            print(f"0x{target:x} 호출 지점: {[hex(x) for x in calls]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
