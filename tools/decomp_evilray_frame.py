#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EvilRay 실제 Frame/Draw의 상태와 음향/dirty/타원 인자를 격리 PE에서 관찰한다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_fierytail_frame import FieryOracle, ROOT, OBJECT, PARTICLES, POOL, PALETTE, COLORS
from decomp_fierytail_lifetime import LifetimeOracle
from decomp_evilray_lifetime import semantic
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT / 'cpppj/tests/fixtures/evilray-frame-x86.tsv'  # 원문 상태/입자/출력 인자를 보존하는 새 fixture다.
ENTRIES = (0x453290, 0x453540, 0x453040, 0x418430, 0x487b20)  # 기존 공통 timer/dirty/ftol에 실제 EvilRay 몸체만 더한다.


class EvilRayFrameOracle(FieryOracle):
    """프레임/그리기 수치 계산은 원문에서 실행하고 메타데이터/음향/출력/삭제만 관찰한다."""

    ret = LifetimeOracle.ret  # 외부 cdecl/thiscall 반환과 purge를 공급하는 기존 격리 경계다.
    integer = LifetimeOracle.integer  # little endian DWORD 조회만 재사용한다.

    def __init__(self):
        """현재 정밀 함수표의 정확한 분할 범위를 추가한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 함수 경계 밖의 임의 실행은 기존 Oracle처럼 거부한다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """외부 경계의 실제 인자와 순서를 보존하며 source 저장 포인터 진단 복귀도 관찰한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x49a840:
            self.selected_type = self.integer(esp + 4)
            self.ret(PALETTE + 0x8000)
            return
        if address == 0x419850:
            direction, frame = struct.unpack('<2I', mu.mem_read(esp + 4, 8))
            self.events.append(f'M:{self.selected_type}:{direction}:{frame}')
            self.ret(PALETTE + 0x9000, 8)
            return
        if address == 0x4ac550:
            sid = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
            end = self.integer(esp) == 0x4534bf
            self.events.append(f'T:{sid}:{int(end)}')
            sound = self.end_sound if end else self.source_sound if sid != 51 else self.parent_sound
            mu.mem_write(PALETTE + 0x8070, struct.pack('<2I', sound, self.end_sound))
            self.ret(PALETTE + 0x8000)
            return
        if address in (0x4a9c30, 0x499310, 0x4dae20):
            count = 4 if address == 0x4dae20 else 5
            payload = bytes(mu.mem_read(esp + 4, count * 4)).hex()
            self.events.append(('S:' if address == 0x4a9c30 else 'D:' if address == 0x499310 else 'R:') + payload)
            self.ret()
            return
        if address == 0x4a1580:
            self.events.append('C:' + ','.join(map(str, struct.unpack('<4i', mu.mem_read(esp + 4, 16)))))
            self.ret()
            return
        if address == 0x41bf10:
            self.events.append('K:' + str(self.integer(esp + 4)))
            self.killed = True
            self.ret(0, 4)
            return
        if address == 0x4e0620:
            expression, file, line = struct.unpack('<3I', mu.mem_read(esp + 4, 12))
            text = bytes(mu.mem_read(expression, 100)).split(b'\0')[0].decode('ascii')
            actual_file = bytes(mu.mem_read(file, 80)).split(b'\0')[0]  # 다른 하위 원문 진단이면 그 문자열/위치를 실패 근거로 보존한다.
            assert actual_file == b'.\\Evilray.cpp', (text, actual_file, line)
            self.events.append(f'A:{line}:{text}')
            if self.mutate_source:
                mu.mem_write(OBJECT + 0x54, struct.pack('<I', 51))
            self.ret()
            return
        super().instruction(mu, address, size, data)

    def sample(self, case, control):
        """독립 현재 필드를 설치하고 실제 Frame/Draw를 호출하여 결과만 읽는다."""
        name, mode, raw, particles, pool, state, parameters = case
        self.capacity = struct.unpack_from('<I', raw, 0x24)[0]
        width, self.source_sound, self.parent_sound, self.end_sound, self.mutate_source = parameters
        mu = self.mu
        mu.mem_write(OBJECT, raw)
        mu.mem_write(PARTICLES, particles or bytes(4))
        mu.mem_write(POOL, bytes(5000))
        # 실제 source0/50와 부모51의 원본 raw 입력을 서로 다른 슬롯에 둔다.
        for offset, sid in enumerate((0, 50, 51)):
            mu.mem_write(POOL + sid * 50, pool[offset * 50:(offset + 1) * 50])
        mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        mu.mem_write(0x5c847c, struct.pack('<I', 100))
        mu.mem_write(0x55b4d0, state[:8])
        mu.mem_write(0x59a924, state[8:16])
        mu.mem_write(0x59a934, state[16:])
        mu.mem_write(0x55a6f0, struct.pack('<I', PALETTE))
        mu.mem_write(PALETTE, struct.pack('<2I', 3, COLORS))
        mu.mem_write(COLORS, struct.pack('<3I', 17, 0x12345678, 0xabcdef01))
        mu.mem_write(PALETTE + 0x9018, struct.pack('<h', width))
        self.events, self.killed = [], False
        if mode == 'frame':
            self.call(0x453290, OBJECT, b'', 0, control)
        if not self.killed:
            self.call(0x453540, OBJECT, struct.pack('<2I', 0x11223344, 0x55667788), 8, control)
        after = bytes(mu.mem_read(OBJECT, 0x60))
        return semantic(after).hex(), bytes(mu.mem_read(PARTICLES, len(particles))).hex() or '-', ';'.join(self.events) or '-'


def cases():
    """기한/전체 delta/꼬리/중단/좌표 저장 경계의 독립 입력을 구성하며 기대 결과를 계산하지 않는다."""
    result = []  # 원문에 넘길 객체/입자/물리/현재 상태다.
    variants = [
        (3, 0, 0, 1.13, 0, 0, 50, 0, 9),
        (3, 1, 1, 1.13, 0, 0, 50, 0, 9),
        (3, 1, 1, 1.025, 0, 0, 50, 1, -9),
        (0, 0, 1, 1.13, 0, 0, 50, 1, 10),
        (3, 3, 1, 1.13, 0, 0, 50, 0, 9),
        (3, 2, 1, 1.13, 100, 1, 50, 1, 9),
        (3, 0, 1, 1.13, 100, 1, 50, 0, 9),
        (3, 1, 1, .9, 100, 0, 50, 0, 9),
        (3, 1, 1, 1.13, 0, 0, 0, 1, 9),
        (3, 1, 1, 1.000000001, .12345679104328156, 0, 50, 0, 9),
    ]  # capacity/total/first/now/due/stopping/source/fallback/폭 입력이다.
    # 각 현재 필드를 실제 Native 배치에만 기록한다.
    for index, (capacity, total, first, now, due, stopping, source, fallback, width) in enumerate(variants):
        raw = bytearray(b'\xa5' * 0x60)
        struct.pack_into('<3I4f2i', raw, 0, 0x508e80, 1234, 51, 12.25, 13.75, 30.5, 35.5, 0, -25)
        struct.pack_into('<3I2d2fI', raw, 0x24, capacity, total, PARTICLES, 1., .123456789, 123.25, -456.5, first)
        struct.pack_into('<f2I', raw, 0x50, due, source, stopping)
        particles = b''.join(struct.pack('<5fI', 25.999999 + i, 30.999999 - i, .125, -1.25, .1 if stopping else 13.0 if i % 2 == 0 else -1., 0xcafe0000 + i) for i in range(capacity))
        pool = bytearray(150)
        # 형상이 조회할 TYPE/방향/프레임과 음향이 조회할 raw 좌표를 각각 명시한다.
        for offset, sid in enumerate((0, 50, 51)):
            start = offset * 50
            pool[start + 10] = 70 + offset
            pool[start + 40] = 3
            struct.pack_into('<H', pool, start + 6, 1)
            struct.pack_into('<2f', pool, start + 14, 20.75 + offset, 21.9 - offset)
            struct.pack_into('<I', pool, start + 36, 123 + offset)
        state = struct.pack('<d5f', now, .75 if fallback else 16., 1.25 if fallback else 11., .3, .25, -.5)
        result.append((f'frame{index}', 'frame', bytes(raw), particles, bytes(pool), state, (width, 0 if fallback else 11, 21, 31, int(source == 0))))
    # 저장 전/후 정수화·반지름·양수/비유한 수명은 직접 Draw에서 관찰한다.
    for index, life in enumerate((13., 12.75, 6.5, .001, 0., -1., float('nan'), float('inf'))):
        raw = bytearray(result[1][2]);struct.pack_into('<2I', raw, 0x24, 1, 1)
        particle = struct.pack('<5fI', -1.9999999, 123.999999, .125, -1.25, life, 0xabcdef01)
        result.append((f'draw{index}', 'draw', bytes(raw), particle, result[1][4], result[1][5], result[1][6]))
    return result


def main():
    """두 x87 관측을 새 파일로 보존하거나 기존 기대값 변경 없이 다시 관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle, rows = EvilRayFrameOracle(), []  # 입력마다 현재 상태를 다시 설치한다.
    # C++를 호출하지 않고 실제 원문 반환만 수집한다.
    for case in cases():
        name, mode, raw, particles, pool, state, parameters = case
        for control in (0x027f, 0x037f):
            rows.append((name, mode, control, raw.hex(), particles.hex() or '-', pool.hex(), state.hex(), ','.join(map(str, parameters)), *oracle.sample(case, control)))
    content = '# name mode cw raw particles pool0_50_51 state parameters fieldsAfter particlesAfter events; 실제 PE 관측/게임0\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify로 재관측한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
