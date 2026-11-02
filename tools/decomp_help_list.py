#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CreateHelpList의 실제 부모·BaseFile 생성/쓰기/닫기/소멸을 work 새 관측으로만 고정한다."""
from pathlib import Path
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_mission_abort import Oracle as BaseOracle, POOL, TYPES, STACK, STOP
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

HELP = POOL + 0x80000  # 기존 입력과 겹치지 않는 현재 Help 필드다.


class BoundaryFailure(Exception):
    """원문 경계에 공급한 실패 직전까지의 실제 효과를 보존한다. SEH unwind 관측은 아니다."""


class Oracle(BaseOracle):
    """실제 부모/파일 몸체를 실행하고 UI·TYPE·CRT 파일 장치만 명시한다."""
    def __init__(self):
        """현재 호스트의 정확한 범위만 허용하고 원본 게임/OS는 시작하지 않는다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.allowed = []
        ranges(self, (0x4c63e0, 0x41a5a0, 0x41a910, 0x41a680, 0x41b410, 0x4e4a40))
        self.samples = 0
        self.instructions = 0

    def guard(self, mu, access, address, size, value, data):
        """실제 SEH 체인 DWORD만 기존 격리 쓰기 허용에 더한다."""
        if address == 0 and size == 4:
            return
        super().guard(mu, access, address, size, value, data)

    def stream(self):
        """원문의 현재 +bc를 동일한 지역 파일 주소와 비교하여 기록한다."""
        value = self.words(HELP + 0xbc, 1)[0]
        return '0' if not value else 'F' if value == self.file else 'OLD' if value == 0x12345678 else 'OTHER'

    def instruction(self, mu, address, size, data):
        """현재 필드/개수/이름을 콜백 시점에 읽고 외부 변이 뒤의 재조회를 관측한다."""
        if any(low <= address and address + size <= high for low, high in self.allowed):
            self.instructions += 1
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 5)
        ecx = mu.reg_read(UC_X86_REG_ECX)
        if address == 0x4c62a0:
            self.events.append('Help:' + self.string(args[0]).hex() + ':' + str(args[1]) + ':' + str(args[2]))
            self.ret(HELP if self.present else 0)
            return
        if address == 0x41b4c0:
            self.file = ecx
            self.events.append('Open:' + self.string(args[0]).hex() + ':' + str(args[1]) + ':' + str(args[2]) + ':' + self.stream())
            assert self.words(ecx, 1)[0] == 0 and self.words(ecx + 16, 2) == (0, 0)
            if self.fail == 'open':
                raise BoundaryFailure()
            self.put(ecx, 77)
            self.put(ecx + 4, 0)
            self.ret(purge=12)
            return
        if address == 0x49a840:
            index = args[0]
            self.events.append('Type:' + str(index) + ':' + self.stream())
            self.lookups += 1
            if self.mutation == 1 and self.lookups == 2:
                mu.mem_write(TYPES + index * 500 + 4, b'Changed\0')
            self.ret(TYPES + index * 500)
            return
        if address == 0x4a89c0:
            assert self.string(args[1]) == b'%sType'
            value = self.string(args[2]) + b'Type'
            self.events.append('Format:' + value.hex() + ':' + self.stream())
            mu.mem_write(args[0], value + b'\0')
            self.ret(args[0])
            return
        if address == 0x4c5970:
            assert ecx == HELP
            topic = self.string(args[1])
            self.events.append('Show:' + str(args[0]) + ':' + topic.hex() + ':' + self.stream())
            if self.fail == 'show':
                raise BoundaryFailure()
            self.write(b'[' + topic + b']')
            self.ret(purge=8)
            return
        if address == 0x4662b0:
            self.events.append('Cleanup:' + self.stream())
            self.cleanups += 1
            if self.fail == 'cleanup':
                raise BoundaryFailure()
            if self.mutation == 2 and self.cleanups == 1:
                self.put(0x541348, 1)
            if self.mutation == 3 and self.cleanups == 1:
                self.put(0x541348, 5)
            if self.mutation == 4 and self.cleanups == 1:
                self.put(TYPES + 500 + 0xec, 1)
            self.ret()
            return
        if address == 0x466480:
            assert ecx == HELP
            self.events.append('Kill:' + self.stream())
            if self.fail == 'kill':
                raise BoundaryFailure()
            self.ret()
            return
        if address == 0x4cf960:
            self.events.append('Tell:' + self.string(args[0]).hex() + ':' + self.stream())
            if self.fail == 'tell':
                raise BoundaryFailure()
            self.ret()
            return
        if address == 0x4e54b9:
            self.events.append('Close:' + str(args[0]) + ':' + self.stream())
            self.ret()
            return
        if address == 0x4e44a6:
            assert args[3] == 77 and args[2] == 1
            self.events.append('Write:' + bytes(mu.mem_read(args[0], args[1])).hex() + ':' + str(args[1]) + ':1:' + self.stream())
            self.write(bytes(mu.mem_read(args[0], args[1])))
            if self.fail == 'write':
                raise BoundaryFailure()
            self.ret(0 if self.short else 1)
            return
        raise RuntimeError(f'허용하지 않은 원문 실행 {address:08x}')

    def write(self, value):
        """명시 FILE 장치는 위치0 덮어쓰기/기존 tail을 보존하고 부분 반환도 재시도하지 않는다."""
        if self.short:
            value = value[:1]
        end = self.position + len(value)
        if len(self.contents) < end:
            self.contents.extend(bytes(end - len(self.contents)))
        self.contents[self.position:end] = value
        self.position = end

    def sample(self, case, precision):
        """동일 독립 입력을 두 x87 정밀도로 관측한다. 실패는 unwind 이전의 경계다."""
        self.present, count, self.mutation, self.short, self.fail, old = case
        self.events = []
        self.file = 0
        self.position = 0
        self.contents = bytearray(old.encode('ascii'))
        self.lookups = self.cleanups = 0
        self.mu.mem_write(HELP, bytes(0xd0))
        self.put(HELP + 0xbc, 0x12345678)
        self.put(0x541348, count)
        self.put(0, 0)
        for index, (name, flags) in enumerate((('Alpha', 1), ('Beta', 0), ('Gamma', 0x80000000), ('Delta', 0xffffffff), ('Epsilon', 4))):  # +ec의 임의 비제로를 포함하며 이름은 첫NUL까지만 원문이 읽는다.
            self.mu.mem_write(TYPES + index * 500, bytes(500))
            self.mu.mem_write(TYPES + index * 500 + 4, name.encode('ascii') + b'\0ignored')
            self.put(TYPES + index * 500 + 0xec, flags)
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        for register, value in saved.items():  # 정상 반환에서 비휘발 레지스터와 호출 규약을 함께 고정한다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.put(STACK, STOP)
        outcome = 'ok'
        try:
            self.mu.emu_start(0x4c63e0, STOP, count=100000)
        except BoundaryFailure:
            outcome = self.fail
        if outcome == 'ok':
            assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        self.samples += 1
        return '\t'.join((outcome, self.contents.hex(), str(self.words(0x541348, 1)[0]), self.stream(), ';'.join(self.events)))


def main():
    """새 work fixture와 해시/한계 보고만 작성한다. 제품/기존 기대값/빌드는0이다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    fixture = WORK / 'modified/cpppj/tests/fixtures/quick-help-list-x86.tsv'
    report = WORK / 'native.json'
    assert not fixture.exists() and not report.exists()
    oracle = Oracle()
    rows = [(present, count, mutation, short, '', old) for present, count, mutation, short, old in itertools.product((0, 1), (-2, 0, 1, 3, 5), range(5), (0, 1), ('', 'Z' * 128)) if not mutation or count > 0]
    rows += [(1, 3, 0, 0, fail, '') for fail in ('open', 'write', 'show', 'cleanup', 'kill', 'tell')]
    lines = ['# present,count,mutation,short,fail,old — outcome,FILE bytes,current count,stream,effects; UI/CRT are explicit boundaries, failures stop before SEH unwind.']
    for row in rows:  # 기대값은 Python 제품 계산 없이 실제 PE의 현재 분기/필드에서 읽는다.
        first = oracle.sample(row, 0x027f)
        assert first == oracle.sample(row, 0x037f)
        lines.append(json.dumps(row, separators=(',', ':')) + '\t' + first)
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT / 'originals/Netstorm.exe', ROOT / 'extracted/refined/originals/functions.tsv', ROOT / 'extracted/refined/originals/Netstorm.c')}
    record = dict(unit=766, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', inputs=len(rows), observations=oracle.samples, instructions=oracle.instructions, gameExecutions=0, multiplayerTests=0, builds=0, actual='004c63e0 + 0041a5a0/0041a910/0041a680/0041b410', boundary='OpenHelp/TYPE getter/Show/Cleanup/Kill/Tell/format/whole Open and CRT FILE only; Show supplies a distinct marker, real QuickHelpHtml stream has separate existing native coverage; failure prefixes stop before SEH unwind', sha256=hashes, fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps({k: record[k] for k in ('inputs', 'observations', 'instructions')}))


if __name__ == '__main__':
    main()
