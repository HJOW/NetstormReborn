#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""타입 변경4b1620과 비석/주문 입력을 실제 PE로 실행하고 공간 장치만 분리한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 격리된50BYTE 슬롯과 현재188TYPE 표다. 실제 게임/파일/창을 실행하지 않는다.
POOL, SID, EVENT = 0x12000000, 50, SCRATCH + 0x1c000
# 두 실제 파생과 constructor=0 fallback의 명시 입력이다.
KINDS = {112: (0x4625f0, 0x50a748, 17), 114: (0x420e40, 0x502370, 21),
         116: (0x420e60, 0x502448, 33), 80: (0, 0x501dd0, 5)}


class ConversionOracle(Oracle):
    """진단은 복귀시키고 Unpop/가상Pop/공통Input만 명시 경계로 기록한다."""
    def __init__(self):
        """실제 타입 변경·생성자·QA·Repop 위임 몸체를 허용한다."""
        helpers = [0x4af720, 0x4ac550, 0x49a840, 0x4acd10, 0x4ac220, 0x41c0d0,
                   0x4625f0, 0x420e40, 0x420e60]
        super().__init__('originals', [dict(name='Type', originals='004b1620'),
            dict(name='Buried', originals='00462290'), dict(name='Spell', originals='0041e400')], helpers)
        self.mu.mem_map(POOL, 0x2000)
        self.trace = []  # 실제 원문이 호출한 진단/공간/부모 효과의 순서다.
        self.mutation = 0  # Unpop 경계 뒤 QA/TYPE 변경을 명시하는 외부 입력이다.
        self.calls = {}  # 실제 실행된 함수 시작만 기록한다.
        self.starts = set(self.entries.values()) | set(helpers)

    def finish(self, value=0, purge=0):
        """외부 장치의 실제 ret N만 재현하며 게임 계산을 하지 않는다."""
        stack = self.mu.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack('<I', self.mu.mem_read(stack, 4))[0]
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, stack + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, ret)

    def on_instruction(self, mu, address, size, data):
        """현재 물리 바이트를 장치 경계에서만 바꾸고 나머지 명령은 원문으로 실행한다."""
        if address in self.starts:
            self.calls[address] = self.calls.get(address, 0) + 1
        stack = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e0620:
            args = struct.unpack('<IIII', mu.mem_read(stack, 16))
            self.trace.append('A,' + str(args[3]))
        elif address == 0x4afe50:
            flags = struct.unpack('<I', mu.mem_read(stack + 4, 4))[0]
            self.trace.append('U,' + str(flags))
            raw = bytearray(mu.mem_read(POOL + SID * 50, 50));raw[11] |= 4
            if self.mutation == 1:
                struct.pack_into('<H', raw, 12, 116)
            elif self.mutation == 2:
                raw[10] = 116
            mu.mem_write(POOL + SID * 50, bytes(raw));self.finish(purge=4);return
        elif address == 0x4b02d0:
            args = struct.unpack('<III', mu.mem_read(stack + 4, 12))
            self.trace.append('P,' + ','.join(map(str, args)))
            raw = bytearray(mu.mem_read(POOL + SID * 50, 50));raw[11] &= ~4
            mu.mem_write(POOL + SID * 50, bytes(raw));self.finish(purge=12);return
        elif address == 0x4ae010:
            event = struct.unpack('<I', mu.mem_read(stack + 4, 4))[0]
            code = struct.unpack('<I', mu.mem_read(event, 4))[0]
            self.trace.append('B,' + str(code));self.finish(77, 4);return
        return super().on_instruction(mu, address, size, data)

    def prepare(self, old, state, new, mismatch, mutation=0):
        """입력 TYPE/슬롯/이벤트를 원문 함수와 독립된 명시 바이트로 설치한다."""
        self.trace, self.calls, self.mutation, self.assertions = [], {}, mutation, 0
        self.mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        self.mu.mem_write(0x59ab20, struct.pack('<I', SCRATCH))
        self.mu.mem_write(0x541218, struct.pack('<I', 112))
        self.mu.mem_write(0x542a2c, struct.pack('<I', 0x030000ce))
        for kind, (constructor, _, frame) in KINDS.items():
            # TYPE 입력은 원문500BYTE의 실제 constructor/foot/defaultFrame 위치다.
            raw = bytearray(500)
            struct.pack_into('<I', raw, 0x1d0, constructor)
            struct.pack_into('<II', raw, 0x1d4, 1, 1)
            struct.pack_into('<I', raw, 0x118, frame)
            self.mu.mem_write(SCRATCH + kind * 500, bytes(raw))
        if old != new:
            self.mu.mem_write(SCRATCH + new * 500 + 0x1d4, struct.pack('<II', 2 if mismatch & 1 else 1, 3 if mismatch & 2 else 1))
        raw = bytearray((index * 7 + 9) & 255 for index in range(50))
        struct.pack_into('<I', raw, 0, KINDS[old][1]);raw[10], raw[11] = old, state
        struct.pack_into('<H', raw, 12, new)
        struct.pack_into('<ff', raw, 14, 100.5, 122.25)
        self.mu.mem_write(POOL + SID * 50, bytes(raw))
        self.mu.mem_write(EVENT, bytes(16))
        return bytes(raw)

    def observed(self):
        """원문이 바꾼50BYTE 전부와 효과 순서를 보정 없이 읽는다."""
        return bytes(self.mu.mem_read(POOL + SID * 50, 50)).hex(), ';'.join(self.trace)


def write_fixture(name, header, rows, verify):
    """기존 원문 기대값을 덮지 않고 신규 TSV만 저장한다."""
    target = ROOT / 'cpppj/tests/fixtures' / name
    output = io.StringIO(newline='');writer = csv.writer(output, delimiter='\t', lineterminator='\n')
    writer.writerow(header);writer.writerows(rows);encoded = output.getvalue().encode('utf-8')
    if verify:
        assert target.read_bytes() == encoded, '기존 원문 관측과 불일치: ' + str(target)
    else:
        assert not target.exists(), target
        target.write_bytes(encoded)
    return dict(path=target.relative_to(ROOT).as_posix(), rows=len(rows), sha256=hashlib.sha256(encoded).hexdigest())


def main():
    """두 원문 단계의 출력과 실제 함수 진입을 독립 관측으로 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');options = parser.parse_args()
    oracle = ConversionOracle();core, commands, runs = [], [], []
    code_before = [(low, high, hashlib.sha256(bytes(oracle.mu.mem_read(low, high - low))).hexdigest()) for low, high in oracle.allowed]  # 실제 허용 기계어의 현재 바이트다.
    for old in (112, 114):
        # 진단 복귀 뒤 쓰기와 free/dead의 비추가 관문도 관측한다.
        for new in (112, 114, 116, 80):
            for state in (0, 4, 6, 7):
                for mismatch in (0, 1, 2, 3):
                    oracle.prepare(old, state, new, mismatch)
                    oracle.call('Type', [new], this=POOL + SID * 50, purge=4)
                    core.append((old, new, state, mismatch, *oracle.observed()))
                    runs.append(dict(entry='Type', functions={f'{a:08x}': n for a, n in oracle.calls.items()}))
    for name, old in (('Buried', 112), ('Spell', 114)):
        # 명령 상위 비트와 Unpop 뒤 값 변경을 각각 원문에 전달한다.
        for state in (0, 4):
            for code in (0x030000ce, 0x830000ce, 0x03000015):
                for mutation in (0, 1, 2):
                    oracle.prepare(old, state, 114, 0, mutation)
                    oracle.mu.mem_write(EVENT, struct.pack('<I', code))
                    result = oracle.call(name, [EVENT], this=POOL + SID * 50, purge=4)
                    commands.append((name, state, code, mutation, result, *oracle.observed()))
                    runs.append(dict(entry=name, functions={f'{a:08x}': n for a, n in oracle.calls.items()}))
    evidence = [write_fixture('squid-retype-x86.tsv', ('old', 'new', 'state', 'mismatch', 'raw', 'trace'), core, options.verify),
                write_fixture('object-conversion-x86.tsv', ('body', 'state', 'code', 'mutation', 'result', 'raw', 'trace'), commands, options.verify)]
    assert all(hashlib.sha256(bytes(oracle.mu.mem_read(low, high - low))).hexdigest() == digest for low, high, digest in code_before)
    report = dict(unit=517, host='HJOW-Athlon', lastFullDecompile='HJOW-Athlon / 2026-10-23',
                  originalGameExecuted=False, attackFlyersExecuted=False, binarySha256=oracle.sha256,
                  fixtures=evidence, runs=runs, normalReturns=len(runs), verifiedExistingFixtures=options.verify,
                  codeEvidence=code_before, codeImageUnchanged=True, toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  boundaries=['Unpop4afe50', 'virtualPop4b02d0', 'baseInput4ae010', 'assert4e0620'])
    (ROOT / 'cpppj/object-conversion-native-20261028.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(fixtures=evidence, normalReturns=len(runs))))


if __name__ == '__main__':
    main()
