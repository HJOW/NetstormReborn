#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 공유 빔의 실제 경로 생성자·시작점·재귀 가지·진행/종료를 독립 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE

PATH, OWNER, PARTICLES, POOL, TYPES = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000  # 경로/소유 빔/입자/두 raw 자산/타입의 격리 주소다.
CAPACITY = 120  # 실제 빔이 확보하는24BYTE 입자의 개수다.
ENTRIES = {0x471b20, 0x471f60, 0x471fc0, 0x4720f0, 0x472180, 0x487b20, 0x4558c0, 0x455930, 0x4adea0, 0x49a840, 0x49a900}  # 경계 대체 없이 실행하는 전체 몸체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/lightning-path-x86.tsv'  # 포인터를 명시 토큰으로 정규화한 원본 전체 상태다.
REPORT = ROOT / 'cpppj/lightning-path.json'  # 고유 입력·정밀도·실제 호출/정상 반환과 SHA다.


def bits(value):
    """함수에 넣을 float32 입력만 비트로 바꾸며 출력은 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def inputs():
    """포구 타입/축·비율과 단계별 진행/감소/완료 후 재호출을 독립 조합한다."""
    points = ((20.5, 21.25, 40.125, 35.375), (1.5, 1.5, 1.5, 1.5), (-7.75, 8.25, 12.5, -9.75))
    rows = []
    # 실제 source별 높이 보정과 target의 다른 보정을 모두 방문한다.
    for source, target, point, scale in itertools.product((148, 149, 144), (70, 148, 149, 144), points, ((16, 11), (.75, 1.25))):
        rows.append((0, source, target, *map(bits, point), *map(bits, scale), 0, '-'))
    sequences = (((0, 0), (.025, .25), (.15, .5), (.4, .75), (.8, 1), (1, 1.25), (1.2, 1.5)), ((1, 0),), ((.5, .5), (.25, .75), (.75, 1), (1, 1.25), (1, 2)))
    # 난수 분기와 진행값의 이전/이후 호출 순서는 모두 입력으로 기록한다.
    for (source, target), point, seed, sequence in itertools.product(((148, 70), (148, 149), (144, 144), (149, 148)), points, (0, 1, 0x12345678, 0xdeadbeef), sequences):
        steps = ','.join(f'{bits(progress)}:{bits(life)}' for progress, life in sequence)
        rows.append((1, source, target, *map(bits, point), bits(16), bits(11), seed, steps))
    assert len(rows) == len(set(rows))
    return rows


class Oracle(CallOracle):
    """경로 수치·난수·재귀·Append를 실제 코드로 실행하고 외부 효과를 대체하지 않는다."""
    def __init__(self):
        """현재 PC 정밀 함수표와 PE만 읽어 격리 메모리/쓰기 제한을 준비한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095); self.mu.mem_write(0x400000, image)
        # 객체/입력/스택을 서로 겹치지 않게 확보한다.
        for address, size in ((PATH, 0x10000), (OWNER, 0x10000), (PARTICLES, 0x10000), (POOL, 0x10000), (TYPES, 0x20000), (STACK - 0x7f000, 0x80000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        self.allowed = []; self.calls = Counter(); self.returns = 0
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 함수별 실제 불연속 범위 밖은 실행하지 않는다.
        for entry in ENTRIES:
            # 같은 함수의 여러 원문 구간을 각각 추가한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """새/미검토 몸체나 원본 assert에 들어가면 성공으로 바꾸지 않고 거부한다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 빔 경로 실행 {address:08x}')
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1

    def write(self, mu, access, address, size, value, data):
        """경로64BYTE·사용 수·120입자·난수·스택 밖의 원본/입력 쓰기를 거부한다."""
        regions = ((PATH, PATH + 64), (OWNER + 0x28, OWNER + 0x2c), (PARTICLES, PARTICLES + CAPACITY * 24), (0x532710, 0x532714), (STACK - 0x7f000, STACK + 0x1000))
        if not any(low <= address and address + size <= high for low, high in regions):
            raise RuntimeError(f'허용하지 않은 빔 경로 쓰기 {address:08x}+{size}')

    def token(self, pointer):
        """원본의 실제 입자 포인터만 index+1 토큰으로 바꾸며 NULL/미초기화 패턴은 보존한다."""
        if pointer in (0, 0xa5a5a5a5):
            return pointer
        assert PARTICLES <= pointer < PARTICLES + CAPACITY * 24 and (pointer - PARTICLES) % 24 == 0
        return (pointer - PARTICLES) // 24 + 1

    def snapshot(self):
        """전체 경로와 현재 사용 수·난수를 포인터 위치만 정규화하여 기록한다."""
        raw = bytearray(self.mu.mem_read(PATH, 64))
        struct.pack_into('<I', raw, 0x34, self.token(struct.unpack_from('<I', raw, 0x34)[0]))
        assert struct.unpack_from('<I', raw, 0x3c)[0] == OWNER; struct.pack_into('<I', raw, 0x3c, 1)
        total = bytes(self.mu.mem_read(OWNER + 0x28, 4)); seed = bytes(self.mu.mem_read(0x532710, 4))
        return bytes(raw) + total + seed

    def sample(self, values, precision):
        """실제 ctor 뒤 선택한 시작/단계 호출을 실행하고 중간 상태와 용량 전체를 읽는다."""
        kind, source, target, sx, sy, tx, ty, scale_x, scale_y, seed, steps = values; mu = self.mu
        mu.mem_write(PATH, b'\xa5' * 64); mu.mem_write(OWNER, bytes(0x68)); mu.mem_write(PARTICLES, bytes(CAPACITY * 24))
        mu.mem_write(OWNER + 0x24, struct.pack('<3I', CAPACITY, 0, PARTICLES))
        mu.mem_write(0x5c8464, struct.pack('<I', POOL)); mu.mem_write(0x59ab20, struct.pack('<I', TYPES)); mu.mem_write(0x541348, struct.pack('<I', 188))
        mu.mem_write(0x59a924, struct.pack('<2I', scale_x, scale_y)); mu.mem_write(0x532710, struct.pack('<I', seed))
        # 두 SID의 raw 타입/좌표와 타입별 동일 발자국을 구성한다. 같은 타입의 다른 발자국을 가정하지 않는다.
        for sid, typeid, x, y in ((50, source, sx, sy), (51, target, tx, ty)):
            raw = bytearray(50); raw[10] = typeid; struct.pack_into('<2I', raw, 14, x, y); mu.mem_write(POOL + sid * 50, bytes(raw))
            foot = 3 if typeid in (148, 149) else 2 if typeid == 144 else 1
            mu.mem_write(TYPES + typeid * 500, bytes(500)); mu.mem_write(TYPES + typeid * 500 + 0x1d4, struct.pack('<2i', foot, foot))
        before_pool = bytes(mu.mem_read(POOL + 2500, 100)); before_types = bytes(mu.mem_read(TYPES, 0x20000))
        self.call(0x471b20, PATH, struct.pack('<3I', OWNER, 50, 51), 12, precision, PATH)
        self.returns += 1
        trace = [self.snapshot()]
        if kind:
            self.call(0x4720f0, PATH, b'', 0, precision); self.returns += 1; trace.append(self.snapshot())
            # 입력에 지정한 순서 그대로 진행/감소/완료 후 재호출을 수행한다.
            for step in steps.split(','):
                progress, life = map(int, step.split(':')); self.call(0x472180, PATH, struct.pack('<2I', progress, life), 8, precision); self.returns += 1; trace.append(self.snapshot())
        particles = bytearray(mu.mem_read(PARTICLES, CAPACITY * 24))
        # 원문 용량 전체에서 이전 입자 포인터 위치만 토큰으로 바꾼다.
        for index in range(CAPACITY):
            offset = index * 24 + 20; struct.pack_into('<I', particles, offset, self.token(struct.unpack_from('<I', particles, offset)[0]))
        assert bytes(mu.mem_read(POOL + 2500, 100)) == before_pool and bytes(mu.mem_read(TYPES, 0x20000)) == before_types
        assert bytes(mu.mem_read(OWNER + 0x24, 4)) == struct.pack('<I', CAPACITY) and bytes(mu.mem_read(OWNER + 0x2c, 4)) == struct.pack('<I', PARTICLES)
        return b''.join(trace).hex(), particles.hex()


def main():
    """원문 기대값과 정밀도 차이를 보존하거나 전체 SHA/입력 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); parser.add_argument('--smoke', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 기대 결과를 재생성하지 않고 전체 근거 파일 바이트를 대조한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == report['uniqueInputs'] and len({tuple(row[:11]) for row in rows}) == len(rows)
        assert all(len(row) == 13 and len(bytes.fromhex(row[12])) == CAPACITY * 24 for row in rows)
        print('빔 경로 SHA/고유 입력 감사:', len(rows)); return
    cases = inputs(); cases = [cases[0], next(row for row in cases if row[0])] if args.smoke else cases
    oracles = [Oracle(), Oracle()]; rows = []; different = []
    # 53비트/64비트 결과를 둘 다 실행하되 제품 기본53비트의 원문을 fixture에 보존한다.
    for index, case in enumerate(cases):
        observations = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x027f, 0x037f))]
        if observations[0] != observations[1]:
            different.append(dict(index=index, secondary=observations[1]))
        rows.append('\t'.join([*map(str, case), *observations[0]]))
    if args.smoke:
        print('빔 경로 생성/진행 smoke 정상 반환'); return
    FIXTURE.write_text('# kind source target sx sy tx ty scaleX scaleY seed steps trace72 particles24x120; 포인터만 index+1/owner1로 정규화\n' + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_lightning_path.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(cases), inputsByKind=dict(Counter(str(case[0]) for case in cases)), normalReturnsPerPrecision=oracles[0].returns, normalReturns=sum(oracle.returns for oracle in oracles), precisionDifferentInputs=different, nativeCalls=dict(oracles[0].calls), sha256={path: digest(ROOT / path) for path in paths}, limitations=['실제 경로 생성/Seed/Step·재귀·Append·난수·타입/중심과 x87 sin/cos/atan2/sqrt를 대체 없이 실행했다.', '포인터 두 종류만 입자 index+1·owner1로 정규화했다. 나머지64BYTE와 용량120 전체를 보존했다.', '입자/소유 빔/자산/시계는 독립 입력이다. 실제 빔 생성/Host/Frame/Draw/피해나 사용자 자연 완주의 증거가 아니다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('빔 경로:', len(cases), '고유 입력, 정밀도 차이', len(different))


if __name__ == '__main__':
    main()
