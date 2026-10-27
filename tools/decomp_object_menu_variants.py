#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본10.78의 여덟 파생 우클릭 작성기를 실행하고 큰 공통/AI/HP/GUI 경계를 관측한다."""
import itertools
import json
import random
import socket
import struct
from pathlib import Path
from decomp_object_menu import Oracle as BaseOracle, ROOT, POOL, TYPES, TEXT, VTABLE, CALLBACK, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 PE의 파생+a8와 반환까지의 범위다. 462550은 기존 Ghidra 함수 표에 없어 명령 바이트로 별도 확인했다.
ENTRIES = ((0x41c210, 0x41c28e), (0x422000, 0x4220e3), (0x462390, 0x462411),
           (0x462550, 0x462572), (0x485600, 0x485640), (0x48eca0, 0x48ece0),
           (0x4dcca0, 0x4dccf9), (0x420b30, 0x420be8))
COMMANDS = (0x542ad4, 0x542b5c, 0x542b60, 0x542b64, 0x542a2c, 0x5427ac, 0x5428c8, 0x542754, 0x542974)


class Oracle(BaseOracle):
    """의미를 재계산하지 않고 실제 분기와 호출 인자를 기록한다."""
    def __init__(self):
        """기존 원본 PE/독립 저장소를 사용하며 정확한 파생 범위만 추가한다."""
        super().__init__('originals')
        self.allowed = list(ENTRIES) + [(0x40f100, 0x40f12b)]
        self.counts = {}  # 원문 진입별 실제 실행 명령 수다.

    def change(self, where):
        """명시 콜백 재진입 입력으로 owner/타입/모드가 바뀌는 시점만 재생한다."""
        mutation = self.case[-1]
        if mutation == 1 and where == 'translate':
            self.put(self.raw + 34, 2, 1)
        if mutation == 2 and where == 'count':
            self.put(self.raw + 10, 122, 1)
        if mutation == 3 and where == 'ai':
            self.put(self.raw + 34, 2, 1)
        if mutation == 4 and where == 'process':
            self.put(0x5c85a4, 1)
        if mutation == 5 and where == 'translate':
            self.put(self.raw + 26, 0)

    def instruction(self, mu, address, size, data):
        """원문 자체와 실제40f100은 실행하고 GUI/게임 큰 하위만 명시 입력으로 반환한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = [self.word(esp + 4 + i * 4) for i in range(6)]
        if address in (0x4b1270, 0x44cbd0):
            self.emit('B' if address == 0x4b1270 else 'D', 50); self.ret()
        elif address == 0x44ca00:
            self.emit('S', *args[:3]); self.ret()
        elif address == CALLBACK:
            self.emit('C', 50); self.change('count'); self.ret(self.case[7])
        elif address == 0x40da40:
            self.emit('L'); self.ret(self.word(0x540c70))
        elif address == 0x4adcb0:
            self.emit('H', 50); self.ret(self.case[5])
        elif address == 0x4afb40:
            self.emit('P', 50, args[0]); self.change('process'); self.ret(self.case[6], 4)
        elif address == 0x413dd0:
            self.emit('AI', args[0]); self.change('ai'); self.ret()
        elif address == 0x4de9a0:
            self.emit('T', self.string(args[0]).hex()); self.change('translate'); self.ret(args[0], 4)
        elif address == 0x4a89c0:
            pattern = self.string(args[1]); value = struct.unpack('<i', struct.pack('<I', args[2]))[0]
            self.emit('F', pattern.hex(), args[2]); mu.mem_write(args[0], pattern.replace(b'%d', str(value).encode()) + b'\0'); self.ret(args[0])
        elif address == 0x478020:
            self.emit('A', self.string(args[0]).hex()); self.ret()
        elif address == 0x478260:
            self.emit('X'); self.ret()
        elif address == 0x477cb0:
            self.emit('I', self.string(args[0]).hex(), *args[1:6]); self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'미등록 파생 원문 {address:08x}'
            self.instructions += 1

    def run(self, entry, case, precision):
        """원문 callee 보존/스택/정밀도를 검사하고 현재 슬롯과 전체 효과 순서를 반환한다."""
        self.case = case
        owner, local, rawtype, modes, hp, maximum, process, count, quantity, brain, mutation = case
        self.raw = POOL + 2500
        self.events = []
        self.mu.mem_write(POOL, bytes(0x10000))
        self.put(0x5c8464, POOL); self.put(self.raw, VTABLE); self.put(VTABLE + 0x80, CALLBACK)
        self.put(self.raw + 10, rawtype, 1); self.put(self.raw + 34, owner, 1)
        self.put(self.raw + 26, hp); self.put(self.raw + 12, quantity, 2)
        self.put(0x540c70, local); self.put(0x5c85a4, modes & 1); self.put(0x55a480, modes & 2); self.put(0x594fa4, modes & 4)
        self.put(0x54db38, TEXT); self.mu.mem_write(TEXT, b'About\0')
        self.put(0x59534c, TYPES + 0x18000)
        # 같은 Player의 현재 brain DWORD만 실제40f100에 공급한다.
        for number in range(9):
            self.put(TYPES + 0x18000 + number * 180 + 0xb0, brain)
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 원문이 보존해야 하는 레지스터를 오염 표식으로 초기화한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        control = 0x027f if precision == 53 else 0x037f
        self.mu.reg_write(UC_X86_REG_FPCW, control); self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_ECX, self.raw); self.put(STACK, STOP)
        self.mu.emu_start(entry, STOP, count=2000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(reg) == value for reg, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        return ';'.join(self.events) or '-'


def cases():
    """소유자·모드·부호 HP·수량·프로세스·재진입을 독립 입력하며 기대 메뉴는 만들지 않는다."""
    base = (1, 1, 75, 0, 51, 100, 0, 7, 65535, 0, 0)
    rows = [base]
    changes = {0: (0, 2, 8), 1: (0, 2), 2: (82, 100, 112, 122, 125, 150), 3: range(8),
               4: (-2147483648, -1, 0, 49, 50, 200), 5: (-2147483648, -101, -1, 0, 1, 101, 2147483647),
               6: (0, 77), 7: (0, 0xffffffff), 8: (0, 1, 32768), 9: (0, 0x12345678), 10: range(6)}
    # 한 필드 경계와 실제 사용되는 모드 교차를 별도로 보존한다.
    for index, values in changes.items():
        for value in values:
            row = list(base); row[index] = value; rows.append(tuple(row))
    for owner, modes, process, brain in itertools.product((0, 1, 2), range(8), (0, 77), (0, 0x12345678)):
        row = list(base); row[0] = owner; row[3] = modes; row[6] = process; row[9] = brain; rows.append(tuple(row))
    return list(dict.fromkeys(rows))


def main():
    """구현 전에 실제 파생 명령의 fixture를 고정하고 두 x87 정밀도 차이를 거부한다."""
    assert digest('originals/Netstorm.exe') == 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    oracle = Oracle(); rows = []
    # 각 파생과 같은 독립 입력을 원문에서 두 번 실행한다.
    for low, high in ENTRIES:
        start = oracle.instructions
        for case in cases():
            trace = oracle.run(low, case, 53)
            assert trace == oracle.run(low, case, 64)
            rows.append(f'{low:08x}\t' + ','.join(map(str, case)) + '\t' + trace)
        oracle.counts[f'{low:08x}'] = oracle.instructions - start
    fixture = ROOT / 'cpppj/tests/fixtures/object-menu-variants-x86.tsv'
    fixture.write_text('# entry\tinput\tnative_trace\n' + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon / 2026-10-23', observations=len(rows), precisionDifferences=0,
                  instructions=oracle.counts, boundaries='공통 객체/Damageable/회수/HP/프로세스/큰 AI 디버그와 GUI/번역은 명시 경계다. 실제40f100과 여덟 파생의 분기/인자/순서를 실행했다.',
                  sha256={name: digest(name) for name in ('originals/Netstorm.exe', 'tools/decomp_object_menu_variants.py', 'cpppj/tests/fixtures/object-menu-variants-x86.tsv')})
    (ROOT / 'cpppj/object-menu-variants-native-20261028.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(observations=len(rows), instructions=oracle.counts)))


if __name__ == '__main__':
    main()
