#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 붕괴 스캔·한 칸 처리·수명 감소·금 간 프레임 전환을 실제 원본 x86에서 얻는다.

원본 게임/OS를 실행하지 않는다. Ghidra가 내보낸 함수 몸체만 Unicorn 가상 메모리에서 실행한다.
대체하는 것은 메모리 할당/해제, 소리, 칸 위 이동체 탐색, 기본 destroy, 공통 화면 갱신의 진입점뿐이며
호출 사실과 인자를 사건으로 기록한다. 그래프 조회·이웃 탐색·열린 방향·방문 목록·수명 비트·프레임 검색·
다리 destroy 재정의·스캔 커서/시각은 모두 원본 기계어가 계산한다.

python -X utf8 tools/decomp_bridgedecay_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_bridgedecay_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
"""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

# 저장소 루트와 격리된 분석용 Python 의존성 경로.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
sys.path.insert(0, str(ROOT / 'tools'))
import pefile
from unicorn import (Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE,
    UC_HOOK_MEM_READ)
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 에뮬레이터 내부 주소. 호스트 메모리와 무관하며 서로 겹치지 않는다.
TABLE = 0x11000000      # 그래프 레코드 255개 × 6바이트
VISITED = 0x11001000    # 붕괴 방문 목록(정적 객체 용량 100)
HEAP = 0x11010000       # operator new 대체가 내주는 이웃 목록 버퍼
TYPES = 0x11100000      # 타입 구조체 배열(번호 0부터)
CODES = 0x11200000      # 타입별 프레임 코드 배열
MAP = 0x11300000        # 256×256 ushort 표면 번호 지도(해시 0단계 머리)
SPOTS = 0x11320000      # 256×256 byte spot 지도
POOL = 0x12000000       # Squid 배열
STOP = 0x20000000       # 정상 반환을 확인하는 가상 주소
STACK_BASE, STACK = 0x30000000, 0x3003f000  # 가상 스택(재귀 포함 256KB)
# 풀 슬롯 수. 패치판 스캔의 마지막 번호 23001을 담는다.
CAPACITY = 24000
# 타입 한 개가 가질 수 있는 프레임 코드 수와 입력 타입 수의 한도.
MAX_FRAMES, MAX_TYPES = 64, 256
# 한 번의 호출에 허용하는 명령 수. 접합 고리의 끝없는 재귀는 이 한도나 스택 고갈로 중단된다.
MAX_INSTRUCTIONS = 4000000
# 두 x87 정밀도 제어 워드(53비트·64비트, 반올림 nearest, 예외 마스크).
FPU_CONTROLS = (0x027f, 0x037f)
# 입력만 만드는 고정 시드. 기대 결과는 원본 기계어가 정한다.
SEED = 0x4227e0
# 실제 bridge.type 프레임 표를 쓰는 타입 종류와 섬 한 프레임 타입 종류.
KIND_BRIDGE, KIND_ISLAND = 0, 1
# 섬 타입의 프레임 코드 한 개(사방 연결 A, 변형 A).
ISLAND_CODES = bytes([ord('A'), ord('A'), 1, 0])

# 판본별 함수·대체 진입점·전역·슬롯 오프셋.
SPECS = {
 'originals': dict(binary='originals/Netstorm.exe', stride=50, type_stride=500,
    entries=dict(Scan=0x422bc0, Init=0x422490, Cell=0x4227e0, Life=0x421c30, Crack=0x421b60,
        Normal=0x421bd0, Weaken=0x421db0, Restore=0x421e60, Destroy=0x4220f0),
    stubs={0x4e4391: 'new', 0x4e4354: 'free', 0x4a9d70: 'sound', 0x4202f0: 'carriers',
        0x4af780: 'destroy', 0x4ad500: 'redraw'},
    assert_report=0x4e0620, vtable=0x5034c8, extra_code=[(0x422c93, 0x422c9c)],
    carriers_purge=12, finder_current=0x34,
    globals=dict(types=0x59ab20, pool=0x5c8464, count=0x5c847c, map=0x5c84bc, grid=0x542514,
        spots=0x5c7c44, board=0x531928, visited=0x5453c0, short=0x5453a0, progress=0x5453a4,
        table=0x562c18, first=0x52f96c, last=0x52f970, cursor=0x5453a8, next=0x5453b8,
        now=0x55b4d0, delta=0x55b4e0, pause=0x55b4b0, editor=0x5c85a4, authority=0x540bc4,
        server=0x540bc0, debug_redraw=0x5453ac, debug_keep=0x54db80, asserts=0x5e4794),
    offsets=dict(graph=30, frame=36, frame_size=4, extra=40, foot=0x1d4)),
 # CD판은 한 칸 처리·열린 방향·금 간 프레임 전환이 스캔/수명 함수 안에 인라인되어 별도 진입점이 없다.
 'originalCD': dict(binary='originalCD/NETSTORM.EXE', stride=36, type_stride=468,
    entries=dict(Scan=0x449250, Init=0x449200, Life=0x44a1c0, Destroy=0x449820),
    stubs={0x4f1650: 'new', 0x407c50: 'free', 0x438c00: 'sound', 0x4eae20: 'carriers',
        0x4ab7e0: 'destroy', 0x4ae380: 'redraw'},
    assert_report=0x490040, vtable=0x501ab0, extra_code=[],
    carriers_purge=16, finder_current=0x34,
    globals=dict(types=0x51c960, pool=0x5395dc, count=0x5395f4, map=0x52d590, grid=0x5670cc,
        spots=0x52fe48, board=0x52e9a8, visited=0x565ac0, short=0x565acc, progress=0x565ad4,
        table=0x5207f0, first=0x51f06c, last=0x51f070, cursor=0x565ad0, next=0x565ad8,
        now=0x5484a8, delta=0x548430, pause=0x50f250, editor=0x518904, authority=0x540a2c,
        server=0x540a28, debug_redraw=0x51f064, debug_keep=0x52e928),
    offsets=dict(graph=28, frame=34, frame_size=1, extra=35, foot=0x1b4)),
}
# 추가 10.37 실행 파일은 CD판과 같은 코드 배치다(InsertCD 분기 한 바이트만 다르다).
SPECS['original1037'] = dict(SPECS['originalCD'], binary='original1037/netstorm.exe')


def float_bits(value):
    """단정도 값의 실제 비트를 정수로 돌려준다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def double_bits(value):
    """배정도 값의 실제 비트를 정수로 돌려준다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def digest(path):
    """파일의 SHA-256."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Skip(Exception):
    """원본이 정상 반환하지 않는 입력(접합 고리의 끝없는 재귀 등)을 기대값에서 제외한다."""


class DecayOracle:
    """PE를 가상 메모리에 올리고 검토한 몸체·지정한 진입점 대체·허용 쓰기만 실행한다."""
    def __init__(self, edition):
        """판본의 코드 허용 범위, 전역, 메모리 배치를 준비한다."""
        self.edition = edition
        self.spec = SPECS[edition]
        binary = ROOT / self.spec['binary']
        self.sha256 = digest(binary)
        pe = pefile.PE(str(binary))
        self.base = pe.OPTIONAL_HEADER.ImageBase
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(self.base, (len(image) + 4095) & ~4095)
        self.mu.mem_write(self.base, image)
        # FS:[0](예외 처리 목록)만 받는 0번 페이지. 다른 0번 페이지 접근은 NULL 역참조로 보고 실패시킨다.
        self.mu.mem_map(0, 0x1000)
        self.mu.mem_map(TABLE, 0x20000)
        self.mu.mem_map(TYPES, 0x40000)
        self.mu.mem_map(CODES, 0x20000)
        self.mu.mem_map(MAP, 0x40000)
        self.mu.mem_map(POOL, (CAPACITY * self.spec['stride'] + 4095) & ~4095)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_map(STACK_BASE, 0x40000)
        # 허용 코드 바이트 표. 내보낸 불연속 몸체와 판본별 추가 구간(catch 착지점)만 1이다.
        self.allowed = bytearray(len(image))
        self.facts = []
        with (ROOT / f'extracted/bridgedecay/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 함수마다 Ghidra가 계산한 몸체 범위를 그대로 표시한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed[low - self.base:high + 1 - self.base] = b'\x01' * (high + 1 - low)
        for low, high in self.spec['extra_code']:
            self.allowed[low - self.base:high - self.base] = b'\x01' * (high - low)
        self.stubs = dict(self.spec['stubs'])
        self.g = self.spec['globals']
        self.o = self.spec['offsets']
        self.stride = self.spec['stride']
        self.type_stride = self.spec['type_stride']
        self.events = []
        self.objects = {}
        self.mark_dead = True
        self.assertions = 0
        self.stub_calls = collections.Counter()
        self.instructions = 0
        self.heap_next = HEAP
        self.write_ranges = []
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.on_null_read, begin=0, end=0xfff)
        # 표/목록/지도 포인터와 고정 전역을 에뮬레이터 안에서만 설정한다.
        for name, value in [('types', TYPES), ('pool', POOL), ('count', CAPACITY), ('map', MAP),
                            ('grid', MAP), ('spots', SPOTS), ('board', 256), ('table', TABLE),
                            ('asserts', 0)]:
            if name in self.g:
                self.mu.mem_write(self.g[name], struct.pack('<I', value))

    def sid_of(self, pointer):
        """슬롯 주소를 번호로 바꾼다. 풀 밖 주소는 입력 오류다."""
        offset = pointer - POOL
        if offset < 0 or offset % self.stride or offset // self.stride >= CAPACITY:
            raise RuntimeError(f'풀 밖 객체 포인터: {pointer:08x}')
        return offset // self.stride

    def on_null_read(self, _mu, _access, address, size, _value, _data):
        """FS:[0] 네 바이트 외의 0번 페이지 읽기는 NULL 역참조다."""
        if address != 0 or size != 4:
            raise RuntimeError(f'NULL 역참조 읽기: {self.edition} {address:08x}+{size}')

    def on_write(self, _mu, _access, address, size, _value, _data):
        """스택·예외 목록·목록 버퍼·지정 전역·객체의 수명 단어/프레임 외의 쓰기를 거부한다."""
        if STACK_BASE <= address and address + size <= STACK_BASE + 0x40000:
            return
        if address == 0 and size == 4:
            return
        if any(low <= address and address + size <= high for low, high in self.write_ranges):
            return
        offset = address - POOL
        if 0 <= offset < CAPACITY * self.stride:
            field = offset % self.stride
            frame = self.o['frame']
            if (0xc <= field and field + size <= 0xe) or (frame <= field and field + size <= frame + self.o['frame_size']):
                return
        raise RuntimeError(f'허용하지 않은 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self, mu, address, size, _data):
        """대체 진입점은 사건을 기록하고 반환한다. 그 밖에는 내보낸 몸체 안의 명령만 허용한다."""
        self.instructions += 1
        stub = self.stubs.get(address)
        if stub:
            self.stub_calls[stub] += 1
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, a0, a1, a2 = struct.unpack('<IIII', mu.mem_read(esp, 16))
            ecx = mu.reg_read(UC_X86_REG_ECX)
            purge = 0
            if stub == 'new':
                # 이웃 목록 버퍼(40바이트)만 요청된다. 호출마다 새 주소를 준다.
                if a0 != 0x28:
                    raise RuntimeError(f'예상 밖 할당 크기 {a0}')
                mu.reg_write(UC_X86_REG_EAX, self.heap_next)
                self.heap_next += 0x40
            elif stub == 'free':
                pass
            elif stub == 'sound':
                # cdecl(x, y, 이름, 0): 좌표 비트만 기록한다.
                self.events.append(f'S:{a0}:{a1}')
            elif stub == 'carriers':
                # 칸 위 이동체 탐색기 생성자: 결과 없음(current = 0)으로 만들고 호출 사실만 기록한다.
                # 패치판은 float 좌표 비트, CD판은 _ftol을 거친 정수를 넘긴다. 정수 칸으로 맞춰 적는다.
                mu.mem_write(ecx + self.spec['finder_current'], bytes(4))
                mu.reg_write(UC_X86_REG_EAX, ecx)
                if self.spec['carriers_purge'] == 12:
                    a0, a1 = (int(struct.unpack('<f', struct.pack('<I', value))[0]) for value in (a0, a1))
                self.events.append(f'C:{a0}:{a1}')
                purge = self.spec['carriers_purge']
            elif stub == 'destroy':
                # 기본 destroy의 첫 동작(dead 비트)만 수행하고 인자를 기록한다.
                sid = self.sid_of(ecx)
                self.events.append(f'D:{sid}:{a0}')
                if self.mark_dead:
                    state = mu.mem_read(ecx + 0xb, 1)[0]
                    mu.mem_write(ecx + 0xb, bytes([state | 2]))
                purge = 4
            elif stub == 'redraw':
                self.events.append(f'R:{self.sid_of(ecx)}')
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == self.spec['assert_report']:
            self.assertions += 1
            raise RuntimeError(f'원본 assert 도달: {self.edition}')
        offset = address - self.base
        if offset < 0 or offset + size > len(self.allowed) or not all(self.allowed[offset:offset + size]):
            raise RuntimeError(f'허용하지 않은 실행 주소: {self.edition} {address:08x}')

    def write_global(self, name, value, fmt='<I'):
        """이름 있는 전역에 값을 쓴다."""
        self.mu.mem_write(self.g[name], struct.pack(fmt, value))

    def read_global(self, name, fmt='<I'):
        """이름 있는 전역의 값을 읽는다."""
        return struct.unpack(fmt, self.mu.mem_read(self.g[name], struct.calcsize(fmt)))[0]

    def scene(self, objects, records, bridge_codes, spots=()):
        """객체·타입·지도·그래프 표를 가상 메모리에 놓는다. 결과는 계산하지 않는다.

        objects: (sid, x, y, 폭, 높이, flags1, flags2, 종류, 프레임, state, extra, graph, word)
        records: (그래프 번호, 표면 수, 사용 여부)
        """
        self.mu.mem_write(TYPES, bytes(self.type_stride * MAX_TYPES))
        self.mu.mem_write(MAP, bytes(0x20000))
        self.mu.mem_write(SPOTS, bytes(0x10000))
        # 이전 장면의 슬롯만 지운다(풀 전체를 매번 지우지 않는다).
        for sid in self.objects:
            self.mu.mem_write(POOL + sid * self.stride, bytes(self.stride))
        table = bytearray(255 * 6)
        # 그래프 레코드의 앞 두 WORD(표면 수, 사용 여부)를 채운다.
        for graph, surfaces, in_use in records:
            struct.pack_into('<hh', table, graph * 6, surfaces, in_use)
        self.mu.mem_write(TABLE, bytes(table))
        self.mu.mem_write(CODES, bridge_codes)
        self.mu.mem_write(CODES + 0x1000, ISLAND_CODES)
        cells = [0] * 65536
        self.objects = {}
        # 객체마다 자기 타입 구조체(번호 70부터)와 풀 슬롯을 만든다.
        for index, entry in enumerate(objects):
            sid, x, y, width, height, flags1, flags2, kind, frame, state, extra, graph, word = entry
            if not (0 < sid < CAPACITY) or index + 70 >= MAX_TYPES:
                raise ValueError('입력 번호/타입 수 범위 오류')
            type_number = 70 + index
            t = TYPES + type_number * self.type_stride
            codes, count = (CODES, len(bridge_codes) // 4) if kind == KIND_BRIDGE else (CODES + 0x1000, 1)
            self.mu.mem_write(t + 0xe8, struct.pack('<II', flags1, flags2))
            self.mu.mem_write(t + 0x114, struct.pack('<I', count))
            self.mu.mem_write(t + 0x124, struct.pack('<I', codes))
            self.mu.mem_write(t + self.o['foot'], struct.pack('<II', width, height))
            slot = POOL + sid * self.stride
            self.mu.mem_write(slot, struct.pack('<I', self.spec['vtable']))
            self.mu.mem_write(slot + 0xa, bytes([type_number, state]))
            self.mu.mem_write(slot + 0xc, struct.pack('<H', word))
            self.mu.mem_write(slot + 0xe, struct.pack('<ff', x, y))
            self.mu.mem_write(slot + self.o['graph'], bytes([graph]))
            self.mu.mem_write(slot + self.o['frame'], frame.to_bytes(self.o['frame_size'], 'little'))
            self.mu.mem_write(slot + self.o['extra'], bytes([extra]))
            self.objects[sid] = entry
            # 지도는 오른쪽 아래 기준점에서 발자국 전체에 번호를 적는다(표면 탐색기의 입력 규약).
            for yy in range(y - height + 1, y + 1):
                # 같은 행에서 그 표면이 차지하는 열을 적는다.
                for xx in range(x - width + 1, x + 1):
                    if 0 <= xx < 256 and 0 <= yy < 256:
                        cells[yy * 256 + xx] = sid
        self.mu.mem_write(MAP, struct.pack('<65536H', *cells))
        spot = bytearray(65536)
        # 지정한 spot 비트만 넣는다.
        for x, y, value in spots:
            spot[y * 256 + x] = value
        self.mu.mem_write(SPOTS, bytes(spot))
        self.mu.mem_write(VISITED, bytes(400))
        self.mu.mem_write(self.g['visited'], struct.pack('<III', VISITED, 100, 0))
        self.write_global('short', 0)
        self.write_global('progress', 0)

    def modes(self, server, authority, editor, debug_redraw, debug_keep, pause=0):
        """실행 상태 전역(서버·권한·편집기·디버그·정지 깊이)을 설정한다."""
        for name, value in [('server', server), ('authority', authority), ('editor', editor),
                            ('debug_redraw', debug_redraw), ('debug_keep', debug_keep), ('pause', pause)]:
            self.write_global(name, value)

    def call(self, name, args=(), this=0, purge=0, control=FPU_CONTROLS[0]):
        """원본 호출 규약으로 실행하고 정상 반환·스택 복구·x87 상태 보존을 확인한다."""
        # 이전 호출의 레지스터 흔적을 지운다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                         UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        top = self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800
        words = [STOP] + [value & 0xffffffff for value in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.mu.mem_write(0, bytes(4))
        self.events = []
        self.heap_next = HEAP
        self.write_ranges = [(HEAP, HEAP + 0x10000), (VISITED, VISITED + 400),
            (self.g['visited'] + 8, self.g['visited'] + 12), (self.g['short'], self.g['short'] + 4),
            (self.g['progress'], self.g['progress'] + 4), (self.g['cursor'], self.g['cursor'] + 4),
            (self.g['next'], self.g['next'] + 8), (self.g['first'], self.g['first'] + 4),
            (self.g['last'], self.g['last'] + 4)]
        try:
            self.mu.emu_start(self.spec['entries'][name], STOP, count=MAX_INSTRUCTIONS)
        except UcError as error:
            # 스택 고갈은 끝없는 재귀 입력이다. 그 밖의 메모리 오류는 그대로 실패시킨다.
            if self.mu.reg_read(UC_X86_REG_ESP) < STACK_BASE + 0x1000:
                raise Skip('스택 고갈') from error
            raise
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP:
            raise Skip('명령 수 한도 안에 반환하지 않음')
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{name}: 스택 복구 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 != top:
            raise RuntimeError(f'{name}: x87 제어 워드/TOP 불일치')
        if struct.unpack('<I', self.mu.mem_read(0, 4))[0] != 0:
            raise RuntimeError(f'{name}: 예외 처리 목록을 복구하지 않음')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def slot(self, sid):
        """슬롯 주소."""
        return POOL + sid * self.stride

    def after(self):
        """장면의 모든 객체에서 수명 단어·프레임·state를 읽는다."""
        result = []
        # 입력 순서대로 읽는다.
        for sid in self.objects:
            slot = self.slot(sid)
            word = struct.unpack('<H', self.mu.mem_read(slot + 0xc, 2))[0]
            frame = int.from_bytes(self.mu.mem_read(slot + self.o['frame'], self.o['frame_size']), 'little')
            state = self.mu.mem_read(slot + 0xb, 1)[0]
            result.append((sid, word, frame, state))
        return result

    def cell(self, sid, control, through_scan=False):
        """한 칸 처리 한 번의 사건·결과 상태를 돌려준다.

        별도 진입점이 없는 판본(또는 through_scan)은 스캔 범위를 그 번호 하나로 좁혀 실제 스캔 함수로 실행한다.
        범위 끝 = 번호, 범위 = 1, 남은 시간 0 → 처리 개수 1이 되어 그 칸만 처리한다.
        """
        if 'Cell' in self.spec['entries'] and not through_scan:
            self.call('Cell', [], self.slot(sid), 0, control)
            return list(self.events), self.after()
        self.write_global('first', sid - 1, '<i')
        self.write_global('last', sid, '<i')
        self.write_global('cursor', sid, '<i')
        self.write_global('next', 0.0, '<d')
        self.write_global('now', 0.0, '<d')
        self.write_global('delta', 0.0, '<d')
        self.call('Scan', [], 0, 0, control)
        return list(self.events), self.after()

    def life(self, sid, reduction, control):
        """수명 감소 한 번(외부 효과 진입점은 사건으로 대체)."""
        self.call('Life', [reduction], self.slot(sid), 4, control)
        return list(self.events), self.after()

    def frame_change(self, name, sid, args, purge, control):
        """프레임 전환 함수 한 번의 반환값·사건·결과 상태."""
        value = self.call(name, args, self.slot(sid), purge, control)
        return value, list(self.events), self.after()

    def destroy(self, sid, flags, control):
        """다리 destroy 재정의가 기본 destroy로 넘어가는지."""
        self.call('Destroy', [flags], self.slot(sid), 4, control)
        return list(self.events), self.after()

    def scan_init(self, now, control):
        """스캔 초기화 뒤의 범위·커서·다음 시각."""
        self.write_global('now', now, '<d')
        self.call('Init', [], 0, 0, control)
        return (self.read_global('first', '<i'), self.read_global('last', '<i'),
                self.read_global('cursor', '<i'), self.read_global('next', '<Q'))

    def scan(self, now, delta, control):
        """스캔 한 프레임의 사건과 커서·다음 시각."""
        self.write_global('now', now, '<d')
        self.write_global('delta', delta, '<d')
        self.call('Scan', [], 0, 0, control)
        return list(self.events), self.read_global('cursor', '<i'), self.read_global('next', '<Q')


# ---------------------------------------------------------------------------
# 입력 생성과 기대값 기록. 아래 함수들은 입력만 만들며 결과는 계산하지 않는다.
# ---------------------------------------------------------------------------

# 기대값·근거 기록의 저장 위치.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bridgedecay-x86.tsv'
EVIDENCE = ROOT / 'cpppj/recovery-bridgedecay-evidence.json'
# 실행하는 세 실제 PE. CD와 추가 10.37은 같은 코드 배치다.
EDITIONS = ('originals', 'originalCD', 'original1037')
# 시험 장면의 기준 칸과 첫 번호(세 판본의 풀 범위 안).
ORIGIN_X, ORIGIN_Y, FIRST_SID = 100, 100, 6100
# 타입 플래그: 표면(flags1), 섬·다리(flags2).
SURFACE, ISLAND, BRIDGE = 0x800, 2, 4


def packed(rows):
    """정수 목록들을 한 TSV 필드로 보존한다."""
    return ';'.join(','.join(map(str, row)) for row in rows) or '-'


def joined(events):
    """사건 목록을 한 TSV 필드로 보존한다."""
    return ' '.join(events) or '-'


def frame_index(codes, side, number):
    """프레임 표에서 (방향 글자, 번호)가 같은 첫 프레임 번호. 입력 구성용이다."""
    # 표를 앞에서부터 본다.
    for index in range(len(codes) // 4):
        if codes[index * 4] == ord(side) and codes[index * 4 + 2] == number:
            return index
    raise KeyError((side, number))


def bridge(codes, sid, x, y, side, life=0, number=1, graph=1, state=0, extra=0):
    """다리 한 칸의 입력 레코드."""
    return (sid, x, y, 1, 1, SURFACE, BRIDGE, KIND_BRIDGE, frame_index(codes, side, number), state, extra, graph, life << 3)


def island(sid, x, y, width=1, height=1, graph=1):
    """섬 표면 한 개의 입력 레코드(기준점은 발자국의 오른쪽 아래 칸)."""
    return (sid, x, y, width, height, SURFACE, ISLAND, KIND_ISLAND, 0, 0, 0, graph, 0)


def fixed_scenes(codes):
    """의도한 분기를 지나는 고정 장면들: (이름, 객체들, 그래프 레코드들, spot들)."""
    x0, y0, s = ORIGIN_X, ORIGIN_Y, FIRST_SID
    scenes = []
    # 섬에 붙은 가로 판자 사슬: 끝 칸의 수명 0~7과 길이 1~4.
    for length in range(1, 5):
        for life in range(8):
            objects = [island(s, x0 - 1, y0)]
            objects += [bridge(codes, s + 1 + i, x0 + i, y0, 'K', life if i == length - 1 else (i * 3) % 8) for i in range(length)]
            scenes.append((f'chain{length}-{life}', objects, [(1, 5 + length, 1)], []))
    # 세로 사슬과 금 간/단단한 프레임이 섞인 사슬.
    for number in (1, 2, 11, 12, 20):
        for life in (0, 1, 5, 6):
            objects = [island(s, x0, y0 - 1), bridge(codes, s + 1, x0, y0, 'J', 5, 1),
                       bridge(codes, s + 2, x0, y0 + 1, 'J', life, number), bridge(codes, s + 3, x0, y0 + 2, 'J', life, number)]
            scenes.append((f'vertical{number}-{life}', objects, [(1, 6, 1)], []))
    # 표면 수 경계(4/5)와 음수·0.
    for surfaces in (-3, 0, 1, 4, 5):
        for number in (1, 11, 20):
            objects = [island(s, x0 - 1, y0), bridge(codes, s + 1, x0, y0, 'K', 3, number), bridge(codes, s + 2, x0 + 1, y0, 'K', 2, number)]
            scenes.append((f'graph{surfaces}-{number}', objects, [(1, surfaces, 1)], []))
    # 이웃이 없는 칸: 수명 1 감소.
    for side in 'ABJKLP':
        for life in range(8):
            scenes.append((f'alone{side}{life}', [bridge(codes, s + 1, x0, y0, side, life, 0 if side == 'P' else 1)], [(1, 9, 1)], []))
    # 양쪽이 이어져 열린 쪽이 없는 판자, 섬 사이의 접합.
    scenes.append(('closed-plank', [island(s, x0 - 1, y0), bridge(codes, s + 1, x0, y0, 'K', 3), island(s + 2, x0 + 1, y0)], [(1, 9, 1)], []))
    scenes.append(('closed-junction', [island(s, x0 - 1, y0), bridge(codes, s + 1, x0, y0, 'C', 3), island(s + 2, x0 + 1, y0),
        island(s + 3, x0, y0 + 1)], [(1, 9, 1)], []))
    # T자·모퉁이·끝 칸·십자·접합 사슬. 수명 조합을 바꿔 최소 수명/목표 계산을 지난다.
    shapes = {
        'tee': [('I', -1, 0), ('K', 0, 0), ('C', 1, 0), ('K', 2, 0), ('J', 1, 1)],
        'corner': [('I', -1, 0), ('K', 0, 0), ('G', 1, 0), ('J', 1, 1), ('J', 1, 2)],
        'ends': [('I', -1, 0), ('K', 0, 0), ('C', 1, 0), ('O', 0, 1), ('M', 2, 0), ('L', 1, -1), ('N', 1, 1)],
        'cross': [('I', -2, 0), ('K', -1, 0), ('A', 0, 0), ('K', 1, 0), ('J', 0, -1), ('J', 0, 1)],
        'junctions': [('I', -1, 0), ('E', 0, 0), ('A', 1, 0), ('C', 2, 0), ('K', 3, 0), ('J', 1, 1), ('J', 2, 1), ('N', 0, -1)],
        'island3': [('I3', -1, 1), ('K', 0, 0), ('K', 1, 0), ('B', 2, 0), ('J', 2, -1), ('J', 2, 1)],
    }
    life_sets = [(0,), (7,), (5,), (1,), (0, 5, 3), (6, 6, 6), (2, 0, 7, 1), (5, 4), (1, 1, 0)]
    for name, cells in shapes.items():
        for lives in life_sets:
            for numbers in ((1,), (1, 11), (20, 1), (1, 20, 11)):
                objects, index = [], 0
                # 모양의 칸을 차례로 객체로 바꾼다.
                for side, dx, dy in cells:
                    sid = s + len(objects)
                    if side == 'I':
                        objects.append(island(sid, x0 + dx, y0 + dy))
                    elif side == 'I3':
                        objects.append(island(sid, x0 + dx, y0 + dy, 3, 3))
                    else:
                        objects.append(bridge(codes, sid, x0 + dx, y0 + dy, side, lives[index % len(lives)], numbers[index % len(numbers)]))
                        index += 1
                lives_text = ''.join(map(str, lives))
                numbers_text = '-'.join(map(str, numbers))
                scenes.append((f'{name}-{lives_text}-{numbers_text}', objects, [(1, 7, 1)], []))
    # 방문 목록의 접합 칸이 다른 그래프(표면 수 4)에 속하면 단단해도 제거가 진행된다.
    objects = [island(s, x0 - 1, y0), bridge(codes, s + 1, x0, y0, 'K', 1), bridge(codes, s + 2, x0 + 1, y0, 'C', 1, 20, graph=2),
               bridge(codes, s + 3, x0 + 2, y0, 'K', 1), bridge(codes, s + 4, x0 + 1, y0 + 1, 'J', 1, 20)]
    scenes.append(('split-graph', objects, [(1, 6, 1), (2, 4, 1)], []))
    # 이웃 기준점의 안쪽 spot 비트는 그 이웃을 탐색에서 뺀다.
    objects = [island(s, x0 - 1, y0), bridge(codes, s + 1, x0, y0, 'K', 4), bridge(codes, s + 2, x0 + 1, y0, 'K', 4)]
    scenes.append(('inner-spot', objects, [(1, 6, 1)], [(x0 - 1, y0, 8)]))
    scenes.append(('inner-spot-bridge', objects, [(1, 6, 1)], [(x0, y0, 8)]))
    return scenes


def random_scenes(codes, rng, count):
    """작은 격자에 무작위 다리·섬을 놓은 장면들. 접합 고리는 실행 단계에서 제외된다."""
    letters = 'JJJJKKKKBCDEFGHILMNOA'
    scenes = []
    # 장면마다 3~10칸을 인접하게 키운다.
    for number in range(count):
        cells = {(0, 0)}
        target = rng.randrange(3, 11)
        # 이미 놓은 칸의 상하좌우로 넓힌다.
        while len(cells) < target:
            x, y = rng.choice(sorted(cells))
            dx, dy = rng.choice([(1, 0), (-1, 0), (0, 1), (0, -1)])
            if -3 <= x + dx <= 3 and -3 <= y + dy <= 3:
                cells.add((x + dx, y + dy))
        objects = []
        # 칸마다 섬 또는 임의 글자의 다리를 놓는다.
        for x, y in sorted(cells):
            sid = FIRST_SID + len(objects)
            if rng.random() < 0.18:
                objects.append(island(sid, ORIGIN_X + x, ORIGIN_Y + y))
            else:
                side = rng.choice(letters)
                kind = rng.choice([1, 1, 1, 1, 11, 20])
                life = rng.choice([0, 0, 1, 2, 3, 4, 5, 5, 6, 7])
                objects.append(bridge(codes, sid, ORIGIN_X + x, ORIGIN_Y + y, side, life, kind))
        scenes.append((f'random{number}', objects, [(1, rng.choice([4, 5, 6, 12]), 1)], []))
    return scenes


def run_all(oracles, action):
    """세 판본 × 두 x87 정밀도에서 같은 동작을 실행하고 결과가 모두 같은지 확인한다.

    정상 반환하지 않는 입력은 모든 실행에서 똑같이 제외돼야 한다.
    """
    results = []
    # 판본·정밀도마다 새로 실행한다(action이 장면을 다시 놓는다).
    for edition in EDITIONS:
        for control in FPU_CONTROLS:
            try:
                results.append(action(oracles[edition], control))
            except Skip:
                results.append(None)
    if any(result != results[0] for result in results):
        raise RuntimeError(f'판본/정밀도 불일치: {results}')
    return results[0]


def generate_cells(oracles, codes, rng, rows, counts, skipped):
    """한 칸 처리: 장면마다 살아 있는 모든 다리를 한 번씩 시작 칸으로 삼는다."""
    scenes = fixed_scenes(codes) + random_scenes(codes, rng, 260)
    for index, (name, objects, records, spots) in enumerate(scenes):
        mode_sets = [(1, 0, 0)]
        if index % 3 == 0:
            mode_sets.append((0, 0, 0))
        if index % 7 == 0:
            mode_sets.append((1, 1, 0))
        if index % 11 == 0:
            mode_sets.append((1, 0, 1))
        # 서버·디버그 갱신·디버그 유지 조합마다 실행한다.
        for server, debug_redraw, debug_keep in mode_sets:
            for entry in objects:
                if entry[7] != KIND_BRIDGE:
                    continue
                root = entry[0]

                def action(oracle, control):
                    """장면을 새로 놓고 그 칸 하나를 처리한다."""
                    oracle.mark_dead = True
                    oracle.scene(objects, records, codes, spots)
                    oracle.modes(server, 1, 0, debug_redraw, debug_keep)
                    return oracle.cell(root, control)
                result = run_all(oracles, action)
                if result is None:
                    skipped['cell_no_return'] += 1
                    continue
                # 패치판은 스캔 함수를 거친 경로도 직접 호출과 같아야 한다.
                patch = oracles['originals']
                patch.scene(objects, records, codes, spots)
                patch.modes(server, 1, 0, debug_redraw, debug_keep)
                if patch.cell(root, FPU_CONTROLS[0], through_scan=True) != result:
                    raise RuntimeError(f'패치판 스캔 경유 결과 불일치: {name}/{root}')
                events, after = result
                rows.append('\t'.join(['Cell', packed(objects), packed(records), packed(spots), str(server),
                    str(debug_redraw), str(debug_keep), str(root), joined(events), packed(after)]))
                counts['Cell'] += 1
        if index % 100 == 0:
            print(f'한 칸 처리 장면 {index}/{len(scenes)}', flush=True)


def generate_lives(oracles, codes, rows, counts):
    """수명 감소: 금 간 프레임 전환·제거·칸 위 이동체 확인 조건을 지난다."""
    sid = FIRST_SID
    for side in 'JKBL':
        for number in (1, 11, 20):
            for previous in range(8):
                for reduction in (-7, -2, -1, 0, 1, 2, 3, 5, 8):
                    for server in (0, 1):
                        variants = [(0, 0, 5)]
                        if reduction in (0, 1):
                            variants.append((1, 0, 5))
                        if number == 20:
                            variants += [(0, 0, 4), (0, 1, 5)]
                        # 디버그 갱신·디버그 유지·그래프 표면 수 조합.
                        for debug_redraw, debug_keep, surfaces in variants:
                            objects = [bridge(codes, sid, ORIGIN_X, ORIGIN_Y, side, previous, number)]
                            records = [(1, surfaces, 1)]

                            def action(oracle, control):
                                """객체 하나를 놓고 수명 감소를 한 번 부른다."""
                                oracle.mark_dead = True
                                oracle.scene(objects, records, codes)
                                oracle.modes(server, 1, 0, debug_redraw, debug_keep)
                                return oracle.life(sid, reduction, control)
                            events, after = run_all(oracles, action)
                            rows.append('\t'.join(['Life', packed(objects), packed(records), str(server), str(debug_redraw),
                                str(debug_keep), str(reduction), joined(events), packed(after)]))
                            counts['Life'] += 1
    print(f'수명 {counts["Life"]}개 완료', flush=True)


def generate_destroys(oracles, codes, rows, counts):
    """다리 destroy 재정의가 기본 destroy로 넘어가는 조건."""
    sid = FIRST_SID
    for editor in (0, 1):
        for authority in (0, 1):
            for extra in (0, 1, 2, 8, 9, 0x80):
                for surfaces in (-1, 4, 5, 100):
                    for number in (1, 11, 20):
                        for debug_keep in (0, 1):
                            for flags in (0, 0x2000000):
                                objects = [bridge(codes, sid, ORIGIN_X, ORIGIN_Y, 'K', 3, number, extra=extra)]
                                records = [(1, surfaces, 1)]

                                def action(oracle, control):
                                    """객체 하나에 destroy 재정의를 부르고 기본 destroy 도달 여부를 본다."""
                                    oracle.mark_dead = True
                                    oracle.scene(objects, records, codes)
                                    oracle.modes(1, authority, editor, 0, debug_keep)
                                    return oracle.destroy(sid, flags, control)
                                events, _after = run_all(oracles, action)
                                rows.append('\t'.join(['Destroy', packed(objects), packed(records), str(editor), str(authority),
                                    str(debug_keep), str(flags), joined(events)]))
                                counts['Destroy'] += 1
    print(f'destroy {counts["Destroy"]}개 완료', flush=True)


def generate_frames(patch, codes, rows, counts):
    """금 간/보통/약화/복구 전환. 별도 함수가 있는 패치판 기계어만 실행한다."""
    sid = FIRST_SID

    def drop(keep):
        """조건에 맞는 프레임 코드만 남긴 표."""
        return b''.join(codes[i:i + 4] for i in range(0, len(codes), 4) if keep(codes[i], codes[i + 2]))
    tables = {0: codes,
              1: drop(lambda side, number: not (side in (ord('J'), ord('B')) and number in (11, 12))),
              2: drop(lambda side, number: not (side == ord('K') and number == 1))}
    # 0번 표는 맨 앞 Frames 행에 이미 있다.
    for table in (1, 2):
        rows.append(f'Frames\t{table}\t{tables[table].hex()}')
    calls = [('Crack', '0', [0], 4, True), ('Crack', '1', [1], 4, True), ('Normal', '-', [], 0, True),
             ('Weaken', '-', [sid], 0, False), ('Restore', '-', [sid], 0, False)]
    for table, table_codes in tables.items():
        # 표의 모든 프레임에서 전환을 실행한다.
        for frame in range(len(table_codes) // 4):
            for word in (0x0000, 0x0038, 0xff87, 0x1e55):
                objects = [(sid, ORIGIN_X, ORIGIN_Y, 1, 1, SURFACE, BRIDGE, KIND_BRIDGE, frame, 0, 0, 1, word)]
                reference = None
                # 두 정밀도의 결과가 같아야 한다.
                for control in FPU_CONTROLS:
                    results = []
                    # 다섯 전환을 같은 입력에서 따로 실행한다.
                    for name, variant, args, purge, method in calls:
                        patch.scene(objects, [(1, 9, 1)], table_codes)
                        patch.modes(1, 1, 0, 0, 0)
                        value = patch.call(name, args, patch.slot(sid) if method else 0, purge, control)
                        # 반환값이 없는 두 함수는 EAX를 기록하지 않는다.
                        if name in ('Normal', 'Restore'):
                            value = 0
                        results.append((name, variant, value, list(patch.events), patch.after()[0]))
                    if reference is None:
                        reference = results
                    elif results != reference:
                        raise RuntimeError('프레임 전환의 정밀도 불일치')
                for name, variant, value, events, (_sid, new_word, new_frame, _state) in reference:
                    # 금 간/보통 전환은 수명 단어와 무관하므로 첫 단어에서만 적는다.
                    if name in ('Crack', 'Normal'):
                        if word == 0:
                            rows.append('\t'.join([name, str(table), str(frame), variant, str(value), str(new_frame), joined(events)]))
                            counts[name] += 1
                    else:
                        rows.append('\t'.join([name, str(table), str(frame), str(word), str(value), str(new_word), str(new_frame),
                            joined(events)]))
                        counts[name] += 1
    print('프레임 전환 완료', flush=True)


def generate_scans(oracles, codes, rows, counts, skipped):
    """스캔 커서: 프레임마다 실행 전 상태에서 처리된 번호와 커서·다음 시각."""
    # (범위 시작에서의 거리, state, extra, flags2). 대상이 아닌 객체도 섞는다.
    offsets = [(0, 0, 0, BRIDGE), (1, 0, 0, BRIDGE), (10, 0, 0, BRIDGE), (11, 4, 0, BRIDGE), (12, 0, 0, ISLAND), (13, 0, 1, BRIDGE),
               (14, 0, 8, BRIDGE), (15, 2, 0, BRIDGE), (16, 1, 0, BRIDGE), (17, 0, 0x80, BRIDGE), (33, 0, 0, BRIDGE), (34, 0, 0, BRIDGE),
               (3999, 0, 0, BRIDGE), (4000, 0, 0, BRIDGE), (7856, 0, 0, BRIDGE), (7857, 0, 0, BRIDGE), (7999, 0, 0, BRIDGE),
               (8000, 0, 0, BRIDGE), (8001, 0, 0, BRIDGE)]

    def steady(delta, frames):
        """같은 간격의 프레임 목록."""
        return [(delta, delta, 0, 0, 1)] * frames
    # 프레임: (시각 증가, delta, 정지 깊이, 편집기, 권한).
    sequences = {
        'steady14ms': (0.0, steady(0.014, 760)),
        'steady24hz': (3.5, steady(1.0 / 24.0, 500)),
        'steady60hz': (100.25, steady(1.0 / 60.0, 660)),
        'irregular': (0.0, [(d, d, 0, 0, 1) for d in [0.0, 1e-9, 0.001, 0.0125, 0.0125, 0.05, 0.1, 0.25, 0.5, 1.0, 3.0, 2.0, 5.0,
            10.0, 25.0, 0.014, 0.014, 0.3, 9.9, 0.05, 0.05, 40.0, 0.001, 0.001, 9.999, 0.0005, 0.0005, 0.0005]]),
        # 시각 증가와 delta가 서로 다른 프레임(정지에서 풀린 직후 등)과 실행 조건 전환.
        'modes': (7.0, [(0.5, 0.5, 0, 0, 1), (0.5, 0.5, 1, 0, 1), (0.0, 0.5, 1, 0, 1), (0.5, 0.5, 0, 1, 1), (0.5, 0.5, 0, 0, 0),
            (0.5, 0.5, 0, 0, 1), (9.0, 0.014, 0, 0, 1), (0.014, 9.0, 0, 0, 1), (0.014, 0.014, 0, 0, 1), (0.5, 0.0, 0, 0, 1),
            (20.0, 20.0, 2, 0, 1), (0.014, 0.014, 0, 0, 1), (0.014, 0.014, 0, 0, 1)]),
        # 비정상 delta: 음수, 범위를 넘는 큰 값, int64를 넘는 값, NaN.
        'extreme': (0.0, [(0.1, -0.5, 0, 0, 1), (0.1, 4000.0, 0, 0, 1), (0.1, 0.014, 0, 0, 1), (0.1, 1e300, 0, 0, 1),
            (0.1, float('nan'), 0, 0, 1), (0.1, 2684354.56, 0, 0, 1), (0.1, 5368709.12, 0, 0, 1), (0.1, 0.014, 0, 0, 1),
            (12.0, 0.014, 0, 0, 1), (0.1, 0.014, 0, 0, 1)]),
    }
    for edition in ('originals', 'originalCD'):
        group = [edition] + (['original1037'] if edition == 'originalCD' else [])
        for name, (start, frames) in sequences.items():
            # 1) 기준 궤적: CRT 기본값인 53비트 정밀도로 처음부터 이어서 실행한다.
            oracle = oracles[edition]
            oracle.mark_dead = False
            oracle.scene([], [], codes)
            oracle.modes(1, 1, 0, 0, 0)
            first, last, cursor, next_bits = oracle.scan_init(start, FPU_CONTROLS[0])
            objects = [(first + offset, 10 + index, 10, 1, 1, SURFACE, flags2, KIND_BRIDGE if flags2 == BRIDGE else KIND_ISLAND,
                        frame_index(codes, 'K', 1) if flags2 == BRIDGE else 0, state, extra, 1, 0)
                       for index, (offset, state, extra, flags2) in enumerate(offsets) if first + offset < CAPACITY]
            oracle.scene(objects, [(1, 1, 1)], codes)
            init = (double_bits(start), first, last, cursor, next_bits)
            reference = []
            now = start
            # 프레임마다 실행 전 커서·다음 시각도 함께 적어 각 행을 독립된 입력으로 만든다.
            for step, delta, pause, editor, authority in frames:
                now += step
                before = (cursor, next_bits)
                oracle.modes(1, authority, editor, 0, 0, pause)
                events, cursor, next_bits = oracle.scan(now, delta, FPU_CONTROLS[0])
                if any(not event.startswith('D:') or not event.endswith(':0') for event in events):
                    raise RuntimeError(f'스캔에서 예상 밖 사건: {events}')
                visited = ','.join(event.split(':')[1] for event in events) or '-'
                reference.append((double_bits(now), double_bits(delta), pause, editor, authority, before[0], before[1],
                                  visited, cursor, next_bits))
            # 2) 같은 코드 배치의 다른 PE와 64비트 정밀도는 프레임마다 기준 궤적의 실행 전 상태에서 한 번씩 실행한다.
            dependent = set()
            for member in group:
                for control in FPU_CONTROLS:
                    if member == edition and control == FPU_CONTROLS[0]:
                        continue
                    other = oracles[member]
                    other.mark_dead = False
                    other.scene([], [], codes)
                    other.modes(1, 1, 0, 0, 0)
                    if other.scan_init(start, control) != init[1:]:
                        raise RuntimeError(f'스캔 초기화 불일치: {member}/{name}/{control:#x}')
                    other.scene(objects, [(1, 1, 1)], codes)
                    # 기준 궤적의 각 프레임을 독립 입력으로 다시 실행한다.
                    for index, row in enumerate(reference):
                        now_bits, delta_bits, pause, editor, authority, cursor_before, next_before, visited, cursor, next_bits = row
                        other.modes(1, authority, editor, 0, 0, pause)
                        other.write_global('cursor', cursor_before, '<i')
                        other.write_global('next', next_before, '<Q')
                        events, got_cursor, got_next = other.scan(struct.unpack('<d', struct.pack('<Q', now_bits))[0],
                            struct.unpack('<d', struct.pack('<Q', delta_bits))[0], control)
                        got = (','.join(event.split(':')[1] for event in events) or '-', got_cursor, got_next)
                        if got != (visited, cursor, next_bits):
                            # 같은 정밀도에서 판본이 다르면 오류다. 64비트 정밀도에서만 다른 프레임은 제외하고 센다.
                            if control == FPU_CONTROLS[0]:
                                raise RuntimeError(f'스캔 판본 불일치: {member}/{name} 프레임 {index}: {got} / {row}')
                            dependent.add(index)
            rows.append('	'.join(['Init', edition, name] + [str(value) for value in init]))
            rows.append('	'.join(['ScanScene', edition, name, packed(objects)]))
            counts['Init'] += 1
            counts['ScanScene'] += 1
            # 두 정밀도가 같은 프레임만 기대값으로 남긴다.
            for index, row in enumerate(reference):
                if index in dependent:
                    skipped[f'scan_precision_dependent_{edition}'] += 1
                    continue
                rows.append('	'.join(['Scan', edition, name] + [str(value) for value in row]))
                counts['Scan'] += 1
        print(f'스캔 {edition} 완료', flush=True)


def generate():
    """세 실제 PE의 결과가 같은 범위의 기대값 TSV와 근거 기록을 만든다."""
    from decomp_bridge_oracle import codes as type_codes
    oracles = {edition: DecayOracle(edition) for edition in EDITIONS}
    codes = type_codes('originals', 'bridge')
    if codes != type_codes('originalCD', 'bridge'):
        raise RuntimeError('두 판본의 bridge 프레임 표가 다르다')
    rng = random.Random(SEED)
    rows = ['# 원본 게임 실행 없음. 대체: 할당/해제·소리·칸 위 이동체 탐색·기본 destroy(dead 표시만)·공통 화면 갱신의 진입점.',
            f'Frames\t0\t{codes.hex()}']
    counts = collections.Counter()
    skipped = collections.Counter()
    generate_cells(oracles, codes, rng, rows, counts, skipped)
    generate_lives(oracles, codes, rows, counts)
    generate_destroys(oracles, codes, rows, counts)
    generate_frames(oracles['originals'], codes, rows, counts)
    generate_scans(oracles, codes, rows, counts, skipped)
    counts['Frames'] = sum(1 for row in rows if row.startswith('Frames\t'))
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    exports = {}
    # 판본별 내보내기 두 파일의 해시를 남긴다.
    for edition in EDITIONS:
        for name in ('functions.tsv', 'creation.c'):
            path = f'extracted/bridgedecay/{edition}/{name}'
            exports[path] = digest(ROOT / path)
    sources = ['tools/decomp_bridgedecay_oracle.py', 'tools/decomp_bridge_oracle.py', 'tools/decomp_oracle.py']
    inputs = ('Cell', 'Life', 'Destroy', 'Crack', 'Normal', 'Weaken', 'Restore', 'Init', 'Scan')
    report = {
        'schema': 1, 'primary_target': '10.78',
        'method': 'Unicorn x86 32-bit; 게임 진입점·OS API 실행 없음; Ghidra 내보내기 몸체만 허용',
        'binary_sha256': {edition: oracles[edition].sha256 for edition in EDITIONS},
        'entries': {edition: {name: f'{address:08x}' for name, address in SPECS[edition]['entries'].items()} for edition in EDITIONS},
        'replaced_entry_points': {edition: {f'{address:08x}': name for address, name in SPECS[edition]['stubs'].items()}
                                  for edition in EDITIONS},
        'replaced_entry_calls': {edition: dict(oracles[edition].stub_calls) for edition in EDITIONS},
        'vtables': {edition: f'{SPECS[edition]["vtable"]:08x}' for edition in EDITIONS},
        'x87_control_words': [hex(control) for control in FPU_CONTROLS],
        'seed': SEED, 'cases': dict(counts),
        'x86_inputs': {name: counts[name] for name in inputs},
        'x86_input_total': sum(counts[name] for name in inputs),
        'skipped_inputs': dict(skipped),
        'assert_reports': {edition: oracles[edition].assertions for edition in EDITIONS},
        'instructions': {edition: oracles[edition].instructions for edition in EDITIONS},
        'fixture_sha256': digest(FIXTURE), 'export_sha256': exports,
        'source_sha256': {name: digest(ROOT / name) for name in sources},
        'limitations': [
            '합성 타입/객체·실제 bridge.type 프레임 표·정수 좌표·1×1 다리와 1×1/3×3 섬 입력',
            '표면 번호 지도는 발자국 전체에 번호를 적은 탐색기 입력 규약이다(실제 등록 순서의 복원이 아니다)',
            '기본 destroy는 dead 비트 표시만 대체했다. 실제 Unpop·그래프 분할·SID 반납·소리·낙하는 실행하지 않는다',
            '칸 위 이동체 탐색은 결과 없음으로 대체하고 호출 사실만 기록했다',
            '그래프 번호 254(레코드 NULL)와 접합 칸 고리(끝없는 재귀)는 기대값에서 제외했다',
            'CD/10.37의 한 칸 처리는 스캔 범위를 번호 하나로 좁혀 실제 스캔 함수로 실행했다',
            '스캔 행은 53비트 정밀도(CRT 기본값)의 연속 궤적이며, 64비트 정밀도에서 처리 개수가 달라지는 프레임은 제외하고 skipped_inputs에 셌다',
            '금 간/보통/약화/복구 전환의 별도 함수는 패치판에만 있어 패치판 기계어만 실행했다',
            '스캔의 try/catch 착지점은 실행하지 않았다',
        ],
    }
    EVIDENCE.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': dict(counts), 'skipped': dict(skipped), 'assertions': report['assert_reports']}, ensure_ascii=False))


def verify():
    """저장된 근거 기록과 실제 파일의 SHA-256·행 수가 같은지 확인한다(원본 기계어는 다시 실행하지 않는다)."""
    report = json.loads(EVIDENCE.read_text(encoding='utf-8'))
    problems = []
    # 원본 실행 파일.
    for edition, expected in report['binary_sha256'].items():
        if digest(ROOT / SPECS[edition]['binary']) != expected:
            problems.append(f'실행 파일 해시 불일치: {edition}')
    # 내보내기·도구.
    for group in ('export_sha256', 'source_sha256'):
        for path, expected in report[group].items():
            if not (ROOT / path).exists():
                problems.append(f'파일 없음: {path}')
            elif digest(ROOT / path) != expected:
                problems.append(f'해시 불일치: {path}')
    if digest(FIXTURE) != report['fixture_sha256']:
        problems.append('기대값 파일 해시 불일치')
    lines = FIXTURE.read_text(encoding='utf-8').splitlines()
    counts = collections.Counter(line.split('\t', 1)[0] for line in lines if line and not line.startswith('#'))
    # 행 종류별 개수.
    for name, expected in report['cases'].items():
        if counts[name] != expected:
            problems.append(f'행 수 불일치: {name} {counts[name]} != {expected}')
    if problems:
        raise SystemExit('\n'.join(problems))
    print(f'확인 완료: x86 입력 {report["x86_input_total"]}개, 판본 {len(report["binary_sha256"])}개')


def main():
    """명령줄 진입점."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true', help='저장된 기록만 확인한다')
    args = parser.parse_args()
    if args.verify:
        verify()
    else:
        generate()


# 직접 실행했을 때만 기대값을 만들거나 확인한다.
if __name__ == '__main__':
    main()
