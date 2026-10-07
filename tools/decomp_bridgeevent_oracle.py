#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 이벤트 처리기(vtable +0x5c)·끝 칸 변환·글자별 프레임 구간 표를 세 실제 PE의 제한 x86으로 대조한다.

실제 명령으로 실행하는 것: 이벤트 처리기 몸체(0x2691/0x2692 분기·권한/디버그 검사·_ftol 좌표 분해),
끝 칸 변환(방향 글자 판정·임시 프레임·수명 비트 갱신·교체 여부·필드 복사), 타입/프레임 조회,
단어 쓰기(SetWord), 글자별 프레임 구간 표 생성(0049b060 ↔ CD 00444da0).
명시적으로 대체하는 것(호출 사실과 인자만 기록): 이웃 탐색기 생성(첫 이웃 번호 입력), 표면 알림,
새 객체 생성(헤더만 기록), 가상 destroy·소유자 지정·Pop. 원본 게임/OS/업데이터는 실행하지 않는다.

python -X utf8 tools/decomp_bridgeevent_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_bridgeevent_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
"""
import argparse
import collections
import csv
import hashlib
import importlib.metadata
import itertools
import json
import math
import random
import struct
import sys
import zlib
from pathlib import Path

# 저장소 루트와 격리된 분석용 Python 의존성 경로.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 에뮬레이터 내부 주소. 호스트 메모리와 무관하며 서로 겹치지 않는다.
VTABLE = 0x11010000     # 합성 vtable(+0x10 destroy, +0x74 소유자, +0x90 Pop)
RESULT = 0x11011000     # 반환 x87 값을 저장하는 4바이트
TYPES = 0x11100000      # 타입 구조체 배열(번호 0부터)
CODES = 0x11200000      # 프레임 코드 배열(타입 번호마다 0x400바이트)
POOL = 0x12000000       # Squid 배열
STOP = 0x20000000       # 정상 반환을 확인하는 가상 주소(+0x100.. 대체 함수, +0x200 반환 스텁)
STACK_BASE, STACK = 0x30000000, 0x3003f000
# 풀 슬롯 수와 타입 수.
CAPACITY, MAX_TYPES = 128, 96
# 입력 번호: 다리, 새로 만들어지는 객체, 이웃 탐색기가 돌려주는 첫 이웃.
BRIDGE, NEWBORN, NEIGHBOR = 50, 60, 70
# 실제 게임의 다리 타입 번호(두 PE의 초기값 0x52)와 다른 타입 번호.
BRIDGE_TYPE, OTHER_TYPE = 82, 83
# x87 정밀도 제어 워드(53비트·64비트).
CONTROLS = (0x027f, 0x037f)
# 입력만 만드는 고정 시드. 기대 결과는 원본 기계어가 정한다.
SEED = 0x2692
# 한 호출에 허용하는 명령 수.
MAX_INSTRUCTIONS = 200000
# 반환 스텁: fstp dword [RESULT] 한 명령. 이 명령이 끝난 주소가 호출의 종료 주소다.
RETURN_STUB = STOP + 0x200
RETURN_END = RETURN_STUB + 6
# 대체 가상 함수 진입점.
DESTROY, SETOWNER, POP = STOP + 0x100, STOP + 0x110, STOP + 0x120

# 판본별 함수·대체 진입점·전역·슬롯 오프셋.
SPECS = {
 'originals': dict(binary='originals/Netstorm.exe', stride=50, type_stride=500, patch=True,
    entries=dict(Handler=0x422740, End=0x4215d0, Table=0x49b060),
    stubs={0x4b23e0: 'finder', 0x4214a0: 'notify', 0x4af530: 'create'}, assert_report=0x4e0620,
    globals=dict(types=0x59ab20, pool=0x5c8464, authority=0x540bc4, debug_keep=0x54db80,
        bridge_type=0x5411a0, crack_life=0x52f960),
    offsets=dict(owner=0x22, frame=0x24, frame_size=4, flag=0x28)),
 'originalCD': dict(binary='originalCD/NETSTORM.EXE', stride=36, type_stride=468, patch=False,
    entries=dict(Handler=0x449a20, Table=0x444da0),
    stubs={0x4ebad0: 'finder', 0x448c10: 'notify', 0x4ab390: 'create'}, assert_report=0x490040,
    globals=dict(types=0x51c960, pool=0x5395dc, authority=0x540a2c, debug_keep=0x52e928,
        bridge_type=0x51ca8c, crack_life=0x51f05c),
    offsets=dict(owner=0x20, frame=0x22, frame_size=1, flag=0x23)),
}
# 추가 10.37 실행 파일은 CD판과 같은 코드 배치다.
SPECS['original1037'] = dict(SPECS['originalCD'], binary='original1037/netstorm.exe')
# 합성 프레임 코드 한 칸: (방향 글자, 변형 글자, 번호, 플래그).
# 0:J 1:K 2:A 3:L 4:M 5:N 6:O 7:J 8:K 9:L 10:B 11:J(단단) 12~15는 표 검사용 다른 글자.
LETTERS = 'JKALMNOJKLBJPPCD'
# 판본별 프레임 변형: 교체 대상(J/K 프레임)에 0x40을 켜는 방식. 끝 프레임(L~O)의 플래그는 잡음으로 둔다.
HARD = {0: set(), 1: {0, 1, 7, 8, 11}, 2: {0, 8}}


def frame_codes(variant):
    """합성 프레임 코드 배열을 만든다. 변형마다 교체 금지(0x40) 프레임이 다르다."""
    codes = bytearray()
    for index, letter in enumerate(LETTERS):
        flags = 0x40 if index in HARD[variant] else (0x20 if letter in 'LM' else 0)
        codes += bytes([ord(letter), ord('P'), index + 1, flags])
    return bytes(codes)


def float_bits(value):
    """단정도 값의 실제 비트를 정수로 돌려준다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def digest(path):
    """파일의 SHA-256."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class EventOracle:
    """PE를 가상 메모리에 올리고 검토한 몸체·지정한 진입점 대체·허용 쓰기만 실행한다."""
    def __init__(self, edition):
        """판본의 코드 허용 범위, 전역, 메모리 배치를 준비한다. 호스트 메모리와 원본 파일은 수정하지 않는다."""
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
        self.stride = self.spec['stride']
        self.type_stride = self.spec['type_stride']
        self.o = self.spec['offsets']
        self.g = self.spec['globals']
        for address, size in ((VTABLE, 0x2000), (TYPES, 0x20000), (CODES, 0x40000),
                              (POOL, (CAPACITY * self.stride + 4095) & ~4095), (STOP, 0x1000), (STACK_BASE, 0x40000)):
            self.mu.mem_map(address, size)
        # 내보낸 함수의 불연속 몸체만 실행을 허용한다.
        self.exports = [ROOT / f'extracted/bridgeevent/final-{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed = []
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수마다 Ghidra가 계산한 몸체 범위를 그대로 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.allowed.append((RETURN_STUB, RETURN_END))
        self.stubs = dict(self.spec['stubs'])
        self.stubs.update({DESTROY: 'destroy', SETOWNER: 'owner', POP: 'pop'})
        # 반환 스텁과 합성 vtable.
        self.mu.mem_write(RETURN_STUB, b'\xd9\x1d' + struct.pack('<I', RESULT))
        self.mu.mem_write(VTABLE + 0x10, struct.pack('<I', DESTROY))
        self.mu.mem_write(VTABLE + 0x74, struct.pack('<I', SETOWNER))
        self.mu.mem_write(VTABLE + 0x90, struct.pack('<I', POP))
        for name, value in (('types', TYPES), ('pool', POOL)):
            self.mu.mem_write(self.g[name], struct.pack('<I', value))
        self.events = []
        self.stub_calls = collections.Counter()
        self.native_calls = collections.Counter()
        self.assertions = 0
        self.instructions = 0
        self.finder_first = 0
        self.write_ranges = []
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def slot(self, sid):
        """슬롯 주소."""
        return POOL + sid * self.stride

    def sid_of(self, pointer):
        """슬롯 주소를 번호로 바꾼다. 풀 밖 주소는 입력 오류다."""
        offset = pointer - POOL
        if offset < 0 or offset % self.stride or offset // self.stride >= CAPACITY:
            raise RuntimeError(f'풀 밖 객체 포인터: {pointer:08x}')
        return offset // self.stride

    def on_write(self, _mu, _access, address, size, _value, _data):
        """스택·결과 칸·이번 호출이 허용한 슬롯 필드 밖의 쓰기를 거부한다."""
        if STACK_BASE <= address and address + size <= STACK_BASE + 0x40000:
            return
        if any(low <= address and address + size <= high for low, high in self.write_ranges):
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
            if stub == 'finder':
                # flag 8 이웃 탐색기 생성자. 임시로 바뀐 프레임을 호출 시점에 읽어 기록하고 첫 이웃(+0x34)을 입력대로 준다.
                frame = int.from_bytes(mu.mem_read(self.slot(a0) + self.o['frame'], self.o['frame_size']), 'little')
                self.events.append(f'F:{a0}:{frame}:{a1}')
                mu.mem_write(ecx + 0x34, struct.pack('<I', self.finder_first))
                mu.reg_write(UC_X86_REG_EAX, ecx)
                purge = 8
            elif stub == 'notify':
                self.events.append(f'N:{a0}')
            elif stub == 'create':
                # 새 객체의 헤더(vtable·타입)만 만든다. 실제 할당/postCreate는 대체 경계다.
                self.events.append(f'C:{a0}:{a1}')
                slot = self.slot(NEWBORN)
                mu.mem_write(slot, struct.pack('<I', VTABLE))
                mu.mem_write(slot + 10, bytes([a0 & 0xff, 0]))
                mu.reg_write(UC_X86_REG_EAX, slot)
            elif stub == 'destroy':
                self.events.append(f'D:{self.sid_of(ecx)}:{a0}')
                purge = 4
            elif stub == 'owner':
                self.events.append(f'O:{self.sid_of(ecx)}:{a0}')
                purge = 4
            elif stub == 'pop':
                self.events.append(f'P:{self.sid_of(ecx)}:{a0}:{a1}:{a2}')
                purge = 12
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == self.spec['assert_report']:
            self.assertions += 1
            raise RuntimeError(f'원본 assert 도달: {self.edition}')
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소: {self.edition} {address:08x}')
        for name, entry in self.spec['entries'].items():
            if address == entry: self.native_calls[name] += 1

    def put_globals(self, authority, debug_keep, bridge_type):
        """권한·디버그 유지·다리 타입 전역을 쓴다."""
        for name, value in (('authority', authority), ('debug_keep', debug_keep), ('bridge_type', bridge_type)):
            self.mu.mem_write(self.g[name], struct.pack('<I', value))

    def scene(self, variant):
        """타입 표·프레임 코드·객체 슬롯을 기본 상태로 만든다. 글자별 표는 실제 0049b060/00444da0이 만든다."""
        self.mu.mem_write(TYPES, bytes(self.type_stride * MAX_TYPES))
        codes = frame_codes(variant)
        for number in (BRIDGE_TYPE, OTHER_TYPE):
            base = TYPES + number * self.type_stride
            address = CODES + (number - BRIDGE_TYPE) * 0x400
            self.mu.mem_write(address, codes)
            self.mu.mem_write(base + 0x114, struct.pack('<I', len(codes) // 4))
            self.mu.mem_write(base + 0x124, struct.pack('<I', address))
            # 판본별 실제 표 생성 함수로 글자별 첫 프레임/개수를 채운다.
            self.write_ranges = [(base + 0x12c, base + 0x1b0)]
            self.call('Table', [], base, 0, CONTROLS[0], returns_float=False)
        for sid in (BRIDGE, NEWBORN):
            self.mu.mem_write(self.slot(sid), bytes(self.stride))

    def call(self, name, args, this, purge, control, returns_float=True):
        """원본 호출 규약으로 실행하고 정상 반환·스택 복구·x87 상태를 확인한다. float 반환이면 그 비트를 돌려준다."""
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                         UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        # 반환 주소는 float 반환이면 반환 스텁, 아니면 정지 주소다.
        words = [RETURN_STUB if returns_float else STOP] + [value & 0xffffffff for value in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.mu.mem_write(RESULT, bytes(4))
        self.events = []
        until = RETURN_END if returns_float else STOP
        try:
            self.mu.emu_start(self.spec['entries'][name], until, count=MAX_INSTRUCTIONS)
        except UcError as error:
            raise RuntimeError(f'{name}: 에뮬레이션 오류 {error}') from error
        if self.mu.reg_read(UC_X86_REG_EIP) != until:
            raise RuntimeError(f'{name}: 제한 안에 반환하지 않음')
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{name}: 스택 복구 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError(f'{name}: x87 제어 워드/TOP 불일치')
        return struct.unpack('<I', self.mu.mem_read(RESULT, 4))[0] if returns_float else None

    def table(self, codes, control):
        """프레임 코드 배열만으로 실제 글자별 표 생성 함수를 실행하고 첫 프레임 16개·개수 16개·글자 수를 읽는다."""
        base = TYPES + BRIDGE_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride))
        address = CODES
        self.mu.mem_write(address, codes + bytes(8))
        self.mu.mem_write(base + 0x114, struct.pack('<I', len(codes) // 4))
        self.mu.mem_write(base + 0x124, struct.pack('<I', address))
        self.write_ranges = [(base + 0x12c, base + 0x1b0)]
        self.call('Table', [], base, 0, control, returns_float=False)
        first = struct.unpack('<16i', self.mu.mem_read(base + 0x130, 64))
        count = struct.unpack('<16i', self.mu.mem_read(base + 0x170, 64))
        distinct = struct.unpack('<I', self.mu.mem_read(base + 0x12c, 4))[0]
        return first, count, distinct

    def prepare(self, case):
        """호출 전 다리 슬롯과 전역·탐색 결과를 만든다. 판단 결과는 계산하지 않는다."""
        self.put_globals(case['authority'], case['debug'], case['bridge_global'])
        self.scene(case['variant'])
        slot = self.slot(BRIDGE)
        o = self.o
        self.mu.mem_write(slot, struct.pack('<I', VTABLE))
        self.mu.mem_write(slot + 10, bytes([BRIDGE_TYPE, 0]))
        self.mu.mem_write(slot + 8, struct.pack('<H', case['parent']))
        self.mu.mem_write(slot + 12, struct.pack('<H', case['word']))
        self.mu.mem_write(slot + 14, struct.pack('<II', case['x'], case['y']))
        self.mu.mem_write(slot + o['owner'], bytes([case['owner']]))
        self.mu.mem_write(slot + o['frame'], case['frame'].to_bytes(o['frame_size'], 'little'))
        self.mu.mem_write(slot + o['flag'], bytes([case['flag']]))
        self.finder_first = case['neighbor']
        # 처리기가 쓸 수 있는 필드만 허용한다: 다리의 수명 단어·프레임, 새 객체의 프레임·단어·+8·플래그.
        new = self.slot(NEWBORN)
        self.write_ranges = [
            (slot + 12, slot + 14), (slot + o['frame'], slot + o['frame'] + o['frame_size']),
            (new + 8, new + 10), (new + 12, new + 14), (new + o['frame'], new + o['frame'] + o['frame_size']),
            (new + o['flag'], new + o['flag'] + 1), (RESULT, RESULT + 4)]

    def run(self, case, control):
        """한 입력을 실행하고 (반환 float 비트, 사건, 다리 슬롯, 새 슬롯)을 돌려준다."""
        self.prepare(case)
        if case['entry'] == 'End':
            self.call('End', [case['arg_x'], case['arg_y']], self.slot(BRIDGE), 8, control, returns_float=False)
            result = None
        else:
            result = self.call('Handler', [case['event'], 0, case['payload']], self.slot(BRIDGE), 12, control)
        # 두 슬롯 전체를 Adler-32로 묶는다. 쓰기 허용 목록 밖의 변경은 on_write가 이미 막았다.
        return (result, ';'.join(self.events) or '-',
                zlib.adler32(bytes(self.mu.mem_read(self.slot(BRIDGE), self.stride))),
                zlib.adler32(bytes(self.mu.mem_read(self.slot(NEWBORN), self.stride))))


def result_text(bits):
    """float 비트를 TSV 칸으로 만든다. NaN은 x87 정규화로 비트가 달라지므로 한 이름으로 둔다."""
    if bits is None: return '-'
    value = struct.unpack('<f', struct.pack('<I', bits))[0]
    return 'nan' if math.isnan(value) else str(bits)


def base_case(**changes):
    """기본 입력. 위치는 (20.75, 21.9), 다리 J 프레임, 교체 가능한 변형 0이다."""
    case = dict(entry='Handler', authority=1, debug=0, bridge_global=BRIDGE_TYPE, variant=0, frame=0,
                x=float_bits(20.75), y=float_bits(21.9), word=0x1234, parent=0x2a2b, owner=3, flag=0x10,
                neighbor=NEIGHBOR, event=0x2692, payload=float_bits(5396.0), arg_x=0, arg_y=0)
    case.update(changes)
    return case


def case_columns(case):
    """TSV 입력 칸. 재생 쪽이 같은 순서로 읽는다."""
    return [case['entry'], case['authority'], case['debug'], case['bridge_global'], case['variant'], case['frame'],
            case['x'], case['y'], case['word'], case['parent'], case['owner'], case['flag'], case['neighbor'],
            case['event'], case['payload'], case['arg_x'], case['arg_y']]


def inputs():
    """원본 판단을 포함하지 않는 입력 격자와 시드 고정 표본을 만든다."""
    rng = random.Random(SEED)
    cases = []
    # payload로 만드는 좌표: (x & 255) | (y << 8). 객체 위치(20.75, 21.9) 둘레의 같음/작음/큼과 경계를 넣는다.
    def packed(x, y): return float((y << 8) | x)
    payloads = [0.0, 1.0, packed(20, 21), packed(21, 22), packed(19, 20), packed(255, 255), packed(0, 300), packed(20, 0),
                255.0, 256.0, 65535.0, -1.0, -256.0, 1e30, -1e30, 16777216.0, 8388609.0, math.nan, math.inf, -math.inf, 0.5, 5396.5]
    positions = [(20.75, 21.9), (20.0, 21.0), (19.0, 22.0), (-1.5, 300.5), (21.0, 21.0)]
    # A. 판단 격자: 현재 프레임 글자(J/K/A/L)·위치·payload. 교체 경로까지 가도록 변형 0, 이웃 있음.
    frames_by_letter = {'J': 0, 'K': 1, 'A': 2, 'L': 3}
    for letter, (px, py), payload in itertools.product('JKAL', positions, payloads):
        cases.append(base_case(frame=frames_by_letter[letter], x=float_bits(px), y=float_bits(py), payload=float_bits(payload)))
    # B. 효과 격자: 프레임 변형·현재 프레임(첫 J/K, 둘째 J/K)·이웃·다리 타입 전역·수명 단어·소유자·부모 단어·플래그.
    for variant, frame, neighbor, bridge_global, word, owner, parent, flag in itertools.product(
            range(3), (0, 1, 7, 8, 11), (0, NEIGHBOR), (BRIDGE_TYPE, OTHER_TYPE), (0x0000, 0x0078, 0x1234, 0xffff),
            (0, 3, 255), (0, 0x2a2b), (0x00, 0x10, 0xef)):
        # 표본으로 줄인다: 전체 곱의 일부만 쓰되 모든 값이 여러 번 나오게 시드 고정으로 고른다.
        if rng.random() > 0.12: continue
        cases.append(base_case(variant=variant, frame=frame, neighbor=neighbor, bridge_global=bridge_global, word=word,
                               owner=owner, parent=parent, flag=flag,
                               payload=float_bits(rng.choice([packed(20, 21), packed(21, 22), packed(19, 20), packed(40, 5)]))))
    # C. 가드 격자: 권한·디버그 유지·이벤트 번호·payload.
    for authority, debug, event, payload in itertools.product((0, 1), (0, 1), (0x2691, 0x2692, 0x2690, 0x2693, 0, 1, 0xffffffff, 0x10002692),
                                                              (0.0, packed(20, 21), -2.5, math.nan)):
        for frame in (0, 1):
            cases.append(base_case(authority=authority, debug=debug, event=event, frame=frame, payload=float_bits(payload)))
    # D. 끝 칸 변환 직접 호출(패치판만): 좌표 float 비교의 같음/작음/큼/NaN/무한대 경계.
    bounds = [-1.0, 19.9, 20.0, 20.75, 21.0, 21.9, 22.0, 1e30, math.nan, math.inf, -math.inf]
    for frame, (ax, ay), (px, py) in itertools.product((0, 1, 2), ((20.0, 21.0), (20.75, 21.9)), itertools.product(bounds, bounds)):
        cases.append(base_case(entry='End', frame=frame, x=float_bits(ax), y=float_bits(ay), arg_x=float_bits(px), arg_y=float_bits(py)))
    return cases


def frames_rows():
    """프레임 변형별 코드 배열 행(재생 쪽이 같은 배열을 만든다)."""
    return [['Frames', variant, frame_codes(variant).hex()] for variant in range(3)]


def table_inputs():
    """글자별 구간 표 입력: 모든 글자가 있는 배열, 빈 배열, 중복/흩어진 글자, 범위 밖 글자, 무작위 배열."""
    rng = random.Random(SEED ^ 0x49b060)
    def code(letter, number=0, flags=0):
        """프레임 코드 한 칸(방향 글자, 변형 'P', 번호, 플래그)의 바이트."""
        return bytes([ord(letter), ord('P'), number, flags])
    arrays = [b'', frame_codes(0), code('A', 1), code('P') * 3]
    # 알파벳 전체를 한 번씩, 그리고 각 글자를 두 번씩 넣은 배열.
    arrays.append(b''.join(code(letter, number) for number, letter in enumerate('ABCDEFGHIJKLMNOPQRSTUVWXYZ', 1)))
    arrays.append(b''.join(code(letter, number) for number, letter in enumerate('ABCDEFGHIJKLMNOP' * 2, 1)))
    letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
    # 시드 고정 무작위 배열: 글자는 대소문자 알파벳, 나머지 바이트는 임의 값이다.
    for _ in range(300):
        count = rng.randrange(0, 41)
        arrays.append(b''.join(bytes([ord(rng.choice(letters)), rng.randrange(256), rng.randrange(256), rng.randrange(256)])
                               for _ in range(count)))
    return arrays


def generate():
    """세 실제 PE와 두 x87 정밀도에서 새 fixture·SHA 근거를 만든다."""
    rows = []
    counts = collections.Counter()
    reports = {}
    cases = inputs()
    tables = table_inputs()
    table_rows = None
    for edition in SPECS:
        oracle = EventOracle(edition)
        for case in cases:
            # 끝 칸 변환 단독 진입점은 패치판에만 있다. CD/10.37은 처리기 안에 인라인되어 있다.
            if case['entry'] == 'End' and not oracle.spec['patch']: continue
            # 두 x87 정밀도의 관찰이 다르면 입력 계약이 잘못된 것이므로 중단한다.
            first, second = (oracle.run(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'x87 정밀도에 따라 결과가 다름: {edition} {case}')
            bits, events, bridge, born = first
            rows.append([case['entry'], edition, *case_columns(case)[1:], result_text(bits), events, bridge, born])
            counts[case['entry']] += 1
        # 글자별 표 생성은 세 PE와 두 정밀도에서 모두 같아야 한다. 같은 입력의 행은 판본 공통으로 한 번만 저장한다.
        this_table = []
        for index, codes in enumerate(tables):
            observed = {oracle.table(codes, control) for control in CONTROLS}
            if len(observed) != 1: raise RuntimeError(f'글자 표가 정밀도에 따라 다름: {edition} {index}')
            first, count, distinct = observed.pop()
            this_table.append(['Table', index, codes.hex() or '-', ','.join(map(str, first)), ','.join(map(str, count)), distinct])
        if table_rows is None:
            table_rows = this_table
            counts['Table'] += len(this_table)
        elif table_rows != this_table: raise RuntimeError(f'글자 표가 판본마다 다름: {edition}')
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
            entries={k: f'{v:08x}' for k, v in oracle.spec['entries'].items()},
            replaced_entry_points={f'{k:08x}': v for k, v in oracle.stubs.items()},
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls),
            assertions=oracle.assertions, instructions=oracle.instructions)
    header = ['# 실제 PE 제한 x86 다리 이벤트 처리기/끝 칸 변환/글자별 프레임 구간 표 기대값; 이웃 탐색기·표면 알림·생성·destroy·소유자·Pop은 계약 대체',
              '# 두 x87 정밀도의 관찰이 같은 입력만 저장한다. 슬롯 열은 Adler-32다.',
              '# Frames 변형 코드 | Handler/End 판본 권한 디버그 전역다리타입 변형 프레임 x y 단어 부모 소유자 플래그 이웃 이벤트 payload 인자x 인자y 반환 사건 다리슬롯 새슬롯 | Table 번호 코드 첫프레임 개수 글자수']
    rows = frames_rows() + rows + table_rows
    FIXTURE.write_text('\n'.join(header) + '\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = [Path(__file__), FIXTURE, ROOT / 'tools/ghidra/bridgeevent-functions.json']
    for edition in SPECS:
        paths.extend([ROOT / reports[edition]['binary'], ROOT / f'extracted/bridgeevent/final-{edition}/creation.c',
                      ROOT / f'extracted/bridgeevent/final-{edition}/functions.tsv'])
    report = dict(schema=1, primary_target='10.78', method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
        unicorn_version=importlib.metadata.version('unicorn'), seed=SEED, x87_control_words=[hex(v) for v in CONTROLS],
        cases=dict(counts), total=sum(counts.values()), editions=reports, os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in paths},
        limitations=['prepared first-neighbor result of the flag-8 finder', 'external surface notify/create/destroy/owner/pop calls',
                     'synthetic type frame codes and bridge type number', 'no GameWorld, Kernel scheduling or real Pop',
                     'CD/10.37 end-cell conversion is inline in the handler (no direct entry): reached only with integral payload coordinates'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=dict(counts), total=sum(counts.values())), ensure_ascii=False))


def verify():
    """저장된 입력/도구/fixture/몸체 SHA와 행 수를 확인한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts = collections.Counter(line.split('\t', 1)[0] for line in rows)
    counts.pop('Frames', None)
    if dict(counts) != report['cases'] or sum(counts.values()) != report['total']: raise RuntimeError('fixture 행 수 불일치')
    if any(item['assertions'] for item in report['editions'].values()): raise RuntimeError('assert 도달')
    print(f'bridgeevent 검증 통과: {sum(counts.values())}개')


# 새 결과 파일은 이전 기록/fixture와 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bridgeevent-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-bridgeevent-evidence.json'


def main():
    """새 기대값 생성과 저장 결과 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true', help='파일을 쓰지 않고 판본마다 첫 몇 행만 실행해 도구를 점검한다')
    args = parser.parse_args()
    if args.smoke:
        smoke()
    elif args.verify:
        verify()
    else:
        generate()


def smoke():
    """파일을 쓰지 않고 판본마다 대표 입력 몇 개와 표 입력 몇 개를 실행해 출력한다."""
    cases = inputs()
    for edition in SPECS:
        oracle = EventOracle(edition)
        picks = [c for c in cases if c['entry'] == 'Handler'][:3] + [c for c in cases if c['entry'] == 'Handler'][600:603]
        for case in picks:
            bits, events, bridge, born = oracle.run(case, CONTROLS[0])
            print(edition, result_text(bits), events)
        print(edition, 'table', oracle.table(frame_codes(0), CONTROLS[0]))


if __name__ == '__main__':
    main()
