#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 실제 hasLevel/getLevel/setLevel을 독립 x86으로 실행한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, TARGET, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

SPEC = {  # 보호 PE의 실제 세 함수 진입점이다.
    'originals': (0x4ad0a0, 0x4ad0f0, 0x4ad110),
    'originalCD': (0x4acdc0, 0x4ace00, 0x4ace20),
}
FIXTURE = ROOT / 'cpppj/tests/fixtures/squid-level-x86.tsv'  # 기존 Native 기대값과 분리한다.
REPORT = ROOT / 'cpppj/squid-level.json'  # 이번 실제 입력/출력과 PE/export SHA다.


class LevelOracle(OwnerOracle):
    """기존 PE/type조회 매핑을 재사용하고 원본 진단의 외부 경계만 관찰한다."""

    def __init__(self, edition):
        """새 함수 export 범위를 더하고 정상진단 이후 현재 BYTE 변경을 관찰한다."""
        super().__init__(edition)
        self.spec = dict(self.spec)  # 기존 Owner 전역 사전을 바꾸지 않는다.
        self.mutate = False  # 진단 callback의 명시 원문 변경 입력이다.
        folders = [ROOT / f'extracted/squid-level/{edition}']  # 실제 세 함수의 export다.
        if edition == 'originalCD':
            folders.append(ROOT / 'extracted/squid-level2/originalCD')
            folders.append(ROOT / 'extracted/squid-level3/originalCD')
        # 실제 getter 하위도 export 범위 안에서만 실행한다.
        for folder in folders:
            self.exports.extend(folder / name for name in ('creation.c', 'functions.tsv'))
            with (folder / 'functions.tsv').open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp, delimiter='\t'):
                    self.entries.add(int(row['entry'], 16))
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'))
                        self.allowed.append((low, high + 1))

    def on_instruction(self, mu, address, size, data):
        """진단 함수는 실제 요청 수를 기록하고 cdecl 반환 뒤 원래 setter 명령을 계속한다."""
        if address == self.spec['assert_report']:
            self.assertions += 1
            if self.mutate:
                mu.mem_write(self.slot(TARGET)+11, b'\x14')
            esp = mu.reg_read(UC_X86_REG_ESP)  # 실제 진단 호출의 현재 스택이다.
            result = struct.unpack('<I', mu.mem_read(esp, 4))[0]  # 원래 호출자 반환 주소다.
            mu.reg_write(UC_X86_REG_ESP, esp+4)
            mu.reg_write(UC_X86_REG_EIP, result)
            return
        super().on_instruction(mu, address, size, data)

    def call(self, entry, argument=None):
        """현재 동일 slot/type와 정상 return sentinel에서 실제 함수 하나를 실행한다."""
        stack = struct.pack('<I', STOP)  # 원래 함수의 정상 반환 경계다.
        if argument is not None:
            stack += struct.pack('<I', argument)
        self.mu.mem_write(STACK, stack)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, self.slot(TARGET))
        self.mu.emu_start(entry, STOP, count=2000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        return self.mu.reg_read(UC_X86_REG_EAX)

    def sample(self, state, group, value, mutation):
        """입력만 만들고 기대 BYTE/level/진단 수는 실제 PE 반환에서 얻는다."""
        raw = bytearray(self.stride)  # 명시 합성 슬롯 입력이며 호스트 메모리를 쓰지 않는다.
        raw[10], raw[11] = 82, state
        self.mu.mem_write(self.slot(TARGET), bytes(raw))
        record = bytearray(self.type_stride)  # 타입82의 명시 group/flags 입력이다.
        struct.pack_into('<I', record, 0x9c, group)
        self.mu.mem_write(TYPES+82*self.type_stride, bytes(record))
        self.write_ranges = [(self.slot(TARGET), self.slot(TARGET)+self.stride)]
        self.assertions = 0
        self.mutate = bool(mutation)
        has = self.call(SPEC[self.edition][0])
        self.call(SPEC[self.edition][2], value)
        level = self.call(SPEC[self.edition][1]) & 0xff
        output = bytes(self.mu.mem_read(self.slot(TARGET), self.stride)).hex()
        return has, level, self.assertions, output


def main():
    """96고유 입력의 전체 슬롯 BYTE를 기존 보고서와 별도 파일로 고정한다."""
    rows, editions = [], {}  # 입력과 실제 원본 관찰 자료다.
    inputs = list(itertools.product((0, 4, 0x9c), (0, 10), (0, 7, 8, 0xffffffff), (0, 1)))  # 판본별48 입력이다.
    # 양 보호 PE를 별도 매핑하여 원본 명령을 실행한다.
    for edition in SPEC:
        oracle = LevelOracle(edition)
        assertions = 0  # 원본이 요청한 진단 수이며 실패 반환으로 숨기지 않는다.
        for state, group, value, mutation in inputs:
            has, level, asserted, output = oracle.sample(state, group, value, mutation)
            rows.append((edition, state, group, value, mutation, has, level, asserted, output))
            assertions += asserted
        editions[edition] = dict(samples=len(inputs), assertions=assertions, pe=oracle.sha256,
                                exports={path.relative_to(ROOT).as_posix(): digest(path) for path in oracle.exports})
    FIXTURE.write_text('\n'.join('\t'.join(map(str, row)) for row in rows)+'\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon', uniqueInputs=len(rows), editions=editions,
                                     fixtureSha=digest(FIXTURE), toolSha=digest(ROOT / 'tools/decomp_squid_level.py'),
                                     boundary='원본 범위 진단만 관찰/반환하며 setter/getter/hasLevel/type조회는 실제 PE 명령'),
                                ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('Squid Level 독립 Native:', len(rows), '고유 입력, 정상 반환', len(rows))


if __name__ == '__main__':
    main()
