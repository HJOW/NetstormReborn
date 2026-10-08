#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""섬 받침의 preDestroy와 noIsland의 postDestroy 재정의를 세 실제 PE의 제한 x86으로 대조한다.

실제 명령으로 실행하는 것: 섬 받침 preDestroy(00442240 ↔ CD 004d0250)의 중심 계산·flag 0 연결 이웃 순회·다리/dead 판정,
noIsland postDestroy(00442640 ↔ CD 004d2a40)의 한 칸 일반 탐색(0단계 건너뜀)·walker 판정, 패치판의 지연 낙하 넘김 함수(00421f90)와
한 칸 탐색기 생성자(004202f0). 탐색·필터·기하는 대체하지 않는다.
명시적으로 대체하는 것(호출 사실과 인자만 기록): 지연 낙하 예약(00421530 ↔ CD 004490b0), 가상 walker 낙하(+0xc8),
공통 preDestroy(004b0950 ↔ CD 004add20), 공통 postDestroy(004b0840 ↔ CD 004adbe0).
원본 게임/OS/업데이터는 실행하지 않는다.

python -X utf8 tools/decomp_islandlifecycle_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_islandlifecycle_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
"""
import argparse
import collections
import csv
import importlib.metadata
import itertools
import json
import random
import struct
from pathlib import Path

from decomp_bridgeevent_oracle import ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE, VTABLE, STOP, digest
from decomp_bridgeconnect_oracle import (ConnectOracle, ISLE, ISLE_BIG, PLAIN, ISLAND, STALAG, NO_ISLAND, node, scene_of,
    scene_columns)
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 대체 가상 walker 낙하(+0xc8)의 진입점.
WALKER_FALL = STOP + 0x160
# 입력만 만드는 고정 시드. 기대 결과는 원본 기계어가 정한다.
SEED = 0x442240
# 판본별 진입점·대체 진입점·전역. fall은 패치판이 thiscall(00421530), CD가 cdecl(004490b0)이다.
EXTRA = {
    'originals': dict(entries=dict(IslandPre=0x442240, SurfacePost=0x442640),
        stubs={0x421530: 'fall_this', 0x4b0950: 'basepre', 0x4b0840: 'basepost_destroy'}, debug_check=0x5e4794),
    'originalCD': dict(entries=dict(IslandPre=0x4d0250, SurfacePost=0x4d2a40),
        stubs={0x4490b0: 'fall_cdecl', 0x4add20: 'basepre', 0x4adbe0: 'basepost_destroy'}, debug_check=None),
}
EXTRA['original1037'] = EXTRA['originalCD']
# 이 도구가 직접 처리하는 대체 이름.
HANDLED = ('fall_this', 'fall_cdecl', 'basepre', 'basepost_destroy', 'walker')
# 새 결과 파일은 이전 기록/fixture와 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/islandlifecycle-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-islandlifecycle-evidence.json'


class IslandOracle(ConnectOracle):
    """연결 실행기의 장면 준비를 그대로 쓰고 삭제 훅 몸체와 그 외부 효과 대체를 더한다."""
    def __init__(self, edition):
        """새 내보내기의 몸체를 허용하고 대체 진입점과 합성 가상 표의 +0xc8을 연결한다."""
        super().__init__(edition)
        extra = EXTRA[edition]
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **extra['entries']))
        self.stubs.update(extra['stubs'])
        self.stubs[WALKER_FALL] = 'walker'
        self.debug_check = extra['debug_check']
        self.mu.mem_write(VTABLE + 0xc8, struct.pack('<I', WALKER_FALL))
        paths = [ROOT / f'extracted/islandlifecycle/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def on_instruction(self, mu, address, size, data):
        """삭제 훅의 외부 효과 진입점은 사건을 기록하고 반환한다. 그 밖에는 부모의 처리를 따른다."""
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
        if name == 'fall_this':
            # 패치판 00421530(this = 다리, x, y): 지연 낙하 예약.
            self.events.append(f'G:{self.sid_of(ecx)}:{a0}:{a1}')
            purge = 8
        elif name == 'fall_cdecl':
            # CD 004490b0(다리 번호, x, y).
            self.events.append(f'G:{a0}:{a1}:{a2}')
        elif name == 'walker':
            self.events.append(f'W:{self.sid_of(ecx)}')
        else:
            self.events.append(f'{"X" if name == "basepre" else "Y"}:{self.sid_of(ecx)}:{a0}')
            purge = 4
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def hook(self, entry, scene, flags, debug, control):
        """삭제 훅 한 번의 사건 순서와 인자를 돌려준다. 원본 몸체는 풀을 쓰지 않는다."""
        self.prepare_scene(scene)
        self.write_ranges = []
        if self.debug_check is not None:
            self.mu.mem_write(self.debug_check, struct.pack('<I', debug))
        self.call(entry, [flags], self.slot(BRIDGE), 4, control, returns_float=False)
        # 부모 실행기가 적는 탐색기 관찰(F:/I:)은 빼고 외부 효과만 남긴다.
        return ';'.join(event for event in self.events if not event.startswith(('F:', 'I:'))) or '-'


def island_inputs():
    """섬 받침 preDestroy: 받침의 extra·flags(0x1000 포함)와 발자국 둘레의 다리(죽은 것·abstract·한 축 프레임)·표면·내부 spot."""
    rng = random.Random(SEED)
    result = []
    for index in range(260):
        x, y = rng.choice(((30.0, 30.0), (30.0, 30.0), (30.5, 41.25), (60.0, 9.0)))
        owners = {BRIDGE: rng.choice((0, 3))}
        nodes = [node(BRIDGE, ISLAND, x, y, frame=rng.choice((8, 0, 2)), level=rng.choice((1, 2, 3)),
                      extra=rng.choice((0, 0, 0, 0, 0, 0x10, 0x80, 1, 8)))]
        sid = 60
        # 3×3 발자국의 바깥 칸(동·서·남·북의 여러 자리)에 다리나 표면을 둔다.
        for dx, dy in ((1, 0), (1, -1), (1, -2), (-3, 0), (-3, -1), (0, 1), (-1, 1), (-2, 1), (0, -3), (-2, -3), (2, 0), (1, 1)):
            choice = rng.choice(('none', 'none', 'bridge', 'bridge', 'isle', 'plain'))
            if choice == 'none': continue
            if choice == 'bridge':
                nodes.append(node(sid, BRIDGE_TYPE, x + dx, y + dy, frame=rng.choice((2, 2, 2, 0, 1)),
                                  state=rng.choice((0, 0, 0, 0, 2)), extra=rng.choice((0, 0, 0, 0, 1, 8))))
                owners[sid] = rng.randrange(1, 9)
            elif choice == 'isle':
                nodes.append(node(sid, ISLE, x + dx, y + dy, frame=2, state=rng.choice((0, 0, 2))))
            else:
                nodes.append(node(sid, PLAIN, x + dx, y + dy, level=rng.choice((1, 2, 3))))
            sid += 1
        spots = []
        if rng.random() < 0.06:
            # 발자국 전체가 내부이면 탐색하지 않는다.
            spots = [(int(x) - i, int(y) - j, 8) for i in range(3) for j in range(3)]
        elif rng.random() < 0.15 and len(nodes) > 1:
            other = rng.choice(nodes[1:])
            ox, oy = (int(struct.unpack('<f', struct.pack('<I', other[i]))[0]) for i in (4, 5))
            spots = [(ox, oy, 8)]
        if rng.random() < 0.25: rng.shuffle(nodes)
        flags = rng.choice((0, 0, 0, 1, 0x1000, 0x1000, 0x1001, 0x20000, 0x800, 0xffffefff, 0xffffffff))
        result.append((scene_of(nodes, spots, owners, variant=rng.choice((0, 0, 1, 2))), flags, rng.choice((0, 1))))
    return result


def surface_inputs():
    """noIsland postDestroy: 표면 칸의 extra·위치(소수 포함)와 같은 칸/옆 칸의 walker·비 walker·매몰 객체를 여러 해시 단계에 둔다."""
    rng = random.Random(SEED ^ 0x442640)
    result = []
    for index in range(260):
        x, y = rng.choice(((30.0, 30.0), (30.0, 30.0), (30.75, 41.25), (5.0, 250.0)))
        nodes = [node(BRIDGE, NO_ISLAND, x, y, level=0, extra=rng.choice((0, 0, 0, 0, 0, 0x10, 0x80, 1, 8, 9)))]
        sid = 60
        for _ in range(rng.randrange(0, 8)):
            kind = rng.choice((PLAIN, PLAIN, PLAIN, ISLE, STALAG, ISLAND, ISLE_BIG))
            # 같은 칸, 소수만 다른 같은 칸, 옆 칸. 큰 발자국은 옆 자리에서도 그 칸을 덮는다.
            ox, oy = rng.choice(((0, 0), (0, 0), (0, 0), (0.25, 0.5), (1, 0), (0, 1), (-1, 0), (2, 2), (1, 1)))
            nodes.append(node(sid, kind, x + ox, y + oy, level=rng.choice((0, 1, 1, 2, 3)), state=rng.choice((0, 0, 0, 2)),
                              extra=rng.choice((0, 0, 0, 0, 1, 8))))
            sid += 1
        if rng.random() < 0.25: rng.shuffle(nodes)
        flags = rng.choice((0, 0, 1, 0x1000, 0x20000, 0xffffffff))
        result.append((scene_of(nodes, [], {}), flags, 0))
    return result


def both(run):
    """두 x87 정밀도에서 실행해 관찰이 같을 때만 돌려준다."""
    first, second = (run(control) for control in CONTROLS)
    if first != second: raise RuntimeError(f'x87 정밀도에 따라 결과가 다름: {first} / {second}')
    return first


def generate(smoke=False):
    """세 실제 PE와 두 x87 정밀도에서 새 fixture·SHA 근거를 만든다."""
    islands, surfaces = island_inputs(), surface_inputs()
    if smoke: islands, surfaces = islands[:14], surfaces[:14]
    rows = []
    reports = {}
    scene_numbers = {}
    paths = {Path(__file__), ROOT / 'tools/decomp_bridgeevent_oracle.py', ROOT / 'tools/decomp_neighbor_oracle.py',
             ROOT / 'tools/decomp_bridgeconnect_oracle.py', FIXTURE, ROOT / 'tools/ghidra/islandlifecycle-functions.json'}

    def ref(scene):
        """장면 입력 칸을 등록하고 그 번호를 돌려준다."""
        return scene_numbers.setdefault(tuple(map(str, scene_columns(scene))), len(scene_numbers))

    for edition in SPECS:
        oracle = IslandOracle(edition)
        for kind, cases in (('IslandPre', islands), ('SurfacePost', surfaces)):
            for scene, flags, debug in cases:
                rows.append([kind, edition, ref(scene), flags, debug, both(lambda c: oracle.hook(kind, scene, flags, debug, c))])
        print(f'{edition}: {dict(collections.Counter(row[0] for row in rows if row[1] == edition))}', flush=True)
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
            entries={k: f'{v:08x}' for k, v in oracle.spec['entries'].items()},
            replaced_entry_points={f'{k:08x}': v for k, v in oracle.stubs.items()},
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls), finder_calls=oracle.finder_calls,
            assertions=oracle.assertions, instructions=oracle.instructions)
        paths.add(ROOT / oracle.spec['binary'])
        paths.update(oracle.exports)
    if smoke:
        for row in rows[:28]: print('\t'.join(map(str, row)))
        return
    header = ['# 실제 PE 제한 x86 섬 받침 preDestroy·noIsland postDestroy 기대값; 지연 낙하 예약(G)·walker 낙하(W)·공통 pre/postDestroy(X/Y)는 호출 기록 대체',
              '# 두 x87 정밀도의 관찰이 같은 입력만 저장한다. Scene 번호 객체 spot 프레임변형 소유자 전역 — 세 판본 공통 입력(형식은 bridgeconnect와 같다)',
              '# IslandPre/SurfacePost 판본 장면 flags 디버그검사 사건']
    scene_rows = [['Scene', number, *columns] for columns, number in scene_numbers.items()]
    FIXTURE.write_text('\n'.join(header) + '\n' + '\n'.join('\t'.join(map(str, row)) for row in scene_rows + rows) + '\n',
                       encoding='utf-8', newline='\n')
    counts = collections.Counter(row[0] for row in rows)
    report = dict(schema=1, primary_target='10.78', method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
        unicorn_version=importlib.metadata.version('unicorn'), seed=SEED, x87_control_words=[hex(v) for v in CONTROLS],
        cases=dict(counts), total=sum(counts.values()), scenes=len(scene_rows), editions=reports, os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['delayed-fall scheduling, virtual walker fall and common pre/postDestroy are recorded substitutes',
                     'synthetic type numbers with the real 10.78 flags/footprints, synthetic frame codes and hash registration',
                     'no GameWorld, Kernel scheduling or real deletion'])
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
        if not item['native_calls'].get('IslandPre') or not item['native_calls'].get('SurfacePost'):
            raise RuntimeError(f'실제 몸체 실행 누락: {edition}')
    print(f'islandlifecycle 검증 통과: {sum(counts.values())}개')


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
