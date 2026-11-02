#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 옵션 저장 부모의 전체 경로 작성·지역 복사·changed 관문을 CPU로 관측한다."""
import argparse
import csv
import json
import socket
from pathlib import Path

from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

FIXTURE = ROOT / 'cpppj/tests/fixtures/options-path-parent-x86.tsv'  # NULL/명시 root의 최초 독립 경로 기대값이다.


class OptionsOracle(NameOracle):
    """실제441d10/441de0→459c60/strchr를 실행하고 설정 저장 진입만 관측한다."""
    def __init__(self):
        """원문 부모의 함수표 범위만 추가하고 저장 호출 목록은 매 관측마다 초기화한다."""
        super().__init__()
        self.saved = []  # 실제440f30에 전달된 지역 문자열과 경로 고리 번호다.
        facts = {int(row['entry'], 16): row for row in csv.DictReader(
            (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        for entry in (0x441d10, 0x441de0):  # 함수 사이 다른 코드는 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 떨어진 실제 코드 범위를 보존한다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """저장 본체를 합성하지 않고 실제 인자가 고리가 아닌 부모 지역 복사인지 확인한다."""
        if address == 0x440f30:
            target = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]
            assert STACK - 0x2000 <= target < STACK - 16
            self.saved.append(dict(pathHex=self.string(target).hex(), localCopy=True,
                                   index=self.words(INDEX, 1)[0]))
            self.ret()
            return
        super().instruction(mu, address, size, data)

    def parent(self, root, precision, changed=None):
        """NULL/빈 root와 별도 changed 부모를 비영 스택·보존 레지스터로 실행한다."""
        self.saved.clear()
        self.thrown = False  # throw와 정상 저장 진입을 구별한다.
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, 0)
        self.put(0x532728, 92, 1)
        self.put(0x558dfc, 0 if changed is None else changed)
        value = b'' if root is None else root  # NULL은 문자열 포인터0이며 빈 문자열은 유효 포인터다.
        self.mu.mem_write(TEXT, value + b'\0')
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 실제 cdecl 보존 규약을 비영 값으로 대조한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        saved = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.put(STACK + 4, 0 if root is None else TEXT)
        self.mu.emu_start(0x441d10 if changed is None else 0x441de0, STOP, count=30000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert saved == [self.mu.reg_read(register) for register in REGISTERS]
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert bytes(self.mu.mem_read(TEXT, len(value) + 1)) == value + b'\0'
        index = self.words(INDEX, 1)[0]
        assert index == len(self.saved) == (0 if changed == 0 else 1)
        return dict(index=index, saves=list(self.saved), slotHex=bytes(self.mu.mem_read(RING + index * 256, 256)).hex())


def main():
    """기존 fixture를 덮지 않고 최초 원문 출력 또는 동일 재관측을 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    oracle = OptionsOracle()
    roots = [None, b'', b'C:\\root', b'C:/root//', b'\\\\server\\share\\', b'relative', b'dir\\', b'.']
    fixture = ['# root_kind root_hex output_hex current_slot_hex']
    rows = []
    for root in roots:  # NULL/빈/절대/상대/UNC와 중복 separator를 구별한다.
        result = oracle.parent(root, 0x027f)
        assert result == oracle.parent(root, 0x037f)
        fixture.append(f'{"null" if root is None else "value"}\t{(root or b"").hex() or "-"}\t{result["saves"][0]["pathHex"]}\t{result["slotHex"]}')
        rows.append(result)
    conditional = []
    for changed in (0, 1):  # 변경0은 경로 고리/저장에 진입하지 않는다.
        result = oracle.parent(None, 0x027f, changed)
        assert result == oracle.parent(None, 0x037f, changed)
        conditional.append(dict(changed=changed, **result))
    content = '\n'.join(fixture) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, 'utf-8', newline='\n')
    report = dict(unit=729, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  rows=rows, conditional=conditional, fixtureRows=len(roots), observations=20,
                  boundary='실제441d10/441de0→459c60/strchr 전체이며440f30은 지역 인자만 기록하고 반환한다. 설정 직렬화/디스크/changed 후속 변경은 실행하지 않는다.',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)): digest(path) for path in (
                      ROOT / 'originals/Netstorm.exe', Path(__file__), ROOT / 'tools/decomp_file_name_formatter.py', FIXTURE)})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(fixtureRows=len(roots), observations=20, conditional=[row['index'] for row in conditional])))


if __name__ == '__main__':
    main()
