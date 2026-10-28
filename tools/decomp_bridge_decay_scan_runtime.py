#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00422bc0의 현재 주기·슬롯 관문·칸 효과 후 전역 재조회를 실제 PE에서 동결한다."""
import hashlib
import json
import math
import struct
from pathlib import Path
from decomp_bridgedecay_oracle import DecayOracle, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 현재 저장소와 원문 실행만 허용하는 작업 자료/동결 결과 경로다.
ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / 'extracted/cpp-bridge-decay-runtime-550-20261029'
FIXTURE = ROOT / 'cpppj/tests/fixtures/bridge-decay-scan-runtime-x86.tsv'


def bits(value):
    """원문 double의 정밀도·비유한 비트도 정수 열로 보존한다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


class ScanOracle(DecayOracle):
    """실제 한 칸 몸체만 호출 기록/명시 전역 변경으로 바꾸며 스캔 기계어는 그대로 실행한다."""
    def __init__(self):
        super().__init__('originals')
        self.g = dict(self.g, period=0x52f968)  # 기존 분석기의 고정10 입력과 분리한 현재 주기 주소다.
        self.visits = []  # 원문이 선택한 실제 SID 호출 순서다.
        self.mutation = 0  # 첫 칸에서만 적용할 외부 효과 입력 번호다.

    def on_instruction(self, mu, address, size, data):
        """외부 한 칸 호출 직전에 기록하고 ret한다. 계산·필터·커서는 원문이 처리한다."""
        if address != 0x4227e0:
            return super().on_instruction(mu, address, size, data)
        self.visits.append(self.sid_of(mu.reg_read(UC_X86_REG_ECX)))
        if len(self.visits) == 1:
            if self.mutation == 1:
                self.write_global('period', 0.25, '<f')
            elif self.mutation == 2:
                self.write_global('last', 15002, '<i')
            elif self.mutation == 3:
                self.write_global('cursor', 15003, '<i')
            elif self.mutation == 4:
                self.write_global('first', 15002, '<i')
                self.write_global('next', -2.0, '<d')
        esp = mu.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', mu.mem_read(esp, 4))[0]
        mu.reg_write(UC_X86_REG_ESP, esp + 4)
        mu.reg_write(UC_X86_REG_EIP, target)


def main():
    """테스트 입력만 구성하고 기대 방문/커서/기한은 두 정밀도의 실제 PE 실행에서 읽는다."""
    WORK.mkdir(parents=True, exist_ok=True)
    oracle = ScanOracle()
    # 첫 칸 정상/다음 free,dead,void,abstract/여섯째 buried/일곱째 bridge 아닌 타입이다.
    states = (0, 1, 2, 4, 0, 0, 0)
    extras = (0, 0, 0, 0, 1, 8, 0)
    objects = [(15000+i, 30+i, 30, 1, 1, 0x800, 0 if i == 6 else 4, 0, 0,
                states[i], extras[i], 0, 0) for i in range(7)]
    cases = []
    # 주기·게임 delta의 부호/유한 경계와 원문의 double 덧셈 순서를 분리한다.
    for period in (10.0, 7.75, 0.125, 0.0, -1.0, math.inf, math.nan):
        for delta in (0.0, 0.01, 1.0, 10.0, -0.5, 1e20):
            cases.append((period, 15000, 15006, 15000, 20.0, 1.0, delta, 0, 0, 1, 0))
    # 기한 전/같음/지남, 마지막 슬롯 포함, 빈 범위/영역 밖 커서를 관찰한다.
    for cursor in (14999, 15000, 15005, 15006, 15007):
        for now in (9.0, 10.0, 10.01, 16777217.25):
            cases.append((10.0, 15000, 15006, cursor, 10.0, now, 2.0, 0, 0, 1, 0))
    for first, last in ((15000, 15000), (15006, 15000), (15000, 23001)):
        cases.append((10.0, first, last, 15000, 0.0, 1.0, 0.0, 0, 0, 1, 0))
    # 입구의 세 관문은 현재 스캔 상태를 전혀 쓰지 않는다.
    for paused, editor, boss in ((1, 0, 1), (0, 1, 1), (0, 0, 0), (1, 1, 0)):
        cases.append((10.0, 15000, 15006, 15000, 0.0, 1.0, 2.0, paused, editor, boss, 0))
    for mutation in range(1, 5):
        cases.append((10.0, 15000, 15006, 15000, 0.0, 1.0, 2.0, 0, 0, 1, mutation))
    rows = ['# PATCH00422bc0: FPCW,periodBits,first,last,cursor,nextBits,nowBits,deltaBits,paused,editor,boss,mutation => visits,first,last,cursor,nextBits']
    precision_differences = []
    # 각 입력마다 독립 장면을 다시 쓰고 외부 한 칸만 대체한다.
    for index, case in enumerate(cases):
        observations = []
        for control in (0x027f, 0x037f):
            period, first, last, cursor, next_time, now, delta, paused, editor, boss, mutation = case
            oracle.scene(objects, [(0, 7, 1)], b'AP\x01\x00')
            oracle.modes(1, boss, editor, 0, 0, paused)
            oracle.write_global('period', period, '<f')
            oracle.write_global('first', first, '<i')
            oracle.write_global('last', last, '<i')
            oracle.write_global('cursor', cursor, '<i')
            oracle.write_global('next', next_time, '<d')
            oracle.visits = []
            oracle.mutation = mutation
            _, got_cursor, got_next = oracle.scan(now, delta, control)
            wanted = [','.join(map(str, oracle.visits)) or '-', oracle.read_global('first', '<i'),
                      oracle.read_global('last', '<i'), got_cursor, got_next]
            observations.append(wanted)
            if control == 0x027f:
                period_bits = struct.unpack('<I', struct.pack('<f', period))[0]
                row = [control, period_bits, first, last, cursor, bits(next_time), bits(now), bits(delta),
                       paused, editor, boss, mutation, *wanted]
                rows.append('\t'.join(map(str, row)))
        if observations[0] != observations[1]:
            precision_differences.append(index)
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(cases=len(cases), native_observations=len(cases)*2, precision_differences=precision_differences,
                  binary_sha256=oracle.sha256, fixture_sha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  boundary='004227e0만 기록/명시 전역 변경으로 대체; 슬롯/주기/커서/기한은 원문 실행',
                  host='HJOW-Athlon', last_full_decompilation='HJOW-Athlon / 2026-10-23')
    (WORK/'native-scan.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
