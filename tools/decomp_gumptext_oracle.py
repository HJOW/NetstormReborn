#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 실제 글자 측정/기본 Gump 위임을 격리 x86으로 실행해 C++ 기대값을 저장한다.

폭 표와 슬롯 높이는 명시 입력이다. 원본 함수·내부 strlen 루프를 대체하지 않는다.
게임 프로세스, GUI, OS API는 실행하지 않는다. --verify는 저장한 근거의 SHA와 행 수만 검사한다.
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

# 합성 입력만 만드는 시드·바이트 표 크기·원본 별도 슬롯 높이다. 출력은 실제 PE가 결정한다.
SEED, TABLE_STRIDE = 0x1078a3df, 0x1410
HEIGHTS = (14, 13, 0, 20, 48, 14, 12)
# 격리 데이터/반환/스택은 실제 PE 주소 및 서로와 겹치지 않는다.
DATA, STOP, STACK_BASE, STACK = 0x19000000, 0x20000000, 0x30000000, 0x3000f000
OUTPUT, CONTEXT, NODE, TEXT = DATA + 0x100, DATA + 0x200, DATA + 0x300, DATA + 0x1000
# 판본별 전체 측정 몸체·기본 Gump 위임·캐시 표/별도 높이 전역이다.
SPECS = {
    'originals': dict(measure=0x4a3df0, delegate=0x465020, height=0x5c7a04,
        tables=(0x5a4068, 0x5ad130, 0x5b5e70, 0x59b3e8, 0x5bec40)),
    'originalCD': dict(measure=0x425f90, delegate=0x493ea0, height=0x516c10,
        tables=(0x549b58, 0x55bf58, 0x552df8)),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 저장한 fixture는 원본 파일/Python 없이 C++ 검사에서 읽는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/gumptext-x86.tsv'
REPORT = ROOT / 'cpppj/gumptext.json'


def available(slot, style):
    """10.78 초기화에서 공급할 명시 캐시 입력의 슬롯/스타일 조합을 정한다."""
    return slot != 2 and (style == 0 or slot in (0, 5, 6))


def font_inputs():
    """캐시의 첫 폭 표에만 임의 signed DWORD를 공급한다. 기대 폭을 계산하지 않는다."""
    rng = random.Random(SEED)
    result = {}
    # 지원 조합의 각 바이트 폭을 독립 난수와 극단 정수로 채워 부호/감김 오류를 노출한다.
    for slot in range(7):
        # 스타일마다 다른 표를 사용해 잘못된 선택을 결과에 드러낸다.
        for style in range(5):
            if available(slot, style):
                values = [rng.getrandbits(32) for _ in range(256)]
                values[0] = 0x70000000 + slot * 17 + style
                values[65] = 0x7fffffff - slot * 101 - style * 13
                result[slot, style] = struct.pack('<256I', *values)
    return result


def cases():
    """NUL/명시 길이·음수·그림자·빈 슬롯·잘못된 스타일·출력 alias 입력만 만든다."""
    result = []
    # 모든 슬롯과 정상/범위 밖 스타일에 짧은 NUL 종료 바이트열을 공급한다.
    for slot in range(7):
        # flags 4(외곽선)와 최상위 비트도 측정의 그림자 bit 2와 구분한다.
        for style in (0, 1, 2, 3, 4, 5, 0xffffffff):
            # 명시 길이 4는 중간 NUL/255도 포함하며 -2 및 INT_MIN은 읽을 글자가 없다.
            for index, length in enumerate((-1, 0, 4, -2, -0x80000000)):
                result.append((slot, style, (0, 2, 4, 6, 0x80000002)[index],
                    (1, -3, 0x7fffffff, -0x80000000, 17)[index],
                    (-1, 5, -0x80000000, 0x7fffffff, -19)[index],
                    length, (slot + style + index) % 2, bytes((65, 98, 0, 255, 67, 63, 0))))
    rng = random.Random(SEED + 1)
    # 같은 난수 문자열에 유한 길이와 NUL 탐색을 섞고 두 출력의 alias도 독립적으로 공급한다.
    for _ in range(60):
        text = bytes(rng.randrange(1, 256) for _ in range(rng.randrange(1, 24))) + b'\0'
        result.append((rng.randrange(7), rng.choice((0, 1, 2, 3, 4, 99)), rng.getrandbits(32),
            rng.randrange(-0x80000000, 0x80000000), rng.randrange(-0x80000000, 0x80000000),
            rng.choice((-1, len(text), 1)), rng.randrange(2), text))
    return result


class TextOracle:
    """읽기 전용 실제 몸체만 실행하며 출력/스택 이외의 쓰기와 ABI 손상을 거부한다."""
    def __init__(self, edition, fonts):
        """원본 PE와 Ghidra 범위를 읽고 명시 캐시/높이 표를 가상 메모리에 준비한다."""
        self.edition, self.spec = edition, SPECS[edition]
        binary = ROOT / BINARIES[edition]
        self.binary_sha = digest(binary)
        pe = pefile.PE(str(binary))
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
        self.exports = [ROOT / f'extracted/gumptext/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as stream:
            # 내보낸 불연속 함수 범위 밖 코드 진입을 허용하지 않는다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 함수 하나의 모든 body 조각을 명시적으로 포함한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        # 별도 데이터·정상 반환 주소·스택을 각각 격리한다.
        for address, size in ((DATA, 0x2000), (STOP, 0x1000), (STACK_BASE, 0x10000)):
            self.mu.mem_map(address, size)
        self.readonly = []
        # 실제 스타일 은행 전체를 준비한다. 원본이 생성하지 않는 조합은 빈 전역 캐시다.
        for style, base in enumerate(self.spec['tables']):
            # 슬롯별 0x1410 캐시의 머리 다음 첫 폭 표만 명시 입력으로 쓴다.
            for slot in range(7):
                raw = bytearray(TABLE_STRIDE)
                profile = fonts.get((slot, style))
                if edition != 'originals' and slot == 6 and style == 2:
                    profile = None
                if profile:
                    raw[16:1040] = profile
                address = base + slot * TABLE_STRIDE
                self.mu.mem_write(address, bytes(raw))
                self.readonly.append((address, bytes(raw)))
        heights = struct.pack('<7i', *HEIGHTS)
        self.mu.mem_write(self.spec['height'], heights)
        self.readonly.append((self.spec['height'], heights))
        self.calls = collections.Counter()
        self.instructions = self.returns = 0
        self.mu.hook_add(UC_HOOK_CODE, self.on_code)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def on_code(self, _mu, address, size, _data):
        """OS/CRT 대체 없이 두 내보낸 실제 함수 범위 안의 명령만 허용한다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'GumpText 허용 범위 밖 실행: {self.edition} {address:08x}')
        self.instructions += 1
        if address in self.entries:
            self.calls[f'{address:08x}'] += 1

    def on_write(self, _mu, _access, address, size, _value, _data):
        """현재 출력 두 DWORD와 스택만 쓰게 한다. 캐시/문맥/노드는 읽기 전용이다."""
        if STACK_BASE <= address and address + size <= STACK_BASE + 0x10000:
            return
        if OUTPUT <= address and address + size <= self.output_end:
            return
        raise RuntimeError(f'GumpText 허용 범위 밖 쓰기: {address:08x}+{size}')

    def run(self, case, mode):
        """직접/기본 문맥 위임/명시 문맥 위임의 실제 반환 결과와 보호 상태를 읽는다."""
        slot, style, flags, sx, sy, length, alias, text = case
        context = struct.pack('<6I2i', flags, slot, 0x12345678, style, 0x99999999, 0xabcdef01, sx, sy)
        node = bytes(128) + context
        self.mu.mem_write(CONTEXT, context)
        self.mu.mem_write(NODE, node)
        self.mu.mem_write(TEXT, text)
        guard = bytes((i * 19 + 37) & 255 for i in range(32))
        self.mu.mem_write(OUTPUT - 8, guard)
        height_out = OUTPUT if alias else OUTPUT + 4
        self.output_end = height_out + 4
        args = (TEXT, OUTPUT, height_out, CONTEXT if mode != 'Default' else 0, length & 0xffffffff)
        entry = self.spec['measure'] if mode == 'Direct' else self.spec['delegate']
        purge = 0 if mode == 'Direct' else 20
        sp = STACK
        self.mu.mem_write(sp, struct.pack('<6I', STOP, *args))
        self.mu.reg_write(UC_X86_REG_ESP, sp)
        self.mu.reg_write(UC_X86_REG_ECX, NODE)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        saved = ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789),
            (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab))
        # callee-save 레지스터를 서로 다른 값으로 두어 정상 반환의 ABI를 확인한다.
        for register, value in saved:
            self.mu.reg_write(register, value)
        self.mu.emu_start(entry, STOP, count=50000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != sp + 4 + purge:
            raise RuntimeError('GumpText 반환/스택 ABI 오류')
        # 모든 callee-save 값과 readonly 입력을 각 호출 뒤 전수 대조한다.
        for register, value in saved:
            if self.mu.reg_read(register) != value:
                raise RuntimeError('GumpText callee-save ABI 오류')
        # 캐시·높이 전역·문맥·노드·문자열 중 어떤 것도 바뀌면 실패한다.
        for address, raw in (*self.readonly, (CONTEXT, context), (NODE, node), (TEXT, text)):
            if bytes(self.mu.mem_read(address, len(raw))) != raw:
                raise RuntimeError('GumpText 읽기 전용 입력 변경')
        result_guard = bytes(self.mu.mem_read(OUTPUT - 8, 32))
        if result_guard[:8] != guard[:8] or result_guard[8 + self.output_end - OUTPUT:] != guard[8 + self.output_end - OUTPUT:]:
            raise RuntimeError('GumpText 출력 보호 바이트 변경')
        width = struct.unpack('<i', self.mu.mem_read(OUTPUT, 4))[0]
        height = struct.unpack('<i', self.mu.mem_read(height_out, 4))[0]
        self.returns += 1
        return width, height


def verify():
    """기록한 원본/내보내기/fixture/도구 SHA와 입력·관찰 행 수를 검사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    assert report['script_sha256'] == digest(Path(__file__))
    assert report['fixture_sha256'] == digest(FIXTURE)
    assert report['plan_sha256'] == digest(ROOT / 'tools/ghidra/gumptext-functions.json')
    counts = collections.Counter()
    # 폰트 입력 행은 관찰 수로 계산하지 않는다.
    for line in FIXTURE.read_text(encoding='utf-8').splitlines():
        if line.startswith('Case\t'):
            counts[line.split('\t')[1]] += 1
    # 세 원본 SHA/두 내보내기 SHA/정상 반환·행 수를 모두 확인한다.
    for edition, evidence in report['editions'].items():
        assert evidence['binary_sha256'] == digest(ROOT / BINARIES[edition])
        assert counts[edition] == evidence['cases'] == evidence['normal_returns']
        for path, sha in evidence['exports'].items():
            assert digest(ROOT / path) == sha
    assert sum(counts.values()) == report['total_cases']
    print(f'GumpText 근거 감사 통과: {sum(counts.values())}개')


def main():
    """새 native 관찰을 기록하거나 --verify로 기존 결과만 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify()
        return
    fonts, inputs = font_inputs(), cases()
    lines = ['# GumpText: Font 입력(slot/style/캐시높이/첫 폭 표); Case는 실제 PE 출력(width/height).']
    # C++에 그대로 공급할 모든 지원 조합의 원본 폭 표 입력이다.
    for (slot, style), profile in fonts.items():
        lines.append(f'Font\t{slot}\t{style}\t{31 + slot}\t{profile.hex()}')
    evidence = {}
    # 세 판본의 동일 명시 입력을 직접/위임 경로에 번갈아 공급한다.
    for edition in SPECS:
        oracle = TextOracle(edition, fonts)
        for index, case in enumerate(inputs):
            mode = ('Direct', 'Default', 'Override')[index % 3]
            width, height = oracle.run(case, mode)
            lines.append('\t'.join(map(str, ('Case', edition, mode, *case[:-1], case[-1].hex(), width, height))))
        evidence[edition] = dict(binary_sha256=oracle.binary_sha, cases=len(inputs),
            normal_returns=oracle.returns, instructions=oracle.instructions, calls=dict(oracle.calls),
            exports={str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in oracle.exports})
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    report = dict(description='실제 정수 몸체·strlen·Gump 위임. 대체 함수/게임/GUI/OS 호출 0.',
        hostname=socket.gethostname(), last_decompile_host='VM-W11-CODEX', date='2026-10-12',
        seed=SEED, total_cases=len(inputs) * len(SPECS), font_inputs=len(fonts),
        fixture_sha256=digest(FIXTURE), script_sha256=digest(Path(__file__)),
        plan_sha256=digest(ROOT / 'tools/ghidra/gumptext-functions.json'), editions=evidence)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    verify()


if __name__ == '__main__':
    main()
