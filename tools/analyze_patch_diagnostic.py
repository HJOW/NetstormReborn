#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 공유 진단 DWORD 초기값과 실제 명령 참조를 읽기 전용으로 확인한다. 원문 실행 검사가 아니다."""
import argparse
import csv
import hashlib
import json
import platform
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 실제 저장소와 격리 분석 의존성 기준이다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_AC_WRITE
from capstone.x86 import X86_OP_MEM

ADDRESS = 0x5318ec  # 보호 10.78의 최초6 진단·정책 공유 DWORD다.
BODIES = {0x494580: ('사제 회복', 4), 0x4d55e0: ('Player 내용물 통지', 4),
          0x488370: ('BFS', 0), 0x498220: ('일반 그림자', 0),
          0x499fe0: ('Present 후 카메라', 0)}  # 함수 진입부터 해석하고 직접 쓰기 수를 따로 확인한다.


def digest(path):
    """원문·정밀 함수 범위·분석기 바이트의 SHA256을 보관한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect():
    """함수의 각 원문 범위에서 공유 DWORD 명령과 초기 PE 데이터를 정적으로 읽는다."""
    binary = ROOT / 'originals/Netstorm.exe'
    table = ROOT / 'extracted/refined/originals/functions.tsv'
    pe = pefile.PE(str(binary))
    image = pe.get_memory_mapped_image()
    initial = struct.unpack_from('<I', image, ADDRESS - pe.OPTIONAL_HEADER.ImageBase)[0]
    assert initial == 6, initial
    with table.open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    bodies = []
    for entry, (name, expected_writes) in BODIES.items():  # 보호 함수 범위를 넘어 다른 명령을 이어 붙이지 않는다.
        references = []
        for part in facts[entry]['ranges'].split(';'):  # 불연속 함수 조각도 정밀 내보내기의 실제 시작 경계를 따른다.
            first, last = (int(value, 16) for value in part.split('-'))
            offset = first - pe.OPTIONAL_HEADER.ImageBase
            for instruction in decoder.disasm(image[offset:offset + last - first + 1], first):  # 올바른 x86 경계에서만 주소 피연산자를 읽는다.
                operands = [operand for operand in instruction.operands if operand.type == X86_OP_MEM and operand.mem.disp == ADDRESS]
                if not operands:
                    continue
                assert all(operand.size == 4 for operand in operands)
                references.append(dict(address=f'{instruction.address:08x}', bytes=instruction.bytes.hex(),
                    mnemonic=instruction.mnemonic, operands=instruction.op_str,
                    writes=any(operand.access & CS_AC_WRITE for operand in operands)))
        assert references, hex(entry)
        assert sum(reference['writes'] for reference in references) == expected_writes, references
        bodies.append(dict(entry=f'{entry:08x}', name=name, references=references))
    return dict(host=platform.node(), address=f'{ADDRESS:08x}', initial=initial,
        method='정적 PE 데이터·명령 읽기, 원문 실행0', bodies=bodies,
        sha256={path.relative_to(ROOT).as_posix(): digest(path) for path in (binary, table, Path(__file__).resolve())})


def main():
    """기존 보고서를 덮어쓰지 않고 명령·원문 해시를 UTF-8 JSON으로 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    report = inspect()
    with args.report.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print(json.dumps(dict(initial=report['initial'], bodies=len(report['bodies']),
        references=sum(len(body['references']) for body in report['bodies'])), ensure_ascii=False))


if __name__ == '__main__':
    main()
