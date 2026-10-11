#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표시 수집·영역 환산·교차·정렬 비교를 세 실제 PE의 격리 x86으로 실행한다. 게임/OS 호출은 없다."""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, POOL, STACK, STOP, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 격리된 등록 목록/객체/입출력 주소와 원본 객체 간격이다. 출력은 8개의 16바이트 기록이다.
DATA = 0x14000000
TABLE, OBJECTS, VIEW, OUTPUT, BOUNDS = (DATA + offset for offset in (0, 0x1000, 0x4000, 0x5000, 0x6000))
STRIDE, OUTPUT_COUNT = 64, 8
# 실제 원본 진입점·전역이다. CD의 Pentium FDIV 우회는 0인 입력만 실행한다.
SPECS = {
    'originals': dict(collect=0x487f10, bounds=0x487a40, parent=0x41bfb0, intersect=0x487740, order=0x497900,
        table=0x595138, count=0x59513c, sx=0x59a924, sy=0x59a928, bias=0x509144, fdiv=None),
    'originalCD': dict(collect=0x45c4b0, bounds=0x45c9e0, parent=0x48f870, intersect=None, order=0x458240,
        table=0x5209c0, count=0x5209c4, sx=0x565c3c, sy=0x565c40, bias=0x502428, fdiv=0x546778)
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 입력 열과 저장 근거 경로다. 부동소수 값은 IEEE 비트로 기록한다.
KEYS = ('sx', 'sy', 'left', 'top', 'right', 'bottom', 'vleft', 'vtop', 'vright', 'vbottom',
        'slots', 'pattern', 'limit', 'z', 'x', 'y', 'flags')
FIXTURE = ROOT / 'cpppj/tests/fixtures/rendercollection-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-rendercollection-evidence.json'
# 부모 상태 비트와 무관한 수집, 깊이 WORD 절삭, 현재 좌표와 y 보정을 검사할 입력 시드다.
SEED = 0x487f1045
REGISTERS = {UC_X86_REG_EBX: 0x13572468, UC_X86_REG_ESI: 0x24681357,
             UC_X86_REG_EDI: 0x31415926, UC_X86_REG_EBP: 0x27182818}


def bits(value):
    """합성 float 입력을 저장 비트로 바꾼다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def real(value):
    """저장 float 입력을 읽는다. 기대 영역/교차/정렬은 계산하지 않는다."""
    return struct.unpack('<f', struct.pack('<I', value))[0]


def case(**changes):
    """원본에 전달할 입력 필드만 준비한다."""
    value = dict(zip(KEYS, [bits(v) for v in (16, 11, 80, 55, 160, 110, 0, 0, 20, 20)] +
        [5, 0, 8, 0xffffffff, bits(20.5), bits(21.25), 0]))
    value.update(changes)
    return value


def rects():
    """엄격한 경계·점/선·역전·NaN/무한대 입력이다. 판정 결과는 넣지 않는다."""
    return [(0, 0, 10, 10), (2, 2, 8, 8), (-5, -5, 0, 0), (10, 2, 12, 8),
        (2, 10, 8, 12), (0, 2, 0, 8), (5, 5, 5, 5), (10, 0, 0, 10), (0, 10, 10, 0),
        (-0.0, -0.0, 0.0, 0.0), (-float('inf'), -1, float('inf'), 11),
        (float('nan'), 0, 10, 10), (0, float('nan'), 10, 10), (0, 0, float('nan'), 10), (0, 0, 10, float('nan'))]


def plan():
    """환산/교차/목록 순서와 구멍/용량/부모 상태/깊이 경계/정렬 키의 유한 입력 계획이다."""
    rows = []
    scales = [(16, 11), (10, 3), (1, 1), (13.7, 7.1), (-16, 11), (2**-20, 2**17)]
    # 환산은 부모 좌표와 무관하게 현재 등록 사각형을 사용한다.
    for scale, rect in itertools.product(scales, rects()[:11]):
        rows.append(('Bounds', case(sx=bits(scale[0]), sy=bits(scale[1]),
            **dict(zip(('left', 'top', 'right', 'bottom'), map(bits, rect))), slots=1)))
    rng = random.Random(SEED)
    # float 역수를 먼저 반올림하는 잘못된 구현을 구별하는 다양한 정상 환산 입력이다.
    for _ in range(160):
        sx, sy = rng.choice(scales)
        coordinates = [bits(rng.randrange(-1000000, 1000000) / rng.choice([1, 7, 13, 1024])) for _ in range(4)]
        rows.append(('Bounds', case(sx=bits(sx), sy=bits(sy),
            **dict(zip(('left', 'top', 'right', 'bottom'), coordinates)), slots=1)))
    # CD에는 독립 교차 helper가 없으므로 전체 collector 안의 인라인 교차를 실행한다.
    for view, rect in itertools.product(rects(), repeat=2):
        rows.append(('Intersect', case(sx=bits(1), sy=bits(1), slots=1,
            **dict(zip(('left', 'top', 'right', 'bottom'), map(bits, rect))),
            **dict(zip(('vleft', 'vtop', 'vright', 'vbottom'), map(bits, view))))))
    # 빈/40/80칸과 네 가지 구멍 배치에서 출력 개수 제한을 실제 원본으로 확인한다.
    for slots, pattern, limit, view in itertools.product((0, 1, 5, 40, 80), range(4), (-1, 0, 1, 2, 5, 8), rects()[:7]):
        rows.append(('Collect', case(slots=slots, pattern=pattern, limit=limit & 0xffffffff,
            **dict(zip(('vleft', 'vtop', 'vright', 'vbottom'), map(bits, view))))))
    # 기존 collector가 검사하지 않는 부모 상태와 DWORD zOrder의 WORD 경계를 독립적으로 입력한다.
    for flags, z, y in itertools.product((0, 1, 2, 4, 8, 255), (0, 32767, 32768, 65535, 65536, 0x12348001), (0, 21.25, 2**24)):
        rows.append(('Collect', case(flags=flags, z=z, y=bits(y))))
    # 정렬 비교 루트는 현재 좌표·signed WORD 깊이·동률을 반환값 그대로 관찰한다.
    for x, y, z in itertools.product((-1, 0, 1), (-1, 0, 1), (0, 32767, 32768, 65535)):
        rows.append(('Order', case(x=bits(x), y=bits(y), z=z)))
    return rows


def objects(c):
    """슬롯/필드 입력 목록만 만든다. 관찰 파일에 이 완성 입력을 그대로 저장해 C++는 재생성하지 않는다."""
    values = []
    # 출력 순서/범위를 시험하려고 부모 좌표/깊이/사각형을 슬롯마다 다르게 공급한다.
    for slot in range(c['slots']):
        if c['pattern'] == 3 or (c['pattern'] == 1 and slot % 3 == 0) or (c['pattern'] == 2 and slot != c['slots'] - 1):
            continue
        coordinates = [bits(real(c[name]) + slot * 16) if slot and real(c[name]) not in (float('inf'), -float('inf')) else c[name]
                       for name in ('left', 'top', 'right', 'bottom')]
        values.append([slot, 50 + slot, (c['z'] + slot * 17) & 0xffffffff, *coordinates,
            bits(real(c['x']) + slot * .25), bits(real(c['y']) - slot * .5), c['flags']])
    return values


class CollectionOracle(ProcessOracle):
    """수집/환산/getter/교차/비교의 실제 명령만 허용한다. 계산/효과 응답 대체가 없다."""
    def __init__(self, edition):
        """PE를 읽기 전용으로 매핑하고 검토한 함수 범위와 출력 쓰기만 허용한다."""
        super().__init__(edition)
        self.d = SPECS[edition]
        self.extra = [ROOT / f'extracted/rendercollection/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entry_names = [], {}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이의 코드와 미검토 원본 함수는 실행하지 않는다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entry_names[int(row['entry'], 16)] = row['entry']
                # 한 함수가 여러 코드 구간을 가지면 각각 정확한 마지막 명령까지만 허용한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.mu.mem_map(DATA, 0x10000)
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000),
            (OUTPUT, OUTPUT + OUTPUT_COUNT * 16), (BOUNDS, BOUNDS + 16)]
        self.put(self.s['sid'][0], POOL)
        if self.d['fdiv']: self.put(self.d['fdiv'], 0)
        if self.u32(self.d['bias']) != 0x3a83126f: raise RuntimeError('원본 y bias 상수 오류')
        self.returns = collections.Counter()
        self.events = []

    def on_instruction(self, mu, address, size, data):
        """실제 helper 진입 순서와 토큰을 관찰한다. 응답 경계/OS/assert 호출은 허용하지 않는다."""
        if address == self.s['stubs']['assert_']:
            self.asserts += 1
            raise RuntimeError('표시 수집 원본 assert 도달')
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'표시 수집 허용하지 않은 실행: {self.binary} {address:08x}')
        if address in self.entry_names:
            self.hits[self.entry_names[address]] += 1
            key = next(key for key in ('collect', 'bounds', 'parent', 'intersect', 'order') if self.d[key] == address)
            token = (mu.reg_read(UC_X86_REG_ECX) - OBJECTS) // STRIDE + 1 if key in ('bounds', 'parent') else 0
            self.events.append(f'{key}:{token}')

    def invoke(self, key, args, this=0, purge=0):
        """비영 보존 레지스터·스택·x87·SEH와 루트 정상 반환을 확인한다."""
        mu = self.mu
        # 보존 대상은 0이 아닌 서로 다른 값으로 설정해 실제 복구를 확인한다.
        for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
        # 반환/임시 레지스터만 초기화한다. 기대 함수 결과를 공급하지 않는다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): mu.reg_write(reg, 0)
        mu.reg_write(UC_X86_REG_ECX, this);mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_ESP, STACK);mu.reg_write(UC_X86_REG_FPCW, self.control);mu.reg_write(UC_X86_REG_FPSW, 0)
        self.put(0, 0xffffffff)
        mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        mu.emu_start(self.d[key], STOP, timeout=10000000, count=1000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError('표시 수집 반환/스택 오류')
        if any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()): raise RuntimeError('표시 수집 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != self.control or self.u32(0) != 0xffffffff:
            raise RuntimeError('표시 수집 x87/SEH 복구 오류')
        self.returns[f'{self.d[key]:08x}'] += 1
        return mu.reg_read(UC_X86_REG_EAX)

    def run(self, operation, c, control):
        """완성 입력을 원본에 전달하고 출력 전체 바이트·helper 순서를 읽는다."""
        self.control = control;self.events = [];mu = self.mu;d = self.d
        mu.mem_write(DATA, bytes(0x10000));mu.mem_write(OUTPUT, b'\xa5' * (OUTPUT_COUNT * 16));mu.mem_write(BOUNDS, b'\xa5' * 16)
        self.put(d['table'], TABLE);self.put(d['count'], c['slots']);self.put(d['sx'], c['sx']);self.put(d['sy'], c['sy'])
        mu.mem_write(VIEW, struct.pack('<4I', *(c[key] for key in ('vleft', 'vtop', 'vright', 'vbottom'))))
        records = objects(c)
        # 목록과 부모 raw는 실행 중 읽기만 허용한다. 부착/타입/vtable 효과를 대체하지 않는다.
        for slot, parent, z, left, top, right, bottom, x, y, flags in records:
            pointer = OBJECTS + slot * STRIDE;self.put(TABLE + slot * 4, pointer)
            self.put(pointer + 8, parent);self.put(pointer + 0x20, z)
            mu.mem_write(pointer + 0xc, struct.pack('<4I', left, top, right, bottom))
            raw = POOL + parent * self.s['stride'];mu.mem_write(raw, bytes(self.s['stride']))
            self.put(raw + 6, 7, 2);self.put(raw + 14, x);self.put(raw + 18, y);self.put(raw + 11, flags, 1)
        args = [c[key] for key in ('vleft', 'vtop', 'vright', 'vbottom')] + [OUTPUT, c['limit']]
        result = 0
        if operation == 'Bounds': self.invoke('bounds', [BOUNDS], OBJECTS, 4)
        elif operation == 'Intersect' and d['intersect']:
            result = self.invoke('intersect', [c[key] for key in ('left', 'top', 'right', 'bottom')], VIEW, 16)
        elif operation == 'Order':
            mu.mem_write(OUTPUT, struct.pack('<II I H BB', c['x'], c['y'], 0, c['z'] & 0xffff, 3, 165))
            mu.mem_write(OUTPUT + 16, struct.pack('<ffIhBB', 0, 0, 0, 0, 3, 165))
            result = self.invoke('order', [OUTPUT, OUTPUT + 16])
        else: result = self.invoke('collect', args)
        output = bytearray(mu.mem_read(OUTPUT, OUTPUT_COUNT * 16))
        if operation == 'Collect' or (operation == 'Intersect' and not d['intersect']):
            # 실제 포인터만 슬롯+1 토큰으로 정규화한다. 영역/좌표/깊이/종류/잔여 바이트는 계산하지 않는다.
            for index in range(result):
                pointer = struct.unpack_from('<I', output, index * 16 + 8)[0]
                struct.pack_into('<I', output, index * 16 + 8, (pointer - OBJECTS) // STRIDE + 1)
        return [';'.join(','.join(map(str, record)) for record in records) or '-', str(result), output.hex(),
            bytes(mu.mem_read(BOUNDS, 16)).hex(), ';'.join(self.events) or '-']


def generate(smoke):
    """C++ 계산을 사용하지 않고 두 x87 관찰을 비교한 뒤 fixture/SHA를 저장한다."""
    rows = [];editions = {};cases = plan()
    files = {ROOT / 'tools/decomp_rendercollection_oracle.py', ROOT / 'tools/decomp_process_oracle.py',
        ROOT / 'tools/ghidra/rendercollection-functions.json', FIXTURE}
    if smoke: cases = cases[::43] + cases[-4:]
    # 세 원본 PE를 각각 읽고 두 정밀도의 정상 반환/출력을 독립 관찰한다.
    for edition in BINARIES:
        oracle = CollectionOracle(edition)
        # 입력 행마다 두 x87 정밀도에서 실제 명령의 모든 관찰을 대조한다.
        for index, (operation, c) in enumerate(cases, 1):
            observed = [oracle.run(operation, c, control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'표시 수집 x87 차이: {edition} {operation} {c}')
            rows.append([edition, operation, ','.join(str(c[key]) for key in KEYS), *observed[0]])
            if index % 400 == 0: print(f'{edition}: {index}/{len(cases)}', flush=True)
        editions[edition] = dict(cases=dict(collections.Counter(op for op, _ in cases)), entries=dict(oracle.hits),
            returns=dict(oracle.returns), assertions=oracle.asserts)
        files.update(oracle.exports + oracle.extra);files.add(ROOT / BINARIES[edition])
        print(f'{edition}: {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 표시 수집/환산/교차/정렬 비교. 계산 및 OS 응답 대체 없음.\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), controls=list(CONTROLS), host='HJOW-Athlon', date='2026-10-11',
        os_calls=0, boundaries={}, editions=editions, files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['전체 수집/환산/부모 getter/교차/비교의 실제 원본 명령; 계산/효과 응답 대체 없음',
            '빈/80칸/구멍/개수 제한·부모 모든 상태 비트·현재 좌표·depth WORD/tail BYTE·점/선/역전/NaN/무한대 영역',
            '배율은 유한 비영 float, 부모 head 비영 합성 raw, CD FDIV 우회=0; 손상 부모/목록/assert/OS는 실행하지 않음',
            '클라이언트 전체 렌더러·CRT qsort 순서·실제 SHP/팔레트/장치 출력 근거가 아님',
            '비영 보존 레지스터/스택/x87/SEH·허용 실행/출력 쓰기·전체 출력/잔여 바이트·입력/진입/반환/SHA 감사']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """기대 계산을 재생성하지 않고 입력/출력 폭·SHA·실제 helper 진입/루트 반환을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 관찰을 만든 PE/함수 내보내기/도구/fixture가 당시 바이트와 같은지 확인한다.
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError('표시 수집 SHA 불일치: ' + name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls'] or report['boundaries']:
        raise RuntimeError('표시 수집 총계/경계 오류')
    if set(report['editions']) != set(BINARIES): raise RuntimeError('표시 수집 판본 누락')
    cases = plan()
    # 모든 행과 두 정밀도 진입/반환을 저장 관찰에서만 집계한다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0] == edition];d = SPECS[edition]
        entries = collections.Counter();returns = collections.Counter()
        if len(chosen) != len(cases) or item['assertions']: raise RuntimeError('표시 수집 행/assert 오류')
        # 기대 계산을 만들지 않고 각 행의 입력과 관찰 폭을 확인한다.
        for row, (op, c) in zip(chosen, cases, strict=True):
            records = ';'.join(','.join(map(str, record)) for record in objects(c)) or '-'
            if len(row) != 8 or row[1:4] != [op, ','.join(str(c[key]) for key in KEYS), records]: raise RuntimeError('표시 수집 입력 순서/폭 오류')
            if len(bytes.fromhex(row[5])) != OUTPUT_COUNT * 16 or len(bytes.fromhex(row[6])) != 16: raise RuntimeError('표시 수집 출력 폭 오류')
            result = int(row[4])
            if not 0 <= result <= 0xffffffff or (op in ('Collect', 'Intersect') and result > OUTPUT_COUNT): raise RuntimeError('표시 수집 반환 폭 오류')
            root = 'collect' if op == 'Collect' or (op == 'Intersect' and not d['intersect']) else op.lower()
            returns[f'{d[root]:08x}'] += 2
            # 실제 진입 사건을 두 정밀도의 횟수로 합쳐 저장한 실행 총계와 비교한다.
            for event in row[7].split(';'):
                key, token = event.split(':');entries[f'{d[key]:08x}'] += 2
                if key in ('bounds', 'parent') and not 1 <= int(token) <= max(c['slots'], 1): raise RuntimeError('표시 수집 helper 토큰 오류')
        if dict(entries) != item['entries'] or dict(returns) != item['returns'] or item['cases'] != dict(collections.Counter(op for op, _ in cases)):
            raise RuntimeError('표시 수집 진입/반환 집계 오류')
    print(f'rendercollection 검증 통과: {len(rows)}개, 응답 경계/OS/assert 0')


def main():
    """전체 원본 관찰 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true');parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__ == '__main__':
    main()
