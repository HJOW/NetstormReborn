#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DataManager 검색/선택의 원본 두 본문과 현재 포인터 재조회를 독립 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, NODES, STRIDE = IMAGE + 8, IMAGE + 0x100, 76  # 같은 manager12BYTE/노드60BYTE와 양쪽 보호8BYTE다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-selection-x86.tsv'  # 최초 관측 뒤 재생성하지 않는다.
REPORT = ROOT / 'cpppj/datamanager-selection-native.json'  # 원본/도구/관측 자료의 바이트 식별자다.


class SelectionOracle(Oracle):
    """검색44ce60과 선택44ceb0만 실행하며 CRT 비교와 진단만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 함수표의 정확한 본문 범위를 읽고 범위 밖 쓰기를 거부한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.entries = {0x44ce60: 0, 0x44ceb0: 0}
        self.allowed = []
        # 원본 두 본문은 공용 Oracle의 대체 호출 없이 직접 실행한다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """선택되지 않은 노드와 보호 바이트도 원문 그대로 기록한다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 28)) + bytes(self.mu.mem_read(NODES, STRIDE * 3))

    def write(self, mu, access, address, size, value, data):
        """두 본문이 쓰는 current/pos와 격리 스택 이외 쓰기를 거부한다."""
        valid = MANAGER + 8 <= address and address + size <= MANAGER + 12
        # 모든 노드의 pos만 원본이 직접 변경할 수 있다.
        for index in range(3):
            node = NODES + index * STRIDE + 8
            valid |= node + 4 <= address and address + size <= node + 8
        assert valid or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def instruction(self, mu, address, size, data):
        """비교/진단 뒤 current를 바꾸는 명시 입력도 같은 원본 재조회로 관측한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e65ed:
            first, second = self.words(esp + 4, 2)
            left, right = self.string(first), self.string(second)
            assert left.isascii() and right.isascii()
            self.events.append('C,' + right.hex() + ',' + self.snapshot().hex())
            self.comparisons += 1
            if self.variant in (1, 2) and self.comparisons == 1:
                self.put(MANAGER + 8, NODES + 2 * STRIDE + 8)
            self.ret(0 if self.variant == 1 and self.comparisons == 1 else (left.lower() > right.lower()) - (left.lower() < right.lower()))
            return
        if address == 0x4e0620:
            line = self.words(esp + 12)[0]
            self.events.append('A,' + str(line) + ',' + self.snapshot().hex())
            if line == 0x68 or self.variant == 3:
                self.put(MANAGER + 8, NODES + 2 * STRIDE + 8)
            self.ret()
            return
        if address in self.entries:
            self.entries[address] += 1
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1
        assert self.instructions < 1000

    def observe(self, inputs):
        """목록 순서/대소문자/빈 버퍼/쓰기 허용과 외부 경계 변이를 교차한다."""
        kind, count, reverse, query, empty, writable, self.variant = inputs
        self.events, self.instructions, self.comparisons = [], 0, 0
        self.mu.mem_write(MANAGER - 8, bytes((i * 13 + 7) & 255 for i in range(28)))
        self.mu.mem_write(NODES, bytes((i * 17 + 3) & 255 for i in range(STRIDE * 3)))
        order = list(range(count))
        if reverse:
            order.reverse()
        self.put(MANAGER + 4, NODES + order[0] * STRIDE + 8 if order else 0)
        names = (b'State', b'Territory', b'sTaTe')
        # 데이터 주소는 불투명 입력이며 해당 버퍼 본문을 실행하거나 읽지 않는다.
        for index in range(3):
            node = NODES + index * STRIDE + 8
            self.put(node, 0 if empty == 1 else 0x12200000 + index * 64)
            self.put(node + 8, 0 if empty == 2 else 28)
            self.mu.mem_write(node + 16, names[index] + b'\0')
            self.put(node + 0x38, 0)
        # 중복 이름은 실제 목록 순서대로 첫 일치를 선택해야 한다.
        for position, index in enumerate(order):
            self.put(NODES + index * STRIDE + 8 + 0x38, NODES + order[position + 1] * STRIDE + 8 if position + 1 < count else 0)
        name = (b'sTaTe', b'TERRITORY', b'missing')[query]
        self.mu.mem_write(TEXT, name + b'\0')
        before = self.snapshot()
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, MANAGER)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, TEXT, writable))
        self.mu.emu_start(0x44ce60 if kind == 'F' else 0x44ceb0, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + (8 if kind == 'F' else 12)
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        return [','.join(map(str, inputs)), before.hex(), self.snapshot().hex(), str(self.mu.reg_read(UC_X86_REG_EAX)), ';'.join(self.events) or '-']


def main():
    """제품 결과 없이 최초 원문을 한 번 보존하거나 이미 보존한 원문의 해시만 확인한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, value in report['hashes'].items():
            assert digest(ROOT / path) == value, path
        assert sum(1 for _ in csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t')) - 1 == report['rows']
        print(json.dumps({'verified': report['rows']}, ensure_ascii=False))
        return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = SelectionOracle()
    rows, instructions = [], 0
    # 각 입력은 다른 입력의 결과/제품 구현과 독립적이다.
    for kind in ('F', 'S'):
        for count in (0, 1, 3):
            for reverse in (0, 1):
                for query in range(3):
                    for empty in range(3):
                        for writable in (0, 1):
                            for variant in range(4):
                                rows.append(oracle.observe([kind, count, reverse, query, empty, writable, variant]))
                                instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(['input', 'before', 'after', 'eax', 'events'])
        writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_datamanager_selection.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'instructions': instructions, 'entries': {hex(k): v for k, v in oracle.entries.items()}, 'limitations': ['CRT stricmp는 ASCII 섹션 이름 비교의 명시 경계다.', '진단은 기록/명시 current 수리 입력이며 실제 UI/CRT/OS/게임 실행이 아니다.', '누락 current가 진단 뒤에도 0인 잘못된 메모리 경로는 정상 반환 관측에 포함하지 않는다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
