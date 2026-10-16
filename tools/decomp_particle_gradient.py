#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의 색 띠 생성·세 번의 실제 CRT 정렬·Particle 팔레트 병합을 독립 관찰한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본과 새 검증 자료의 기준 경로다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

SOURCE, PALETTE, FIRST, SECOND, MERGED = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000  # 입력 RGBX/기초 팔레트/두 색 띠/병합 결과의 격리 영역이다.
STACK, STOP = 0x3007f000, 0x20000000  # 실제 스택과 게임 코드 밖의 정상 반환 지점이다.
ENTRIES = {0x439de0, 0x43a8e0, 0x43ad20, 0x43abe0, 0x43a4a0, 0x43a0c0, 0x43a300, 0x43a3d0, 0x43fa50, 0x4e49c0, 0x4e6010, 0x4e5fa0, 0x487cb0, 0x487d20}  # 대체하지 않고 실제 실행할 정밀 함수다.
PRECISION_ENTRIES = {0x4e81da, 0x4f2a7b, 0x4f2a49, 0x4f2929, 0x4f29bb}  # 실제 CRT 기본 정밀도 설정과 비트 변환 함수다.
CALLBACK, CALLBACK_END = 0x43a660, 0x43a79f  # 전체 정밀 함수표에서 빠졌으나 vtable/PE 명령/ret4로 확인한 색 축 callback 범위다.
SCRATCH = (0x555df0, 0x555da8, 0x555d80, 0x555db8, 0x555db0, 0x555dd0, 0x555da0, 0x555dc0, 0x555d90, 0x555d98, 0x555de0, 0x555de8, 0x555dc8, 0x555e00)  # 원본 callback과 원통 정렬이 쓰는 double 전역이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/particle-gradient-x86.tsv'  # 이전 건설/불타는 자료와 분리한 새 기대값이다.
REPORT = ROOT / 'cpppj/particle-gradient.json'  # 고유 입력·정밀도 차이·실제 helper와 SHA를 보존한다.


def digest(path):
    """근거 파일 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def double_bits(value):
    """대상 RGB/폭의 double 입력을 손실 없는 정수 문자열로 기록한다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def encode_gradient(values):
    """색 띠의 RGB 여섯 double·요청 색 수·폭을 손실 없이 fixture 입력으로 기록한다."""
    return (','.join(str(double_bits(value)) for value in (*values[0], *values[1])), values[2], double_bits(values[3]))


def inputs():
    """실제 팔레트와 중복/회색/다색 입력에서 서로 다른 원통 및 선택 수를 방문한다."""
    palettes = []  # 원본 COL의 RGBX 입력과 읽은 보호 파일 경로다.
    # 실제 팔레트는 파일에서 읽으며 헤더의 색 수를 재해석하거나 바꾸지 않는다.
    for name in ('GIFCLOUD.COL', 'BULF.COL', 'SUNCANNON.COL'):
        path = ROOT / 'originals/d' / name
        data = path.read_bytes()
        assert len(data) == 776
        rgba = b''.join(data[8 + index * 3:11 + index * 3] + b'\0' for index in range(256))
        palettes.append((name, rgba, path.relative_to(ROOT).as_posix(), True))
    palettes.append(('gray', bytes(channel for index in range(256) for channel in (index, index, index, 0)), None, False))
    palettes.append(('duplicates', bytes(channel for index in range(256) for channel in (((index % 8) & 1) * 255, ((index % 8) >> 1 & 1) * 255, ((index % 8) >> 2 & 1) * 255, index)), None, False))
    palettes.append(('spectrum', bytes(channel for index in range(256) for channel in ((index * 17) & 255, (index * 43) & 255, (index * 97) & 255, 4)), None, False))
    # 두 Fiery Tail RGB 끝점은 PE double과 같은 입력 값을 사용한다.
    fiery = ((1.0, 195 / 255, 6 / 255), (185 / 255, 0.0, 0.0), 8, 2.4)
    core = ((1.0, 195 / 255, 6 / 255), (1.0, 1.0, 173 / 255), 3, 2.4)
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    image = pe.get_memory_mapped_image()
    # 비율 표현의 binary64가 실제 효과 생성자의 PE 상수와 같은지 먼저 확인한다.
    for address, value in ((0x509578, fiery[0][1]), (0x509570, fiery[0][2]), (0x509568, fiery[1][0]), (0x509560, core[1][2])):
        assert struct.unpack_from('<d', image, address - 0x400000)[0] == value
    black, white = (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)  # 시작/끝의 독립 RGB 입력이다.
    groups, seen = [], set()  # 같은 binary64/팔레트 입력을 중복 관찰로 세지 않는다.
    # 색 선택을 Python으로 계산하지 않고 입력 조합만 준비한다.
    for name, rgba, path, actual in palettes:
        pairs = [((black, white, 8, 2.4), (white, black, 3, 2.4)), ((black, white, 2, 1.5), (white, black, 13, 2.4)), ((black, white, 31, 1.25), (white, black, 7, 2.4))]
        if actual:
            pairs += [(fiery, core), (((0.0, 0.0, 1.0), white, 8, 2.4), (white, (.5, .5, .5), 3, 2.4))]
        # 같은 팔레트 바이트/두 RGB/개수/폭이 같은 입력은 한 번만 기록한다.
        for index, (a, b) in enumerate(pairs):
            key = (rgba, a, b)
            if key in seen:
                continue
            seen.add(key)
            groups.append((f'{name}-{index}', rgba, path, a, b))
    return groups


class Oracle:
    """팔레트 구성·색 선택·CRT 정렬·병합은 실제 몸체를 쓰고 확보/반납만 외부 입력으로 공급한다."""
    def __init__(self):
        """현재 PC 정밀 함수표와 별도 확인한 callback 범위만 실행 가능하게 준비한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 실제 게임이나 창을 열지 않는 CPU 실행기다.
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        mapped = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(mapped) + 4095) & ~4095)
        self.mu.mem_write(0x400000, mapped)
        # 원본 포인터와 겹치지 않는 입력/출력 영역을 분리한다.
        for address in (SOURCE, PALETTE, FIRST, SECOND, MERGED):
            self.mu.mem_map(address, 0x10000)
        self.mu.mem_map(STACK - 0x7f000, 0x80000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_write(0x555d88, struct.pack('<I', PALETTE))
        self.allowed = [(CALLBACK, CALLBACK_END)]  # 누락 callback은 PE의 실제 ret4까지 별도 범위로 제한한다.
        self.calls = Counter()  # 실제 진입 수와 외부 new의 호출 수다.
        self.allocated = []  # 마지막 병합의 실제 요청 바이트 수다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 내보낸 각 함수의 불연속 범위를 그대로 사용한다.
        for entry in ENTRIES | PRECISION_ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """계산/비교/정렬은 대체하지 않으며 병합 new/free의 성공만 외부 입력으로 돌려준다."""
        if address == 0x4e4354:
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, pointer = struct.unpack('<II', mu.mem_read(esp, 8))
            assert pointer == MERGED + 0x1000
            self.calls['free-boundary'] += 1
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == 0x4e4391:
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, amount = struct.unpack('<II', mu.mem_read(esp, 8))
            assert 0 < amount <= 3 * 256 * 4
            self.allocated.append(amount)
            self.calls['new-boundary'] += 1
            mu.reg_write(UC_X86_REG_EAX, MERGED + 0x1000)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소 {address:08x}')
        if address in ENTRIES | PRECISION_ENTRIES or address == CALLBACK:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """현재 출력 객체·입자 색 표·알려진 색 정렬 전역·스택 밖의 쓰기를 거부한다."""
        regions = ((PALETTE, PALETTE + 0x3800), (FIRST, FIRST + 0x5020), (SECOND, SECOND + 0x5020), (MERGED, MERGED + 8), (MERGED + 0x1000, MERGED + 0x1c00), (STACK - 0x7f000, STACK + 0x1000))
        if any(low <= address and address + size <= high for low, high in regions) or any(low <= address and address + size <= low + 8 for low in SCRATCH):
            return
        raise RuntimeError(f'허용하지 않은 쓰기 {address:08x}+{size}')

    def call(self, entry, this, arguments, purge, precision, result=None, after_precision=None):
        """한 실제 호출의 스택/비휘발/x87 제어와 스택 균형을 확인한다."""
        self.mu.mem_write(STACK, struct.pack('<I', STOP) + arguments + b'\xc0\xde\xc0\xde')
        registers = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789, UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}  # 호출마다 새 sentinel을 공급한다.
        # 앞 호출의 레지스터/FPU 상태를 다음 독립 관찰에 넘기지 않는다.
        for register, value in registers.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.emu_start(entry, STOP, count=15000000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + purge
        assert bytes(self.mu.mem_read(STACK + 4 + len(arguments), 4)) == b'\xc0\xde\xc0\xde'
        if result is not None:
            assert self.mu.reg_read(UC_X86_REG_EAX) == result
        # 각 비휘발 레지스터와 x87 제어/스택을 반환 뒤 검사한다.
        for register, value in registers.items():
            assert self.mu.reg_read(register) == value
        assert self.mu.reg_read(UC_X86_REG_FPCW) == (precision if after_precision is None else after_precision)
        assert self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0
        assert self.mu.reg_read(UC_X86_REG_FPTAG) == 0xffff

    def gradient(self, address, values, precision):
        """실제 ctor를 호출한 뒤 색 선택에 쓰는48바이트 의미 필드를256항목 전부 기록한다."""
        start, end, count, width = values
        self.mu.mem_write(address, b'\xa5' * 0x5020)
        self.call(0x43ad20, address, struct.pack('<6dId', *start, *end, count, width), 60, precision, address)
        raw = bytes(self.mu.mem_read(address, 0x5020))
        nearest_from, nearest_to, desired, selected = struct.unpack_from('<4i', raw, 0x5008)
        assert desired == count
        threshold = struct.unpack('<Q', self.mu.mem_read(0x555e00, 8))[0]
        semantic = bytearray()  # HSV와 미사용 패딩을 색 선택의 비교 범위에 포함하지 않는다.
        # 원본80바이트 항목에서 실제 RGB 차/번호/거리/투영/선택 DWORD를 차례로 보존한다.
        for index in range(256):
            offset = 8 + index * 80
            semantic += raw[offset + 0x18:offset + 0x34] + raw[offset + 0x38:offset + 0x4c]
        assert len(semantic) == 256 * 48
        return ','.join(map(str, (nearest_from, nearest_to, selected, threshold))), semantic.hex()

    def sample(self, case, precision):
        """같은 팔레트의 두 색 띠와 실제 병합까지 실행한다. 결과 색은 Python으로 계산하지 않는다."""
        name, rgba, path, a, b = case
        self.mu.mem_write(SOURCE, rgba)
        self.mu.mem_write(PALETTE, b'\xa5' * 0x3800)
        self.call(0x439de0, PALETTE, struct.pack('<I', SOURCE), 4, precision, PALETTE)
        palette_before = bytes(self.mu.mem_read(PALETTE, 0x3800))
        first = self.gradient(FIRST, a, precision)
        second = self.gradient(SECOND, b, precision)
        gradients_before = (bytes(self.mu.mem_read(FIRST, 0x5020)), bytes(self.mu.mem_read(SECOND, 0x5020)))
        self.mu.mem_write(MERGED, b'\xa5' * 0x3000)
        self.call(0x487cb0, MERGED, b'', 0, precision)
        self.allocated = []
        outputs = []  # 1/2/3색 띠 병합의 실제 색 번호를 각각 보존한다.
        # 원본 free→새 색 수→new→복사 순서를 같은 팔레트 객체에 실행한다.
        for pointers, expected in (((FIRST, SECOND, 0), a[2] + b[2]), ((FIRST, SECOND, FIRST), a[2] * 2 + b[2]), ((FIRST, 0, 0), a[2])):
            self.call(0x487d20, MERGED, struct.pack('<III', *pointers), 12, precision)
            count, data = struct.unpack('<II', self.mu.mem_read(MERGED, 8))
            assert count == expected and data == MERGED + 0x1000
            outputs.append(bytes(self.mu.mem_read(data, count * 4)).hex())
        assert self.allocated == [(a[2] + b[2]) * 4, (a[2] * 2 + b[2]) * 4, a[2] * 4]
        assert bytes(self.mu.mem_read(SOURCE, 1024)) == rgba
        assert bytes(self.mu.mem_read(PALETTE, 0x3800)) == palette_before
        assert bytes(self.mu.mem_read(FIRST, 0x5020)) == gradients_before[0]
        assert bytes(self.mu.mem_read(SECOND, 0x5020)) == gradients_before[1]
        return (*first, *second, *outputs)


def main():
    """두 x87 정밀도의 실제 결과를 저장하거나 기존 전체 바이트 근거를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본·팔레트·정밀 범위·도구·기대값을 재계산 없이 확인한다.
        for name, wanted in report['sha256'].items():
            assert digest(ROOT / name) == wanted, name
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == report['uniqueInputs'] * 2
        assert all(len(row) == 16 for row in rows)
        assert len({tuple(row[1:9]) for row in rows}) == len(rows)
        print('ParticleGradient 근거 SHA/고유 행 확인:', len(rows))
        return
    cases = inputs()
    if args.smoke:
        cases = cases[:1]
    oracle = Oracle()
    # 원본 CRT의 __cinit(1)→__fpmath→__setdefaultprecision이 선택하는 기본 정밀도를 직접 확인한다.
    oracle.call(0x4e81da, 0, b'', 0, 0x037f, after_precision=0x023f)
    rows, different, palette_different = [], [], []  # 두 정밀도의 의미 필드 차이와 최종 색 번호 차이를 구별한다.
    # 같은 RGB 입력을 두 정밀도에서 각각 실제로 실행한다.
    for case in cases:
        name, rgba, path, a, b = case
        results = []
        # 기본027f와 비교037f의 기대값을 모두 보존한다.
        for precision in (0x027f, 0x037f):
            result = oracle.sample(case, precision)
            results.append(result)
            rows.append((name, precision, rgba.hex(), *encode_gradient(a), *encode_gradient(b), *result))
        if results[0] != results[1]:
            different.append(name)
        if results[0][4:] != results[1][4:]:
            palette_different.append(name)
        print(name, '정상 반환', flush=True)
    if args.smoke:
        print('smoke:', len(cases), '입력', '정밀도 차이', len(different))
        return
    FIXTURE.write_text('# name cw rgbx aRgb aCount aWidth bRgb bCount bWidth aHeader aFields bHeader bFields merged2 merged3 merged1. 기본027f/비교037f 모두 실제 PE 결과.\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = {'originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', FIXTURE.relative_to(ROOT).as_posix()}  # 추가한 파일의 전체 바이트 근거다.
    paths.update(case[2] for case in cases if case[2])
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(cases), nativeGradientReturns=len(cases) * 4, nativeMergeReturns=len(cases) * 6, precisionDifferentInputs=different, precisionDifferentPalettes=palette_different, precisionSetup=dict(entry='004e81da', initial='037f', returned='023f', precisionBits='0200', roundingBits='0000', chain='CRT __cinit(1) → __fpmath → __setdefaultprecision', note='제어워드 전체가027f인 것은 아니며 연산 정밀도/반올림/예외 마스킹 비트가027f 관찰과 같다.', countedAsGradient=False), nativeCalls=dict(sorted(oracle.calls.items())), callbackRange=dict(entry='0043a660', exclusiveEnd='0043a79f', missingFromRefinedTable=True, basis='vtable00506688과 PE 명령의 ret4까지 확인한 범위'), sha256={name: digest(ROOT / name) for name in sorted(paths)}, limitations=['new/free의 성공·주소만 외부 입력이다. 색 선택·실제 CRT 정렬·팔레트 생성/병합은 원본 몸체다.', '80바이트 항목의 색 선택 의미48바이트를 비교한다. HSV/패딩과 팔레트 할당 뒤 인접 DWORD는 포함하지 않는다.', 'C++는기본027f를대조한다. 비교037f의 실제 출력도 보존하나 지원 완료로 세지 않는다.', 'Fiery Tail 프로세스·표시·실제 사용자 자연 완주의 실행 증거가 아니다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('ParticleGradient:', len(cases), '고유 입력', '색 띠 정상 반환', len(cases) * 4)


if __name__ == '__main__':
    main()
