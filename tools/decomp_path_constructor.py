#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 경로8 cdecl 생성자/할당 실패를 실제 PE에서 격리한다. 게임/창/Host는 실행하지 않는다."""
import argparse
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 신규 fixture의 기준이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/path-constructor-x86.tsv'  # 전체100080BYTE의 실제 실행 결과를 RLE로 보존한다.
OBJECT, STACK, STOP, SIZE = 0x12000020, 0x20000800, 0x30000000, 0x186f0  # 서로 분리한 객체/스택/반환 주소/원문 크기다.
CONSTRUCTORS = ((23, 0x48be40), (24, 0x48bf30), (38, 0x48bed0), (39, 0x48bea0), (40, 0x48bf90), (41, 0x48bf00), (42, 0x48be70), (54, 0x48bf60))  # 원문 표에서 읽은 주소다.


class PathConstructorOracle:
    """실제 wrapper/common을 실행하고 operator new의 크기/결과만 외부 입력으로 공급한다."""

    def __init__(self):
        """그대로 읽은 PE와 앞뒤 보호 바이트를 둔 독립 객체를 설치한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(OBJECT & ~4095, 0x1a000)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        self.entry, self.fail, self.allocations, self.object_writes = 0, False, [], 0  # 이번 입력의 실행/할당 관찰이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """원문 두 몸체 밖 실행은 거부하고 할당 경계만 cdecl로 반환한다."""
        if address == 0x4e4391:
            sp = mu.reg_read(UC_X86_REG_ESP)
            ret, amount = struct.unpack('<2I', mu.mem_read(sp, 8))
            assert amount == SIZE
            self.allocations.append(amount)
            mu.reg_write(UC_X86_REG_EAX, 0 if self.fail else OBJECT)
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        assert self.entry <= address <= self.entry + 0x28 or 0x48b2c0 <= address <= 0x48b330, hex(address)

    def write(self, mu, access, address, size, value, data):
        """부모/전역/장치 쓰기를 허용하지 않고 스택과 객체 내부 저장만 기록한다."""
        if OBJECT <= address and address + size <= OBJECT + SIZE:
            self.object_writes += 1
            return
        assert STACK - 0x100 <= address and address + size <= STACK

    def observe(self, entry, fill, ignored, fail=False):
        """남은 caller 인자와 ECX를 바꿔도 생성자는 읽지 않음을 실제 반환/전체 바이트로 확인한다."""
        self.entry, self.fail, self.allocations, self.object_writes = entry, fail, [], 0
        self.mu.mem_write(OBJECT - 32, bytes([0x6b]) * (SIZE + 64))
        self.mu.mem_write(OBJECT, bytes([fill]) * SIZE)
        arguments = struct.pack('<4I', STOP, ignored, 0, 0xabcdef01)
        self.mu.mem_write(STACK, arguments)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, ignored)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.emu_start(entry, STOP, count=20000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert bytes(self.mu.mem_read(STACK, len(arguments))) == arguments
        assert self.allocations == [SIZE]
        assert self.mu.reg_read(UC_X86_REG_EAX) == (0 if fail else OBJECT)
        assert bytes(self.mu.mem_read(OBJECT - 32, 32)) == bytes([0x6b]) * 32
        assert bytes(self.mu.mem_read(OBJECT + SIZE, 32)) == bytes([0x6b]) * 32
        result = bytes(self.mu.mem_read(OBJECT, SIZE))
        if fail:
            assert self.object_writes == 0 and result == bytes([fill]) * SIZE
        else:
            assert self.object_writes == 10016, self.object_writes
        return ','.join(f'{len(list(group))}:{byte}' for byte, group in itertools.groupby(result))


def main():
    """8함수×두 초기 heap×두 잔여 인자를 실행하고 별도로8회 NULL 할당 복귀를 관찰한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle = PathConstructorOracle()
    rows = []  # 원문 전체 바이트 관찰이며 C++ 결과는 입력하지 않는다.
    # 각 wrapper와 독립적인 미초기화/잔여 인자 입력을 교차한다.
    for kind, entry in CONSTRUCTORS:
        for fill, ignored in itertools.product((0, 165), (0, 65535)):
            rows.append((kind, entry, fill, ignored, oracle.observe(entry, fill, ignored)))
        oracle.observe(entry, 165, 65535, True)
    content = '# type entry heapFill ignoredCallerArgument nativeWholeObjectRle; PATCH cdecl32행/NULL8행 별도관찰\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify를 사용한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'nativeRows': len(rows), 'nullAllocationObservations': 8, 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
