#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 표시 목록 CRT 정렬/비교/교환을 격리 x86으로 실행한다. 게임/OS/응답 대체는 없다."""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 가상 입력 배열과 앞뒤 보호 구간이다. 표시 항목은 실제 함수가 전체 16바이트를 교환한다.
DATA, ARRAY = 0x15000000, 0x15000100
GUARD, STRIDE = 16, 16
# 실제 함수 주소와 CD의 표시 정렬 옵션 전역이다. 패치의 shortsort는 끝 포인터를 EAX로 받는다.
SPECS = {
    'originals': dict(sort=0x4e6010, short=0x4e5fa0, compare=0x497900, wrapper=None, option=None),
    'originalCD': dict(sort=0x4f1c00, short=0x4f1d60, swap=0x4f1dc0, compare=0x458240,
        wrapper=0x4582b0, option=0x520418),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 입력 시드·크기·보존 레지스터와 저장 관찰 경로다. 입력에는 결과나 정렬된 순서를 넣지 않는다.
SEED = 0x4e60101c
COUNTS = (0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 256)
REGISTERS = {UC_X86_REG_EBX: 0x13572468, UC_X86_REG_ESI: 0x24681357,
    UC_X86_REG_EDI: 0x31415926, UC_X86_REG_EBP: 0x27182818}
FIXTURE = ROOT / 'cpppj/tests/fixtures/drawsort-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-drawsort-evidence.json'


def bits(value):
    """합성 float 입력을 IEEE 비트로 만든다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def plan(edition):
    """8/9 경계·분할 깊이·전체 동률·signed 깊이·혼합 종류·무작위 입력의 유한 계획이다."""
    cases = [('Sort', n, profile, SEED, 1) for n in COUNTS for profile in range(9)]
    rng = random.Random(SEED)
    # 다양한 구간 크기에서 동일 키와 역순/교차 분할을 입력한다.
    for index in range(100): cases.append(('Sort', rng.randrange(1, 193), 6, SEED + index + 1, 1))
    # 작은 구간 몸체는 두 판본의 실제 호출 규약으로 별도 관찰한다.
    for n in range(1, 9):
        # 선택 정렬에 공급할 모든 키 형태다.
        for profile in range(9): cases.append(('Short', n, profile, SEED, 1))
    if SPECS[edition]['wrapper']:
        # CD의 원본 옵션 wrapper는 정렬 끔/켬 분기를 모두 정상 반환까지 실행한다.
        for n in (0, 1, 8, 9, 64):
            # 혼합 종류와 같은 키가 옵션 끔에서도 원래 바이트 순서를 유지하는지 관찰한다.
            for profile in range(9):
                # 0 또는 1의 현재 옵션 전역만 바꾼다.
                for enabled in (0, 1): cases.append(('Wrapper', n, profile, SEED, enabled))
    return cases


def inputs(count, profile, seed):
    """원본 배열의 완성 바이트를 만든다. 기대 비교/피벗/교환/정렬 순서는 계산하지 않는다."""
    rng = random.Random(seed);records = []
    extreme = (1, 0x80000001, 0, 0x80000000, bits(1e30), bits(-1e30), 0x7f7fffff, 0xff7fffff)
    # 토큰/종류/tail은 서로 다르게 만들어 키 이외의 바이트도 같이 이동하는지 관찰한다.
    for index in range(count):
        x, y, z = 0x80000000 if index % 2 else 0, 0, 0
        if profile == 1: z = index - count // 2
        elif profile == 2: z = count // 2 - index
        elif profile == 3: y = bits(index / 7)
        elif profile == 4: y = bits(-index / 7)
        elif profile == 5: x, y = bits((index % 5) - 2), bits(index % 3)
        elif profile == 6: x, y, z = bits(rng.randrange(-4, 5)), bits(rng.randrange(-4, 5)), rng.randrange(-3, 4)
        elif profile == 7: x, y, z = bits(index % 3), bits(index % 7), (-32768, 32767)[index % 2]
        elif profile == 8: x, y, z = extreme[index % 8], extreme[(index * 3 + 1) % 8], index % 3 - 1
        records.append(struct.pack('<IIIhBB', x, y, index + 1, z, index % 5, (index ^ 0xa5) & 255))
    return b''.join(records)


class DrawSortOracle(ProcessOracle):
    """실제 qsort/shortsort/swap/비교/wrapper만 허용하고 호스트/OS 경계를 두지 않는다."""
    def __init__(self, edition):
        """읽기 전용 PE 매핑에서 검토한 함수 범위를 별도 내보내기로 제한한다."""
        super().__init__(edition);self.d = SPECS[edition]
        self.extra = [ROOT / f'extracted/drawsort/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entry_names = [], {}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 내보낸 몸체만 실행한다. 함수 사이 코드도 허용하지 않는다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entry_names[int(row['entry'], 16)] = row['entry']
                # 한 몸체의 불연속 구간도 각각 검사한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.mem_map(DATA, 0x10000);self.returns = collections.Counter();self.calls = collections.Counter()
        self.trace = hashlib.sha256()

    def on_instruction(self, mu, address, size, data):
        """함수 진입과 실제 비교 대상 바이트의 순서를 읽는다. 실행 명령을 대체하지 않는다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'표시 정렬 허용하지 않은 실행: {self.binary} {address:08x}')
        if address in self.entry_names:
            self.calls[self.entry_names[address]] += 1;self.hits[self.entry_names[address]] += 1
            if address == self.d['compare']:
                a, b = struct.unpack('<2I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 8))
                if not all(ARRAY <= p < ARRAY + self.count * STRIDE and (p - ARRAY) % STRIDE == 0 for p in (a, b)):
                    raise RuntimeError('표시 정렬 비교 포인터 범위 오류')
                self.trace.update(bytes(mu.mem_read(a, STRIDE)));self.trace.update(bytes(mu.mem_read(b, STRIDE)))

    def run(self, operation, count, profile, seed, enabled, control):
        """배열을 실제 몸체에 전달하고 전체 바이트·보호 구간·비교 순서·ABI 정상 반환을 관찰한다."""
        mu, d = self.mu, self.d;self.count = count;self.calls = collections.Counter();self.trace = hashlib.sha256()
        source = inputs(count, profile, seed);mu.mem_write(ARRAY - GUARD, b'\xa5' * (len(source) + 2 * GUARD))
        if source: mu.mem_write(ARRAY, source)
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000), (ARRAY, ARRAY + len(source))]
        if d['option']: self.put(d['option'], enabled)
        # 보존 레지스터를 비영 값으로 두어 실제 복구를 확인한다.
        for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
        # 임시/반환 레지스터는 결과 입력 없이 초기화한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX): mu.reg_write(reg, 0)
        mu.reg_write(UC_X86_REG_EFLAGS, 2);mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_FPCW, control);mu.reg_write(UC_X86_REG_FPSW, 0);self.put(0, 0xffffffff)
        args, purge, key = [ARRAY, count, STRIDE, d['compare']], 0, 'sort'
        if operation == 'Wrapper': args, purge, key = [ARRAY, count], 8, 'wrapper'
        elif operation == 'Short':
            key = 'short';last = ARRAY + (count - 1) * STRIDE
            if self.patch:
                args = [ARRAY, STRIDE, d['compare']];mu.reg_write(UC_X86_REG_EAX, last)
            else: args = [ARRAY, last, STRIDE, d['compare']]
        mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        mu.emu_start(d[key], STOP, timeout=30000000, count=20000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError('표시 정렬 정상 반환/스택 오류')
        if any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()): raise RuntimeError('표시 정렬 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.u32(0) != 0xffffffff:
            raise RuntimeError('표시 정렬 x87/SEH 복구 오류')
        self.returns[f'{d[key]:08x}'] += 1
        guard = bytes(mu.mem_read(ARRAY - GUARD, GUARD)) + bytes(mu.mem_read(ARRAY + len(source), GUARD))
        if guard != b'\xa5' * (2 * GUARD): raise RuntimeError('표시 정렬 보호 바이트 손상')
        return [source.hex() or '-', bytes(mu.mem_read(ARRAY, len(source))).hex() if source else '-', guard.hex(),
            json.dumps(dict(self.calls), sort_keys=True, separators=(',', ':')), self.trace.hexdigest()]


def generate(smoke):
    """두 x87 관찰이 같은 정상 반환 결과만 fixture와 SHA로 기록한다. C++는 사용하지 않는다."""
    rows, editions = [], {};files = {ROOT / 'tools/decomp_drawsort_oracle.py', ROOT / 'tools/decomp_process_oracle.py',
        ROOT / 'tools/ghidra/drawsort-functions.json', FIXTURE}
    # 각 PE의 CRT/비교/wrapper를 서로 다른 격리 메모리에서 실행한다.
    for edition in BINARIES:
        oracle = DrawSortOracle(edition);cases = plan(edition)
        if smoke: cases = cases[::29] + cases[-4:]
        # 입력별 두 실제 정밀도의 출력·보호 구간·진입·비교 순서를 대조한다.
        for index, (op, n, profile, seed, enabled) in enumerate(cases, 1):
            observed = [oracle.run(op, n, profile, seed, enabled, control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'표시 정렬 x87 관찰 차이: {edition} {index}')
            rows.append([edition, op, f'{n},{profile},{seed},{enabled}', *observed[0]])
            if index % 100 == 0: print(f'{edition}: {index}/{len(cases)}', flush=True)
        editions[edition] = dict(cases=dict(collections.Counter(op for op, *_ in cases)), entries=dict(oracle.hits), returns=dict(oracle.returns))
        files.update(oracle.exports + oracle.extra);files.add(ROOT / BINARIES[edition]);print(f'{edition}: {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 CRT 표시 목록 정렬. 입력/전체 출력/보호 바이트/진입/비교 순서 관찰. 응답 대체 없음.\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), host='HJOW-Athlon', date='2026-10-11', controls=list(CONTROLS),
        os_calls=0, assertions=0, boundaries={}, editions=editions,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['실제 qsort/shortsort/byte swap/표시 비교/CD 옵션 wrapper 전체 정상 반환; 응답 대체 없음',
            '유한 x/y·signed WORD 깊이·0~256항목·8/9 경계·동률/역순/혼합 종류·전체 16바이트 보존',
            'NaN/무한대 키·일반 크기 CRT·손상 포인터/개수·전체 Renderer/수집/장치 출력은 범위 밖',
            '비영 보존 레지스터/스택/x87/SEH·허용 실행/쓰기·SHA/입력/폭/진입/루트 반환/비교 순서 감사']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """입력/SHA/바이트 폭/실제 진입/정상 반환을 감사한다. 기대 정렬 순서는 재계산하지 않는다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 독립 관찰을 만든 실제 파일의 바이트가 유지됐는지 확인한다.
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError('표시 정렬 SHA 불일치: ' + name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls'] or report['assertions'] or report['boundaries']:
        raise RuntimeError('표시 정렬 총계/경계 오류')
    if set(report['editions']) != set(BINARIES): raise RuntimeError('표시 정렬 판본 누락')
    # 저장한 각 행을 원본 입력 계획과 실제 진입 총계로 감사한다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0] == edition];cases = plan(edition);entries, returns = collections.Counter(), collections.Counter()
        if len(chosen) != len(cases): raise RuntimeError('표시 정렬 행 수 오류')
        # 완성 입력 바이트만 재생성하며 기대 출력은 저장 관찰에서 읽는다.
        for row, (op, n, profile, seed, enabled) in zip(chosen, cases, strict=True):
            source = inputs(n, profile, seed).hex() or '-'
            if len(row) != 8 or row[1:4] != [op, f'{n},{profile},{seed},{enabled}', source]: raise RuntimeError('표시 정렬 입력 순서/폭 오류')
            output = b'' if row[4] == '-' else bytes.fromhex(row[4])
            if len(output) != n * STRIDE or row[5] != (b'\xa5' * (2 * GUARD)).hex() or len(row[7]) != 64: raise RuntimeError('표시 정렬 관찰 폭/보호 오류')
            key = 'sort' if op == 'Sort' else op.lower();returns[f'{SPECS[edition][key]:08x}'] += 2
            # 두 정밀도의 함수 진입 횟수를 합쳐 전체 실행 총계와 대조한다.
            for address, count in json.loads(row[6]).items(): entries[address] += count * 2
        if dict(entries) != item['entries'] or dict(returns) != item['returns'] or item['cases'] != dict(collections.Counter(op for op, *_ in cases)):
            raise RuntimeError('표시 정렬 진입/반환 집계 오류')
    print(f'drawsort 검증 통과: {len(rows)}개, 응답 경계/OS/assert 0')


def main():
    """전체 원본 관찰 생성·무저장 표본·저장 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke', action='store_true');parser.add_argument('--verify', action='store_true')
    args = parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
