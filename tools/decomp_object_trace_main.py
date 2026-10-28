#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Main/Task의 원문 분기·현재 필드와 전체 자산 가상 표를 독립 관측한다."""
import csv
import hashlib
import json
import math
import re
import random
import socket
import struct
import sys
from decomp_object_trace_fields import FieldsOracle
from decomp_trace_crt import TraceCrtOracle
from decomp_bridgedecay_oracle import DecayOracle, ROOT, POOL, TYPES, UC_X86_REG_ESP

# 자산의 실제 Main/Task와 두 내부 조회 본체다.
ENTRIES = dict(Main=0x4b0fb0, Task=0x450d60, Graph=0x4ad390, Island=0x4acda0)
# 원문에서 짧은 parent/previous가 Ghidra 함수로 분리되지 않은 실제9/5BYTE 구간이다.
LEAVES = ((0x4ad000, 9), (0x4ad010, 5))
# 원문 자료·보고서와 새 고정 기대값 경로다.
WORK = ROOT/'extracted/cpp-object-trace-main-556-20261029'
FIXTURE = ROOT/'cpppj/tests/fixtures/object-trace-main-x86.tsv'
# 가상 분배의 독립 PE 표를 C++ 검사에서 읽을 별도 기록이다.
VTABLE_FIXTURE = ROOT/'cpppj/tests/fixtures/object-trace-vtables-x86.tsv'
# 원본 CRT의 float32 좌표→고정 소수점5자리 출력 전체 바이트다.
COORDINATE_FIXTURE = ROOT/'cpppj/tests/fixtures/object-trace-coordinate-x86.tsv'


class MainOracle(FieldsOracle):
    """현재 가상 링크/graph/island/필드 계산은 실제 코드로, CRT와 로그만 외부 경계로 실행한다."""
    def __init__(self):
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for entry in ENTRIES.values():  # 지정된 네 원문 몸체만 허용 범위에 더한다.
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.add_body(low, high+1-low)
        for low, size in LEAVES:  # PE의 원래 바이트를 그대로 실행하고 실행 전후 불변을 확인한다.
            self.add_body(low, size)
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **ENTRIES))
        self.crt = TraceCrtOracle()  # CRT 계산도 별도 원문 엔진에서 실행해 로그 함수의 현재 스택을 보존한다.

    def add_body(self, low, size):
        """허용 구간과 실행 전 코드 바이트를 같은 원본 PE에서 기록한다."""
        self.allowed[low-self.base:low+size-self.base] = b'\x01'*size
        self.bodies.append((low, bytes(self.mu.mem_read(low, size))))

    def formatted(self, fmt, address):
        """CRT경계의 정수/문자열/double 인자를 실제 cdecl 위치에서 읽는다."""
        arguments = []
        for match in re.finditer(rb'%(?:[-+ #0]*\d*(?:\.\d+)?)?([sdxXf%])', fmt):  # Main의 두 double도8BYTE로 정확히 소비한다.
            code = match.group(1)
            if code == b'%':
                continue
            if code == b'f':
                value = bytes(self.mu.mem_read(address, 8))
                assert math.isfinite(struct.unpack('<d', value)[0])
                address += 8
                arguments.append(('f', struct.unpack('<II', value)))
            else:
                value = self.number(address)
                address += 4
                if code == b's':
                    arguments.append(('s', self.text(value)))
                else:
                    arguments.append(('i', (value,)))
        return self.crt.format(fmt, arguments, self.control)

    def on_instruction(self, mu, address, size, data):
        """현재 Main 로그 인자를 보존하고 지정된 로그 반환에서만 외부 상태를 바꾼다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e58e9:
            result = self.formatted(self.text(self.number(sp+8)), sp+12)
            self.mu.mem_write(self.number(sp+4), result+b'\0')
            self.returned(len(result))
            return
        if address == 0x4c2630:
            fmt = self.text(self.number(sp+8))
            line = self.formatted(fmt, sp+12)
            self.events.append(f'{self.number(sp+4)}:{line.hex()}')
            if b'Main(' in fmt:
                # 문자열 이외의 최종 double/정수도 기록해 CRT 반올림과 원문 계산을 구별한다.
                self.arguments = bytes(self.mu.mem_read(sp+12, 16)).hex()+':'+','.join(str(self.number(sp+28+i*4)) for i in range(4))
            if len(self.events) == 1 and self.mutation == 1:
                self.number(POOL+self.sid*50+40, 255, 1)
                self.number(POOL+self.sid*50+26, 12345)
                self.number(POOL+self.sid*50+30, 254, 1)
                self.number(0x5c8454, 1)
                self.number(POOL+self.sid*50+12, 0x5106, 2)
            if len(self.events) == 2 and self.mutation == 2:
                self.number(POOL+self.sid*50+12, 0x5104, 2)
            self.returned()
            return
        DecayOracle.on_instruction(self, mu, address, size, data)

    def sample(self, case, control):
        """raw 입력/현재 TYPE/모드만 설치하고 기대 결과는 실제 기계어가 계산한다."""
        kind, self.sid, suppress, flags1, flags2, self.mutation, raw = case
        self.control = control  # 바깥 함수와 CRT도 같은53/64비트 정밀도로 독립 실행한다.
        self.arguments = ''
        self.mu.mem_write(POOL+self.sid*50, raw)
        self.mu.mem_write(TYPES, bytes(188*500))
        self.mu.mem_write(TYPES+82*500+4, b'assetName\0')
        self.number(TYPES+82*500+0xe8, flags1)
        self.number(TYPES+82*500+0xec, flags2)
        self.number(0x5c8454, suppress)
        self.call(kind, this=POOL+self.sid*50, control=control)
        assert all(bytes(self.mu.mem_read(address, len(body))) == body for address, body in self.bodies)
        return ';'.join(self.events)+'|'+self.arguments


def cases():
    """각 상태 비트/TYPE 관문/좌표/Task 경계와 로그 사이 현재 필드 변경을 입력한다."""
    result = []
    def add(kind, special, flags1=0, flags2=0, suppress=0, task=0, state=0, x=12.25, y=-3.5, mutation=0):
        """원문50BYTE의 서로 겹치는 좌표/contained 필드는 한 물리 입력으로만 만든다."""
        raw = bytearray(50)
        struct.pack_into('<I', raw, 0, 0x501dd0)
        struct.pack_into('<HHH', raw, 4, 7, 9, 65535)
        raw[10:12] = bytes((82, state))
        struct.pack_into('<Hff', raw, 12, task, x, y)
        struct.pack_into('<i', raw, 26, -2147483647)
        raw[30], raw[33], raw[34], raw[35] = 255, 128, 255, 255
        struct.pack_into('<I', raw, 36, 0x80000001)
        raw[40] = special
        result.append((kind, 123, suppress, flags1, flags2, mutation, bytes(raw)))
    for special in range(256):  # Main의상태8비트전체를각각독립관측한다.
        add('Main', special)
    for f1 in (0, 0x10, 0x800, 0x810):
        for f2 in (0, 2, 4, 6, 0x400000, 0x400002, 0x400006, 0xffffffff):
            for suppress in (0, 1, 2):
                add('Main', 129, f1, f2, suppress, state=8)
    for task in (0, 1, 0x5100, 0x5101, 0x5102, 0x5103, 0x5104, 0x5105, 0x5106, 0x5107, 0x5108, 65535):
        add('Task', 128, 0x810, 2, task=task)
    for x, y in ((0.0, -0.0), (0.0078125, -0.0078125), (0.015625, -0.015625), (255.99996948242188, 0.00001), (1234567.5, -1234567.5), (-0.000001, 1.0/3.0)):
        add('Main', 0, x=x, y=y)
    add('Main', 0, 0x810, 2, task=0x5101, mutation=1)
    add('Task', 0, 0x810, 2, task=0x5101, mutation=1)
    add('Task', 0, 0x810, 2, task=0x5101, mutation=2)
    return result


def inventory(oracle):
    """현재 생성자표의83개자산VT를PE에서읽어+0/+4/+70분배를기록한다."""
    source = (ROOT/'cpppj/src/o/DerivedConstructors.inc').read_text(encoding='utf-8').split('// originalCD')[0]
    tables = {int(value, 16) for value in re.findall(r'\{0x[0-9a-f]+U,0x([0-9a-f]+)U', source)} | {0x501dd0, 0x512110, 0x512098, 0x512188, 0x512208}
    return {f'{table:08x}': [f'{oracle.number(table+offset):08x}' for offset in (0, 4, 0x70)] for table in sorted(tables)}


def coordinates(crt):
    """정상/특수/동률/극단 float32와 고정seed 입력을 원본 CRT에서 두 번 관측한다."""
    inputs = {0, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7f800001, 0xff800001}
    for exponent in (0, 1, 10, 50, 100, 120, 127, 130, 150, 180, 200, 230, 254, 255):  # 지수 경계와가수의양끝을포함한다.
        for mantissa in (0, 1, 0x400000, 0x7fffff):
            for sign in (0, 0x80000000):
                inputs.add(sign | exponent << 23 | mantissa)
    for value in (0.015625, -0.015625, 0.046875, -0.046875, 0.0078125, -0.0078125, 255.99996948242188, 0.000005, -0.000005):
        inputs.add(struct.unpack('<I', struct.pack('<f', value))[0])
    rng = random.Random(0x4e58e9)  # 기대결과가아닌입력만고정한다.
    inputs.update(rng.getrandbits(32) for _ in range(768))
    rows = ['# float32Bits => 원본sprintf의%2.5f 전체 바이트']
    for bits in sorted(inputs):  # 같은float32승격을독립53/64비트CRT에서출력한다.
        value = struct.unpack('<f', struct.pack('<I', bits))[0]
        words = struct.unpack('<II', struct.pack('<d', value))
        a, b = (crt.format(b'%2.5f', [('f', words)], control) for control in (0x27f, 0x37f))
        assert a == b
        rows.append(f'{bits:08x}\t{a.hex()}')
    COORDINATE_FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    return dict(uniqueInputs=len(inputs), nativeObservations=len(inputs)*2,
                fixtureSha256=hashlib.sha256(COORDINATE_FIXTURE.read_bytes()).hexdigest())


def main():
    """두 정밀도의 전체 로그/최종 수치 인자가 같은 입력만 고정한다."""
    if '--coordinates-only' in sys.argv:
        report = json.loads((WORK/'native.json').read_text(encoding='utf-8'))
        report['coordinates'] = coordinates(TraceCrtOracle())
        (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
        print(json.dumps(report['coordinates']), flush=True)
        return
    oracle = MainOracle()
    inputs = cases()
    rows = ['# kind sid suppress flags1 flags2 mutation raw50 => 실제로그/최종double및정수인자']
    for case in inputs:  # 동일 입력을 독립53/64비트로 새로 설치한다.
        a, b = (oracle.sample(case, control) for control in (0x27f, 0x37f))
        assert a == b
        rows.append('\t'.join(map(str, (*case[:-1], case[-1].hex(), a))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    tables = inventory(oracle)
    VTABLE_FIXTURE.write_text('# table parent previous trace\n'+'\n'.join('\t'.join((table, *values)) for table, values in tables.items())+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23',
                  uniqueInputs=len(inputs), nativeObservations=len(inputs)*2, entries=ENTRIES,
                  vtables=tables, binarySha256=oracle.sha256,
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  vtableFixtureSha256=hashlib.sha256(VTABLE_FIXTURE.read_bytes()).hexdigest(),
                  crtEntry='004e58e9', crtFloatDispatch='004e3de3', crtInstructions=len(oracle.crt.executed),
                  boundary='Main/Task/TYPE/parent/previous/graph/island와원본sprintf/실수변환실제코드. 로그출력만장치기록. OS/게임실행0.')
    report['coordinates'] = coordinates(oracle.crt)
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'vtables'}, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
