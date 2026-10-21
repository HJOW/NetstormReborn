#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""State 최소 확장·임시 stream 생성·이름 교체와 모든 MemStream/목록 하위를 독립 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, NODES, BUFFERS, NAMES = IMAGE + 8, IMAGE + 0x100, HEAP + 0x100, TEXT + 0x100  # 실제 OS/힙과 구별한 격리 주소다.
NODE_STRIDE, BUFFER_STRIDE = 76, 80  # 각 노드/버퍼의 양쪽8BYTE를 포함한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-resize-x86.tsv'  # 최초 전체 관측은 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/datamanager-resize-native.json'  # 원본/실행 도구/최초 자료의 바이트 식별자다.


class ResizeOracle(Oracle):
    """부모와 열 개 실제 본문을 실행하고 CRT 비교/new/free/진단만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 전체 함수표의 범위를 제한하며 SEH 최종 복구도 확인한다."""
        super().__init__()
        self.entries = {entry: 0 for entry in (0x44cc20, 0x44cce0, 0x44cd40, 0x44ce10, 0x44d040, 0x44d0f0, 0x44ce60, 0x44d070, 0x44d160, 0x44d290)}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        # 공용 실행기의 대체 호출로 자식 생성/복사/삭제를 가로채지 않는다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """전체796BYTE에서 모든 노드/버퍼와 보호 바이트·실제 이름 인자를 보존한다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 28)) + bytes(self.mu.mem_read(NODES, 4 * NODE_STRIDE)) + bytes(self.mu.mem_read(BUFFERS, 5 * BUFFER_STRIDE)) + bytes(self.mu.mem_read(NAMES, 64))

    def write(self, mu, access, address, size, value, data):
        """노드/버퍼의 원본 쓰기와 SEH/격리 스택만 허용한다."""
        valid = MANAGER + 4 <= address and address + size <= MANAGER + 12
        # 기존 노드의 이름은 보존하고 새/이름 교체 노드만 이름40BYTE 쓰기를 허용한다.
        for index in range(4):
            node = NODES + index * NODE_STRIDE + 8
            valid |= node <= address and address + size <= node + 16
            valid |= node + 0x38 <= address and address + size <= node + 0x3c
            if index == 3:
                valid |= node + 16 <= address and address + size <= node + 56
        # 원본 생성/복사/압축은 할당한64BYTE 물리 범위 밖을 쓸 수 없다.
        for index in range(5):
            buffer = BUFFERS + index * BUFFER_STRIDE + 8
            valid |= buffer <= address and address + size <= buffer + 64
        assert valid or 0 <= address and address + size <= 4 or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def event(self, kind, value):
        """명시 경계 효과 직전의 전체 메모리를 기록한다."""
        self.events.append(f'{kind},{value},{self.snapshot().hex()}')

    def instruction(self, mu, address, size, data):
        """외부 효과 뒤의 원문/길이/현재 포인터 재조회는 원본 명령에 맡긴다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e65ed:
            left, right = (self.string(pointer) for pointer in self.words(esp + 4, 2));assert left.isascii() and right.isascii()
            self.event('Q', left.hex() + '/' + right.hex());self.ret((left.lower() > right.lower()) - (left.lower() < right.lower()));return
        if address == 0x4e4391:
            length = self.words(esp + 4)[0]
            if self.kind == 'G' and length == 60 and not self.node_allocated:
                self.event('S', length);self.node_allocated = True;self.ret(NODES + 3 * NODE_STRIDE + 8);return
            assert length <= 64 and self.allocations < 2;self.event('A', length)
            pointer = BUFFERS + (3 + self.allocations) * BUFFER_STRIDE + 8
            if self.variant == 1 and self.kind == 'G' and not self.allocations:
                self.put(self.source_node, BUFFERS + ((self.state_position + 1) % 3) * BUFFER_STRIDE + 8);self.put(self.source_node + 8, 2);self.put(MANAGER + 8, 0xdead1234)
            if self.variant == 3 and not self.allocations:
                self.put(NODES + 3 * NODE_STRIDE + 8 + 12, length + 1)
            self.allocations += 1;self.ret(pointer);return
        if address == 0x4e4354:
            target = self.words(esp + 4)[0];self.event('N' if NODES <= target < NODES + 4 * NODE_STRIDE else 'F', target)
            if self.variant == 2:
                self.put(MANAGER + 8, 0xdead5678)
            self.ret();return
        if address == 0x4e0620:
            self.event('D', self.words(esp + 12)[0]);raise AssertionError('정상 입력의 예상 밖 원본 진단')
        if address in self.entries:
            self.entries[address] += 1
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1;assert self.instructions < 20000

    def observe(self, inputs):
        """현재 State 위치/길이와 이름/크기/힙 효과 변이를 같은 원본 부모에서 관측한다."""
        self.kind, seed, self.state_position, end, minimum, name_kind, self.variant = inputs
        self.events, self.instructions, self.allocations, self.node_allocated = [], 0, 0, False
        self.mu.mem_write(MANAGER - 8, bytes((index * 13 + seed) & 255 for index in range(28)))
        self.mu.mem_write(NODES, bytes((index * 17 + seed) & 255 for index in range(4 * NODE_STRIDE)))
        self.mu.mem_write(BUFFERS, bytes((index * 19 + seed) & 255 for index in range(5 * BUFFER_STRIDE)))
        self.mu.mem_write(NAMES, bytes((index * 23 + seed) & 255 for index in range(64)));self.put(0, 0)
        self.put(MANAGER + 4, NODES + 8);self.source_node = NODES + self.state_position * NODE_STRIDE + 8
        # 실제 목록에 있는 기존 세 노드와 선택적 네 번째 임시 노드를 구성한다.
        for index in range(4):
            node = NODES + index * NODE_STRIDE + 8
            if index < 3 or self.kind == 'R':
                for field, value in enumerate((BUFFERS + index * BUFFER_STRIDE + 8, 17, end if index == self.state_position else 7, 64)):
                    self.put(node + field * 4, value)
                name = b'State' if index == self.state_position else b'T$e$m$p' if index == 3 else b'Other' + bytes([65 + index])
                self.mu.mem_write(node + 16, name + b'\0')
            self.put(node + 0x38, NODES + (index + 1) * NODE_STRIDE + 8 if index + 1 < (4 if self.kind == 'R' else 3) else 0)
        desired = (b'State', b'sTaTe', b'NewName', b'')[name_kind]
        self.mu.mem_write(NAMES, desired + b'\0');self.mu.mem_write(NAMES + 32, b'T$e$m$p\0')
        if self.kind == 'G':
            entry, target, arguments = 0x44d290, MANAGER, [NAMES, minimum & 0xffffffff]
        elif self.kind == 'R':
            entry, target, arguments = 0x44d160, MANAGER, [NAMES, NAMES + 32]
        else:
            entry, target, arguments = 0x44d070, NODES + 3 * NODE_STRIDE + 8, [0 if name_kind == 2 else NAMES + 32 if name_kind == 1 else NAMES, minimum & 0xffffffff]
        before = self.snapshot()
        # 모든 정상 반환의 ABI/보존 레지스터/x87/SEH 복구를 확인한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, target);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, *arguments));self.mu.emu_start(entry, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 12 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f and self.words(0)[0] == 0
        # 원본 본문이 보존해야 하는 네 레지스터만 반환 계약에 포함한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        return [','.join(map(str, inputs)), before.hex(), self.snapshot().hex(), str(self.mu.reg_read(UC_X86_REG_EAX)), ';'.join(self.events) or '-']


def cases():
    """실제28BYTE 확장과 signed 최소 크기·독립 이름 교체/생성을 교차한다."""
    # State가 처음/중간/끝인 경우와 기존 길이의 최소 경계를 관측한다.
    for seed in (0, 255):
        for position in range(3):
            for end in (0, 1, 3, 8, 27, 28, 32):
                for variant in range(4):
                    yield ['G', seed, position, end, 28, 0, variant]
    # 음수/0/이미 충분한 양의 최소 크기는 새 노드를 만들지 않아야 한다.
    for seed in (0, 255):
        for minimum in (-1, 0, 1):
            yield ['G', seed, 0, 3, minimum, 0, 0]
    # 기존 대상 이름의 제거와 없는 이름/빈 이름의 원문 복사를 별도로 확인한다.
    for seed in (0, 255):
        for position in (0, 2):
            for name in range(4):
                for variant in (0, 2):
                    yield ['R', seed, position, 7, 0, name, variant]
    # 생성자의 NULL/빈/일반/임시 이름과 최소 할당1 보정을 관측한다.
    for seed in (0, 255):
        for minimum in (-1, 0, 1, 3, 28):
            for name in range(4):
                for variant in (0, 3):
                    yield ['C', seed, 0, 7, minimum, name, variant]


def main():
    """최초 관측을 한 번 보존하거나 보존한 원문의 SHA만 확인한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본/처음 도구/fixture는 관측 당시 같은 바이트여야 한다.
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        print(json.dumps({'verified': report['rows']}));return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = ResizeOracle();rows, instructions = [], 0
    # 제품 결과 없이 각 명시 입력을 독립적으로 실행한다.
    for inputs in cases():
        rows.append(oracle.observe(inputs));instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['# input', 'before', 'after', 'eax', 'events']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_datamanager_resize.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'instructions': instructions, 'entries': {hex(key): value for key, value in oracle.entries.items()}, 'limitations': ['기존 열 개 본문을 직접 실행하며 CRT ASCII 비교/new/free/진단만 명시 경계다.', '실제 CRT/힙/OS/게임 실행이 아니다. OOM/예외·잘못된 노드/물리 범위는 정상 관측에 포함하지 않는다.', 'SEH는 격리 FS0의 정상 반환 복구만 확인하며 실제 Windows 예외 실행은 아니다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}))


if __name__ == '__main__':
    main()
