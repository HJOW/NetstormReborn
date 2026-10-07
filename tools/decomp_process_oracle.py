#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""객체 부착 프로세스(ProcessForm)·Kernel 슬롯·RegularProcess 지연/주기 이벤트를 세 실제 PE의 제한 x86으로 실행한다.

BaseProcess 생성자(패치 0041bd20 ↔ CD 0048f590)의 form 생성/부착, 종속 체인, Kernel 등록/제거/프레임 실행,
RegularProcess 생성/실행/재예약/종료, 이벤트 조회, form의 공통 destroy와 부모 삭제 중 종속 정리를 원본 명령으로 실행한다.
부모 객체의 가상 pre/post/이벤트 처리기, 프로세스 객체의 new/free, 로그, 삭제 전파만 명시적으로 대체한다(부모는 void 상태에서만 삭제한다).
게임/창/OS 실행은 없다.

    python -X utf8 tools/decomp_process_oracle.py            # fixture 생성
    python -X utf8 tools/decomp_process_oracle.py --verify   # 기록 SHA/행 수 감사
    python -X utf8 tools/decomp_process_oracle.py --trace    # 허용 목록 밖 진입점 조사(기록을 쓰지 않는다)
"""
import argparse
import collections
import csv
import hashlib
import importlib.metadata
import json
import random
import struct
import sys
import zlib
from pathlib import Path

# 이 도구가 쓰는 저장소 루트와 격리된 선택적 Python 의존성 경로.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 PE 경로. CD와 새 10.37은 같은 코드 배치다.
BINARIES = {'originals': 'originals/Netstorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
            'original1037': 'original1037/netstorm.exe'}
# x87 가수부 53/64비트 제어 워드. 두 값의 관찰이 다르면 시나리오를 제외하고 기록한다.
CONTROLS = (0x027f, 0x037f)
# 재현 가능한 합성 입력 시드, 판본당 시나리오 수, 시나리오당 연산 수 범위.
INPUT_SEED = 0x10780307
SCENARIOS = 120
OPS = (10, 18)
# SID 풀 크기(C++ 검사와 같은 값)와 격리 메모리 배치. 모든 주소는 에뮬레이터 내부 주소다.
CAPACITY = 32768
SCRATCH, TYPES, POOL = 0x10000000, 0x10001000, 0x10020000
ARENA, ARENA_SIZE = 0x12000000, 0x40000
PROCS, VTABLE, TABLE, INDEX, TEMP = (ARENA + offset for offset in (0x0, 0x10000, 0x11000, 0x12000, 0x13000))
STUBS, STOP, STACK = 0x13000000, 0x20000000, 0x3000f000
PRE, POST, UNPOP, HANDLER = (STUBS + 0x100 + index * 0x20 for index in range(4))
# 프로세스 객체 하나의 격리 크기(원본 new 0x28)와 한 시나리오의 최대 개수.
PROC_STRIDE, MAX_PROCS = 0x40, 64
# 관찰하는 Kernel 슬롯 수. 합성 시나리오는 이보다 적은 프로세스만 만든다.
WATCH_SLOTS = 96
# 판본별 실제 주소. SID 전역 순서는 풀 포인터, 내부 포인터들, freeCount, 개수, 예측 커서, 서버 플래그다.
SPECS = {
    'originals': dict(patch=True, stride=50, server_first=15000,
        sid=(0x5c8464, 0x5c8468, 0x5c846c, 0x5c8470, 0x5c8474, 0x5c8478, 0x5c847c, 0x5424b0, 0x540bc0),
        logs=(0x59a698, 0x59a738), reset=0x4abb10, allocate=0x4af1d0,
        type_ptr=0x59ab20, type_stride=500, type_count=188, ctor_off=0x1d0, form_ctor=0x4ae3d0,
        form_vtable=0x512098, base_vtable=0x501dd0, regular_vtable=0x50f420,
        kernel=0x56b808, kernel_slots=40000, now=0x55b4d0, boss=0x540bc4,
        pre_depth=0x5c8480, post_depth=0x5c8484, pop_depth=0x5c8488, rotation=0x59af74, selected=0x5caea0,
        regular_ctor=0x496e80, base_ctor=0x41bd20, kernel_run=0x471a30, kill=0x41bf10,
        find=0x4afb40, find_masked=0x4afbe0, destroy=0x4af780,
        stubs=dict(free=0x4e4354, log=0x4c2630, assert_=0x4e0620, transmit=0x4ac260)),
    'originalCD': dict(patch=False, stride=36, server_first=6000,
        sid=(0x5395dc, 0x5395e0, 0x5395e4, 0x5395e8, 0x5395ec, 0x5395f0, 0x5395f4, 0x5395c0, 0x540a28),
        logs=(0x5890b0, 0x589010), reset=0x4aaad0, allocate=0x4aae40,
        type_ptr=0x51c960, type_stride=468, type_count=171, ctor_off=0x1b0, form_ctor=0x4af5c0,
        form_vtable=0x505990, base_vtable=0x5058d8, regular_vtable=0x504670,
        kernel=0x5b7bb0, kernel_slots=4000, now=0x5484a8, boss=0x540a2c,
        pre_depth=0x5395f8, post_depth=0x5395fc, pop_depth=0x539600, rotation=None, selected=None,
        regular_ctor=0x48efd0, base_ctor=0x48f590, kernel_run=0x4ed5d0, kill=0x48f7b0,
        find=0x4ac890, find_masked=0x4ac9c0, destroy=0x4ab7e0,
        stubs=dict(free=0x4f1a50, log=0x48ca70, assert_=0x490040, transmit=0x4abb40)),
}
SPECS['original1037'] = SPECS['originalCD']
# 합성 부모 타입과 genus(flags2). 122는 두 PE에서 geyser 번호 전역의 초기값이다.
PARENT_TYPES = {82: 0, 90: 0x4000, 122: 0x10000010, 100: 0x400000}
# 처리기 반환값과 시간 증가분. 0은 종료, 음수는 유지, 양수는 재예약이다.
HANDLER_VALUES = [1.0, 0.5, 0.0, -1.0, 2.5, 0.25, 0.0, 1.0]
TIME_STEPS = [0.0, 0.25, 0.5, 1.0, 3.0]


def digest(path):
    """원본/도구/내보내기/fixture의 정확한 바이트 SHA를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fbits(value):
    """float을 IEEE 32비트 패턴으로 바꾼다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def dbits(value):
    """double을 IEEE 64비트 패턴으로 바꾼다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


class ProcessOracle:
    """PE를 가상 메모리로 읽고 검토된 프로세스/SID/삭제 몸체만 허용해 실행한다."""
    def __init__(self, binary, trace=False):
        """판본별 허용 범위·쓰기 범위·명시적 대체 경계를 준비한다. 호스트 메모리와 원본 파일은 수정하지 않는다."""
        self.binary, self.s, self.trace = binary, SPECS[binary], trace
        self.patch = self.s['patch']
        path = ROOT / BINARIES[binary]
        self.sha256 = digest(path)
        pe = pefile.PE(str(path))
        self.base = pe.OPTIONAL_HEADER.ImageBase
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(self.base, (len(image) + 0xfff) & ~0xfff)
        self.mu.mem_write(self.base, image)
        # 0번 페이지는 FS:[0](예외 처리 목록)만 받는다. 다른 쓰기는 허용 목록 밖이라 실패한다.
        for address, size in ((0, 0x1000), (SCRATCH, 0x420000), (ARENA, ARENA_SIZE), (STUBS, 0x1000),
                              (STOP, 0x1000), (STACK - 0xf000, 0x10000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/process/{binary}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.hits, self.entry_names = [], collections.Counter(), {}
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 함수의 실제 불연속 몸체만 허용한다. 주소 사이 코드는 실행 금지다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
                self.entry_names[int(row['entry'], 16)] = row['entry']
        # 처리기 대체는 x87 반환이 필요하므로 실제 명령 몇 개로 둔다: mov eax,[INDEX]; fld [TABLE+eax*4]; inc [INDEX]; ret 0xc
        code = b'\xa1' + struct.pack('<I', INDEX) + b'\xd9\x04\x85' + struct.pack('<I', TABLE) + \
               b'\xff\x05' + struct.pack('<I', INDEX) + b'\xc2\x0c\x00'
        self.mu.mem_write(HANDLER, code)
        self.allowed.append((HANDLER, HANDLER + len(code)))
        for address in (PRE, POST, UNPOP): self.mu.mem_write(address, b'\xc2\x04\x00')
        s = self.s
        self.writes = [(0, 4), (POOL, POOL + CAPACITY * s['stride']), (ARENA, ARENA + ARENA_SIZE),
                       (STACK - 0xf000, STACK + 0x1000), (s['kernel'], s['kernel'] + 4 * s['kernel_slots'])]
        self.writes += [(address, address + 4) for address in s['sid'][:8]]
        self.writes += [(address, address + 160) for address in s['logs']]
        self.writes += [(s[name], s[name] + 4) for name in ('pre_depth', 'post_depth', 'pop_depth')]
        if s['rotation']: self.writes.append((s['rotation'], s['rotation'] + 4))
        self.stub_addresses = {value for value in s['stubs'].values() if value} | {PRE, POST, UNPOP}
        self.events, self.freed, self.unknown, self.previous = [], [], collections.Counter(), (0, 0)
        self.asserts = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)
        self.snapshots = {}

    def u32(self, address):
        """가상 메모리의 부호 없는 DWORD를 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def put(self, address, value, width=4):
        """가상 메모리에 little endian 정수를 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'little'))

    def sid_of(self, pointer):
        """풀 안 포인터를 SID로 바꾼다."""
        return (pointer - POOL) // self.s['stride']

    def on_write(self, mu, access, address, size, value, user_data):
        """허용 목록 밖 쓰기를 즉시 실패시킨다."""
        if not any(low <= address and address + size <= high for low, high in self.writes):
            raise RuntimeError(f'허용하지 않은 쓰기: {self.binary} {address:08x}/{size}')

    def on_instruction(self, mu, address, size, data):
        """명시적 대체를 먼저 처리하고, 나머지는 허용한 실제 몸체인지 확인한다."""
        previous, self.previous = self.previous, (address, size)
        if address in self.stub_addresses:
            s, stubs = self.s, self.s['stubs']
            esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
            ret, first = struct.unpack('<2I', mu.mem_read(esp, 8))
            purge = 0
            if address in (PRE, POST, UNPOP):
                # 부모의 가상 pre/post는 깊이만 줄이고 Unpop은 void만 켠다. 실제 자산 장부/공간 해제는 이전 단계의 범위다.
                sid = self.sid_of(ecx)
                if address == UNPOP:
                    # 생성기는 void 부모만 삭제하므로 이 대체에 도달하면 입력 계약 위반이다.
                    raise RuntimeError('부모 Unpop 대체 도달')
                else:
                    counter = s['pre_depth'] if address == PRE else s['post_depth']
                    self.put(counter, self.u32(counter) - 1)
                    self.events.append(f'{"P" if address == PRE else "O"}:{sid}:{first}')
                purge = 4
            elif address == stubs['free']:
                # 프로세스 객체 해제는 포인터만 기록한다.
                self.freed.append((first - PROCS) // PROC_STRIDE)
            elif address == stubs['transmit']:
                # 삭제 전파는 네트워크 효과이므로 인자만 기록한다(수신자 -1, 명령 -2, payload 0, flags).
                flags = struct.unpack('<I', mu.mem_read(esp + 16, 4))[0]
                self.events.append(f'X:{self.sid_of(ecx)}:{flags}')
                purge = 16
            elif address == stubs['assert_']:
                self.asserts += 1
                if not self.trace: raise RuntimeError(f'원본 assert 도달: {self.binary} 반환 {ret:08x}')
            # 로그는 cdecl 가변 인자이므로 반환만 한다.
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge); mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == HANDLER:
            # 실제 명령으로 반환값을 올리기 전에 인자를 기록한다.
            esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
            event, count, payload = struct.unpack('<3I', mu.mem_read(esp + 4, 12))
            if self.u32(INDEX) >= self.script_length: raise RuntimeError('처리기 호출이 입력 목록보다 많음')
            self.events.append(f'H:{self.sid_of(ecx)}:{event}:{count}:{payload}')
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            if not self.trace: raise RuntimeError(f'허용하지 않은 실행 주소: {self.binary} {address:08x}')
            # 조사 모드: 직전 명령과 이어지지 않는 첫 주소만 진입점 후보로 센다.
            if previous[0] + previous[1] != address: self.unknown[address] += 1
        elif address in self.entry_names: self.hits[self.entry_names[address]] += 1

    def call(self, entry, args, this=0, purge=0):
        """원본 호출 규약으로 실행하고 정상 반환·스택·x87 스택·FS:[0] 복구를 확인한다."""
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, self.control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.mem_write(0, struct.pack('<I', 0xffffffff))
        words = [STOP] + [value & 0xffffffff for value in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.previous = (0, 0)
        self.mu.emu_start(entry, STOP, timeout=60000000, count=20000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'호출 반환/스택 오류: {self.binary} {entry:08x}')
        if self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or self.mu.reg_read(UC_X86_REG_FPCW) != self.control:
            raise RuntimeError(f'x87 스택/제어 워드 오류: {self.binary} {entry:08x}')
        if self.u32(0) != 0xffffffff: raise RuntimeError(f'FS:[0] 복구 오류: {self.binary} {entry:08x}')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def reset(self, server):
        """실제 Reset으로 풀을 초기화한 상태를 서버 플래그별로 한 번만 만들고 이후에는 그 바이트를 되돌린다."""
        s = self.s
        if server not in self.snapshots:
            self.mu.mem_write(POOL, bytes([0xa5]) * (CAPACITY * s['stride']))
            for address, value in ((s['sid'][0], POOL), (s['sid'][6], CAPACITY), (s['sid'][8], server)):
                self.put(address, value)
            for address in s['logs']: self.mu.mem_write(address, bytes(160))
            self.control = CONTROLS[0]
            self.call(s['reset'], [0])
            self.snapshots[server] = (bytes(self.mu.mem_read(POOL, CAPACITY * s['stride'])),
                                      [self.u32(address) for address in s['sid']])
        pool, values = self.snapshots[server]
        self.mu.mem_write(POOL, pool)
        for address, value in zip(s['sid'], values): self.put(address, value)
        for address in s['logs']: self.mu.mem_write(address, bytes(160))

    def begin(self, scenario, control):
        """전역·타입 표·합성 부모 가상 표·Kernel 슬롯을 처음 상태로 만든다."""
        s = self.s
        self.control = control
        self.reset(scenario['server'])
        self.mu.mem_write(ARENA, bytes(ARENA_SIZE))
        self.mu.mem_write(s['kernel'], bytes(4 * s['kernel_slots']))
        self.mu.mem_write(TYPES, bytes(s['type_count'] * s['type_stride']))
        self.put(s['type_ptr'], TYPES)
        self.put(TYPES + 2 * s['type_stride'] + s['ctor_off'], s['form_ctor'])
        for number, genus in PARENT_TYPES.items(): self.put(TYPES + number * s['type_stride'] + 0xec, genus)
        # 부모는 실제 base 가상 표에서 pre/post/Unpop/이벤트 처리기만 대체한 표를 쓴다. destroy는 실제 공통 함수다.
        table = bytearray(self.mu.mem_read(s['base_vtable'], 0xb4))
        for offset, address in ((0x10, s['destroy']), (0x14, PRE), (0x18, POST), (0x48, UNPOP), (0x5c, HANDLER)):
            struct.pack_into('<I', table, offset, address)
        self.mu.mem_write(VTABLE, bytes(table))
        for name, value in (('boss', scenario['boss']), ('pre_depth', 0), ('post_depth', 0), ('pop_depth', 0)):
            self.put(s[name], value)
        if s['rotation']: self.put(s['rotation'], scenario['rotation'])
        if s['selected']: self.put(s['selected'], 0)
        self.mu.mem_write(s['now'], struct.pack('<Q', scenario['now']))
        self.now = scenario['now']
        self.parents, self.procs, self.script_length = [], [], 0
        self.events, self.freed, self.asserts = [], [], 0

    def alive(self):
        """Kernel 슬롯에 등록된 프로세스 번호를 슬롯 순서로 돌려준다."""
        s, result = self.s, []
        for slot in range(1, WATCH_SLOTS):
            pointer = self.u32(s['kernel'] + slot * 4)
            if pointer: result.append((slot, (pointer - PROCS) // PROC_STRIDE))
        return result

    def new_process(self):
        """원본 operator new(0x28)를 격리 메모리의 다음 블록으로 대체한다."""
        if len(self.procs) >= MAX_PROCS: raise RuntimeError('시나리오 프로세스 수 초과')
        self.procs.append(PROCS + len(self.procs) * PROC_STRIDE)
        return self.procs[-1]

    def apply(self, op):
        """연산 하나를 실제 함수로 실행하고 반환값을 돌려준다."""
        s, kind = self.s, op[0]
        self.events, self.freed = [], []
        if kind == 'P':
            # 부모: 실제 Allocate로 번호를 받고 합성 가상 표·타입·상태·extra를 쓴다.
            sid = self.call(s['allocate'], [op[1]])
            slot = POOL + sid * s['stride']
            self.put(slot, VTABLE); self.put(slot + 10, op[2], 1); self.put(slot + 11, op[3], 1)
            self.put(slot + (40 if self.patch else 35), op[4], 1)
            self.parents.append(sid)
            return sid
        if kind == 'A':
            # 실제 RegularProcess 생성자: 이벤트, 부착 SID, payload.
            pointer = self.new_process()
            self.call(s['regular_ctor'], [op[2], self.parents[op[1]], op[3]], this=pointer, purge=12)
            return len(self.procs) - 1
        if kind == 'B':
            # 실제 BaseProcess 생성자를 임의 flags로 호출한다. 객체의 나머지 필드는 Regular 생성자와 같은 값으로 채운다.
            pointer = self.new_process()
            self.put(pointer, s['regular_vtable'])
            self.call(s['base_ctor'], [46, self.parents[op[1]], 0, op[4]], this=pointer, purge=16)
            self.mu.mem_write(pointer + 0x10, struct.pack('<Q', self.now))
            self.put(pointer + 0x18, op[2]); self.put(pointer + 0x1c, op[3]); self.put(pointer + 0x20, 0)
            return len(self.procs) - 1
        if kind == 'T':
            self.now = op[1]
            self.mu.mem_write(s['now'], struct.pack('<Q', op[1]))
            return 0
        if kind == 'R':
            self.put(INDEX, 0)
            self.mu.mem_write(TABLE, struct.pack('<' + 'I' * len(op[1]), *op[1]))
            self.script_length = len(op[1])
            self.call(s['kernel_run'], [])
            return self.u32(INDEX)
        if kind == 'K':
            self.call(s['kill'], [op[2]], this=self.procs[op[1]], purge=4)
            return 0
        if kind in ('F', 'M'):
            parent = POOL + self.parents[op[1]] * s['stride']
            if kind == 'F': pointer = self.call(s['find'], [op[2]], this=parent, purge=4)
            else: pointer = self.call(s['find_masked'], [op[2], op[3]], this=parent, purge=8)
            return (pointer - PROCS) // PROC_STRIDE if pointer else 0xffffffff
        if kind == 'D':
            self.call(s['destroy'], [op[2]], this=POOL + self.parents[op[1]] * s['stride'], purge=4)
            return 0
        if kind == 'S':
            # 합성 상태 변경: void/abstract 비트만 뒤집는다(실제 Pop/Unpop은 이전 단계의 범위다).
            slot = POOL + self.parents[op[1]] * s['stride']
            offset = 11 if op[2] == 0 else (40 if self.patch else 35)
            self.put(slot + offset, self.mu.mem_read(slot + offset, 1)[0] ^ op[3], 1)
            return 0
        raise ValueError(kind)

    def observe(self, result):
        """C++ 재생기가 같은 순서로 만드는 관찰 문자열이다."""
        s = self.s
        slots = ','.join(f'{slot}:{index}' for slot, index in self.alive()) or '-'
        fields = []
        for slot, index in self.alive():
            pointer = self.procs[index]
            form, parent = self.u32(pointer + 4), self.u32(pointer + 8)
            time = struct.unpack('<Q', self.mu.mem_read(pointer + 0x10, 8))[0]
            fields.append(f'{index}:{form}:{parent}:{time}:{self.u32(pointer + 0x18)}:{self.u32(pointer + 0x1c)}:{self.u32(pointer + 0x20)}')
        free, cursor = self.u32(s['sid'][5]), self.u32(s['sid'][7])
        heads = struct.unpack('<H', self.mu.mem_read(POOL + 4, 2))[0], struct.unpack('<H', self.mu.mem_read(POOL + 18, 2))[0]
        tails = [(self.u32(s['sid'][index]) - POOL) // s['stride'] for index in (2, 4)]
        pool = zlib.adler32(self.mu.mem_read(POOL, CAPACITY * s['stride']))
        logs = zlib.adler32(b''.join(bytes(self.mu.mem_read(address, 160)) for address in s['logs']))
        depth = [self.u32(s[name]) for name in ('pre_depth', 'post_depth', 'pop_depth')]
        rotation = self.u32(s['rotation']) if s['rotation'] else 0
        numbers = [result, free, cursor, heads[0], tails[0], heads[1], tails[1], pool, logs, *depth, rotation]
        return '|'.join([','.join(str(v) for v in numbers), slots, ';'.join(fields) or '-',
                         ';'.join(self.events) or '-', ','.join(str(v) for v in sorted(self.freed)) or '-'])


def format_op(op):
    """연산을 TSV 한 칸의 문자열로 만든다."""
    return ':'.join([op[0]] + [','.join(str(v) for v in item) if isinstance(item, (list, tuple)) else str(item) for item in op[1:]])


def choose(oracle, rng, scenario):
    """현재 실제 상태에서 계약 안의 다음 연산을 고른다. 기대값은 계산하지 않는다."""
    s, server, patch = oracle.s, scenario['server'], oracle.patch
    alive = [index for _, index in oracle.alive()]
    def state(sid): return oracle.mu.mem_read(POOL + sid * s['stride'] + 11, 1)[0]
    usable = [i for i, sid in enumerate(oracle.parents) if not state(sid) & 3]
    any_parent = list(range(len(oracle.parents)))
    kind = rng.choice('AAAABRRRRRRTTTKFFMDDS')
    if kind in 'AB':
        # CD는 free/dead 부모에서 assert를 보고하므로 살아 있는 부모만 쓴다. 패치는 조용히 건너뛰는 경로도 넣는다.
        targets = any_parent if patch and rng.random() < 0.2 else usable
        if not targets or len(oracle.procs) >= MAX_PROCS - 1: return None
        event = rng.choice([0x2692, 1, 2, 5, 0x491, 0x5105, 0x10001, 0x10002])
        payload = fbits(rng.choice([0.5, 1.0, 0.2, 8.0, 0.0, 5.0]))
        if kind == 'A': return ('A', rng.choice(targets), event, payload)
        flags = rng.choice([0x50, 0x10, 0x40] if server else [0x50, 0x10])
        return ('B', rng.choice(targets), event, payload, flags)
    if kind == 'R': return ('R', [fbits(rng.choice(HANDLER_VALUES)) for _ in range(len(alive) + 2)])
    if kind == 'T': return ('T', dbits(struct.unpack('<d', struct.pack('<Q', oracle.now))[0] + rng.choice(TIME_STEPS)))
    if kind == 'K':
        if not alive: return None
        index = rng.choice(alive)
        return ('K', index, rng.choice([0, 0x10]))
    if kind == 'F':
        if not any_parent: return None
        return ('F', rng.choice(any_parent), rng.choice([0x2692, 1, 2, 5, 0x491, 0x5105, 0x10001, 0x10002]))
    if kind == 'M':
        if not any_parent: return None
        return ('M', rng.choice(any_parent), rng.choice([0xffff, 0x10000, 0xff]), rng.choice([0x2692, 0x10000, 1, 2]))
    if kind == 'D':
        if not usable: return None
        index = rng.choice(usable)
        # 실제 자산 Unpop은 이전 단계의 범위이므로 삭제 전에 합성 void 전환을 먼저 넣는다.
        if not state(oracle.parents[index]) & 4: return ('S', index, 0, 4)
        # 서버가 아니면 서버 번호 객체의 삭제는 수신 삭제 flag가 필요하다.
        client = oracle.parents[index] < s['server_first']
        return ('D', index, rng.choice([0, 0x10] if server or client else [8, 0x18]))
    if not usable: return None
    return ('S', rng.choice(usable), rng.choice([0, 1]), rng.choice([4, 1]))


def make_scenario(rng, patch):
    """서버/보스 플래그·시작 시각·부모 구성을 정한다."""
    server = rng.choice([1, 1, 0])
    parents = []
    for _ in range(rng.randrange(2, 5)):
        number = rng.choice(list(PARENT_TYPES))
        # 서버가 아닌 풀에서는 client 번호만 할당한다.
        flag = rng.choice([2, 0]) if server else 2
        parents.append(('P', flag, number, rng.choice([0, 0, 0, 4]), rng.choice([0, 0, 1])))
    return dict(server=server, boss=rng.choice([1, 1, 0]), now=dbits(rng.choice([0.0, 10.0, 123.456, 1e6])),
                rotation=rng.choice([0, 5, 186, 187]) if patch else 0, parents=parents)


def run_scenario(oracle, scenario, ops, control, rng=None):
    """시나리오를 실행한다. rng가 있으면 연산을 새로 고르고 없으면 주어진 연산을 재생한다."""
    oracle.begin(scenario, control)
    observations, executed = [oracle.observe(0)], []
    for op in scenario['parents']:
        observations.append(oracle.observe(oracle.apply(op)))
    if rng is not None:
        target, guard = rng.randrange(*OPS), 0
        while len(executed) < target and guard < 200:
            guard += 1
            op = choose(oracle, rng, scenario)
            if op is None: continue
            executed.append(op)
            observations.append(oracle.observe(oracle.apply(op)))
    else:
        for op in ops:
            executed.append(op)
            observations.append(oracle.observe(oracle.apply(op)))
    return executed, observations


def generate(trace):
    """세 PE에서 두 x87 정밀도로 실행하고 관찰이 같은 시나리오만 fixture에 저장한다."""
    rows = ['# 원본 x86 기계어에서 생성. 두 x87 정밀도 관찰이 같은 시나리오만 저장. 부모 가상 pre/post/처리기·new/free·로그·전파만 대체.']
    report = dict(schema=1, method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
                  unicorn_version=importlib.metadata.version('unicorn'), seed=INPUT_SEED, editions={},
                  tool_sha256=digest(Path(__file__)), fpu_controls=list(CONTROLS), capacity=CAPACITY)
    total_ops = total_scenarios = 0
    for binary in ('originals', 'originalCD', 'original1037'):
        oracle = ProcessOracle(binary, trace)
        rng = random.Random(INPUT_SEED)
        accepted = sensitive = ops_count = 0
        kinds = collections.Counter()
        for number in range(SCENARIOS if not trace else 12):
            scenario = make_scenario(rng, oracle.patch)
            try:
                ops, first = run_scenario(oracle, scenario, None, CONTROLS[0], rng)
                _, second = run_scenario(oracle, scenario, ops, CONTROLS[1])
            except Exception as error:
                # 조사 모드에서는 실패 지점까지 모은 후보를 출력하고 다음 판본으로 넘어간다.
                if not trace: raise
                print(binary, '조사 중단:', repr(error))
                break
            if first != second:
                sensitive += 1
                continue
            header = [binary, str(number), str(scenario['server']), str(scenario['boss']), str(scenario['now']), str(scenario['rotation'])]
            everything = list(scenario['parents']) + ops
            rows.append('\t'.join(header + [' '.join(format_op(op) for op in everything), ' '.join(first)]))
            accepted += 1; ops_count += len(everything)
            for op in everything: kinds[op[0]] += 1
        report['editions'][binary] = dict(binary_sha256=oracle.sha256, scenarios=accepted, operations=ops_count,
            precision_sensitive=sensitive, operation_kinds=dict(kinds), native_hits=dict(oracle.hits),
            exports={p.name: digest(p) for p in oracle.exports})
        total_ops += ops_count; total_scenarios += accepted
        if trace:
            print(binary, '허용 밖 진입점 후보:', ' '.join(f'{address:08x}' for address in sorted(oracle.unknown)), 'assert', oracle.asserts)
    if trace: return
    payload = '\n'.join(rows) + '\n'
    report.update(total_scenarios=total_scenarios, total_operations=total_ops,
                  fixture_sha256=hashlib.sha256(payload.encode('utf-8')).hexdigest())
    report['stubbed'] = ['부모 가상 preDestroy/postDestroy(깊이 감소만)', '부모 이벤트 처리기(vtable +0x5c, 입력 목록 반환)',
                         '프로세스 객체 new/free', '삭제 로그', '삭제 전파(인자 기록)', '원본 assert 보고(도달 시 실패)']
    report['limitations'] = ['합성 부모 타입/상태·로컬 프로세스(전송 flags 0x50 계열)만', '게임/창/OS 실행 없음',
                             'CD의 free/dead 부모 부착과 프로세스 전송 직렬화는 범위 밖', 'Kernel 슬롯은 앞 %d개만 직접 비교' % WATCH_SLOTS]
    fixture = ROOT / 'cpppj/tests/fixtures/process-x86.tsv'
    fixture.write_text(payload, encoding='utf-8', newline='\n')
    (ROOT / 'cpppj/recovery-process-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n',
                                                               encoding='utf-8', newline='\n')
    print(f'프로세스 x86 fixture 시나리오 {total_scenarios}개·연산 {total_ops}개 → {fixture}')


def verify():
    """기계어를 다시 실행하지 않고 원본/도구/내보내기/fixture SHA와 행 수를 감사한다."""
    report = json.loads((ROOT / 'cpppj/recovery-process-evidence.json').read_text(encoding='utf-8'))
    assert report['tool_sha256'] == digest(Path(__file__)), '도구 SHA 불일치'
    for binary, data in report['editions'].items():
        assert data['binary_sha256'] == digest(ROOT / BINARIES[binary]), f'원본 SHA 불일치: {binary}'
        for name, sha in data['exports'].items():
            assert sha == digest(ROOT / f'extracted/process/{binary}/{name}'), f'내보내기 SHA 불일치: {binary}/{name}'
    path = ROOT / 'cpppj/tests/fixtures/process-x86.tsv'
    assert hashlib.sha256(path.read_bytes()).hexdigest() == report['fixture_sha256'], 'fixture SHA 불일치'
    rows = [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == report['total_scenarios'] == sum(d['scenarios'] for d in report['editions'].values())
    assert sum(len(row[6].split(' ')) for row in rows) == report['total_operations']
    print(f'프로세스 SHA/행 수/원본 경계 감사: 시나리오 {len(rows)}개·연산 {report["total_operations"]}개')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--trace', action='store_true')
    options = parser.parse_args()
    verify() if options.verify else generate(options.trace)
