#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 ESC 선택 정리 부분과 기존 Select(0,0,0)의 전체 효과/현재 전역을 독립 대조한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import decomp_user_selection as original
from unicorn.x86_const import UC_X86_REG_EIP

TRAMPOLINE = original.STOP + 0x800  # 부분 입구의 부모가 저장한 비휘발 레지스터/지역 스택을 명시 준비한다.
CONTINUE = TRAMPOLINE + 0x40  # 선택 정리 후 커서/메뉴를 실행하지 않고 호출 검사기로 돌아가는 위치다.


class EscapeOracle(original.Oracle):
    """원문 부분 몸체는 바꾸지 않고 진입/후속점만 명시 트램펄린으로 제어한다."""
    def __init__(self, escape):
        """같은 PE/풀/가상 장치를 쓰며 ESC 관측에만 부모 스택을 준비한다."""
        super().__init__()
        self.escape = escape  # False면 기존 전체 Select 입구를 그대로 실행한다.
        prefix = b'\x53\x56\x57\x81\xec' + struct.pack('<I', 0x120)
        self.mu.mem_write(TRAMPOLINE, prefix + b'\xe9' + struct.pack('<i', 0x4d9494-(TRAMPOLINE+len(prefix)+5)))
        self.mu.mem_write(CONTINUE, b'\x81\xc4'+struct.pack('<I', 0x120)+b'\x5f\x5e\x5b\xc3')
        self.allowed.extend(((TRAMPOLINE, TRAMPOLINE+len(prefix)+5), (CONTINUE, CONTINUE+10), (0x4d9494, 0x4d9574)))

    def instruction(self, mu, address, size, data):
        """원문 선택 정리가 끝난 지점에서만 후속 커서/메뉴 대신 부모 반환점으로 간다."""
        if self.escape and address == 0x4d9574:
            mu.reg_write(UC_X86_REG_EIP, CONTINUE)
            return
        return super().instruction(mu, address, size, data)

    def call(self, entry, this, arguments, purge, precision, result=None, after_precision=None):
        """부분 관측도 기존 스택 canary/비휘발/x87 검사를 통과하도록 명시 부모를 빌린다."""
        return super().call(TRAMPOLINE if self.escape else entry, this, arguments, purge, precision, result, after_precision)

    def sample(self, case, precision):
        """사용하지 않는 EAX는0으로 직렬화하고 실제 trace/전역만 비교한다."""
        _, trace, current = super().sample(case, precision)
        return '0', trace, current


def cases():
    """입력 관문/다중 목록/단일 선택/강조와 실제 도달 가능한 콜백 변경을 교차한다."""
    base = [0, 0, 0, 0, 51, 0, 1, 0, 0, 0, 0, 0x10000, 1, 0, 1, 0, 0, 0, 0, 4, 73,
            original.bits(12.5), original.bits(-3.25), original.bits(12.5), original.bits(-3.25), 2, 0, 0, 0, 320, 240]
    rows = []  # 기대 분기는 Python이 계산하지 않는다.
    # enabled/blocked는 비정규 DWORD도 bool로 미리 축소하지 않는다.
    for enabled, blocked, selected, count, variant, mode in itertools.product((0, 1, 0x80000000), (0, 0xffffffff), (0, 51), (0, 2, 5), (0, 1), (0, 4)):
        row = base.copy()
        row[6], row[7], row[4], row[25], row[26], row[19] = enabled, blocked, selected, count, variant, mode
        rows.append(tuple(row))
    # 정지는 이 선택 해제 몸체의 관문이 아니며 통지/프로세스 변경 뒤의 현재 재조회를 비교한다.
    for paused, mutation in itertools.product((0, 1), (1, 2, 7, 11, 12)):
        row = base.copy()
        row[5], row[27] = paused, mutation
        rows.append(tuple(row))
    return list(dict.fromkeys(rows))


def main():
    """새 fixture만 기록하고 두 실제 PE 몸체/두 정밀도의 일치와 독립 출처를 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    select, escape = EscapeOracle(False), EscapeOracle(True)
    rows = ['# input result events current']
    # 모든 입력을 매번 새 전역/풀로 설치하고 x87 두 제어어에서 양쪽 전체 효과를 대조한다.
    for row in cases():
        outputs = [oracle.sample(row, precision) for precision in (0x27f, 0x37f) for oracle in (select, escape)]
        assert all(value == outputs[0] for value in outputs), (row, outputs)
        rows.append(','.join(map(str, row))+'\t'+'\t'.join(outputs[0]))
    args.fixture.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=742, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  uniqueInputs=len(rows)-1, escapeExecutions=escape.returns, selectZeroExecutions=select.returns, controls=[0x27f, 0x37f],
                  nativeEscapeRange='004d9494..004d9573', selectEntry='004d5c90', allEqual=True, eaxCompared=False,
                  wrapper='명시 EBX/ESI/EDI 저장과0x120 지역 스택. 원문9574 직전에 반환하여 커서/메뉴는 실행하지 않는다.',
                  originalGameExecutions=0, fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  peSha256=original.digest(original.ROOT/'originals/Netstorm.exe'),
                  toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  sharedOracleSha256=hashlib.sha256(Path(original.__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(record, ensure_ascii=False))


if __name__ == '__main__':
    main()
