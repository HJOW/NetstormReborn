#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 파일 팔레트 로더와 전체 색 검색을 제한 x86으로 실행한다. 게임/OS는 실행하지 않는다."""
import argparse
import collections
import csv
import json
import random
import re
import struct
from decomp_owner_oracle import ROOT, SPECS, OwnerOracle, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 로더/검색과 파일·할당·OS 팔레트 경계의 판본별 주소다.
SPEC = {
    'originals': dict(entry=0x4a4850, nearest=0x4a2820, saved=0x5acd28, logical=0x5c7954,
        basic=0x54d928, count=0x531858, dirty=0x555e08, mode=0x5c78d4, weather=0x5b5da8,
        brown=0x5acd24, open=0x41b4c0, size=0x41aa80, seek=0x41a700, read=0x41a7a0,
        apply=0x4a2640, alloc=0x4e4391, close=0x41b410),
    'originalCD': dict(entry=0x424760, nearest=0x4246e0, saved=0x55bb48, logical=0x516b7c,
        basic=0x539570, count=0x539574, dirty=0x52055c, mode=0x516af0, weather=0x549b40,
        brown=0x55bacc, open=0x4ba700, size=0x4bb2d0, seek=0x4bafa0, read=0x4bb030,
        apply=0x424500, alloc=0x4f1650, close=0x4ba5a0)
}
# 10.37은 CD판과 함수/전역 배치가 같다.
SPEC['original1037'] = dict(SPEC['originalCD'])
# 합성 논리 팔레트·기본색 배열·파일 이름은 호스트와 무관한 에뮬레이터 주소다.
LOGICAL, BASIC, FILENAME = TYPES + 0x10000, TYPES + 0x11000, TYPES + 0x12000
# 입력 시드·두 x87 정밀도·표본 수 및 출력 위치다. 기대값은 실제 명령이 결정한다.
SEED, CONTROLS, SAMPLES = 0x4a4850, (0x027f, 0x037f), 8
FIXTURE = ROOT / 'cpppj/tests/fixtures/palettecolors-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-palettecolors-evidence.json'


def inputs():
    """동률·회색·임의 색·예약 바이트와 두 파일 형식/할당 경로를 조합한다. 기대값을 계산하지 않는다."""
    rng = random.Random(SEED)
    cases = []
    # 네 색 분포마다 RGB COL과 BGRX 파일을 각각 만든다.
    for profile in range(4):
        rgb = [(73, 101, 129)] * 256 if profile == 0 else [(i, i, i) for i in range(256)]
        if profile >= 2:
            rgb = [(rng.randrange(256), rng.randrange(256), rng.randrange(256)) for _ in range(256)]
        if profile == 3:
            rgb[7] = rgb[199] = (255, 22, 22)
            rgb[8] = rgb[200] = (181, 140, 111)
        # 이전 저장 배열은 COL 로더가 건드리지 않는 네 번째 바이트를 관찰하기 위한 입력이다.
        for bgrx in (False, True):
            previous = bytes(rng.randrange(256) for _ in range(1024))
            data = bytes(rng.randrange(256) for _ in range(8)) if not bgrx else b''
            # 파일 채널 배치를 입력에만 적용한다. 변환 결과는 원본 read 명령으로 얻는다.
            for red, green, blue in rgb:
                data += bytes((blue, green, red, rng.randrange(256))) if bgrx else bytes((red, green, blue))
            cases.append((len(cases), (profile + int(bgrx)) % 2, data.hex(), previous.hex()))
    return cases


class ColorsOracle(OwnerOracle):
    """파일 변환·검색·색 전역/별칭 대입을 실제 실행하고 파일/할당/장치 경계만 대체한다."""
    def __init__(self, edition):
        """읽기 전용 PE·Ghidra 몸체 범위·정확한 전역 쓰기 허용 목록을 준비한다."""
        super().__init__(edition)
        self.b = SPEC[edition]
        self.mu.mem_map(0, 0x1000)
        self.exports = [ROOT / f'extracted/palettecolors/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # OS 코드 없이 내보낸 불연속 몸체만 실행한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((low, high + 1))
        source = self.exports[0].read_text(encoding='utf-8').split('// ==== FUN_', 2)[1]
        matches = re.findall(r'(\w+)\s*=\s*FUN_' + f'{self.b["nearest"]:08x}' + r'\(([^)]+)\);', source)
        self.queries = [(self.b['brown'] if name == 'uVar3' else int(name.removeprefix('_DAT_').removeprefix('DAT_'), 16),
                         tuple(int(v.strip(), 0) for v in args.split(','))) for name, args in matches]
        if len(self.queries) != (58 if edition == 'originals' else 50):
            raise RuntimeError('전체 색 검색 수 불일치')
        if self.read(self.b['count'], '<I') != (9,):
            raise RuntimeError('PE 기본색 배열 개수가 9가 아님')
        self.stubs = {self.b[name]: name for name in ('open', 'size', 'seek', 'read', 'apply', 'alloc', 'close')}
        self.boundaries, self.returns = collections.Counter(), 0
        self.write_ranges = [(0, 4), (self.b['saved'], self.b['saved'] + 1024), (LOGICAL, LOGICAL + 1024),
            (BASIC, BASIC + 36), (self.b['basic'], self.b['basic'] + 4), (self.b['dirty'], self.b['dirty'] + 4),
            (self.b['weather'], self.b['weather'] + 16)] + [(address, address + 4) for address, _ in self.queries]

    def read(self, address, format):
        """에뮬레이터 메모리에서 지정한 정수 배치를 읽는다."""
        return struct.unpack(format, self.mu.mem_read(address, struct.calcsize(format)))

    def ret(self, purge=0, value=0):
        """명시 경계의 호출 규약에 맞춰 반환한다. 원본 내부 몸체를 건너뛰는 데 사용하지 않는다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        target, = self.read(sp, '<I')
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def on_instruction(self, mu, address, size, data):
        """검색 인자/반환을 실제 관찰하고 명시 파일/장치 경계 외 실행은 부모의 범위 감사를 거친다."""
        if self.pending and address == self.pending[0]:
            self.observed.append((*self.pending[1], mu.reg_read(UC_X86_REG_EAX)))
            self.pending = None
        if address == self.b['nearest']:
            sp = mu.reg_read(UC_X86_REG_ESP)
            self.pending = (self.read(sp, '<I')[0], self.read(sp + 4, '<3I'))
        name = self.stubs.get(address)
        if not name:
            return super().on_instruction(mu, address, size, data)
        self.boundaries[name] += 1
        self.events[name] += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        if name == 'open':
            if self.read(sp + 4, '<3I') != (FILENAME, 0, 1): raise RuntimeError('파일 열기 인자 오류')
            self.ret(12)
        elif name == 'size': self.ret(value=len(self.file))
        elif name == 'seek':
            if self.read(sp + 4, '<2I') != (8, 0): raise RuntimeError('COL seek 인자 오류')
            self.position = 8
            self.ret(8)
        elif name == 'read':
            destination, length, remaining = self.read(sp + 4, '<3I')
            if remaining != 0xffffffff or self.position + length > len(self.file): raise RuntimeError('파일 읽기 인자 오류')
            if not self.b['saved'] <= destination <= self.b['saved'] + 1024 - length: raise RuntimeError('파일 읽기 저장 범위 오류')
            mu.mem_write(destination, self.file[self.position:self.position + length])
            self.position += length
            self.ret(12, length)
        elif name == 'alloc':
            if self.read(sp + 4, '<I') != (36,): raise RuntimeError('기본색 할당 크기 오류')
            self.ret(value=BASIC)
        elif name == 'apply':
            if self.read(sp + 4, '<4I') != (0, 256, 0, 1): raise RuntimeError('팔레트 적용 인자 오류')
            # 이미 독립 대조한 SetPalette의 논리/끝점 계약만 대체한다. GDI/DirectDraw는 실행하지 않는다.
            mu.mem_write(self.b['saved'], bytes(4))
            mu.mem_write(self.b['saved'] + 1020, bytes((255, 255, 255, 0)))
            saved = bytes(mu.mem_read(self.b['saved'], 1024))
            logical = bytearray()
            # 검색이 실제 읽는 PALETTEENTRY 순서를 준비한다. 예약 바이트는 논리 플래그로 사용하지 않는다.
            for i in range(256):
                blue, green, red, _ = saved[i * 4:i * 4 + 4]
                logical.extend((red, green, blue, 0 if i in (0, 255) else 4))
            mu.mem_write(LOGICAL, bytes(logical))
            self.ret()
        else: self.ret()

    def run(self, case, control):
        """정상 파일 로딩 전체를 실행하고 저장 배열·기본색·날씨 별칭·모든 검색/전역 대입을 관찰한다."""
        number, reuse, file_hex, previous_hex = case
        mu, b = self.mu, self.b
        self.file, self.position = bytes.fromhex(file_hex), 0
        self.events, self.observed, self.pending = collections.Counter(), [], None
        mu.mem_write(0, bytes(4))
        mu.mem_write(b['saved'], bytes.fromhex(previous_hex))
        mu.mem_write(b['logical'], struct.pack('<I', LOGICAL))
        mu.mem_write(b['basic'], struct.pack('<I', BASIC if reuse else 0))
        mu.mem_write(b['dirty'], bytes(4))
        mu.mem_write(b['mode'], bytes(4))
        mu.mem_write(BASIC, bytes(36))
        mu.mem_write(FILENAME, b'oracle.col\0')
        mu.mem_write(STACK - 0x100, bytes(0x100))
        mu.mem_write(STACK, struct.pack('<III', STOP, FILENAME, 0))
        preserved = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_EBP: 0x23456789,
                     UC_X86_REG_ESI: 0x34567890, UC_X86_REG_EDI: 0x45678901}
        # 새 보존 레지스터 값으로 매 진입/정상 반환 계약을 확인한다.
        for register, value in preserved.items(): mu.reg_write(register, value)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, control)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        self.instructions = 0
        mu.emu_start(b['entry'], STOP, count=5000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError(f'팔레트 로더 반환/스택 오류: {self.edition}')
        if any(mu.reg_read(reg) != value for reg, value in preserved.items()): raise RuntimeError('보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW) != control or mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or self.read(0, '<I') != (0,):
            raise RuntimeError('x87/SEH 복구 오류')
        expected = dict(open=1, size=2 if len(self.file) == 1024 else 1,
            read=1 if len(self.file) == 1024 else 768, apply=1, close=1)
        if len(self.file) == 776: expected['seek'] = 1
        if not reuse: expected['alloc'] = 1
        if dict(self.events) != expected or self.position != len(self.file): raise RuntimeError('파일/할당 경계 수 오류')
        if self.read(b['dirty'], '<I') != (1,) or self.read(b['basic'], '<I') != (BASIC,): raise RuntimeError('전역 로딩 상태 오류')
        if self.pending or len(self.observed) != len(self.queries): raise RuntimeError('색 검색 정상 반환 수 오류')
        # 전역에 최종 저장된 값까지 실제 반환과 대조한다. 목적 RGB는 Ghidra 인자와 비교한다.
        for (address, rgb), observed in zip(self.queries, self.observed):
            if tuple(observed[:3]) != rgb or self.read(address, '<I') != (observed[3],):
                raise RuntimeError('색 검색 인자/전역 대입 오류')
        self.returns += 1
        return [bytes(mu.mem_read(b['saved'], 1024)).hex(), bytes(mu.mem_read(BASIC, 36)).hex(),
            bytes(mu.mem_read(b['weather'], 16)).hex(), bytes(mu.mem_read(LOGICAL, 1024)).hex(),
            '|'.join(','.join(map(str, color)) for color in self.observed)]


def generate(smoke=False):
    """두 x87 실행이 일치한 실제 원본 관찰만 fixture에 쓰고 입력/몸체 SHA를 별도 저장한다."""
    cases = inputs()[:1] if smoke else inputs()
    rows, editions = [], {}
    paths = {__file__, ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/palettecolors-functions.json', FIXTURE}
    # 각 실제 PE에서 동일 입력을 독립 실행한다.
    for edition in SPECS:
        oracle = ColorsOracle(edition)
        for case in cases:
            first = oracle.run(case, CONTROLS[0])
            second = oracle.run(case, CONTROLS[1])
            if first != second: raise RuntimeError(f'x87 정밀도 차이: {edition} {case[0]}')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=oracle.returns, native_calls=dict(oracle.native_calls),
            boundaries=dict(oracle.boundaries), assertions=oracle.assertions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 전체 팔레트 색 {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 x86 파일 로더/전체 색 검색 관찰. SetPalette/파일/할당은 명시 경계. 게임 실행 아님.\n' +
        '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), host='HJOW-Athlon', date='2026-10-10', controls=list(CONTROLS),
        os_calls=0, editions=editions, files={str(p.relative_to(ROOT).as_posix()): digest(p) for p in sorted(map(type(ROOT), paths))},
        limits=['파일 열기/크기/seek/read/닫기와 9색 메모리 할당은 합성 경계',
                'SetPalette 전체 적용은 기존 논리 팔레트/끝점 계약으로 대체; 실제 GDI/DirectDraw 미실행',
                '화면 지우기 인자 0의 정상 파일 경로만 실행; 파일 오류/assert/전체화면 지우기 및 SEH 예외 경로 제외']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """입력·SHA·로더/검색/정리의 진입 수와 정상 반환·파일 경계의 수를 다시 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본 PE와 도구·내보낸 몸체·fixture가 생성 시점과 같은지 검사한다.
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError(f'SHA 불일치: {name}')
    if report['total'] != len(rows) or report['os_calls'] or report['controls'] != list(CONTROLS) or set(report['editions']) != set(SPECS):
        raise RuntimeError('전체 팔레트 색 근거 계수 오류')
    # 모든 원본의 입력 순서와 두 정밀도의 몸체 호출/경계 호출 수를 감사한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]
        cases, b = inputs(), SPEC[edition]
        if len(selected) != len(cases) or item['cases'] != len(cases) or item['returns'] != len(cases) * 2 or item['assertions']:
            raise RuntimeError('표본/정상 반환 수 오류')
        expected = {f'{b["entry"]:08x}': len(cases) * 2, f'{b["nearest"]:08x}': len(cases) * 2 * (58 if edition == 'originals' else 50)}
        if edition != 'originals': expected['00424df8'] = len(cases) * 2
        boundary = collections.Counter()
        for row, case in zip(selected, cases):
            if len(row) != 10 or row[1:5] != list(map(str, case)): raise RuntimeError('fixture 입력 순서 오류')
            file = bytes.fromhex(case[2])
            boundary.update(open=2, size=4 if len(file) == 1024 else 2, read=2 if len(file) == 1024 else 1536, apply=2, close=2)
            if len(file) == 776: boundary['seek'] += 2
            if not case[1]: boundary['alloc'] += 2
        if item['native_calls'] != expected or item['boundaries'] != dict(boundary): raise RuntimeError('몸체/경계 호출 수 오류')
    print(f"전체 팔레트 색 감사 통과: {report['total']}개")


def main():
    """전체 생성·한 표본 직접 실행·저장 근거 감사 중 하나를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
