#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 Guide 앞 F12의 전체화면/뗌 관문과 실제 마우스 모드 인자를 격리 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import sys

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 선택 의존성을 읽는 실제 저장소다.
sys.path.insert(0, str(ROOT/'extracted/oracle-python'))
import decomp_production_shortcuts as original
from unicorn.x86_const import *

ENTRY, STOP = 0x4d6e34, 0x4d6e61  # F12 검사부터 현재 Guide 호출 준비 직전까지의 원문이다.
FLAGS = 0x5c78d4  # 원문이 매 사건에서 직접 읽는 현재 화면 모드 DWORD다.


class MouseKeyOracle(original.Oracle):
    """현재 사건의 ECX와 부모 스택을 공급하고 실제 화면 장치 CALL만 관측한다."""
    def __init__(self):
        """기존 읽기 전용 원문 적재와 스택 쓰기 보호를 재사용한다."""
        super().__init__()
        self.original[ENTRY] = bytes(self.mu.mem_read(ENTRY, STOP-ENTRY))  # 관측할 앞쪽 몸체도 매 호출 불변을 확인한다.

    def instruction(self, mu, address, size, data):
        """현재 모드 읽기/장치 인자/Guide 앞 도달 순서를 원문 명령에서 기록한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        if address == 0x435ba0:
            self.event('M', (self.number(mu.reg_read(UC_X86_REG_ESP)+4),))
            self.returned()
            return
        if not ENTRY <= address < STOP or address+size > STOP:
            raise RuntimeError(f'미명시 F12 실행 주소 {address:08x}')
        if address == 0x4d6e4c:
            self.event('R', (self.number(FLAGS),))
        self.instructions += 1

    def run_key(self, code, flags, control):
        """기대 분기를 계산하지 않고 원문의 trace/스택/비휘발/x87 상태를 읽는다."""
        self.trace, self.instructions, self.completed = [], 0, False
        self.number(FLAGS, flags)
        self.mu.mem_write(original.STACK-64, b'\xa5'*128)
        saved = {UC_X86_REG_EBX:0x12345678, UC_X86_REG_ESI:0x87654321, UC_X86_REG_EDI:0, UC_X86_REG_EBP:0x19283746}  # 실제 부모의 비휘발 입력이다.
        for register, value in saved.items():  # 호출 전 비휘발 값을 독립적으로 공급한다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, code)
        self.mu.reg_write(UC_X86_REG_EAX, 0)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_ESP, original.STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.emu_start(ENTRY, STOP+1, count=1000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP) == original.STACK
        assert bytes(self.mu.mem_read(original.STACK-64, 48)) == b'\xa5'*48 and bytes(self.mu.mem_read(original.STACK+16, 48)) == b'\xa5'*48
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control and self.mu.reg_read(UC_X86_REG_FPSW) == 0
        for address, expected in self.original.items():  # 기존 범위와 F12 원문 모두 CPU 관측 전후 같아야 한다.
            assert bytes(self.mu.mem_read(address, len(expected))) == expected
        return ';'.join(self.trace) or '-'


def main():
    """상위 입력 표시/수정키/뗌/전체 DWORD와 다른 사건의 독립 관측을 새 fixture로 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle = MouseKeyOracle()
    modes = (0,10,1,65,171,235,0xffffffff,0x80000001)  # 유효 장치 실행과 구별하는 명시 원시 DWORD 입력이다.
    modifier_bits = (0x20000000,0x10000000,0x08000000,0x40000000,0x04000000)  # Shift/Control/Alt/폴링 표시의 실제 비트다.
    inputs = []
    for combination, release, flags in itertools.product(range(32), (0,0x80000000), modes):  # 상위 비트가 정확한 사건 마스크에 미치는 영향을 교차한다.
        code = 0x7b0000 | release | sum(bit for index, bit in enumerate(modifier_bits) if combination & (1 << index))
        inputs.append((code,flags))
    inputs.extend(itertools.product((0,0x7b,0x7a0000,0x7c0000,0x027b0000), modes))
    rows = ['# code,flags\ttrace']
    for code, flags in inputs:  # 두 정밀도에서 실제 몸체를 실행하고 동일한 관측만 한 행으로 고정한다.
        first = oracle.run_key(code,flags,0x27f)
        assert first == oracle.run_key(code,flags,0x37f)
        rows.append(f'{code},{flags}\t{first}')
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record = dict(unit=744, date='2026-11-02', host=socket.gethostname(), lastFullDecompile=dict(host='HJOW-Athlon',date='2026-10-23'), inputs=len(inputs), executions=len(inputs)*2, controls=[0x27f,0x37f], nativeRange='004d6e34..004d6e60', stopBefore='004d6e61 Guide', stackDelta=0, explicitBoundary='00435ba0 화면 장치의 실제 단일 DWORD 인자만 관찰', gameProcesses=0, fixtureSha256=original.digest(args.fixture), peSha256=original.digest(original.PE_PATH), toolSha256=original.digest(Path(__file__)), sharedOracleSha256=original.digest(Path(original.__file__)))
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(record,ensure_ascii=False))


if __name__ == '__main__':
    main()
