#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""삭제/해체 보상(SP) 경로를 세 실제 PE의 제한 x86으로 실행한다. 게임/창/OS 실행은 없다.

패치 0044c2f0 ↔ CD/10.37 004626d0과 비용 곡선·환불·SP 저장소·AI 지갑·샘 풀을 실제 기계어로 실행한다.
UI 갱신/시각 재시드/힙 할당/assert 보고와 파생 vtable +0x80의 개수만 명시적으로 대체한다.

    python -X utf8 tools/decomp_reward_oracle.py            # fixture 생성
    python -X utf8 tools/decomp_reward_oracle.py --verify   # 기록 SHA/호출 경계 감사
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
# x87 가수부 53/64비트 제어 워드. 두 값의 결과가 다르면 입력을 제외하고 기록한다.
CONTROLS = (0x027f, 0x037f)
# 재현 가능한 합성 입력 시드와 판본당 목표 입력 수.
INPUT_SEED = 0x10780205
CASES_PER_BINARY = 2400
# 격리 메모리 배치. 모든 주소는 에뮬레이터 내부 주소다.
ARENA, ARENA_SIZE = 0x12000000, 0x40000
POOL, TYPES, PLAYERS, AIOBJ, MODE, VTABLE, SCRATCH, MALLOC = (ARENA + offset for offset in
    (0x0, 0x1000, 0x18000, 0x19000, 0x1c000, 0x1d000, 0x1e000, 0x20000))
STUBS, STOP, STACK = 0x13000000, 0x20000000, 0x3000f000
COUNT_STUB = STUBS + 0x100
# 합성 객체의 SID와 AI 지갑 필드 위치(두 판본 공통).
SID, WALLET = 6, 0x2d8
# 판본별 실제 주소. 전역 이름은 이번 분석에서 확인한 의미를 따른다.
SPECS = {
    'originals': dict(patch=True, reward=0x44c2f0, ftol=0x4e49c0, pool_ptr=0x5c8464, stride=50,
        type_ptr=0x59ab20, type_stride=500, count=188, extra=0x28, hp_width=4,
        players_stride=0xb4, ai_off=0xb0, sp_read=0x40ec50, sp_write=0x40edb0, base_count=0x4ad0f0,
        prefix=(0x49c338, 0x49c359), cost_table=0x59ab50,
        g=dict(none=0x5412e8, dais=0x5412ec, silent=0x5412a4, alias=0x5412b0, full=0x5412c0, priest=0x5412d0,
               percent=0x5424bc, local=0x540c70, geyser=0x595088, fixed_a=0x594fb8, fixed_b=0x5c85a4,
               half=0x594fbc, mirror_off=0x594fc8, players=0x59534c, mode_obj=0x5634a0, quarter=0x5ca8f0,
               debug=0x5e4794, rng=0x532710, inited=0x54db24, offset=0x568af8, init_copy=0x54d4e0,
               perm=0x531890, pools=0x594fd0),
        stubs=dict(ui=0x43dad0, reseed=0x4559d0, malloc=0x4e4391, assert_=0x4e0620),
        entries=['0044c2f0', '0044b220', '0043cb30', '0044c240', '004adc60', '004adcb0', '0049a840', '0049fce0',
                 '004ad0f0', '004ad0a0', '0040eec0', '0041df80', '0040ec50', '0040edb0', '0040eba0', '004558c0',
                 '00413bb0', '00463b50', '00485f70', '004e49c0']),
    'originalCD': dict(patch=False, reward=0x4626d0, ftol=0x4f161c, pool_ptr=0x5395dc, stride=36,
        type_ptr=0x51c960, type_stride=468, count=171, extra=0x23, hp_width=2,
        players_stride=0xac, ai_off=0xa8, base_count=0x4ace00,
        g=dict(none=0x51cbd4, dais=0x51cbd8, silent=0x51cb90, alias=0x51cb9c, full=0x51cbac, priest=0x51cbbc,
               percent=0x5395d0, local=0x50f6c8, geyser=0x512110, sp=0x5120d4, fixed_a=0x540a1c, fixed_b=0x518904,
               half=0x540a20, mirror_off=0x540a24, players=0x50f834, mode_obj=0x549a08, quarter=0x511c90),
        stubs=dict(ui=0x4ee9d0),
        entries=['004626d0', '004147c0', '00414830', '00462610', '004aecf0', '004aed30', '00444720', '004abae0',
                 '004abac0', '004acdc0', '004ace00', '00414680', '004d8cc0', '004d81f0', '00415d00', '004175a0',
                 '00417060', '004f161c', '004acd90', '004448e0', '004abf50']),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 입력 수가 많은 표 값들. 정수에 가까운 값과 소수/음수/float 반올림 경계를 함께 둔다.
COSTS = [0.0, 1.0, 0.5, 12.5, 100.0, 250.0, 1000.75, 123456.0, -3.5, -100.0, 3.9, 16777216.0, 99999999.0]
CD_ONLY_COSTS = [3e9, -3e9, 123456789.5]
MAXHPS = [0, 1, 3, 10, 40, 100, 255]
HPS = [0, 1, 5, 10, 37, 50, 99, 100, 101, 255, -1, -7, 40000, -40000]
PERCENTS = [25, 25, 25, 0, 1, 50, 100, 150, 200, 1000000, 2147483647, 4294967295]
GEYSERS = [0, 5, -7, 2147483000, -2147483000]
SPS = [0.0, 100.0, 1500.5, 194162.0, -50.0, 3000.0, 1e9, 0.1, 99999.9]
WALLETS = [0, 100, 1190, 1200, -50, 2147483000, -2147483000]
TYPE_CHOICES = [82, 70, 95, 100, 120, 147, 150, 154, 158, 164, 165, 82, 100, 150, 158, 165]


def digest(path):
    """원본/도구/내보내기/fixture의 정확한 바이트 SHA를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """float을 IEEE 32비트 패턴으로 바꾼다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


class RewardOracle:
    """PE를 가상 메모리로 읽고 검토된 보상 함수 몸체만 허용해 실행한다."""
    def __init__(self, binary):
        """판본별 허용 범위·쓰기 범위·명시적 대체 경계를 준비한다. 호스트 메모리와 원본 파일은 수정하지 않는다."""
        self.binary, self.s = binary, SPECS[binary]
        self.patch = self.s['patch']
        path = ROOT / BINARIES[binary]
        self.sha256 = digest(path)
        pe = pefile.PE(str(path))
        self.base = pe.OPTIONAL_HEADER.ImageBase
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(self.base, (len(image) + 0xfff) & ~0xfff)
        self.mu.mem_write(self.base, image)
        for address, size in ((ARENA, ARENA_SIZE), (STUBS, 0x1000), (STOP, 0x1000), (STACK - 0xf000, 0x10000)):
            self.mu.mem_map(address, size)
        # 대체 진입점은 실제 코드가 아니라 호출 규약만 지키는 ret 한 바이트다.
        self.mu.mem_write(COUNT_STUB, b'\xc3')
        self.facts = ROOT / f'extracted/reward/{binary}/functions.tsv'
        self.exports = [ROOT / f'extracted/reward/{binary}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.hits = [], collections.Counter()
        self.entry_names = {}
        with self.facts.open(encoding='utf-8') as fp:
            rows = {row['entry']: row for row in csv.DictReader(fp, delimiter='\t')}
        # 허용 목록의 함수만 실행한다. 파생 vtable/finder/UI 조회 등 내보냈더라도 목록 밖은 즉시 실패한다.
        for entry in self.s['entries']:
            for part in rows[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
            self.entry_names[int(entry, 16)] = entry
        if self.patch:
            # 정수 비용 표를 채우는 실제 로더 접두 구간 하나만 추가로 허용한다.
            self.allowed.append((self.s['prefix'][0], self.s['prefix'][1]))
        self.writes = [(ARENA, ARENA + ARENA_SIZE), (STACK - 0xf000, STACK + 0x1000)]
        g = self.s['g']
        for name in ('geyser', 'sp', 'rng', 'inited', 'offset', 'init_copy'):
            if name in g: self.writes.append((g[name], g[name] + 4))
        if self.patch:
            self.writes += [(g['perm'], g['perm'] + 64), (g['pools'], g['pools'] + 0x80),
                            (self.s['cost_table'], self.s['cost_table'] + 4 * self.s['count'])]
        self.stub_hits, self.asserts, self.script, self.malloc_next = collections.Counter(), 0, [], 0
        self.stub_addresses = set(self.s['stubs'].values())
        self.fill = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def u32(self, address):
        """가상 메모리의 부호 없는 DWORD를 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def put(self, address, value, width=4):
        """가상 메모리에 little endian 정수를 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'little'))

    def on_write(self, mu, access, address, size, value, user_data):
        """허용 목록 밖 쓰기를 즉시 실패시킨다."""
        if not any(low <= address and address + size <= high for low, high in self.writes):
            raise RuntimeError(f'허용하지 않은 쓰기: {self.binary} {address:08x}/{size}')

    def on_instruction(self, mu, address, size, data):
        """명시적 대체를 먼저 처리하고, 나머지는 허용한 실제 몸체인지 확인한다."""
        s = self.s
        stubs = s['stubs']
        if address == COUNT_STUB or address in self.stub_addresses:
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret = struct.unpack('<I', mu.mem_read(esp, 4))[0]
            result = None
            if address == COUNT_STUB:
                # 파생 vtable +0x80 대체: 입력 목록의 다음 개수를 반환한다.
                if not self.script: raise RuntimeError('가상 개수 대체 호출이 입력보다 많음')
                self.stub_hits['count'] += 1
                result = self.script.pop(0) & 0xffffffff
            elif address == stubs.get('ui'):
                # SP 표시 갱신은 화면 효과이므로 호출만 센다.
                self.stub_hits['ui'] += 1
            elif address == stubs.get('reseed'):
                # 시각 기반 재시드는 입력 상태로 대체한다. 호출 뒤 전역 난수 상태는 입력 시드다.
                self.stub_hits['reseed'] += 1
                self.put(s['g']['rng'], self.seed)
            elif address == stubs.get('malloc'):
                # 0x24바이트 힙 블록. 내용은 초기화되지 않은 힙을 나타내는 입력 채움값이다.
                if struct.unpack('<I', mu.mem_read(esp + 4, 4))[0] != 0x24: raise RuntimeError('SP 저장소 할당 크기 오류')
                result = MALLOC + self.malloc_next * 0x40; self.malloc_next += 1
                mu.mem_write(result, struct.pack('<I', self.fill) * 9)
                self.stub_hits['malloc'] += 1
            else:
                self.asserts += 1
            if result is not None: mu.reg_write(UC_X86_REG_EAX, result)
            mu.reg_write(UC_X86_REG_ESP, esp + 4); mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]] += 1

    def call(self, entry, args, control, this=0):
        """원본 cdecl 규약으로 실행하고 정상 반환·스택·x87 스택을 확인한다."""
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                         UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        words = [STOP] + [value & 0xffffffff for value in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.mu.emu_start(entry, STOP, timeout=30000000, count=2000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError(f'호출 반환/스택 오류: {self.binary} {entry:08x}')
        if self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or self.mu.reg_read(UC_X86_REG_FPCW) != control:
            raise RuntimeError(f'x87 스택/제어 워드 오류: {self.binary} {entry:08x}')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def encode_cost(self, number, control):
        """패치 로더의 ftol(cost + 번호*23) 저장 접두 구간을 실제로 실행한다."""
        s = self.s
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EBX, 0)
        self.mu.reg_write(UC_X86_REG_ESI, number * s['type_stride'])
        self.mu.reg_write(UC_X86_REG_EDI, number)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2); self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control); self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.put(STACK + 0x10, number * 23)
        self.mu.emu_start(s['prefix'][0], s['prefix'][1], timeout=30000000, count=2000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != s['prefix'][1] or self.mu.reg_read(UC_X86_REG_ESP) != STACK:
            raise RuntimeError('비용 인코딩 접두 구간 종료 오류')
        if self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800: raise RuntimeError('비용 인코딩 x87 스택 오류')
        self.hits['cost_encode'] += 1

    def pools_adler(self):
        """확보한 32개 SP 배열의 전체 바이트를 한 체크섬으로 묶는다."""
        data = bytearray()
        for index in range(32):
            pointer = self.u32(self.s['g']['pools'] + index * 4)
            data += self.mu.mem_read(pointer, 0x24) if pointer else bytes(0x24)
        return zlib.adler32(bytes(data))

    def read_sp(self, control):
        """패치는 실제 읽기 함수로, CD는 전역 float 비트로 SP를 읽는다."""
        g = self.s['g']
        if not self.patch: return [self.u32(g['sp'])]
        values = []
        for owner in range(9):
            self.call(self.s['sp_read'], [owner, SCRATCH], control)
            values.append(self.u32(SCRATCH))
        return values

    def run(self, case, control):
        """입력 상태를 처음부터 만들고 실제 보상 함수를 한 번 실행한다."""
        s, g = self.s, self.s['g']
        self.mu.mem_write(ARENA, bytes(ARENA_SIZE))
        self.seed, self.fill = case['seed'], case['fill']
        self.script, self.malloc_next = list(case['script']), 0
        self.stub_hits.clear(); self.asserts = 0
        # 타입 표: 번호 순서대로 겹치는 입력은 호출자가 같은 값을 준다.
        self.put(s['type_ptr'], TYPES)
        records = ((case['type'],) + case['t']), ((147,) + case['silent']), ((164,) + case['altar'])
        for number, cost, max_hp, flags1, flags2 in records:
            address = TYPES + number * s['type_stride']
            self.put(address, max_hp); self.put(address + 0x9c, 10)
            self.put(address + 0xc4, cost); self.put(address + 0xe8, flags1); self.put(address + 0xec, flags2)
        if self.patch:
            self.mu.mem_write(s['cost_table'], bytes(4 * s['count']))
            for number, *_ in records: self.encode_cost(number, control)
        # 객체 슬롯과 판본별 상태/HP 필드.
        self.put(s['pool_ptr'], POOL)
        slot = POOL + SID * s['stride']
        self.put(VTABLE + 0x80, s['base_count'] if case['script_mode'] == 'base' else COUNT_STUB)
        self.put(slot, VTABLE); self.put(slot + 10, case['type'], 1); self.put(slot + 11, case['state'], 1)
        self.put(slot + s['extra'], case['extra'], 1); self.put(slot + 0x1a, case['hp'], s['hp_width'])
        # 플레이어 표와 AI 지갑. AI가 없는 소유자의 포인터는 0이다.
        self.put(g['players'], PLAYERS)
        for owner in range(9):
            attached = (case['ai_mask'] >> owner) & 1
            self.put(PLAYERS + owner * s['players_stride'] + s['ai_off'], AIOBJ + owner * 0x400 if attached else 0)
            self.put(AIOBJ + owner * 0x400 + WALLET, case['wallets'][owner])
        # 지갑 하한 객체는 없음/값 0/값 있음의 세 상태를 만든다.
        flags = case['flags']
        if flags & 0x10 or flags & 0x80:
            self.put(g['mode_obj'], MODE); self.put(MODE + 0x68, 1 if flags & 0x10 else 0)
        else:
            self.put(g['mode_obj'], 0)
        for name, bit in (('fixed_a', 0), ('fixed_b', 1), ('half', 2), ('mirror_off', 3), ('quarter', 5)):
            self.put(g[name], (flags >> bit) & 1)
        if self.patch: self.put(g['debug'], (flags >> 6) & 1)
        self.put(g['percent'], case['percent']); self.put(g['local'], case['local']); self.put(g['geyser'], case['geyser'])
        # SP 초기화: 패치는 실제 저장 함수로 소유자 0~8을 차례로 쓴다. 첫 호출이 실제 초기화를 실행한다.
        if self.patch:
            for name in ('inited', 'rng', 'offset', 'init_copy'): self.put(g[name], 0)
            self.mu.mem_write(g['perm'], bytes(64)); self.mu.mem_write(g['pools'], bytes(0x80))
            for owner in range(9): self.call(s['sp_write'], [owner, case['sp'][owner]], control)
            init = (self.u32(g['rng']), self.pools_adler())
        else:
            self.put(g['sp'], case['sp'][0]); init = (0, 0)
        self.stub_hits['ui'] = 0
        base_key = f"{s['base_count']:08x}"
        base_before = self.hits[base_key]
        self.call(s['reward'], [SID, case['recipient'], case['salvage']], control)
        result = dict(geyser=self.u32(g['geyser']), wallets=[self.u32(AIOBJ + o * 0x400 + WALLET) for o in range(9)],
                      sp=self.read_sp(control), init=init, ui=self.stub_hits['ui'],
                      vcalls=self.stub_hits['count'] if case['script_mode'] != 'base' else self.hits[base_key] - base_before,
                      leftover=len(self.script), asserts=self.asserts)
        result.update(rng=self.u32(g['rng']) if self.patch else 0, offset=self.u32(g['offset']) if self.patch else 0,
                      pools=self.pools_adler() if self.patch else 0)
        return result


def make_cases(binary, rng):
    """경계 목록과 시드 고정 조합으로 합성 입력을 만든다. 기대값은 계산하지 않고 기계어 결과를 쓴다."""
    s = SPECS[binary]
    patch = s['patch']
    costs = COSTS if patch else COSTS + CD_ONLY_COSTS
    cases = []
    for index in range(CASES_PER_BINARY):
        number = rng.choice(TYPE_CHOICES) if index % 5 else [82, 147, 150, 154, 158, 164, 165][index // 5 % 7]
        record = lambda: (bits(rng.choice(costs)), rng.choice(MAXHPS), rng.choice((0x10, 0x10, 0, 0x810)),
                          rng.choice((0, 0, 0x400000, 0x4000)))
        t = record()
        silent = t if number == 147 else record()
        # 패치 디버그 assert를 피하려면 altar HP 타입에 HP 비트가 있어야 하므로 flags1을 0x10으로 고정한다.
        altar = t if number == 164 else (bits(rng.choice(costs)), rng.choice(MAXHPS[1:]), 0x10, 0)
        state = rng.randrange(8) << 5 | rng.randrange(32)
        recipient, local = rng.randrange(9), rng.randrange(9)
        if rng.random() < 0.5: local = recipient
        mode = rng.random()
        script_mode = 'base' if mode < 0.55 else 'stub'
        script = [] if script_mode == 'base' else [rng.choice([-2, -1, 0, 1, 2, 3, 4, 5, 7, 9, 100, 1000]) for _ in range(3)]
        flags = rng.randrange(1 << 8)
        cases.append(dict(
            type=number, t=t, silent=silent, altar=altar, state=state, extra=rng.choice((0, 1, 8, 9, 0x80, 0xff)),
            hp=rng.choice(HPS), recipient=recipient, local=local, salvage=int(rng.random() < 0.3),
            percent=rng.choice(PERCENTS), geyser=rng.choice(GEYSERS), flags=flags,
            ai_mask=rng.randrange(512), wallets=[rng.choice(WALLETS) for _ in range(9)],
            sp=[bits(rng.choice(SPS)) for _ in range(9)], seed=rng.choice((0, 1, rng.randrange(1 << 32), rng.randrange(1 << 32))),
            fill=rng.choice((0, 0xcdcdcdcd, rng.randrange(1 << 32))), script_mode=script_mode, script=script))
    return cases


def row_values(case, result, binary):
    """C++ 재생기가 같은 순서로 읽는 TSV 열이다."""
    def record(values): return ','.join(str(v) for v in values)
    sp = result['sp']
    return [binary, str(case['type']), record(case['t']), record(case['silent']), record(case['altar']),
            str(case['state']), str(case['extra']), str(case['hp']), str(case['recipient']), str(case['local']),
            str(case['salvage']), str(case['percent']), str(case['geyser']), str(case['flags']), str(case['ai_mask']),
            record(case['wallets']), record(case['sp'] if SPECS[binary]['patch'] else case['sp'][:1]),
            str(case['seed']), str(case['fill']), 'base' if case['script_mode'] == 'base' else record(case['script']),
            str(result['geyser']), record(result['wallets']), record(sp), str(result['init'][0]), str(result['init'][1]),
            str(result['rng']), str(result['pools']), str(result['offset']), str(result['ui']), str(result['vcalls'])]


def generate(args):
    """세 PE에서 두 x87 정밀도로 실행하고 결과가 같은 입력만 fixture에 저장한다."""
    rows = ['# 원본 x86 기계어에서 생성. 두 x87 정밀도 결과가 같은 입력만 저장. UI/재시드/힙/가상 개수/assert만 대체.']
    report = dict(schema=1, method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
                  unicorn_version=importlib.metadata.version('unicorn'), seed=INPUT_SEED, editions={},
                  tool_sha256=digest(Path(__file__)), fpu_controls=list(CONTROLS))
    total = 0
    for binary in ('originals', 'originalCD', 'original1037'):
        oracle = RewardOracle(binary)
        rng = random.Random(INPUT_SEED)
        accepted = sensitive = 0
        counts = collections.Counter()
        for number, case in enumerate(make_cases(binary, rng)):
            first = oracle.run(case, CONTROLS[0]); second = oracle.run(case, CONTROLS[1])
            if first != second:
                sensitive += 1
                continue
            # 가상 개수 목록은 최대 세 번 쓰이므로 남는 값은 정상이다. assert 보고만 계약 위반이다.
            if first['asserts']:
                raise RuntimeError(f'입력/호출 계약 위반: {binary} {number}')
            rows.append('\t'.join(row_values(case, first, binary)))
            accepted += 1
            counts['applied'] += int(first['ui'] > 0)
        # 실제 호출 수와 대체 호출 수를 모두 기록한다. 한 입력은 두 정밀도로 두 번 실행한다.
        report['editions'][binary] = dict(binary_sha256=oracle.sha256, accepted=accepted, precision_sensitive=sensitive,
            reward_applied=counts['applied'], native_hits=dict(oracle.hits), exports={p.name: digest(p) for p in oracle.exports})
        total += accepted
    payload = '\n'.join(rows) + '\n'
    report['total_cases'] = total
    report['fixture_sha256'] = hashlib.sha256(payload.encode('utf-8')).hexdigest()
    report['stubbed'] = ['SP 표시 갱신', '시각 기반 난수 재시드(패치)', '0x24바이트 힙 할당(패치)', '패치 assert 보고', '파생 vtable +0x80 개수']
    report['limitations'] = ['합성 타입/객체·소유자 0~8·정수 비용 표 범위·재귀 없는 개수 입력', '게임/창/OS 실행 없음',
                             '파생 vtable +0x80(dais 전달/프레임 기반 개수)은 호출자 계약', 'SP 표시와 AI 객체 안쪽은 지갑 필드만 대조']
    fixture = ROOT / 'cpppj/tests/fixtures/reward-x86.tsv'
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text(payload, encoding='utf-8', newline='\n')
    (ROOT / 'cpppj/recovery-reward-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n',
                                                              encoding='utf-8', newline='\n')
    print(f'삭제 보상 x86 fixture {total}개 → {fixture}')


def verify():
    """기계어를 다시 실행하지 않고 원본/도구/내보내기/fixture SHA와 행 수를 감사한다."""
    report = json.loads((ROOT / 'cpppj/recovery-reward-evidence.json').read_text(encoding='utf-8'))
    assert report['tool_sha256'] == digest(Path(__file__)), '도구 SHA 불일치'
    for binary, data in report['editions'].items():
        assert data['binary_sha256'] == digest(ROOT / BINARIES[binary]), f'원본 SHA 불일치: {binary}'
        for name, sha in data['exports'].items():
            assert sha == digest(ROOT / f'extracted/reward/{binary}/{name}'), f'내보내기 SHA 불일치: {binary}/{name}'
    path = ROOT / 'cpppj/tests/fixtures/reward-x86.tsv'
    assert hashlib.sha256(path.read_bytes()).hexdigest() == report['fixture_sha256'], 'fixture SHA 불일치'
    rows = [line for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows) == report['total_cases'] == sum(d['accepted'] for d in report['editions'].values())
    print(f'삭제 보상 SHA/행 수/원본 경계 감사: {len(rows)}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    verify() if options.verify else generate(options)
