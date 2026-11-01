#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tab 최근 건설 위치의 실제 순환/좌표 유효성 및 최초5칸 초기화를 동결한다(718)."""
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys
from decomp_oracle import Oracle as BaseOracle, STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 같은 PC의 정밀 자료와 보호 PE만 읽는다.
HISTORY, INDEX, CELLS = 0x558ea4, 0x558e78, 0x531928  # 원문의40BYTE/순번DWORD/세계 크기 signed DWORD다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/construction-history-key-x86.tsv'  # 최초 관측 뒤 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/construction-history-key-native-20261102.json'  # 실행 범위/큰 경계/정밀도 결과를 구별한다.
WORK = ROOT / 'extracted/cpp-construction-history-718'  # 전체 원시 관측의 Git 제외 경로다.
INVALID = 0xc61c4000  # 실제004af170이 초기화하는545370/374의 -10000.0f다.


def digest(path):
    """원문/도구/최초 fixture의 전체 물리 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """입력 float만 raw DWORD로 기록하며 기대 분기를 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def image(points):
    """다섯 x/y의 원시 비트를 순서대로 직렬화한다."""
    return struct.pack('<10I', *(word for point in points for word in point)).hex()


class PartialStop(Exception):
    """Center 경계에서 요청한 명시 중단이며 정상 복귀와 구별한다."""


class Oracle(BaseOracle):
    """실제 키 부모·Point.IsValid·최초 초기화만 실행하고 정지/즉시 카메라는 명시 경계로 둔다."""
    def __init__(self):
        """원문 코드/전역을 같은 격리 메모리에 적재한다."""
        super().__init__('originals', [dict(name='unused', originals='004d62b0')], helpers=[])
        self.allowed = [(0x4d96b6, 0x4d9bbb), (0x40e800, 0x40e84d), (0x43994c, 0x439966)]
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def dword(self, address, value=None):
        """원시 네 바이트만 읽고 입력/명시 callback 변경만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def guard(self, mu, access, address, size, value, data):
        """실제 CPU 쓰기는 같은 스택·40BYTE 이력·순번 네 바이트로 제한한다."""
        assert STACK - 0x100 <= address and address + size <= STACK + 0x1000 or HISTORY <= address and address + size <= HISTORY + 40 or address == INDEX and size == 4, (hex(address), size)

    def returned(self, value=0):
        """현재 cdecl 경계의 반환 주소만 소비하고 caller의 인자 정리는 실제 원문에 맡긴다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EIP, self.dword(sp))

    def on_instruction(self, mu, address, size, data):
        """Point의 x87 비교는 실제 실행하고 큰 정지/Center만 현재 입력 장치로 제공한다."""
        if address == 0x460de0:
            self.trace.append('P:' + str(self.case['paused']))
            self.returned(self.case['paused'])
            return
        if address == 0x433aa0:
            sp = mu.reg_read(UC_X86_REG_ESP)
            self.trace.append(f'C:{self.dword(sp + 4)}:{self.dword(sp + 8)}')
            if self.case['mutate']:
                self.dword(INDEX, self.case['callbackIndex'])
                self.dword(HISTORY, INVALID)
                self.dword(HISTORY + 4, bits(250.25))
            if self.case['stop']:
                raise PartialStop()
            self.returned()
            return
        if address == 0x40e800:
            self.point_checks += 1
        return super().on_instruction(mu, address, size, data)

    def run(self, case, control):
        """원문 순환 뒤의 이력/현재 순번·실제 호출·x87/스택을 관측한다."""
        self.case, self.trace, self.point_checks = case, [], 0
        self.mu.mem_write(HISTORY, bytes.fromhex(case['points']))
        self.dword(INDEX, case['index'])
        self.dword(CELLS, case['cells'])
        self.mu.mem_write(STACK - 0x100, bytes(0x1100))
        self.dword(STACK + 0x14, case['event'])
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ECX, INVALID)
        self.mu.reg_write(UC_X86_REG_EDX, INVALID)
        start, end = (0x43994c, 0x439966) if case['kind'] == 'Init' else (0x4d96b6, 0x4d9bbb)
        completed = True
        try:
            self.mu.emu_start(start, end, timeout=1000000, count=4000)
        except PartialStop:
            completed = False
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        if completed:
            assert self.mu.reg_read(UC_X86_REG_EIP) == end and self.mu.reg_read(UC_X86_REG_ESP) == STACK
        return dict(points=bytes(self.mu.mem_read(HISTORY, 40)).hex(), index=self.dword(INDEX), trace=';'.join(self.trace) or '-', completed=int(completed), pointChecks=self.point_checks, fpsw=self.mu.reg_read(UC_X86_REG_FPSW), stackDelta=self.mu.reg_read(UC_X86_REG_ESP) - STACK)


def inputs():
    """이력/순번/수정키·정지·좌표 경계와 재진입 입력만 만들고 기대 결과는 계산하지 않는다."""
    valid = [(bits(20 + i * 17.25), bits(30 + i * 9.5)) for i in range(5)]  # 각 실제 Center 인자를 구별한다.
    empty = [(INVALID, INVALID)] * 5
    patterns = [empty, valid, valid[:2] + empty[2:], empty[:1] + valid[1:], valid[:2] + empty[2:3] + valid[3:]]
    rows = []
    for points, index, paused, modifiers in itertools.product(patterns, range(5), (0, 1), range(8)):  # 모든 수정키 조합은 원문과 같은 별도 상위 비트다.
        flags = ((modifiers & 1) * 0x20000000) | ((modifiers & 2) * 0x08000000) | ((modifiers & 4) * 0x02000000)
        rows.append(dict(kind='Key', event=9 | flags, points=image(points), index=index, cells=256, paused=paused, mutate=0, callbackIndex=0, stop=0))
    edges = [(0, bits(1)), (0x80000000, bits(1)), (bits(1), 0), (1, 1), (bits(1), bits(1)), (0x437fffff, bits(1)), (bits(256), bits(1)), (0x43800001, bits(1)), (0x7f800000, bits(1)), (0xff800000, bits(1)), (0x7fc12345, bits(1)), (bits(1), 0x7fc12345), (bits(-1), bits(1)), (0x4f000000, bits(1))]
    for point, cells in itertools.product(edges, (0, 1, 2, 256, 0x7fffffff, 0xffffffff)):  # signed 세계 크기→단정도 상한과 엄격한 양 축 비교를 실제 기계어에 맡긴다.
        rows.append(dict(kind='Key', event=9, points=image([point] + valid[1:]), index=0, cells=cells, paused=0, mutate=0, callbackIndex=0, stop=0))
    for index, changed, stop in itertools.product((0, 1), (0, 4, 5, 0xffffffff, 0x7fffffff, 0x80000000), (0, 1)):  # 카메라 복귀 뒤 현재 순번의 signed 나머지와 부분 쓰기를 구별한다.
        rows.append(dict(kind='Key', event=9, points=image(valid), index=index, cells=256, paused=0, mutate=1, callbackIndex=changed, stop=stop))
    for index in (0, 4, 0xffffffff):  # 최초5칸 쓰기는 현재 순번을 초기화하지 않는 원문을 관측한다.
        rows.append(dict(kind='Init', event=0, points=image(valid), index=index, cells=256, paused=0, mutate=0, callbackIndex=0, stop=0))
    return rows


def main():
    """최초 원문 기대값만 생성하며 verify는 이후 자료의 해시만 확인한다."""
    if '--verify' in sys.argv:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():  # 제품 변경에 맞춰 최초 fixture를 다시 쓰지 않는다.
            assert digest(ROOT / name) == expected, name
        print('Tab 최근 건설 위치 최초 원문 보존 확인')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle = Oracle()
    rows, observations = [], []
    for case in inputs():  # 원문이 반환한 전체 이력/현재 순번과 호출만 고정한다.
        outputs = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert outputs[0] == outputs[1] == outputs[2], case
        result = outputs[0]
        rows.append([case[name] for name in ('kind', 'event', 'points', 'index', 'cells', 'paused', 'mutate', 'callbackIndex', 'stop')] + [result[name] for name in ('points', 'index', 'trace', 'completed', 'pointChecks', 'fpsw', 'stackDelta')])
        observations.append(dict(input=case, observations=outputs))
    FIXTURE.write_text('# kind event points index cells paused mutate callbackIndex stop outPoints outIndex trace completed pointChecks fpsw stackDelta\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows), 'utf-8', newline='\n')
    (WORK / 'native-observations.json').write_text(json.dumps(observations, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    paths = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/raw.c', 'tools/decomp_construction_history_key.py', 'tools/decomp_oracle.py', FIXTURE.relative_to(ROOT).as_posix()]
    record = dict(unit=718, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(rows), observations=len(rows) * 3, normalReturns=sum(row[12] for row in rows), partialStops=sum(not row[12] for row in rows), precisionOrRepeatDifferences=0, gameExecutions=0, multiplayerTests=0, ranges=['004d96b6..004d9bba', '0040e800..0040e84c', '0043994c..00439965'], boundaries=['정지00460de0/즉시 중심00433aa0만 명시 장치이며 실제 x87 Point 유효성을 실행한다.', '초기 순번은 물리0..4다. Center callback이 바꾼 순번의 signed 나머지/실패 부분 효과도 관측한다.', '뗌/Console/Guide/Gump 관문은 부분 실행 앞의 경계다. 전체 GUI/건설 월드/OS/통신을 실행한 관측은 아니다.', 'x87 조건/NaN 상태는 관측하며 무조건FPSW0으로 판정하지 않는다.'], sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps({key: value for key, value in record.items() if key not in ('sha256', 'boundaries')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
