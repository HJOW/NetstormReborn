#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH00414710의전체초기분류/실제기술사슬을원본기계어로관찰한다. 원본게임실행없음."""
import csv
import json
import struct
import sys
from decomp_player_technology import Oracle as TechnologyOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, VTABLE, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제AI소유메모리입력의핸들이며원본실제주소를주장하지않는다.
BODY, BUILDERS = 0x13000000, 0x13000b08


def nodes(pattern):
    """독립내용물타입/flags입력이며원본분류결과를계산하지않는다."""
    return [[], [1, 2, 3, 4, 5, 133], [2], [2, 3], [1] * 6, [1] * 16 + [4], [1, 2, 3, 4], [5, 1]][pattern]


class Oracle(TechnologyOracle):
    """기존실제기술/contained원본명령을재사용하고초기갱신/워크샾/ftol만추가허용한다."""
    def __init__(self):
        """동일PE와현재정밀불연속범위를읽는다."""
        super().__init__();self.mu.mem_map(BODY, 0x10000)
        for path, selected in [('extracted/ai-initial-update/originals/functions.tsv', None), ('extracted/refined/originals/functions.tsv', {0x4e49c0})]:
            with (ROOT / path).open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    if selected is not None and int(row['entry'], 16) not in selected:continue
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """현재언어/기술부모/TYPE에너지입력과실제생성요청만외부경계로받는다."""
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp + 4, 8)
        if address == 0x4410f0:
            assert self.string(args[0]) == b'defaultAiName' and args[1:4] == (0, 0, 0);self.events.append('D');mu.mem_write(TEXT + 0x2000, b'Default AI\0');self.ret(TEXT + 0x2000);return
        if address == 0x48fc90:self.ret(self.words(PLAYERS + args[0] * 180 + 0x9c, 1)[0]);return
        if address == 0x490250:
            self.events.append('J,' + str(args[0]));self.put(PLAYERS + args[0] * 180 + 0x9c, 5);self.next = 6;self.ret(5);return
        if address == 0x49a840 and self.words(esp, 1)[0] in (0x4148cb, 0x4148f3):self.events.append('E,' + str(args[0]))
        if address == 0x4af530:self.born.append(self.next)
        super().instruction(mu, address, size, data)

    def run(self, values):
        """초기화대상원문/현재Player/내용물만명시input으로준비한다."""
        pattern, mission, named, tech, existing, prior, seed = values;self.kind = 'update';self.owner = 1;self.mutate = 0;self.next = 5;self.events = [];self.born = [];self.instructions = 0
        self.mu.mem_write(PLAYERS, b'\0' * 1620);self.mu.mem_write(PLAYERS + 180 + 12, b'Old\0');self.put(PLAYERS + 180 + 0x9c, 50 if existing else 0);self.put(0x59534c, PLAYERS);self.put(0x595344, 1);self.put(0x594fa4, mission);self.put(0x5c8464, POOL);self.put(0x541348, 188);self.put(0x541088, 6);self.put(0x5e4794, 0, 1);self.put(0x54126c, 133)
        for address, value in [(0x5411b0, 86), (0x5411b4, 87), (0x5411b8, 88), (0x5411ac, 85)]:self.put(address, value)
        self.mu.mem_write(POOL + 5 * 50, b'\0' * 50);self.mu.mem_write(POOL + 50 * 50, b'\0' * (32 * 50));self.put(VTABLE + 0x50, STOP + 0x200);self.put(VTABLE + 0x10, STOP + 0x300)
        kinds = nodes(pattern);parent = 50 if existing else 5;self.put(POOL + parent * 50 + 6, 60 if kinds else 0, 2)
        for index, kind in enumerate(kinds):
            raw = POOL + (60 + index) * 50;self.put(raw, VTABLE);self.put(raw + 10, 46 if pattern == 6 and index == 1 else 6, 1);self.put(raw + 11, [0, 8, 9][index % 3], 1);self.put(raw + 4, 61 + index if index + 1 < len(kinds) else 0, 2);self.put(raw + 18, kind, 2);self.put(raw + 20, 0 if pattern == 6 and index == 2 else 4, 2)
        for index in range(188):self.put(TYPES + index * 500 + 0x98, 0);self.put(TYPES + index * 500 + 0xec, 0);self.put(TYPES + index * 500 + 0xc4, 0);self.mu.mem_write(TYPES + index * 500 + 0xa0, b'\0')
        metadata = [(1, 0, 0x10000, 150.5, 's'), (2, 0, 0x800, 100.0, 'ss'), (3, 1, 0x40000, 200.75, 'wz'), (4, 2, 0x800, 800.25, 'rrrr'), (5, 3, 0, float('nan') if pattern == 7 else -1.0, ''), (133, 5, 0x10000, 99.0, 's')]
        for index, theme, flags, cost, energy in metadata:self.put(TYPES + index * 500 + 0x98, theme);self.put(TYPES + index * 500 + 0xec, flags);self.mu.mem_write(TYPES + index * 500 + 0xc4, struct.pack('<f', cost));self.mu.mem_write(TYPES + index * 500 + 0xa0, energy.encode() + b'\0')
        for index, cost in [(85, 200.25), (86, 150.75), (87, 500.5), (88, 300.0)]:self.mu.mem_write(TYPES + index * 500 + 0xc4, struct.pack('<f', cost))
        self.mu.mem_write(BODY, bytes((index * 29 + seed) & 255 for index in range(0xb08)));self.mu.mem_write(BUILDERS, b'\0' * 40)
        for offset, value in [(0x260, 1), (0x284, BUILDERS), (0x288, 10), (0x28c, prior), (0x3b0, prior), (0x394, 85 if prior else 0), (0x390, 3), (0xad8, 7 if prior else 2), (0x2d8, 999)]:self.put(BODY + offset, value)
        if prior:
            self.put(BUILDERS, 99);self.put(BUILDERS + 4, 98);self.put(BODY + 0x398, 1);self.put(BODY + 0x39c, 3)
        profile = bytearray((index * 37 + seed) & 255 for index in range(600));profile[:3 if named else 1] = b'AI\0' if named else b'\0';text = b'T1;T2\0' if tech else b'\0';profile[30:30 + len(text)] = text;struct.pack_into('<I', profile, 0x22c, 777);self.mu.mem_write(TYPES + 0x1f000, bytes(profile))
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<II', STOP, TYPES + 0x1f000));self.mu.reg_write(UC_X86_REG_ECX, BODY);self.mu.emu_start(0x414710, STOP)
        raw = bytes(self.mu.mem_read(BODY, 0xb08)).hex();builders = bytes(self.mu.mem_read(BUILDERS, 40)).hex();players = bytes(self.mu.mem_read(PLAYERS, 1620)).hex();born = ':'.join(bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex() for sid in self.born) or '-'
        return [','.join(map(str, values)), raw, builders, players, born, ';'.join(self.events) or '-']


def cases():
    """정상조건과원본재호출누적state를독립input으로만든다."""
    for pattern in range(8):
        for mission in (0, 1):
            for named in (0, 1):
                for tech in (0, 1):
                    for existing in (0, 1):yield [pattern, mission, named, tech, existing, 0, 19]
    for pattern in range(1, 6):yield [pattern, 1, 1, 0, 1, 2, 93]


def main():
    """고유관찰전체와소스/근거SHA를저장/감사한다."""
    fixture = ROOT / 'cpppj/tests/fixtures/ai-initial-update-x86.tsv';report = ROOT / 'cpppj/ai-initial-update.json'
    sources = ['originals/Netstorm.exe', 'extracted/ai-initial-update/originals/creation.c', 'extracted/ai-initial-update/originals/functions.tsv', 'extracted/player-technology/originals/functions.tsv', 'extracted/refined/originals/functions.tsv', 'tools/ghidra/ai-initial-update-functions.json', 'tools/decomp_ai_initial_update.py', 'tools/decomp_player_technology.py', 'tools/decomp_priestmenu.py', 'cpppj/src/o/RawAiBrainInitialUpdate.h', 'cpppj/src/o/RawAiBrainInitialUpdate.cpp', 'cpppj/tests/AiBrainInitialUpdateTests.cpp', 'cpppj/tests/fixtures/ai-initial-update-x86.tsv']
    if '--verify' in sys.argv:
        prior = json.loads(report.read_text(encoding='utf-8'))
        for name, value in prior['sha256'].items():assert digest(ROOT / name) == value, name
        with fixture.open(encoding='utf-8') as stream:rows = list(csv.DictReader(stream, delimiter='\t'))
        assert len(rows) == prior['observations'];assert len({row['input'] for row in rows}) == len(rows);print('AiInitialUpdate 원본고유input/SHA감사통과:', len(rows));return
    oracle = Oracle();rows = [oracle.run(values) for values in cases()];assert len({row[0] for row in rows}) == len(rows)
    with fixture.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['input', 'raw', 'builders', 'players', 'born', 'trace']);writer.writerows(rows)
    facts = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'observations': len(rows), 'boundaries': ['caller-owned0xb08/builder-array/profile/contained input state', 'default language name and existing PlayerWorld Technology/Create wrappers supplied as external state responses', 'PlayerTechnology/contained/finder/workshop-switch/ftol machine bodies real', 'temporary heap/Factory/virtual attachment are prior explicit boundaries; no initial SP/wallet mutation added'], 'sha256': {name: digest(ROOT / name) for name in sources}}
    report.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8');print('AiInitialUpdate 원본고유input:', len(rows))


if __name__ == '__main__':main()
