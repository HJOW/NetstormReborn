#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본의 공통 객체 명령만 격리 실행하여 가상 호출 인자와 반환을 보존한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 별도 기대값 파일을 찾는 저장소다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
)

FIXTURE = ROOT / 'cpppj/tests/fixtures/object-command-x86.tsv'  # 제품 구현과 독립인 원본 반환/호출 기록이다.
REPORT = ROOT / 'cpppj/object-command-native.json'  # 관찰 범위와 보호 원본/도구/기대값의 해시다.
OBJECT, VTABLE, EVENT, STACK, STOP = 0x12000000, 0x12100000, 0x12200000, 0x12300800, 0x12400000  # 서로 독립인 호출자 메모리다.
OWNER, HIT, DESTROY, DESCRIBE = (STOP + 0x100 + index * 0x20 for index in range(4))  # 실제 가상 효과를 기록하는 경계다.
SPECS = {
    'originals': dict(file='originals/Netstorm.exe', entry=0x4ae010, base=0x4ac9b0, baseEnd=0x4ac9ef,
                      commands=(0x5429b0, 0x542a7c, 0x54274c, 0x5428b0), local=0x540c70, editor=0x5c85a4, mission=0x594fa4),
    'originalCD': dict(file='originalCD/NETSTORM.EXE', entry=0x4af0a0, base=0x4ac490, baseEnd=0x4ac4cf,
                       commands=(0x512b38, 0x512c04, 0x5128d4, 0x512a38), local=0x50f6c8, getter=0x4015d0),
}  # CD는 HP WORD/무조건 Hurt와 현재 소유자 getter를 별도 관찰한다.


def sha(path):
    """파일의 실제 전체 바이트 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """명령 상위 비트, 모드, 현재 HP의 감김과 판본 차이를 독립 입력으로 만든다."""
    # 같은 명령이라도 상위 여섯 비트는 원본 마스크를 거쳐야 한다.
    for command, upper, mode, hp in itertools.product(range(5), (0, 0xfc000000), range(4), (0, 1, 0x80000000, 0xffffffff)):
        yield command, upper, mode & 1, mode >> 1, hp, 2


class Oracle:
    """OS/게임을 시작하지 않고 두 짧은 제어 흐름과 명시 가상 경계만 실행한다."""

    def __init__(self, edition):
        """보호 PE의 메모리 사본과 격리 호출자 영역을 만든다."""
        self.spec = SPECS[edition]  # 선택 판본의 원본 주소다.
        self.edition = edition  # fixture에 쓸 판본 식별자다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 실제 Windows와 분리된 x86 실행기다.
        image = pefile.PE(str(ROOT / self.spec['file'])).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        # 호출자/객체/가상 표/중단 지점은 별도 페이지에 놓는다.
        for address in (OBJECT, VTABLE, EVENT, STACK & ~4095, STOP):
            self.mu.mem_map(address, 4096)
        self.commands = [self.get(address) for address in self.spec['commands']]  # 원본 데이터 DWORD를 직접 읽는다.
        self.instructions = 0  # 명시 경계 이외에 실행한 원본 명령 수다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def get(self, address):
        """현재 비정렬 DWORD를 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, 4), 'little')

    def put(self, address, value):
        """호출자 입력과 명시 반환을 DWORD 폭으로만 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def ret(self, purge, value=0):
        """가상 함수의 실제 thiscall 스택 정리 폭을 보존한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.get(sp))
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)

    def instruction(self, mu, address, size, data):
        """가상 호출 인자는 경계 진입에서 읽고 원본 외의 실행을 거부한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address in (OWNER, HIT, DESTROY, DESCRIBE):
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT
            name, count = {OWNER: ('owner', 1), HIT: ('hit', 3), DESTROY: ('destroy', 1), DESCRIBE: ('describe', 0)}[address]
            arguments = [self.get(sp + 4 + index * 4) for index in range(count)]
            self.events.append(':'.join([name, *map(str, arguments)]))
            self.ret(count * 4)
            return
        if address == self.spec.get('getter'):
            self.events.append('local')
            self.ret(0, self.get(self.spec['local']))
            return
        if not (self.spec['entry'] <= address < self.spec['entry'] + (105 if self.edition == 'originals' else 94)
                or self.spec['base'] <= address and address + size <= self.spec['baseEnd'] + 1):
            raise RuntimeError(f'공통 객체 명령 범위 밖 실행: {address:08x}')
        self.instructions += 1

    def sample(self, case):
        """한 독립 입력의 정상 반환/보존 레지스터/스택/가상 인자를 관측한다."""
        command, upper, editor, mission, hp, local = case
        self.events = []  # 이번 호출의 가상 효과 순서다.
        raw = bytearray([0xa5] * 50)
        struct.pack_into('<I', raw, 0, VTABLE)
        struct.pack_into('<I', raw, 26, hp)
        self.mu.mem_write(OBJECT, bytes(raw))
        # 원본과 같은 네 가상 슬롯만 명시 경계에 연결한다.
        for offset, address in ((0x74, OWNER), (0x98, HIT), (0x10, DESTROY), (0x70, DESCRIBE)):
            self.put(VTABLE + offset, address)
        self.put(EVENT, (self.commands[command] if command < 4 else 0x030003fe) | upper)
        self.put(self.spec['local'], local)
        if self.edition == 'originals':
            self.put(self.spec['editor'], editor)
            self.put(self.spec['mission'], mission)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, EVENT, 0xdec0dec0))
        registers = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789, UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}
        # 비휘발 레지스터가 원본 반환 뒤 보존되는지 확인한다.
        for register, value in registers.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, OBJECT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.emu_start(self.spec['entry'], STOP, count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 8
        assert self.get(STACK + 8) == 0xdec0dec0 and bytes(self.mu.mem_read(OBJECT, 50)) == bytes(raw)
        # 명시 가상 경계 이외에 원본이 호출자 레지스터를 손상하지 않았는지 확인한다.
        for register, value in registers.items():
            assert self.mu.reg_read(register) == value
        return str(self.mu.reg_read(UC_X86_REG_EAX)), ';'.join(self.events) or '-'


def main():
    """독립 기대값을 최초 생성하거나, 원본/도구/기대값의 기존 해시만 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if options.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 제품 소스가 아니라 원본 관측 자료의 바이트를 확인한다.
        for name, expected in report['sha256'].items():
            assert sha(ROOT / name) == expected, name
        print('공통 객체 명령 원본 자료 해시 일치:', report['normalReturns'], '정상 반환')
        return
    rows, counts = [], {}
    # 두 판본은 서로 다른 메모리 사본과 실제 주소로 실행한다.
    for edition in SPECS:
        oracle = Oracle(edition)
        # 기대값의 분기나 인자를 Python에서 계산하지 않고 실제 원본 반환만 저장한다.
        for case in cases():
            rows.append((edition, ','.join(map(str, case)), *oracle.sample(case)))
        counts[edition] = dict(normalReturns=len(list(cases())), instructions=oracle.instructions, commands=oracle.commands)
    FIXTURE.write_text('# 판본\t독립 입력(command,upper,editor,mission,hp,local)\t반환\t가상 호출\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8')
    paths = [Path(spec['file']) for spec in SPECS.values()] + [Path(__file__).relative_to(ROOT), FIXTURE.relative_to(ROOT)]
    record = dict(date='2026-10-20', host=socket.gethostname(), lastDecompileHost='VM-W11-Codex', lastDecompileDate='2026-10-20',
                  normalReturns=len(rows), editions=counts, gameExecutions=0,
                  limits=['공통 객체 명령과 Form 부모의 제어 흐름만 실행했다. 가상 소유자/피해/삭제/진단과 CD 소유자 getter는 명시 경계다. 실제 플레이와 가상 효과 전체의 증거가 아니다.'],
                  sha256={path.as_posix(): sha(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(counts, ensure_ascii=False))


if __name__ == '__main__':
    main()
