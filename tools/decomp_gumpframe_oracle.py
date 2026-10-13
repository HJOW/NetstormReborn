#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump 통지·idle·재그리기 요청 전체 몸체를 격리 x86에서 정상 반환까지 관찰한다."""
import argparse
import collections
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

# 실제 함수/등록표/idle 표/개수 전역이다. CD·10.37은 같은 코드 배치를 사용한다.
SPECS = {
    'originals': dict(Broadcast=0x464100, Idle=0x464170, RedrawNode=0x464c20, RedrawRoots=0x4660f0,
        registry=0x563e38, idle=0x5634d0, idle_size=500, count=0x565dc8),
    'originalCD': dict(Broadcast=0x492940, Idle=0x492990, RedrawNode=0x4939e0, RedrawRoots=0x4927c0,
        registry=0x56b748, idle=0x56b5b8, idle_size=80, count=0x535e80),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 여섯 기본 노드와 읽기 전용 가상 표/명시 경계의 격리 주소 및 입력 시드다.
DATA, SIZE, COUNT, SEED = 0x18000000, 160, 6, 0x46417078
TOKENS = tuple(DATA + 16 + i * 192 for i in range(COUNT))
VTABLE = DATA + 0x9000
VIRTUAL = {STUBS + 0xa00: ('Idle', 0), STUBS + 0xa20: ('Redraw', 1), STUBS + 0xa40: ('Broadcast', 3)}
# 실제 실행 관찰과 근거 해시 보고서를 저장한다. C++ 구현은 이 도구에서 읽거나 호출하지 않는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/gumpframe-x86.tsv'
REPORT = ROOT / 'cpppj/gumpframe.json'


def nodes():
    """기대 결과 없이 유한 float 필드와 독립 미사용 바이트를 가진 트리를 준비한다."""
    result = []
    # root의 직계 자식 둘과 두 단계 손자를 구성한다. 링크는 원본 토큰이다.
    for index in range(COUNT):
        raw = bytearray((index * 31 + byte * 17 + 37) & 255 for byte in range(SIZE))
        values = {0: VTABLE, 4: 0x21, 12: 0, 20: 0, 24: 0}
        # float 필드는 호스트 재직렬화에서 NaN 정규화가 생기지 않도록 유한 값만 쓴다.
        for offset in range(56, 84, 4): values[offset] = 0x3f800000 + index * 0x10000
        # 입력의 지정 DWORD만 작성한다. callback의 예상 변경 상태는 넣지 않는다.
        for offset, value in values.items(): struct.pack_into('<I', raw, offset, value)
        result.append(bytes(raw))
    result[0] = changed(result[0], 24, TOKENS[1])
    result[1] = changed(changed(result[1], 12, TOKENS[2]), 24, TOKENS[3])
    result[3] = changed(changed(result[3], 12, TOKENS[4]), 24, TOKENS[5])
    return result


def changed(raw, offset, value):
    """노드 입력 또는 명시 응답의 한 DWORD만 바꾼다."""
    result = bytearray(raw); struct.pack_into('<I', result, offset, value & 0xffffffff)
    return bytes(result)


def rle(values):
    """전체 표의 연속 같은 값만 압축한다. 호출 대상이나 필터 결과를 계산하지 않는다."""
    return ';'.join(f'{len(list(group))}:{value}' for value, group in itertools.groupby(values))


def state(count, registry, idle):
    """현재 개수와 전체 등록/idle 표를 원본 순서대로 직렬화한다."""
    return '#'.join((str(count), rle(registry), rle(idle)))


def encoded_nodes(raw):
    """전체 0xa0바이트 노드 여섯 개의 미사용 필드까지 보존한다."""
    return ';'.join(f'{token}={value.hex()}' for token, value in zip(TOKENS, raw, strict=True))


def encoded(case):
    """완성 입력과 응답 쓰기만 저장하며 예상 호출/반환 순서는 넣지 않는다."""
    actions = ';'.join(','.join(map(str, action)) for action in case['actions'])
    responses = ';'.join(f'{kind},{token}:' + '/'.join(','.join(map(str, patch)) for patch in patches)
        for kind, token, patches in case['responses']) or '-'
    return [actions, encoded_nodes(case['nodes']), state(case['count'], case['registry'], case['idle']), responses]


def plan(edition):
    """표 경계/필터/현재 개수/콜백 전후 링크의 독립 입력을 만든다."""
    result = []; capacity = SPECS[edition]['idle_size']; a, b, c, d, e, f = TOKENS

    def add(actions, raw=None, count=6, responses=()):
        """각 행은 독립 입력에서 시작하며 연속 action 사이의 실제 상태만 유지한다."""
        registry = [0] * 1000; registry[:COUNT] = TOKENS
        case = dict(actions=actions, nodes=list(raw or nodes()), registry=registry, idle=[0] * capacity,
            count=count, responses=list(responses)); result.append(case)
        return case

    add([('Idle', 0)])
    # 표의 처음/중간/마지막과 같은 객체의 중복 호출은 플래그에 무관하게 실행한다.
    for flags in (0, 1, 8, 0x80, 0x10000, 0x100089, 0xffffffff):
        # 판본별 활성 표 경계를 독립 입력으로 교차한다.
        for slot in sorted({0, 1, 39, capacity - 1}):
            raw = nodes(); raw[1] = changed(raw[1], 4, flags)
            case = add([('Idle', 0)], raw); case['idle'][slot] = b
    case = add([('Idle', 0), ('Idle', 0)]); case['idle'][0] = case['idle'][-1] = b
    case = add([('Idle', 0)], responses=[('Idle', b, [(2, 0, c), (2, 2, c), (2, capacity - 1, d), (0, 0, 0xffffffff)])])
    case['idle'][0] = b
    case = add([('Idle', 0)], responses=[('Idle', b, [(2, capacity - 1, 0)])]); case['idle'][0] = b; case['idle'][-1] = c

    # count는 비영 칸 방문 수이며 root와 필터 성공 수가 아니다. signed 음수도 정상 조기 종료한다.
    for count in (0, 0xffffffff, 1, 2, 5, 6, 1000):
        # visible/통지 비트/dead 조합과 루트의 조건 일치 여부를 교차한다.
        for flags in (0, 1, 0x100, 0x101, 0x109, 0x10101, 0x80000101):
            raw = [changed(value, 4, flags) for value in nodes()]
            case = add([('Broadcast', 0, 0x81234567, 0x76543210, 0xfedcba98)], raw, count)
            case['registry'] = [0] * 1000; case['registry'][0] = a
            case['registry'][1] = b; case['registry'][19] = c; case['registry'][999] = d
    # 콜백이 count를 줄이거나 늘리고 미래 슬롯을 바꾸는 완성 응답이다.
    for count in (0, 1, 4, 0xffffffff):
        raw = [changed(value, 4, 0x101) for value in nodes()]
        add([('Broadcast', 0, 7, 11, 13)], raw, 2,
            [('Broadcast', b, [(0, 0, count), (1, 0, f), (1, 2, 0), (1, 10, d)])])

    # 단일 노드는 자신 visible/dead 없이 실행하고 최상위 순회만 visible/비 dead를 요구한다.
    for op in ('RedrawNode', 'RedrawRoots'):
        # 생략 비트와 자식 전파 비트/자기 visible/dead를 서로 독립으로 공급한다.
        for flags in (0, 1, 9, 0x20, 0x21, 0x10001, 0x10021, 0x2001, 0x2021, 0x40021, 0x42021):
            # 하위 자식은 visible이면 dead/void여도 실행하며 숨은 자식은 건너뛴다.
            for child_flags in (0, 1, 9, 0x10001, 0x21):
                raw = nodes(); raw[1] = changed(raw[1], 4, flags); raw[3] = changed(raw[3], 4, child_flags)
                add([(op, b if op == 'RedrawNode' else 0)], raw)
    # 부모의 자식 전파 비트/자식 머리를 가상 요청 뒤 현재 값으로 읽는다.
    for flags in (1, 0x21, 0x2021):
        # 콜백은 대상 노드의 현재 링크/플래그만 바꾸며 예상 호출 목록은 계산하지 않는다.
        for replacement in (0, e):
            add([('RedrawNode', b)], responses=[('Redraw', b, [(b, 4, flags), (b, 24, replacement)])])
    # 최상위 next는 콜백 전에 저장하지만 하위 next는 재귀 뒤 현재 값을 읽는다.
    add([('RedrawRoots', 0)], responses=[('Redraw', b, [(b, 12, 0)]), ('Redraw', d, [(d, 12, 0)])])
    add([('RedrawNode', b)], responses=[('Redraw', d, [(d, 12, 0), (d, 4, 1)])])
    add([('RedrawRoots', 0)], responses=[('Redraw', b, [(c, 4, 0x10001), (c, 12, e)]), ('Redraw', e, [(e, 4, 9)])])
    raw = nodes(); raw[0] = changed(raw[0], 4, 0); add([('RedrawRoots', 0)], raw)
    raw = nodes(); raw[0] = changed(raw[0], 24, 0); add([('RedrawRoots', 0)], raw)

    rng = random.Random(SEED)
    # 유한 트리 구조는 유지하며 플래그/표 구멍/개수/상위 비트 인자를 무작위로 교차한다.
    for index in range(64):
        raw = [changed(value, 4, rng.getrandbits(32)) for value in nodes()]
        op = ('Idle', 'Broadcast', 'RedrawNode', 'RedrawRoots')[index % 4]
        action = (op, b) if op == 'RedrawNode' else (op, 0, *[rng.getrandbits(32) for _ in range(3)]) if op == 'Broadcast' else (op, 0)
        case = add([action], raw, rng.choice((0, 1, 3, 6, 999, 0xffffffff)))
        # 슬롯 배치는 기대 결과와 독립이다. 같은 노드를 중복 배치하는 경우도 포함한다.
        for table in (case['registry'], case['idle']):
            # 적은 수의 실제 포인터만 넣고 나머지는 빈 칸으로 유지한다.
            for _ in range(8): table[rng.randrange(len(table))] = rng.choice(TOKENS)
        case['registry'][0] = a
    return result


class GumpFrameOracle:
    """원본 PE 함수 밖 실행/미등록 쓰기를 거부하고 세 가상 경계만 명시 응답으로 대체한다."""
    def __init__(self, edition):
        """원본 파일은 읽기만 하고 독립 가상 메모리에 코드·표를 준비한다."""
        self.edition = edition; self.d = SPECS[edition]; self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / BINARIES[edition])); image = pe.get_memory_mapped_image()
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
        # FS:[0]/노드/경계/중단/호출 스택은 원본 프로세스와 무관한 격리 메모리다.
        for address, size in ((0, 4096), (DATA, 0x10000), (STUBS, 4096), (STOP, 4096), (STACK - 0xf000, 0x10000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/gumpframe/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed = []; self.names = {}
        with self.exports[1].open(encoding='utf-8') as stream:
            # 실제 함수의 불연속 범위만 허용한다. 이웃 함수/OS/assert를 실행하지 않는다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.names[int(row['entry'], 16)] = row['entry']
                # 범위 끝은 Ghidra inclusive 주소라 허용 검사에만 1을 더한다.
                for item in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in item.split('-')); self.allowed.append((low, high + 1))
        self.hits = collections.Counter(); self.returns = collections.Counter(); self.boundaries = collections.Counter()
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def u32(self, address):
        """원본 메모리의 DWORD를 unsigned 비트값으로 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def put(self, address, value):
        """명시 입력/응답의 하위 DWORD만 격리 메모리에 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def current_nodes(self):
        """현재 전체 노드와 미사용 바이트를 기록한다."""
        return encoded_nodes([bytes(self.mu.mem_read(token, SIZE)) for token in TOKENS])

    def current_state(self):
        """현재 개수와 표 전체를 콜백 직전/정상 반환 뒤 같은 형식으로 읽는다."""
        registry = struct.unpack('<1000I', self.mu.mem_read(self.d['registry'], 4000))
        idle = struct.unpack('<' + 'I' * self.d['idle_size'], self.mu.mem_read(self.d['idle'], self.d['idle_size'] * 4))
        return state(self.u32(self.d['count']), registry, idle)

    def on_write(self, mu, access, address, size, value, data):
        """실제 명령의 허용 노드/표/개수/스택 쓰기만 받는다."""
        if not any(low <= address and address + size <= high for low, high in self.writes):
            raise RuntimeError(f'Gump 프레임 허용 밖 쓰기: {address:08x}/{size}')

    def on_instruction(self, mu, address, size, data):
        """세 가상 경계의 호출 직전 전체 상태를 관찰하고 입력 응답만 적용한다."""
        if address in VIRTUAL:
            kind, count = VIRTUAL[address]; esp = mu.reg_read(UC_X86_REG_ESP); token = mu.reg_read(UC_X86_REG_ECX)
            args = [self.u32(esp + 4 + i * 4) for i in range(count)]
            self.calls.append(','.join(map(str, (kind, token, *args))) + '@' + self.current_state() + '@' + self.current_nodes())
            self.boundaries[kind] += 1
            # 기대 호출 대상/flags는 계산하지 않고 완성 응답의 raw 쓰기만 적용한다.
            for target, offset, value in self.responses.get((kind, token), ()):
                if target == 0:
                    if offset != 0: raise RuntimeError('Gump 프레임 응답 전역 범위 오류')
                    destination = self.d['count']
                elif target in (1, 2):
                    limit = 1000 if target == 1 else self.d['idle_size']
                    if not 0 <= offset < limit: raise RuntimeError('Gump 프레임 응답 표 범위 오류')
                    destination = self.d['registry' if target == 1 else 'idle'] + offset * 4
                else:
                    if target not in TOKENS or not 0 <= offset <= SIZE - 4: raise RuntimeError('Gump 프레임 응답 노드 범위 오류')
                    destination = target + offset
                self.put(destination, value)
            mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_ESP, esp + 4 + count * 4)
            mu.reg_write(UC_X86_REG_EIP, self.u32(esp)); return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'Gump 프레임 허용 밖 실행: {self.edition} {address:08x}')
        if address in self.names:
            self.entries[self.names[address]] += 1; self.hits[self.names[address]] += 1

    def run(self, case, control):
        """연속 action의 정상 반환/ABI/x87/SEH/보호 바이트와 전체 호출 상태를 관찰한다."""
        mu = self.mu; self.calls = []; self.entries = collections.Counter(); returns = collections.Counter()
        self.responses = {(kind, token): patches for kind, token, patches in case['responses']}
        mu.mem_write(DATA, b'\xa5' * 0x1000)
        # 입력 노드와 두 표의 완성 바이트만 공급한다.
        for token, raw in zip(TOKENS, case['nodes'], strict=True): mu.mem_write(token, raw)
        mu.mem_write(self.d['registry'], struct.pack('<1000I', *case['registry']))
        mu.mem_write(self.d['idle'], struct.pack('<' + 'I' * self.d['idle_size'], *case['idle']))
        self.put(self.d['count'], case['count'])
        # 실제 가상 슬롯 +8/+0x2c/+0x30만 준비한다. 표는 실행 중 읽기 전용이다.
        for offset, address in ((8, STUBS + 0xa20), (0x2c, STUBS + 0xa40), (0x30, STUBS + 0xa00)):
            self.put(VTABLE + offset, address)
        vtable_before = bytes(mu.mem_read(VTABLE, 64))
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000), (self.d['count'], self.d['count'] + 4),
            (self.d['registry'], self.d['registry'] + 4000), (self.d['idle'], self.d['idle'] + self.d['idle_size'] * 4)]
        self.writes.extend((token, token + SIZE) for token in TOKENS)
        # 각 최상위 정상 반환을 확인하며 중간 상태는 다음 action에 유지한다.
        for action in case['actions']:
            op, token = action[:2]; args = action[2:]
            # 비영 보존 레지스터를 고정해 콜백/재귀 뒤 ABI 오염을 검출한다.
            for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
            mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_EDX, 0); mu.reg_write(UC_X86_REG_ECX, token)
            mu.reg_write(UC_X86_REG_ESP, STACK); mu.reg_write(UC_X86_REG_EFLAGS, 2)
            mu.reg_write(UC_X86_REG_FPCW, control); mu.reg_write(UC_X86_REG_FPSW, 0); self.put(0, 0xffffffff)
            mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
            mu.emu_start(self.d[op], STOP, timeout=30000000, count=2000000)
            if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 or any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()):
                raise RuntimeError('Gump 프레임 정상 반환/ABI 오류')
            if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.u32(0) != 0xffffffff:
                raise RuntimeError('Gump 프레임 x87/SEH 오류')
            key = f'{self.d[op]:08x}'; returns[key] += 1; self.returns[key] += 1
        guards = bytearray(mu.mem_read(DATA, 0x1000))
        # 노드 영역 이외의 보호 바이트와 가상 표 불변을 확인한다.
        for token in TOKENS: guards[token - DATA:token - DATA + SIZE] = b'\xa5' * SIZE
        if guards != b'\xa5' * 0x1000 or bytes(mu.mem_read(VTABLE, 64)) != vtable_before:
            raise RuntimeError('Gump 프레임 보호/읽기 전용 변경')
        return [*encoded(case), self.current_nodes(), self.current_state(), '|'.join(self.calls) or '-',
            json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')), json.dumps(dict(returns), sort_keys=True, separators=(',', ':'))]


def generate(smoke):
    """두 x87 모드의 관찰이 같은 행만 fixture로 저장한다. 기대 동작은 직접 계산하지 않는다."""
    rows = []; editions = {}; files = {FIXTURE, ROOT / 'tools/ghidra/gumpframe-functions.json',
        ROOT / 'tools/decomp_gumpframe_oracle.py', ROOT / 'tools/decomp_process_oracle.py', ROOT / 'tools/decomp_drawsort_oracle.py'}
    # 세 PE를 독립 가상 메모리에서 실행하며 게임/GUI/OS 호출은 허용하지 않는다.
    for edition in BINARIES:
        oracle = GumpFrameOracle(edition); cases = list(enumerate(plan(edition)))
        if smoke: cases = cases[::13] + cases[-3:]
        # 각 행의 최상위 반환과 모든 콜백 직전 상태를 두 정밀도에서 대조한다.
        for index, case in cases:
            observed = [oracle.run(case, control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'Gump 프레임 x87 차이: {edition}/{index}')
            rows.append([edition, str(index), *observed[0]])
        editions[edition] = dict(cases=len(cases), entries=dict(oracle.hits), returns=dict(oracle.returns), boundaries=dict(oracle.boundaries))
        files.update(oracle.exports); files.add(ROOT / BINARIES[edition]); print(f'{edition}: {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump 통지/idle/재그리기 전체 몸체. 세 가상 호출만 명시 입력 경계.\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(primary_target='10.78', total=len(rows), host=socket.gethostname(), date='2026-10-12',
        controls=list(CONTROLS), os_calls=0, assertions=0, editions=editions,
        files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(files)},
        limits=['등록표 통지/idle 순회/자식·루트 재그리기 요청의 전체 정상 함수와 실제 재귀',
            '가상 +0x2c/+0x30/+8만 명시 응답; 파생 함수/픽셀 출력/OS/실제 게임 프레임 제외',
            '전체 노드/표/개수/콜백 직전 상태·인자, ABI/x87/SEH/허용 실행·쓰기/보호/읽기 전용/SHA 감사']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """저장된 입력/함수 범위/바이트 해시와 두 정밀도 정상 반환 집계를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 실제 도구/PE/내보내기/fixture 파일의 정확한 바이트를 감사한다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError('Gump 프레임 SHA 불일치: ' + name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls'] or report['assertions']:
        raise RuntimeError('Gump 프레임 총계 오류')
    # 판본별 완성 입력과 실제 관찰 행을 순서대로 비교한다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0] == edition]; cases = plan(edition)
        if len(chosen) != len(cases) or item['cases'] != len(cases): raise RuntimeError('Gump 프레임 판본 행 수 오류')
        entries = collections.Counter(); returns = collections.Counter(); boundaries = collections.Counter()
        # 기대 출력은 재계산하지 않고 입력/폭/실제 진입 및 최상위 반환 기록만 검사한다.
        for index, (row, case) in enumerate(zip(chosen, cases, strict=True)):
            if len(row) != 11 or row[1] != str(index) or row[2:6] != encoded(case): raise RuntimeError('Gump 프레임 입력/열 오류')
            if len(row[6].split(';')) != COUNT or any(len(bytes.fromhex(value.split('=')[1])) != SIZE for value in row[6].split(';')):
                raise RuntimeError('Gump 프레임 노드 폭 오류')
            parts = row[7].split('#')
            if len(parts) != 3 or [sum(int(value.split(':')[0]) for value in part.split(';')) for part in parts[1:]] != [1000, SPECS[edition]['idle_size']]:
                raise RuntimeError('Gump 프레임 표 폭 오류')
            if dict(collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions'])) != json.loads(row[10]):
                raise RuntimeError('Gump 프레임 정상 반환 누락')
            # 보고서는 두 모드의 실제 호출 횟수이고 fixture는 공통 관찰 한 행이다.
            for address, count in json.loads(row[9]).items(): entries[address] += count * 2
            # 각 최상위 action의 정상 반환을 양 모드에서 집계한다.
            for address, count in json.loads(row[10]).items(): returns[address] += count * 2
            if row[8] != '-':
                # 실제 가상 경계 기록의 종류만 집계한다.
                for call in row[8].split('|'): boundaries[call.split(',')[0]] += 2
        if dict(entries) != item['entries'] or dict(returns) != item['returns'] or dict(boundaries) != item['boundaries']:
            raise RuntimeError('Gump 프레임 진입/반환/경계 집계 오류')
    total_returns = sum(sum(item['returns'].values()) for item in report['editions'].values())
    print(f'gumpframe 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {total_returns}회, OS/assert 0')


def main():
    """전체 기록 생성/무저장 표본/저장 감사 모드를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--smoke', action='store_true'); parser.add_argument('--verify', action='store_true')
    args = parser.parse_args(); verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
