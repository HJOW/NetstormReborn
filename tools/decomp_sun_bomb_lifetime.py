#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 TYPE163 생성자와 삭제 가상 진입을 읽기 전용으로 기록한다."""
import argparse
import hashlib
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 보호 입력과 별도 관측의 기준이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/sun-bomb-lifetime-vtables.tsv'  # 구현 출력과 독립인 PE 정적 관측이다.
REPORT = ROOT/'cpppj/sun-bomb-lifetime-native.json'  # 정상 반환 횟수로 합산하지 않는 정적 근거다.


def digest(path):
    """파일을 변경하지 않고 현재 바이트의 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observe():
    """기존 생성자 fixture와 보호 PE의 대입 명령 및 실제 가상 표만 읽는다."""
    constructors = {}  # 이전에 독립 실행으로 동결한 판본별163 생성자 주소다.
    # 기존 원문은 읽기만 하며 새 기대값으로 대체하지 않는다.
    for line in (ROOT/'cpppj/tests/fixtures/constructors-x86.tsv').read_text('utf-8').splitlines():
        if line and not line.startswith('#'):
            edition, kind, address = line.split('\t')
            if kind == '163':
                constructors[edition] = int(address)
    rows = []  # 각 판본의 생성자/표/삭제/전처리/후처리/Unpop 원문 주소다.
    disassembly = {}  # 가상 표 대입을 재검토할 수 있는 원문 명령이다.
    # 두 판본은 같은 주소/몸체로 추정하지 않고 각 PE를 별개로 읽는다.
    for edition in ('originals', 'originalCD'):
        pe = pefile.PE(str(ROOT/edition/'Netstorm.exe'))
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        address = constructors[edition]  # 기존 fixture가 가리키는 실제 생성자다.
        instructions = list(decoder.disasm(pe.get_data(address-pe.OPTIONAL_HEADER.ImageBase, 32), address))
        assignments = [i for i in instructions if i.mnemonic == 'mov' and len(i.operands) == 2
                       and i.operands[0].type == capstone.CS_OP_MEM and i.operands[1].type == capstone.CS_OP_IMM]
        assert len(assignments) == 1
        table = assignments[0].operands[1].imm  # 원본 mov의 상수이며 C++에서 읽지 않는다.
        slots = [struct.unpack('<I', pe.get_data(table-pe.OPTIONAL_HEADER.ImageBase+offset, 4))[0]
                 for offset in (0x10, 0x14, 0x18, 0x48)]
        rows.append('\t'.join([edition, '163', *(f'{value:08x}' for value in (address, table, *slots))]))
        disassembly[edition] = [f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in instructions]
    return '# edition type constructor vtable destroy pre post unpop; PE 정적 관측\n'+'\n'.join(rows)+'\n', disassembly


def main():
    """최초 관측을 보존하거나 기존 바이트와 읽기 전용 재관측을 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--replay', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 과거 기대값이나 해시를 다시 쓰지 않는다.
        for path, expected in report['sha256'].items():
            assert digest(ROOT/path) == expected, path
        print('훨리기그 폭탄 가상 표 정적 근거 감사 통과')
        return
    content, disassembly = observe()
    if args.replay:
        assert content.encode('utf-8') == FIXTURE.read_bytes()
        print('두 판본 폭탄 가상 표 재관측 일치')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    names = ['originals/Netstorm.exe', 'originalCD/Netstorm.exe', 'cpppj/tests/fixtures/constructors-x86.tsv',
             'tools/decomp_sun_bomb_lifetime.py', FIXTURE.relative_to(ROOT).as_posix()]  # 기존 원문과 이번 정적 도구/결과만 고정한다.
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(), lastDecompile='VM-W11-Codex / 2026-10-19',
        observations=2, normalReturns=0, scope='PE 정적 읽기. 10.78 TYPE163의45d2a0→509b98과 가상10/14/18/48. CD10.72는 별도 주소 기록만 하며 실행 연결 완료로 세지 않는다.',
        disassembly=disassembly, sha256={name:digest(ROOT/name) for name in names}), ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(content)


if __name__ == '__main__':
    main()
