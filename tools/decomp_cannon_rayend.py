#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 썬 캐논 사정선과 하위 함수 전체를 격리 x86에서 실행해 기대값을 만든다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 검증 자료의 저장소 기준이다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

POOL, TYPES, OUTPUT, STACK, STOP = 0x12000000, 0x12100000, 0x12200000, 0x3007f000, 0x20000000  # 원본 주소와 겹치지 않는 입력/반환/스택이다.
SID = 50  # 합성 raw TYPE128 자산의 실제 슬롯 번호다.
ENTRIES = {0x481750, 0x41ce90, 0x466df0, 0x4ac550, 0x49a840, 0x49ace0, 0x4e49c0, 0x41d0d0}  # 대체하지 않고 실행할 전체 함수 집합이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/cannon-rayend-x86.tsv'  # 기존 기하 관찰을 덮지 않는 독립 입력/출력이다.
REPORT = ROOT / 'cpppj/cannon-rayend.json'  # 실제 반환 수와 검증 자료의 SHA-256이다.


def digest(path):
    """근거 파일 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    """원본 함수에 공급할 입력 좌표만 binary32 비트로 바꾼다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """모든 허용 프레임·방향·대각선·동일점·경계 좌표·절삭 사정거리를 교차한다."""
    frames = (0, 1, 2, 3, 24, 25, 26, 27)  # 원본 표가 허용 방향을 가지는 물리 프레임이다.
    origins = ((10, 20), (.5, .5), (254.75, 255.5), (255, 1), (100.25, 120.75), (-20, 100), (280, -40))  # 보드 안팎과 분수 포구 입력이다.
    offsets = ((0, -20), (20, 0), (0, 20), (-20, 0), (20, 20), (-20, 20), (20, -20), (-20, -20), (0, 0), (1, .99999994), (.99999994, 1))  # 방향 동률의 세로 우선과 동일점도 포함한다.
    ranges = (0, .9, 9.75, 256.9, -2.5, -2147483648.0, 2147483520.0)  # 절삭·음수·정수 곱 감김 경계다.
    rows = []  # 원본 기대값 계산 없이 입력 비트만 보관한다.
    # binary32 반올림 후 중복되는 합성 좌표는 고유 입력 하나로 센다.
    for frame, (x, y), (dx, dy), distance in itertools.product(frames, origins, offsets, ranges):
        rows.append((frame, bits(x), bits(y), bits(x + dx), bits(y + dy), bits(distance)))
    # 부호 있는 0도 입력 비트가 구별되는 독립 관찰이다.
    for frame in frames:
        rows.append((frame, bits(-0.0), bits(0.0), bits(0.0), bits(-0.0), bits(9.75)))
    return list(dict.fromkeys(rows))


class Oracle:
    """사정선·방향·타입 조회·__ftol·clamp를 전부 실제 원본 명령으로 실행한다."""
    def __init__(self):
        """보호 PE와 현재 PC 정밀 함수 범위를 격리 메모리에 준비한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 게임 프로세스를 시작하지 않는 CPU 실행기다.
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        mapped = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(mapped) + 4095) & ~4095)
        self.mu.mem_write(0x400000, mapped)
        # 풀/타입/출력/스택/종료 주소는 각각 분리해 잘못된 쓰기를 감지한다.
        for address, size in ((POOL, 0x10000), (TYPES, 0x20000), (OUTPUT, 0x1000), (STACK - 0x7f000, 0x80000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.mu.mem_write(0x5c8464, struct.pack('<I', POOL))
        self.mu.mem_write(0x59ab20, struct.pack('<I', TYPES))
        self.allowed = []  # 현재 PC에서 내보낸 원본 함수 몸체의 정확한 명령 범위다.
        self.calls = Counter()  # helper를 대체하지 않았음을 확인하는 실제 진입 수다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 함수표 밖의 코드 진입을 허용하지 않는다.
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(v, 16) for v in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """검토한 실제 몸체만 실행하고 assert나 외부 callback 대체는 허용하지 않는다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소 {address:08x}')
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """반환 8바이트와 현재 스택 외에 원본/입력/전역을 쓰면 실패시킨다."""
        if (OUTPUT <= address and address + size <= OUTPUT + 8) or (STACK - 0x7f000 <= address and address + size <= STACK + 0x1000):
            return
        raise RuntimeError(f'허용하지 않은 쓰기 {address:08x}+{size}')

    def run(self, values, precision):
        """동일한 새 입력으로 한 호출을 수행하고 반환 비트·ABI·x87 균형을 확인한다."""
        frame, x, y, target_x, target_y, distance = values
        raw = bytearray(50)  # TYPE와 frame만 필요한 실제 50바이트 자산 슬롯이다.
        raw[10] = 128
        struct.pack_into('<I', raw, 36, frame)
        self.mu.mem_write(POOL + SID * 50, bytes(raw))
        self.mu.mem_write(TYPES + 128 * 500 + 0x44, struct.pack('<I', distance))
        self.mu.mem_write(OUTPUT, b'\xa5' * 16)
        self.mu.mem_write(STACK, struct.pack('<8I', STOP, OUTPUT, x, y, target_x, target_y, SID, 0xc0dec0de))
        registers = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789, UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}  # 비휘발 레지스터의 독립 sentinel이다.
        # 앞 호출이 남긴 레지스터나 FPU 스택을 입력으로 재사용하지 않는다.
        for register, value in registers.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.emu_start(0x481750, STOP, count=3000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert self.mu.reg_read(UC_X86_REG_EAX) == OUTPUT
        # 모든 비휘발 레지스터와 x87 제어/스택을 원본 반환 뒤 검사한다.
        for register, value in registers.items():
            assert self.mu.reg_read(register) == value
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0
        assert self.mu.reg_read(UC_X86_REG_FPTAG) == 0xffff
        assert bytes(self.mu.mem_read(POOL + SID * 50, 50)) == bytes(raw)
        assert bytes(self.mu.mem_read(OUTPUT + 8, 8)) == b'\xa5' * 8
        return struct.unpack('<2I', self.mu.mem_read(OUTPUT, 8))


def main():
    """기계어 기대값을 별도 fixture에 저장하거나 현재 근거의 SHA와 고유 행을 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(REPORT.read_text(encoding='utf-8'))
        # 이전 원본 관찰의 파일 전체 바이트가 그대로인지 확인한다.
        for name, wanted in record['sha256'].items():
            assert digest(ROOT / name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == record['uniqueInputs']
        assert len({tuple(row[:6]) for row in rows}) == len(rows)
        print('CannonRayEnd 검증:', len(rows), '고유 입력, 근거 SHA 일치')
        return
    oracle = Oracle()
    rows = []  # 기대 좌표는 원본 CPU의 반환값만 기록한다.
    differences = 0  # x87 027f/037f의 반환 비트 차이 수다.
    # 두 정밀도에 같은 raw 입력을 공급하고 결과를 덮어쓰지 않고 비교한다.
    for values in cases():
        first = oracle.run(values, 0x027f)
        second = oracle.run(values, 0x037f)
        differences += first != second
        rows.append((*values, *first))
    assert differences == 0, differences
    FIXTURE.write_text('# 10.78 TYPE128 00481750 및 모든 하위 몸체. 값은 DWORD 비트, 기본 x87=027f.\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_cannon_rayend.py', FIXTURE.relative_to(ROOT).as_posix())  # 원본/정밀 범위/도구/기대값의 불변 근거다.
    record = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(rows), normalReturns=len(rows) * 2, precisionDifferences=differences, nativeCalls=dict(sorted(oracle.calls.items())), sha256={name: digest(ROOT / name) for name in paths}, limitations=['합성 TYPE128/raw 프레임·사정거리 입력이다. 실제 Missile 생성·비행·사용자 플레이 증거로 세지 않는다.', '다른 TYPE/판본과 허용 방향이 없는 프레임은 포함하지 않는다.'])
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('CannonRayEnd:', len(rows), '고유 입력,', len(rows) * 2, '정상 반환, 정밀도 차이', differences)


if __name__ == '__main__':
    main()
