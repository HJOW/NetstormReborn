#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Devastation/폭발 주문의 자격·실제 기하·가상 피해/삭제 순서를 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, PROCESS, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from decomp_bomb_targets import RADIUS_BODY, bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-blast-x86.tsv'  # 제품 구현 전에 동결하는 독립 원문이다.
REPORT = ROOT/'cpppj/bomb-blast-native.json'  # 실제 계산 몸체와 선언 효과 경계를 구분한다.
THUNK, INPUT, CODES, ALT = STOP+0xa00, STOP+0x600, PROCESS+0x500, STOP+0x800  # 직접 자격→효과 호출자와 격리 입력/프레임/교체 표다.
DAMAGE, DAMAGE_ALT, DESTROY, DESTROY_ALT, RADIUS_OVERRIDE = (STOP+0x200+32*i for i in range(5))  # 큰 가상 효과를 구분하는 토큰이다.
ENTRIES = (0x41e9c0, 0x41ea30, 0x422560, 0x4adc60, 0x4ac200, 0x4ac550,
           0x49a840, 0x49a900, 0x4adea0, 0x41e460, 0x41d7a0, 0x41d0d0, 0x41cfc0, 0x4e49c0, 0x421f70)  # 자격/기하/HP 보유/프레임 보호/끝칸 요청을 실제 실행한다.
POINTS = ((51, 53), (53, 53), (53.5, 53), (53.500003814697266, 53), (55, 53), (float('nan'), 53))  # 안쪽/경계/바깥/비교불가의 명시 대상 좌표다.


def base():
    """operation/kind/genus/flags1/extra/frame/code/boss/QA/SID/두owner/동맹/range/두좌표/foot/변경/state/HP 입력이다."""
    return [0, 70, 0x200000, 0x10, 0, 1, 0, 1, 51, 60, 1, 2, 0, bits(3.9), bits(50.75), bits(52.5), bits(53.5), bits(53), 1, 1, 0, 0, 100]


def cases():
    """정상 분기와 실제 HP값/속성의 차이·보호 프레임·동맹·거리·재진입을 교차한다."""
    rows = []
    # 자격은 HP 수치가 아니라 TYPE_HAS_HP/dais abstract를 읽는다.
    for kind, genus, flags1, extra, code, state, hp in itertools.product((70, 150, 158),
            (0, 4, 0x400000, 0x200000, 0x10000, 0x80074200, 0x80000000, 0x4200), (0, 0x10), (0, 1), (0, 0x40, 0xff), (0, 2), (0, 100)):
        row = base(); row[1:5] = kind, genus, flags1, extra; row[6] = code; row[21:23] = state, hp; rows.append(tuple(row))
    # 직접 효과와 자격→효과 합성 호출은 서로 다른 범위로 기록한다.
    for op, kind, genus, boss, allied, point in itertools.product((1, 2), (70, 150, 158), (0, 4, 0x200000), (0, 1), (0, 1, 0xffffffff), POINTS):
        row = base(); row[0:3] = op, kind, genus; row[7] = boss; row[12] = allied; row[16:18] = map(bits, point); rows.append(tuple(row))
    # 자격은0/운반체도 읽을 수 있지만 효과는 QA와 같은 전체 대상번호를 먼저 건너뛴다.
    for op, sid, qa in itertools.product((0, 1, 2), (0, 51, 60), (0, 51)):
        row = base(); row[0] = op; row[8:10] = qa, sid; rows.append(tuple(row))
    # 가상 radius 재진입과 피해/삭제 뒤 캡처 위치/현재 표를 구별한다.
    for genus, kind, mutation, point in itertools.product((4, 0x200000), (70, 150, 158), range(1, 12), (POINTS[0], POINTS[3], POINTS[4])):
        row = base(); row[0:3] = 1, kind, genus; row[20] = mutation; row[16:18] = map(bits, point); rows.append(tuple(row))
    # 후보 중심/반경의 중간float 저장과 signed 반경을 확인한다.
    for width, height, radius, point in itertools.product((1, 2, 7), (1, 3), (-1, 0, 1, 3.9), (POINTS[1], POINTS[2], POINTS[3])):
        row = base(); row[0] = 1; row[2] = 4; row[13] = bits(radius); row[16:20] = bits(point[0]), bits(point[1]), width, height; rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """실제 두 몸체/보조를 실행하고 큰 피해/삭제/지면 효과·끝칸 변경만 경계로 둔다."""
    def __init__(self):
        """보호PE·격리 슬롯과 실제 자격→효과 호출자를 준비한다."""
        super().__init__(); self.mu.mem_map(POOL, 0x10000); self.allowed = []; ranges(self, ENTRIES)
        assert bytes(self.mu.mem_read(0x41e3c0, len(RADIUS_BODY))) == RADIUS_BODY
        self.allowed.append((0x41e3c0, 0x41e3c0+len(RADIUS_BODY)))
        assert (self.get(0x5412b0), self.get(0x5412d0)) == (150, 158)
        code = b'\xff\x35'+struct.pack('<I', INPUT)+b'\xe8'+struct.pack('<i', 0x41e9c0-(THUNK+11))+b'\x85\xc0\x74\x12\x50\xff\x35'+struct.pack('<I', INPUT)+b'\xb9'+struct.pack('<I', POOL+50*50)+b'\xe8'+struct.pack('<i', 0x41ea30-(THUNK+32))+b'\x58\xc3'
        assert len(code) == 34
        self.mu.mem_write(THUNK, code); self.instructions = 0; self.calls = collections.Counter()
        # 대상의 현재 표가 변경되면 실제 다른 경계 주소로 분배해야 한다.
        for table, damage, destroy in ((0x501dd0, DAMAGE, DESTROY), (ALT, DAMAGE_ALT, DESTROY_ALT)):
            self.put(table+0x98, damage); self.put(table+0x10, destroy)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """실제 효과 인자를 기록하고 선언한 콜백 재진입만 현재 상태에 반영한다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP); args = struct.unpack('<4I', mu.mem_read(sp+4, 16)); this = mu.reg_read(UC_X86_REG_ECX)
        target = POOL+self.case[9]*50; parent = POOL+50*50; mutation = self.case[20]
        if address in ENTRIES or address == 0x41e3c0:
            self.calls[f'{address:08x}'] += 1
        if address == RADIUS_OVERRIDE:
            assert this == parent
            self.event('radius-override', 50)
            if mutation == 1:
                self.put(target+14, bits(200)); self.put(target+18, bits(200))
            if mutation == 2:
                self.put(target, ALT)
            if mutation == 3:
                self.put(0x540bc4, 0)
            if mutation == 4:
                self.put(target+10, 71, 1); self.put(TYPES+70*500+0x1d4, 8)
            self.ret(3); return
        if address in (DAMAGE, DAMAGE_ALT):
            assert this == target
            self.event('damage' if address == DAMAGE else 'damage-alt', self.case[9], *args[:3])
            if mutation == 5:
                self.put(target+14, bits(200)); self.put(target+18, bits(200))
            if mutation == 6:
                self.put(parent+34, 3, 1)
            if mutation == 7:
                self.put(target, ALT)
            if mutation == 8:
                self.put(target+11, 2, 1); self.put(target+26, 1)
            self.ret(1, 12); return
        if address in (DESTROY, DESTROY_ALT):
            assert this == target
            self.event('destroy' if address == DESTROY else 'destroy-alt', self.case[9], args[0])
            if mutation == 9:
                self.put(target+14, bits(200)); self.put(target+18, bits(200))
            if mutation == 10:
                self.put(target, ALT)
            self.ret(0, 4); return
        if address == 0x470ac0:
            self.event('ground', *args[:2])
            if mutation == 11:
                self.put(target+40, 1, 1)
            self.ret(); return
        if address == 0x4215d0:
            assert this == target
            self.event('edge', self.case[9], *args[:2]); self.ret(0, 8); return
        if THUNK <= address < THUNK+34:
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Bomb 폭발 미허용 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 몸체는 스택 외의raw/TYPE/전역을 직접 쓰지 않는다."""
        if STACK-0x7f000 <= address and address+size <= STACK+0x1000:
            return
        raise RuntimeError(f'Bomb 폭발 미허용 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지와 격리 raw/TYPE/프레임/호출자/스택만 읽는다."""
        spans = ((0, 4), (0x400000, 0x600000), (POOL, POOL+0x10000), (TYPES, TYPES+0x20000),
                 (PROCESS, PROCESS+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in spans):
            raise RuntimeError(f'Bomb 폭발 미허용 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """명시 raw/TYPE/프레임/동맹 입력을 공급하고 실제 결과/전체 슬롯을 관측한다."""
        self.case = case; self.events = []; self.mu.mem_write(POOL, bytes(0x10000)); self.mu.mem_write(TYPES, bytes(0x20000))
        parent = bytearray(50); struct.pack_into('<I', parent, 0, 0x502370); parent[10] = 168; parent[34] = case[10]
        struct.pack_into('<H', parent, 12, case[8]); struct.pack_into('<2I', parent, 14, *case[14:16]); self.mu.mem_write(POOL+50*50, bytes(parent))
        raw = bytearray(50); struct.pack_into('<I', raw, 0, 0x501dd0); raw[10] = case[1]; raw[11] = case[21]; raw[34] = case[11]; raw[40] = case[4]
        struct.pack_into('<2I', raw, 14, *case[16:18]); struct.pack_into('<I', raw, 36, case[5]); struct.pack_into('<I', raw, 26, case[22]); self.mu.mem_write(POOL+case[9]*50, bytes(raw))
        start = TYPES+case[1]*500
        self.put(start+0xe8, case[3]); self.put(start+0xec, case[2]); self.put(start+0x124, CODES)
        self.put(start+0x1d4, case[18]); self.put(start+0x1d8, case[19]); self.put(TYPES+168*500+0x44, case[13])
        # 네 프레임은 입력이며 현재 프레임의 보호 비트 선택은 원본이 계산한다.
        self.mu.mem_write(CODES, b''.join(bytes((74, 0, index+1, case[6]^(index*17))) for index in range(4)))
        self.mu.mem_write(0x595200, bytes(81*4)); self.put(0x595200+(case[10]*9+case[11])*4, case[12])
        for address, value in ((0x5c8464, POOL), (0x59ab20, TYPES), (0x540bc4, case[7]), (0x5412b0, 150), (0x5412d0, 158), (0x541348, 188)):
            self.put(address, value)
        self.put(0x502370+0xb4, RADIUS_OVERRIDE if 1 <= case[20] <= 4 else 0x41e3c0)
        self.put(0, 0x87654321); self.put(INPUT, case[9])
        entry = 0x41e9c0 if case[0] == 0 else 0x41ea30 if case[0] == 1 else THUNK
        InvokeOracle.invoke(self, entry, struct.pack('<I', case[9]) if case[0] < 2 else b'', precision, False)
        self.returns += 1; assert self.get(0) == 0x87654321
        result = self.mu.reg_read(UC_X86_REG_EAX) if case[0] != 1 else 0
        raws = ','.join(bytes(self.mu.mem_read(POOL+sid*50, 50)).hex() for sid in (0, 50, 51, 60))
        return str(result), ';'.join(self.events) or '-', raws, str(self.get(0x540bc4)), str(self.get(TYPES+70*500+0x1d4))


def main():
    """독립 원문을 최초 동결하거나 기존 입력/해시/두 정밀도 일치를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if options.verify:
        record = json.loads(REPORT.read_text('utf-8'))
        # 기대값을 재계산하거나 덮어쓰지 않는다.
        for name, wanted in record['sha256'].items():
            assert digest(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == record['uniqueInputs'] and len(rows)*2 == record['normalReturns']
        assert all(row[1:6] == row[6:11] for row in rows)
        print('Bomb 폭발 자격/효과 원문 감사', len(rows), record['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 동결 원문은 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    # 반환/효과/전체 슬롯을 두 실제 정밀도에서 읽어 일치할 때만 동결한다.
    for case in cases():
        try:
            outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        except Exception as error:
            raise RuntimeError(str(case)) from error
        assert outputs[0] == outputs[1], case
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input return53 effects53 raw53 boss53 width53 return64 effects64 raw64 boss64 width64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_bomb_blast.py', 'tools/decomp_bomb_targets.py',
             'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_carrier_falling.py', 'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=234, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'),
                  uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles),
                  nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), specialType=150, priestType=158,
                  boundaries='41e9c0/41ea30/422560/HP보유/TYPE/프레임보호/기하/기본radius/421f70 전체. 실제가상98피해/10삭제/470ac0지면효과/4215d0끝칸변경은 경계다. 일부입력의파생b4 override도명시경계이며원본기하를대체하지않는다. GP/retN/canary/SEH보존/x87/실행·읽기·쓰기범위확인. 실제HP전환·그래프·미션/게임/UI성공의증거가아니다.',
                  sha256={name: digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
