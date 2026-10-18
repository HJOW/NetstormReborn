#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 10.78의 회수 요청·54h 송신·수신을 격리 실행한다. 게임/OS는 실행하지 않는다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, TYPES, VTABLE, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import *

POOL = 0x14000000  # C++의 실제24000슬롯과 동일한 크기의 격리 풀이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/salvage-command-x86.tsv'  # 새 독립 원본 기대값이다.
REPORT = ROOT / 'cpppj/salvage-command.json'  # 정밀 PC/원본 함수/실제 반환과 출처를 고정한다.
ENTRIES = (0x44c420, 0x4437c0, 0x4436f0, 0x40aaa0, 0x4df230, 0x49a840)  # 전체 실행하는 원본 몸체다.
PACKET, THUNK, DELETE, ALT_DELETE = STOP + 0x700, STOP + 0x800, STOP + 0x900, STOP + 0x920  # 호출자/가상 효과를 분리한다.


def bits(value):
    """명시 비용 입력을 float32 비트로 저장한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """관문·현재 상태 재독·폭·패킷·비용 비교를 독립 입력으로 교차한다."""
    # op,sid,status,type,kind,owner,local,deny,checking,mission,boss,flags,sender,capacity,chain,altarCost,daisCost,mutation,protocol,message,length다.
    base = [0, 50, 0, 128, 0, 1, 1, 0, 0, 1, 1, 0x200000, 1, 24000, 0, bits(500), bits(500), 0, 1, 0x54, 8]
    rows = []
    for status, kind, content, owner, local, deny in itertools.product((0, 1, 2, 3, 4, 8), (6, 69, 70, 128, 154, 164), (0, 154, 65535), (0, 1, 2), (1, 257), (0, 1, 0xffffffff)):  # 요청은 free/dead와 로컬 회수 금지/내용물 WORD를 구별한다.
        row = base.copy(); row[2:8] = status, kind, content, owner, local, deny; rows.append(row)
    for chain, mutation in itertools.product(range(3), (0, 1, 2, 8)):  # 제단에서 다이스를 반복 조회한 뒤 현재 조건을 다시 읽는다.
        row = base.copy(); row[3] = 164; row[14] = chain; row[17] = mutation; rows.append(row)
    for cost, mutation in itertools.product((bits(500), bits(501), 0x7fc01234, bits(float('inf')), 0x80000000), (0, 3, 4)):  # checking의 부동소수 비교/복귀 진단 뒤 재독이다.
        row = base.copy(); row[8] = 1; row[16] = cost; row[17] = mutation; rows.append(row)
    row = base.copy(); row[1] = 0; rows.append(row)
    for sid, flag, protocol, length in itertools.product((0, 50, 65535, 0x10032, 0xffffffff), (0, 0x200000, 0x12345678, 0xffffffff), (0, 1, 0x80, 0xff), (0, 3, 4, 8, 65535)):  # writer의 SID/flags 상위WORD와 현재 헤더 BYTE/WORD다.
        row = base.copy(); row[0] = 1; row[1] = sid; row[11] = flag; row[18] = protocol; row[20] = length; rows.append(row)
    for sid, status, mission, sender, flag in itertools.product((0, 1, 49, 50, 23999, 24000, 65535), (0, 1, 2, 3, 4, 8), (0, 1), (1, 2, 257), (0, 0x200000, 0x210000, 0xffff1234)):  # 수신은 free만 거르고 전체DWORD 송신자/미션 모드·풀 범위를 확인한다.
        row = base.copy(); row[0] = 2; row[1] = sid; row[2] = status; row[9] = mission; row[11] = flag; row[12] = sender; rows.append(row)
    for kind, owner, boss, mutation in itertools.product((6, 69, 70, 128, 164), (1, 2), (0, 1), (0, 5, 6, 7)):  # assert 뒤 boss/패킷 재독과 환불 뒤 현재 가상Destroy를 관찰한다.
        row = base.copy(); row[0] = 2; row[3] = kind; row[5] = owner; row[10] = boss; row[17] = mutation; rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """큰 세계 효과만 경계로 두고 보호 코드의 조건·직렬화·호출 순서를 관찰한다."""
    def __init__(self):
        """기존 격리 메모리에 실제 허용 함수와 두 가상 삭제 주소를 설치한다."""
        super().__init__(); self.mu.mem_map(POOL, 0x130000); self.allowed = []; ranges(self, ENTRIES)
        self.instructions = 0; self.calls = collections.Counter(); self.returns = 0
        self.put(VTABLE + 0x10, DELETE); self.put(VTABLE + 0x110, ALT_DELETE)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def mutate(self, boundary, sid=50):
        """명시 효과가 현재 raw/전역/packet을 바꾸며 계산 결과는 입력하지 않는다."""
        mutation = self.case[17]
        if boundary == 'query':
            if mutation == 1: self.put(POOL + 51 * 50 + 11, 3, 1)
            if mutation == 2: self.put(0x595078, 1)
            if mutation == 8: self.put(0x5412e8, 128)
        if boundary == 'assert' and self.line == 0x295:
            if mutation == 3: self.put(POOL + 50 * 50 + 10, 6, 1); self.put(POOL + 50 * 50 + 18, 154, 2)
            if mutation == 4: self.put(0x540c70, 2); self.put(0x595078, 1)
        if boundary == 'assert' and self.line == 0x563 and mutation == 5:
            self.put(0x540bc4, 1); self.put(PACKET + 4, 51, 2); self.put(PACKET + 6, 0xffff, 2)
            self.put(POOL + sid * 50, VTABLE + 0x100); self.put(POOL + sid * 50 + 34, 2, 1)
        if boundary == 'refund':
            if mutation == 6: self.put(POOL + sid * 50, VTABLE + 0x100); self.put(PACKET + 4, 51, 2); self.put(PACKET + 6, 0, 2)
            if mutation == 7: self.put(POOL + sid * 50 + 34, 2, 1); self.put(POOL + sid * 50 + 11, 1, 1)

    def instruction(self, mu, address, size, data):
        """하위 경계의 실제 ABI만 반환하고 범위 밖 실행을 거부한다."""
        self.instructions += 1
        if address in ENTRIES: self.calls[f'{address:08x}'] += 1
        sp = mu.reg_read(UC_X86_REG_ESP); args = struct.unpack('<4I', mu.mem_read(sp + 4, 16))
        sid = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
        if address == 0x448bb0:
            self.event('Q', sid); self.mutate('query', sid)
            target = 51 if sid == 50 and self.case[14] else 52 if sid == 51 and self.case[14] == 2 else 0
            self.ret(target); return
        if address == 0x4cf960:
            self.event('T', self.string(args[0])); self.ret(); return
        if address == 0x4e0620:
            self.line = args[2]; self.event('A', self.line); self.mutate('assert', 50)
            self.ret(); return
        if address == 0x4e2dc0:
            self.event('S', args[0]); self.packet = bytes(mu.mem_read(args[1], 8)).hex(); self.ret(); return
        if address == 0x44c2f0:
            self.event('R', *args[:3]); self.mutate('refund', args[0]); self.ret(); return
        if address in (DELETE, ALT_DELETE):
            self.event('D' if address == DELETE else 'd', sid, args[0]); self.ret(0, 4); return
        if THUNK <= address < THUNK + 64: return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'회수 원본 실행 범위 밖 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """원본의 직접 쓰기를 호출자 스택으로 한정한다."""
        if STACK - 0x7f000 <= address and address + size <= STACK + 0x1000: return
        raise RuntimeError(f'회수 원본 직접 쓰기 범위 밖 {address:08x}+{size}')

    def read(self, mu, access, address, size, value, data):
        """보호 이미지/격리 raw·TYPE·VT/호출자 범위만 읽게 한다."""
        spans = ((0x400000, 0x600000), (POOL, POOL + 0x130000), (TYPES, TYPES + 0x20000), (VTABLE, VTABLE + 0x1000), (STOP, STOP + 0x1000), (STACK - 0x7f000, STACK + 0x1000))
        if not any(low <= address and address + size <= high for low, high in spans):
            raise RuntimeError(f'회수 원본 읽기 범위 밖 {address:08x}+{size}')

    def sample(self, case, precision):
        """독립 입력 뒤 ABI·빈 FPU·전체 raw/현재 전역/패킷/효과를 관찰한다."""
        self.case = case; self.events = []; self.packet = '-'; self.line = 0
        # 물리 후보와 마지막23999는 명시 상태이며24000/65535는 실제 풀 범위 밖이다.
        for sid in (*range(1, 100), 23999):
            raw = bytearray(b'\xa5' * 50); struct.pack_into('<I', raw, 0, VTABLE); raw[10] = case[3]; raw[11] = case[2]; raw[34] = case[5]
            struct.pack_into('<H', raw, 18, case[4]); self.mu.mem_write(POOL + sid * 50, bytes(raw))
        if case[14]:
            self.put(POOL + 51 * 50 + 10, 164 if case[14] == 2 else 128, 1); self.put(POOL + 52 * 50 + 10, 128, 1)
        for address, value in ((0x5c8464, POOL), (0x5c847c, case[13]), (0x59ab20, TYPES), (0x540c70, case[6]), (0x595078, case[7]), (0x5e4794, case[8]), (0x594fa4, case[9]), (0x540bc4, case[10]), (0x5412e8, 164), (0x5412ec, 165), (0x5412c0, 154)):
            self.put(address, value)  # 현재 전역을 입력 시점에만 초기화한다.
        self.put(TYPES + 164 * 500 + 0xc4, case[15]); self.put(TYPES + 165 * 500 + 0xc4, case[16])
        self.put(0x52ee1f, case[18], 1); self.put(0x52ee1e, case[19], 1); self.put(0x52ee20, case[20], 2)
        self.mu.mem_write(PACKET, struct.pack('<BBHHH', case[18] & 0x7f, case[19], case[20], case[1] & 65535, (case[11] >> 16) & 65535))
        entry = (0x44c420, 0x4437c0, 0x4436f0)[case[0]]
        arguments = [case[1]] if case[0] == 0 else [case[1], case[11]] if case[0] == 1 else [case[12], 0x12345678, PACKET]
        code = b''.join(b'\x68' + struct.pack('<I', value) for value in reversed(arguments))
        code += b'\xe8' + struct.pack('<i', entry - (THUNK + len(code) + 5)) + b'\x83\xc4' + bytes([4 * len(arguments)]) + b'\xc3'
        self.mu.mem_write(THUNK, code); self.mu.ctl_remove_cache(THUNK, THUNK + 64); self.mu.mem_write(STACK, struct.pack('<I', STOP) + b'\xc0\xde\xc0\xde')
        saved = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789, UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}
        for register, value in saved.items(): self.mu.reg_write(register, value)  # 보존 레지스터 canary다.
        for register, value in ((UC_X86_REG_EAX, 0), (UC_X86_REG_EDX, 0), (UC_X86_REG_ECX, 0), (UC_X86_REG_ESP, STACK), (UC_X86_REG_EFLAGS, 2), (UC_X86_REG_FPCW, precision), (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
            self.mu.reg_write(register, value)  # 휘발/부동소수 입력도 매 실행마다 초기화한다.
        self.mu.emu_start(THUNK, STOP, count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert bytes(self.mu.mem_read(STACK + 4, 4)) == b'\xc0\xde\xc0\xde'
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and not self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 and self.mu.reg_read(UC_X86_REG_FPTAG) == 0xffff
        self.returns += 1
        state = ','.join(str(self.get(address)) for address in (0x540c70, 0x595078, 0x5e4794, 0x594fa4, 0x540bc4, 0x5412e8))
        raws = ','.join(bytes(self.mu.mem_read(POOL + sid * 50, 50)).hex() for sid in (50, 51, 52))
        return ';'.join(self.events) or '-', self.packet, state, raws, bytes(self.mu.mem_read(PACKET, 8)).hex()


def main():
    """최초 독립 기대값을 생성하거나 기존 원문·입력·출처를 읽기 전용 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8')); rows = [line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        for path, want in report['sha256'].items(): assert digest(ROOT / path) == want, path  # 독립 결과를 다시 쓰지 않는다.
        assert [row[0] for row in rows] == [','.join(map(str, case)) for case in cases()] and len(rows) == report['uniqueInputs']
        assert all(row[1:6] == row[6:11] for row in rows) and len(rows) * 2 == report['normalReturns']
        print('회수 원본 감사', len(rows), report['normalReturns']); return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 원본 기대를 덮어쓰지 않는다'
    oracles = [Oracle(), Oracle()]; rows = []
    for case in cases():  # C++ 제품/결과를 읽지 않고 두 x87 정밀도의 정상 반환을 먼저 동결한다.
        observed = [oracle.sample(case, control) for oracle, control in zip(oracles, (0x27f, 0x37f))]
        assert observed[0] == observed[1], case
        rows.append((','.join(map(str, case)), *observed[0], *observed[1]))
    counts = collections.Counter(case[0] for case in cases())  # 캐시 때문에 다른 연산으로 실행되는 잘못된 관측을 거부한다.
    assert sum(o.calls['0044c420'] for o in oracles) == counts[0] * 2
    assert sum(o.calls['004436f0'] for o in oracles) == counts[2] * 2
    assert sum(o.calls['004437c0'] for o in oracles) >= counts[1] * 2
    FIXTURE.write_text('# input trace53 packet53 state53 raw53 received53 trace64 packet64 state64 raw64 received64\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', 'utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_salvage_command.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_particle_gradient.py', 'extracted/refined/originals/functions.tsv', 'cpppj/tests/fixtures/salvage-command-x86.tsv')
    report = dict(host=socket.gethostname(), date='2026-10-19', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=[], boundaries='44c420/4437c0/4436f0/40aaa0/4df230/49a840 전체. typed DaisAt/진단/Tell/전송/환불/가상Destroy는 명시 큰 하위 경계다. 두 x87/전체 raw 세 슬롯·현재 전역·caller packet/GP·ABI·canary/실행·읽기·쓰기 범위를 확인한다. 게임/OS/실제 사용자 회수/완주 증거는 아니다.', sha256={path: digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n'); print(json.dumps({key: report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
