#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 공통 객체 메뉴 Open/위치의 실제 반환과 효과 순서를 격리 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, VTABLE, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

POOL, EVENT, MENU = 0x13000000, 0x13400000, 0x13401000  # 같은24000슬롯 입력과 별도 사건/현재 메뉴다.
POPULATE, DELEGATE = STOP + 0x400, STOP + 0x420  # 실제 가상+a8/+ac의 서로 다른 명시 경계다.
ENTRIES = (0x4adb30, 0x425f90)  # 공통 Open과 전체 위치 몸체다. _ftol도 실제 명령을 실행한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/object-menu-open-x86.tsv'  # 기존 메뉴 기대값은 보존한다.
REPORT = ROOT / 'cpppj/object-menu-open.json'  # 함수 범위/입력/반환/명시 경계 및 근거SHA다.


def bits(value):
    """독립 float 입력을DWORD로 저장하며 기대 위치를 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """거부/제단 경계/현재TYPE/사건 재조회와 위치 정수 감김을 독립 입력으로 구성한다."""
    base = [0, 0, 0, 0, 0x87654321, 0, 0x100, 320, 220, 500, 170, 77, bits(1.75), bits(180.5), 1024]
    rows = []  # operation/extra/flags/altar/result/mutation/newFlags/eventXY/projectXY/content/range/width/screen다.
    # extra bit1만 조기 거부한다. void/preview/dead에 별도 부모 필터는 없다.
    for extra, flags, altar in itertools.product((0, 1, 2, 8, 9, 0x30, 0xff), (0, 0x400000, 0x100, 0x10000, 0x20000, 0x400100), (0, 1, 51, 23999, 24000, 65535)):
        row = base.copy();row[1:4] = extra, flags, altar;rows.append(row)
    # 외부 몸체가 바꾼TYPE와사건/extra/화면폭은 원본 후속 읽기에만 반영된다.
    for mutation, flags, new_flags in itertools.product(range(1, 7), (0, 0x100), (0, 0x100)):
        row = base.copy();row[2] = flags;row[5:7] = mutation, new_flags;rows.append(row)
    values = (0, bits(-3.75), bits(1.75), bits(2.75), bits(2147483648.0), bits(4294967296.0), 0x7f800000, 0x7fc01234)
    # 위치 함수는 noForm에서사정거리/폭/화면폭을 읽지 않는다. 같은 투영점을 쓰는 양쪽 갈래를 관측한다.
    for point, content, radius, width, screen in itertools.product(((0, 0), (1000, 170), (0x80000000, 0x7fffffff), (0x7ffffff0, 0xffffffff)), (0, 77), values, (bits(180.5), bits(-33.75), 0x4f000000, 0x7fc01234), (1024, 0xffffffff)):
        row = base.copy();row[0] = 1;row[9:15] = *point, content, radius, width, screen;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """공통Open과위치/_ftol은실제CPU로실행하고 GUI·가상호출·투영/종속조회만 명시 경계로 둔다."""
    def __init__(self):
        """보호PE와기존ABI/canary/x87검사를 재사용하며 OS나게임창은시작하지않는다."""
        super().__init__();self.mu.mem_map(POOL, 0x400000);self.mu.mem_map(EVENT, 0x2000)
        self.allowed = [];ranges(self, (*ENTRIES, 0x4e49c0));self.instructions = 0;self.calls = collections.Counter()
        self.put(VTABLE + 0xa8, POPULATE);self.put(VTABLE + 0xac, DELEGATE)

    def mutate(self, boundary):
        """외부 효과의 입력 변경만 공급하며 부모 분기/위치의 기대값을 대신 계산하지 않는다."""
        if self.case[5] == boundary:
            self.put(POOL + 50 * 50 + 10, 101, 1)
        if self.case[5] == 4 and boundary == 3:
            self.put(EVENT + 4, 99);self.put(EVENT + 8, -33)
        if self.case[5] == 5 and boundary == 2:
            self.put(POOL + 50 * 50 + 40, 1, 1)
        if self.case[5] == 6 and boundary == 6:
            self.put(0x5c78f0, 400)

    def instruction(self, mu, address, size, data):
        """원본이 실제 선택한 전체 인자/현재사건/가상receiver를 기록한다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<4I', mu.mem_read(esp + 4, 16));this = mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x49a840:
            if self.get(esp) == 0x425fe7:
                self.event('R', args[0])
            self.ret(TYPES + args[0] * 500);return
        if address == 0x447930:
            self.event('D', (this - POOL) // 50);self.ret(self.case[3]);return
        if address == DELEGATE:
            self.event('V', (this - POOL) // 50, *struct.unpack('<3I', mu.mem_read(args[0], 12)));self.ret(self.case[4], 4);return
        if address == 0x477ae0:
            self.event('B');self.mutate(1);self.ret();return
        if address == 0x4ada70:
            self.event('S', args[0]);self.mutate(2);self.ret();return
        if address == POPULATE:
            self.event('M', (this - POOL) // 50);self.mutate(3);self.ret();return
        if address == 0x497220:
            self.event('P', args[1], args[2]);point = struct.pack('<2I', *self.case[9:11]);mu.mem_write(this, point);mu.mem_write(args[0], point);self.ret(purge=12);return
        if address == 0x4ac9f0:
            self.event('C', (this - POOL) // 50, *args[:3]);self.ret(self.case[11], 12);return
        if address == 0x475c40:
            self.event('W');self.mutate(6);self.ret(MENU);return
        if address == 0x475cf0:
            self.event('A', *args[:3]);self.ret();return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'객체메뉴Open 범위밖실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 명령은 격리 스택에만 쓴다. GUI/월드 외부 효과는 위 명시 경계가 공급한다."""
        if not STACK - 0x7f000 <= address <= STACK + 0x1000 - size:
            raise RuntimeError(f'객체메뉴Open 범위밖쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """각입력을독립설치하고 실제반환/경계순서/현재필드만 출력한다."""
        self.case = case;self.events = [];self.put(0x5c8464, POOL);self.put(0x5c847c, 24000);self.put(0x5c78f0, case[14])
        # 위임대상은raw범위경계까지준비한다. invalid대상은원본이접근하지않는다.
        for sid in (1, 50, 51, 77, 23999):
            raw = bytearray(50);struct.pack_into('<I', raw, 0, VTABLE);raw[10] = 100;raw[40] = case[1] if sid == 50 else 0
            struct.pack_into('<2I', raw, 14, bits(12.5), bits(-3.25));mu = self.mu;mu.mem_write(POOL + sid * 50, bytes(raw))
        self.put(POOL + 77 * 50 + 18, 102, 2);self.put(TYPES + 100 * 500 + 0xec, case[2]);self.put(TYPES + 101 * 500 + 0xec, case[6]);self.put(TYPES + 102 * 500 + 0x44, case[12]);self.put(MENU + 0x3c, case[13])
        self.mu.mem_write(EVENT, struct.pack('<3I', 0x00020000, case[7], case[8]))
        arguments = struct.pack('<I', EVENT if case[0] == 0 else 50)
        self.call(ENTRIES[case[0]], POOL + 50 * 50 if case[0] == 0 else 0, arguments, 4 if case[0] == 0 else 0, precision);self.returns += 1
        result = self.mu.reg_read(UC_X86_REG_EAX) if case[0] == 0 else 0
        current = [self.mu.mem_read(POOL + 50 * 50 + 10, 1)[0], self.mu.mem_read(POOL + 50 * 50 + 40, 1)[0], self.get(EVENT + 4), self.get(EVENT + 8), self.get(0x5c78f0)]
        return str(result), ';'.join(self.events) or '-', ','.join(map(str, current))


def main():
    """새 관측자료를 생성하거나 기존입력/출력/SHA/정상반환수를 재계산 없이 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        # 기대분기를재계산하지않고입력집합과원문바이트/두정밀도기록을검증한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2
        assert not report['precisionDifferentInputs'] and all(len(row) == 4 for row in rows)
        print('객체 메뉴 Open/위치 근거 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = []
    # 두정밀도의전체원본출력을대조하고 C++구현은읽거나호출하지않는다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case;rows.append((','.join(map(str, case)), *outputs[0]))
    FIXTURE.write_text('# input result events current\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_object_menu_open.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), entries=[f'{x:08x}' for x in ENTRIES], controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=[], boundaries='제목4ada70/실제파생+a8/+ac, 장식생성/객체부착, 투영/종속조회/타입조회/현재메뉴조회는 명시입력. 두부모와_ftol만전체실행. 게임/OS실행없음.', sha256={path:digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
