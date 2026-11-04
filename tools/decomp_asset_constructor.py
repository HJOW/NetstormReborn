#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 자산 bare cdecl의 물리 쓰기를 격리 PE에서 관찰한다. 공중3종/Outpost 새 생성은 제외한다."""
import argparse
import json
import re
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 새 fixture 기준이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/asset-constructor-current-x86.tsv'  # 실제 전체 슬롯 전후를 보존한다.
POOL, STACK, STOP = 0x12000000, 0x20000800, 0x30000000  # 원문 포인터와 겹치지 않는 격리 주소다.
SIDS = (0, 4, 5, 50, 23001, 32767, 65534)  # 예약/free/dead와 같은 물리 끝 경계다.
EDITIONS = {'originals': ('originals/Netstorm.exe', 'Patch', 50, 0x5c8464), 'originalCD': ('originalCD/NETSTORM.EXE', 'Cd', 36, 0x5395dc)}  # 원문 판본 폭/전역이다.
EXCLUDED_TYPES = (73, 126, 138, 144)  # 기존 사용자 제한의 세 공중 actor와 MP Outpost다.


def addresses(label):
    """기존 주소 표는 입력 함수 탐색에만 사용한다. expected BYTE는 PE 실행에서 얻는다."""
    constructors = (ROOT / 'cpppj/src/o/TypeConstructors.inc').read_text(encoding='utf-8').split('k' + label + 'Constructors{', 1)[1].split('};', 1)[0]
    types = [int(value, 16) for value in re.findall(r'0x([0-9a-fA-F]+)U', constructors)]
    records = (ROOT / 'cpppj/src/o/DerivedConstructors.inc').read_text(encoding='utf-8').split('k' + label + 'Recipes{{', 1)[1].split('}};', 1)[0]
    recipes = {int(value, 16) for value in re.findall(r'\{0x([0-9a-fA-F]+)U,0x[0-9a-fA-F]+U,', records)}
    excluded = {types[index] for index in EXCLUDED_TYPES}
    return sorted(recipes - excluded), sorted(excluded)


class AssetConstructorOracle:
    """실제 cdecl과 그 내부 호출을 실행하고 슬롯 밖 저장/알 수 없는 진단은 거부한다."""

    def __init__(self, path, stride, global_pool):
        """PE/전체 물리 풀/스택을 설치한다. 실제 게임/할당/장치 API는 호출하지 않는다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / path)).get_memory_mapped_image()
        self.image_end = 0x400000 + ((len(image) + 4095) & ~4095)
        self.mu.mem_map(0x400000, self.image_end - 0x400000)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(POOL, 0x400000)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_write(global_pool, struct.pack('<I', POOL))
        self.stride, self.target, self.entry, self.mutation = stride, POOL, 0, 0  # 이번 물리 입력이다.
        self.diagnostics, self.pool_reads, self.writes = [], 0, []  # 실제 진단/조회/저장 관찰이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def string(self, address):
        """실제 PE가 넘긴 진단 문자열을 끝까지 읽는다."""
        value = bytearray()
        # NUL 또는 명시 최대1024BYTE까지 읽는다.
        for index in range(1024):
            byte = self.mu.mem_read(address + index, 1)[0]
            if not byte:
                return value.decode('ascii')
            value.append(byte)
        raise RuntimeError('진단 문자열 경계 초과')

    def instruction(self, mu, address, size, data):
        """Dais 보조의 단일 진단만 입력 경계로 반환한다. 다른 진단/API는 성공으로 대신하지 않는다."""
        if address == 0x4e0620:
            assert self.entry == 0x44a420
            sp = mu.reg_read(UC_X86_REG_ESP)
            ret, expression, file, line = struct.unpack('<4I', mu.mem_read(sp, 16))
            diagnostic = [self.string(expression), self.string(file), line]
            assert diagnostic == ['false', '.\\Dais.cpp', 0x484]
            self.diagnostics.append(diagnostic)
            if self.mutation:
                mu.mem_write(self.target + 10, bytes([75]))
                mu.mem_write(self.target + 34, bytes([4]))
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        assert 0x400000 <= address < self.image_end, hex(address)

    def write(self, mu, access, address, size, value, data):
        """목표 슬롯의 실제 저장과 자기 스택 이외 쓰기를 거부한다."""
        if self.target <= address and address + size <= self.target + self.stride:
            self.writes.append([address - self.target, size, value & ((1 << (size * 8)) - 1)])
            return
        assert STACK - 0x100 <= address and address + size <= STACK, (hex(address), size)

    def read(self, mu, access, address, size, value, data):
        """NULL 함수는 풀/SID 메모리도 읽지 않는지 실제 실행 접근만 센다."""
        if POOL <= address < POOL + 0x400000:
            self.pool_reads += 1

    def observe(self, entry, sid, mutation=0):
        """다른 payload와 TYPE74/free/dead를 공급하고 전체 슬롯·정상 cdecl 반환을 관찰한다."""
        self.target, self.entry, self.mutation = POOL + sid * self.stride, entry, mutation
        before = bytearray((index * 29 + (entry & 255) * 7 + sid * 13) & 255 for index in range(self.stride))
        before[10], before[11] = 74, sid % 4
        self.mu.mem_write(self.target, bytes(before))
        arguments = struct.pack('<4I', STOP, sid, 0, 0xabcdef01)
        self.mu.mem_write(STACK, arguments)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, 0xdeadbeef)
        self.diagnostics, self.pool_reads, self.writes = [], 0, []
        self.mu.emu_start(entry, STOP, count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert bytes(self.mu.mem_read(STACK, 16)) == arguments
        after = bytes(self.mu.mem_read(self.target, self.stride))
        assert after[10] == (75 if mutation else 74) and after[11] == before[11]
        if entry == 0x4429e0:
            assert not self.pool_reads and not self.writes and not self.diagnostics and self.mu.reg_read(UC_X86_REG_EAX) == 0 and after == before
        return before.hex(), after.hex(), self.mu.reg_read(UC_X86_REG_EAX), len(self.diagnostics)


def main():
    """허용 recipe145개와 PATCH 두 특수 함수만 실행한다. 기존 기대값은 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows, selected = [], {}  # 실제 입력/출력과 제외한 함수 목록이다.
    # 양판본의 일반 recipe를 각각 실행하며 네 제외 주소는 실행하지 않는다.
    for edition, (path, label, stride, global_pool) in EDITIONS.items():
        oracle = AssetConstructorOracle(path, stride, global_pool)
        recipes, excluded = addresses(label)
        selected[edition] = {'recipes': len(recipes), 'excluded': [f'{value:08x}' for value in excluded]}
        for entry in recipes:
            for sid in SIDS:
                rows.append((edition, entry, sid, 0, *oracle.observe(entry, sid)))
        if edition == 'originals':
            for sid in SIDS:
                rows.append((edition, 0x4429e0, sid, 0, *oracle.observe(0x4429e0, sid)))
                for mutation in (0, 1):
                    rows.append((edition, 0x44a420, sid, mutation, *oracle.observe(0x44a420, sid, mutation)))
            oracle.observe(0x4429e0, 65535)
    content = '# edition entry sid mutation nativeBefore nativeAfter nativeEax diagnostics; 실제 물리 생성/공중3종과MP전용0\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify를 사용한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'nativeRows': len(rows), 'selection': selected, 'extraNullInvalidSid': 1, 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
