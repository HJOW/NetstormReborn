#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH timer57의 Init 없는 cdecl/기본 생성자를 격리 PE에서 실행한다. 게임/창/통신은 실행하지 않는다."""
import argparse
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 신규 관찰의 기준이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bomb-timer-constructor-x86.tsv'  # 전체40BYTE/미사용400BYTE를 원문 그대로 보존한다.
OBJECT, MEMBERS, STACK, STOP = 0x12000020, 0x12001020, 0x20000800, 0x30000000  # 각 주소 앞뒤에 보호 바이트를 둔다.
NOW = 0x55b4d0  # 생성자가 읽지 않아야 하는 현재 게임 시각이다.


class BombTimerConstructorOracle:
    """원문 wrapper/공통 생성자는 그대로 실행하고 두 new의 결과만 외부에서 공급한다."""

    def __init__(self):
        """PE와 객체/목록/스택/SEH 첫 포인터를 분리해 설치한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(OBJECT & ~4095, 0x2000)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_map(0, 0x1000)
        self.fill, self.failure, self.allocations, self.object_writes, self.clock_reads = 0, 0, [], 0, 0  # 이번 입력의 관찰이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """공통/base/SEH 정상 복귀를 실제 실행하며 new만 cdecl로 반환한다."""
        if address == 0x4e4391:
            sp = mu.reg_read(UC_X86_REG_ESP)
            ret, amount = struct.unpack('<2I', mu.mem_read(sp, 8))
            expected = (40, 400)[len(self.allocations)]
            assert amount == expected
            self.allocations.append(amount)
            value = OBJECT if amount == 40 else MEMBERS
            if self.failure == len(self.allocations):
                value = 0
            mu.reg_write(UC_X86_REG_EAX, value)
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        assert 0x420ad0 <= address <= 0x420b25 or 0x420960 <= address <= 0x4209c5, hex(address)

    def write(self, mu, access, address, size, value, data):
        """객체 내부/현재 SEH/스택 이외의 쓰기는 거부한다. 목록은 비어 있어 쓰면 안 된다."""
        if OBJECT <= address and address + size <= OBJECT + 40:
            self.object_writes += 1
            return
        assert address == 0 and size == 4 or STACK - 0x100 <= address and address + size <= STACK

    def read(self, mu, access, address, size, value, data):
        """현재 시각을 실제로 조회하는 명령이 없는지 별도로 센다."""
        if address < NOW + 8 and address + size > NOW:
            self.clock_reads += 1

    def observe(self, fill, ignored, now_bits, failure=0):
        """미초기화 flags/남은 인자/현재 시각과 두 할당 실패 위치를 독립 입력한다."""
        self.fill, self.failure, self.allocations, self.object_writes, self.clock_reads = fill, failure, [], 0, 0
        self.mu.mem_write(OBJECT - 32, bytes([0x6b]) * 104)
        self.mu.mem_write(OBJECT, bytes([fill]) * 40)
        self.mu.mem_write(MEMBERS - 32, bytes([0x6b]) * 464)
        self.mu.mem_write(MEMBERS, bytes([0xd7]) * 400)
        self.mu.mem_write(0, struct.pack('<I', 0x11223344))
        self.mu.mem_write(NOW, struct.pack('<Q', now_bits))
        arguments = struct.pack('<4I', STOP, ignored, 0, 0xabcdef01)
        self.mu.mem_write(STACK, arguments)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, ignored)
        self.mu.emu_start(0x420ad0, STOP, count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert bytes(self.mu.mem_read(STACK, 16)) == arguments
        assert bytes(self.mu.mem_read(0, 4)) == struct.pack('<I', 0x11223344)
        assert self.clock_reads == 0
        assert self.allocations == ([40] if failure == 1 else [40, 400])
        assert self.mu.reg_read(UC_X86_REG_EAX) == (0 if failure == 1 else OBJECT)
        assert bytes(self.mu.mem_read(OBJECT - 32, 32)) == bytes([0x6b]) * 32
        assert bytes(self.mu.mem_read(OBJECT + 40, 32)) == bytes([0x6b]) * 32
        assert bytes(self.mu.mem_read(MEMBERS - 32, 32)) == bytes([0x6b]) * 32
        assert bytes(self.mu.mem_read(MEMBERS + 400, 32)) == bytes([0x6b]) * 32
        members = bytes(self.mu.mem_read(MEMBERS, 400))
        assert members == bytes([0xd7]) * 400
        result = bytes(self.mu.mem_read(OBJECT, 40))
        if failure == 1:
            assert self.object_writes == 0 and result == bytes([fill]) * 40
        else:
            assert self.object_writes == 7 and result[32:] == bytes([fill]) * 8
            assert struct.unpack_from('<I', result, 12)[0] == (0 if failure == 2 else MEMBERS)
        return result.hex(), members.hex()


def main():
    """정상12행과 외부/내부 할당 NULL4회를 실제 실행하고 기존 fixture 덮어쓰기를 거부한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle = BombTimerConstructorOracle()
    rows = []  # C++ 계산과 독립적인 원문 관찰이다.
    # heap/미사용 인자/시각의 독립 조합을 순서대로 실행한다.
    for fill, ignored, now in itertools.product((0, 165), (0, 65535), (0, 0x4059100000000000, 0x7ff8000000000000)):
        raw, members = oracle.observe(fill, ignored, now)
        rows.append((fill, ignored, now, raw, members))
    # NULL 외부 객체는0반환, NULL 내부 목록은 원문처럼 객체 자체를 반환한다.
    for fill, failure in itertools.product((0, 165), (1, 2)):
        oracle.observe(fill, 65535, 0x7ff8000000000000, failure)
    content = '# heapFill ignoredCallerArgument nowBits nativeObject40 nativeMembers400; 실제 원문12행/NULL4회 별도관찰\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify를 사용한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'nativeRows': len(rows), 'nullAllocationObservations': 4, 'reobserved': args.verify, 'clockReads': 0, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
