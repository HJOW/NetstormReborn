#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 버튼/좌표와 생성·캡처·idle·부모 전달·팝업 삭제 몸체를 격리 x86으로 관찰한다."""
import argparse
import collections
import copy
import csv
import itertools
import json
import random
import socket
import struct
from decomp_process_oracle import (ROOT, BINARIES, CONTROLS, STACK, STOP, STUBS, digest,
    pefile, Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)
from decomp_drawsort_oracle import REGISTERS
from decomp_gumplifetime_oracle import SPECS as LIFE
from decomp_gumpcreate_oracle import SPECS as CREATE

# 의미별 호스트와 무관한 전체 노드·보호 바이트·가상 표·투영 풀·결과 영역이다.
DATA, SIZE, STRIDE, COUNT = 0x19000000, 0x170, 0x190, 6
TOKENS = tuple(DATA + 16 + i * STRIDE for i in range(COUNT))
VTABLE, POOL, RESULT = DATA + 0x8000, DATA + 0xa000, DATA + 0xc000
# 결과/입력 시드와 명시 외부 가상 경계다. 기대값은 Python/C++에서 계산하지 않는다.
SEED = 0x4249a078
FIXTURE = ROOT / 'cpppj/tests/fixtures/buttongump-x86.tsv'
ALTERNATE = ROOT / 'cpppj/tests/fixtures/buttongump-x87-64.json'
REPORT = ROOT / 'cpppj/buttongump.json'
VIRTUAL = {STUBS + 0xa00: ('Redraw', 1), STUBS + 0xa20: ('Parent', 3), STUBS + 0xa40: ('Push', 0)}
# 실제 함수/전역. 호출 규약과 인자는 각 실제 entry의 ret/call 명령에 맞춘다.
SPECS = {
    'originals': dict(Construct=0x4253e0, Value=0x424960, Pressed=0x424980, StoneValue=0x425150,
        StonePressed=0x425200, Enabled=0x425470, Blink=0x4254a0, Pulse=0x424c90, Handle=0x4249a0,
        Broadcast=0x424dd0, Idle=0x424cd0, Local=0x4650a0, Contains=0x4248e0,
        clock=0x460d70, poll=0x452e00, base=0x464990, sound=0x4a9cb0, project=0x497220,
        value_event=0x5428b8, click_event=0x5427a8, right_event=0x542740, repeat_event=0x5427b4, color=0x540453),
    'originalCD': dict(Construct=0x440bb0, Value=0x441c20, Pressed=0x441c40, StoneValue=0x4417b0,
        StonePressed=0x441850, Enabled=0x418860, Blink=0x418890, Pulse=0x441020, Handle=0x440ce0,
        Broadcast=0x441170, Idle=0x441050, Local=0x493f20,
        clock=0x4011b0, poll=0x4a6820, base=0x493720, sound=0x438b50, project=0x455c90,
        value_event=0x512a40, click_event=0x512930, right_event=0x5128c8, repeat_event=0x51293c, color=0x535f88),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 전체 스칼라와 다섯 표를 동일 순서로 기록한다. scope 밖의 숨은 상태도 바뀌면 대조에 드러난다.
SCALARS = ('count', 'serial', 'focus', 'kill_count', 'dialog_count', 'current', 'override')
TABLES = (('registry', 1000), ('idle', 0), ('overlay', 100), ('kill', 1000), ('dialogs', 16))


def bits(value):
    """완성 float 입력을 원본 DWORD로 준비한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def dbits(value):
    """완성 double 입력/반환을 원본 QWORD 비트로 기록한다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def patch(raw, offset, value):
    """예상 결과 없이 지정한 DWORD 입력만 바꾼다."""
    raw = bytearray(raw); struct.pack_into('<I', raw, offset, value & 0xffffffff); return bytes(raw)


def rle(values):
    """표의 같은 연속 DWORD만 압축한다."""
    return ';'.join(f'{len(list(group))}:{value}' for value, group in itertools.groupby(values))


def encoded_nodes(nodes):
    """여섯 전체 노드의 미사용 바이트도 그대로 보존한다."""
    return ';'.join(f'{token}={raw.hex()}' for token, raw in zip(TOKENS, nodes, strict=True))


def encoded_state(state, tables):
    """일곱 현재 스칼라와 다섯 전체 표를 원본 순서로 기록한다."""
    return '#'.join([','.join(str(state[name]) for name in SCALARS), *(rle(tables[name]) for name, _ in TABLES)])


def initial(edition):
    """루트·버튼·형제/부모·팝업 후보·두 배경의 완성 입력을 만든다."""
    raw = []
    # 각 노드는 유한 좌표와 독립 미사용 바이트를 가지며 원본 vtable 주소와 별개인 가상 표를 쓴다.
    for i in range(COUNT):
        item = bytes((37 + i * 31 + offset * 17) & 255 for offset in range(SIZE))
        values = {0: VTABLE + i * 0x100, 4: 1, 8: 0, 12: 0, 16: 0, 20: 0, 24: 0,
            28: i, 32: 0x12340000 + i, 36: i, 40: 0, 44: 75 + i, 48: 0xfffffff0,
            52: 0, 56: bits(1), 60: bits(75), 64: bits(19), 68: bits(75), 72: bits(19),
            76: bits(10.25 if i == 1 else -.75), 80: bits(20.5 if i == 1 else .25),
            120: 0, 124: 0, 160: 0, 164: 0, 168: 0, 172: 0, 256: TOKENS[5]}
        # DWORD 의미별 입력과 double 예약 두 개만 작성한다.
        for offset, value in values.items(): item = patch(item, offset, value)
        item = bytearray(item); struct.pack_into('<2d', item, 176, .4, .05)
        if i in (4, 5): struct.pack_into('<I', item, 180, 0x3e00000)
        raw.append(bytes(item))
    raw[0] = patch(raw[0], 24, TOKENS[1])
    # 루트의 위치는 로컬 역변환에서 제외하며 직계 형제 next는 현재 팝업 순회에 사용한다.
    for i in range(1, COUNT): raw[i] = patch(raw[i], 20, TOKENS[0])
    raw[1] = patch(patch(raw[1], 12, TOKENS[2]), 24, TOKENS[5])
    raw[2] = patch(raw[2], 12, TOKENS[3]); raw[5] = patch(raw[5], 20, TOKENS[1])
    state = dict(count=6, serial=91, focus=0, kill_count=0, dialog_count=0, current=0, override=0)
    tables = {name: [0] * (LIFE[edition]['idle_size'] if name == 'idle' else size) for name, size in TABLES}
    tables['registry'][:COUNT] = TOKENS
    return dict(nodes=raw, state=state, tables=tables, kinds=[0] * COUNT, actions=[], responses=[],
        times=[dbits(1 + i * .001) for i in range(12)], poll=[0x10000, 999, 888],
        parent_returns=[1] * COUNT, base_return=0x81234567, color=56, anchors=[(i, 17 + i, -19 - i) for i in range(COUNT)])


def plan(edition):
    """분기·콜백 변경·표 경계·연속 입력·좌표의 완성 입력만 만들고 기대 상태/호출을 계산하지 않는다."""
    result = []; rng = random.Random(SEED); b = TOKENS[1]

    def add(actions, fields=(), kind=0):
        """각 행은 새 상태를 쓰고 연속 action 사이에만 실행 결과를 유지한다."""
        case = initial(edition); case['actions'] = list(actions); case['kinds'][1] = kind
        # 사용자가 명시한 raw 노드 쓰기만 입력에 반영한다.
        for token, offset, value in fields: case['nodes'][TOKENS.index(token)] = patch(case['nodes'][TOKENS.index(token)], offset, value)
        result.append(case); return case

    # 실제 기반 Gump 생성까지 실행해 등록 칸·serial/색 BYTE·보존 바이트를 관찰한다.
    for i, width in enumerate((75, 0, -1, 16777217, 2147483647, -2147483648)):
        case = add([('Construct', b, width & 0xffffffff, (19 - i) & 0xffffffff, rng.getrandbits(32))])
        case['tables']['registry'][1] = 0; case['state']['count'] = 5; case['state']['serial'] = 0xffffffff if i % 2 else 91
        case['color'] = (0, 127, 128, 255)[i % 4]
    # 기본/돌 버튼은 동일 값에도 redraw하며 콜백 뒤 두 상태 OR로 배경 방향을 판정한다.
    for kind, op, old, value in itertools.product((0, 1), ('Value', 'Pressed'), ((0, 0), (1, 0), (0, 1), (7, 3)), (0, 1, 0x80000007)):
        add([(op, b, value)], [(b, 160, old[0]), (b, 164, old[1])], kind)
    for op in ('Value', 'Pressed'):
        case = add([(op, b, 1)], kind=1)
        case['responses'] = [('Redraw', b, [(b, 160, 0), (b, 164, 0)])]
        case = add([(op, b, 0)], [(b, 160, 1)], kind=1)
        case['responses'] = [('Redraw', b, [(b, 256, TOKENS[4])])]
    # 상태 setter의 비영 정수·플래그 보존·첫/끝 idle 칸과 중복 호출을 관찰한다.
    for value in (0, 1, 0xffffffff):
        add([('Enabled', b, value), ('Blink', b, value)], [(b, 172, 0x80000001)])
        add([('Pulse', b, value), ('Pulse', b, value), ('Pulse', b, 0)])
    # 일반/토글/즉시·비활성·상위 플래그·부모 반환·프로그램 값 사건과 원래 좌표 비트를 교차한다.
    for flags in (0, 1, 2, 4, 6, 0x10, 0x20, 0x22, 0x80000000):
        for event, first, second in ((0x10000, 11, 21), (0x80010000, 11, 21), (0x80010000, 100, 100),
            (0x20000, 11, 21), (0x80020000, 11, 21), (0x12345, 76, 0x80000013), (0x12345, 75, 999), (0x76543, 88, 99)):
            add([('Handle', b, event, first, second)], [(b, 172, flags), (b, 164, 1)], kind=flags % 2)
    # 누름→통지 밖/복귀→뗌과 반복 idle를 같은 실제 캡처/idle/부모 저장소에서 실행한다.
    for flags in (0, 2, 4, 6, 0x20):
        add([('Handle', b, 0x10000, 11, 21), ('Broadcast', b, 0xfedcba98, 100, 100),
            ('Broadcast', b, 0, 11, 21), ('Handle', b, 0x80010000, 11, 21)] if not flags & 4 else
            [('Handle', b, 0x10000, 11, 21), ('Handle', b, 0x10000, 11, 21)], [(b, 172, flags)], kind=1)
    for flags, value in itertools.product((4, 6, 2), (0, 7)):
        add([('Handle', b, 0x10000, 11, 21), ('Handle', b, 0x80010000, 11, 21)], [(b, 172, flags), (b, 160, value)])
    # 프로그램 값 명령은 원래 사건 분기로 계속 진행하며 도움말 위임의 전체 반환을 보존한다.
    case = add([('Handle', b, 0x12345, 76, 7)])
    case['parent_returns'][0] = 0x80000017
    case['responses'] = [('Parent', TOKENS[0], [(b, 4, 0x901), (b, 44, 999), (b, 160, 8)])]
    case = add([('Handle', b, 0x20000, 11, 21)], [(b, 124, TOKENS[4])]); case['base_return'] = 0
    # 팝업 필터의 visible/void/dead 차이와 가상 Push 뒤 저장한 next를 실제 Kill 몸체로 관찰한다.
    for flags in (0x4000, 0x4009, 0x14001):
        case = add([('Handle', b, 0x10000, 11, 21)], [(TOKENS[2], 4, flags), (TOKENS[3], 4, 0x4009)])
        case['state']['focus'] = TOKENS[2]
        case['responses'] = [('Push', TOKENS[2], [(TOKENS[2], 12, 0)])]
    # 통지는 폴링의 좌표를 사용하지 않는다. 현재 value·같은 pressed·표의 마지막 칸도 보존한다.
    for flags, point, release in itertools.product((0, 1, 2), ((10, 20), (11, 21), (85, 39), (86, 40)), (0, 0x80000000)):
        case = add([('Broadcast', b, 0x76543210, *point)], [(b, 172, flags), (b, 4, 0x101), (b, 160, 7)])
        case['poll'][0] |= release
    # idle는 비활성/visible을 필터하지 않고 시간 조회 사이의 현재 예약/플래그를 사용한다.
    for flags, states, deadline in itertools.product((0, 1, 4, 0x10, 0x14), ((0, 0), (0, 1), (7, 0)), (.5, 1, 2)):
        case = add([('Idle', b)], [(b, 172, flags), (b, 160, states[0]), (b, 164, states[1])])
        item = bytearray(case['nodes'][1]); struct.pack_into('<2d', item, 176, deadline, deadline); case['nodes'][1] = bytes(item)
    case = add([('Idle', b)], [(b, 164, 1)])
    case['responses'] = [('Now', 0, [(b, 176, 0), (b, 180, 0x40000000)])]
    case = add([('Idle', b)], [(b, 164, 1)])
    case['responses'] = [('Parent', TOKENS[0], [(b, 164, 0), (b, 172, 4)])]
    # 53/64비트 덧셈 반올림 차이가 생길 수 있는 clock 공급 값을 별도 보존한다.
    for number in (0, .001, .6, 1.0, 1048576.0, 4294967.295):
        case = add([('Handle', b, 0x10000, 11, 21)])
        case['times'] = [dbits(number)] * 12
    # 로컬 변환의 루트 제외·각 float 저장·월드 부착 부모·정수 기준점 감김을 직접 관찰한다.
    for profile, point in itertools.product(range(4), ((11, 21), (-7, -9), (16777217, -16777217), (2147483647, -2147483648))):
        case = add([('Local', b, 0xabcdef01, point[0] & 0xffffffff, point[1] & 0xffffffff)])
        if profile == 0: case['nodes'][1] = patch(case['nodes'][1], 20, 0)
        if profile in (2, 3): case['nodes'][1] = patch(case['nodes'][1], 20, TOKENS[2])
        if profile == 3:
            case['nodes'][2] = patch(case['nodes'][2], 4, 0x2001)
            case['anchors'][2] = (2, 2147483647, -2147483648)
    if edition == 'originals':
        # CD는 포함 함수가 인라인이다. 세 판본 Handle/Broadcast의 좌표 분기와 별도로 패치 helper를 관찰한다.
        for point in ((0, 0), (-0.0, 0), (74.999, 18.999), (75, 0), (0, 19), (-.01, 1), (float('nan'), 1)):
            add([('Contains', b, bits(point[0]), bits(point[1]))])
    return result


def encoded(case):
    """완성 입력·명시 응답을 fixture로 직렬화하며 기대 분기를 만들지 않는다."""
    actions = ';'.join(','.join(map(str, action)) for action in case['actions'])
    extras = '#'.join((','.join(map(str, case['kinds'])), ','.join(map(str, case['times'])),
        ','.join(map(str, case['poll'])), ','.join(map(str, case['parent_returns'])),
        str(case['base_return']), str(case['color']), ';'.join(','.join(map(str, a)) for a in case['anchors'])))
    responses = ';'.join(f'{kind},{token}:' + '/'.join(','.join(map(str, p)) for p in patches)
        for kind, token, patches in case['responses']) or '-'
    return [actions, encoded_nodes(case['nodes']), encoded_state(case['state'], case['tables']), extras, responses]


class ButtonOracle:
    """실제 생성/입력/시간 분기와 Gump 하위 몸체를 실행하며 장치/시계/폴링/가상 출력만 응답한다."""
    def __init__(self, edition):
        """PE를 격리 메모리에 읽고 같은 호스트의 내보내기 범위만 실행 허용한다."""
        self.edition = edition; self.d = dict(SPECS[edition]); self.d.update(LIFE[edition]); self.d.update(CREATE[edition])
        if edition != 'originals': self.d['diagnostics'] = DATA + 0xd000
        self.d['registry'] = self.d['table']; self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / BINARIES[edition])); raw = pe.get_memory_mapped_image()
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(raw) + 4095) & ~4095); self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, raw)
        # 원본 OS/게임 프로세스와 관계없는 스택·FS·데이터·경계 영역이다.
        for address, size in ((0, 4096), (DATA, 0x10000), (STUBS, 4096), (STOP, 4096), (STACK - 0xf000, 0x10000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/buttongump/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed = []; self.names = {}
        with self.exports[1].open(encoding='utf-8') as stream:
            # 함수를 포함한 실제 불연속 몸체만 허용하고 이웃 코드는 거부한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.names[int(row['entry'], 16)] = row['entry']
                for item in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in item.split('-')); self.allowed.append((low, high + 1))
        self.stubs = {self.d[name]: name for name in ('clock', 'poll', 'base', 'sound', 'project')}
        self.total_entries = collections.Counter(); self.total_returns = collections.Counter(); self.boundaries = collections.Counter()
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def read(self, address, fmt):
        """인자/현재 상태를 정확한 원본 폭으로 읽는다."""
        return struct.unpack(fmt, self.mu.mem_read(address, struct.calcsize(fmt)))

    def put(self, address, value):
        """완성 입력/응답의 DWORD를 격리 메모리에 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def nodes(self):
        """현재 전체 노드 여섯 개를 반환한다."""
        return encoded_nodes([bytes(self.mu.mem_read(token, SIZE)) for token in TOKENS])

    def state(self):
        """현재 모든 스칼라/표를 콜백 직전과 정상 반환 뒤 동일하게 읽는다."""
        state = {name: self.read(self.d[name], '<I')[0] for name in SCALARS}
        tables = {name: self.read(self.d[name], '<' + 'I' * (self.d['idle_size'] if name == 'idle' else size)) for name, size in TABLES}
        return encoded_state(state, tables)

    def respond(self, kind, token, args=()):
        """인자/콜백 직전 전체 상태를 기록하고 명시 입력 쓰기만 반영한다."""
        key = f'{kind},{token}'; self.boundaries[kind] += 1
        self.calls.append(','.join(map(str, (kind, token, *args))) + '@' + self.state() + '@' + self.nodes())
        # 반환/분기 기대값이 아니라 입력에 명시된 callback 효과만 쓴다.
        for target, offset, value in self.responses.get(key, []):
            if target in TOKENS: address = target + offset
            elif target == 0: address = self.d[SCALARS[offset]]
            elif target == 1: address = self.d['idle'] + offset * 4
            else: raise RuntimeError('버튼 응답 대상 오류')
            self.put(address, value)

    def ret(self, purge=0, value=0):
        """명시 경계의 호출 규약에 맞게 반환하며 실제 함수 분기는 건너뛰지 않는다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP); target = self.read(sp, '<I')[0]
        self.mu.reg_write(UC_X86_REG_EAX, value); self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge); self.mu.reg_write(UC_X86_REG_EIP, target)

    def on_write(self, mu, access, address, size, value, data):
        """원본 명령의 쓰기가 노드/표/스칼라/스택/결과 범위 안인지 감사한다."""
        if not any(low <= address and address + size <= high for low, high in self.writes):
            raise RuntimeError(f'버튼 허용 밖 쓰기 {address:08x}+{size}')

    def on_instruction(self, mu, address, size, data):
        """시계·폴링·기본 도움말·소리·투영과 세 가상 장치 경계에만 입력 응답을 제공한다."""
        sp, token = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        if address in VIRTUAL:
            name, count = VIRTUAL[address]; args = self.read(sp + 4, '<' + 'I' * count)
            self.respond(name, token, args)
            value = self.case['parent_returns'][TOKENS.index(token)] if name == 'Parent' else 0
            self.ret(count * 4, value); return
        name = self.stubs.get(address)
        if name == 'clock':
            pointer = self.read(sp + 4, '<I')[0]; value = self.case['times'][self.time_index]
            self.respond('Now', self.time_index, (value,)); self.time_index += 1
            if not STACK - 0xf000 <= pointer <= STACK + 0xff8: raise RuntimeError('벽 시계 목적지 범위 오류')
            mu.mem_write(pointer, struct.pack('<Q', value)); self.ret(value=pointer); return
        if name == 'poll':
            pointer, code = self.read(sp + 4, '<2I')
            self.respond('Poll', 0, (code,)); mu.mem_write(pointer, struct.pack('<3I', *self.case['poll'])); self.ret(value=pointer); return
        if name == 'base':
            args = self.read(sp + 4, '<3I'); self.respond('Base', token, args); self.ret(12, self.case['base_return']); return
        if name == 'sound':
            pointer, *args = self.read(sp + 4, '<6I')
            if bytes(mu.mem_read(pointer, 11)) != b'button.wav\0' or any(args): raise RuntimeError('버튼 소리 인자 오류')
            self.respond('Sound', 0); self.ret(); return
        if name == 'project':
            pointer, sid, y = self.read(sp + 4, '<3I'); response = self.anchors[sid]
            if y: raise RuntimeError('버튼 투영 입력 오류')
            self.respond('Project', sid); mu.mem_write(token, struct.pack('<2i', *response)); mu.mem_write(pointer, struct.pack('<2i', *response))
            self.ret(12, token); return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'버튼 허용 밖 실행 {self.edition} {address:08x}')
        if address in self.names:
            self.entries[self.names[address]] += 1; self.total_entries[self.names[address]] += 1

    def run(self, case, control):
        """모든 action을 정상 반환까지 실행하고 ABI/x87/SEH/범위/보호 영역과 실제 반환을 보존한다."""
        self.case = case; self.calls = []; self.time_index = 0; self.entries = collections.Counter(); returns = collections.Counter(); results = []
        self.responses = {f'{kind},{token}': patches for kind, token, patches in case['responses']}
        self.anchors = {sid: (x, y) for sid, x, y in case['anchors']}; mu = self.mu; d = self.d
        mu.mem_write(DATA, b'\xa5' * 0x1000)
        # 노드와 실행 중 바뀔 수 없는 가상 표는 입력 종류에 따라 실제 상태 setter를 가리킨다.
        for i, (token, raw) in enumerate(zip(TOKENS, case['nodes'], strict=True)):
            mu.mem_write(token, raw); table = VTABLE + i * 0x100
            for offset, function in ((8, STUBS + 0xa00), (0x18, STUBS + 0xa40), (0x28, STUBS + 0xa20),
                (0x40, SPECS[self.edition]['StoneValue' if case['kinds'][i] else 'Value']),
                (0x44, SPECS[self.edition]['StonePressed' if case['kinds'][i] else 'Pressed']),
                (0x48, SPECS[self.edition]['Pulse']), (0x4c, SPECS[self.edition]['Blink'])): self.put(table + offset, function)
        vtables = bytes(mu.mem_read(VTABLE, COUNT * 0x100))
        # 원본 표/전역에 완성 입력을 공급한다. 예상 캡처나 호출 대상은 만들지 않는다.
        for name in SCALARS: self.put(d[name], case['state'][name])
        for name, _ in TABLES: mu.mem_write(d[name], struct.pack('<' + 'I' * len(case['tables'][name]), *case['tables'][name]))
        self.put(d['pool'], POOL); self.put(d['diagnostics'], 0)
        for sid, _, _ in case['anchors']: mu.mem_write(POOL + sid * d['stride'] + 14, struct.pack('<2I', sid, 0))
        for name, value in (('value_event', 0x12345), ('click_event', 0x12346), ('right_event', 0x12347), ('repeat_event', 0x12348)): self.put(d[name], value)
        mu.mem_write(d['color'], bytes([case['color']]))
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000), (RESULT, RESULT + 8)]
        self.writes += [(token, token + SIZE) for token in TOKENS]
        self.writes += [(d[name], d[name] + 4) for name in SCALARS]
        self.writes += [(d[name], d[name] + (d['idle_size'] if name == 'idle' else size) * 4) for name, size in TABLES]
        # 연속 action 사이에는 모든 실제 노드/표 상태를 유지한다.
        for op, token, *args in case['actions']:
            entry = SPECS[self.edition][('Stone' + op) if op in ('Value', 'Pressed') and case['kinds'][TOKENS.index(token)] else op]
            stack_args = [RESULT, *args] if op == 'Local' else args
            for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
            mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_EDX, 0); mu.reg_write(UC_X86_REG_ECX, token)
            mu.reg_write(UC_X86_REG_ESP, STACK); mu.reg_write(UC_X86_REG_EFLAGS, 2)
            mu.reg_write(UC_X86_REG_FPCW, control); mu.reg_write(UC_X86_REG_FPSW, 0); self.put(0, 0xffffffff)
            mu.mem_write(STACK, struct.pack('<' + 'I' * (len(stack_args) + 1), STOP, *stack_args))
            mu.emu_start(entry, STOP, timeout=30000000, count=2000000)
            if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + len(stack_args) * 4:
                raise RuntimeError(f'버튼 반환/스택 오류 {op}')
            if any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()): raise RuntimeError('버튼 보존 레지스터 오류')
            if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.read(0, '<I')[0] != 0xffffffff:
                raise RuntimeError('버튼 x87/SEH 오류')
            value = ','.join(map(str, self.read(RESULT, '<2I'))) if op == 'Local' else str(mu.reg_read(UC_X86_REG_EAX)) if op in ('Construct', 'Contains', 'Handle') else '-'
            results.append(f'{op}:{value}'); returns[f'{entry:08x}'] += 1; self.total_returns[f'{entry:08x}'] += 1
        guards = bytearray(mu.mem_read(DATA, 0x1000))
        for token in TOKENS: guards[token - DATA:token - DATA + SIZE] = b'\xa5' * SIZE
        if guards != b'\xa5' * 0x1000 or bytes(mu.mem_read(VTABLE, COUNT * 0x100)) != vtables:
            raise RuntimeError('버튼 보호/가상 표 변경')
        return [*encoded(case), ';'.join(results), self.nodes(), self.state(), '|'.join(self.calls) or '-',
            json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')), json.dumps(dict(returns), sort_keys=True, separators=(',', ':'))]


def generate(smoke):
    """53비트 관찰을 C++ 입력으로 저장하고 다른 64비트 관찰은 별도 자료에 완전히 보존한다."""
    rows = []; alternate = []; editions = {}; files = {ROOT / 'tools/decomp_buttongump_oracle.py', ROOT / 'tools/ghidra/buttongump-functions.json'}
    for edition in BINARIES:
        oracle = ButtonOracle(edition); cases = list(enumerate(plan(edition)))
        if smoke: cases = cases[::11] + cases[-4:]
        for index, case in cases:
            observed = [oracle.run(case, mode) for mode in CONTROLS]
            rows.append([edition, str(index), *observed[0]])
            if observed[0] != observed[1]: alternate.append(dict(edition=edition, index=index, observation=observed[1]))
        editions[edition] = dict(cases=len(cases), entries=dict(oracle.total_entries), returns=dict(oracle.total_returns), boundaries=dict(oracle.boundaries))
        files.update(oracle.exports); files.add(ROOT / BINARIES[edition]); print(edition, len(cases), '정상 관찰')
    if smoke: print('버튼 무저장 표본', len(rows), '64비트 차이', len(alternate)); return
    FIXTURE.write_text('# edition\tindex\tactions\tin_nodes\tin_state\textras\tresponses\tresults\tout_nodes\tout_state\tcalls\tentries\treturns\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    ALTERNATE.write_text(json.dumps(alternate, ensure_ascii=False, separators=(',', ':')) + '\n', encoding='utf-8', newline='\n')
    files.update((FIXTURE, ALTERNATE, ROOT / 'tools/decomp_process_oracle.py', ROOT / 'tools/decomp_drawsort_oracle.py',
        ROOT / 'tools/decomp_gumplifetime_oracle.py', ROOT / 'tools/decomp_gumpcreate_oracle.py'))
    report = dict(host=socket.gethostname(), independent_cases=len(rows), controls=list(CONTROLS), alternative_differences=len(alternate),
        editions=editions, boundaries=['Redraw', 'Parent', 'Push', 'Now', 'Poll', 'Base', 'Sound', 'Project'],
        files={str(p.relative_to(ROOT)).replace('\\', '/'): digest(p) for p in sorted(files)})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); verify()


def verify():
    """SHA·입력 순서/전수 행·실제 진입/정상 반환·경계 집계를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError('버튼 근거 SHA 불일치: ' + name)
    with FIXTURE.open(encoding='utf-8') as stream: rows = [r for r in csv.reader(stream, delimiter='\t') if r and not r[0].startswith('#')]
    if len(rows) != report['independent_cases']: raise RuntimeError('버튼 근거 행 수 오류')
    alternate = json.loads(ALTERNATE.read_text(encoding='utf-8'))
    if len(alternate) != report['alternative_differences']: raise RuntimeError('버튼 대안 모드 기록 수 오류')
    alternative = {(r['edition'], r['index']): r['observation'] for r in alternate}
    for edition, totals in report['editions'].items():
        selected = [r for r in rows if r[0] == edition]; expected = plan(edition)
        if len(selected) != len(expected) or len(selected) != totals['cases']: raise RuntimeError('버튼 입력 전수 수 오류')
        entries = collections.Counter(); returns = collections.Counter(); boundaries = collections.Counter()
        for i, row in enumerate(selected):
            if len(row) != 13 or int(row[1]) != i or row[2:7] != encoded(expected[i]): raise RuntimeError('버튼 완성 입력/순서 오류')
            observations = [row[2:], alternative.get((edition, i), row[2:])]
            for observed in observations:
                entries.update(json.loads(observed[9])); returns.update(json.loads(observed[10]))
                if observed[8] != '-':
                    for call in observed[8].split('|'): boundaries[call.split(',')[0]] += 1
        if dict(entries) != totals['entries'] or dict(returns) != totals['returns'] or dict(boundaries) != totals['boundaries']:
            raise RuntimeError('버튼 진입/반환/경계 집계 오류')
    print('buttongump 근거 감사 통과:', len(rows), '개, 두 x87 정상 반환', sum(sum(x['returns'].values()) for x in report['editions'].values()), '회')


def main():
    """무저장 표본/전수 생성/보존 자료 감사를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--smoke', action='store_true'); parser.add_argument('--verify', action='store_true')
    args = parser.parse_args(); verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
