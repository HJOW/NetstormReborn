#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Pop 입구/Point.isValid의 signed세계크기→float저장과 ordered 비교만 실제 PE에서 격리 관찰한다."""
import argparse
import itertools
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 새 관찰 기준이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/pop-world-bounds-x86.tsv'  # 입력/원문 비교값과 실제 읽기 횟수다.
POOL, POINT, STACK, STOP = 0x12000000, 0x12000300, 0x20000800, 0x30000000  # 함수 선택에 필요한 독립 물리 입력이다.
LIMITS = (-2147483648, -1, 0, 1, 2, 10, 32, 64, 255, 256, 257, 16777217, 2147483647)  # signed/경계/float저장 반올림 입력이다.
POINTS = ((0, 1), (-0.0, 1), (-1, 1), (0.125, 0.25), (1, 1), (10, 10), (31.999, 20), (32, 20), (40, 20), (255.5, 255.5), (256, 10), (16777216, 10), (float('nan'), 10), (10, float('nan')), (float('inf'), 10), (-float('inf'), 10), (10, float('inf')), (10, -float('inf')), (33, float('nan')), (1, -0.0))  # NaN/무한값도 명시 데이터이며 게임 객체를 만들지 않는다.


def bits(value):
    """입력을 먼저f32로 저장해 양판본 정밀도 관측에서 같은 비트를 사용한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


class PopWorldBoundsOracle:
    """실제 Pop의 SetPosition 진입까지와 실제40e800 전체만 실행한다. 큰 Pop 하위는 실행하지 않는다."""

    def __init__(self):
        """슬롯/세계크기/메타맵 무로그와 독립 스택만 준비한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(POOL, 0x1000)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        # Pop 앞단이 읽는 실제 물리 범위와 로그 억제 입력이다.
        for address, value in ((0x5c8464, POOL), (0x5c847c, 24000), (0x594fc8, 1)):
            self.mu.mem_write(address, struct.pack('<I', value))
        raw = bytearray(50)
        raw[10], raw[11] = 74, 4
        self.mu.mem_write(POOL + 50 * 50, bytes(raw))
        self.context = self.mu.context_save()
        self.mode, self.reads, self.output = '', 0, None  # 이번 실행 범위/원문 읽기/SetPosition 인자다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """SetPosition은 원문이 선택한 인자만 기록하고 실제 몸체를 실행하기 전에 멈춘다."""
        if self.mode == 'front' and address == 0x4210a0:
            assert mu.reg_read(UC_X86_REG_ECX) == POOL + 2500
            self.output = tuple(struct.unpack('<2I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 8)))
            mu.emu_stop()
            return
        assert 0x4b02d0 <= address <= 0x4b03b0 if self.mode == 'front' else 0x40e800 <= address <= 0x40e84c, hex(address)

    def read(self, mu, access, address, size, value, data):
        """세계 크기의 실제 메모리 읽기만 세어x단락/한 번 저장을 확인한다."""
        if address <= 0x531928 < address + size:
            self.reads += 1

    def write(self, mu, access, address, size, value, data):
        """부분 실행은 스택 외의 게임 메모리를 바꾸면 안 된다."""
        assert STACK - 0x100 <= address and address + size <= STACK + 16

    def observe(self, limit, xbits, ybits, control):
        """두 실행의 CPU/x87 문맥을 독립 복원해 중단된 앞단의 레지스터를 다음 입력에 남기지 않는다."""
        observed = []  # 실제 helper 반환/조회와 실제 Pop이 선택한 위치다.
        # 같은 입력으로 완전 helper와 Pop 앞단을 각각 실행한다.
        for mode in ('point', 'front'):
            self.mu.context_restore(self.context)
            self.mu.reg_write(UC_X86_REG_FPCW, control)
            self.mu.reg_write(UC_X86_REG_FPSW, 0)
            self.mu.mem_write(0x531928, struct.pack('<i', limit))
            self.mode, self.reads, self.output = mode, 0, None
            self.mu.reg_write(UC_X86_REG_ESP, STACK)
            if mode == 'point':
                self.mu.mem_write(POINT, struct.pack('<2I', xbits, ybits))
                self.mu.mem_write(STACK, struct.pack('<I', STOP))
                self.mu.reg_write(UC_X86_REG_ECX, POINT)
                self.mu.emu_start(0x40e800, STOP, count=200)
                assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
                observed.extend((self.mu.reg_read(UC_X86_REG_EAX), self.reads))
            else:
                self.mu.mem_write(STACK, struct.pack('<4I', STOP, xbits, ybits, 0))
                self.mu.reg_write(UC_X86_REG_ECX, POOL + 2500)
                self.mu.emu_start(0x4b02d0, STOP, count=200)
                assert self.output is not None
                observed.extend((*self.output, self.reads))
        return tuple(observed)


def main():
    """260입력을53/64비트 정밀도에서 각각 실행하고 같은 전체 관찰을 재관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle = PopWorldBoundsOracle()
    rows = []  # 기대 분기는 C++에서 얻지 않고 실제 두 원문 실행에서 수집한다.
    # 현재 크기/좌표 입력을 교차한다.
    for limit, point in itertools.product(LIMITS, POINTS):
        x, y = map(bits, point)
        first = oracle.observe(limit, x, y, 0x27f)
        assert first == oracle.observe(limit, x, y, 0x37f)
        rows.append((limit, x, y, *first))
    content = '# worldCells xBits yBits nativePointValid pointLimitReads nativePopXBits nativePopYBits popLimitReads; helper전체/Pop좌표앞단260행\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify를 사용한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'nativeRows': len(rows), 'nativeFunctionObservations': len(rows) * 4, 'reobserved': args.verify, 'gameExecutions': 0, 'scope': 'whole40e800 andPopfrontonly, noSetPosition/bodyexecution'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
