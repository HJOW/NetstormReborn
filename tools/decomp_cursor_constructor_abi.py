#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""UI50/51 cdecl wrapper의 new 크기/두 인자 전달/NULL·SEH 복귀만 격리 관찰한다. UI 몸체는 명시 경계다."""
import argparse
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 선택 출력의 기준이다.
OBJECT, STACK, STOP = 0x12000000, 0x20000800, 0x30000000  # 원문 포인터가 아닌 격리 외부 입력이다.
ENTRIES = ((0x4741d0, 0x4737f0, 224), (0x474900, 0x474520, 512))  # 원문 wrapper/별도 몸체/new 크기다.


class CursorConstructorAbiOracle:
    """wrapper 전체를 실행하며 이미 독립 복원한 UI 몸체와 allocator만 외부 계약으로 둔다."""

    def __init__(self):
        """PE/스택/SEH 주소를 설치하고 다른 몸체 실행/쓰기와 스택 손상을 거부한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_map(0, 0x1000)
        self.entry, self.body, self.amount, self.fail, self.events = 0, 0, 0, False, []  # 이번 선택과 실제 외부 인자다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """new는 cdecl, 실제 UI Construct는 thiscall ret8 계약으로만 돌려준다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e4391:
            ret, amount = struct.unpack('<2I', mu.mem_read(sp, 8))
            assert amount == self.amount
            self.events.append(['new', amount])
            mu.reg_write(UC_X86_REG_EAX, 0 if self.fail else OBJECT)
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == self.body:
            ret, parent, form = struct.unpack('<3I', mu.mem_read(sp, 12))
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT
            self.events.append(['body', self.body, parent, form])
            mu.reg_write(UC_X86_REG_EAX, OBJECT)
            mu.reg_write(UC_X86_REG_ESP, sp + 12)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        assert self.entry <= address <= self.entry + 0x62, hex(address)

    def write(self, mu, access, address, size, value, data):
        """원문 wrapper의 현재 SEH와 자기 스택 외 저장을 거부한다."""
        assert address == 0 and size == 4 or STACK - 0x100 <= address and address + size <= STACK

    def observe(self, entry, body, amount, parent, form, fail):
        """전체 DWORD 인자와 allocator NULL 여부를 교차하고 cdecl 스택/SEH 복원을 확인한다."""
        self.entry, self.body, self.amount, self.fail, self.events = entry, body, amount, fail, []
        arguments = struct.pack('<4I', STOP, parent, form, 0xabcdef01)
        self.mu.mem_write(STACK, arguments)
        self.mu.mem_write(0, struct.pack('<I', 0x11223344))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, 0xdeadbeef)
        self.mu.emu_start(entry, STOP, count=200)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert self.mu.reg_read(UC_X86_REG_EAX) == (0 if fail else OBJECT)
        assert bytes(self.mu.mem_read(STACK, 16)) == arguments
        assert bytes(self.mu.mem_read(0, 4)) == struct.pack('<I', 0x11223344)
        assert self.events == ([['new', amount]] if fail else [['new', amount], ['body', body, parent, form]])
        return {'entry': f'{entry:08x}', 'parent': parent, 'form': form, 'allocationNull': fail, 'events': self.events}


def main():
    """실제32wrapper 입력을 새 출력에 쓰거나 기존 관찰과 대조한다. UI/게임/Host는 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.output and args.verify:
        raise RuntimeError('output/verify는 함께 사용하지 않는다.')
    oracle = CursorConstructorAbiOracle()
    rows = []  # UI 본체 결과를 만들지 않고 wrapper의 전달 인자만 수집한다.
    # 양 wrapper에 네 부모·두Form·두 확보 결과를 공급한다.
    for entry, body, amount in ENTRIES:
        for parent, form, fail in itertools.product((0, 5, 50, 65535), (0, 65535), (False, True)):
            rows.append(oracle.observe(entry, body, amount, parent, form, fail))
    result = {'observations': rows, 'count': len(rows), 'bodyExecutions': 0, 'guiGameExecutions': 0, 'scope': 'actual wrapper only; Construct and operator new are external boundary'}
    if args.verify:
        assert json.loads(args.verify.read_text(encoding='utf-8-sig')) == result
    if args.output:
        if args.output.exists():
            raise RuntimeError('기존 관찰 덮어쓰기 거부')
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=1) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'nativeWrapperRows': len(rows), 'reobserved': bool(args.verify), 'bodyExecutions': 0, 'guiGameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
