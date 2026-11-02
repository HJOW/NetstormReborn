#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DrawWater 전체 표시·실제 영역 finder/투영/외곽선/VFX를 work의 새 fixture로만 관측한다."""
from pathlib import Path
import argparse
import collections
import csv
import hashlib
import itertools
import json
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_rawfinder_oracle import FinderOracle
from decomp_bridgedecay_oracle import POOL, HEAP, TYPES, STACK_BASE, STACK, STOP, CAPACITY
from decomp_minimap_blit import changed
from unicorn.x86_const import *

CANVAS, SURFACE, GDT = 0x12500100, 0x12600000, 0x40000000  # 작은 분리8비트 표면과 평면 세그먼트다.
WIDTH, HEIGHT, PITCH = 96, 80, 100  # 실제 Screen 폭과 행 padding을 구별한다.
VIEW = HEAP + 0xe000  # snapshot과 별개인 실제 Renderer 앞64BYTE 입력이다.
ENTRIES = (0x434dd0, 0x497530, 0x497220, 0x41def0, 0x41d770, 0x41d7a0, 0x4e49c0, 0x4a1580, 0x4a2270, 0x4a46d0, 0x4dade0, 0x4011f1)  # 대체 없이 실행하는 추가 원문 몸체다.


def bits(value):
    """독립 단정도 입력의 실제 DWORD만 만든다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def signed(value):
    """원문 DWORD를 같은 signed32 비트로 표시한다."""
    return struct.unpack('<i', struct.pack('<I', value & 0xffffffff))[0]


class Oracle(FinderOracle):
    """실제 finder·viewport·투영·rect/VFX를 실행하며 화면 장치 잠금/출력만 명시한다."""
    def __init__(self):
        """기존 실제 finder를 재사용하고 새 몸체/픽셀 쓰기 범위만 더한다."""
        super().__init__('originals')
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], DrawWater=0x434dd0))
        self.mu.mem_map(CANVAS & ~4095, 0x10000)
        self.mu.mem_map(SURFACE, 0x1000)
        self.mu.mem_map(GDT, 0x1000)
        self.mu.mem_write(GDT, struct.pack('<QQQ', 0, 0x00cf9a000000ffff, 0x00cf92000000ffff))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 23, 0))
        self.mu.reg_write(UC_X86_REG_CS, 8)
        for register in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES):  # VFX push/pop ES도 실제32BIT 스택을 사용한다.
            self.mu.reg_write(register, 16)
        facts = {int(r['entry'], 16): r for r in csv.DictReader((ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        for entry in ENTRIES:  # 현재 호스트 정밀 함수표의 불연속 몸체만 더한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(v, 16) for v in part.split('-'))
                self.allowed[low - self.base:high + 1 - self.base] = b'\1' * (high + 1 - low)
        self.calls = collections.Counter()

    def number(self, address, value=None):
        """장치/원문 전역의 같은 DWORD를 읽고 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def done(self, value=0, purge=0):
        """명시 외부 장치의 원래 cdecl/thiscall 반환 폭만 처리한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.number(esp))
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)

    def clip(self):
        """현재 exclusive clip을 Native 필드에서 그대로 읽는다."""
        return tuple(signed(self.number(0x5c7a44 + i * 4)) for i in range(4))

    def on_write(self, mu, access, address, size, value, data):
        """현재 stack/clip/lock/FILE 표면 밖의 게임/TYPE/해시 쓰기는 거부한다."""
        if any(low <= address and address + size <= high for low, high in ((STACK_BASE, STACK_BASE + 0x40000), (CANVAS, CANVAS + PITCH * HEIGHT), (0x5c7a44, 0x5c7a54), (0x5b5de0, 0x5b5df0), (0x5c7920, 0x5c7924), (0x5c78ec, 0x5c78f0), (0x5c78f8, 0x5c78fc), (0x59af88, 0x59af8c))):
            return
        raise RuntimeError(f'DrawWater 미명시 쓰기 {address:08x}/{size}')

    def on_instruction(self, mu, address, size, data):
        """픽셀/선/클립의 계산은 원문에 맡기고 하위 화면 장치만 관측한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = tuple(self.number(esp + 4 + i * 4) for i in range(5))
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x41def0:
            self.events.append('Begin:' + ':'.join(map(str, args)))
        if address == 0x4a1580:
            self.events.append('Clip:' + ':'.join(str(signed(v)) for v in args[:4]))
        if address == 0x4a2270:
            self.events.append('Rect:' + ':'.join(str(signed(v)) for v in args[:4]) + ':' + str(args[4]))
            self.rects += 1
            if self.rects == 1:
                if self.mutation == 1:
                    self.number(0x5ad128, 0x123456a1)
                elif self.mutation == 2:
                    self.number(0x59a908, 7)
                    self.number(0x59a90c, 13)
                elif self.mutation == 3:
                    self.number(0x54d930, 0)
                elif self.mutation == 4:
                    self.mu.mem_write(self.slot(63) + self.o['extra'], b'\x08')
        if address == 0x4a14e0:
            self.events.append('Lock:' + ':'.join(map(str, self.clip())))
            self.number(0x5c7920, self.number(0x5c7920) + 1)
            self.number(0x5c7948, SURFACE)
            self.number(0x59af88, CANVAS)
            self.number(0x5c78f8, CANVAS)
            self.number(0x5c78ec, PITCH)
            self.done()
            return
        if address == 0x4a0950:
            self.events.append('Unlock:' + ':'.join(map(str, self.clip())))
            self.done()
            return
        if address == 0x4a1800:
            self.events.append('Present:' + ':'.join(str(signed(v)) for v in args[:4]))
            self.mu.mem_write(0x5c7a44, struct.pack('<4i', 0, 0, WIDTH, HEIGHT))
            self.done()
            return
        super().on_instruction(mu, address, size, data)

    def run(self, case, control):
        """현재 풀/해시와 독립 Renderer/snapshot을 설치하고 실제 전체 본문을 정상RET까지 실행한다."""
        active, cell_x, cell_y, origin_x, origin_y, camera_x, camera_y, self.mutation, scene = case
        objects = []
        for i, sid in enumerate((50, 60, 61, 62, 63, 64)):  # 일반 합성 객체만 사용하며 금지된 공중3종/전용 생성은 실행하지 않는다.
            x, y = ((20 + i % 3 + .25, 20 + i // 3 + .5) if scene != 2 else (100 + i, 110 + i))
            objects.append((sid, 100 + i, (0, 1, 2, 4, 8, 6)[i], 8 if scene == 1 and i % 2 else 0, bits(x), bits(y), 1 + i % 3, 1 + i % 2, 0, i % 4))
        self.setup(objects)
        self.mu.mem_write(VIEW, bytes(64))
        self.mu.mem_write(VIEW, struct.pack('<2f2i', origin_x, origin_y, cell_x, cell_y))
        self.number(0x59a8bc, VIEW)
        self.mu.mem_write(0x59a900, bytes(64))
        self.mu.mem_write(0x59a908, struct.pack('<2i', cell_x, cell_y))
        self.mu.mem_write(0x59a91c, struct.pack('<2i', camera_x, camera_y))
        self.number(0x54d930, active)
        self.number(0x5ad128, 0x1000002d)
        self.number(0x5c78d4, 2)
        self.number(0x5c78f0, WIDTH)
        self.number(0x5c78f4, HEIGHT)
        self.number(0x5c7920, 0)
        self.mu.mem_write(0x5c7a44, struct.pack('<8i', 5, 7, 91, 73, 0, 0, WIDTH, HEIGHT))
        self.mu.mem_write(0x5b5ddc, struct.pack('<I4i', SURFACE, 5, 7, 90, 72))
        self.mu.mem_write(SURFACE, struct.pack('<4I', CANVAS, PITCH - 1, HEIGHT - 1, 1))
        self.mu.mem_write(CANVAS - 32, bytes([0x7d]) * (PITCH * HEIGHT + 64))
        self.mu.mem_write(CANVAS, bytes([199]) * (PITCH * HEIGHT))
        self.rects = 0
        self.call('DrawWater', control=control)
        assert self.number(0x5c7920) == 0
        assert bytes(self.mu.mem_read(CANVAS - 32, 32)) + bytes(self.mu.mem_read(CANVAS + PITCH * HEIGHT, 32)) == bytes([0x7d]) * 64
        raw = bytes(self.mu.mem_read(CANVAS, PITCH * HEIGHT))
        return '\t'.join((changed(raw, 199), ':'.join(map(str, self.clip())), str(self.number(0x54d930)), str(self.number(0x5ad128)), ';'.join(self.events) or '-'))


def main():
    """최초 관측/해시만 새 work에 보존하며 기존 fixture/제품/빌드를 건드리지 않는다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    fixture = WORK / 'modified/cpppj/tests/fixtures/draw-water-x86.tsv'
    report = WORK / 'native.json'
    assert not fixture.exists() and not report.exists()
    cases = []
    for active, cells, origin, scene in itertools.product((0, 1, 0xffffffff), ((16, 11), (32, 22), (7, 13), (-7, -13)), ((19.25, 19.5), (20.99, 20.125)), range(3)):  # 비영 전체DWORD·서로 다른 축/음수 half 및 Renderer/snapshot 차이를 독립 입력으로 공급한다.
        cases.append((active, *cells, *origin, 304, 209, 0, scene))
    cases += [(1, 16, 11, 19.25, 19.5, 304, 209, mutation, 0) for mutation in range(1, 5)]
    oracle = Oracle()
    lines = ['# active,cellX,cellY,RendererX,RendererY,snapshotPixelX,snapshotPixelY,mutation,scene — changed runs,final clip,flag,color,events; actual finder/project/rect/VFX, screen lock/present are explicit boundaries']
    for case in cases:  # 기대 표시/순회/픽셀은 호스트 계산으로 만들지 않는다.
        first = oracle.run(case, 0x027f)
        assert first == oracle.run(case, 0x037f), case
        lines.append(json.dumps(case, separators=(',', ':')) + '\t' + first)
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    record = dict(unit=768, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', inputs=len(cases), observations=len(cases) * 2, calls=dict(oracle.calls), gameExecutions=0, multiplayerTests=0, builds=0, actual=[f'{value:08x}' for value in ENTRIES] + ['actual RawSquidFinder Begin/Next and lower helpers'], boundary='Screen Lock/Unlock/Present are explicit memory device boundaries, synthetic generic TYPE/SID/hash are independent input; no actual OS/GUI/whole frame execution', fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(inputs=len(cases), observations=len(cases) * 2, rectCalls=oracle.calls['004a2270'])))


if __name__ == '__main__':
    main()
