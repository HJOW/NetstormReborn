#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 Renderer 진단 전역과 +40 격자 접근을 읽기 전용으로 열거한다. 게임은 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
import platform
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 마지막 정밀 함수표를 찾는 저장소 루트다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import capstone
import pefile

GLOBALS = {0x59a8cc: 'afterEach', 0x59a8b4: 'hideGroups', 0x55a484: 'debugKeys',
           0x540ce8: 'sort'}  # 화면 본체가 읽는 실제 10.78 DWORD 주소다.


def audit():
    """함수표 범위 안의 기계어만 해독하고 실제 메모리 피연산자/접근 권한을 기록한다."""
    executable = ROOT / 'originals/Netstorm.exe'
    pe = pefile.PE(str(executable))
    mapped = pe.get_memory_mapped_image()
    image_base = pe.OPTIONAL_HEADER.ImageBase
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    references = {name: [] for name in GLOBALS.values()}
    grid = []  # Draw 본체의 실제 this 레지스터 EBP+40 접근이며 스택 지역 변수는 제외한다.
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        functions = list(csv.DictReader(stream, delimiter='\t'))
    # 불연속 함수 범위도 내보낸 실제 구간별로 읽고 데이터 영역을 선형 해독하지 않는다.
    for function in functions:
        entry = int(function['entry'], 16)
        # 각 함수의 분리된 몸체를 독립 해독한다.
        for part in function['ranges'].split(';'):
            first, last = (int(value, 16) for value in part.split('-'))
            # 실행 가능한 원본 함수의 메모리 피연산자만 살핀다.
            for instruction in decoder.disasm(mapped[first-image_base:last-image_base+1], first):
                # 한 명령에 여러 피연산자가 있어도 실제 대상 메모리만 기록한다.
                for operand in instruction.operands:
                    if operand.type != capstone.CS_OP_MEM:
                        continue
                    record = dict(address=f'{instruction.address:08x}', function=f'{entry:08x}',
                                  instruction=instruction.mnemonic+' '+instruction.op_str,
                                  read=bool(operand.access & capstone.CS_AC_READ),
                                  write=bool(operand.access & capstone.CS_AC_WRITE))
                    if not operand.mem.base and not operand.mem.index and operand.mem.disp in GLOBALS:
                        references[GLOBALS[operand.mem.disp]].append(record)
                    if entry == 0x4994b0 and operand.mem.base == capstone.x86.X86_REG_EBP and operand.mem.disp == 0x40:
                        grid.append(record)
    initial = {}  # BSS도 메모리 이미지의 실제 0 바이트로 읽는다.
    # 네 실제 전역의 초기값을 해석과 별도로 보존한다.
    for address, name in GLOBALS.items():
        offset = address-image_base
        initial[name] = int.from_bytes(mapped[offset:offset+4], 'little')
    return dict(host=platform.node(), last_full_decompilation_host='HJOW-Athlon',
                last_full_decompilation_date='2026-10-23', game_executed=False,
                executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                initial=initial, references=references, renderer_offset40=grid,
                limitation='직접 메모리 피연산자 열거이며 간접 포인터 alias의 부재를 단독으로 증명하지 않는다.')


def main():
    """필수 출력 경로에 UTF-8 관측 보고서를 쓰며 원본과 정밀 소스는 변경하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    report = audit()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(initial=report['initial'], references={name: len(rows) for name, rows in report['references'].items()},
                         renderer_offset40=report['renderer_offset40']), ensure_ascii=False))


if __name__ == '__main__':
    main()
