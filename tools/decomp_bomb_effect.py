#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 0x5f 수신의 현재 대상 효과·지연 처리·카운터·삭제 순서를 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, PROCESS, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-effect-x86.tsv'  # 제품 구현 전에 동결하는 새 원문이다.
REPORT = ROOT/'cpppj/bomb-effect-native.json'  # 실제 실행 몸체와 선언 경계의 기록이다.
ENTRIES = (0x420bf0, 0x4ac550, 0x49a840, 0x4e49c0, 0x40ace0, 0x485f70)  # 본문·TYPE·절삭·대상 주소·누적 카운터다.
PACKET, THUNK, ALT = STOP+0x600, STOP+0xb00, STOP+0x800  # 격리 패킷·cdecl 호출자·교체 가상 표다.
APPLY, APPLY_ALT, DESTROY, DESTROY_ALT = (STOP+0x200+32*i for i in range(4))  # 현재 가상 표의 효과와 삭제를 구분한다.


def bits(value):
    """float 비용 입력의 원래 비트만 공급한다. 결과 카운터는 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """수신 초기 관문·운반체 상태·중복/0번 대상·재진입·지연/확보 실패를 교차한다."""
    rows = []
    # 입력은부모상태/QA상태/QA/타입/권한/대상수/대상상태패턴/변경/비용비트/누적값/두번째확보성공이다.
    for flags, carrier, kind, count, pattern in itertools.product((0, 1, 2, 3, 4, 8), (0, 1, 2, 4, 8), (117, 118, 168), (0, 1, 3, 6), (0, 1, 2, 3)):
        rows.append((flags, carrier, 51, kind, 1, count, pattern, 0, bits(1001.75), 0xfffffff0, 1))
    # 첫효과뒤패킷count가자라더라도최초배열capacity까지만저장하는원본을관측한다.
    for kind, boss, count, mutation in itertools.product((117, 118, 168), (0, 1), (0, 2, 6), range(1, 13)):
        rows.append((0, 0, 51, kind, boss, count, 0, mutation, bits(23.75), 19, 1))
    # 실제ftol의signed64절삭과하위DWORD누적을고정한다.
    for qa, cost, allocated in itertools.product((0, 51), (bits(-23.75), bits(0), bits(2147483648), 0x7fc01234, bits(float('inf'))), (0, 1)):
        rows.append((0, 0, qa, 117, 1, 3, 3, 0, cost, 0x7fffffff, allocated))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """수신/TYPE/절삭/누적은PE가실행하고큰가상효과·프로세스·장치만경계로둔다."""
    def __init__(self):
        """독립슬롯·호출자와두현재가상표를설치한다."""
        super().__init__()
        self.mu.mem_map(POOL, 0x10000)
        self.allowed = []
        ranges(self, ENTRIES)
        self.instructions = 0
        self.calls = collections.Counter()
        code = b'\x68'+struct.pack('<I', PACKET)+b'\x6a\x00\x6a\x09\xe8'+struct.pack('<i', 0x420bf0-(THUNK+14))+b'\x83\xc4\x0c\xc3'
        assert len(code) == 18
        self.mu.mem_write(THUNK, code)
        # 두표의다른토큰은효과뒤현재가상표를재조회했는지드러낸다.
        for table, apply, destroy in ((0x502370, APPLY, DESTROY), (ALT, APPLY_ALT, DESTROY_ALT)):
            self.put(table+0xc0, apply); self.put(table+0x10, destroy)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """실제하위호출의현재인자와명시재진입변경을순서대로기록한다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        args = struct.unpack('<6I', mu.mem_read(sp+4, 24))
        this = mu.reg_read(UC_X86_REG_ECX)
        parent = POOL+50*50
        mutation = self.case[7]
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x425f50:
            self.event('stop-cast', args[0])
            if mutation == 1:
                self.put(parent+10, 118, 1)
            if mutation == 11:
                self.put(parent+12, 52, 2)
            self.ret(); return
        if address == 0x4e4391:
            self.allocations += 1
            self.event('allocate', args[0])
            result = PROCESS if self.allocations == 1 else PROCESS+0x200 if self.case[10] else 0
            self.ret(result); return
        if address in (APPLY, APPLY_ALT):
            assert this == parent
            self.event('apply' if address == APPLY else 'apply-alt', 50, args[0])
            self.applied += 1
            if self.applied == 1:
                if mutation == 2:
                    self.put(parent, ALT)
                if mutation == 3:
                    self.put(POOL+62*50+11, 2, 1); self.put(POOL+args[0]*50+11, 2, 1)
                if mutation == 4:
                    self.put(PACKET+6, 1)
                if mutation == 5:
                    self.put(PACKET+6, 5)
                if mutation == 6:
                    self.put(0x540bc4, 0)
                if mutation == 7:
                    self.put(parent+10, 118, 1)
                if mutation == 12:
                    self.put(0x541230, 168)
            self.ret(0, 4); return
        if address == 0x420a40:
            assert this == PROCESS+0x200
            pointer, capacity, count = struct.unpack('<3I', mu.mem_read(args[1], 12))
            assert pointer == PROCESS and count <= capacity
            selected = tuple(struct.unpack('<'+str(count)+'I', mu.mem_read(pointer, 4*count))) if count else ()
            self.event('construct', args[0], args[2], capacity, count, *selected)
            if mutation == 8:
                self.put(parent+10, 168, 1)
            self.ret(PROCESS+0x200, 12); return
        if address == 0x41e0a0:
            self.event('schedule-argument', *args[:2]); self.ret(777); return
        if address == 0x41bd20:
            assert this in (0, PROCESS+0x200)
            self.event('attach', int(this != 0), *args[:4]); self.ret(0, 16); return
        if address == 0x4a9550:
            self.event('sound', *args)
            if mutation == 9:
                self.put(parent, ALT)
            self.ret(); return
        if address in (DESTROY, DESTROY_ALT):
            assert this == parent
            self.event('destroy' if address == DESTROY else 'destroy-alt', 50, args[0])
            if mutation == 10:
                self.put(parent+11, 2, 1)
            self.ret(0, 4); return
        if address == 0x4e4354:
            assert args[0] == PROCESS
            self.event('release', 1); self.ret(); return
        if THUNK <= address < THUNK+18:
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Bomb 효과 미허용 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제본문은배열/누적카운터/SEH/스택밖에직접쓰지않는다."""
        spans = ((0, 4), (STACK-0x7f000, STACK+0x1000), (PROCESS, PROCESS+400), (0x595088, 0x59508c))
        if any(low <= address and address+size <= high for low, high in spans):
            return
        raise RuntimeError(f'Bomb 효과 미허용 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """원본PE·격리raw/TYPE/배열·호출자/스택으로읽기를한정한다."""
        spans = ((0, 4), (0x400000, 0x600000), (POOL, POOL+0x10000), (TYPES, TYPES+0x20000),
                 (PROCESS, PROCESS+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in spans):
            raise RuntimeError(f'Bomb 효과 미허용 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """동일명시입력을설치하고정상반환뒤효과/raw/패킷/카운터를관측한다."""
        self.case = case
        self.events = []
        self.allocations = self.applied = 0
        self.mu.mem_write(POOL, bytes(0x10000))
        # 각대상의상태는명시입력이며최종선택여부를미리계산하지않는다.
        for sid in (0, 50, 51, 52, 60, 61, 62, 63, 64, 65):
            raw = bytearray(50)
            struct.pack_into('<I', raw, 0, 0x502370)
            raw[10] = case[3] if sid == 50 else 133
            raw[11] = case[0] if sid == 50 else case[1] if sid in (0, 51, 52) else 1 if case[6] == 1 and sid % 2 else 2 if case[6] == 2 else 0
            self.mu.mem_write(POOL+50*sid, bytes(raw))
        self.put(POOL+50*50+12, case[2], 2)
        self.mu.mem_write(TYPES, bytes(0x20000))
        # 번호별 비용/소리는 서로 달라야 현재 TYPE 재조회가 드러난다.
        for kind, delay in ((117, 9), (118, 11), (168, 13)):
            self.put(TYPES+kind*500+0xc4, case[8] if kind == case[3] else bits(33.75))
            self.put(TYPES+kind*500+0x70, 1000+kind); self.put(TYPES+kind*500+0x1cc, delay)
        for address, value in ((0x5c8464, POOL), (0x59ab20, TYPES), (0x540bc4, case[4]),
                               (0x54122c, 117), (0x541230, 118), (0x541154, 57), (0x595088, case[9])):
            self.put(address, value)
        targets = (60, 60, 0, 63, 64, 65) if case[6] == 3 else tuple(range(60, 66))
        self.mu.mem_write(PACKET, struct.pack('<BBHHI6H', 1, 0x5f, 22, 50, case[5], *targets))
        self.put(0, 0x87654321)
        InvokeOracle.invoke(self, THUNK, b'', precision, False)
        self.returns += 1
        assert self.get(0) == 0x87654321
        raws = ','.join(bytes(self.mu.mem_read(POOL+sid*50, 50)).hex() for sid in (0, 50, 51, 52, 60, 61, 62, 63, 64, 65))
        return ';'.join(self.events) or '-', raws, bytes(self.mu.mem_read(PACKET, 22)).hex(), str(self.get(0x595088)), str(self.get(0x540bc4)), str(self.get(0x541230))


def main():
    """새원문을최초동결하거나보호해시·입력·두정밀도일치를읽기전용감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if options.verify:
        record = json.loads(REPORT.read_text('utf-8'))
        # 고정한원문/입력도구를다시생성하지않는다.
        for name, expected in record['sha256'].items():
            assert digest(ROOT/name) == expected, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == record['uniqueInputs'] and len(rows)*2 == record['normalReturns']
        assert all(row[1:7] == row[7:13] for row in rows)
        print('Bomb 효과 원문 감사', len(rows), record['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존동결원문은덮어쓰지않는다'
    oracles = [Oracle(), Oracle()]
    rows = []
    # 큰효과의기대값을제품에서얻지않고두실제원문실행을비교한다.
    for case in cases():
        try:
            outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        except Exception as error:
            raise RuntimeError(str(case)) from error
        assert outputs[0] == outputs[1], case
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input effects53 raw53 packet53 counter53 boss53 second53 effects64 raw64 packet64 counter64 boss64 second64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_bomb_effect.py',
             'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_carrier_falling.py',
             'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=231, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'),
                  uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles),
                  nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())),
                  boundaries='420bf0/TYPE/ftol/40ace0/485f70 전체. 425f50 시전 프로세스 종료·힙·현재 가상c0효과·지연420a40 생성/스케줄·소리·현재가상10삭제/free는 명시 경계다. 실제 내용물 소비/파생 피해/지연효과/게임/UI 성공으로 세지 않는다. GP/retN/canary/SEH/x87/실행·읽기·쓰기범위를 검사했다.',
                  sha256={name: digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
