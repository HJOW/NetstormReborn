#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CanonDecoder 다음 칸의 현재 checking·명시 프레임 진단과 복귀를 원문에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestgeometry_oracle import GeometryOracle, DEC, CODES, TYPES
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

CHECKING = 0x5e4794  # 실제 Client 진단 DWORD의 원본 주소다.


def inputs():
    """두 유효 프레임·빈 기본값·잘못된 명시값과 진단의 현재 상태를 교차한다."""
    rows = list(itertools.product((0, 1), (-1, 0, 1), (0, 1, 0x80000000), (0, 0x20), (0, 1), (0, 1)))
    rows.extend((1, 2, checking, cracked, 0, mutation) for checking, cracked, mutation in itertools.product((0, 1, 0x80000000), (0, 0x20), (0, 1)))  # 물리 코드 범위 밖은 원문 진단에서 중단시킨다.
    return rows


class CanonCheckingOracle(GeometryOracle):
    """정상 다음 칸 전체와 현재 진단의 선택/복귀만 격리한다."""
    def __init__(self):
        """기존 정밀 Canon 함수 범위와 분석용 프레임/객체 저장소를 사용한다."""
        super().__init__('originals')

    def string(self, address):
        """NUL 종료된 원문 식/파일을 변환 없이 읽는다."""
        result = bytearray()
        for offset in range(512):  # 잘못된 포인터를 무한히 읽지 않는다.
            value = self.mu.mem_read(address + offset, 1)[0]
            if not value:
                return bytes(result)
            result.append(value)
        raise ValueError('Canon 진단 문자열 종료 없음')

    def on_instruction(self, mu, address, size, data):
        """진단은 사건과 원문 인자를 남기며 복귀 시 현재 checking만 바꿀 수 있다."""
        if address == 0x4e0620:
            sp = mu.reg_read(UC_X86_REG_ESP)
            args = [int.from_bytes(mu.mem_read(sp+4+4*i, 4), 'little') for i in range(3)]  # 실제 cdecl 식/파일/행 인자다.
            self.events.append('X:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.case[4]:
                raise BoundaryFailure('assertion')
            if self.case[5]:
                mu.mem_write(CHECKING, bytes(4))
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, sp+4)
            mu.reg_write(UC_X86_REG_EIP, int.from_bytes(mu.mem_read(sp, 4), 'little'))
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, case, precision):
        """생성 초기값과 같은 반복자를 준비하고 첫 칸/끝 상태를 원문으로 구한다."""
        self.case, self.events = case, []
        explicit, frame, checking, cracked, _, _ = case
        typ = TYPES+158*self.type_stride
        self.mu.mem_write(typ, bytes(self.type_stride))
        self.mu.mem_write(typ+0x114, struct.pack('<Ii', 2, frame))
        self.mu.mem_write(typ+0x124, struct.pack('<I', CODES))
        self.mu.mem_write(CODES, bytes((65, 80, 0, cracked, 65, 80, 1, cracked)))
        raw = bytearray(84)
        for offset, value in ((0, 158), (4, frame), (16, -1), (20, 1), (28, 1), (40, typ), (56, 1), (60, 1), (80, explicit)):  # 현재 비패턴 생성자의 의미 있는 초기 필드만 공급한다.
            struct.pack_into('<I', raw, offset, value & 0xffffffff)
        struct.pack_into('<2f', raw, 44, 20.5, -3.25)
        self.mu.mem_write(DEC, bytes(raw))
        self.mu.mem_write(CHECKING, struct.pack('<I', checking))
        outcome, first, last = 'return', '-', '-'
        try:
            self.call(self.s['next'], [], precision)
            first = ','.join(map(str, self.snapshot()))
            if self.snapshot()[1]:
                self.call(self.s['next'], [], precision)
            last = ','.join(map(str, self.snapshot()))
        except BoundaryFailure as boundary:
            outcome = str(boundary)
        return (';'.join(self.events), outcome, first, last, str(int.from_bytes(self.mu.mem_read(CHECKING, 4), 'little')))


def main():
    """최초 원문 결과와 두 정밀도 일치 여부를 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    oracles, rows = [CanonCheckingOracle(), CanonCheckingOracle()], []
    for case in inputs():  # C++ 결과를 기대값으로 사용하지 않는다.
        values = [oracle.observe(case, precision) for oracle, precision in zip(oracles, (0x027f, 0x037f))]
        assert values[0] == values[1], case
        rows.append((','.join(map(str, case)), *values[0]))
    args.output.write_text('# input6 events outcome firstState lastState checking\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), entry='00425860', rows=len(rows), observations=2*len(rows),
                  assertionStops=sum(row[2]=='assertion' for row in rows), diagnosticInputs=sum(bool(row[1]) for row in rows),
                  gameExecutions=0, boundaries=['비패턴 한 칸/끝 순회·진단은 명시 장치', '물리 프레임 코드 범위 밖은 진단에서 중단·게임/GUI/MP 없음'])
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
