#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""파일 이미지 섹션 병합 부모와 mark/SNART/생성/제거의 실제 명령을 독립 관측한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle, ROOT, IMAGE, HEAP, TEXT, STACK, STOP, digest
from decomp_datamanager_create import encode, decode
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

MANAGER, STREAM, NODES, BUFFERS, INPUTS, NAMES, VTABLE = IMAGE + 8, IMAGE + 0x48, IMAGE + 0x100, HEAP + 0x100, HEAP + 0x1000, TEXT + 0x100, TEXT + 0x200  # OS와 분리한 실제 관측 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/datamanager-read-x86.tsv'  # 최초 성공 자료를 덮지 않는다.
REPORT = ROOT / 'cpppj/datamanager-read-native.json'  # 원본/최초 도구/fixture의 바이트 식별자다.


class ReadOracle(Oracle):
    """큰 부모의 인라인 복사/검색/압축과 실제 하위 아홉 본문을 실행한다."""
    def __init__(self):
        """같은 PC 정밀 범위 밖은 비교/new/free/이름 표 가상 호출만 허용한다."""
        super().__init__()
        self.entries = {entry: 0 for entry in (0x44cc20, 0x44cce0, 0x44cdb0, 0x44d040, 0x44d070, 0x44d0f0, 0x44d380, 0x4a8800, 0x4a8850)}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        # 호출자 본문에 인라인된 하위도 대체 반환하지 않는다.
        for entry in self.entries:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def snapshot(self):
        """manager/mark20/여섯 노드/일곱 버퍼/두 입력/이름 표의 전체1400BYTE다."""
        return bytes(self.mu.mem_read(MANAGER - 8, 28)) + bytes(self.mu.mem_read(STREAM - 8, 36)) + bytes(self.mu.mem_read(NODES, 6 * 76)) + bytes(self.mu.mem_read(BUFFERS, 7 * 80)) + bytes(self.mu.mem_read(INPUTS, 256)) + bytes(self.mu.mem_read(NAMES, 64))

    def write(self, mu, access, address, size, value, data):
        """입력 바이트/보호 바이트를 바꾸지 않고 실제 필드·SNART 변환·할당 안의 쓰기만 허용한다."""
        valid = MANAGER + 4 <= address and address + size <= MANAGER + 12 or STREAM <= address and address + size <= STREAM + 20 or NAMES <= address and address + size <= NAMES + 64
        # 기존 이름은 보존하고 새 노드의 이름40BYTE만 쓸 수 있다.
        for index in range(6):
            node = NODES + index * 76 + 8;valid |= node <= address and address + size <= node + 16 or node + 56 <= address and address + size <= node + 60
            if index >= 3:
                valid |= node + 16 <= address and address + size <= node + 56
        for index in range(7):
            buffer = BUFFERS + index * 80 + 8;valid |= buffer <= address and address + size <= buffer + 64
        assert valid or 0 <= address and address + size <= 4 or STACK - 0x7f000 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def event(self, kind, value):
        """모든 명시 경계의 효과 직전 전체 바이트를 무손실 보존한다."""
        self.events.append(f'{kind},{value},{encode(self.snapshot())}')

    def instruction(self, mu, address, size, data):
        """할당 뒤 입력data 재조회와 free 뒤 current 보존을 실제 본문으로 확인한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == STOP + 0x100:
            self.event('T', 'names');self.ret(NAMES);return
        if address == 0x4e65ed:
            left, right = (self.string(pointer) for pointer in self.words(esp + 4, 2));assert left.isascii() and right.isascii()
            self.event('Q', left.hex() + '/' + right.hex());self.ret((left.lower() > right.lower()) - (left.lower() < right.lower()));return
        if address == 0x4e4391:
            length = self.words(esp + 4)[0]
            if length == 60:
                self.event('S', length);assert self.created < 3;pointer = NODES + (3 + self.created) * 76 + 8;self.created += 1
            else:
                self.event('A', length);assert length <= 64;index = (self.allocating - NODES - 8) // 76;pointer = BUFFERS + (index if index >= 3 else 6) * 80 + 8
                if self.variant == 1:
                    self.put(STREAM, INPUTS + 128)
            self.ret(pointer);return
        if address == 0x4e4354:
            target = self.words(esp + 4)[0];self.event('N' if NODES <= target < NODES + 6 * 76 else 'F', target)
            if self.variant == 2:
                self.put(MANAGER + 8, 0xdead5678)
            self.ret();return
        if address == 0x4e0620:
            self.event('D', self.words(esp + 12)[0]);raise AssertionError('정상 파일 이미지의 예상 밖 진단')
        if address in self.entries:
            self.entries[address] += 1
        if address == 0x44cc20:
            self.allocating = mu.reg_read(UC_X86_REG_ECX)
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1;assert self.instructions < 20000

    def observe(self, inputs):
        """빈/기존 목록과 빈payload 유지·원문data 변경·미변환/변환된 이름 표를 교차한다."""
        seed, existing, first, middle, last, converted, self.variant = inputs
        self.created, self.instructions, self.events = 0, 0, []
        self.mu.mem_write(MANAGER - 8, bytes((index * 13 + seed) & 255 for index in range(28)));self.put(MANAGER, VTABLE);self.put(VTABLE + 4, STOP + 0x100)
        self.mu.mem_write(STREAM - 8, bytes((index * 11 + seed) & 255 for index in range(36)))
        self.mu.mem_write(NODES, bytes((index * 17 + seed) & 255 for index in range(6 * 76)))
        self.mu.mem_write(BUFFERS, bytes((index * 19 + seed) & 255 for index in range(7 * 80)))
        self.mu.mem_write(INPUTS, bytes((index * 23 + seed) & 255 for index in range(256)));self.mu.mem_write(NAMES, bytes((index * 29 + seed) & 255 for index in range(64)));self.put(0, 0)
        self.put(MANAGER + 4, NODES + 8 if existing else 0)
        # 기존 State/Territory/Oldtail은 새 empty 섹션을 읽어도 유지할 수 있다.
        for index in range(3):
            node = NODES + index * 76 + 8
            for field, value in enumerate((BUFFERS + index * 80 + 8, 17, 7, 64)):
                self.put(node + field * 4, value)
            self.mu.mem_write(node + 16, (b'State', b'Territory', b'Oldtail')[index] + b'\0');self.put(node + 56, NODES + (index + 1) * 76 + 8 if index < 2 else 0)
        image, alternate = bytearray(b'F\0'), bytearray(b'F\0')
        # 같은 길이의 대체 입력은 할당 뒤 다시 읽은 data에서 다른payload를 공급한다.
        for section, length in enumerate((first, middle, last)):
            header = struct.pack('<H', length + 2);image.extend(header);alternate.extend(header)
            image.extend((index * 7 + section * 31 + seed) & 255 for index in range(length));alternate.extend((index * 5 + section * 37 + seed + 1) & 255 for index in range(length))
        self.mu.mem_write(INPUTS, bytes(image));self.mu.mem_write(INPUTS + 128, bytes(alternate))
        for field, value in enumerate((INPUTS, 2, len(image) - (3 if self.variant == 2 else 0), 128, 0xffffffff)):
            self.put(STREAM + field * 4, value)
        names = b'/State/Territory/Last//\0';names = names[:-1].replace(b'/', b'\0') + b'\xff' if converted else names;self.mu.mem_write(NAMES, names)
        before = self.snapshot();assert len(before) == 1400
        # 정상 반환의 ABI/보존 레지스터/x87/SEH 복구를 입력마다 확인한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0x76543210 + register)
        self.mu.reg_write(UC_X86_REG_ECX, MANAGER);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.mem_write(STACK, struct.pack('<II', STOP, STREAM));self.mu.emu_start(0x44d380, STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 8 and self.mu.reg_read(UC_X86_REG_FPCW) == 0x27f and self.words(0)[0] == 0
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0x76543210 + register
        after = self.snapshot();assert decode(encode(before)) == before and decode(encode(after)) == after
        return [','.join(map(str, inputs)), encode(before), encode(after), ';'.join(self.events) or '-']


def cases():
    """세 섹션의 빈/비어 있지 않은 조합과 기존 목록·이름 초기 상태를 독립 입력으로 만든다."""
    for seed in (0, 255):
        for existing in (0, 1):
            for lengths in ((0, 0, 0), (1, 3, 7), (7, 0, 3), (0, 7, 0), (16, 1, 0), (16, 16, 16)):
                for converted in (0, 1):
                    for variant in (0, 1, 2):
                        yield [seed, existing, *lengths, converted, variant]


def main():
    """제품 수정 전에 최초 관측만 만들며 --verify는 저장한SHA만 확인한다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['hashes'].items():
            assert digest(ROOT / path) == expected, path
        print(json.dumps({'verified': report['rows']}));return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 자료 덮어쓰기 금지'
    oracle = ReadOracle();rows, instructions = [], 0
    # 원본 명령만으로 각 입력을 관측하며 현재 C++ 제품은 실행하지 않는다.
    for inputs in cases():
        rows.append(oracle.observe(inputs));instructions += oracle.instructions
    with FIXTURE.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['# input', 'before-rle', 'after-rle', 'events-rle']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_local_mission_transport.py', 'tools/decomp_datamanager_create.py', 'tools/decomp_datamanager_read.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'host': socket.gethostname(), 'rows': len(rows), 'normal_returns': len(rows), 'snapshot_bytes': 1400, 'instructions': instructions, 'entries': {hex(key): value for key, value in oracle.entries.items()}, 'limitations': ['큰 부모의 인라인 본문과 실제 하위 아홉 본문을 직접 실행한다. CRT ASCII 비교/new/free/이름 표 가상 조회만 명시 경계다.', 'void 부모의 EAX는 반환 계약에 포함하지 않는다. ABI/보존 레지스터/x87/정상 SEH 복구와 전체 쓰기 범위를 확인한다.', '파일 OS 읽기/실제 힙/CRT/게임/Windows 예외 실행은 아니다. 잘못된 포인터/음수 섹션 길이/OOM은 정상 관측 범위 밖이다.'], 'hashes': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key: report[key] for key in ('host', 'rows', 'normal_returns', 'instructions', 'entries')}))


if __name__ == '__main__':
    main()
