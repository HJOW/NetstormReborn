#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Dude 정기 분배·네 하위 처리·목표 이동 wrapper·population 초기화를 독립 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, digest
from decomp_cursor_position import ranges
from decomp_nugget_regular import Oracle as InvokeOracle, POOL, CALLER, RESULT, bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

PROVIDERS, CODES, POPULATION = 0x14800000, 0x14900000, 0x14a00000  # 격리 공급자 목록·코드·population 배열이다.
DESTROY, UNPOP, POP = STOP+0x300, STOP+0x320, STOP+0x340  # 원본 부모 가상의 명시 효과 경계다.
FIXTURE = ROOT/'cpppj/tests/fixtures/dude-regular-x86.tsv'  # 제품 작성 전에 동결할 새 원문이다.
REPORT = ROOT/'cpppj/dude-regular-native.json'  # 실제 반환·범위·효과·보호 해시 기록이다.
ENTRIES = (0x450fa0, 0x451190, 0x451210, 0x451270, 0x451340, 0x4508b0, 0x4ad710, 0x4ac550, 0x49a840, 0x4ac220, 0x4558c0, 0x4558f0, 0x496e70, 0x44fb20)  # 실행할 실제 몸체 전체다.
OPERATIONS = (0x450fa0, 0x451190, 0x451210, 0x451270, 0x451340, 0x4508b0, 0x4fc9b0)  # 독립 호출의 실제 진입점이다.


def cases():
    """분배 관문·signed count·난수·공급자 순환·현재 프레임/좌표·재진입을 명시 입력으로 만든다."""
    base = [0, 0x5102, 0, bits(.3), 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, bits(21.25)]
    rows = []
    # 열: op,event_or_quick,count,payload,status,checking,walk,space,nearest,move,providerCount,index,seed,frame,mutation,targetStatus,ybits.
    for event, count, payload in itertools.product((0x1a2b, 0x5100, 0x5101, 0x5102, 0x5103, 0x5104, 0x5105, 0x5106, 0x5107, 0x5108, 1234), (0, 1, 4, 5, 10, 11, 101, 0x80000000, 0xffffffff), (bits(.3), 0x7fc01234)):
        row = base.copy(); row[1:4] = event, count, payload; rows.append(tuple(row))
    # poll 우선/진단 fallback/낙하 우선/기존 경로 대기와5107 예외를 구분한다.
    for status, checking, walk, space, event, count in itertools.product((0, 1, 2), (0, 1), (0, 1), (0, 1), (0x5102, 0x5105, 0x5107), (0, 1, 11)):
        row = base.copy(); row[1:3] = event, count; row[4:8] = status, checking, walk, space; rows.append(tuple(row))
    # 보행은 원래 count!=0의 확률 관문과 현재 공급자 목록/전역 순환/quick 인자를 읽는다.
    for count, providers, index, seed, move, quick in itertools.product((0, 1, 0xffffffff), (0, 1, 3), (0, 2), (0, 1, 0xffffffff), (0, 1), (0, 1)):
        if providers == 1 and index or providers == 0 and index:
            continue
        row = base.copy(); row[0:3] = 1, quick, count; row[9:13] = move, providers, index, seed; rows.append(tuple(row))
    # 복귀/드롭은 최대 횟수와 실제 가상 효과 후 좌표·프레임 변경을 구분한다.
    for operation, count, checking, walk, nearest, move, mutation, target in itertools.product((2, 3), (0, 1, 4, 5, 0xffffffff), (0, 1), (0, 1), (0, 1), (0, 1), (0, 1), (0, 3)):
        row = base.copy(); row[0] = operation; row[2] = count; row[5:7] = checking, walk; row[8:10] = nearest, move; row[14:16] = mutation, target; rows.append(tuple(row))
    # 낙하는100회/10회 경계와 현재 단계 재조회·실제 RNG·y 상한을 관측한다.
    for count, frame, seed, mutation, status in itertools.product((0, 1, 9, 10, 11, 99, 100, 101, 0x80000000, 0xffffffff), (0, 3, 5), (0, 1, 0xffffffff), (0, 1), (0, 2)):
        row = base.copy(); row[0] = 4; row[2] = count; row[4] = status; row[12:15] = seed, frame, mutation; rows.append(tuple(row))
    for count, y in itertools.product((0, 10, 100, 0x80000000), (bits(253.99), bits(254.5), bits(-10), 0x7fc01234)):
        row = base.copy(); row[0] = 4; row[2] = count; row[16] = y; rows.append(tuple(row))
    # 일반 목표 wrapper의 SID0/죽은 목표 진단과 현재 quick 인자다.
    for nearest, target, quick, move in itertools.product((0, 1), (0, 1, 2, 3), (0, 1), (0, 1)):
        row = base.copy(); row[0:2] = 5, quick; row[8:10] = nearest, move; row[15] = target; rows.append(tuple(row))
    # 실제 static initializer는 항상30칸을 요청하며 할당 배열 자체는 초기화하지 않는다.
    for fill in (0, 0xa5, 0xff):
        row = base.copy(); row[0] = 6; row[10] = fill; rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """정기/보행/복귀/드롭/낙하·RNG·타입·WORD 쓰기와 실제 정적 생성자를 실행한다."""
    def __init__(self):
        """원본 파일은 읽기만 하고 효과/자료/float32 호출자를 격리 주소에 준비한다."""
        super().__init__()
        # 기존 보호 이미지/스택과 겹치지 않는 실제 입력 저장소다.
        for address, size in ((POOL, 0x10000), (PROVIDERS, 0x1000), (CODES, 0x1000), (POPULATION, 0x1000)):
            self.mu.mem_map(address, size)
        self.allowed = []  # 실제 몸체 범위만 허용한다.
        ranges(self, ENTRIES)
        self.allowed.append((0x4fc9b0, 0x4fc9c8))  # 보호PE에서 확인한 initializer의push30..ret 전체다.
        self.instructions = 0  # 실제 실행 명령 수다.
        self.calls = collections.Counter()  # 진입점별 실제 호출 수다.
        self.case = None  # 이번 입력을 효과 경계가 읽는다.
        self.mu.mem_write(CALLER, b'\xd9\x1d'+struct.pack('<I', RESULT)+b'\xe9'+struct.pack('<i', STOP-(CALLER+11)))
        self.put(0x508cc8+0x10, DESTROY); self.put(0x508cc8+0x48, UNPOP); self.put(0x508cc8+0x90, POP)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def instruction(self, mu, address, size, data):
        """실제 계산을 유지하고 선택한 가상/공간/경로/진단·등록 효과의 인자만 기록한다."""
        self.instructions += 1
        if address in (*ENTRIES, 0x4fc9b0):
            self.calls[f'{address:08x}'] += 1
        sp = mu.reg_read(UC_X86_REG_ESP); args = struct.unpack('<5I', mu.mem_read(sp+4, 20)); this = mu.reg_read(UC_X86_REG_ECX); sid = (this-POOL)//50
        if CALLER <= address < CALLER+11:
            return
        if address == 0x450e80:
            self.event('space', sid); self.ret(self.case[7]); return
        if address == 0x4b1a90:
            self.event('lookup', *args[:4]); self.ret(52 if self.case[6] else 0); return
        if address == 0x450670:
            self.event('task', sid, *args[:2]); self.put(this+12, args[0], 2); self.ret(0, 8); return
        if address == 0x48fd30:
            self.event('nearest', args[0], args[1], args[2], args[3]); self.ret(51 if self.case[8] else 0); return
        if address == 0x4505a0:
            self.event('point', args[1]); self.put(args[0], bits(51.25)); self.put(args[0]+4, bits(52.75)); self.ret(args[0]); return
        if address == 0x450760:
            self.event('move', sid, *args[:3]); self.ret(self.case[9], 12); return
        if address == 0x4acee0:
            self.event('frame', sid, *args[:2]); self.put(this+36, args[0]); self.ret(0, 8); return
        if address == DESTROY:
            self.event('destroy', sid, args[0]); self.put(this+11, self.get(this+11)&255 | 2, 1); self.ret(0, 4); return
        if address == UNPOP:
            self.event('unpop', sid, args[0]); self.put(this+11, self.get(this+11)&255 | 4, 1)
            if self.case[14]:
                self.put(this+14, bits(42.5)); self.put(this+18, bits(43.5)); self.put(this+36, 3)
            self.ret(0, 4); return
        if address == POP:
            self.event('pop', sid, *args[:3]); self.put(this+14, args[0]); self.put(this+18, args[1]); self.put(this+11, (self.get(this+11)&255)&~4, 1); self.ret(0, 12); return
        if address == 0x4e0620:
            self.event('assert', args[2]); self.ret(); return
        if address == 0x4e4391:
            self.event('new', args[0]); assert args[0] == 120; self.ret(POPULATION); return
        if address == 0x472880:
            self.event('register', args[0]); self.ret(); return
        if address == 0x4e5b23:
            self.event('atexit', args[0]); self.ret(); return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Dude Regular 범위 밖 실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """현재 raw/순환·RNG/목록 필드·SEH·호출자 결과/스택 외 쓰기는 거부한다."""
        regions = ((STACK-0x7f000, STACK+0x1000), (POOL, POOL+0x10000), (0x55a4ec, 0x55a500), (0x532710, 0x532714), (RESULT, RESULT+4), (0, 4))
        if any(low <= address and address+size <= high for low, high in regions):
            return
        raise RuntimeError(f'Dude Regular 범위 밖 쓰기 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지·명시 raw/TYPE/목록/코드·호출자/스택만 읽는다."""
        regions = ((0, 4), (0x400000, 0x600000), (POOL, POOL+0x10000), (TYPES, TYPES+188*500), (PROVIDERS, PROVIDERS+0x1000), (CODES, CODES+0x1000), (POPULATION, POPULATION+0x1000), (STOP, STOP+0x1000), (STACK-0x7f000, STACK+0x1000))
        if any(low <= address and address+size <= high for low, high in regions):
            return
        raise RuntimeError(f'Dude Regular 범위 밖 읽기 {address:08x}+{size}')

    def sample(self, case, precision):
        """명시 입력만 설치하고 float32 반환/전체 raw/실제 RNG·순환/초기화를 관측한다."""
        self.case = case; self.events = []; self.mu.mem_write(POOL, bytes(0x10000)); raw = POOL+50*50
        self.mu.mem_write(raw, b'\xa5'*50); self.put(raw, 0x508cc8); self.put(raw+10, 70, 1); self.put(raw+11, case[4], 1); self.put(raw+12, 0x5102, 2); self.put(raw+14, bits(20.75)); self.put(raw+18, case[16]); self.put(raw+34, 2, 1); self.put(raw+36, case[13])
        # 공급자/거주 구역 목표는 같은 현재 슬롯이며 진단 입력도 별도로 보존한다.
        for sid in (51, 52, 53):
            self.put(POOL+sid*50+10, 71, 1); self.put(POOL+sid*50+11, case[15], 1)
        self.put(0x5c8464, POOL); self.put(0x59ab20, TYPES); self.put(0x5e4794, case[5]); self.put(0x5410d0, 24); self.put(0x532710, case[12]); self.put(0x55a4fc, case[11]); self.put(0x592f84, PROVIDERS); self.put(0x592f8c, case[10]); self.mu.mem_write(PROVIDERS, struct.pack('<3I', 51, 52, 53)); self.put(0, 0x87654321)
        # 3개A/2개B/1개I의 명시 코드/Run 입력이다. 기대 프레임을 계산하지 않는다.
        self.mu.mem_write(CODES, b'AP\x00\0AP\x01\0AP\x02\0BP\x00\0BP\x01\0IP\x00\0'); self.put(TYPES+70*500+0x124, CODES)
        for direction, first, count in ((0, 0, 3), (1, 3, 2), (8, 5, 1)):
            self.put(TYPES+70*500+0x130+direction*4, first); self.put(TYPES+70*500+0x170+direction*4, count)
        operation = case[0]
        arguments = (case[1], case[2], case[3]) if operation == 0 else (case[2], case[1]) if operation == 1 else (52 if case[6] else 0, case[2]) if operation in (2, 3) else (case[2],) if operation == 4 else (51 if case[8] else 0, case[1]) if operation == 5 else ()
        if operation == 6:
            self.mu.mem_write(POPULATION, bytes([case[10]])*120); self.mu.mem_write(0x55a4ec, bytes(16))
        InvokeOracle.invoke(self, OPERATIONS[operation], struct.pack('<'+'I'*len(arguments), *arguments), precision, operation < 5)
        self.returns += 1; assert self.get(0) == 0x87654321
        result = self.get(RESULT) if operation < 5 else self.mu.reg_read(UC_X86_REG_EAX)
        population = ','.join(map(str, (self.get(0x55a4ec), int(self.get(0x55a4f0) == POPULATION), self.get(0x55a4f4), self.get(0x55a4f8)))) if operation == 6 else '-'
        if operation == 6:
            assert bytes(self.mu.mem_read(POPULATION, 120)) == bytes([case[10]])*120
        return '|'.join((str(result), ';'.join(self.events) or '-', bytes(self.mu.mem_read(raw, 50)).hex(), str(self.get(0x532710)), str(self.get(0x55a4fc)), population))


def main():
    """최초 새 원문만 수집하거나 원본/도구/입력 순서/정밀도·정상 반환을 읽기 전용 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 원본과 최초 도구/원문 해시는 각각 보존한다.
        for name, wanted in report['sha256'].items():
            assert digest(ROOT/name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [row[0] for row in rows] == [','.join(map(str, case)) for case in cases()]
        assert len(rows) == report['uniqueInputs'] and len(rows)*2 == report['normalReturns'] and all(row[1] == row[2] for row in rows)
        print('Dude Regular 원본 감사:', len(rows), '입력/', report['normalReturns'], '정상 반환'); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 원문은 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    # 제품 C++나 분기 결과를 기대값 생성에 사용하지 않는다.
    for case in cases():
        output = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert output[0] == output[1], case
        rows.append((','.join(map(str, case)), *output))
    FIXTURE.write_text('# input returnBits|effects|raw|rng|index|population53 output64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/functions.tsv', 'tools/decomp_dude_regular.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_nugget_regular.py', 'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), date='2026-10-21', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-20'), uniqueInputs=len(rows), normalReturns=sum(oracle.returns for oracle in oracles), instructions=sum(oracle.instructions for oracle in oracles), nativeCalls=dict(sum((oracle.calls for oracle in oracles), collections.Counter())), controls=[0x27f, 0x37f], boundaries='450fa0/451190/451210/451270/451340/4508b0 및 실제 RNG/TYPE/FrameSide/WORD/공통496e70, initializer4fc9b0/44fb20 실행. CheckSpace/lookup/task/nearest/진입점/실제 Move·가상 공간/삭제/프레임·new/registry/atexit는 명시 인자 경계. caller float32/GP/retN/canary/SEH/x87/코드·읽기·쓰기 범위 검사. 배열의 미사용 heap은 보존하지만 제품의 미초기화 heap 복제를 요구하는 근거는 아니다. 게임/OS/창 실행 없음.', sha256={path: digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__':
    main()
