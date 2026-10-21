#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""State getter의 빈 버퍼 분기와28BYTE 초기화의 실제 원본 명령을 제품 수정 전에 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADER, MANAGER, NODE, BUFFER = IMAGE + 8, IMAGE + 0x40, IMAGE + 0x100, HEAP + 0x100  # 실제 자료와 분리한 격리 포인터다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/player-state-initialization-x86.tsv'  # 최초 관측은 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/player-state-initialization-native.json'  # 원본/관측 도구/입력 전체 바이트 식별자다.


class StateOracle(Oracle):
    """원본4bd550과 MemStream 쓰기44cce0만 실행하며 선택/생성/최소확장은 명시 경계다."""
    def __init__(self):
        """현재 PC 함수표의 두 본문 범위를 제한하고 원본 쓰기를 감시한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.entries = {0x4bd550: 0, 0x44cce0: 0}
        self.allowed = []
        # 직접 진입/중첩 쓰기는 공용 실행기의 대체 호출 없이 실행한다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """원본은 현재 stream pos/end와 대상 버퍼28BYTE 및 격리 스택만 변경할 수 있다."""
        valid = NODE + 4 <= address and address + size <= NODE + 12
        valid |= BUFFER <= address and address + size <= BUFFER + 28
        assert valid or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def instruction(self, mu, address, size, data):
        """빈 State 생성은 경계로 기록하고 원본 초기화/쓰기의 판단을 대체하지 않는다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x44ceb0:
            name, writable = self.words(esp + 4, 2)
            assert self.string(name) == b'State' and writable == 1
            self.events.append('S');self.put(NODE + 4, 0);self.ret(NODE, 8);return
        if address == 0x44d1b0:
            assert self.string(self.words(esp + 4)[0]) == b'State'
            self.events.append('C')
            # 원본 생성자의 세부 힙/목록 처리는 이번 부모 관측의 외부 입력이다.
            for index, value in enumerate((BUFFER, 0, 0, 0x4000)):
                self.put(NODE + index * 4, value)
            self.ret(NODE, 4);return
        if address == 0x44d290:
            name, size = self.words(esp + 4, 2)
            assert self.string(name) == b'State' and size == 28 and self.words(NODE + 8)[0] >= 28
            self.events.append('E');self.ret(NODE, 8);return
        if address in self.entries:
            self.entries[address] += 1
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1
        assert self.instructions < 1000

    def observe(self, seed, pattern, present, end):
        """全256개 flags 입력과 미정 바이트 패턴을 빈/기존 State 조건에 교차한다."""
        self.events, self.instructions = [], 0
        self.mu.mem_write(STACK - 0x100, bytes((seed + (index * 17 if pattern else 0)) & 255 for index in range(0x100)))
        before = bytes(self.mu.mem_read(STACK - 28, 28))
        self.mu.mem_write(BUFFER, bytes((index * 13 + seed) & 255 for index in range(64)))
        self.put(HEADER, MANAGER);self.put(MANAGER + 8, NODE)
        # 버퍼 존재와 end는 원본이 독립적으로 검사하는 두 입력이다.
        for index, value in enumerate((BUFFER if present else 0, 17, end, 0x4000)):
            self.put(NODE + index * 4, value)
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, HEADER);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<I', STOP));self.mu.emu_start(0x4bd550, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        # 부모와 중첩 MemStream 쓰기가 보존해야 하는 레지스터를 확인한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        assert self.mu.reg_read(UC_X86_REG_EAX) == BUFFER
        return [','.join(map(str, (seed, pattern, present, end))), before.hex(), bytes(self.mu.mem_read(STACK - 28, 28)).hex(), bytes(self.mu.mem_read(NODE, 16)).hex(), bytes(self.mu.mem_read(BUFFER, 64)).hex(), ''.join(self.events)]


def main():
    """최초 관측을 한 번 보존하거나 보존한 바이트와 관측 수만 확인한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 최초 원본·도구·fixture는 현재 바이트와 정확히 같아야 한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        print(json.dumps({'verified': report['rows']}, ensure_ascii=False));return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = StateOracle();rows, instructions = [], 0
    # 모든 flags BYTE와 미정 꼬리 패턴은 제품 구현을 사용하지 않고 공급한다.
    for seed in range(256):
        for pattern in (0, 1):
            for present, end in ((0, 0), (0, 28), (1, 0), (1, 28), (1, 32)):
                rows.append(oracle.observe(seed, pattern, present, end));instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(['# input', 'stack_before', 'stack_after', 'stream_after', 'buffer_after', 'events']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_player_state_initialization.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'instructions': instructions, 'entries': {hex(key): value for key, value in oracle.entries.items()}, 'limitations': ['실행 본문은4bd550과44cce0이다. 검색/선택44ceb0, 새 섹션44d1b0, 최소 확장44d290은 명시 경계다.', '생성 뒤 alloc0x4000과 현재 버퍼는 명시 경계 입력이다. 실제 힙/게임/OS 실행이 아니다.', '빈/기존28·32BYTE만 관측한다. 기존 short State 확장은 이전 별도 근거의 범위다.', '스택 미정 바이트는 입력으로 보존하며 원본 보장 초기값으로 해석하지 않는다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
