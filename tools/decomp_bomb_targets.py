#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 주문 대상 검색의 실제 네 해시 탐색·거리·발자국·저장 순서를 독립 실행한다."""
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
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-targets-x86.tsv'  # 제품 구현 전에 동결하는 독립 입력/출력이다.
REPORT = ROOT/'cpppj/bomb-targets-native.json'  # 실제 몸체/정밀도/경계를 기록한다.
HASH, POOL_SIZE = 0x14200000, 0x140000  # 24000개 물리 슬롯과 겹치지 않는 네 해시 입력 공간이다.
ARRAY, OUTPUT, ALT = PROCESS, PROCESS+0x800, STOP+0x800  # DWORD100개·원본 배열 헤더·교체 가상 표다.
ELIGIBLE, ELIGIBLE_ALT = STOP+0x200, STOP+0x220  # 파생별 가상bc 자격 효과만 선언 경계로 둔다.
ENTRIES = (0x41e4b0, 0x41e3c0, 0x41d7a0, 0x41d770, 0x41d0d0, 0x41cfc0,
           0x4ac550, 0x49a840, 0x49a900, 0x4adea0, 0x4e49c0, 0x41def0,
           0x4b16d0, 0x4b1810, 0x434380, 0x4b2b00, 0x44daa0)  # 검색·기하·기본radius/TYPE/CRT를 전부 실행한다.
IDS = (1, 4, 5, 14999, 15000, 15001, 15002, 15003, 15004, 15005, 15006, 15007)  # 예약/클라이언트/서버 경계를 함께 포함한다.
RADIUS_BODY = bytes.fromhex('e88be10800d94044e9f3650c00')  # 현재 정밀 표에 빠진 실제13바이트 range 썽크다. 코드 바이트를 확인한 뒤 해당 범위만 허용한다.


def bits(value):
    """명시 float 입력의 비트를 저장한다. 선택 결과는 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """기하/상태/최대 저장 수/콜백 재진입을 독립 입력으로 만든다."""
    rows = []
    # 입력은 장면/권한/range비트/부모x,y/용량/기존개수/자격모드/변경/등록역순이다.
    for scene, radius, capacity, reverse in itertools.product(range(5), (0, .9, 1, 3.9, 5, 10, -1), (0, 1, 100), (0, 1)):
        rows.append((scene, 1, bits(radius), bits(50.75), bits(52.5), capacity, 0, 1, 0, reverse))
    # 0.99999 스냅/0.5 차감/보드 제한을 실제 원본 보조 함수로 확인한다.
    for point, radius in itertools.product(((0, 0), (1, 1), (255, 255), (256, 256), (-1, -1),
            (50.000005, 52.000005), (50.00002, 52.00002), (50.99999, 52.5), (50.5, 52.5)), (0, 3.9, 5)):
        rows.append((4, 1, bits(radius), bits(point[0]), bits(point[1]), 100, 0, 1, 0, 0))
    # 파생 자격은 큰 경계이며 그 효과 뒤 현재 목록/후속 후보/가상 표를 재조회한다.
    for boss, accept, mutation in itertools.product((0, 1), (0, 1, 2), range(1, 9)):
        rows.append((0, boss, bits(5), bits(50.75), bits(52.5), 4, 0, accept, mutation, 0))
    # 배열 용량/기존개수는 signed 비교이며 넘친 기존개수를 먼저 정규화하지 않는다.
    for capacity, count in itertools.product((0, 1, 3, 100, 0xffffffff), (0, 1, 3)):
        rows.append((0, 1, bits(5), bits(50.75), bits(52.5), capacity, count, 1, 0, 0))
    # range getter의 실제signed64절삭 하위DWORD를 관측한다.
    for radius in (0x7fc01234, bits(float('inf')), bits(2147483648)):
        rows.append((4, 1, radius, bits(50.75), bits(52.5), 100, 0, 1, 0, 0))
    return list(dict.fromkeys(rows))


def objects(scene):
    """정수 SID·등록 단계·좌표·발자국/상태 입력만 공급한다. Pop/선택 결과를 계산하지 않는다."""
    result = []
    positions = ((50.75, 52.5), (51, 53), (51.5, 53), (52, 53), (54.5, 53), (54.500003814697266, 53),
                 (53.5, 55), (47.5, 53), (50.75, 48), (57, 53), (52, 55), (51, 52))
    # 발자국/표면/네 단계/예약 번호가 같은 공간 검색 안에서 공존한다.
    for index, (sid, position) in enumerate(zip(IDS, positions)):
        width, height = (1, 1) if scene != 1 else ((index % 4)+1, (index % 3)+1)
        state = (0, 1, 2, 4, 8, 0, 0, 0, 0, 0, 0, 0)[index] if scene == 2 else 0
        extra = (0, 0, 0, 0, 0, 1, 8, 16, 32, 0, 1, 0)[index] if scene in (2, 3) else 0
        x, y = position
        if scene == 4:
            x, y = ((1.5, 1.5), (255, 255), (1, 255), (255, 1), *positions[4:])[index]
        result.append((sid, 70+index, state, extra, bits(x), bits(y), width, height, index % 4))
    return result


class Oracle(BaseOracle):
    """해시 탐색/기하/저장은 실제 명령이며 파생 자격과 복귀 진단만 경계다."""
    def __init__(self):
        """보호PE와 독립24000슬롯/네 해시/배열을 설치한다."""
        super().__init__()
        self.mu.mem_map(POOL, POOL_SIZE)
        self.mu.mem_map(HASH, 0x30000)
        self.allowed = []
        ranges(self, tuple(entry for entry in ENTRIES if entry != 0x41e3c0))
        assert bytes(self.mu.mem_read(0x41e3c0, len(RADIUS_BODY))) == RADIUS_BODY
        self.allowed.append((0x41e3c0, 0x41e3c0+len(RADIUS_BODY)))
        self.instructions = 0
        self.calls = collections.Counter()
        self.bases = []
        address = HASH
        # 실제 네 배열의 머리 SID를 원본 SquidHash 전역 구조에 연결한다.
        for scale in (1, 2, 4, 16):
            self.bases.append(address); address += (256//scale)**2*2
        self.map_size = address-HASH
        self.mu.mem_write(0x542508, struct.pack('<7I', 0, 1, 256, *self.bases))
        # 범위 계산은 실제원본41e3c0이며 현재 자격만 별도 경계로 식별한다.
        for table, eligible in ((0x502370, ELIGIBLE), (ALT, ELIGIBLE_ALT)):
            self.put(table+0xb4, 0x41e3c0); self.put(table+0xbc, eligible)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """실제 관찰 시점의 기하와 자격 인자/선언 재진입 효과를 기록한다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        args = struct.unpack('<5I', mu.mem_read(sp+4, 20))
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x41e52d:
            self.geometry[:2] = struct.unpack('<2I', mu.mem_read(sp+0x18, 8))
        if address == 0x41e533:
            self.geometry[2] = mu.reg_read(UC_X86_REG_EAX)
        if address == 0x41def0:
            self.geometry[3:] = args[:4]
        if address == 0x4e0620:
            self.event('assert', args[2]); self.ret(); return
        if address in (ELIGIBLE, ELIGIBLE_ALT):
            assert mu.reg_read(UC_X86_REG_ECX) == POOL+50*50
            self.event('eligible' if address == ELIGIBLE else 'eligible-alt', args[0])
            self.queries += 1
            if self.queries == 1:
                mutation = self.case[8]
                if mutation == 1:
                    self.put(POOL+50*50, ALT)
                if mutation == 2:
                    self.put(POOL+15003*50+40, 1, 1)
                if mutation == 3:
                    self.put(POOL+15003*50+14, bits(200))
                if mutation == 4:
                    self.put(POOL+args[0]*50+4, 0, 2)
                if mutation == 5:
                    self.put(POOL+50*50+14, bits(200)); self.put(POOL+50*50+18, bits(200))
                if mutation == 6:
                    self.put(OUTPUT+4, 0)
                if mutation == 7:
                    self.put(OUTPUT+8, self.get(OUTPUT+4))
                if mutation == 8:
                    self.put(TYPES+77*500+0x1d4, 8)
            accepted = self.case[7] == 1 or self.case[7] == 2 and args[0] % 2 == 0
            self.ret(int(accepted), 4); return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Bomb 대상 미허용 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """원본이 직접 쓰는 것은 스택/배열/배열개수이며 raw/해시는 읽기 전용이다."""
        spans = ((STACK-0x7f000, STACK+0x1000), (ARRAY, ARRAY+400), (OUTPUT+8, OUTPUT+12))
        if any(low <= address and address+size <= high for low, high in spans):
            return
        raise RuntimeError(f'Bomb 대상 미허용 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """허용한 보호 이미지/물리 풀/TYPE/해시/배열/호출자 범위만 읽는다."""
        spans = ((0, 4), (0x400000, 0x600000), (POOL, POOL+POOL_SIZE), (HASH, HASH+0x30000),
                 (TYPES, TYPES+0x20000), (PROCESS, PROCESS+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in spans):
            raise RuntimeError(f'Bomb 대상 미허용 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """명시 등록 상태를 준비한 뒤 실제원본만 실행해 기하/목록/현재상태를 읽는다."""
        self.case = case; self.events = []; self.queries = 0; self.geometry = [0]*7
        self.mu.mem_write(TYPES, bytes(0x20000)); self.mu.mem_write(HASH, bytes(self.map_size))
        rows = objects(case[0])
        # 같은 버킷에는 원본 등록 입력과 같이 머리 삽입한다. 판단/탐색 결과는 계산하지 않는다.
        for sid, kind, state, extra, xb, yb, width, height, level in reversed(rows) if case[9] else rows:
            raw = bytearray(50); struct.pack_into('<I', raw, 0, 0x502370)
            raw[10] = kind; raw[11] = state; raw[40] = extra; struct.pack_into('<2I', raw, 14, xb, yb)
            x, y = (int(struct.unpack('<f', struct.pack('<I', b))[0]) for b in (xb, yb))
            scale = (1, 2, 4, 16)[level]; address = self.bases[level]+((y//scale)*(256//scale)+x//scale)*2
            struct.pack_into('<H', raw, 4, self.get(address)&0xffff); self.put(address, sid, 2)
            self.mu.mem_write(POOL+sid*50, bytes(raw)); self.put(TYPES+kind*500+0x1d4, width); self.put(TYPES+kind*500+0x1d8, height)
        raw = bytearray(50); struct.pack_into('<I', raw, 0, 0x502370); raw[10] = 168; struct.pack_into('<2I', raw, 14, *case[3:5]); self.mu.mem_write(POOL+50*50, bytes(raw))
        self.put(TYPES+168*500+0x44, case[2]); self.put(0x5c8464, POOL); self.put(0x59ab20, TYPES); self.put(0x531928, 256)
        self.put(0x540bc4, case[1]); self.put(0x541348, 188); self.put(0x5e4794, 0); self.put(0x594fc8, 0)
        self.mu.mem_write(ARRAY, struct.pack('<100I', *(0x11110000+i for i in range(100))))
        self.mu.mem_write(OUTPUT, struct.pack('<3I', ARRAY, case[5], case[6])); self.put(0, 0x87654321)
        InvokeOracle.invoke(self, 0x41e4b0, struct.pack('<I', OUTPUT), precision, False)
        self.returns += 1; assert self.get(0) == 0x87654321
        raws = ','.join(bytes(self.mu.mem_read(POOL+sid*50, 50)).hex() for sid in (50, *IDS))
        return ','.join(map(str, self.geometry)), ';'.join(self.events) or '-', bytes(self.mu.mem_read(ARRAY, 400)).hex(), \
            str(self.get(OUTPUT+4)), str(self.get(OUTPUT+8)), raws, str(self.get(TYPES+77*500+0x1d4))


def main():
    """독립 원문을 최초 동결하거나 기존 보호 해시/두 정밀도/입력만 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    if options.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 기대값을 다시 생성하거나 고치지 않는다.
        for name, wanted in report['sha256'].items():
            assert digest(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == report['uniqueInputs'] and len(rows)*2 == report['normalReturns']
        assert all(row[1:8] == row[8:15] for row in rows)
        print('Bomb 대상 실제 해시/기하 원문 감사', len(rows), report['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존원문은덮어쓰지않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    # 실제 두 정밀도의 정상 반환과 부수효과만 비교하여 보존한다.
    for case in cases():
        try:
            outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        except Exception as error:
            raise RuntimeError(str(case)) from error
        assert outputs[0] == outputs[1], case
        rows.append((','.join(map(str, case)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input geometry53 effects53 array53 capacity53 count53 raw53 width53 geometry64 effects64 array64 capacity64 count64 raw64 width64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_bomb_targets.py',
             'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_carrier_falling.py',
             'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=233, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'),
                  uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles),
                  nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())),
                  boundaries='41e4b0/41e3c0/스냅/제한/거리/중심/TYPE/ftol/float사각형생성/4단계실제Begin·Next/버킷/기본true필터 전체. 파생가상bc 자격과 복귀 진단만 명시 경계다. 네 배열/체인은 직접 등록한 입력이며 실제Pop/미션 경로 성공 증거가 아니다. GP/retN/canary/SEH보존/x87/실행·읽기·쓰기범위 확인. 실제 피해/게임/UI는 별도다.',
                  sha256={name: digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
