#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Player 기술/40개 목록과 기존 contained 조회·생성 명령을 격리 대조한다. 게임 실행은 없다."""
import csv
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 원본 주소와 겹치지 않는 목록 포인터/저장소 및 진단용 가상표다.
LIST, VALUES, VTABLE = TYPES + 0x1e000, TYPES + 0x1e100, POOL + 0x10000
# 명시 fixture 타입 입력은48개 이름/그룹이며 기대 목록은 원본이 직접 계산한다.
COUNT = 48


class Oracle(MemoryOracle):
    """정밀 선택5함수와 이미 복원한 contained 실제 기계어만 허용한다."""
    def __init__(self):
        """현재 PE/정밀범위를 읽고 SEH용 격리0페이지를 추가한다."""
        super().__init__();self.allowed = [];self.mu.mem_map(0, 0x1000)
        for path, selected in [('extracted/player-technology/originals/functions.tsv', None), ('extracted/refined/originals/functions.tsv', {0x4ac9f0, 0x4b2270, 0x4b1b20, 0x44daa0, 0x4afa70})]:
            with (ROOT / path).open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    if selected is not None and int(row['entry'], 16) not in selected:continue
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """타입조회/힙/부착/가상삭제 경계만 관찰하며 기술 판단은 실제 명령으로 실행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);ecx = mu.reg_read(UC_X86_REG_ECX);args = self.words(esp + 4, 8)
        if address == 0x4e0620:
            self.events.append('A,' + str(args[2]));self.ret()
        elif address == 0x4411e0:
            if self.kind == 'stalled':
                self.item_calls[args[1]] = self.item_calls.get(args[1], 0) + 1
                if self.item_calls[args[1]] > 2:self.stalled_index = args[1];mu.emu_stop();return
            parts = self.string(args[0]).decode().split(';');value = parts[args[1]] if args[1] < len(parts) else '';mu.mem_write(TEXT + 0x1000, value.encode() + b'\0');self.ret(TEXT + 0x1000)
        elif address == 0x49a860:
            name = self.string(args[0]).decode();self.events.append('F,' + name.encode().hex());value = next((index for index in range(COUNT) if ('T' + str(index)).lower() == name.lower()), -1);self.ret(value)
        elif address == 0x49a840:self.ret(TYPES + args[0] * 500)
        elif address == 0x4e65ed:
            first, second = self.string(args[0]).lower(), self.string(args[1]).lower();self.ret((first > second) - (first < second))
        elif address == 0x4e4391:
            assert args[0] == 160;self.ret(VALUES)
        elif address == 0x4e4354:
            assert args[0] == VALUES;self.ret()
        elif address == 0x4af530:
            assert args[0] == 6 and args[1] == 2;self.events.append('C,6,2');raw = POOL + self.next * 50;self.next += 1;mu.mem_write(raw, b'\0' * 50);self.put(raw, 0x512208);self.put(raw + 10, 6, 1);self.put(raw + 11, 4, 1);self.ret(raw)
        elif address in (STOP + 0x200, 0x4aec10):
            sid = (ecx - POOL) // 50;self.events.append('B,' + str(sid) + ',' + str(args[0]) + ',' + str(args[1]));parent = POOL + args[0] * 50;head = self.words(parent + 6, 1)[0] & 0xffff;self.put(ecx + 4, head, 2);self.put(parent + 6, sid, 2);self.ret(purge=8)
        elif address == STOP + 0x300:
            sid = (ecx - POOL) // 50;self.events.append('D,' + str(sid) + ',' + str(args[0]));parent = POOL + 50 * 50;previous = parent + 6;current = int.from_bytes(mu.mem_read(previous, 2), 'little')
            while current and current != sid:previous = POOL + current * 50 + 4;current = int.from_bytes(mu.mem_read(previous, 2), 'little')
            assert current == sid;self.put(previous, int.from_bytes(mu.mem_read(ecx + 4, 2), 'little'), 2)
            if self.mutate:self.put(PLAYERS + self.owner * 180 + 0x9c, 51)
            self.ret(purge=4)
        elif not any(low <= address < high for low, high in self.allowed):raise RuntimeError(f'검토하지않은기술명령 {address:08x}')
        self.instructions += 1
        if self.instructions > 250000:raise RuntimeError('기술명령상한초과')

    def run(self, kind, owner, type_num, known, pattern, mutate, text):
        """입력체인/타입/목록만준비하며기대조회/생성/삭제목록을계산하지않는다."""
        self.kind = kind;self.owner = owner;self.mutate = mutate;self.next = 5;self.events = [];self.instructions = 0;self.item_calls = {};self.stalled_index = None
        self.mu.mem_write(POOL + 50 * 50, b'\0' * (20 * 50));self.mu.mem_write(PLAYERS, b'\0' * 1620);self.put(0x59534c, PLAYERS);self.put(0x5c8464, POOL);self.put(0x595344, known);self.put(0x541348, COUNT);self.put(0x541088, 6);self.put(0x5e4794, 0, 1)
        self.put(PLAYERS + owner * 180 + 0x9c, 50);self.put(POOL + 50 * 50, VTABLE);self.put(POOL + 50 * 50 + 10, 152, 1);self.put(VTABLE + 0x50, STOP + 0x200);self.put(VTABLE + 0x10, STOP + 0x300)
        for index in range(COUNT):self.put(TYPES + index * 500 + 0x9c, 10 if index % 7 == 0 else index % 10)
        if pattern:
            self.put(POOL + 50 * 50 + 6, 60, 2)
            for index in range(4):
                raw = POOL + (60 + index) * 50;self.put(raw, VTABLE);self.put(raw + 10, 6 if index != 1 else 46, 1);self.put(raw + 4, 61 + index if index < 3 else 0, 2);self.put(raw + 18, [1, 2, 2, 7][index], 2);self.put(raw + 20, [4, 4, 1, 4][index], 2)
        self.mu.mem_write(VALUES, b'\0' * 160);self.mu.mem_write(LIST, struct.pack('<III', VALUES, 40, 0));self.mu.mem_write(TEXT, text.encode() + b'\0')
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        entries = {'find': 0x490330, 'clear': 0x4903b0, 'ensure': 0x490420, 'parse': 0x4914e0, 'stalled': 0x4914e0, 'add': 0x4915c0}
        args = [owner, type_num & 0xffffffff] if kind in ('find', 'ensure') else [owner] if kind == 'clear' else [LIST, TEXT] if kind in ('parse', 'stalled') else [owner, TEXT]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args));self.mu.emu_start(entries[kind], STOP)
        count = self.words(LIST + 8, 1)[0];values = self.words(VALUES, count) if kind in ('parse', 'stalled') else ();result = str(self.mu.reg_read(UC_X86_REG_EAX)) if kind == 'find' else str(self.stalled_index) if kind == 'stalled' else '-'
        fields = [owner, type_num, known, pattern, mutate];input_text = ','.join(map(str, fields)) + '|' + text.encode().hex()
        raw = bytes(self.mu.mem_read(POOL + 50 * 50, 50)).hex() + ':' + ':'.join(bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex() for sid in range(60, 64))
        born = ':'.join(bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex() for sid in range(5, self.next)) or '-';parent = self.words(PLAYERS + owner * 180 + 0x9c, 1)[0]
        return [kind, input_text, result, ';'.join(self.events) or '-', ','.join(map(str, values)) or '-', raw, born, str(parent)]


def cases():
    """각외부입력조합을한번씩만관찰한다."""
    for owner in (1, 8):
        for known in (0, 1):
            for pattern in (0, 1):
                for kind in ('find', 'ensure'):
                    for type_num in (-1, 0, 1, 2, 7, 47, 48):yield kind, owner, type_num, known, pattern, 0, ''
                for mutate in (0, 1):yield 'clear', owner, 0, known, pattern, mutate, ''
    texts = ['', 'T1', 't1;T2;T1', 'T1;;T2', 'ALL', 'T1;all;T47', ';T1', ';'.join('T' + str(index) for index in range(1, COUNT)), ';'.join('T1' for _ in range(60))]
    for text in texts:
        yield 'parse', 1, 0, 1, 0, 0, text
        for known in (0, 1):yield 'add', 1, 0, known, 1, 0, text
    for text in ('missing', 'T0', 'T1;missing;T2'):yield 'stalled', 1, 0, 1, 0, 0, text


def main():
    """실제관찰결과와소스/근거SHA를저장하거나재생성없이감사한다."""
    fixture = ROOT / 'cpppj/tests/fixtures/player-technology-x86.tsv';report = ROOT / 'cpppj/player-technology.json'
    sources = ['originals/Netstorm.exe', 'extracted/player-technology/originals/creation.c', 'extracted/player-technology/originals/functions.tsv', 'extracted/refined/originals/functions.tsv', 'tools/ghidra/player-technology-functions.json', 'tools/decomp_player_technology.py', 'tools/decomp_priestmenu.py', 'cpppj/src/o/RawPlayerTechnology.h', 'cpppj/src/o/RawPlayerTechnology.cpp', 'cpppj/tests/PlayerTechnologyTests.cpp', 'cpppj/tests/fixtures/player-technology-x86.tsv']
    if '--verify' in sys.argv:
        prior = json.loads(report.read_text(encoding='utf-8'))
        for name, value in prior['sha256'].items():assert digest(ROOT / name) == value, name
        with fixture.open(encoding='utf-8') as stream:rows = list(csv.DictReader(stream, delimiter='\t'))
        assert len(rows) == prior['observations'];assert len({(row['kind'], row['input']) for row in rows}) == len(rows);print('PlayerTechnology 원본고유입력/SHA감사통과:', len(rows));return
    oracle = Oracle();rows = [oracle.run(*case) for case in cases()];assert len({(row[0], row[1]) for row in rows}) == len(rows)
    with fixture.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['kind', 'input', 'result', 'trace', 'values', 'raw', 'born', 'parent']);writer.writerows(rows)
    facts = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'observations': len(rows), 'kinds': {kind: sum(row[0] == kind for row in rows) for kind in sorted({row[0] for row in rows})}, 'boundaries': ['48-type name/group fixture input; real RiftTypeTable finder binds separately', 'temporary160-byte allocation/free addresses and OOM excluded; host owns40 entries without synthetic N/X trace', 'factory/virtual attach/virtual destroy external effects', 'stalled entries observe two real loop iterations then stop without claiming normal return'], 'sha256': {name: digest(ROOT / name) for name in sources}}
    report.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8');print('PlayerTechnology 원본고유입력:', len(rows))


if __name__ == '__main__':main()
