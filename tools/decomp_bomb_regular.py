#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Bomb Regular의 초기화·시간 경계·가변 대상 패킷을 제품과 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, PROCESS, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL, CALLER, RESULT
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-regular-x86.tsv'  # 새 독립 원문만 최초 동결한다.
REPORT = ROOT/'cpppj/bomb-regular-native.json'  # 실제 실행 몸체와 선언 경계를 구분한다.
ENTRIES = (0x41e6a0, 0x4ac550, 0x49a840, 0x496e70, 0x4df590, 0x40ace0, 0x4e4a40)  # 실제 PE 실행 범위다.


def bits(value):
    """명시 float 입력을 DWORD로 저장한다. 결과 기대값은 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """초기/반복·signed count·단정도 저장·NaN·대상100개·효과 재진입을 교차한다."""
    rows = []
    # 입력은 event/count/payload/casttime/QA/boss/flags/대상수/변경/프로토콜/길이/소유자/첫프레임이다.
    for count, payload, cast, boss in itertools.product((0, 1, 2, 10, 0x7fffffff, 0x80000000, 0xffffffff),
            (0, bits(.1), bits(1), bits(-1), bits(float('inf')), 0x7fc01234), (0, 1, 10, 0x7fffffff), (0, 1)):
        rows.append((0x123, count, payload, cast, 51, boss, 0, 3, 0, 1, 10, 257, 17))
    # 실제 시전 분기에서 패킷 최대 길이와 복귀 뒤 현재 상태를 확인한다.
    for qa, flags, targets, mutation in itertools.product((0, 51), (0, 1, 2, 4, 8), (0, 1, 3, 100), range(9)):
        rows.append((0xffffffff, 2, bits(1), 1, qa, 1, flags, targets, mutation, 0x81, 2, 257, 17))
    # 초기 notice/frame 변경 뒤 TYPE와 boss 재조회, 정확히 같은 시간 경계다.
    for count, mutation in itertools.product((0, 1, 2, 10, 16777217), range(9)):
        rows.append((0, count, bits(1), 16777216 if count == 16777217 else 1, 51, 1, 0, 3, mutation, 1, 10, 9, 17))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """Regular/TYPE/기본 반환/패킷 writer를 실행하고 큰 월드 효과만 기록한다."""
    def __init__(self):
        """격리 raw/대상 배열과 실제 float32 복귀 호출자를 설치한다."""
        super().__init__()
        self.mu.mem_map(POOL, 0x10000)
        self.allowed = []
        ranges(self, ENTRIES)
        self.instructions = 0
        self.calls = collections.Counter()
        self.mu.mem_write(CALLER, b'\xd9\x1d'+struct.pack('<I', RESULT)+b'\xe9'+struct.pack('<i', STOP-(CALLER+11)))
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def packet(self, address):
        """원본이 FF로 초기화한210바이트 전체를 보존한다."""
        return bytes(self.mu.mem_read(address, 210)).hex()

    def instruction(self, mu, address, size, data):
        """하위 입력과 효과 순서를 기록하며 명시 재진입만 raw/전역에 반영한다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        args = struct.unpack('<4I', mu.mem_read(sp+4, 16))
        sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//50
        mutation = self.case[8]
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x45a6c0:
            self.event('notice', *args[:3])
            if mutation == 1:
                self.put(POOL+50*50+10, 169, 1)
            self.ret(); return
        if address == 0x4acee0:
            self.event('frame', sid, *args[:2])
            self.put(POOL+50*50+36, args[0])
            if mutation == 2:
                self.put(0x540bc4, 0)
            self.ret(0, 8); return
        if address == 0x4afc90:
            self.event('animate', sid, *args[:2])
            if mutation == 3:
                self.put(POOL+50*50+10, 169, 1)
            if mutation == 4:
                self.put(0x540bc4, 0)
            self.ret(0, 8); return
        if address == 0x4e4391:
            self.event('allocate', args[0]); self.ret(PROCESS); return
        if address == 0x41e4b0:
            assert self.get(args[0]) == PROCESS and self.get(args[0]+4) == 100 and self.get(args[0]+8) == 0
            self.event('targets', sid, 100)
            # 대상 검색은 명시 경계이며 독립 입력 번호만 배열에 넣는다.
            for index in range(self.case[7]):
                self.put(PROCESS+4*index, 60+index)
            self.put(args[0]+8, self.case[7])
            if mutation == 5:
                self.put(0x52ee3b, 3, 1); self.put(0x544c6c, 6, 2); self.put(0x540c70, 513)
            self.ret(0, 4); return
        if address == 0x4e2e30:
            self.events.append('broadcast:'+self.packet(args[0]))
            if mutation == 6:
                self.put(0x540c70, 769); self.put(args[0]+10, 222, 2)
            self.ret(); return
        if address == 0x420bf0:
            self.events.append(f'receive:{args[0]}:{args[1]}:'+self.packet(args[2]))
            if mutation == 7:
                self.put(POOL+50*50+11, 2, 1)
            self.ret(); return
        if address == 0x4e4354:
            assert args[0] == PROCESS
            self.event('release', 400)
            if mutation == 8:
                self.put(POOL+50*50+11, 2, 1)
            self.ret(); return
        if CALLER <= address < CALLER+11:
            return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Bomb Regular 미허용 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """몸체의 직접 쓰기는 스택·SEH·실제 호출자 결과만 허용한다."""
        if STACK-0x7f000 <= address and address+size <= STACK+0x1000 or address == 0 and size == 4 or address == RESULT and size == 4:
            return
        raise RuntimeError(f'Bomb Regular 미허용 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지·독립 raw/TYPE/배열·호출자·스택 밖 읽기를 거부한다."""
        spans = ((0, 4), (0x400000, 0x600000), (POOL, POOL+0x10000), (TYPES, TYPES+0x20000),
                 (PROCESS, PROCESS+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if not any(low <= address and address+size <= high for low, high in spans):
            raise RuntimeError(f'Bomb Regular 미허용 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """제품 출력 없이 명시 입력을 공급하고 원본 반환/효과/전체raw만 읽는다."""
        self.case = case
        self.events = []
        raw = bytearray(50)
        raw[10] = 168; raw[11] = case[6]
        struct.pack_into('<H', raw, 12, case[4])
        self.mu.mem_write(POOL+50*50, bytes(raw))
        self.mu.mem_write(TYPES, bytes(0x20000))
        # 초기/교체 TYPE 값은 외부 효과의 독립 입력이다.
        for kind, cast, frame in ((168, case[3], case[12]), (169, 1, 99)):
            self.put(TYPES+500*kind+0x1b8, cast); self.put(TYPES+500*kind+0x130, frame)
        for address, value in ((0x5c8464, POOL), (0x59ab20, TYPES), (0x540bc4, case[5]), (0x540c70, case[11])):
            self.put(address, value)
        self.put(0x52ee3a, 0x5f, 1); self.put(0x52ee3b, case[9], 1); self.put(0x544c6c, case[10], 2); self.put(0, 0x87654321)
        InvokeOracle.invoke(self, 0x41e6a0, struct.pack('<3I', *case[:3]), precision, True)
        self.returns += 1
        assert self.get(0) == 0x87654321
        return str(self.get(RESULT)), ';'.join(self.events) or '-', bytes(self.mu.mem_read(POOL+50*50, 50)).hex()


def main():
    """최초 원문을 동결하거나 입력/보호 해시/두 정밀도의 기존 원문만 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if options.verify:
        record = json.loads(REPORT.read_text('utf-8'))
        # 기존 기대값을 다시 계산하거나 변경하지 않는다.
        for path, wanted in record['sha256'].items():
            assert digest(ROOT/path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int, row[0].split(','))) for row in rows] == cases()
        assert len(rows) == record['uniqueInputs'] and len(rows)*2 == record['normalReturns']
        assert all(row[1:4] == row[4:7] for row in rows)
        print('Bomb Regular 원문 감사', len(rows), record['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 동결 원문은 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]
    rows = []
    # 두 정밀도의 실제 반환과 부수효과가 같아야 같은 행으로 동결한다.
    for case in cases():
        try:
            results = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        except Exception as error:
            raise RuntimeError(str(case)) from error
        assert results[0] == results[1], case
        rows.append((','.join(map(str, case)), *results[0], *results[1]))
    FIXTURE.write_text('# input return53 effects53 raw53 return64 effects64 raw64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_bomb_regular.py',
             'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_carrier_falling.py',
             'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=230, host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-19'),
                  uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles),
                  nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())),
                  boundaries='41e6a0/TYPE/기본496e70/가변패킷writer 전체. 45a6c0 통지·프레임/애니메이션·new400/free·대상검색41e4b0·배포4e2e30·실제효과420bf0는 명시 경계다. GP/retN/canary/SEH/x87/실행·읽기·쓰기범위를 검사했다. 피해/내용물 소비/실제 시전/UI 완료의 증거가 아니다.',
                  sha256={path: digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
