#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""섹션 생성/복제 부모와 MemStream 대입의 실제 명령·전체 할당 바이트를 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, SOURCE, NODES, BUFFERS, LARGE, NAMES = IMAGE + 8, IMAGE + 0x48, IMAGE + 0x100, HEAP + 0x100, HEAP + 0x10000, TEXT + 0x100  # 실제 OS/힙과 분리한 주소다.
NODE_STRIDE, BUFFER_STRIDE = 76, 80  # 실제60/64BYTE의 양쪽8BYTE 보호 범위다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-create-x86.tsv'  # 최초 성공한 원문은 덮지 않는다.
REPORT = ROOT / 'cpppj/datamanager-create-native.json'  # 전체 원문과 당시 도구의 식별자다.


def encode(data):
    """8개 이상 같은 바이트만 무손실 반복 표기로 저장하며 전체 바이트를 생략하지 않는다."""
    result, pending, index = [], bytearray(), 0
    # 16KiB 할당의 같은 바이트 반복도 양쪽 보호 바이트와 함께 복원할 수 있다.
    while index < len(data):
        end = index + 1
        while end < len(data) and data[end] == data[index]:
            end += 1
        if end - index >= 8:
            result.extend((pending.hex(), f'~{data[index]:02x}x{end-index}~'));pending.clear()
        else:
            pending.extend(data[index:end])
        index = end
    result.append(pending.hex());return ''.join(result)


def decode(text):
    """관측용 반복 표기를 원래 전체 바이트로 되돌린다."""
    result = bytearray()
    # 홀수 조각만 BYTE값/개수이고 짝수 조각은 일반16진 문자열이다.
    for index, part in enumerate(text.split('~')):
        if index % 2:
            value, count = part.split('x');result.extend(bytes.fromhex(value) * int(count))
        else:
            result.extend(bytes.fromhex(part))
    return bytes(result)


class CreateOracle(Oracle):
    """생성/복제와 모든 하위를 실행하며 비교/new/free/진단만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 전체 정밀 함수표의 열한 본문에만 실행을 허용한다."""
        super().__init__()
        self.entries = {entry: 0 for entry in (0x44cc20, 0x44cc80, 0x44cce0, 0x44cd40, 0x44ce10, 0x44ce60, 0x44d040, 0x44d070, 0x44d0f0, 0x44d1b0, 0x44d230)}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        # 부모 호출의 자식들을 공용 Oracle의 대체 반환으로 가로채지 않는다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """두 manager/다섯 노드/여섯 작은 버퍼/전체16KiB 버퍼/이름의17380BYTE다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 28)) + bytes(self.mu.mem_read(SOURCE - 8, 28)) + bytes(self.mu.mem_read(NODES, 5 * NODE_STRIDE)) + bytes(self.mu.mem_read(BUFFERS, 6 * BUFFER_STRIDE)) + bytes(self.mu.mem_read(LARGE, 0x4010)) + bytes(self.mu.mem_read(NAMES, 64))

    def write(self, mu, access, address, size, value, data):
        """실제 본문의 필드/물리 할당/SEH 정상 쓰기만 허용한다."""
        valid = MANAGER + 4 <= address and address + size <= MANAGER + 12 or SOURCE + 4 <= address and address + size <= SOURCE + 12
        # 기존 노드 이름은 보존하고 새 노드 이름40BYTE만 허용한다.
        for index in range(5):
            node = NODES + index * NODE_STRIDE + 8
            valid |= node <= address and address + size <= node + 16 or node + 56 <= address and address + size <= node + 60
            if index == 4:
                valid |= node + 16 <= address and address + size <= node + 56
        # 작은 할당과 실제16384BYTE 전체를 양쪽 보호 바이트 안에서만 쓴다.
        for index in range(6):
            buffer = BUFFERS + index * BUFFER_STRIDE + 8;valid |= buffer <= address and address + size <= buffer + 64
        valid |= LARGE + 8 <= address and address + size <= LARGE + 8 + 0x4000
        assert valid or 0 <= address and address + size <= 4 or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def event(self, kind, value):
        """외부 경계 직전 전체 바이트도 같은 무손실 표기로 보존한다."""
        self.events.append(f'{kind},{value},{encode(self.snapshot())}')

    def instruction(self, mu, address, size, data):
        """할당/free가 원문과 current를 바꾼 뒤의 재조회는 실제 명령에 맡긴다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e65ed:
            left, right = (self.string(pointer) for pointer in self.words(esp + 4, 2));assert left.isascii() and right.isascii()
            self.event('Q', left.hex() + '/' + right.hex());self.ret((left.lower() > right.lower()) - (left.lower() < right.lower()));return
        if address == 0x4e4391:
            length = self.words(esp + 4)[0]
            if self.kind != 'M' and length == 60 and not self.node_allocated:
                self.event('S', length);self.node_allocated = True;self.ret(NODES + 4 * NODE_STRIDE + 8);return
            self.event('A', length)
            if length == 0x4000:
                pointer = LARGE + 8
            else:
                assert length <= 64 and self.small_allocations < 2;pointer = BUFFERS + (4 + self.small_allocations) * BUFFER_STRIDE + 8;self.small_allocations += 1
            if self.variant == 1 and length == 0x4000 or self.variant == 3 and self.kind == 'M':
                self.put(self.source_node, BUFFERS + 2 * BUFFER_STRIDE + 11);self.put(self.source_node + 8, 2)
            self.ret(pointer);return
        if address == 0x4e4354:
            target = self.words(esp + 4)[0];self.event('N' if NODES <= target < NODES + 5 * NODE_STRIDE else 'F', target)
            if self.variant == 2:
                self.put(MANAGER + 8, 0xdead5678);self.put(self.source_node, BUFFERS + BUFFER_STRIDE + 8);self.put(self.source_node + 8, 3)
            self.ret();return
        if address == 0x4e0620:
            self.event('D', self.words(esp + 12)[0]);raise AssertionError('정상 생성/복제 입력의 예상 밖 진단')
        if address in self.entries:
            self.entries[address] += 1
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1;assert self.instructions < 30000

    def observe(self, inputs):
        """기존 섹션 위치/빈 목록/원문 길이/NULL data/경계 변이로 전체 부모를 관측한다."""
        self.kind, seed, position, end, null_data, self.variant = inputs
        self.events, self.instructions, self.small_allocations, self.node_allocated = [], 0, 0, False
        self.mu.mem_write(MANAGER - 8, bytes((index * 13 + seed) & 255 for index in range(28)))
        self.mu.mem_write(SOURCE - 8, bytes((index * 11 + seed) & 255 for index in range(28)))
        self.mu.mem_write(NODES, bytes((index * 17 + seed) & 255 for index in range(5 * NODE_STRIDE)))
        self.mu.mem_write(BUFFERS, bytes((index * 19 + seed) & 255 for index in range(6 * BUFFER_STRIDE)))
        self.mu.mem_write(LARGE, bytes([seed ^ 0x65]) * 0x4010);self.mu.mem_write(NAMES, bytes((index * 23 + seed) & 255 for index in range(64)));self.put(0, 0)
        self.put(MANAGER + 4, 0 if position == -2 else NODES + 8);self.put(SOURCE + 4, NODES + 3 * NODE_STRIDE + 8);self.source_node = NODES + 3 * NODE_STRIDE + 8
        # 기존 세 대상 노드와 다른 manager의 원문 노드를 독립 구성한다.
        for index in range(5):
            node = NODES + index * NODE_STRIDE + 8
            if index < 4 or self.kind == 'M':
                for field, value in enumerate((0 if index == 3 and null_data else BUFFERS + index * BUFFER_STRIDE + 8, 17, end if index in (3, 4) else 7, 64)):
                    self.put(node + field * 4, value)
                name = b'State' if index in (position, 3) else b'Other' + bytes([65 + index]);self.mu.mem_write(node + 16, name + b'\0')
            self.put(node + 56, NODES + (index + 1) * NODE_STRIDE + 8 if index < 2 else 0)
        self.mu.mem_write(NAMES, (b'State' if seed == 0 else b'sTaTe') + b'\0')
        if self.kind == 'N':
            entry, target, arguments = 0x44d1b0, MANAGER, [NAMES]
        elif self.kind == 'C':
            entry, target, arguments = 0x44d230, MANAGER, [SOURCE, NAMES]
        else:
            entry, target, arguments = 0x44cc80, NODES + 4 * NODE_STRIDE + 8, [self.source_node]
        before = self.snapshot();assert len(before) == 17380
        # 보존 레지스터/x87/스택/정상 SEH 복구를 모든 반환에서 검사한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, target);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1), STOP, *arguments));self.mu.emu_start(entry, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 * (len(arguments) + 1) and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f and self.words(0)[0] == 0
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        after = self.snapshot();assert decode(encode(before)) == before and decode(encode(after)) == after
        return [','.join(map(str, inputs)), encode(before), encode(after), str(self.mu.reg_read(UC_X86_REG_EAX)) if self.kind != 'M' else '-', ';'.join(self.events) or '-']


def cases():
    """빈 목록/이름 부재/처음·중간·끝 교체와 원문0~32BYTE 경계를 교차한다."""
    # 생성자는 항상16384BYTE를 할당하며 마지막 원문 압축과 구별한다.
    for seed in (0, 255):
        for position in (-2, -1, 0, 1, 2):
            for variant in (0, 1, 2):
                yield ['N', seed, position, 7, 0, variant]
    # 복제는 source.data==0일 때 대입을 건너뛰고 생성된 큰 버퍼를 유지한다.
    for seed in (0, 255):
        for position in (-2, -1, 0, 1, 2):
            for end in (0, 1, 7, 28, 32):
                for variant in (0, 1, 2):
                    yield ['C', seed, position, end, 0, variant]
            yield ['C', seed, position, 0, 1, 0]
    # 독립 대입의 free 뒤 원문 재조회/할당 뒤 길이·data 재조회를 분리한다.
    for seed in (0, 255):
        for end in (0, 1, 7, 28, 32):
            for variant in (0, 2, 3):
                yield ['M', seed, -1, end, 0, variant]


def main():
    """제품 결과 없이 최초 원문을 한 번 쓰거나 보존된 전체 파일SHA만 검증한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        print(json.dumps({'verified': report['rows']}));return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = CreateOracle();rows, instructions = [], 0
    # 각 입력을 원래 PE에서 독립 실행하며 C++ 제품을 호출하지 않는다.
    for inputs in cases():
        rows.append(oracle.observe(inputs));instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['# input', 'before-rle', 'after-rle', 'eax-or-void', 'events-rle']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_datamanager_create.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'snapshot_bytes': 17380, 'instructions': instructions, 'entries': {hex(key): value for key, value in oracle.entries.items()}, 'limitations': ['열한 실제 본문을 직접 실행하며 CRT ASCII 비교/new/free/진단만 명시 경계다.', 'void 대입의 미정 EAX는 반환 계약에 넣지 않는다. 모든 정상 반환은 ABI/보존 레지스터/x87/FS0 복구를 확인한다.', '실제 CRT/힙/OS/게임/Windows 예외 실행은 아니다. OOM과잘못된 포인터는 정상 관측에 포함하지 않는다.', '반복 바이트도 무손실 표기로 전체 저장하며 실제16384BYTE와 양쪽 보호 바이트를 빠짐없이 비교한다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}))


if __name__ == '__main__':
    main()
