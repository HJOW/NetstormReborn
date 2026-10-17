#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 PE의 내용물 표시 타이머 생성/프레임/빈 통지/소멸을 독립 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

POOL, PROCESS, DESTROY = 0x13600000, 0x13800000, STOP + 0x300  # 기존 관찰과 분리한 부모/24바이트/가상 삭제 경계다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/content-preview-process-x86.tsv'  # 새 원문 관찰이며 기존 fixture를 바꾸지 않는다.
REPORT = ROOT / 'cpppj/content-preview-process.json'  # 실제 두 정밀도의 결과와 보호 입력 SHA다.


def double_bits(value):
    """입력 시각을 원문 IEEE double 비트로 보존한다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def cases():
    """생성 반올림/정확한 만료/비교 불가/진단 재진입/소멸 flags를 독립 입력으로 만든다."""
    clocks = tuple(map(double_bits, (0.0, -0.0, -1.0, 0.1, 1.0000000000000002, 16777217.25, 2**52 + 1.0, 1e200, -1e200, float('inf'), -float('inf')))) + (0x7ff8123456789abc,)
    durations = (0, 0x80000000, 0x3e4ccccd, 0xbe4ccccd, 0x33800000, 0x00800000, 1, 0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc01234)
    rows = []
    # op,clock64,duration32,deadline64,parent,type,head,mutation,flags다. 결과 분기는 Python에 없다.
    for clock, duration, mutation in itertools.product(clocks, durations, (0, 1, 2)):
        # 두 NaN 피연산자의 payload 선택은 별도 x87 연산 복원 범위로 남긴다.
        if clock == clocks[-1] and duration == durations[-1]:
            continue
        rows.append((0, clock, duration, double_bits(2.0), 50, 47, 90, mutation, 0))
    # 현재 공유 타입과 전체 부모 번호는 원문 DWORD 그대로 부착 경계에 전달한다.
    for kind, parent in itertools.product((10, 47, 61, 0xffffffff), (0, 50, 23999)):
        rows.append((0, double_bits(16777217.25), 0x3e4ccccd, 0, parent, kind, 90, 0, 0))
    frame_times = clocks + tuple(map(double_bits, (1.9999999999999998, 2.0, 2.0000000000000004)))
    deadlines = tuple(map(double_bits, (-float('inf'), -1.0, 0.0, 2.0, float('inf')))) + (0x7ff8123456789abc,)
    # 41bfb0의 진단은 원래 부모 포인터를 고정한다. 콜백이 필드를 바꿔도 삭제 대상은 원래 부모다.
    for now, deadline, head, mutation in itertools.product(frame_times, deadlines, (0, 90), (0, 3, 4)):
        rows.append((1, now, 0, deadline, 50, 47, head, mutation, 0))
    # 실제 빈 통지는 부모/시각조차 읽지 않는다. 소멸은 두 DWORD와 VT만 바꾼다.
    for op, flags in itertools.product((2, 3), (0, 1, 2, 3, 0x80000000, 0xffffffff)):
        rows.append((op, double_bits(2.0), 0x7fc01234, double_bits(2.25), 50, 47, 0, 0, flags))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """실제7개 몸체만 실행하며 부착/부모 가상 삭제/할당 해제/진단은 명시 경계다."""
    def __init__(self):
        """정밀 표에 누락된 짧은 몸체도 보호 PE의 실제 ret 끝 범위만 허용한다."""
        super().__init__();self.mu.mem_map(POOL, 0x200000);self.mu.mem_map(PROCESS, 0x1000)
        self.allowed = [(0x428700, 0x42876f), (0x428770, 0x428792), (0x480260, 0x480263), (0x468230, 0x46824e), (0x4ab2c0, 0x4ab2c5), (0x41bd00, 0x41bd12), (0x41bfb0, 0x41bfe1)]
        self.instructions = 0;self.calls = collections.Counter();self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.put(STOP + 0x400 + 0x10, DESTROY)

    def instruction(self, mu, address, size, data):
        """보호 몸체가 고른 현재 인자/바이트만 기록하며 만료 판단은 대체하지 않는다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<4I', mu.mem_read(esp + 4, 16));this = mu.reg_read(UC_X86_REG_ECX)
        if address in tuple(low for low, _ in self.allowed):
            self.calls[f'{address:08x}'] += 1
        if address == 0x41bd20:
            self.event('A', *args, bytes(mu.mem_read(PROCESS, 24)).hex());self.put(PROCESS + 4, 90);self.put(PROCESS + 8, args[1])
            if self.case[7] == 1:
                self.put(0x55b4d0, double_bits(17.25), 8)
            if self.case[7] == 2:
                self.put(PROCESS + 16, double_bits(7.25), 8)
            self.ret(PROCESS, 16);return
        if address == 0x4e0620:
            self.event('!', self.string(args[0]), args[2])
            if self.case[7] == 3:
                self.put(PROCESS + 8, 51)
            self.ret();return
        if address == DESTROY:
            self.event('D', (this - POOL) // 50, args[0])
            if self.case[7] == 4:
                mu.mem_write(PROCESS, b'\xcc' * 24)
            self.ret(0, 4);return
        if address == 0x4e4354:
            assert args[0] == PROCESS;self.event('F', bytes(mu.mem_read(PROCESS, 24)).hex());self.ret();return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'내용물 타이머 범위밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 타이머24바이트/SEH/스택 이외의 원본 명령 쓰기를 거부한다."""
        if STACK - 0x7f000 <= address <= STACK + 0x1000 - size or address == 0 and size == 4 or PROCESS <= address <= PROCESS + 24 - size:
            return
        raise RuntimeError(f'내용물 타이머 범위밖 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지와 실제 입력/스택/가상 슬롯만 읽게 해 NULL 자료 의존을 막는다."""
        regions = ((0, 4), (0x400000, 0x600000), (STACK - 0x7f000, STACK + 0x1000), (POOL, POOL + 0x200000), (PROCESS, PROCESS + 24), (STOP + 0x400, STOP + 0x480))
        if not any(low <= address and address + size <= high for low, high in regions):
            raise RuntimeError(f'내용물 타이머 범위밖 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """매 입력마다 raw/시계/SEH를 초기화하고 정상 반환 뒤24바이트 전체를 관측한다."""
        self.case = case;self.events = [];op, now, duration, deadline, parent, kind, head, mutation, flags = case
        raw = bytearray(b'\xa5' * 24);struct.pack_into('<3I', raw, 0, 0x503c1c, 90, parent);struct.pack_into('<Q', raw, 16, deadline);self.mu.mem_write(PROCESS, bytes(raw))
        self.put(0x55b4d0, now, 8);self.put(0x54112c, kind);self.put(0x5c8464, POOL);self.put(0, 0x87654321)
        self.put(POOL + parent * 50, STOP + 0x400);self.put(POOL + parent * 50 + 6, head, 2)
        entry = (0x428700, 0x428770, 0x480260, 0x468230)[op]
        args = struct.pack('<2I', parent, duration) if op == 0 else b'' if op == 1 else struct.pack('<3I', flags, 0x80000000, 0xffffffff) if op == 2 else struct.pack('<I', flags)
        self.call(entry, PROCESS, args, (8, 0, 12, 4)[op], precision, PROCESS if op in (0, 3) else None);self.returns += 1
        assert self.get(0) == 0x87654321
        return bytes(self.mu.mem_read(PROCESS, 24)).hex(), ';'.join(self.events) or '-'


def main():
    """새 격리 관찰을 기록하거나 기존 기대값/입력/보호SHA를 변경 없이 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2
        assert [index for index, row in enumerate(rows) if row[1:3] != row[3:5]] == report['precisionDifferentInputs']
        print('내용물 타이머 원본 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = [];different = []
    # 제품 C++를 읽거나 실행하지 않고 실제 PE의 두 정밀도 관측을 각각 보존한다.
    for index, case in enumerate(cases()):
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        if outputs[0] != outputs[1]:
            different.append(index)
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input raw53 events53 raw64 events64\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_content_preview_process.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_particle_gradient.py', 'cpppj/tests/fixtures/content-preview-process-x86.tsv')
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=different, boundaries='부착41bd20/부모가상삭제/할당해제/진단은명시경계. 428700/428770/480260/468230/4ab2c0/41bd00/41bfb0전체실행. 부모삭제가Host수명을정리하는범위는별도C++검사. 두NaN피연산자동시덧셈payload선택은범위밖. 원본/OS창실행없음.', sha256={path:digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls', 'precisionDifferentInputs')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
