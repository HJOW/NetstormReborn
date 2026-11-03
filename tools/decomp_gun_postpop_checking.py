#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""네 Gun PostPop의 현재 진단 위치와 복귀를 게임 실행 없이 원문에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_assetpostpop_oracle import AssetOracle, KEYS, TARGET
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW


def inputs():
    """본문 종류·생성 관문·진단·확보 실패와 콜백 뒤 현재 전역 변경을 교차한다."""
    rows = []
    base = dict(zip(KEYS, (1, 1, 0, 1, 1, 0, 7, 0, 25, 0x1000, 1000, 0, 0, 0)))
    for kind, flags, extra, server, load, checking, forms, reserve in itertools.product(
            (1, 2, 3, 4), (0, 1, 2, 0x51), (0, 1), (0, 1), (0, 1), (0, 0x80000000), (0, 1), (0, 7)):  # 네 부모의 정상/진단 선택과 생성 실패를 원문으로 관찰한다.
        case = base.copy()
        case.update(kind=kind, flags=flags, extra=extra, battle=server, load=load, reserve=reserve)
        rows.append((case, (checking, forms, 1, 0, load)))
    for kind, mutation, checking, forms, resume, reserve in itertools.product(
            (1, 2, 3, 4), range(1, 6), (0, 0xffffffff), (0, 1), (0, 1), (0, 7)):  # 생성/조회/진단 뒤 바뀐 전역을 다음 관문이 다시 읽게 한다.
        case = base.copy()
        case.update(kind=kind, load=0, reserve=reserve)
        rows.append((case, (checking, forms, resume, mutation, 0)))
    return rows


class GunPostPopOracle(AssetOracle):
    """실제 네 부모와 isClientSid를 실행하고 생성·Form 판정·진단만 장치로 둔다."""
    def __init__(self):
        """기존 자산 ABI 장치와 현재 정밀 함수표의 SID 판정 범위를 사용한다."""
        super().__init__('originals')
        add_ranges(self, (0x441fa0,))

    def string(self, address):
        """원본 진단의 짧은 NUL 종료 ASCII를 그대로 반환한다."""
        result = bytearray()
        for offset in range(512):  # 손상된 진단 주소의 무한 읽기를 막는다.
            value = self.mu.mem_read(address + offset, 1)[0]
            if not value:
                return bytes(result)
            result.append(value)
        raise ValueError('진단 문자열 종료 없음')

    def on_instruction(self, mu, address, size, data):
        """원문 위치에서 입력을 설치하고 명시 효과가 바꾼 현재 상태를 보존한다."""
        p = self.p
        if address == p['entries'][self.case['kind']]:
            mu.mem_write(p['checking'], struct.pack('<I', self.control[0]))
            mu.mem_write(p['fort'], struct.pack('<I', self.control[4]))
            mu.reg_write(UC_X86_REG_FPCW, self.precision)
        mutation = self.control[3]
        if address == p['allocate'] and mutation in (1, 4):
            mu.mem_write(p['checking'], struct.pack('<I', 0 if mutation == 1 else 1))
            mu.mem_write(p['load'], struct.pack('<I', 1 if mutation == 1 else 0))
            mu.mem_write(p['fort'], struct.pack('<I', 1 if mutation == 1 else 0))
        if address == p['remove'] and mutation == 5:
            mu.mem_write(p['battle'], bytes(4))
        if address == p['check']:
            self.events.append('C:' + str(TARGET))
            if mutation == 2:
                mu.mem_write(p['checking'], bytes(4))
            sp = mu.reg_read(UC_X86_REG_ESP)
            mu.reg_write(UC_X86_REG_EAX, self.control[1])
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, self.read(sp))
            return
        if address == 0x4e0620:
            sp = mu.reg_read(UC_X86_REG_ESP)
            args = [self.read(sp + 4 + 4 * index) for index in range(3)]  # cdecl의 식/파일/행 인자를 그대로 읽는다.
            self.events.append('X:' + self.string(args[0]).hex() + ':' + self.string(args[1]).hex() + ':' + str(args[2]))
            if not self.control[2]:
                raise BoundaryFailure('assertion')
            if mutation == 3:
                mu.mem_write(p['checking'], bytes(4))
                mu.mem_write(p['load'], struct.pack('<I', 1))
                mu.mem_write(p['fort'], struct.pack('<I', 1))
                mu.mem_write(self.slot(TARGET) + 12, struct.pack('<H', 0x1234))
                mu.mem_write(self.slot(TARGET) + 36, struct.pack('<I', 0x6789))
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, sp + 4)
            mu.reg_write(UC_X86_REG_EIP, self.read(sp))
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, case, control, precision):
        """원문 반환/진단 중단 모두에서 전체 raw와 현재 전역을 수집한다."""
        self.control, self.precision = control, precision
        outcome = 'return'
        try:
            self.run_body(case)
        except BoundaryFailure as boundary:
            outcome = str(boundary)
        raw = bytearray(self.mu.mem_read(self.slot(TARGET), self.stride))
        struct.pack_into('<I', raw, 0, self.p['tables'][case['kind']])
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        return (';'.join(self.events), raw.hex(), outcome,
                *(str(self.read(self.p[name])) for name in ('checking', 'load', 'fort', 'battle')))


def main():
    """두 x87 정밀도의 독립 원문 결과를 대조하고 최초 관측을 고정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    oracles = [GunPostPopOracle(), GunPostPopOracle()]
    rows = []
    for case, control in inputs():  # C++ 기대값을 계산하지 않고 두 실제 원문 결과가 일치하는지 확인한다.
        outputs = [oracle.observe(case, control, precision) for oracle, precision in zip(oracles, (0x027f, 0x037f))]
        assert outputs[0] == outputs[1], (case, control)
        rows.append((','.join(str(case[key]) for key in KEYS), ','.join(map(str, control)), *outputs[0]))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('# input14 control5 events raw50 outcome checking load fort server\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), entries=['0046a520', '0046a600', '0046aca0', '0046a810', '00441fa0'],
                  rows=len(rows), observations=2*len(rows), normalReturns=sum(row[4] == 'return' for row in rows),
                  assertionStops=sum(row[4] == 'assertion' for row in rows), diagnosticInputs=sum('X:' in row[2] for row in rows),
                  boundaries=['생성/할당·Form 판정·공통 Damageable·진단은 명시 장치', '게임/발사/공격/MP/진단 UI 실행 없음'])
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
