#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 레인 신전 Draw의 그림/효과음 호출 순서를 격리 x86으로 관찰한다. 실제 창은 실행하지 않는다."""
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path

# 저장소와 현재 PC의 격리 분석 의존성이다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP
from cpp_menu_smoke import source
from taff import TaffArchive
from shp import read_clusters

# 서로 겹치지 않는 합성 입력과 스택/반환 주소다. 실제 파일에는 쓰지 않는다.
TYPES, RAW, CODES, STACK, STOP = 0x11000000, 0x12000000, 0x13000000, 0x1400f000, 0x15000000
# 실제 가상 Draw와 TYPE 표/그림/소리 제출 주소다. 그림/소리 두 외부 경계만 관찰로 대체한다.
SPECS = {
    'originals': ('originals/Netstorm.exe', 0x4dcd00, 0x59ab20, 500, 0x497280, 0x4a9cb0, 36, 4),
    'originalCD': ('originalCD/NETSTORM.EXE', 0x44fe00, 0x51c960, 468, 0x455ee0, 0x438b50, 34, 1),
}


def digest(path):
    """근거 파일을 SHA-256으로 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """실제 Draw·TYPE·방향 조회를 실행하며 그림과 음향 제출 인자만 기록한다."""
    def __init__(self, edition, codes):
        """읽기 전용 PE 복사본과 현재 export의 실행 허용 범위를 준비한다."""
        self.edition, self.codes = edition, codes
        self.binary, self.entry, global_types, stride, self.draw, self.sound, self.frame_offset, self.frame_width = SPECS[edition]
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / self.binary))
        data = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(data) + 4095) & ~4095)
        self.mu.mem_write(0x400000, data)
        # 입력 표/객체/프레임 코드/스택/반환 위치를 독립 구역에 둔다.
        for address, size in ((TYPES, 0x20000), (RAW, 0x1000), (CODES, 0x1000), (STACK - 0xf000, 0x10000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.mu.mem_write(global_types, struct.pack('<I', TYPES))
        type_base = TYPES + 124 * stride
        self.mu.mem_write(type_base + 0xdc, struct.pack('<I', 0x12345678))
        self.mu.mem_write(type_base + 0x124, struct.pack('<I', CODES))
        self.mu.mem_write(CODES, b''.join(bytes((ord(side), 0, 0, 0)) for side in codes))
        # 원본 .type에서 읽은 방향별 첫 번호와 개수를 입력 표로 작성한다.
        for index in range(16):
            letter = chr(65 + index)
            self.mu.mem_write(type_base + 0x130 + index * 4, struct.pack('<i', codes.find(letter)))
            self.mu.mem_write(type_base + 0x170 + index * 4, struct.pack('<i', codes.count(letter)))
        self.allowed = []
        with (ROOT / f'extracted/rain-draw/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 실제 Ghidra의 불연속 함수 범위만 명령 실행을 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.events, self.calls, self.checking = [], {}, False
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """실행 중 스택 밖 쓰기를 거부한다. 원본 Draw는 객체/TYPE를 바꾸지 않는다."""
        if self.checking and not STACK - 0xf000 <= address <= STACK + 0x1000 - size:
            raise AssertionError(f'허용하지 않은 쓰기 {address:08x}+{size}')

    def instruction(self, mu, address, size, data):
        """그림/소리의 실제 인자와 순서를 보존하고 나머지는 원본 명령으로 진행한다."""
        if address in (self.draw, self.sound):
            esp = mu.reg_read(UC_X86_REG_ESP)
            count = 4 if address == self.draw else 6
            args = struct.unpack(f'<{count + 1}I', mu.mem_read(esp, (count + 1) * 4))
            if address == self.draw:
                assert args[1:4] == (311, 233, 0x12345678)
                assert mu.reg_read(UC_X86_REG_ECX) == RAW
                self.events.append(f'F{args[4]}')
            else:
                assert args[2:] == (0, 0, 0, 0, 0)
                self.events.append('S')
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, esp + (20 if address == self.draw else 4))
            mu.reg_write(UC_X86_REG_EIP, args[0])
            return
        assert any(low <= address and address + size <= high for low, high in self.allowed), f'미등록 명령 {address:08x}'
        key = f'{address:08x}'
        self.calls[key] = self.calls.get(key, 0) + 1

    def run(self, frame, word):
        """한 실제 입력의 전체 호출 순서·반환·보존 레지스터를 확인한다."""
        raw = bytearray(50 if self.frame_width == 4 else 36)
        struct.pack_into('<I', raw, 0, 0x5196f0 if self.frame_width == 4 else 0x501d28)
        raw[10] = 124
        struct.pack_into('<H', raw, 12, word)
        raw[self.frame_offset:self.frame_offset + self.frame_width] = frame.to_bytes(self.frame_width, 'little')
        self.mu.mem_write(RAW, bytes(raw))
        saved = {UC_X86_REG_EBX: 0x13579, UC_X86_REG_ESI: 0x24680, UC_X86_REG_EDI: 0x56789, UC_X86_REG_EBP: 0x67890}
        # 임의의 보존 레지스터를 모든 입력에서 확인한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, RAW)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, 311, 233))
        self.events, self.checking = [], True
        self.mu.emu_start(self.entry, STOP, count=10000)
        self.checking = False
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 12
        assert all(self.mu.reg_read(reg) == value for reg, value in saved.items())
        assert bytes(self.mu.mem_read(RAW, len(raw))) == raw
        return ','.join(self.events)


def main():
    """원본 TYPE 코드의 모든 정상 프레임과 네 WORD 상태를 판본별로 관찰한다."""
    archive = TaffArchive(ROOT / 'originals/netstorm.tarc')
    text = source(ROOT / 'originals', archive, 'd/rainvortex.type').decode('cp1252')
    codes = ''.join(cluster[0][0] for cluster in read_clusters(text))
    assert codes == 'A' + 'B' * 14 + 'C' * 13 + 'P'
    rows, reports = [], {}
    # 판본별 frame 폭 차이와 WORD0/1/2/65535를 각각 독립 입력으로 사용한다.
    for edition in SPECS:
        oracle = Oracle(edition, codes)
        for frame in range(len(codes)):
            for word in (0, 1, 2, 65535):
                rows.append(f'{edition}\t{frame}\t{word}\t{oracle.run(frame, word)}')
        reports[edition] = {'binary_sha256': digest(ROOT / oracle.binary), 'entry': f'{oracle.entry:08x}', 'cases': len(codes) * 4,
            'native_instruction_count': sum(oracle.calls.values()), 'leaf_observations': ['shape-submit', 'sound-submit'],
            'exports': {name: digest(ROOT / f'extracted/rain-draw/{edition}/{name}') for name in ('creation.c', 'functions.tsv')}}
    fixture = ROOT / 'cpppj/tests/fixtures/rain-draw-x86.tsv'
    fixture.write_text('# 원본 레인 신전 가상 Draw. 그림/음향 제출 경계는 관찰이며 픽셀/장치 검사는 별도다.\n' + '\n'.join(rows) + '\n', encoding='utf-8')
    report = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'cases': len(rows), 'editions': reports, 'fixture_sha256': digest(fixture), 'tool_sha256': digest(Path(__file__))}
    (ROOT / 'cpppj/rain-draw.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'레인 Draw {len(rows)}개 정상 반환, 스택/보존 레지스터/raw 불변 확인')


if __name__ == '__main__':
    main()
