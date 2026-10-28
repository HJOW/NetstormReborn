#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""객체 Base/내장 Form의 현재 필드·서식을 원본 x86에서 관측한다."""
import csv
import hashlib
import json
import re
import socket
import struct
from decomp_process_summary import SummaryOracle
from decomp_bridgedecay_oracle import DecayOracle, ROOT, POOL, TYPES, UC_X86_REG_ESP

# 이름 순서는 fixture의 직접 진입 함수 번호다. 가상표를 임의 호출하지 않는다.
ENTRIES = dict(Base=0x4ac400, Form=0x4ae1e0, Process=0x4ae3f0, Depend=0x4ae530, Content=0x4aea90)
# 원시 기록과 새 독립 기대값의 경로다.
WORK = ROOT/'extracted/cpp-object-trace-fields-555-20261029'
FIXTURE = ROOT/'cpppj/tests/fixtures/object-trace-fields-x86.tsv'


class FieldsOracle(SummaryOracle):
    """TYPE/필드/호출 순서는 실제 기계어로 실행하고 CRT서식/로그만 기록한다."""
    def __init__(self):
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for entry in ENTRIES.values():  # 지정 함수의 실제 불연속 코드 범위만 더한다.
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)
                self.bodies.append((low, bytes(self.mu.mem_read(low, high+1-low))))
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **ENTRIES))

    def formatted(self, fmt, address):
        """외부 CRT의 실제 varargs 포인터에서 %s/%d/%x와 %%만 읽는다."""
        values = []
        for match in re.finditer(rb'%(?:[-+ #0]*\d*(?:\.\d+)?)?([sdxX%])', fmt):  # 이 다섯 몸체가 쓰는 CRT변환만 처리한다.
            code = match.group(1)
            if code == b'%':
                continue
            value = self.number(address)
            address += 4
            if code == b's':
                value = self.text(value)
            elif code == b'd' and value >= 0x80000000:
                value -= 0x100000000
            values.append(value)
        return fmt % tuple(values)

    def on_instruction(self, mu, address, size, data):
        """최종 level/로그 바이트를 기록하며 첫 로그의 명시 외부 변경을 적용한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e58e9:
            result = self.formatted(self.text(self.number(sp+8)), sp+12)
            self.mu.mem_write(self.number(sp+4), result+b'\0')
            self.returned(len(result))
            return
        if address == 0x4c2630:
            result = self.formatted(self.text(self.number(sp+8)), sp+12)
            self.events.append(f'{self.number(sp+4)}:{result.hex()}')
            if len(self.events) == 1 and self.mutation:
                # 다음 파생 줄은 현재 parent/PID/TYPE/수량/flags를 다시 읽어야 한다.
                for offset, value, width in ((14, 65535, 2), (16, 4321, 2), (18, 94, 4), (12, 65534, 2), (22, 0, 2)):
                    self.number(POOL+self.sid*50+offset, value, width)
            self.returned()
            return
        DecayOracle.on_instruction(self, mu, address, size, data)

    def sample(self, case, control):
        """서식 결과를 계산하지 않고 raw50BYTE·이름·모드의 독립 입력만 설치한다."""
        kind, self.sid, suppress, self.mutation, name, raw = case
        self.mu.mem_write(POOL+self.sid*50, raw)
        self.mu.mem_write(TYPES, bytes(188*500))
        for number, value in ((46, name), (82, name), (94, b'islandAfterCallback')):  # TYPE 전체 이름을 실제+4 C문자열로 설치한다.
            self.mu.mem_write(TYPES+number*500+4, value+b'\0')
        self.number(0x5c8454, suppress)
        self.call(kind, this=POOL+self.sid*50, control=control)
        assert all(bytes(self.mu.mem_read(address, len(body))) == body for address, body in self.bodies)
        return ';'.join(self.events)


def cases():
    """상태16조합·모드·전체 이름/축약·링크·signed PID와 콜백 변경을 독립 입력으로 만든다."""
    result = []
    for kind in ENTRIES:  # 같은 입력이 각 실제 파생 몸체에서 어떻게 사용되는지 따로 관측한다.
        for state in range(16):
            for suppress in (0, 1, 2):
                raw = bytearray(50)
                struct.pack_into('<HHHH', raw, 4, 1234, 65535, 0, 0)
                raw[10:12] = bytes((46, state))
                struct.pack_into('<HHHHHI', raw, 12, 123, 65535, 77, 82, 0x1234, 0x80000000)
                result.append((kind, 123, suppress, 0, b'LongTypeNameOverTwentyBytes', bytes(raw)))
        for name in (b'', b'A', b'percent%%name', b'%sGenerator', b'~yMarkup', b'name\0ignored', b'x'*60):
            raw = bytearray(50)
            raw[10] = 46
            struct.pack_into('<HHHHHH', raw, 12, 65535, 0, 65535, 82, 65535, 0)
            result.append((kind, 12345, 0, 0, name, bytes(raw)))
        raw = bytearray(50)
        raw[10] = 46
        struct.pack_into('<HHHHHH', raw, 12, 0, 7, 9, 82, 0, 0)
        result.append((kind, 123, 0, 1, b'beforeCallback', bytes(raw)))
        for pid in (0, 1, 999, 1000, 0x7fffffff, 0x80000000, 0xffffffff):
            raw = bytearray(50)
            raw[10] = 46
            struct.pack_into('<I', raw, 18, pid)
            # Content의 이 필드는TYPE WORD여서 유효한 원문 표 범위로만 입력한다.
            if kind != 'Content':
                result.append((kind, 123, 0, 0, b'processField', bytes(raw)))
    return result


def main():
    """같은 독립 입력의 두 정밀도 결과만 고정하고 최초 원문을 파일로 보존한다."""
    oracle = FieldsOracle()
    inputs = cases()
    rows = ['# kind sid suppress mutation nameHex raw50Hex => 실제 level/서식 바이트']
    for case in inputs:  # C++ 제품을 호출하지 않고 원본 기계어에서만 기대값을 얻는다.
        a, b = (oracle.sample(case, control) for control in (0x27f, 0x37f))
        assert a == b
        rows.append('\t'.join(map(str, (*case[:4], case[4].hex() or '-', case[5].hex(), a))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23',
                  uniqueInputs=len(inputs), nativeObservations=len(inputs)*2, entries=ENTRIES,
                  binarySha256=oracle.sha256, fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  boundary='Base/내장Form 다섯 실제 몸체와 TYPE 조회 실행; CRT sprintf/로그만 외부 장치. 게임/OS실행0.')
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
