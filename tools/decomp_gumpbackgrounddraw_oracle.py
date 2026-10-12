#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 배경 가상 Draw/전체 분배/질감/실제 단색·체크 픽셀/클립/CRT를 격리 x86으로 관찰한다.

Position·패치 타입/프레임 조회·VFX 그림/선/색 설치만 명시 외부 경계다.
실제 소프트웨어 fill과 CRT 절삭, CD Bounds/인라인 자산 조회는 원본 몸체를 실행한다.
게임/창/OS 실행 없이 fixture와 SHA/ABI/범위/보호/두 x87 모드 근거를 생성한다.
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
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 외부 OS 없이 쓰는 격리 입력/타입/SHP/픽셀과 정상 반환·스택 주소다.
DATA, NODE, TYPES, SHAPE, PIXELS = 0x19000000, 0x19000100, 0x19001000, 0x19010000, 0x19013000
STOP, STACK_BASE, STACK = 0x20000000, 0x30000000, 0x3000f000
# 작은 실제 출력 표면과 합성 입력 시드, x87 두 정밀도다.
PITCH, HEIGHT, SEED, CONTROLS = 32, 24, 0x465c60, (0x027f, 0x037f)
SPECS = {
    'originals': dict(draw=0x465c60, background=0x465880, tile=0x465690, position=0x464740,
        fill=0x4a2060, checker=0x4a2180, clipset=0x4a1580, clip=0x5c7a44, vfx=0x5b5de0,
        raw=0x4daa70, mapped=0x4daab0, line=0x4dade0, map=0x4dabe0,
        bright=0x556238, dark=0x556438, debug=0x565de0, debugcolor=0x565e40,
        near=0x5bec2c, far=0x5bec3c, flags=0x5c78d4, pitch=0x5c78ec, pixels=0x5c78f8,
        stride=500, typeget=0x49a840, frameget=0x419850),
    'originalCD': dict(draw=0x494e40, background=0x494a20, tile=0x494720, position=0x4934c0,
        fill=0x423960, checker=0x423a80, clipset=0x422e10, clip=0x552880, vfx=0x55bb1c,
        raw=0x4d64e0, mapped=0x4d6510, line=0x4d6850, map=0x4d6630,
        bright=0x56c7e8, dark=0x56e0e8, debug=0x535ea0, debugcolor=0x535ea4,
        near=0x55bad0, far=0x552df0, flags=0x516af0, pitch=0x516b0c, pixels=0x516b18,
        stride=468, typebase=0x51c960),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# fixture는 CTest가 실제 게임/원본/Python 없이 읽을 입력과 원본 출력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/gumpbackgrounddraw-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-gumpbackgrounddraw-evidence.json'


def signed(value):
    """임의 DWORD 비트를 signed 출력으로 읽는다. 기대 계산용 helper가 아니다."""
    return struct.unpack('<i', struct.pack('<I', value & 0xffffffff))[0]


def cases():
    """전체 배경 우선순위/테두리/명암/좌표·크기/디버그 입력만 만든다."""
    result = []
    rng = random.Random(SEED)
    flags_inputs = []
    # 무배경/단색/체크/동시 플래그의 모든 16개 테두리 조합이다.
    for edges in range(16):
        # checker가 solid와 texture보다 먼저 선택되는 입력도 포함한다.
        for mode in (0, 0x4000000, 0x8000000, 0xe000000):
            flags_inputs.append(mode | (edges << 21) | 0x91)
    # 질감의 pressed와 두 명암 비트, 전체 테두리를 독립 조합한다.
    for edges in range(16):
        # 들어오는 bright/dark 비트를 지우지 않는 원본 OR 동작이다.
        for shade in (0, 0x80000, 0x100000, 0x180000):
            # 눌림은 질감 테두리의 두 색 표 방향만 바꾼다.
            for pressed in (0, 0x40000):
                flags_inputs.append(0x2000000 | (edges << 21) | shade | pressed)
    # 모든 출력 입력에 직접/가상 경로·타입·프레임·잘린 화면 원점을 분배한다.
    for index, flags in enumerate(flags_inputs):
        x, y = rng.randrange(-13, 29), rng.randrange(-11, 20)
        width, height = rng.choice((0.0, 1.5, 17.75, 34.25)), rng.choice((0.0, 2.5, 9.75, 25.5))
        result.append((index % 3 != 0, x, y, width, height, flags, 82 + index % 2, index % 2,
            rng.choice((3, 7, 33)), rng.choice((3, 6, 15)), (0, 0, 32, 24) if index % 2 else (3, 2, 21, 17),
            0, 0x12345678, 0x1234ab, 0x9876cd))
    # 잘못된 타일 크기는 clip/map/shape 없는 조기 반환이다. 질감 함수의 매 조회는 여전히 발생한다.
    for width, height in ((1, 6), (7, 1), (0, 0), (-1, 7), (-32768, 32767)):
        result.append((1, 5, 6, 17.75, 9.75, 0x3e00000, 82, 0, width, height, (0, 0, 32, 24), 0, 77, 91, 93))
    # debug 분기의 이전 색 사용·DWORD 증가/감김과 실제 CRT 절삭의 큰 float32 폭을 관찰한다.
    for index in range(12):
        result.append((1, -3, 4, (17.75, 4294967296.0, 2147483648.0)[index % 3], 9.75,
            flags_inputs[index], 0, 0, 7, 6, (0, 0, 32, 24), 1,
            (0xffffffff, 0x100, 37)[index % 3], 91, 93))
    return result


class BackgroundDrawOracle:
    """아홉 원본 몸체의 정상 반환/두 x87 상태/전체 픽셀과 경계 호출을 감사한다."""
    def __init__(self, edition):
        """읽기 전용 PE와 내보낸 범위, 격리 타입/프레임/표면을 준비한다."""
        self.edition, self.spec = edition, SPECS[edition]
        binary = ROOT / BINARIES[edition]
        self.binary_sha = digest(binary)
        pe = pefile.PE(str(binary))
        raw = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(raw) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, raw)
        self.exports = [ROOT / f'extracted/gumpbackgrounddraw/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as stream:
            # 불연속 함수 몸체의 실제 코드 조각만 실행 범위에 포함한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 범위 사이 패딩을 실행 허용 영역으로 합치지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        # 실제 힙/장치를 실행하지 않고 합성 입력/정상 반환/스택만 추가한다.
        for address, size in ((DATA, 0x20000), (STOP, 0x1000), (STACK_BASE, 0x10000)):
            self.mu.mem_map(address, size)
        s = self.spec
        self.mu.mem_write(s['flags'], struct.pack('<I', 0x80))
        self.mu.mem_write(s['pitch'], struct.pack('<I', PITCH))
        self.mu.mem_write(s['pixels'], struct.pack('<I', PIXELS))
        if edition != 'originals':
            self.mu.mem_write(s['typebase'], struct.pack('<I', TYPES))
        self.calls, self.boundaries = collections.Counter(), collections.Counter()
        self.instructions = self.returns = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_code)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def words(self, address, count):
        """실제 x86 인자를 부호 없는 DWORD 비트로 읽는다."""
        return struct.unpack(f'<{count}I', self.mu.mem_read(address, count * 4))

    def event(self, name, values):
        """경계의 실제 인자 비트만 기록한다. 출력 좌표/순서 기대값은 계산하지 않는다."""
        self.trace.append(name + ',' + ','.join(map(str, values)))

    def ret(self, value=0, purge=0):
        """명시 외부 경계를 원본 ABI로 반환한다. CRT/소프트웨어 픽셀은 대체하지 않는다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        target, = self.words(sp, 1)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def on_code(self, _mu, address, size, _data):
        """자산/Position/VFX 경계만 대체하고 실제 fill/clip/CRT 진입은 그대로 실행한다."""
        s = self.spec
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        if address == s['position']:
            output, = self.words(sp + 4, 1)
            assert self.mu.reg_read(UC_X86_REG_ECX) == NODE
            assert STACK_BASE <= output <= STACK_BASE + 0x10000 - 8
            self.mu.mem_write(output, struct.pack('<2i', self.x, self.y))
            self.boundaries['position'] += 1
            self.ret(output, 4)
            return
        if address == s.get('typeget'):
            type_, = self.words(sp + 4, 1)
            assert type_ in (82, 83)
            self.boundaries['type'] += 1
            self.ret(TYPES + type_ * s['stride'])
            return
        if address == s.get('frameget'):
            type_ = (self.mu.reg_read(UC_X86_REG_ECX) - TYPES) // s['stride']
            abstract, frame = self.words(sp + 4, 2)
            assert type_ in (82, 83) and abstract == 0 and frame in (0, 1)
            self.boundaries['frame'] += 1
            self.ret(SHAPE + (type_ - 82) * 0x1000 + 0x100 + frame * 0x100, 8)
            return
        if address in (s['raw'], s['mapped']):
            shape, frame, x, y = self.words(sp + 4, 4)
            type_ = 82 + (shape - SHAPE) // 0x1000
            assert shape in (SHAPE, SHAPE + 0x1000) and frame in (0, 1)
            self.event('D', (type_, frame, x, y, self.shade if address == s['mapped'] else 0))
            self.boundaries['shape'] += 1
            self.ret()
            return
        if address == s['map']:
            pointer, = self.words(sp + 4, 1)
            assert pointer in (s['bright'], s['dark'])
            self.shade = 1 if pointer == s['bright'] else 2
            self.event('M', (self.shade,))
            self.boundaries['map'] += 1
            self.ret()
            return
        if address == s['line']:
            self.event('L', self.words(sp + 4, 5))
            self.boundaries['line'] += 1
            self.ret()
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'배경 Draw 허용 범위 밖 실행: {self.edition} {address:08x}')
        self.instructions += 1
        if address in self.entries:
            self.calls[f'{address:08x}'] += 1
        if address == s['tile']:
            self.event('T', self.words(sp + 24, 2))
        if address == s['clipset']:
            self.event('C', self.words(sp + 4, 4))
        if address in (s['fill'], s['checker']):
            self.event('F', (*self.words(sp + 4, 5), int(address == s['checker'])))

    def on_write(self, _mu, _access, address, size, _value, _data):
        """실제 픽셀/스택/클립·VFX 전역/디버그 색 외 모든 원본 쓰기를 거부한다."""
        s = self.spec
        ranges = ((STACK_BASE, STACK_BASE + 0x10000), (PIXELS, PIXELS + PITCH * HEIGHT),
            (s['clip'], s['clip'] + 16), (s['vfx'], s['vfx'] + 16), (s['debugcolor'], s['debugcolor'] + 4))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'배경 Draw 허용 범위 밖 쓰기: {address:08x}+{size}')

    def run(self, case, control):
        """같은 입력을 원본 전체 몸체에 공급하고 실제 사건/픽셀/최종 전역을 읽는다."""
        virtual, self.x, self.y, width, height, flags, type_, frame, tw, th, clip, debug, color, near, far = case
        s, mu = self.spec, self.mu
        node = bytearray(0xb8)
        struct.pack_into('<2f', node, 0x44, width, height)
        struct.pack_into('<6I', node, 0xa0, 0xdeadbeef, 123, 456, type_, frame, flags)
        mu.mem_write(NODE, bytes(node))
        readonly = [(NODE, bytes(node))]
        # CD의 실제 인라인 SHP 조회와 패치 조회 경계에 같은 포인터/헤더 입력을 공급한다.
        for number in (82, 83):
            cache = bytearray(s['stride'])
            shape = SHAPE + (number - 82) * 0x1000
            struct.pack_into('<I', cache, 0xdc, shape)
            address = TYPES + number * s['stride']
            mu.mem_write(address, bytes(cache))
            readonly.append((address, bytes(cache)))
            blob = bytearray(0x400)
            # 두 프레임의 36바이트 추가 헤더와 상대 VFX 테이블 주소를 그대로 만든다.
            for index in range(2):
                metadata = shape + 0x100 + index * 0x100
                struct.pack_into('<I', blob, 8 + index * 8, metadata + 36 - shape)
                struct.pack_into('<hh', blob, 0x100 + index * 0x100 + 24, tw, th)
            mu.mem_write(shape, bytes(blob))
            readonly.append((shape, bytes(blob)))
        # 화면/색/디버그 입력을 매 호출마다 초기화해 앞선 출력 상태가 다음 기대값에 섞이지 않게 한다.
        for name, value in (('debug', debug), ('debugcolor', color), ('near', near), ('far', far)):
            mu.mem_write(s[name], struct.pack('<I', value))
        mu.mem_write(s['clip'], struct.pack('<4i', *clip))
        mu.mem_write(s['vfx'], struct.pack('<4i', clip[0], clip[1], clip[2] - 1, clip[3] - 1))
        initial = bytes((i * 19 + 37) & 255 for i in range(PITCH * HEIGHT))
        guard = bytes(range(16))
        mu.mem_write(PIXELS - 16, guard + initial + guard)
        self.trace, self.shade = [], 0
        if virtual:
            entry, args = s['draw'], ()
        else:
            entry = s['background']
            args = (self.x, self.y, signed(int(self.x + width)), signed(int(self.y + height)), flags, type_, frame)
        mu.mem_write(STACK, struct.pack(f'<{len(args) + 1}I', STOP, *(value & 0xffffffff for value in args)))
        mu.reg_write(UC_X86_REG_ECX, NODE)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, control)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        saved = ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789),
            (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab))
        # 모든 callee-save 레지스터와 정상 반환/스택 purge를 확인한다.
        for register, value in saved:
            mu.reg_write(register, value)
        mu.emu_start(entry, STOP, count=1000000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert mu.reg_read(UC_X86_REG_FPCW) == control and mu.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0
        # 원본 타입/SHP/노드 입력은 전체 바이트가 불변이어야 한다.
        for register, value in saved:
            assert mu.reg_read(register) == value
        for address, raw in readonly:
            assert bytes(mu.mem_read(address, len(raw))) == raw
        assert self.words(s['clip'], 4) == tuple(value & 0xffffffff for value in clip)
        assert bytes(mu.mem_read(PIXELS - 16, 16)) == guard
        assert bytes(mu.mem_read(PIXELS + PITCH * HEIGHT, 16)) == guard
        self.returns += 1
        return (';'.join(self.trace) or '-', self.words(s['debugcolor'], 1)[0], self.shade,
            bytes(mu.mem_read(PIXELS, PITCH * HEIGHT)).hex())


def verify():
    """원본/내보내기/도구/fixture SHA·행/정상 반환·명시 사건 수를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    assert report['script_sha256'] == digest(Path(__file__))
    assert report['fixture_sha256'] == digest(FIXTURE)
    assert report['plan_sha256'] == digest(ROOT / 'tools/ghidra/gumpbackgrounddraw-functions.json')
    counts, events = collections.Counter(), collections.Counter()
    # 원본 사건 문자열에서 실제 진입/장치 경계 수를 다시 집계한다.
    for line in FIXTURE.read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or not line:
            continue
        row = line.split('\t')
        counts[row[0]] += 1
        assert len(bytes.fromhex(row[-1])) == PITCH * HEIGHT
        # 사건 종류별 기록 수를 native 진입/외부 경계 수와 교차 대조한다.
        for event in row[-4].split(';'):
            events[row[0], event[0]] += 1
    # 두 x87 실행은 저장 한 행의 두 독립 정상 반환이며 고유 입력 수에 중복 합산하지 않는다.
    for edition, evidence in report['editions'].items():
        s = SPECS[edition]
        assert counts[edition] == evidence['cases']
        assert evidence['normal_returns'] == counts[edition] * len(CONTROLS)
        assert evidence['binary_sha256'] == digest(ROOT / BINARIES[edition])
        for name, event in (('tile', 'T'), ('clipset', 'C')):
            assert evidence['calls'].get(f'{s[name]:08x}', 0) == events[edition, event] * len(CONTROLS)
        for name, event in (('shape', 'D'), ('map', 'M'), ('line', 'L')):
            assert evidence['boundaries'].get(name, 0) == events[edition, event] * len(CONTROLS)
        assert sum(evidence['calls'].get(f'{s[name]:08x}', 0) for name in ('fill', 'checker')) == events[edition, 'F'] * len(CONTROLS)
        for path, sha in evidence['exports'].items():
            assert digest(ROOT / path) == sha
    assert report['controls'] == list(CONTROLS) and sum(counts.values()) == report['total_cases']
    print(f'배경 Draw 근거 감사 통과: {sum(counts.values())}개')


def main():
    """독립 원본 관찰을 생성하거나 저장 근거를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
        return
    inputs = cases()
    rows = ['# 배경 가상/직접 출력 입력과 실제 PE의 사건/디버그색/현재 명암/소프트웨어 픽셀.']
    evidence = {}
    # 세 실제 PE의 각 입력을 두 x87 모드에서 실행하고 모든 출력을 대조한다.
    for edition in SPECS:
        oracle = BackgroundDrawOracle(edition)
        for case in inputs:
            first, second = (oracle.run(case, control) for control in CONTROLS)
            assert first == second, f'배경 Draw x87 모드 차이: {edition} {case}'
            virtual, x, y, width, height, flags, type_, frame, tw, th, clip, debug, color, near, far = case
            bits = struct.unpack('<2I', struct.pack('<2f', width, height))
            row = (edition, 'Virtual' if virtual else 'Direct', x, y, *bits, flags, type_, frame, tw, th,
                ','.join(map(str, clip)), debug, color, near, far, *first)
            rows.append('\t'.join(map(str, row)))
        evidence[edition] = dict(binary_sha256=oracle.binary_sha, cases=len(inputs), normal_returns=oracle.returns,
            instructions=oracle.instructions, calls=dict(oracle.calls), boundaries=dict(oracle.boundaries),
            exports={str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in oracle.exports})
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    report = dict(description='실제 배경 가상 Draw/전체 분배/질감/소프트웨어 fill/clip/CRT. Position·패치 조회·VFX 그림/선/색 설치만 명시 경계.',
        hostname=socket.gethostname(), last_decompile_host='VM-W11-CODEX', date='2026-10-12', controls=list(CONTROLS),
        seed=SEED, total_cases=len(inputs) * len(SPECS), fixture_sha256=digest(FIXTURE), script_sha256=digest(Path(__file__)),
        plan_sha256=digest(ROOT / 'tools/ghidra/gumpbackgrounddraw-functions.json'), editions=evidence)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    verify()


if __name__ == '__main__':
    main()
