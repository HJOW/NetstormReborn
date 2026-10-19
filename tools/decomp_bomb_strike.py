#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 네 직접 피해 주문의 자격·난수·효과·현재 HP 분기를 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, PROCESS, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from decomp_bomb_targets import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-strike-x86.tsv'  # 제품 구현과 독립적으로 먼저 동결할 원문이다.
REPORT = ROOT/'cpppj/bomb-strike-native.json'  # 실제 몸체와 큰 효과 경계를 구별한 기록이다.
TABLES = (0x502908, 0x5029d0, 0x502aa8, 0x502b70)  # 보호PE에서 확인한 실제 네 가상 표다.
QUALIFY = (0x41f2f0, 0x41f3d0, 0x41f3d0, 0x41f3d0)  # 첫 계열은 마스크 자체를 반환하고 나머지는0/1이다.
APPLY = (0x41f310, 0x41f410, 0x41f4e0, 0x41f650)  # 현재 가상c0의 실제 몸체다.
ENTRIES = (*set(QUALIFY), *APPLY, 0x4ac200, 0x4ac550, 0x49a840, 0x49a900, 0x4adc60, 0x4adea0, 0x4558f0)  # 실제 HP 보유/중심/난수도 실행한다.
DAMAGE, ALT_DAMAGE, DESTROY, ALT_DESTROY, POP = (STOP+0x200+32*i for i in range(5))  # 큰 가상 피해/삭제/Pop만 서로 다른 경계로 둔다.
ALT, INPUT, THUNK = STOP+0x800, STOP+0x600, STOP+0xa00  # 교체 표·대상 인자·실제 자격→효과 호출자다.
SLOTS = (0, 50, 51, 60, 70)  # 전체 바이트를 관측하는 원본 물리 슬롯이다.


def base():
    """op/계열/kind/genus/flags1/extra/status/QA/SID/owner/HP/boss/seed/변경/부모xy/대상xy/footxy 입력이다."""
    return [1, 0, 70, 0x200000, 0x10, 0, 0, 51, 60, 1, 1000, 1, 0, 0, bits(50.75), bits(52.5), bits(53.5), bits(53), 1, 3]


def cases():
    """네 계열 자격/합성·초기 죽음·현재 HP와 재진입·공유 난수 소비를 구별한다."""
    rows = []
    # 자격은 실제 HP값을 읽지 않으며dais의extra1을 TYPE_HAS_HP와 구별한다.
    for variant, genus, flags, extra in itertools.product(range(4), (0, 4, 0x10000, 0x20000, 0x100000, 0x120000, 0x200000, 0x410000, 0x600000, 0x80074200), (0, 0x10), (0, 1)):
        row = base(); row[0:2] = 0, variant; row[3:6] = genus, flags, extra; rows.append(tuple(row))
    # signed HP의 사전 캡처·피해 뒤 생존/죽음과 두 난수 반복의 조기 종료를 구별한다.
    for variant, hp, seed, status in itertools.product(range(4), (0, 1, 20, 50, 100, 500, 2000, 0xffffffff), (0, 1, 2, 3, 17, 0xffffffff), (0, 1, 2, 3)):
        row = base(); row[1] = variant; row[6] = status; row[10] = hp; row[12] = seed; rows.append(tuple(row))
    # 생성/Pop/음향/피해/빔/투영 뒤의 현재 슬롯·전역 재조회 순서를 관측한다.
    for variant, mutation, seed in itertools.product(range(4), range(1, 16), (0, 3, 17)):
        row = base(); row[1] = variant; row[12:14] = seed, mutation; rows.append(tuple(row))
    # QA 제외는 전체 대상 인자와WORD를 비교한다. 자격→효과도 직접 호출과 따로 관측한다.
    for variant, op, sid, qa, kind, boss in itertools.product(range(4), (1, 2), (0, 51, 60), (0, 51, 60), (70, 138), (0, 1)):
        row = base(); row[0:3] = op, variant, kind; row[7:9] = qa, sid; row[11] = boss; rows.append(tuple(row))
    # 실제 중심의float 저장·홀수/음수 발자국과 위치 캡처를 별도로 관측한다.
    for variant, foot, point in itertools.product(range(4), ((1, 1), (7, 3), (-3, -5)), ((1.25, -2.5), (16777216, 53.5))):
        row = base(); row[1] = variant; row[16:20] = bits(point[0]), bits(point[1]), *foot; rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """실제 네 주문/자격/게임 난수를 실행하고 월드·음향·빔·화면 투영만 경계로 둔다."""
    def __init__(self):
        """보호PE와 격리 슬롯, 원본 가상 표의 정확한 주소를 준비한다."""
        super().__init__(); self.mu.mem_map(POOL, 0x10000); self.allowed = []; ranges(self, ENTRIES)
        self.instructions = 0; self.calls = collections.Counter()
        for table, qualify, apply in zip(TABLES, QUALIFY, APPLY):
            assert (self.get(table+0xbc), self.get(table+0xc0)) == (qualify, apply)
        assert tuple(self.get(a) for a in (0x541280, 0x541310, 0x54131c)) == (138, 174, 177)
        for table, damage, destroy in ((0x501dd0, DAMAGE, DESTROY), (ALT, ALT_DAMAGE, ALT_DESTROY)):
            self.put(table+0x98, damage); self.put(table+0x10, destroy); self.put(table+0x90, POP)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """큰 효과는 인자만 기록하고 명시 입력의 재진입만 현재 상태에 적용한다."""
        self.instructions += 1; sp = mu.reg_read(UC_X86_REG_ESP); args = struct.unpack('<6I', mu.mem_read(sp+4, 24)); this = mu.reg_read(UC_X86_REG_ECX)
        target = POOL+self.case[8]*50; parent = POOL+50*50; mutation = self.case[13]
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x4af530:
            self.event('create', *args[:2])
            if mutation == 1: self.put(target+10, 138, 1)
            if mutation == 2: self.put(target+14, bits(200)); self.put(target+18, bits(201))
            if mutation == 3: self.put(parent+34, 3, 1); self.put(target, ALT)
            self.ret(POOL+70*50); return
        if address == POP:
            assert this == POOL+70*50; self.event('pop', 70, *args[:3])
            if mutation == 4: self.put(target+14, bits(200)); self.put(target+18, bits(201))
            if mutation == 5: self.put(target+10, 138, 1); self.put(0x540bc4, 0)
            if mutation == 6: self.put(target+10, 70, 1); self.put(target, ALT); self.put(parent+34, 4, 1)
            self.ret(0, 12); return
        if address == 0x4a9d70:
            assert self.string(args[2]) == 'thunderCrack.wav'; self.event('sound', *args[:2], args[3])
            if mutation == 7: self.put(target+11, 2, 1)
            if mutation == 8: self.put(parent+12, 0, 2); self.put(parent+34, 6, 1)
            self.ret(); return
        if address in (DAMAGE, ALT_DAMAGE):
            assert this == target; self.event('damage' if address == DAMAGE else 'damage-alt', self.case[8], *args[:3])
            self.put(target+26, self.get(target+26)-args[0])
            if mutation == 9: self.put(target+26, 1000000)
            if mutation == 10: self.put(parent+34, 7, 1); self.put(target, ALT)
            if mutation == 11: self.put(target+11, 2, 1); self.put(target+26, 0)
            if mutation == 12: self.put(parent+12, self.case[8], 2)
            self.ret(1, 12); return
        if address in (DESTROY, ALT_DESTROY):
            assert this == target; self.event('destroy' if address == DESTROY else 'destroy-alt', self.case[8], args[0]); self.ret(0, 4); return
        if address == 0x472700:
            self.event('beam', *args[:2])
            if mutation == 13: self.put(target+11, 2, 1); self.put(parent+12, self.case[8], 2)
            self.ret(); return
        if address == 0x497220:
            self.event('project', *args[1:3]); self.projects += 1
            x, y = 17+2*self.projects, -23-2*self.projects
            self.put(this, x); self.put(this+4, y); self.put(args[0], x); self.put(args[0]+4, y)
            if mutation == 15: self.put(target+14, bits(200)); self.put(target+18, bits(201))
            self.ret(this, 12); return
        if address == 0x472770:
            self.event('column', *args)
            if mutation == 14: self.put(parent+14, bits(200)); self.put(parent+18, bits(201))
            self.ret(); return
        if THUNK <= address < THUNK+34:
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'직접 피해 주문 미허용 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 몸체의 쓰기는 격리 스택과 공유 게임 난수DWORD만 허용한다."""
        if STACK-0x7f000 <= address and address+size <= STACK+0x1000 or address == 0x532710 and size == 4:
            return
        raise RuntimeError(f'직접 피해 주문 미허용 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지·격리 현재raw/TYPE/스택/호출자만 읽는다."""
        spans = ((0, 4), (0x400000, 0x600000), (POOL, POOL+0x10000), (TYPES, TYPES+0x20000), (PROCESS, PROCESS+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in spans):
            raise RuntimeError(f'직접 피해 주문 미허용 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """독립 입력에서 실제 반환·효과 순서·전체 슬롯·공유 난수를 읽는다."""
        self.case = case; self.events = []; self.projects = 0; self.mu.mem_write(POOL, bytes(0x10000)); self.mu.mem_write(TYPES, bytes(0x20000))
        parent = bytearray(50); struct.pack_into('<I', parent, 0, TABLES[case[1]]); parent[10] = 168; parent[34] = case[9]
        struct.pack_into('<H', parent, 12, case[7]); struct.pack_into('<2I', parent, 14, *case[14:16]); self.mu.mem_write(POOL+50*50, bytes(parent))
        for sid in (51, 60):
            raw = bytearray(50); struct.pack_into('<I', raw, 0, 0x501dd0); raw[10] = 70; self.mu.mem_write(POOL+sid*50, bytes(raw))
        raw = bytearray(50); struct.pack_into('<I', raw, 0, 0x501dd0); raw[10] = case[2]; raw[11] = case[6]; raw[40] = case[5]
        struct.pack_into('<2I', raw, 14, *case[16:18]); struct.pack_into('<I', raw, 26, case[10]); self.mu.mem_write(POOL+case[8]*50, bytes(raw))
        self.put(POOL+70*50, 0x501dd0)
        start = TYPES+case[2]*500; self.put(start+0xe8, case[4]); self.put(start+0xec, case[3]); self.put(start+0x1d4, case[18]); self.put(start+0x1d8, case[19])
        self.put(TYPES+168*500+0x1d4, 1); self.put(TYPES+168*500+0x1d8, 3)
        for address, value in ((0x5c8464, POOL), (0x59ab20, TYPES), (0x540bc4, case[11]), (0x532710, case[12]), (0x541280, 138), (0x541310, 174), (0x54131c, 177), (0x541348, 188)):
            self.put(address, value)
        qualify, apply = QUALIFY[case[1]], APPLY[case[1]]
        code = b'\xff\x35'+struct.pack('<I', INPUT)+b'\xe8'+struct.pack('<i', qualify-(THUNK+11))+b'\x85\xc0\x74\x12\x50\xff\x35'+struct.pack('<I', INPUT)+b'\xb9'+struct.pack('<I', POOL+50*50)+b'\xe8'+struct.pack('<i', apply-(THUNK+32))+b'\x58\xc3'
        assert len(code) == 34; self.mu.mem_write(THUNK, code); self.put(INPUT, case[8]); self.put(0, 0x87654321)
        entry = qualify if case[0] == 0 else apply if case[0] == 1 else THUNK
        InvokeOracle.invoke(self, entry, struct.pack('<I', case[8]) if case[0] < 2 else b'', precision, False)
        self.returns += 1; assert self.get(0) == 0x87654321
        result = self.mu.reg_read(UC_X86_REG_EAX) if case[0] != 1 else 0
        raws = ','.join(bytes(self.mu.mem_read(POOL+sid*50, 50)).hex() for sid in SLOTS)
        return str(result), ';'.join(self.events) or '-', raws, str(self.get(0x532710)), str(self.get(0x540bc4))


def main():
    """새 원문을 한 번 동결하거나 기존 기대값을 재계산 없이 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if options.verify:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, wanted in record['sha256'].items():
            assert digest(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == record['uniqueInputs'] and len(rows)*2 == record['normalReturns'] and all(row[1:6] == row[6:11] for row in rows)
        print('직접 피해 주문 원문 감사', len(rows), record['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 원문은 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    # 두 정밀도의 반환과 효과·전체 상태가 같을 때만 기대값으로 동결한다.
    for case in cases():
        try:
            outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        except Exception as error:
            raise RuntimeError(str(case)) from error
        assert outputs[0] == outputs[1], case; rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input return53 effects53 raw53 rng53 boss53 return64 effects64 raw64 rng64 boss64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_bomb_strike.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_carrier_falling.py', 'tools/decomp_bomb_targets.py', 'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=235, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'), uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), tables=TABLES,
                  boundaries='41f2f0/41f3d0/41f310/41f410/41f4e0/41f650과HP보유/TYPE/중심/4558f0게임난수는 실제 몸체다. Factory생성/현재가상90Pop/98피해/10삭제·음향·472700빔·497220투영·472770기둥효과는 명시 경계다. 경계의HP감소/재진입은 입력이며 실제월드피해·표시·게임/UI성공을 증명하지 않는다. GP/retN/canary/SEH/x87/실행·읽기·쓰기범위 확인.', sha256={name: digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
