#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""MemStream 할당/쓰기/압축과 DataManager 추가/제거의 전체 하위 명령을 독립 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, NODES, BUFFERS, SOURCE = IMAGE + 8, IMAGE + 0x100, HEAP + 0x100, TEXT + 0x100  # 격리 manager/노드/버퍼/복사 입력이다.
NODE_STRIDE, BUFFER_STRIDE = 76, 80  # 각60/64BYTE 자료의 양쪽8BYTE 보호 영역을 포함한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-streams-x86.tsv'  # 최초 관측을 다시 생성하지 않는다.
REPORT = ROOT / 'cpppj/datamanager-streams-native.json'  # 원본과 처음 실행 도구/자료의 전체 바이트 식별자다.


class StreamOracle(Oracle):
    """여섯 실제 본문은 직접 실행하고 operator new/free와 진단만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 함수표의 전체 범위를 읽고 격리 메모리 쓰기를 제한한다."""
        super().__init__()
        self.entries = {entry: 0 for entry in (0x44cc20, 0x44cce0, 0x44cd40, 0x44ce10, 0x44d040, 0x44d0f0)}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        # 공용 실행기의 대체 호출로 중첩 압축/할당/쓰기를 가로채지 않는다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """전체716BYTE에서 선택되지 않은 노드/버퍼·이름·링크·보호 바이트도 기록한다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 28)) + bytes(self.mu.mem_read(NODES, 4 * NODE_STRIDE)) + bytes(self.mu.mem_read(BUFFERS, 4 * BUFFER_STRIDE)) + bytes(self.mu.mem_read(SOURCE, 64))

    def write(self, mu, access, address, size, value, data):
        """원본이 쓰는 manager head/current·stream16BYTE/next·버퍼64BYTE만 허용한다."""
        valid = MANAGER + 4 <= address and address + size <= MANAGER + 12
        # 이름40BYTE와 각 노드의 양쪽 보호 바이트는 수정할 수 없다.
        for index in range(4):
            node = NODES + index * NODE_STRIDE + 8
            valid |= node <= address and address + size <= node + 16
            valid |= node + 0x38 <= address and address + size <= node + 0x3c
            buffer = BUFFERS + index * BUFFER_STRIDE + 8
            valid |= buffer <= address and address + size <= buffer + 64
        assert valid or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def event(self, kind, value):
        """외부 경계가 메모리를 바꾸기 전의 현재 전체 메모리를 보존한다."""
        self.events.append(f'{kind},{value},{self.snapshot().hex()}')

    def instruction(self, mu, address, size, data):
        """힙 효과/진단 뒤 재조회는 실제 원본 명령으로 수행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e4391:
            length = self.words(esp + 4)[0];assert length <= 64;self.event('A', length)
            if self.variant == 1:
                self.put(self.allocating + 12, length // 2);self.put(self.allocating + 4, 0x87654321)
            self.ret(BUFFERS + 3 * BUFFER_STRIDE + 8);return
        if address == 0x4e4354:
            target = self.words(esp + 4)[0];is_node = NODES <= target < NODES + 4 * NODE_STRIDE
            self.event('N' if is_node else 'F', target)
            if self.variant == 2 and not is_node:
                self.put(self.selected + 4, 0x87654321);self.put(self.selected + 8, 0x1234);self.put(MANAGER + 8, 0x456789ab)
            self.ret();return
        if address == 0x4e0620:
            line = self.words(esp + 12)[0];self.event('D', line)
            if self.kind == 'W' and self.variant == 1:
                self.put(self.selected + 4, 5)
            self.ret();return
        if address in self.entries:
            self.entries[address] += 1
        if address == 0x44cc20:
            self.allocating = mu.reg_read(UC_X86_REG_ECX)
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1;assert self.instructions < 10000

    def observe(self, inputs):
        """각 본문의 유효 입력과 외부 효과 뒤 재조회를 같은 메모리에서 관측한다."""
        self.kind, seed, count, select, present, end, allocation, position, length, self.variant = inputs
        self.events, self.instructions = [], 0
        self.mu.mem_write(MANAGER - 8, bytes((index * 13 + seed) & 255 for index in range(28)))
        self.mu.mem_write(NODES, bytes((index * 17 + seed) & 255 for index in range(4 * NODE_STRIDE)))
        self.mu.mem_write(BUFFERS, bytes((index * 19 + seed) & 255 for index in range(4 * BUFFER_STRIDE)))
        self.mu.mem_write(SOURCE, bytes((index * 23 + seed) & 255 for index in range(64)))
        self.put(MANAGER + 4, NODES + 8 if count else 0)
        # 목록 밖 네 번째 노드는 추가 인자로만 사용하며 나머지 원문도 보존한다.
        for index in range(4):
            node = NODES + index * NODE_STRIDE + 8
            for field, value in enumerate((BUFFERS + index * BUFFER_STRIDE + 8 if present else 0, position, end, allocation)):
                self.put(node + field * 4, value)
            self.put(node + 0x38, NODES + (index + 1) * NODE_STRIDE + 8 if index + 1 < count else 0)
        self.selected = NODES + (count - 1 if self.kind == 'P' and count else select) * NODE_STRIDE + 8
        entry = {'A': 0x44cc20, 'W': 0x44cce0, 'C': 0x44cd40, 'P': 0x44ce10, 'R': 0x44d0f0}[self.kind]
        source = BUFFERS + 9 if self.variant == 3 and self.kind == 'W' else SOURCE
        arguments = {'A': [length], 'W': [source, length], 'C': [], 'P': [NODES + 3 * NODE_STRIDE + 8 if select else 0], 'R': [self.selected]}[self.kind]
        before = self.snapshot()
        # ABI는 모든 정상 반환에서 같은 보존 레지스터/제어어/정리 폭으로 확인한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, MANAGER if self.kind in ('P', 'R') else self.selected)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1), STOP, *arguments));self.mu.emu_start(entry, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + len(arguments) * 4 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        # EAX는 void 본문의 미정 값이므로 결과 계약에 넣지 않는다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        return [','.join(map(str, inputs)), before.hex(), self.snapshot().hex(), ';'.join(self.events) or '-']


def cases():
    """정수 폭·빈 자료·DWORD/끝 복사·겹침·링크 위치와 경계 변이를 교차한다."""
    # 초기화는 data 비영 진단 뒤 계속하는 경로와 allocator 뒤 alloc 재조회도 포함한다.
    for seed in (0, 255):
        for length in (0, 1, 3, 4, 7, 16):
            for present in (0, 1):
                for variant in (0, 1):
                    yield ['A', seed, 1, 0, present, 8, 16, 3, length, variant]
    # 쓰기는 진단 뒤 pos 재조회와 원본 DWORD 단위의 앞쪽부터 겹치는 복사도 관측한다.
    for seed in (0, 255):
        for length in (0, 1, 3, 4, 7, 8, 16):
            for position in (0, 3):
                for allocation in (1, 32):
                    for variant in (0, 1, 3):
                        yield ['W', seed, 1, 0, 1, 7, allocation, position, length, variant]
    # 압축은 양의 길이·빈 자료·음수 end와 할당/해제 뒤의 현재 필드를 구별한다.
    for seed in (0, 255):
        for present, end, allocation in ((1, 0, 0), (1, 0, 16), (0, 0, 16), (1, 3, 3), (1, 3, 16), (1, 8, 3), (1, 8, 16), (1, 0xffffffff, 16)):
            for variant in (0, 1, 2):
                yield ['C', seed, 1, 0, present, end, allocation, 5, 0, variant]
    # 추가는 빈/비어 있지 않은 head와 NULL 새 노드도 같은 원본 처리로 관측한다.
    for seed in (0, 255):
        for count in (0, 1, 3):
            for end in (0, 3, 8):
                for select in (0, 1):
                    for variant in (0, 1, 2):
                        yield ['P', seed, count, select, 1, end, 16, 5, 0, variant]
    # 제거는 실제 첫/중간/마지막 노드를 사용하며 없는 노드의 잘못된 접근을 정상 예로 다루지 않는다.
    for seed in (0, 255):
        for count, select in ((1, 0), (3, 0), (3, 1), (3, 2)):
            for present in (0, 1):
                for variant in (0, 2):
                    yield ['R', seed, count, select, present, 8, 16, 5, 0, variant]


def main():
    """최초 전체 관측을 한 번 보존하고 --verify에서는 SHA와 행 수만 읽는다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 저장한 전체 바이트를 변환 없이 대조한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        print(json.dumps({'verified': report['rows']}));return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = StreamOracle();rows, instructions = [], 0
    # 제품의 결과/분기 없이 명시 입력을 각각 독립 실행한다.
    for inputs in cases():
        rows.append(oracle.observe(inputs));instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['# input', 'before', 'after', 'events']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_datamanager_streams.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'instructions': instructions, 'entries': {hex(key): value for key, value in oracle.entries.items()}, 'limitations': ['본문44cc20/44cce0/44cd40/44ce10/44d040/44d0f0은 직접 실행한다. new/free/진단만 명시 경계다.', '실제 CRT/힙/OS/게임 실행이 아니다. void EAX는 결과 계약에 포함하지 않는다.', '유효한 기존 노드의 제거만 관측한다. 없는 노드의 잘못된 메모리 경로와 OOM/예외 경로는 미검증이다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}))


if __name__ == '__main__':
    main()
