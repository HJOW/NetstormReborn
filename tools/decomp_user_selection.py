#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 세 인자 선택·다중 선택 정리·사제/Carrier/Archer 메뉴 입구를 격리 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, VTABLE, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

POOL, MULTIPLE, PROCESS, EVENT = 0x13000000, 0x13400000, 0x13401000, 0x13402000  # 실제 범위 경계와 분리된 입력 영역이다.
SELECT, REFRESH, NOTICE = STOP + 0x400, STOP + 0x420, STOP + 0x440  # 파생 가상+7c/+88/프로세스+1c의 명시 경계다.
ENTRIES = (0x4d5c90, 0x4d5490, 0x4d5440, 0x4268c0, 0x41e660, 0x492080)  # operation0..5가 실행하는 전체 몸체다.
HELPERS = (0x40e850, 0x40ea40, 0x4d5bb0, 0x4d5480, 0x4b22d0, 0x4b1b80, 0x4b2340, 0x425e80, 0x4b1bf0, 0x4ac1e0, 0x4ac200)  # 비교/강조/ProcessFinder/flags 조회도 대체하지 않는다.
GLOBALS = (0x5caea0, 0x5caea4, 0x5caf38, 0x5caf3c, 0x5caf34, 0x5cae90, 0x5434e4)  # 반환 뒤 실제 현재 저장소를 읽을 순서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/user-selection-x86.tsv'  # 기존 강조/메뉴 관찰을 덮어쓰지 않는 새 자료다.
REPORT = ROOT / 'cpppj/user-selection.json'  # 관찰 몸체·경계·ABI·SHA를 기록한다.


def bits(value):
    """기대 분기를 계산하지 않고 독립 float 입력의 원문 비트를 보관한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """세 인자 차이/기존 선택/관문/다중 선택/콜백 재조회와 제단 반복을 교차한다."""
    # op,sid,force,activate,selected,paused,enabled,blocked,state,extra,flags1,flags2,owner,editor,local,movable,content,moving,altar,mode,mouseType,rememberXY,xy,multiCount,multiVariant,mutation,baseResult,eventXY다.
    base = [0, 50, 0, 0, 51, 0, 1, 0, 0, 0, 0, 0x10000, 1, 0, 1, 0, 0, 0, 0, 4, 73, bits(12.5), bits(-3.25), bits(12.5), bits(-3.25), 2, 0, 0, 0x87654321, 320, 240]
    rows = []  # 실제 PE 출력과 독립적인 입력만 준비한다.
    # pause는 nonzero 대상에서만 조회하고 enabled는0 대상 조기 거부에만 관여한다.
    for sid, paused, enabled, blocked, old in itertools.product((0, 50), (0, 1), (0, 1), (0, 1), (0, 51)):
        row = base.copy();row[1] = sid;row[4:8] = old, paused, enabled, blocked;rows.append(row)
    # 둘째/셋째 DWORD의 역할을 섞지 않는 선택/이동 관문과 owner/editor를 관찰한다.
    for force, activate, flags1, flags2, extra, owner, editor in itertools.product((0, 1), (0, 1), (0, 0x08000000, 0x10000000, 0x18000000), (0, 0x10000, 0x20000), (0, 1, 8, 9), (1, 2), (0, 1)):
        row = base.copy();row[2:4] = force, activate;row[9:14] = extra, flags1, flags2, owner, editor;rows.append(row)
    # state2만 대상 선택을 거부한다. free/contained/void 비트는 별도 조기 필터가 아니다.
    for state, movable, content, moving in itertools.product((0, 1, 2, 3, 4, 8, 0xff), (0, 1), (0, 72), (0, 1)):
        row = base.copy();row[8] = state;row[15:18] = movable, content, moving;row[3] = 1;rows.append(row)
    # 40e850은 float==다. 부호0·NaN·Infinity의 원문과 Remember의 조건부 쓰기를 관측한다.
    for x, remembered, extra, flags2 in itertools.product((0, 0x80000000, bits(12.5), 0x7fc01234, 0x7f800000), (0, 0x80000000, bits(12.5), 0x7fc01234, 0x7f800000), (0, 8), (0, 0x10000)):
        row = base.copy();row[23] = x;row[21] = remembered;row[9] = extra;row[11] = flags2;rows.append(row)
    # callback의 현재 입력 변경은 다음 원본 읽기에만 영향을 준다.
    for mutation, operation in itertools.product(range(1, 13), (0, 3, 4, 5)):
        row = base.copy();row[0] = operation;row[27] = mutation;row[3] = 1;row[15:17] = 1, 72;rows.append(row)
    # 제단의0/유효/경계밖 번호와 반복의 관문을 직접 실행한다.
    for altar, mutation in itertools.product((0, 1, 52, 23999, 24000, 65535), (0, 9)):
        row = base.copy();row[18] = altar;row[27] = mutation;rows.append(row)
    # 짧은 세 메뉴 입구는 extra1일 때 선택을 건너뛰지만 공통+aс는 항상 호출한다.
    for operation, extra, paused in itertools.product((3, 4, 5), (0, 1, 8, 9, 0xff), (0, 1)):
        row = base.copy();row[0] = operation;row[9] = extra;row[5] = paused;rows.append(row)
    # 다중 선택 정리는 signed 개수/물리 범위/state&3와 통지 뒤 개수 변경을 보존한다.
    for count, variant, mutation in itertools.product((0, 1, 2, 5, 0xffffffff), (0, 1), (0, 1, 12)):
        row = base.copy();row[0] = 1;row[25:28] = count, variant, mutation;rows.append(row)
    # 별도 Remember의 DWORD bit30000만 조건이다.
    for flags in (0, 0x10000, 0x20000, 0x30000, 0xffffffff):
        row = base.copy();row[0] = 2;row[11] = flags;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """선택 부모/관련 짧은 몸체/ProcessFinder/float비교/강조는 실제 CPU로 실행한다."""
    def __init__(self):
        """보호 PE와 원본 범위 및 ABI 검사를 준비하며 게임이나 창은 실행하지 않는다."""
        super().__init__();self.mu.mem_map(POOL, 0x400000);self.mu.mem_map(MULTIPLE, 0x3000)
        self.allowed = [(0x425e80, 0x425e88)];ranges(self, (*ENTRIES, *(entry for entry in HELPERS if entry != 0x425e80)));self.instructions = 0;self.calls = collections.Counter()
        self.put(VTABLE + 0x7c, SELECT);self.put(VTABLE + 0x88, REFRESH);self.put(VTABLE + 0x1c, NOTICE);self.put(PROCESS, VTABLE)

    def mutate(self, boundary):
        """외부 몸체가 바꾸는 독립 입력만 공급하며 선택/반환 기대값은 계산하지 않는다."""
        if self.case[27] != boundary:
            return
        if boundary == 1:
            self.put(0x5caf34, 1);self.put(0x5caea4, 77)
        elif boundary == 2:
            self.put(0x5caea0, 99)
        elif boundary == 3:
            self.put(POOL + 50 * 50 + 10, 101, 1)
        elif boundary == 4:
            self.put(POOL + 50 * 50 + 10, 102, 1)
        elif boundary == 5:
            self.put(POOL + 50 * 50 + 40, 1, 1)
        elif boundary == 6:
            self.put(0x5caea0, 0)
        elif boundary == 7:
            self.put(0x5caf38, self.case[23]);self.put(0x5caf3c, self.case[24])
        elif boundary == 8:
            self.put(0x5caea0, 202);self.put(0x5caea4, 77)
        elif boundary == 9:
            self.put(0x5434e4, 0);self.put(0x5c89b8, 1)
        elif boundary == 10:
            self.put(0x5caea0, 99);self.put(0x5caea4, 77)
        elif boundary == 11:
            self.put(POOL + 60 * 50 + 4, 65, 2)
        elif boundary == 12:
            self.put(0x5caf34, 5)

    def instruction(self, mu, address, size, data):
        """실제 하위의 전체 인자/현재 선택을 기록하고 큰 외부 계산과 장치만 경계로 둔다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<4I', mu.mem_read(esp + 4, 16));this = mu.reg_read(UC_X86_REG_ECX)
        if address in (*ENTRIES, *HELPERS):
            self.calls[f'{address:08x}'] += 1
        if address == 0x460de0:
            self.event('P');self.ret(self.case[5]);return
        if address == 0x463b50:
            self.event('B');self.ret(self.get(0x5c89b8));return
        if address == SELECT:
            sid = (this - POOL) // 50;self.event('N', sid, args[0], self.get(0x5caea0));self.mutate(1 if sid == 55 else 12 if sid == 56 else 2 if sid == 51 else 6 if args[0] else 0);self.ret(purge=4);return
        if address == REFRESH:
            sid = (this - POOL) // 50;self.event('R', sid, self.get(0x5caea0));self.mutate(7 if sid != 51 else 0);self.ret();return
        if address == 0x4ae2d0:
            self.event('F', (this - POOL) // 50);self.ret(PROCESS);return
        if address == NOTICE:
            self.event('Q', *args[:3]);self.mutate(11);self.ret(purge=12);return
        if address == 0x44dab0:
            self.event('D', args[0]);self.mutate(3);self.ret();return
        if address == 0x49a840:
            self.ret(TYPES + args[0] * 500);return
        if address in (0x4741d0, 0x474900):
            self.event('T' if address == 0x4741d0 else 'C', *args[:2]);self.mutate(5 if address == 0x474900 else 0);self.ret();return
        if address == 0x474240:
            self.event('V', args[0]);self.mutate(4);self.ret(self.case[15]);return
        if address == 0x4ac9f0:
            self.event('I', (this - POOL) // 50, *args[:3]);self.ret(self.case[16], 12);return
        if address == 0x447930:
            self.event('A', (this - POOL) // 50);self.mutate(9);self.ret(self.case[18]);return
        if address == 0x425ed0:
            self.event('M', args[0]);self.ret(self.case[17]);return
        if address == 0x426080:
            self.event('E', args[0]);self.ret();return
        if address == 0x4a4410:
            self.event('K', args[0]);self.ret();return
        if address == 0x4971e0:
            self.event('J', args[1], args[2]);mu.mem_write(args[0], struct.pack('<2I', bits(7.5), bits(9.25)));self.mutate(8);self.ret(args[0]);return
        if address == 0x4284a0:
            self.event('H', *args);self.ret();return
        if address == 0x4a9d70:
            self.event('S', args[0], args[1], self.string(args[2]), args[3]);self.mutate(10);self.ret();return
        if address == 0x4e0620:
            self.event('!', self.string(args[0]), args[2]);self.ret();return
        if address == 0x4adb30:
            self.event('O', (this - POOL) // 50, *struct.unpack('<3I', mu.mem_read(args[0], 12)));self.ret(self.case[28], 4);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'선택 범위밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """원본 쓰기는 격리 스택과 선택 관련 일곱 전역에만 허용한다."""
        if STACK - 0x7f000 <= address <= STACK + 0x1000 - size:
            return
        if size == 4 and address in GLOBALS:
            return
        raise RuntimeError(f'선택 범위밖 쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """각 입력을 독립 설치하고 실제 정상 반환/현재 전역/가상 효과 순서를 출력한다."""
        self.case = case;self.events = [];self.put(0x5c8464, POOL);self.put(0x5c847c, 24000);self.put(0x5434e4, case[6]);self.put(0x5c89b8, case[7]);self.put(0x5c85a4, case[13]);self.put(0x540c70, case[14])
        self.put(0x541138, 50);self.put(0x54113c, 51);self.put(0x54116c, 70);self.put(0x5412ec, 165);self.put(0x5e4794, 0)
        self.put(0x5caea0, case[4]);self.put(0x5caea4, case[20]);self.put(0x5caf38, case[21]);self.put(0x5caf3c, case[22]);self.put(0x5cae90, case[19]);self.put(0x5cae94, 320);self.put(0x5cae98, 240)
        self.put(0x5caf2c, MULTIPLE);self.put(0x5caf34, case[25]);mu = self.mu
        multiple = (55, 56, 57, 58, 59) if not case[26] else (0, 24000, 55, 0xffffffff, 56)
        mu.mem_write(MULTIPLE, struct.pack('<5I', *multiple));mu.mem_write(EVENT, struct.pack('<3I', 0x20000, case[29], case[30]))
        # 첫 대상/기존 선택/유효 제단/다중 선택과 무관한 슬롯을 독립 초기화한다.
        for sid in (1, 50, 51, 52, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 99, 202, 23999):
            raw = bytearray(50);struct.pack_into('<I', raw, 0, VTABLE);raw[10] = 100;raw[34] = case[12];struct.pack_into('<2I', raw, 14, case[23], case[24]);mu.mem_write(POOL + sid * 50, bytes(raw))
        self.put(POOL + 50 * 50 + 11, case[8], 1);self.put(POOL + 50 * 50 + 40, case[9], 1)
        if case[18]:
            self.put(POOL + 50 * 50 + 10, 165, 1)
        self.put(POOL + 51 * 50 + 6, 60, 2)
        # ProcessFinder 자체가 자산/type<10을 건너뛰고 next를 효과 전에 저장하는지 실제로 관측한다.
        for sid, kind in ((60, 50), (61, 51), (62, 49), (63, 100), (64, 6), (65, 5)):
            self.put(POOL + sid * 50 + 10, kind, 1);self.put(POOL + sid * 50 + 4, sid + 1 if sid < 65 else 0, 2)
        self.put(POOL + 57 * 50 + 11, 1, 1);self.put(POOL + 58 * 50 + 11, 2, 1);self.put(POOL + 59 * 50 + 11, 8, 1)
        for kind in (100, 101, 102, 165):
            self.put(TYPES + kind * 500 + 0xe8, case[10] if kind != 101 else 0x10000000);self.put(TYPES + kind * 500 + 0xec, case[11] if kind != 102 else 0)
        op = case[0];args = struct.pack('<3I', case[1], case[2], case[3]) if op == 0 else b'' if op == 1 else struct.pack('<3I', case[23], case[24], 100) if op == 2 else struct.pack('<I', EVENT)
        self.call(ENTRIES[op], POOL + 50 * 50 if op >= 3 else 0, args, 4 if op >= 3 else 0, precision);self.returns += 1
        result = self.mu.reg_read(UC_X86_REG_EAX) if op == 0 or op >= 3 else 0
        current = [self.get(address) for address in GLOBALS] + [int.from_bytes(self.mu.mem_read(POOL + 50 * 50 + offset, width), 'little') for offset, width in ((10, 1), (40, 1))] + [int.from_bytes(self.mu.mem_read(POOL + 60 * 50 + 4, 2), 'little')]
        return str(result), ';'.join(self.events) or '-', ','.join(map(str, current))


def main():
    """기존 fixture를 재계산하지 않는 근거 감사 또는 새 독립 관찰을 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2 and not report['precisionDifferentInputs']
        print('세 인자 선택/메뉴 입구 근거 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환');return
    oracles = [Oracle(), Oracle()];rows = []
    # 두 정밀도에서 실제 CPU 출력만 얻는다. 제품 C++나 테스트는 이 도구에서 읽지 않는다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case;rows.append((','.join(map(str, case)), *outputs[0]))
    FIXTURE.write_text('# input result events current\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_user_selection.py', 'cpppj/tests/fixtures/user-selection-x86.tsv')
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), entries=[f'{x:08x}' for x in ENTRIES], helpers=[f'{x:08x}' for x in HELPERS], controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=[], boundaries='큰 진단/생산/운반/이동/가상+7c/+88/프로세스+1c/음향/커서/관문과 공통메뉴Open은 명시입력. 선택/다중정리/기억좌표/ProcessFinder/비교/강조/세메뉴입구는 전체실행. 게임/OS창실행없음. 원본4d5c90은 실제스택상3인자.', sha256={path:digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
