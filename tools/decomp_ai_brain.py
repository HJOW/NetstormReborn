#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH AI생성자/소멸자의전체본체/Player/방향별동맹/실제목록수명을격리관찰한다. 게임실행없음."""
import csv
import json
import socket
import struct
import sys
import zlib
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

# 실제소유메모리fixture주소공간이며원본실제프로세스포인터를주장하지않는다.
BASE, SIZE, ISLANDS = 0x13000000, 0xb08, 0x568b60


class Oracle(MemoryOracle):
    """생성자/소멸자와기존방향별동맹기계어만허용한다."""
    def __init__(self):
        """현재정밀범위를읽고실제본체/목록/SEH격리storage를추가한다."""
        super().__init__();self.mu.mem_map(BASE, 0x100000);self.mu.mem_map(POOL + 0x100000, 0x100000);self.mu.mem_map(0, 0x1000);self.allowed = []
        for path, selected in [('extracted/ai-brain/originals/functions.tsv', None), ('extracted/refined/originals/functions.tsv', {0x490ea0, 0x48f6a0, 0x48f550})]:
            with (ROOT / path).open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    if selected is not None and int(row['entry'], 16) not in selected:continue
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def allocate(self, size):
        """명시fixture의실제allocator응답을채우며생성자가쓰지않은byte를보존한다."""
        token = self.next;index = len(self.blocks);self.next += (size + 7) & ~7;self.blocks.append((token, size));self.mu.mem_write(token, bytes((byte * 29 + self.seed + index * 17) & 255 for byte in range(size)));return token

    def instruction(self, mu, address, size, data):
        """힙/실제사제·지면·섬소유/metadata만관찰하며초기화·동맹은실제명령이다."""
        esp = mu.reg_read(UC_X86_REG_ESP);ecx = mu.reg_read(UC_X86_REG_ECX);args = self.words(esp + 4, 8)
        if address == 0x4e4391:self.ret(self.allocate(args[0]))
        elif address == 0x4e4354:self.frees.append(args[0]);self.ret()
        elif address == 0x4921f0:self.events.append('P,' + str(args[0]));self.ret(self.priest)
        elif address == 0x46f530:
            self.events.append('R,' + str(args[0]) + ',' + str(args[1]));self.ret(self.region, 8)
        elif address == 0x46ee30:
            index = args[0];self.ret(int(index < 3), 4)
        elif address == 0x46ef10:
            region, owner = args[:2];self.put(ISLANDS + 4 + region * 32 + 8, owner)
            if self.words(PLAYERS + owner * 180 + 0x78, 1)[0] == 127:self.put(PLAYERS + owner * 180 + 0x78, region)
            self.ret(purge=8)
        elif address == 0x49a840:self.ret(TYPES + args[0] * 500)
        elif address == 0x4e65ed:
            type_num = (args[0] - TYPES - 0xa0) // 500;self.events.append('E,' + str(type_num));first, second = self.string(args[0]).lower(), self.string(args[1]).lower();self.ret((first > second) - (first < second))
        elif not any(low <= address < high for low, high in self.allowed):raise RuntimeError(f'검토하지않은AI생성명령 {address:08x}')
        self.instructions += 1
        if self.instructions > 100000:raise RuntimeError('AI생성명령상한초과')

    def run(self, values):
        """명시input을설정한뒤전체본체와모든공유상태/할당/소멸을관찰한다."""
        owner, local, first, ability, mask, metadata, self.priest, self.region, self.seed = values;self.next = BASE;self.blocks = [];self.frees = [];self.events = [];self.instructions = 0
        self.mu.mem_write(PLAYERS, b'\0' * 1620);self.put(PLAYERS + owner * 180 + 0x78, first);self.put(0x59534c, PLAYERS);self.put(0x5c8464, POOL);self.put(0x5c847c, 32768);self.put(0x540c70, local);self.put(0x595344, 0);self.put(0x594fc8, 0);self.put(0x594fbc, 0);self.put(0x541348, 48);self.put(0x50c038, 127);self.put(0x540410, 254)
        for player in range(9):
            for target in range(9):self.put(0x595200 + (player * 9 + target) * 4, 1 if player == target else 7 if (player + target) % 3 == 0 else 0)
        self.mu.mem_write(ISLANDS, b'\0' * 3204);self.put(ISLANDS, 4)
        for index in range(4):
            self.put(ISLANDS + 4 + index * 32 + 4, int(index < 3));self.put(ISLANDS + 4 + index * 32 + 8, 4);self.mu.mem_write(ISLANDS + 4 + index * 32 + 16, struct.pack('<ffff', 1, 1, 10, 10))
        if 0 < self.priest < 32768:self.mu.mem_write(POOL + self.priest * 50 + 14, struct.pack('<ff', 5.75, 6.25))
        for index in range(48):self.put(TYPES + index * 500 + 0xec, 0);self.mu.mem_write(TYPES + index * 500 + 0xa0, b'\0')
        if metadata == 1:
            for index, flags, text in [(3, 0x40800, b'ws'), (4, 0x800, b'Ss'), (5, 0x40000, b'ss')]:self.put(TYPES + index * 500 + 0xec, flags);self.mu.mem_write(TYPES + index * 500 + 0xa0, text + b'\0')
        elif metadata == 2:self.put(TYPES + 0xec, 0x40800);self.mu.mem_write(TYPES + 0xa0, b'ss\0')
        profile = bytearray((index * 37 + self.seed) & 255 for index in range(600));struct.pack_into('<III', profile, 0x23c, mask, 0, 0);struct.pack_into('<I', profile, 0x248, ability);self.mu.mem_write(TYPES + 0x1f000, bytes(profile));token = self.allocate(SIZE)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<III', STOP, owner, TYPES + 0x1f000));self.mu.reg_write(UC_X86_REG_ECX, token);self.mu.emu_start(0x416860, STOP)
        main = bytes(self.mu.mem_read(token, SIZE)).hex();players = bytes(self.mu.mem_read(PLAYERS, 1620)).hex();allies = bytes(self.mu.mem_read(0x595200, 324)).hex();islands = bytes(self.mu.mem_read(ISLANDS, 3204)).hex();blocks = ';'.join(str(address) + ',' + str(size) + ',' + str(zlib.adler32(bytes(self.mu.mem_read(address, size)))) for address, size in self.blocks)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<I', STOP));self.mu.reg_write(UC_X86_REG_ECX, token);self.mu.emu_start(0x410100, STOP)
        return [','.join(map(str, values)), main, players, allies, islands, blocks, ','.join(map(str, self.frees)), ';'.join(self.events) or '-']


def cases():
    """정상/첫지역/능력/metadata/방향별조건의고유input을생성한다."""
    for owner in (1, 8):
        for local in (1, 8):
            for first in (5, 127):
                for ability in (0, 1):
                    for mask in (0, 1 << local, 0x1fe):
                        for metadata in (0, 1, 2):yield [owner, local, first, ability, mask, metadata, 70, 2, 19]
    for priest in (0, 32767, 32768, 65535):
        for region in (-1, 2, 3, 127):yield [1, 8, 127, 1, 256, 1, priest, region, 93]


def main():
    """실제관찰과전체근거SHA를저장/감사한다."""
    fixture = ROOT / 'cpppj/tests/fixtures/ai-brain-x86.tsv';report = ROOT / 'cpppj/ai-brain.json'
    sources = ['originals/Netstorm.exe', 'extracted/ai-brain/originals/creation.c', 'extracted/ai-brain/originals/functions.tsv', 'extracted/refined/originals/functions.tsv', 'tools/ghidra/ai-brain-functions.json', 'tools/decomp_ai_brain.py', 'tools/decomp_priestmenu.py', 'cpppj/src/o/RawAiBrain.h', 'cpppj/src/o/RawAiBrain.cpp', 'cpppj/tests/AiBrainTests.cpp', 'cpppj/tests/fixtures/ai-brain-x86.tsv']
    if '--verify' in sys.argv:
        prior = json.loads(report.read_text(encoding='utf-8'))
        for name, value in prior['sha256'].items():assert digest(ROOT / name) == value, name
        with fixture.open(encoding='utf-8') as stream:rows = list(csv.DictReader(stream, delimiter='\t'))
        assert len(rows) == prior['observations'];assert len({row['input'] for row in rows}) == len(rows);print('AiBrain 원본고유입력/SHA감사통과:', len(rows));return
    oracle = Oracle();rows = [oracle.run(values) for values in cases()];assert len({row[0] for row in rows}) == len(rows)
    with fixture.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['input', 'raw', 'players', 'allies', 'islands', 'blocks', 'frees', 'trace']);writer.writerows(rows)
    facts = {'host': socket.gethostname(), 'date': '2026-10-15', 'observations': len(rows), 'boundaries': ['body preallocated0xb08 and17actual nested allocations backed by fixture memory', 'priest/region/island-owner/TYPE-energy responses; SetAlliance/AddAlly machine code real', 'initialUpdate00414710 and strategies not exercised; ctor itself never attaches player brain or flags20', 'virtual handles remap host-owned memory; no original pointer/OOM equality claim'], 'sha256': {name: digest(ROOT / name) for name in sources}}
    report.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8');print('AiBrain 원본고유입력:', len(rows))


if __name__ == '__main__':main()
