#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리/섬 연결·연결 객체 생성·소유자 전파를 세 실제 PE의 제한 x86으로 대조한다.

실제 명령으로 실행하는 것: 연결 순회(004213b0 ↔ CD 00448b00), 연결 객체 생성(004210f0 ↔ CD 004487c0),
소유자 전파(00421240 ↔ CD 00448910), flag 0~7 연결 이웃 탐색기의 생성·Next·필터, 일반 사각형 탐색,
좌표 한 칸의 타입/마스크 조회(004b1fa0·004b2070 ↔ CD 004eb4b0·004eb5c0), 소유자 색 프레임(004421a0 ↔ CD 004d0180),
섬/종유석 소유자 지정(00442310 ↔ CD 004d01a0·004d0350), 섬 postPop 접두(004421c0 ↔ CD 004d01d0), 중심·방향·한 칸 이동 기하.
명시적으로 대체하는 것(호출 사실과 인자를 기록): 새 객체 생성(헤더만 만든다), 가상 소유자 지정(+0x74, 소유자 바이트만 쓴다),
가상 Pop(+0x90), 프레임 지정(004acee0 ↔ CD 004acbc0, 프레임 필드만 쓴다), 표면 알림, base 소유자 지정·공통 postPop.
원본 게임/OS/업데이터는 실행하지 않는다.

python -X utf8 tools/decomp_bridgeconnect_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_bridgeconnect_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
python -X utf8 tools/decomp_bridgeconnect_oracle.py --smoke    # 파일을 쓰지 않고 일부 입력만 실행
"""
import argparse
import collections
import csv
import importlib.metadata
import itertools
import json
import multiprocessing
import random
import struct
import zlib
from pathlib import Path

from decomp_bridgeevent_oracle import (ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE, VTABLE, TYPES, CODES, POOL,
    CAPACITY, base_case, float_bits, frame_codes, digest)
from decomp_neighbor_oracle import NeighborOracle, FINDER, packed
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 합성 타입 번호. 플래그·발자국은 실제 10.78 타입 표의 값이다(isle·isleBig·priest·bridgeConnector·island·islandStalag·noIsland).
ISLE, ISLE_BIG, PLAIN, CONNECTOR, ISLAND, STALAG, NO_ISLAND = 83, 84, 85, 90, 91, 92, 93
# 타입 번호 -> (flags1, flags2, 발자국 폭, 발자국 높이)
TYPE_FLAGS = {
    BRIDGE_TYPE: (0x00000802, 0x00000004, 1, 1),
    ISLE: (0x28000803, 0x00000002, 1, 1),
    ISLE_BIG: (0x08000803, 0x01000002, 3, 3),
    PLAIN: (0x00069012, 0x00210000, 1, 1),
    CONNECTOR: (0x28000002, 0x02000000, 1, 1),
    ISLAND: (0x28000003, 0x01000000, 3, 3),
    STALAG: (0x28000002, 0x00000000, 3, 3),
    NO_ISLAND: (0x08000803, 0x01000002, 1, 1),
}
# 대체 생성이 차례로 돌려주는 새 객체 번호와 개수.
BORN_FIRST, BORN_COUNT = 100, 12
# 미리보기 목록(개수 DWORD + 번호 DWORD 배열)의 에뮬레이터 주소. 탐색기 영역 안이라 쓰기가 허용된다.
LIST_COUNT, LIST_ITEMS = FINDER + 0x100, FINDER + 0x104
# 입력만 만드는 고정 시드. 기대 결과는 원본 기계어가 정한다.
SEED = 0x4213b0
# 판본별 진입점·대체 진입점·전역.
EXTRA = {
    'originals': dict(
        entries=dict(Connect=0x4213b0, Link=0x4210f0, Owner=0x421240, FindAt=0x4b1fa0, Color=0x4421a0,
                     IslandOwner=0x442310, StalagOwner=0x442310, IslandPost=0x4421c0, Next=0x4b1e70),
        stubs={0x4acee0: 'setframe', 0x4adf00: 'baseowner', 0x4b0d30: 'basepost'},
        globals=dict(connector=0x5412c4, island=0x5411d0, stalag=0x5411d4, noisland=0x5412cc, battle=0x594fbc,
                     colors=0x531b08, mission=0x594fa0)),
    'originalCD': dict(
        entries=dict(Connect=0x448b00, Link=0x4487c0, Owner=0x448910, FindAt=0x4eb4b0, Color=0x4d0180,
                     IslandOwner=0x4d01a0, StalagOwner=0x4d0350, IslandPost=0x4d01d0, Next=0x4ec030),
        stubs={0x4acbc0: 'setframe', 0x4aefa0: 'baseowner', 0x4ae180: 'basepost'},
        globals=dict(connector=0x51cbb0, island=0x51cabc, stalag=0x51cac0, noisland=0x51cbb8, battle=0x540a20,
                     colors=0x5365a0, mission=0x52e16c)),
}
EXTRA['original1037'] = EXTRA['originalCD']
# 이 도구가 직접 처리하는 대체 이름.
HANDLED = ('create', 'owner', 'setframe', 'baseowner', 'basepost')
# 새 결과 파일은 이전 기록/fixture와 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bridgeconnect-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-bridgeconnect-evidence.json'


class ConnectOracle(NeighborOracle):
    """실제 탐색/해시 실행기 위에 연결 함수 몸체와 외부 효과 대체를 더한다."""
    def __init__(self, edition):
        """새 내보내기의 몸체만 추가로 허용하고 대체 진입점·전역 주소를 연결한다."""
        super().__init__(edition)
        extra = EXTRA[edition]
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **extra['entries']))
        self.stubs.update(extra['stubs'])
        self.xg = extra['globals']
        self.born = 0
        paths = [ROOT / f'extracted/bridgeconnect/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def frame_field(self, sid):
        """판본별 프레임 필드의 주소와 폭."""
        return self.slot(sid) + self.o['frame'], self.o['frame_size']

    def on_instruction(self, mu, address, size, data):
        """외부 효과 진입점은 사건을 기록하고 최소한의 필드만 쓴 뒤 반환한다. 그 밖에는 부모의 허용 범위 검사를 따른다."""
        name = self.stubs.get(address)
        if name not in HANDLED:
            super().on_instruction(mu, address, size, data)
            return
        self.instructions += 1
        self.stub_calls[name] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)
        ret, a0, a1, a2 = struct.unpack('<IIII', mu.mem_read(esp, 16))
        ecx = mu.reg_read(UC_X86_REG_ECX)
        purge = 0
        if name == 'create':
            # 새 객체의 헤더(vtable·타입)만 만든다. 실제 할당/postCreate는 대체 경계다.
            if self.born >= BORN_COUNT:
                raise RuntimeError('새 객체 번호 소진')
            sid = BORN_FIRST + self.born
            self.born += 1
            slot = self.slot(sid)
            mu.mem_write(slot, bytes(self.stride))
            mu.mem_write(slot, struct.pack('<I', VTABLE))
            mu.mem_write(slot + 10, bytes([a0 & 0xff, 0]))
            self.events.append(f'C:{a0}:{a1}')
            mu.reg_write(UC_X86_REG_EAX, slot)
        elif name in ('owner', 'baseowner'):
            # 가상 소유자 지정(+0x74)과 base 소유자 지정: 소유자 바이트만 쓴다. 작업장 목록 효과는 대체 경계다.
            sid = self.sid_of(ecx)
            mu.mem_write(self.slot(sid) + self.o['owner'], bytes([a0 & 0xff]))
            self.events.append(f'{"O" if name == "owner" else "B"}:{sid}:{a0}')
            purge = 4
        elif name == 'setframe':
            # 프레임 지정: 프레임 필드만 쓴다. 표시 갱신·단계 재등록은 대체 경계다.
            sid = self.sid_of(ecx)
            address, width = self.frame_field(sid)
            mu.mem_write(address, (a0 & 0xffffffff).to_bytes(4, 'little')[:width])
            self.events.append(f'S:{sid}:{a0}:{a1}')
            purge = 8
        else:
            self.events.append(f'T:{self.sid_of(ecx)}:{a0}')
            purge = 4
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def prepare_scene(self, scene):
        """장면의 객체·타입·전역·해시 체인을 입력한다. 연결/소유 판단은 계산하지 않는다."""
        nodes = scene['nodes']
        source = next(node for node in nodes if node[0] == BRIDGE)
        case = base_case(variant=scene['variant'], bridge_global=scene['bridge_global'], frame=source[10],
                         x=source[4], y=source[5], flag=source[3], owner=scene['owners'].get(BRIDGE, 0),
                         word=scene.get('word', 0x1234), parent=scene.get('parent', 0x2a2b))
        # 앞선 장면의 슬롯이 남지 않도록 풀 전체를 지운 뒤 객체를 입력한다.
        self.mu.mem_write(POOL, bytes(CAPACITY * self.stride))
        self.prepare_space(case, nodes, scene['spots'])
        codes = frame_codes(scene['variant'])
        foot = 0x1d4 if self.spec['patch'] else 0x1b4
        # 객체가 없는 타입도 조회 대상이므로 모든 합성 타입의 플래그·발자국·프레임 코드를 쓴다.
        for number, (flags1, flags2, width, height) in TYPE_FLAGS.items():
            base = TYPES + number * self.type_stride
            address = CODES + (number - BRIDGE_TYPE) * 0x400
            self.mu.mem_write(address, codes)
            self.mu.mem_write(base + 0xe8, struct.pack('<II', flags1, flags2))
            self.mu.mem_write(base + foot, struct.pack('<II', width, height))
            self.mu.mem_write(base + 0x114, struct.pack('<I', len(codes) // 4))
            self.mu.mem_write(base + 0x124, struct.pack('<I', address))
        # 소유자 바이트는 다른 필드를 지운 뒤에 쓴다.
        for sid, owner in scene['owners'].items():
            self.mu.mem_write(self.slot(sid) + self.o['owner'], bytes([owner]))
        for name, value in (('connector', CONNECTOR), ('island', ISLAND), ('stalag', STALAG), ('noisland', NO_ISLAND),
                            ('battle', scene['battle']), ('mission', scene['mission'])):
            self.mu.mem_write(self.xg[name], struct.pack('<I', value))
        self.mu.mem_write(self.xg['colors'], struct.pack('<10i', *scene['colors']))
        self.born = 0
        self.write_ranges = []
        # 새 객체 슬롯은 지우고, 원본 몸체가 쓸 수 있는 필드만 허용한다: +8 단어, +0xc 단어, 위치·화면 좌표, 프레임.
        for sid in range(BORN_FIRST, BORN_FIRST + BORN_COUNT):
            slot = self.slot(sid)
            self.mu.mem_write(slot, bytes(self.stride))
            address, width = self.frame_field(sid)
            self.write_ranges += [(slot + 8, slot + 10), (slot + 12, slot + 14), (slot + 14, slot + 26), (address, address + width)]
        self.mu.mem_write(LIST_COUNT, bytes(0x100))

    def observed(self, list_used):
        """사건(탐색기 관찰 제외), 만든 객체 슬롯 전체, 풀 Adler-32, 미리보기 목록을 돌려준다."""
        events = [event for event in self.events if not event.startswith(('F:', 'I:'))]
        born = [bytes(self.mu.mem_read(self.slot(BORN_FIRST + index), self.stride)).hex() for index in range(self.born)]
        pool = zlib.adler32(bytes(self.mu.mem_read(POOL, CAPACITY * self.stride)))
        listed = '-'
        if list_used:
            count = struct.unpack('<I', self.mu.mem_read(LIST_COUNT, 4))[0]
            if count > 60: raise RuntimeError('미리보기 목록 개수 이상')
            listed = ','.join(map(str, struct.unpack(f'<{count}I', self.mu.mem_read(LIST_ITEMS, 4 * count)))) or 'empty'
        return [';'.join(events) or '-', ','.join(born) or '-', pool, listed]

    def walk(self, scene, flags, control):
        """실제 탐색기 생성과 Next를 0이 나올 때까지 실행해 이웃 번호를 순서대로 모은다."""
        self.prepare_scene(scene)
        self.mu.mem_write(FINDER, bytes(0x100))
        self.call('Find', [BRIDGE, flags], FINDER, 8, control, returns_float=False)
        self.pending = None
        found = []
        # 원본 호출자와 같이 현재 번호(+0x34)가 0이 아닌 동안 Next를 부른다.
        for _ in range(64):
            current = struct.unpack('<I', self.mu.mem_read(FINDER + 0x34, 4))[0]
            if current == 0: break
            found.append(current)
            self.call('Next', [], FINDER, 0, control, returns_float=False)
        else:
            raise RuntimeError('이웃 순회가 끝나지 않음')
        return ','.join(map(str, found)) or '-'

    def connect(self, scene, use_list, control):
        """연결 순회 전체: 탐색·연결 객체 생성·소유자 전파를 실제 명령으로 실행한다."""
        self.prepare_scene(scene)
        args = [BRIDGE, LIST_ITEMS, LIST_COUNT] if use_list else [BRIDGE, 0, 0]
        self.call('Connect', args, 0, 0, control, returns_float=False)
        return self.observed(use_list)

    def link(self, scene, first, second, use_list, owner, control):
        """연결 객체 생성만 직접 호출한다."""
        self.prepare_scene(scene)
        args = [first, second] + ([LIST_ITEMS, LIST_COUNT] if use_list else [0, 0]) + [owner]
        self.call('Link', args, 0, 0, control, returns_float=False)
        return self.observed(use_list)

    def owner(self, scene, first, second, control):
        """소유자 전파만 직접 호출한다."""
        self.prepare_scene(scene)
        self.call('Owner', [first, second], 0, 0, control, returns_float=False)
        return self.observed(False)[::2]

    def find_at(self, scene, x, y, number, control):
        """좌표 한 칸에서 지정 타입의 첫 객체 번호를 찾는 실제 함수의 반환값."""
        self.prepare_scene(scene)
        self.call('FindAt', [x, y, number], 0, 0, control, returns_float=False)
        return self.mu.reg_read(UC_X86_REG_EAX)

    def color(self, battle, colors, owner, control):
        """소유자 색 프레임 함수의 반환값(부호 있는 DWORD)."""
        self.mu.mem_write(self.xg['battle'], struct.pack('<I', battle))
        self.mu.mem_write(self.xg['colors'], struct.pack('<10i', *colors))
        self.write_ranges = []
        self.call('Color', [owner], 0, 0, control, returns_float=False)
        return struct.unpack('<i', struct.pack('<I', self.mu.reg_read(UC_X86_REG_EAX)))[0]

    def island_owner(self, entry, scene, owner, control):
        """섬/종유석의 소유자 지정 재정의: base 호출 뒤 프레임 필드를 직접 쓰는 부분."""
        self.prepare_scene(scene)
        address, width = self.frame_field(BRIDGE)
        self.write_ranges = [(address, address + width)]
        self.call(entry, [owner], self.slot(BRIDGE), 4, control, returns_float=False)
        return [';'.join(self.events) or '-', int.from_bytes(self.mu.mem_read(address, width), 'little')]

    def island_post(self, scene, flags, control):
        """섬 받침 postPop: 최초 등록이면 종유석 생성·소유자·Pop과 실제 연결 순회를 한 뒤 공통 postPop을 부른다. 공통 함수만 호출 기록 대체다."""
        self.prepare_scene(scene)
        self.call('IslandPost', [flags], self.slot(BRIDGE), 4, control, returns_float=False)
        return self.observed(False)[:3]


def identity():
    """소유자 색 표의 실제 초기값(00531b08): 0~8은 자기 번호, 9번 칸은 1이다."""
    return [0, 1, 2, 3, 4, 5, 6, 7, 8, 1]


def node(sid, number, x, y, frame=2, level=0, state=0, extra=0):
    """탐색 실행기의 객체 입력 한 줄. 타입 플래그·발자국은 합성 타입 표에서 가져온다."""
    flags1, flags2, width, height = TYPE_FLAGS[number]
    # 좌표는 지도 안(1~254)으로 자른다. CD판 일반 탐색은 지도 밖 좌표에서 assert한다.
    x, y = min(254.0, max(1.0, x)), min(254.0, max(1.0, y))
    return (sid, number, state, extra, float_bits(x), float_bits(y), width, height, flags1, flags2, frame, level)


def scene_of(nodes, spots=(), owners=None, variant=0, bridge_global=BRIDGE_TYPE, battle=1, mission=0, colors=None):
    """장면 입력을 한 사전으로 묶는다."""
    return dict(nodes=list(nodes), spots=list(spots), owners=dict(owners or {}), variant=variant,
                bridge_global=bridge_global, battle=battle, mission=mission, colors=list(colors or identity()))


def island_group(rng, first_sid, x, y, big, owners, rich):
    """표면 한 개와 그 아래의 섬·종유석·noIsland 객체를 만든다. 번호는 first_sid부터 쓴다."""
    nodes = []
    sid = first_sid
    surface = ISLE_BIG if big else rng.choice((ISLE, NO_ISLAND))
    state = 2 if rng.random() < 0.08 else 0
    extra = rng.choice((0, 0, 0, 0, 1, 8))
    nodes.append(node(sid, surface, x, y, frame=rng.choice((2, 2, 0, 1, 10)), level=0, state=state, extra=extra))
    sid += 1
    if rich and rng.random() < 0.85:
        # 섬 받침은 표면과 같은 위치이거나 발자국이 그 칸을 덮는 옆 위치다.
        ix, iy = (x, y) if big else (x + rng.choice((0, 0, 1, 2)), y + rng.choice((0, 0, 1, 2)))
        nodes.append(node(sid, ISLAND, ix, iy, frame=rng.choice((8, 8, 0, 3)), level=rng.choice((1, 2, 3, 3)),
                          extra=rng.choice((0, 0, 0, 0, 1, 8))))
        owners[sid] = rng.choice((0, 0, 0, 0, 2, 5))
        sid += 1
        if rng.random() < 0.7:
            nodes.append(node(sid, STALAG, ix, iy, frame=8, level=rng.choice((1, 2, 3))))
            sid += 1
        # 섬 발자국 안팎의 noIsland와 다른 타입. 소유자 전파는 발자국 안의 noIsland만 바꾼다.
        for _ in range(rng.randrange(0, 4)):
            nodes.append(node(sid, rng.choice((NO_ISLAND, NO_ISLAND, ISLE, PLAIN)), ix - rng.randrange(-1, 4), iy - rng.randrange(-1, 4),
                              level=rng.choice((0, 0, 1, 2)), extra=rng.choice((0, 0, 0, 8))))
            sid += 1
    return nodes, sid


def scenes():
    """연결 순회 입력: 자기 종류·위치·소유자·네 방향의 표면/다리/비표면·섬 받침·전역을 시드 고정으로 고른다."""
    rng = random.Random(SEED)
    result = []
    # 색 표 변형: 실제 초기값과 뒤섞인 값(전투 시작 때 플레이어 색을 다시 넣는 경우).
    tables = (identity(), [0, 3, 1, 8, 2, 7, 4, 6, 5, 1], [0, 1, 1, 1, 1, 1, 1, 1, 1, 1])
    for index in range(200):
        owners = {}
        kind = rng.choice(('bridge', 'bridge', 'bridge', 'isle', 'noisland', 'islebig'))
        x, y = rng.choice(((20.0, 21.0), (20.0, 21.0), (40.0, 9.0), (20.75, 21.9), (3.0, 3.0), (250.0, 250.0)))
        if kind == 'bridge':
            nodes = [node(BRIDGE, BRIDGE_TYPE, x, y, frame=rng.choice((2, 2, 2, 2, 0, 1)), extra=rng.choice((0, 0, 0, 0, 0x10, 0x10, 1, 8, 0x80)))]
            owners[BRIDGE] = rng.randrange(1, 9)
        else:
            number = dict(isle=ISLE, noisland=NO_ISLAND, islebig=ISLE_BIG)[kind]
            nodes = [node(BRIDGE, number, x, y, frame=rng.choice((2, 0, 10)), extra=rng.choice((0, 0, 0, 1, 8)))]
            owners[BRIDGE] = rng.choice((0, 0, 3))
        sid = 60
        # 네 방향마다 없음/표면 무리/다리/비표면 가운데 하나를 둔다. 큰 표면은 발자국이 맞닿도록 놓는다.
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            choice = rng.choice(('none', 'surface', 'surface', 'big', 'bridge', 'bridge', 'plain'))
            if kind != 'bridge' and choice in ('surface', 'big') and rng.random() < 0.6: choice = 'bridge'
            nx, ny = x + dx, y + dy
            if choice == 'none' or nx < 3 or ny < 3 or nx > 252 or ny > 252: continue
            if choice in ('surface', 'big'):
                big = choice == 'big'
                if big and dx > 0: nx += 2
                if big and dy > 0: ny += 2
                group, sid = island_group(rng, sid, nx, ny, big, owners, True)
                nodes += group
            elif choice == 'bridge':
                nodes.append(node(sid, BRIDGE_TYPE, nx, ny, frame=rng.choice((2, 2, 0, 1)), state=2 if rng.random() < 0.1 else 0,
                                  extra=rng.choice((0, 0, 0, 1, 8))))
                owners[sid] = rng.randrange(1, 9)
                sid += 1
            else:
                nodes.append(node(sid, PLAIN, nx, ny, level=rng.choice((1, 2, 3))))
                sid += 1
        # 자기가 표면이면 그 아래에도 섬 받침·종유석을 둘 수 있다(다리 쪽에서 소유자가 전파되는 대상).
        if kind != 'bridge' and rng.random() < 0.8:
            nodes.append(node(sid, ISLAND, x, y, frame=rng.choice((8, 0)), level=rng.choice((1, 2, 3)), extra=rng.choice((0, 0, 0, 1))))
            owners[sid] = rng.choice((0, 0, 0, 4))
            sid += 1
            if rng.random() < 0.6:
                nodes.append(node(sid, STALAG, x, y, frame=8, level=rng.choice((1, 2, 3))))
                sid += 1
        if rng.random() < 0.25: rng.shuffle(nodes)
        spots = []
        if rng.random() < 0.08: spots.append((int(x), int(y), 8))
        if rng.random() < 0.15:
            other = rng.choice(nodes)
            ox, oy = (int(struct.unpack('<f', struct.pack('<I', other[i]))[0]) for i in (4, 5))
            spots.append((ox, oy, 8))
            if rng.random() < 0.5: spots.append((min(255, ox + 1), min(255, oy + 1), 8))
        result.append(scene_of(nodes, spots, owners, variant=rng.choice((0, 0, 1, 2)),
                               bridge_global=rng.choice((BRIDGE_TYPE, BRIDGE_TYPE, CONNECTOR)), battle=rng.choice((1, 1, 0)),
                               mission=rng.choice((0, 0, 1)), colors=rng.choice(tables)))
    # 소수 좌표 후보의 "기준점 spot" 칸: 좌표를 그대로 자른 칸과 0.9999를 더해 자른 칸 가운데 어느 쪽의 내부 비트를 보는지 가른다.
    # 네 방향의 후보 하나씩에 대해 (자른 칸만, 더해 자른 칸만, 둘 다, 없음)의 spot을 넣는다. 그 밖의 조건은 모두 통과하는 입력이다.
    for (x, y), (dx, dy), mode in itertools.product(((20.75, 21.9), (20.5, 21.25)), ((-1, 0), (1, 0), (0, -1), (0, 1)), range(4)):
        cx, cy = x + dx, y + dy
        plain, biased = (int(cx), int(cy)), (int(cx + 0.9999), int(cy + 0.9999))
        spots = [(*cell, 8) for cell, used in ((plain, mode in (0, 2)), (biased, mode in (1, 2))) if used]
        nodes = [node(BRIDGE, BRIDGE_TYPE, x, y, frame=2), node(60, ISLE, cx, cy, frame=2)]
        result.append(scene_of(nodes, spots, {BRIDGE: 3}))
    return result


def link_inputs():
    """연결 객체 생성 직접 호출: 다리 위치(소수 포함)와 상대의 종류·위치로 중심 올림·방향·한 칸 이동의 경계를 넣는다."""
    rng = random.Random(SEED ^ 0x4210f0)
    result = []
    positions = ((20.0, 21.0), (20.75, 21.9), (20.5, 21.5), (7.25, 250.0), (100.0, 100.0))
    offsets = ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, 1), (1, -1), (-1, 1), (0, 0), (-2, 1), (2, -1), (1, 2), (-1, -2),
               (3, 3), (-3, 0), (0, 3), (0.5, 0.5), (-0.5, 0.25), (0.25, -0.5), (1.5, 1.5))
    for (x, y), (dx, dy), other in itertools.product(positions, offsets, (ISLE, ISLE_BIG, NO_ISLAND, ISLAND)):
        if not (3 <= x + dx <= 252 and 3 <= y + dy <= 252): continue
        nodes = [node(BRIDGE, BRIDGE_TYPE, x, y), node(70, other, x + dx, y + dy, level=0 if other != ISLAND else 3)]
        owners = {BRIDGE: rng.randrange(0, 9), 70: rng.randrange(0, 9)}
        scene = scene_of(nodes, [], owners, bridge_global=rng.choice((BRIDGE_TYPE, BRIDGE_TYPE, CONNECTOR)))
        # 인자 순서(다리, 상대)와 뒤집은 순서를 모두 넣는다. 원본은 타입을 검사하지 않는다.
        first, second = rng.choice(((BRIDGE, 70), (BRIDGE, 70), (70, BRIDGE)))
        result.append((scene, first, second, rng.random() < 0.35, rng.choice((0, 1, 5, 8, 255))))
    return result


def owner_inputs():
    """소유자 전파 직접 호출: 섬 받침의 유무·소유자·매몰/abstract, 종유석, 발자국 안팎의 noIsland, 전투/색 표."""
    rng = random.Random(SEED ^ 0x421240)
    result = []
    for index in range(330):
        owners = {BRIDGE: rng.randrange(1, 9)}
        x, y = rng.choice(((20.0, 21.0), (20.75, 21.9), (60.0, 5.0)))
        nodes = [node(BRIDGE, BRIDGE_TYPE, x, y, extra=rng.choice((0, 0, 0, 0, 0, 0, 0, 0x10, 0x10, 0x80, 0x80, 1, 8, 9)))]
        bx, by = x + rng.choice((-1, 1, 0)), y + rng.choice((-1, 1, 0))
        if bx < 4 or by < 4: bx, by = bx + 4, by + 4
        surface = rng.choice((ISLE, ISLE_BIG, NO_ISLAND, PLAIN))
        nodes.append(node(70, surface, bx, by, level=0 if surface != PLAIN else 1))
        sid = 71
        mode = rng.choice(('island', 'island', 'island', 'island', 'two', 'none', 'far'))
        if mode in ('island', 'two', 'far'):
            # 섬 받침: 같은 칸, 발자국이 그 칸을 덮는 옆 칸, 덮지 못하는 먼 칸(far).
            ix, iy = (bx + 5, by + 5) if mode == 'far' else (bx + rng.choice((0, 0, 1, 2)), by + rng.choice((0, 0, 1, 2)))
            fraction = rng.choice((0.0, 0.0, 0.0, 0.5))
            nodes.append(node(sid, ISLAND, ix + fraction, iy + fraction, frame=rng.choice((8, 8, 0, 5)),
                              level=rng.choice((1, 2, 2, 3, 3, 3, 3, 3, 3, 0)), extra=rng.choice((0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0x80, 1, 8))))
            owners[sid] = rng.choice((0, 0, 0, 0, 0, 0, 0, 3))
            sid += 1
            if mode == 'two':
                nodes.append(node(sid, ISLAND, bx, by, frame=8, level=rng.choice((1, 2, 3))))
                owners[sid] = rng.choice((0, 6))
                sid += 1
            if rng.random() < 0.75:
                nodes.append(node(sid, STALAG, ix + rng.choice((0, 0, 0, 1)), iy + rng.choice((0, 0, 0, -1)), frame=rng.choice((8, 2)),
                                  level=rng.choice((1, 2, 3, 0))))
                sid += 1
            # 발자국(3×3) 안팎, 여러 해시 단계의 noIsland·다른 타입·매몰 객체.
            for _ in range(rng.randrange(0, 7)):
                nodes.append(node(sid, rng.choice((NO_ISLAND, NO_ISLAND, NO_ISLAND, ISLE, PLAIN, STALAG)),
                                  ix - rng.randrange(-2, 5), iy - rng.randrange(-2, 5), level=rng.choice((0, 0, 1, 2, 3)),
                                  extra=rng.choice((0, 0, 0, 8, 1)), state=rng.choice((0, 0, 0, 2))))
                owners[sid] = rng.choice((0, 0, 7))
                sid += 1
        if rng.random() < 0.3: rng.shuffle(nodes)
        scene = scene_of(nodes, [], owners, battle=rng.choice((1, 1, 1, 0)), mission=rng.choice((0, 1)),
                         colors=rng.choice((identity(), [0, 3, 1, 8, 2, 7, 4, 6, 5, 1])))
        result.append((scene, BRIDGE, 70))
    return result


def find_inputs(connect_scenes):
    """한 칸 조회: 연결 장면의 객체 위치와 그 둘레에서 섬/종유석/표면 타입을 찾는다."""
    rng = random.Random(SEED ^ 0x4b1fa0)
    result = []
    for scene in connect_scenes[:90]:
        for _ in range(3):
            target = rng.choice(scene['nodes'])
            x, y = (struct.unpack('<f', struct.pack('<I', target[i]))[0] for i in (4, 5))
            x, y = x + rng.choice((0, 0, 0, 0, -1, 1, 0.5)), y + rng.choice((0, 0, 0, 0, -1, 1, 0.5))
            # 그 자리에 실제로 있는 타입과 임의 타입을 섞어 찾음/못 찾음을 모두 넣는다.
            number = target[1] if rng.random() < 0.6 else rng.choice((ISLAND, ISLAND, STALAG, NO_ISLAND, ISLE, ISLE_BIG, BRIDGE_TYPE, PLAIN))
            result.append((scene, float_bits(x), float_bits(y), number))
    return result


def color_inputs():
    """소유자 색 프레임: 전투 여부·색 표·소유자 번호(0과 음수 포함)."""
    tables = (identity(), [0, 3, 1, 8, 2, 7, 4, 6, 5, 1], [9, -5, 0, 100, 2, 2, 2, 2, 2, 2])
    return [(battle, table, owner) for battle, table, owner in itertools.product((0, 1), tables, (-1, 0, 1, 2, 5, 8, 9))]


def island_owner_inputs():
    """섬/종유석 소유자 지정: 현재 프레임(중립 8 포함)·미션 플래그·전투 여부·색 표·소유자."""
    result = []
    for entry, frame, mission, battle, owner, table in itertools.product(
            ('IslandOwner', 'StalagOwner'), (8, 0, 3), (0, 1), (0, 1), (0, 1, 4, 8), (identity(), [0, 3, 1, 8, 2, 7, 4, 6, 5, 1])):
        number = ISLAND if entry == 'IslandOwner' else STALAG
        scene = scene_of([node(BRIDGE, number, 30.0, 30.0, frame=frame, level=3)], [], {BRIDGE: 2}, battle=battle, mission=mission, colors=table)
        result.append((entry, scene, owner))
    return result


def island_post_inputs():
    """섬 받침 postPop: 플래그(최초 등록 비트 1 포함/미포함)·소유자·위치와 발자국 둘레의 다리(연결 순회가 실제로 도는 입력)."""
    rng = random.Random(SEED ^ 0x4421c0)
    result = []
    for flags, owner, (x, y) in itertools.product((0, 1, 2, 3, 4, 5, 0x11, 0x20001, 0x2000010), (0, 3, 8), ((30.0, 30.0), (30.5, 41.25))):
        owners = {BRIDGE: owner}
        nodes = [node(BRIDGE, ISLAND, x, y, frame=8, level=3)]
        sid = 60
        # 3×3 발자국의 동/서/남/북 바깥 칸에 다리를 둔다. 프레임 A(2)는 네 방향, J(0)·K(1)는 한 축만 이어진다.
        for dx, dy in ((1, -1), (-3, 0), (-1, 1), (0, -3)):
            if rng.random() < 0.3: continue
            nodes.append(node(sid, BRIDGE_TYPE, x + dx, y + dy, frame=rng.choice((2, 2, 0, 1)), extra=rng.choice((0, 0, 0, 1))))
            owners[sid] = rng.randrange(1, 9)
            sid += 1
        if rng.random() < 0.5:
            nodes.append(node(sid, NO_ISLAND, x - 1, y - 1, level=0))
            owners[sid] = 0
        result.append((scene_of(nodes, [], owners, battle=rng.choice((1, 1, 0)), mission=rng.choice((0, 1))), flags))
    return result


def owners_text(owners):
    """소유자 입력을 TSV 한 칸에 보존한다."""
    return ';'.join(f'{sid},{owner}' for sid, owner in sorted(owners.items())) or '-'


def scene_columns(scene):
    """장면 입력 칸. 재생 쪽이 같은 순서로 읽는다: 객체, spot, 프레임 변형, 소유자, 전역(다리 타입, 전투, 미션, 색 표)."""
    return [packed(scene['nodes']), packed(scene['spots']), scene['variant'], owners_text(scene['owners']),
            ','.join(map(str, [scene['bridge_global'], scene['battle'], scene['mission'], *scene['colors']]))]


def both(run):
    """두 x87 정밀도에서 실행해 관찰이 같을 때만 돌려준다."""
    first, second = (run(control) for control in CONTROLS)
    if first != second: raise RuntimeError(f'x87 정밀도에 따라 결과가 다름: {first} / {second}')
    return first


def observe(job):
    """한 판본의 모든 입력을 실행해 (관찰 행, 장면 번호 표, 근거 기록)을 돌려준다. 판본마다 별도 프로세스에서 실행할 수 있다."""
    edition, smoke = job
    connect_scenes = scenes()
    links, owners, finds = link_inputs(), owner_inputs(), find_inputs(connect_scenes)
    colors, island_owners, island_posts = color_inputs(), island_owner_inputs(), island_post_inputs()
    if smoke:
        connect_scenes, links, owners, finds = connect_scenes[:12], links[:12], owners[:12], finds[:12]
        colors, island_owners, island_posts = colors[:6], island_owners[:8], island_posts[:6]
    rows = []
    # 같은 장면 입력은 한 번만 저장하고 관찰 행은 번호로 가리킨다. 등록 순서가 판본과 무관하므로 번호도 세 판본이 같다.
    scene_numbers = {}

    def ref(scene):
        """장면 입력 칸을 등록하고 그 번호를 돌려준다."""
        return scene_numbers.setdefault(tuple(map(str, scene_columns(scene))), len(scene_numbers))

    oracle = ConnectOracle(edition)
    # 이웃 순회: 장면마다 탐색기 플래그 0~7(flag 8의 표면 지도 경로는 대상 밖).
    for scene in connect_scenes:
        for flags in range(8):
            rows.append(['Walk', edition, ref(scene), flags, both(lambda c: oracle.walk(scene, flags, c))])
    # 연결 순회: 실제 등록 경로(목록 없음)와 미리보기 목록 경로.
    for scene in connect_scenes:
        for use_list in (0, 1):
            rows.append(['Connect', edition, ref(scene), use_list, *both(lambda c: oracle.connect(scene, use_list, c))])
    for scene, first, second, use_list, owner in links:
        rows.append(['Link', edition, ref(scene), first, second, int(use_list), owner,
                     *both(lambda c: oracle.link(scene, first, second, use_list, owner, c))])
    for scene, first, second in owners:
        rows.append(['Owner', edition, ref(scene), first, second, *both(lambda c: oracle.owner(scene, first, second, c))])
    for scene, x, y, number in finds:
        rows.append(['FindAt', edition, ref(scene), x, y, number, both(lambda c: oracle.find_at(scene, x, y, number, c))])
    for battle, table, owner in colors:
        rows.append(['Color', edition, battle, ','.join(map(str, table)), owner, both(lambda c: oracle.color(battle, table, owner, c))])
    for entry, scene, owner in island_owners:
        rows.append([entry, edition, ref(scene), owner, *both(lambda c: oracle.island_owner(entry, scene, owner, c))])
    for scene, flags in island_posts:
        rows.append(['IslandPost', edition, ref(scene), flags, *both(lambda c: oracle.island_post(scene, flags, c))])
    print(f'{edition}: {dict(collections.Counter(row[0] for row in rows))}', flush=True)
    report = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
        entries={k: f'{v:08x}' for k, v in oracle.spec['entries'].items()},
        replaced_entry_points={f'{k:08x}': v for k, v in oracle.stubs.items()},
        native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls), finder_calls=oracle.finder_calls,
        assertions=oracle.assertions, instructions=oracle.instructions)
    return rows, scene_numbers, report


def generate(smoke=False):
    """세 실제 PE와 두 x87 정밀도에서 새 fixture·SHA 근거를 만든다. 판본마다 프로세스 하나를 쓴다(결과 순서는 판본 순서로 고정)."""
    jobs = [(edition, smoke) for edition in SPECS]
    with multiprocessing.Pool(len(jobs)) as pool:
        results = pool.map(observe, jobs)
    rows = []
    reports = {}
    scene_numbers = results[0][1]
    # 판본 순서대로 합친다. 장면 번호 표는 모든 판본에서 같아야 한다.
    for (edition, _), (edition_rows, numbers, report) in zip(jobs, results):
        if numbers != scene_numbers: raise RuntimeError(f'장면 번호가 판본마다 다름: {edition}')
        rows += edition_rows
        reports[edition] = report
    if smoke:
        for row in rows[:16] + [row for row in rows if row[0] in ('Connect', 'Owner', 'Link', 'IslandPost')][:36]: print('\t'.join(map(str, row))[:300])
        return
    header = ['# 실제 PE 제한 x86 다리/섬 연결·연결 객체 생성·소유자 전파 기대값; 생성·가상 소유자 지정·Pop·프레임 지정·표면 알림은 계약 대체',
              '# 두 x87 정밀도의 관찰이 같은 입력만 저장한다. Scene 번호 객체 spot 프레임변형 소유자 전역(다리 타입,전투,미션,색 표 10칸) — 세 판본 공통 입력',
              '# Walk 판본 장면 플래그 이웃목록 | Connect 판본 장면 목록사용 사건 새슬롯 풀Adler 목록 | Link 판본 장면 a b 목록사용 소유자 사건 새슬롯 풀Adler 목록',
              '# Owner 판본 장면 a b 사건 풀Adler | FindAt 판본 장면 x y 타입 번호 | Color 판본 전투 색표 소유자 프레임',
              '# IslandOwner/StalagOwner 판본 장면 소유자 사건 프레임 | IslandPost 판본 장면 플래그 사건 새슬롯 풀Adler']
    scene_rows = [['Scene', number, *columns] for columns, number in scene_numbers.items()]
    FIXTURE.write_text('\n'.join(header) + '\n' + '\n'.join('\t'.join(map(str, row)) for row in scene_rows + rows) + '\n',
                       encoding='utf-8', newline='\n')
    paths = {Path(__file__), ROOT / 'tools/decomp_bridgeevent_oracle.py', ROOT / 'tools/decomp_neighbor_oracle.py', FIXTURE,
             ROOT / 'tools/ghidra/bridgeconnect-functions.json'}
    for edition in SPECS:
        paths.add(ROOT / reports[edition]['binary'])
        paths.update(ROOT / f'extracted/{name}/{edition}{suffix}/{part}' for name, suffix in
                     (('bridgeconnect', ''), ('bridgeevent', ''), ('graphremove', ''), ('graphremove', '/geometry'))
                     for part in ('creation.c', 'functions.tsv'))
    counts = collections.Counter(row[0] for row in rows)
    report = dict(schema=1, primary_target='10.78', method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
        unicorn_version=importlib.metadata.version('unicorn'), seed=SEED, x87_control_words=[hex(v) for v in CONTROLS],
        cases=dict(counts), total=sum(counts.values()), scenes=len(scene_rows), editions=reports, os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['external create/virtual owner/Pop/frame-set/surface-notify/base-owner/common-postPop calls are recorded substitutes',
                     'the owner substitute writes only the owner byte and the frame-set substitute writes only the frame field',
                     'synthetic type numbers with the real 10.78 flags/footprints, synthetic frame codes and hash registration',
                     'neighbor walk covers finder flags 0..7; the flag-8 surface-map iterator is out of scope',
                     'no GameWorld, Kernel scheduling, real Pop or display update'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=dict(counts), total=sum(counts.values())), ensure_ascii=False))


def verify():
    """저장된 입력/도구/fixture/몸체/PE의 SHA, 행 수, assert 0, 실제 몸체 실행을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts = collections.Counter(line.split('\t', 1)[0] for line in rows)
    if counts.pop('Scene', 0) != report['scenes']: raise RuntimeError('장면 행 수 불일치')
    if dict(counts) != report['cases'] or sum(counts.values()) != report['total']: raise RuntimeError('fixture 행 수 불일치')
    for edition, item in report['editions'].items():
        if item['assertions'] or item['stub_calls'].get('finder') or not item['finder_calls']:
            raise RuntimeError(f'원본 실행 증거 오류: {edition}')
        if not all(item['native_calls'].get(name) for name in ('Connect', 'Link', 'Owner', 'FindAt', 'Color', 'IslandOwner', 'IslandPost')):
            raise RuntimeError(f'실제 몸체 실행 누락: {edition}')
    print(f'bridgeconnect 검증 통과: {sum(counts.values())}개')


def main():
    """새 기대값 생성, 저장 결과 감사, 파일을 쓰지 않는 도구 점검 가운데 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true', help='파일을 쓰지 않고 일부 입력만 실행해 도구를 점검한다')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
