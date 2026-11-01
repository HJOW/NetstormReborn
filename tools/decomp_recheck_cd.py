#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""RecheckCD 일반 입력(4d8b8d)의 현재 serial 조회·Kill·0쓰기를 제품 변경 전에 동결한다.

실제 004d62b0 명령 분기, 00435060, 004c8d90, 00464020의 기계어를 그대로 실행하고
00466480(공통 Kill)만 명시 경계로 둔다. 단위696에서 추가했다.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import ROOT
from decomp_game_menu_actions import ARENA, Oracle as MenuOracle, cases
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX

COMMAND = 0x020000fd  # 00543370 이름표가 가리키는 00542ae8 초기 명령이다.
SERIAL = 0x5c85ec  # 4d8b94가 4c8d90에 넘기는 현재 serial DWORD 주소이며 바이너리 전체의 유일한 직접 참조다.
TABLE = 0x563e3c  # 464020이 앞에서부터 훑는 등록 표의 첫 칸이다.
TABLE_END = 0x564dd8  # 464020의 반복 종료 비교값이며 이 주소 이상의 칸은 조회하지 않는다.
OBJECTS = ARENA + 0x800  # 명시 Gump 객체를 놓는 장치 영역이다. 객체 한 개는 0x40바이트다.
OBJECT_SIZE = 0x40  # 객체 간격이다. +4는 플래그, +0x2c는 serial이다.
NATIVE = ((0x435060, 0x435066), (0x4c8d90, 0x4c8db3), (0x464020, 0x464052))  # 실제로 실행하는 호출 본체 범위다.
KILL = 0x466480  # 공통 Kill은 thiscall이며 ECX의 객체를 받는 명시 경계다.

# (이름, 객체 목록[칸, serial, 플래그]) — 칸은 0부터 시작하는 등록 표 위치이며 객체 번호는 목록 순서다.
LAYOUTS = (
    ('empty', ()),
    ('live7@0', ((0, 7, 0),)),
    ('dead7@0', ((0, 7, 0x10000),)),
    ('dead7@0+live7@5', ((0, 7, 0x10000), (5, 7, 0))),
    ('live7@3+live7@9', ((3, 7, 0), (9, 7, 0))),
    ('live7@2+live0@4', ((2, 7, 0), (4, 0, 0))),
    ('dead0@1+live0@6', ((1, 0, 0x10000), (6, 0, 0))),
    ('live7@998', ((998, 7, 0),)),
    ('live7@999', ((999, 7, 0),)),
    ('flags7@1', ((1, 7, 0xfffeffff),)),
    ('liveffffffff@2', ((2, 0xffffffff, 0),)),
)
SERIALS = (0, 7, 0xffffffff, 0x12345)  # 서로 다른 현재 serial 입력이다.


def layout_text(objects):
    """C++ 검사가 같은 등록 표를 다시 만들 수 있는 칸:serial:플래그 표기다."""
    return ','.join(f'{slot}:{serial}:{flags}' for slot, serial, flags in objects) or '-'


class Oracle(MenuOracle):
    """일반 입력 분기와 serial 조회/해제 본체는 실행하고 공통 Kill만 명시 경계로 둔다."""

    def __init__(self):
        """실제 호출 본체 범위가 실행 중 바뀌지 않았는지 확인할 원문 바이트를 보관한다."""
        super().__init__()
        self.native = {low: bytes(self.mu.mem_read(low, high - low)) for low, high in NATIVE}  # 보호 본체 불변 근거다.
        self.serial_writes = 0  # 이번 실행에서 5c85ec에 쓴 횟수다.
        self.objects = ()  # 이번 실행에 설치한 객체 목록이다.

    def guard(self, mu, access, address, size, value, data):
        """기존 허용 범위에 더해 현재 serial 한 DWORD의 쓰기만 허용한다."""
        if address == SERIAL and size == 4:
            self.serial_writes += 1
            self.writes['serial'] += 1
            return
        super().guard(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """실제 본체는 그대로 실행하고 466480 호출만 대상 객체 번호로 기록한다."""
        if address == KILL:
            target = mu.reg_read(UC_X86_REG_ECX)  # thiscall의 this이며 464020이 찾은 객체 주소다.
            index = (target - OBJECTS) // OBJECT_SIZE  # 설치 순서의 객체 번호다.
            if target != OBJECTS + index * OBJECT_SIZE or not 0 <= index < len(self.objects):
                raise RuntimeError(f'미설치 객체 Kill {target:08x}')
            self.body_event = True
            self.event('kill', (index,))
            self.returned()
            return
        if any(low <= address < high for low, high in NATIVE):
            self.instructions += 1
            self.visited.add(address)
            return
        super().instruction(mu, address, size, data)

    def install(self, serial, objects):
        """이전 입력의 등록 표/객체를 모두 지운 뒤 현재 serial과 객체를 설치한다."""
        self.mu.mem_write(TABLE, bytes(TABLE_END - TABLE + 8))  # 범위 밖 한 칸(999)까지 지운다.
        self.mu.mem_write(OBJECTS, bytes(0x800))
        self.objects = objects
        for index, (slot, object_serial, flags) in enumerate(objects):  # 칸마다 명시 객체를 가리킨다.
            base = OBJECTS + index * OBJECT_SIZE
            self.number(base + 4, flags)
            self.number(base + 0x2c, object_serial)
            self.number(TABLE + slot * 4, base)
        self.number(SERIAL, serial)
        self.serial_writes = 0

    def observe(self, case, control, serial, objects):
        """입력을 설치하고 실제 분기를 실행한 뒤 serial 최종값과 쓰기 횟수를 덧붙인다."""
        self.install(serial, objects)
        result = self.run(case, control)
        result['serial'] = self.number(SERIAL)
        result['serialwrites'] = self.serial_writes
        for low, expected in self.native.items():  # 호출 본체가 바뀌지 않았음을 매 입력 뒤 확인한다.
            if bytes(self.mu.mem_read(low, len(expected))) != expected:
                raise RuntimeError('보호 호출 본체 변경')
        return result


def main():
    """두 정밀도와 반복으로 입력을 설치하고 독립 결과만 새 폴더에 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    oracle = Oracle()
    seed = cases()[0]
    plan = []  # (이벤트 코드, 첫째, 둘째, 모드, serial, 배치) 입력 목록이다.
    for serial, (name, objects) in itertools.product(SERIALS, LAYOUTS):  # 모든 serial/배치 조합은 기본 입력으로 본다.
        plan.append((COMMAND, 0, 99, 0, serial, objects))
    for (name, objects), serial in ((LAYOUTS[1], 7), (LAYOUTS[5], 0), (LAYOUTS[0], 7)):  # 입력 변형은 대표 배치에서만 전수로 본다.
        for flags, first, second, mode in itertools.product((0, 0x80000000, 0x38000000, 0xb8000000), (0, 305419896), (0, 1, 0xffffffff), range(4)):  # 뗌/수정키 4종×첫째 인자 2종×둘째 인자 3종×모드 4종을 모두 만든다.
            plan.append((COMMAND | flags, first, second, mode, serial, objects))
    rows, observations = [], []
    for index, (event, first, second, mode, serial, objects) in enumerate(plan):  # 계획한 입력마다 두 정밀도와 반복으로 관측하고 결과 행을 모은다.
        values = list(map(int, seed['globals'].split(':')))
        values[0], values[3] = mode & 1, (mode >> 1) & 1  # Editor와 missionPost 모드를 독립으로 바꾼다.
        values[8] = values[9] = 0
        case = seed.copy()
        case.update(name=f'RecheckCD-{index}', event=f'{event}:{first}:{second}', globals=':'.join(map(str, values)),
                    guideResult=0, gumpResult=0, read=0, filled=0, mutation='-', stop='-')
        results = []
        for control in (0x27f, 0x37f, 0x27f):  # 매번 전체 입력을 다시 설치한다.
            results.append(oracle.observe(case, control, serial, objects))
        assert results[0] == results[1] == results[2]
        result = results[0]
        rows.append((event, first, second, mode, serial, layout_text(objects), result['bodytrace'], result['serial'],
                     result['serialwrites'], result['completed'], result['stackdelta']))
        observations.append(dict(input=case, serial=serial, objects=objects, observations=results))
    fixture = out / 'recheck-cd-x86.tsv'
    schema = ('event', 'first', 'second', 'mode', 'serial', 'layout', 'trace', 'serialout', 'serialwrites', 'completed', 'stackdelta')
    fixture.write_text('\t'.join(schema) + '\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows), encoding='utf-8', newline='\n')
    (out / 'observations.json').write_text(json.dumps(observations, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    report = dict(unit=696, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  cases=len(rows), observations=len(rows) * 3, differences=0, fixtures={fixture.name: hashlib.sha256(fixture.read_bytes()).hexdigest()},
                  boundaries='실제 004d62b0 명령 분기와 00435060/004c8d90/00464020 본체를 실행했다. 00466480(공통 Kill)만 대상 객체 번호를 기록하는 명시 경계다. Client/OS/파일/게임/통신/MP 실행0.',
                  sha256={path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe', 'tools/decomp_recheck_cd.py', 'tools/decomp_game_menu_actions.py', 'tools/decomp_production_shortcuts.py')})
    (out / 'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=len(rows), observations=len(rows) * 3, differences=0)))


if __name__ == '__main__':
    main()
