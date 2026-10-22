#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 데빌메이커80BYTE 생성/송신/수신을 대체 함수 없이 격리 실행한다. 게임 창은 열지 않는다."""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import socket
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 현재 정밀 함수 범위를 읽을 저장소다.
FIELDS, PACKET, OUTPUT, STACK, STOP = 0x1100000, 0x1200000, 0x1300000, 0x2100000, 0x3100000  # 격리된 본체/출력/부모/스택/정상 복귀 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/wind-aviary-state-x86.tsv'  # 독립 원본 출력만 저장한다.
REPORT = ROOT / 'cpppj/wind-aviary-state-20261022.json'  # 정상 복귀와 선택 범위·해시를 기록한다.


def digest(path):
    """실제 원문 바이트의 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """검토한 원본 네 함수만 실행하고 본체80/패킷64/부모4/스택 이외 쓰기를 거부한다."""
    def __init__(self, control):
        """두 x87 정밀도를 독립 실행할 격리 메모리와 관찰기를 만든다."""
        self.control = control  # 호출마다 다시 쓰는 명시 x87 제어어다.
        self.instructions = 0  # 실제 실행한 원본 명령의 누적 개수다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 원본 프로세스에 연결하지 않는 x86 실행기다.
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        # 본체·패킷·부모 출력·스택·복귀 입력을 서로 겹치지 않게 둔다.
        for address in (FIELDS, PACKET, OUTPUT, STACK, STOP):
            self.mu.mem_map(address, 4096)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 전체 정밀 분석에서 확인한 네 함수의 불연속 범위다.
        # 작은 helper나 CRT를 계산 스텁으로 바꾸지 않고 네 함수의 실제 명령만 허용한다.
        for entry in (0x45d730, 0x45d7a0, 0x45b150, 0x45b210):
            # 끝 주소를 포함하는 정밀 범위를 반개방 범위로 바꾼다.
            for part in functions[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """미검토 호출을 바로 거부하고 실제 원본 명령만 센다."""
        assert any(low <= address < high for low, high in self.allowed), hex(address)
        self.instructions += 1

    def write(self, mu, access, address, size, value, data):
        """호출자 입력으로 허용한 네 출력 영역 밖의 쓰기를 거부한다."""
        regions = [(FIELDS, FIELDS + 80), (PACKET, PACKET + 64), (OUTPUT, OUTPUT + 4), (STACK, STACK + 4096)]
        assert any(low <= address and address + size <= high for low, high in regions), (hex(address), size)

    def run(self, kind, values):
        """원문 입력을 준비하고 ret/스택/비휘발 및 전체 출력 버퍼를 보존한다."""
        mu = self.mu
        raw = bytearray([0xa5] * 80)
        struct.pack_into('<d', raw, 16, 123.25)
        struct.pack_into('<I', raw, 24, 7)
        struct.pack_into('<Q', raw, 32, 0x7ff0000000000001)
        struct.pack_into('<Q', raw, 40, 0x8000000000000000)
        struct.pack_into('<I', raw, 48, 0xfedcba98)
        struct.pack_into('<I', raw, 60, 77)
        struct.pack_into('<I', raw, 64, 50)
        packet = bytearray([0xa7] * 64)
        output = 0xdadadada
        if kind == 'C':
            server, mode, fill = values
            raw[:] = bytes([fill]) * 80
            mu.mem_write(0x540bc0, struct.pack('<I', server))
            mu.mem_write(0x55b4d0, struct.pack('<d', (0, 100.25, -5.25)[mode]))
            entry, receiver, args = 0x45d730, FIELDS, []
        elif kind == 'W':
            dirty, include, force, parent, fill = values
            # 원문이 쓰지 않는 주변을 바꿔 같은 패킷 선택에서 잔여 바이트를 확인한다.
            raw[0:8] = bytes([fill]) * 8
            raw[8] = dirty
            struct.pack_into('<I', raw, 64, parent)
            mu.mem_write(0x53278b, b'\x43')
            mu.mem_write(0x53278a, b'\x49')
            entry, receiver, args = 0x45d7a0, FIELDS + 56, [PACKET, include, force]
        else:
            dirty, fill = values
            raw[:] = bytes([fill]) * 80
            packet[:] = bytes((index * 17 + 3) & 255 for index in range(64))
            packet[8] = dirty
            entry, receiver, args = 0x45b210, FIELDS, [PACKET, OUTPUT]
        mu.mem_write(FIELDS, bytes(raw))
        mu.mem_write(PACKET, bytes(packet))
        mu.mem_write(OUTPUT, struct.pack('<I', output))
        mu.mem_write(STACK, b'\xa7' * 4096)
        start = STACK + 2048
        mu.mem_write(start, struct.pack('<' + 'I' * (1 + len(args)), STOP, *args))
        mu.reg_write(UC_X86_REG_FPCW, self.control)
        mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        # 비휘발 입력을 구별해 정상 복귀 뒤 보존 여부를 확인한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            mu.reg_write(reg, 0x12340000 + reg)
        mu.reg_write(UC_X86_REG_ECX, receiver)
        mu.reg_write(UC_X86_REG_ESP, start)
        mu.emu_start(entry, STOP, count=1000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP
        assert mu.reg_read(UC_X86_REG_ESP) == start + 4 + len(args) * 4
        # 호출 규약이 요구하는 네 레지스터를 독립 대조한다.
        for reg in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert mu.reg_read(reg) == 0x12340000 + reg
        if kind == 'C':
            assert mu.reg_read(UC_X86_REG_EAX) == FIELDS
        result = mu.reg_read(UC_X86_REG_EAX) & 65535 if kind == 'W' else 0
        return [kind, ','.join(map(str, values)), bytes(mu.mem_read(FIELDS, 80)).hex(),
                bytes(mu.mem_read(PACKET, 64)).hex(), str(struct.unpack('<I', mu.mem_read(OUTPUT, 4))[0]), str(result)]


def cases():
    """미작성 생성자 바이트와 모든 변경 비트·부모/강제 전송 조합을 독립 입력으로 만든다."""
    # 서버/시각/기존 힙 바이트를 교차한다.
    for server in (0, 1):
        # 실제 double의0/양수/음수를 분리한다.
        for mode in range(3):
            # 생성자가 보존할 잔여 바이트를 구별한다.
            for fill in (0, 0x5a, 0xa5):
                yield 'C', (server, mode, fill)
    # 미사용 비트까지 모든 BYTE 값을 송신한다.
    for dirty in range(256):
        # 부모 선택과 강제 전송은 서로 독립인 두 인자다.
        for include in (0, 1):
            # force 전의 dirty를 가리지 않고 전체 결과를 원본에서 얻는다.
            for force in (0, 1):
                yield 'W', (dirty, include, force, 50, 0xa5)
    # 부모0/WORD 최대/DWORD 최대에서 thunk와 본문의 분기를 구별한다.
    for dirty in (0, 1, 2, 3, 63, 128, 255):
        # 부모는 원문 DWORD이며 호스트 SID 범위로 미리 줄이지 않는다.
        for parent in (0, 65535, 0xffffffff):
            # 부모/force 조합별 packet의 비작성 꼬리도 비교한다.
            for include, force in ((0, 0), (0, 1), (1, 0), (1, 1)):
                yield 'W', (dirty, include, force, parent, 0x5a)
    # 수신이 보존해야 할 원문 padding/미선택 필드를 각각 확인한다.
    for dirty in range(256):
        # 초기 본체의 서로 다른 세 바이트 무늬다.
        for fill in (0, 0x5a, 0xa5):
            yield 'R', (dirty, fill)


def main():
    """게임 비실행 원문 자료를 생성하거나 이미 저장한 출처 SHA를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        # 검증/제품/원본 입력을 같은 바이트 기준으로 검사한다.
        for name, expected in saved['sha256'].items():
            assert digest(ROOT / name) == expected, name
        print('Wind Aviary state SHA:', saved['observations'])
        return
    first, second = Oracle(0x027f), Oracle(0x037f)
    rows = [first.run(kind, values) for kind, values in cases()]
    assert rows == [second.run(kind, values) for kind, values in cases()]
    assert len(rows) == len({(row[0], row[1]) for row in rows})
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(['kind', 'input', 'fields', 'packet', 'parent', 'result'])
        writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_wind_aviary_state.py',
             'cpppj/src/o/RawWindAviaryState.h', 'cpppj/src/o/RawWindAviaryState.cpp',
             'cpppj/tests/WindAviaryStateTests.cpp', 'cpppj/tests/fixtures/wind-aviary-state-x86.tsv']
    facts = dict(host=socket.gethostname(), lastFullDecompileHost='VM-W11-Codex', observations=len(rows), scopes=dict(Counter(row[0] for row in rows)),
                 precisionReplays=[0x027f, 0x037f], normalReturns=len(rows)*2, instructions=first.instructions + second.instructions,
                 gameGuiRuns=0, sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_bytes((json.dumps(facts, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print('Wind Aviary state:', facts['observations'], facts['scopes'], 'instructions', facts['instructions'])


if __name__ == '__main__':
    main()
