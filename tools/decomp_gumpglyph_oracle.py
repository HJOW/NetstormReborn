#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 글리프 반복·그림자/외곽선·Gump wrapper를 격리 x86으로 관찰한다.

VFX 색 표 설치/셰이프 출력, 이미 검증한 Position, 잘못된 스타일 로그만 명시 경계다.
문자열 길이/표 선택/포인터 검사/폭 전진/패스 좌표는 원본 몸체를 실행한다.
게임 프로세스·창·OS API는 실행하지 않는다. --verify는 저장 SHA/행/호출 수를 감사한다.
"""
import argparse
import collections
import csv
import json
import random
import socket
import struct
from pathlib import Path
from decomp_process_oracle import (ROOT, BINARIES, digest, pefile, Uc, UC_ARCH_X86, UC_MODE_32,
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 실제 PE와 겹치지 않는 명시 입력·반환·스택 주소다. 글리프 포인터는 VFX 경계에서만 해석한다.
DATA, CONTEXT, NODE, TEXT, SHAPES = 0x19000000, 0x19000100, 0x19000200, 0x19001000, 0x18000000
STOP, STACK_BASE, STACK = 0x20000000, 0x30000000, 0x3000f000
# 입력 난수 시드와 원본 메모리 캐시 간격이다. 기대 좌표/출력 순서는 계산하지 않는다.
SEED, STRIDE = 0x10783430, 0x1410
SPECS = {
    'originals': dict(loop=0x4a3430, draw=0x4a3f70, wrapper=0x465050,
        position=0x464740, color=0x4dabe0, glyph=0x4daab0, log=0x4c2630, map=0x5beae0,
        tables=(0x5a4068, 0x5ad130, 0x5b5e70, 0x59b3e8, 0x5bec40)),
    'originalCD': dict(loop=0x426250, draw=0x4260f0, wrapper=0x493ed0,
        position=0x4934c0, color=0x4d6630, glyph=0x4d6510, log=0x48ca70, map=0x552898,
        tables=(0x549b58, 0x55bf58, 0x552df8)),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# CTest가 원본/Python 없이 읽을 입력/실제 관찰과 감사 보고서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/gumpglyph-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-gumpglyph-evidence.json'


def font_inputs():
    """임의 세 번째 폭 표와 글리프 포인터 유무를 만든다. 첫 폭/ABC A는 다른 값이다."""
    rng = random.Random(SEED)
    result = {}
    # 원본 초기화가 생성하는 각 슬롯/스타일 입력을 독립적으로 준비한다.
    for slot in range(7):
        # 슬롯 1/3/4는 normal만, 슬롯 2는 모든 표가 비어 있다.
        for style in range(5):
            if slot == 2 or (style != 0 and slot not in (0, 5, 6)):
                continue
            advances = [rng.choice((0, 3, -7, 11, 0x7fffffff, -0x80000000)) for _ in range(256)]
            mask = bytearray(rng.randrange(4) != 0 for _ in range(256))
            mask[0] = mask[32] = mask[65] = mask[255] = 1
            result[slot, style] = (struct.pack('<256i', *advances), bytes(mask))
    return result


def cases():
    """전체 슬롯/스타일·출력 패스·길이·빈 문자열·좌표 감김의 입력만 생성한다."""
    result = []
    # flags의 높은 비트와 extra 필드가 출력 패스에 영향을 주지 않는 입력도 포함한다.
    for slot in range(7):
        # 5/99는 두 판본 모두 fallback, 3/4는 패치에서만 정상 스타일이다.
        for style in (0, 1, 2, 3, 4, 5, 99):
            # 그림자와 외곽선의 독립 조합 네 가지다.
            for flags in (0, 2, 4, 0x80000006):
                result.append((slot, style, flags, 0x1234ab, 0x9876cd, -3, 2,
                    -0x80000000 if flags & 4 else 23, 0x7fffffff if flags & 2 else -17,
                    -1, bytes((65, 32, 127, 255, 98, 0, 67, 0))))
    rng = random.Random(SEED + 1)
    # 빈/명시 길이 0/중간 NUL/전체 길이와 랜덤 signed 좌표를 섞는다.
    for index in range(60):
        raw = bytes((65, 0, 255, 32, 98, 127, 0)) if index % 4 else b'\0'
        result.append((rng.randrange(7), rng.choice((0, 1, 2, 3, 4, 99)), rng.getrandbits(32),
            rng.getrandbits(32), rng.getrandbits(32), rng.randrange(-4, 5), rng.randrange(-4, 5),
            rng.randrange(-0x80000000, 0x80000000), rng.randrange(-0x80000000, 0x80000000),
            rng.choice((-1, 0, len(raw))), raw))
    return result


class GlyphOracle:
    """실제 세 몸체만 실행하고 네 외부 경계/ABI/쓰기/읽기 전용 입력을 감사한다."""
    def __init__(self, edition, fonts):
        """원본 PE와 읽기 전용 내보내기 범위, 별도 입력 메모리를 준비한다."""
        self.edition, self.spec = edition, SPECS[edition]
        binary = ROOT / BINARIES[edition]
        self.binary_sha = digest(binary)
        pe = pefile.PE(str(binary))
        raw = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(raw) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, raw)
        self.exports = [ROOT / f'extracted/gumpglyph/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as stream:
            # Ghidra의 불연속 몸체를 합치지 않고 각각 실행 한도로 지정한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 함수의 실제 코드 조각만 허용한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        # 실제 게임의 힙/OS 없이 입력/반환/스택만 추가한다.
        for address, size in ((DATA, 0x2000), (STOP, 0x1000), (STACK_BASE, 0x10000)):
            self.mu.mem_map(address, size)
        self.readonly = []
        self.pointers = {}
        # 판본의 스타일 은행 전체를 입력으로 준비한다. 빈 조합은 실제 null 포인터 표다.
        for style, base in enumerate(self.spec['tables']):
            # 캐시 머리와 세 전진/ABC 표, 글리프 포인터 표를 각각 공급한다.
            for slot in range(7):
                cache = bytearray(STRIDE)
                profile = fonts.get((slot, style))
                if edition != 'originals' and slot == 6 and style == 2:
                    profile = None
                if profile:
                    advances, mask = profile
                    cache[16:1040] = struct.pack('<256i', *([99] * 256))
                    cache[0x410:0x810] = struct.pack('<256i', *([-31] * 256))
                    cache[0x810:0xc10] = advances
                    # VFX 호출에 도달한 포인터가 선택 슬롯/스타일/바이트를 독립적으로 알려준다.
                    for code, present in enumerate(mask):
                        if present:
                            pointer = SHAPES + ((slot * 5 + style) * 256 + code) * 4
                            struct.pack_into('<I', cache, 0x1010 + code * 4, pointer)
                            self.pointers[pointer] = (slot, style, code)
                address = base + slot * STRIDE
                self.mu.mem_write(address, bytes(cache))
                self.readonly.append((address, bytes(cache)))
        self.calls, self.boundaries = collections.Counter(), collections.Counter()
        self.instructions = self.returns = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_code)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def words(self, address, count):
        """가상 메모리의 x86 DWORD 인자를 읽는다."""
        return struct.unpack(f'<{count}I', self.mu.mem_read(address, count * 4))

    def ret(self, purge=0):
        """명시 경계의 원본 호출 규약대로 반환한다. 출력 기대값을 계산하지 않는다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        target, = self.words(sp, 1)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def on_code(self, _mu, address, size, _data):
        """경계 당시 실제 인자를 기록하고 그 외에는 내보낸 원본 명령만 실행한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        if address == self.spec['color']:
            pointer, = self.words(sp + 4, 1)
            assert pointer == self.spec['map']
            self.ink = self.mu.mem_read(pointer + 100, 1)[0]
            self.trace.append(f'C,{self.ink}')
            self.boundaries['color'] += 1
            self.ret()
            return
        if address == self.spec['glyph']:
            pointer, frame, x, y = self.words(sp + 4, 4)
            assert frame == 0 and pointer in self.pointers
            slot, style, code = self.pointers[pointer]
            self.trace.append(f'G,{slot},{style},{code},{x},{y},{self.ink}')
            self.boundaries['glyph'] += 1
            self.ret()
            return
        if address == self.spec['position']:
            output, = self.words(sp + 4, 1)
            assert self.mu.reg_read(UC_X86_REG_ECX) == NODE
            assert STACK_BASE <= output <= STACK_BASE + 0x10000 - 8
            self.mu.mem_write(output, struct.pack('<2i', *self.position))
            self.boundaries['position'] += 1
            self.ret(4)
            return
        if address == self.spec['log']:
            severity, = self.words(sp + 4, 1)
            assert severity == 29
            self.trace.append('E')
            self.boundaries['log'] += 1
            self.ret()
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'GumpGlyph 허용 범위 밖 실행: {self.edition} {address:08x}')
        self.instructions += 1
        if address in self.entries:
            self.calls[f'{address:08x}'] += 1

    def on_write(self, _mu, _access, address, size, _value, _data):
        """원본 글꼴 색 표의 100번 BYTE와 스택 외 쓰기를 거부한다."""
        if STACK_BASE <= address and address + size <= STACK_BASE + 0x10000:
            return
        if address == self.spec['map'] + 100 and size == 1:
            return
        raise RuntimeError(f'GumpGlyph 허용 범위 밖 쓰기: {address:08x}+{size}')

    def run(self, case, mode):
        """직접/기본 문맥/명시 문맥의 원본 정상 반환과 호출 기록을 읽는다."""
        slot, style, flags, color, shadow, sx, sy, x, y, length, text = case
        context = struct.pack('<6I2i', flags, slot, color, style, 0xabcdef01, shadow, sx, sy)
        node = bytes(128) + context
        self.mu.mem_write(CONTEXT, context)
        self.mu.mem_write(NODE, node)
        self.mu.mem_write(TEXT, text)
        color_map = bytes((i * 7 + 11) & 255 for i in range(256))
        self.mu.mem_write(self.spec['map'], color_map)
        self.position = (-13, 27)
        self.trace = []
        if mode == 'Direct':
            entry, purge = self.spec['draw'], 0
            args = (TEXT, x & 0xffffffff, y & 0xffffffff, CONTEXT, length & 0xffffffff)
        else:
            entry, purge = self.spec['wrapper'], 16
            args = (TEXT, x & 0xffffffff, y & 0xffffffff, 0 if mode == 'Default' else CONTEXT)
        self.mu.mem_write(STACK, struct.pack(f'<{len(args) + 1}I', STOP, *args))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, NODE)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        saved = ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789),
            (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab))
        # callee-save 전체를 서로 다른 값으로 초기화한다.
        for register, value in saved:
            self.mu.reg_write(register, value)
        self.mu.emu_start(entry, STOP, count=50000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + purge
        # 정상 반환의 ABI와 전체 캐시/문맥/노드/문자열 불변을 전수 확인한다.
        for register, value in saved:
            assert self.mu.reg_read(register) == value
        # boundary에서 쓴 Position 출력은 스택 이외 입력을 바꾸지 않는다.
        for address, raw in (*self.readonly, (CONTEXT, context), (NODE, node), (TEXT, text)):
            assert bytes(self.mu.mem_read(address, len(raw))) == raw
        actual = bytes(self.mu.mem_read(self.spec['map'], 256))
        assert actual[:100] == color_map[:100] and actual[101:] == color_map[101:]
        assert actual[100] == color & 255
        self.returns += 1
        return ';'.join(self.trace)


def verify():
    """원본/내보내기/도구/입력 SHA와 관찰·정상 반환·패스/경계 호출 수를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    assert report['script_sha256'] == digest(Path(__file__))
    assert report['fixture_sha256'] == digest(FIXTURE)
    assert report['plan_sha256'] == digest(ROOT / 'tools/ghidra/gumpglyph-functions.json')
    counts, events, modes = collections.Counter(), collections.Counter(), collections.Counter()
    # fixture 출력 행의 실제 사건 수와 실행 기록의 경계 수를 따로 대조한다.
    for line in FIXTURE.read_text(encoding='utf-8').splitlines():
        if not line.startswith('Case\t'):
            continue
        row = line.split('\t')
        edition = row[1]
        counts[edition] += 1
        modes[edition, row[2]] += 1
        # 색 설치/글리프/로그 관찰을 집계한다. 빈 문자열도 C 사건을 갖는다.
        for event in row[-1].split(';'):
            events[edition, event[0]] += 1
    # 모든 판본의 SHA·함수 진입·반환·경계 수는 저장 행과 일치해야 한다.
    for edition, evidence in report['editions'].items():
        spec = SPECS[edition]
        assert evidence['binary_sha256'] == digest(ROOT / BINARIES[edition])
        assert counts[edition] == evidence['cases'] == evidence['normal_returns']
        assert evidence['calls'][f'{spec["draw"]:08x}'] == counts[edition]
        assert evidence['calls'][f'{spec["loop"]:08x}'] == events[edition, 'C'] == evidence['boundaries']['color']
        assert evidence['boundaries']['glyph'] == events[edition, 'G']
        assert evidence['boundaries']['log'] == events[edition, 'E']
        assert evidence['calls'][f'{spec["wrapper"]:08x}'] == evidence['boundaries']['position'] == modes[edition, 'Default'] + modes[edition, 'Override']
        # 내보낸 세 몸체의 코드/범위 SHA도 재확인한다.
        for path, sha in evidence['exports'].items():
            assert digest(ROOT / path) == sha
    assert sum(counts.values()) == report['total_cases']
    print(f'GumpGlyph 근거 감사 통과: {sum(counts.values())}개')


def main():
    """새 관찰을 생성하거나 --verify로 저장 근거만 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
        return
    fonts, inputs = font_inputs(), cases()
    lines = ['# Font는 세 번째 전진 폭/글리프 유무 입력. Case 끝 열은 실제 PE의 C/G/E 호출 기록이다.']
    # C++가 실제 FontStore 캐시에 공급할 입력 표다. 결과 기대값은 포함하지 않는다.
    for (slot, style), (advances, mask) in fonts.items():
        lines.append(f'Font\t{slot}\t{style}\t{advances.hex()}\t{mask.hex()}')
    evidence = {}
    # 세 PE의 동일 입력을 독립적으로 실행한다. 위임 경로는 원본의 길이 -1을 따른다.
    for edition in SPECS:
        oracle = GlyphOracle(edition, fonts)
        # 모든 슬롯/스타일을 직접·기본·명시 문맥 경로에 분배한다.
        for index, case in enumerate(inputs):
            mode = ('Direct', 'Default', 'Override')[index % 3] if case[-2] == -1 else 'Direct'
            trace = oracle.run(case, mode)
            lines.append('\t'.join(map(str, ('Case', edition, mode, *case[:-1], case[-1].hex(), trace))))
        evidence[edition] = dict(binary_sha256=oracle.binary_sha, cases=len(inputs), normal_returns=oracle.returns,
            instructions=oracle.instructions, calls=dict(oracle.calls), boundaries=dict(oracle.boundaries),
            exports={str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in oracle.exports})
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    report = dict(description='실제 글리프 반복/표 선택/길이/패스/Gump wrapper. VFX 색 설치/그림·Position·로그만 명시 경계.',
        hostname=socket.gethostname(), last_decompile_host='VM-W11-CODEX', date='2026-10-12', seed=SEED,
        total_cases=len(inputs) * len(SPECS), font_inputs=len(fonts), fixture_sha256=digest(FIXTURE),
        script_sha256=digest(Path(__file__)), plan_sha256=digest(ROOT / 'tools/ghidra/gumpglyph-functions.json'), editions=evidence)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    verify()


if __name__ == '__main__':
    main()
