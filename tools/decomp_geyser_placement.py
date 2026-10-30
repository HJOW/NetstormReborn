#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""간헐천 일반 배치의 지형/허공·플레이어 제외 사각형을 실제 PE로 고정한다."""
import argparse
import collections
import csv
import json
import socket
import struct
from pathlib import Path
import decomp_local_geyser as base
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ROOT = base.ROOT  # 현재 저장소의 보호 원천만 읽는다.
KEYS = (*base.KEYS, 'exclude', 'land', 'void', 'rects', 'mixed', 'exPlayers', 'exists', 'alter')  # 모두 원문 호출 전에 배치하는 명시 입력이다.


def inputs():
    """지형 두 종류/혼합·제외 경계·원본 Player 관문과 생성0/음수/권한을 독립 입력한다."""
    values = []
    for seed in (0xfed, 1, 0xffffffff):  # 후보 난수의 실제 반환 좌표는 기대 모델로 계산하지 않는다.
        for spot in (0, 2, 3, 0x40, 0x42, 0x80, 0x82):  # 허용 가능한 두 지형과 거부 비트/상위 비트를 구별한다.
            for land, void in ((0, 0), (0, 1), (1, 0), (1, 1)):  # 각 허용 인수의 네 조합이다.
                values.append(case(seed=seed, spots=spot, land=land, void=void))
    for rect in range(6):  # 포함/경계/멀리/역전·다중 제외 사각형이다.
        for land, void, spot in ((0, 1, 0), (1, 0, 2), (1, 1, 0)):
            values.append(case(rects=rect, land=land, void=void, spots=spot))
    for spot in (0, 2):  # 3×3 중 한 칸만 다른 표면인 경우의 균일성 조건이다.
        values.append(case(spots=spot, land=1, void=1, mixed=1))
    for surface in (1, 2, 3):  # 현재 지면 프레임 문자 필터도 일반 모드에서 유지한다.
        values.append(case(spots=2, land=1, void=0, surfaces=surface))
    for quantity in (-1, 0, 1, 2):  # 최근 목록 초기화/생성 전 constructor 위치를 포함한다.
        for players in range(6):  # flags1/다른 flags/8명/후반 owner를 구별한다.
            for exists in (0, 1, 2):  # 실제 목록 조회의 명시 응답이다.
                values.append(case(op='P', quantity=quantity, exclude=1, land=1, void=1, exPlayers=players, exists=exists))
    for alter in (1, 2):  # 앞선 하위가 바꾼 다음 Player를 즉시 재조회하는지 검사한다.
        values.append(case(op='P', quantity=0, exclude=1, exPlayers=1, exists=1, alter=alter))
    for exclude in (0, 2, 0x80000000):  # bool 정규화 전 원본 비영 인수를 보존한다.
        values.append(case(op='P', quantity=1, exclude=exclude, land=2, void=0x80000000, exPlayers=5, exists=1))
    for boss, fort in ((0, 0), (1, 1)):
        values.append(case(op='P', boss=boss, fort=fort, exclude=1, exPlayers=3, exists=1))  # 권한/요새 조기 반환은 constructor에 진입하지 않는다.
    return list({tuple(value[key] for key in KEYS): value for value in values}.values())


def case(**changes):
    """기존 원시 입력 규격에 일반 배치 인수만 더한다."""
    value = base.case(op='T')
    value.update(exclude=0, land=0, void=1, rects=0, mixed=0, exPlayers=0, exists=0, alter=0)
    value.update(changes)
    return value


def rectangles(profile):
    """후보 finder에 직접 넣는 제외 사각형의 독립 입력이다."""
    return [[], [(30, 34, 36, 40)], [(33, 37, 33, 37)], [(50, 50, 55, 55)],
            [(0, 0, 1, 1)] * 7 + [(30, 34, 36, 40)], [(36, 40, 30, 34)]][profile]


class Oracle(base.Oracle):
    """현재 PC의 전체 정밀 범위로 기존 후보/생성 명령을 빌린다."""
    def __init__(self):
        """과거 PC의 선택 export 대신 현재 전체 정밀 표를 사용한다."""
        pe = base.common.pefile.PE(str(base.common.BINARY))
        self.data = pe.get_memory_mapped_image()
        self.base = pe.OPTIONAL_HEADER.ImageBase
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(self.base, (len(self.data) + 4095) & ~4095)
        self.mu.mem_write(self.base, self.data)
        for address, size in ((base.POOL, 0x10000), (base.TYPES, 0x20000), (base.PLAYERS, 0x1000), (base.SPOTS, 0x10000), (base.RANDOM, 0x1000), (base.FINDER, 0x1000), (base.WORDS, 0x20000), (base.THREAD, 0x1000), (base.STOP, 0x1000), (base.common.STACK_BASE, 0x10000)):  # 원본 이미지와 분리한 각 입력 영역이다.
            self.mu.mem_map(address, size)
        self.paths = [ROOT / 'extracted/refined/originals/functions.tsv']
        with self.paths[0].open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        self.entries = {}
        for entry in (0x486360, 0x486130, 0x486030, 0x485890, 0x4857d0, 0x41daf0, 0x43fa60, 0x4bca20, 0x4e49c0, 0x4e62a0, 0x4bcab0, 0x49a840):  # 직접 계산하는 원본 몸체만 허용한다.
            self.entries[entry] = f'{entry:08x}'
            for part in facts[entry]['ranges'].split(';'):  # 떨어진 함수 조각도 실제 표대로 포함한다.
                low, high = (int(item, 16) for item in part.split('-'))
                self.allowed.append((low, high + 1))
        self.allowed.extend([(0x4ad680, 0x4ad6be), (0x4e65ed, 0x4e6656), (0x4ef510, 0x4ef55e)])
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.native = collections.Counter()
        self.boundaries = collections.Counter()
        self.returns = 0

    def prepare_policy(self, populate):
        """최상위 진입 전에 호출 인수/명시 raw Player/spot만 설치한다."""
        value = self.c
        if value['mixed']:
            self.put(base.SPOTS + 35 * 256 + 31, 0 if value['spots'] == 2 else 2, 1)
        if populate:
            esp = self.mu.reg_read(UC_X86_REG_ESP)
            for offset, key in ((28, 'exclude'), (32, 'land'), (36, 'void')):  # 원본의 마지막 세 cdecl 인수다.
                self.put(esp + offset, value[key])
            for owner in range(1, 9):  # 프로필별 raw flags/region을 입력하며 원문 제외 결과를 미리 계산하지 않는다.
                profile = value['exPlayers']
                enabled = (profile == 1 and owner == 1) or profile == 3 or (profile == 4 and owner in (1, 2, 3)) or (profile == 5 and owner == 4)
                flags = 1 if enabled else (2 if profile == 2 and owner == 1 else 32 if profile == 2 and owner == 2 else 0)
                self.put(base.PLAYERS + owner * 180 + 0x50, flags, 1)
                self.put(base.PLAYERS + owner * 180 + 0x78, owner if enabled else 127)
        else:
            self.put(base.FINDER + 0xa4, value['land'])
            self.put(base.FINDER + 0xa8, value['void'])
            areas = rectangles(value['rects'])
            self.put(base.FINDER + 0x10, len(areas))
            for index, rectangle in enumerate(areas):  # 제외 사각형 자체만 입력하며 후보점은 만들지 않는다.
                self.mu.mem_write(base.FINDER + 0x14 + index * 16, struct.pack('<4f', *rectangle))

    def instruction(self, mu, address, size, data):
        """최상위 입력과 실제 IslandList 경계만 추가하며 후보/반복은 기존 기계어다."""
        if not self.prepared and address == (0x486130 if self.c['op'] == 'P' else 0x485890):
            self.prepared = True
            self.prepare_policy(self.c['op'] == 'P')
        if address == 0x46ee30:
            region = self.get(mu.reg_read(UC_X86_REG_ESP) + 4)
            self.trace.append('Exists:' + str(region))
            if self.c['alter'] == 1 and region == 1:
                self.put(base.PLAYERS + 2 * 180 + 0x50, 1, 1)
                self.put(base.PLAYERS + 2 * 180 + 0x78, 2)
            self.ret(int(region < 9 and (self.c['exists'] == 1 or (self.c['exists'] == 2 and region % 2))), 4)
            return
        if address == 0x48f5c0 and self.c['alter'] == 2:
            self.put(base.PLAYERS + 3 * 180 + 0x50, 1, 1)
            self.put(base.PLAYERS + 3 * 180 + 0x78, 3)
        return super().instruction(mu, address, size, data)

    def run(self, value):
        """각 원문 호출의 실제 상태와 효과 순서를 반환한다."""
        self.prepared = False
        result = super().run(value)
        return [*result, bytes(self.mu.mem_read(base.PLAYERS, 1620)).hex()]


def main():
    """독립 두 emulator의 최초 출력을 비교하고 새 fixture만 생성한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    plan = inputs()
    first, second = Oracle(), Oracle()
    rows = [[*[str(value[key]) for key in KEYS], *map(str, first.run(value))] for value in plan]  # 원문 결과만 기대값으로 저장한다.
    replay = [[*[str(value[key]) for key in KEYS], *map(str, second.run(value))] for value in plan]
    for name, observed in (('first.json', rows), ('replay.json', replay)):  # 독립 원시 결과를 둘 다 보존한다.
        (output / name).write_text(json.dumps(observed, indent=2) + '\n', encoding='utf-8', newline='\n')
    assert rows == replay
    fixture = ROOT / 'cpppj/tests/fixtures/geyser-placement-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# ' + '/'.join(KEYS) + '/실제원문11출력\n')
        stream.writelines('\t'.join(row) + '\n' for row in rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_local_geyser.py', 'tools/decomp_geyser_placement.py', 'cpppj/tests/fixtures/geyser-placement-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'cases': len(rows), 'observations': len(rows) * 2, 'differences': [], 'gameStarted': False,
              'scope': '00486030/004857d0/00485890/00486130과 기존 난수/CRT 실제 명령. IslandList/Player bounds/typedFind/생성·삭제/Pop/건설확정은 명시 경계. 게임/포획/희생/공격/승리/MP 없음.', 'sourceSha256': {path: base.common.digest(ROOT / path) for path in paths}, 'evidenceSha256': {path.name: base.common.digest(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/geyser-placement-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': len(rows), 'observations': len(rows) * 2, 'differences': 0}))


if __name__ == '__main__':
    main()
