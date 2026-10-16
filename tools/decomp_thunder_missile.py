#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 TYPE131의 실제 포구/사정선·Missile 생성/프레임을 독립 x86으로 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_crossbow_missile import KEYS as COMMON_KEYS, sample as common_sample
from decomp_gunmissile_oracle import MissileOracle, SPEC, OBJECT, VTABLE, HEADER, OWNER, POP, UNPOP, DESTROY, bits
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

KEYS = (*COMMON_KEYS, 'sourceFrame', 'side')  # 앞21열은 기존 독립 입력 형식이며 현재 물리 프레임/문자를 추가한다.
OUTPUT, CODES = HEADER + 0x800, HEADER + 0x1000  # 반환 좌표와 실제4BYTE 프레임 코드의 격리 메모리다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/thunder-missile-x86.tsv'  # 기존 썬/크로스보우 관찰을 덮지 않는 기대값이다.
REPORT = ROOT / 'cpppj/thunder-missile.json'  # 실제 몸체/외부 경계·고유 입력·정밀도·SHA 기록이다.
NATIVE = {0x481a50, 0x482340, 0x40e800, 0x40e880, 0x40e8f0, 0x41cfc0, 0x41d0d0, 0x41d180,
          0x41d6e0, 0x468cc0, 0x431f90, 0x43fa50, 0x4e49c0, 0x481750, 0x41ce90, 0x466df0,
          0x4ac550, 0x49a840, 0x49ace0, 0x45ae60, 0x4d4d00, 0x4ad680, 0x4adea0, 0x49a900, 0x4ac950}  # 실제로 실행할 전체 수치/좌표/방향/프레임 몸체다.
ACTUAL = {0x481750, 0x4ac550, 0x49a840, 0x4adea0, 0x49a900, 0x4ac950}  # 선행 Oracle의 외부 자료 대체 대신 실행하는 실제 함수다.


def sample(**values):
    """독립 입력 기본값만 구성하며 원본의 필드/좌표/픽셀 결과는 계산하지 않는다."""
    result = common_sample()
    result.update(sourceFrame=3, side=ord('N'))
    result.update(values)
    return result


def cases():
    """생성 보정/모든 방향·일반 및 코너 프레임·미사용 문자 fallback을 방문한다."""
    rows = []
    # 같은 사정선 방향과 다른 사정선 방향의 실제 생성 필드를 모두 남긴다.
    for side, point, hot, typehot, corner, sound in itertools.product(
            map(ord, 'LMNO'), ((50, 50), (80.25, 64.75)), ((0, 0), (16, 11), (-8, -5)),
            ((0, 0), (16, 11)), (0, 1), ((0, 0), (0, 789), (123, 789))):
        rows.append(sample(side=side, x=bits(point[0]), y=bits(point[1]), hotx=hot[0], hoty=hot[1], typehotx=typehot[0], typehoty=typehot[1], corner=corner, firingSound=sound[0], displaySound=sound[1]))
    # 보호 자원의 L/M/N/O 마지막 프레임·사거리42와 표시4방향/12프레임을 실제 조합으로 공급한다.
    for (frame, side), end, corner in itertools.product(((8, 76), (17, 77), (26, 78), (35, 79)), ((50, 10), (90, 50), (50, 90), (10, 50)), (0, 1)):
        rows.append(sample(sourceFrame=frame, side=side, endx=bits(end[0]), endy=bits(end[1]), corner=corner, range=bits(42), directions=4, frames=12))
    # frame은 ctor 출력에 의존하지 않는2초 비행 이미지다.
    for fraction, state, previous, corner, distance, sound in itertools.product((0, 1, 3, 4, 5, 8, 12), range(4), (0, 1), (0, 1), (9, 40), (0, 456)):
        rows.append(sample(kind=1, endx=bits(50), endy=bits(90), fraction=fraction, targetstate=state, previous=previous, corner=corner, range=bits(distance), impactSound=sound))
    # 실제 충돌 억제 두 플래그도 별도 입력으로 관찰한다.
    for extra, passed, corner in itertools.product((0, 16), (0, 1), (0, 1)):
        rows.append(sample(kind=1, endx=bits(50), endy=bits(90), fraction=4, previous=1, extra=extra, passed=passed, corner=corner))
    # 포구는 현재 프레임/astral에 관계없이 원시 좌표를 복사한다.
    for point, extra in itertools.product(((.5, 10.25), (94.125, 108.375), (-10000, -10000), (255.75, 255.5)), (0, 1)):
        rows.append(sample(kind=2, x=bits(point[0]), y=bits(point[1]), extra=extra))
    # signed 문자 fallback과 동률 세로 우선·음수/경계 포구·절삭 사거리를 함께 방문한다.
    for side, point, delta, distance in itertools.product((76, 77, 78, 79, 65, 0, 127, 255), ((10, 20), (254.75, -1.5)), ((0, -20), (20, 0), (0, 20), (-20, 0), (20, 20), (-20, 20), (0, 0)), (.9, 9.75, -2.5)):
        rows.append(sample(kind=3, side=side, x=bits(point[0]), y=bits(point[1]), endx=bits(point[0] + delta[0]), endy=bits(point[1] + delta[1]), range=bits(distance)))
    return list({tuple(row[key] for key in KEYS): row for row in rows}.values())


class Oracle(MissileOracle):
    """외부 자산/SHP/사운드/피해만 경계로 두고 실제 썬더 수치 경로를 실행한다."""
    def __init__(self, control):
        """현재 PC 정밀 범위와 보호 PE의 TYPE131 가상 포구를 확인한다."""
        OwnerOracle.__init__(self, 'originals')
        self.p = SPEC['originals']; self.control = control; self.allowed = []; self.events = []; self.returns = 0
        self.mu.mem_map(0, 0x1000)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.entries.update(NATIVE)
        # 각 실제 몸체의 불연속 범위를 그대로 등록한다.
        for entry in NATIVE:
            # 원문에 기록된 구간 밖의 실행을 허용하지 않는다.
            for segment in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in segment.split('-'))
                self.allowed.append((low, high + 1))
        self.entries.add(0x41c110); self.allowed.append((0x41c110, 0x41c122))
        assert self.read(0x50bad0 + 0xb4) == 0x41c110
        self.boundaries = Counter()

    def on_instruction(self, mu, address, size, data):
        """실제 선/타입/방향/치수/중심 조회는 선행 Oracle의 대체 분기로 보내지 않는다."""
        if address in ACTUAL:
            OwnerOracle.on_instruction(self, mu, address, size, data)
        else:
            super().on_instruction(mu, address, size, data)

    def run_body(self, case):
        """전체128BYTE·세 슬롯·외부 사건과 정상 반환/ABI/x87/쓰기 범위를 기록한다."""
        self.case = case; self.events = []; mu = self.mu; p = self.p; kind = case['kind']
        mu.mem_write(0, bytes(4)); mu.mem_write(OUTPUT, bytes(8)); mu.mem_write(CODES, bytes(144))
        fields = bytearray((index * 13 + 7) & 255 for index in range(128))
        mu.mem_write(OBJECT, bytes(fields)); mu.mem_write(VTABLE, bytes(0x400))
        # 실제 포구와 명시 외부 자산 가상 효과를 서로 구별한다.
        for offset, entry in ((0xb4, 0x41c110), (0x74, OWNER), (0x90, POP), (0x48, UNPOP), (0x10, DESTROY)):
            mu.mem_write(VTABLE + offset, struct.pack('<I', entry))
        # 같은 source/target/display 세 슬롯과 실제 타입 필드만 공급한다.
        for sid, typeid in ((50, 131), (60, 71), (90, 132)):
            raw = bytearray(50); struct.pack_into('<I', raw, 0, VTABLE); raw[10] = typeid; raw[34] = 1
            raw[11] = case['targetstate'] if sid == 60 else 0; raw[40] = case['extra'] if sid in (50, 60) else 0
            struct.pack_into('<II', raw, 14, case['x'] if sid == 50 else bits(50), case['y'] if sid == 50 else bits(50))
            struct.pack_into('<I', raw, 36, case['sourceFrame'] if sid == 50 else 0)
            mu.mem_write(self.slot(sid), bytes(raw)); mu.mem_write(TYPES + typeid * self.type_stride, bytes(self.type_stride))
        source = TYPES + 131 * self.type_stride; display = TYPES + 132 * self.type_stride
        mu.mem_write(source + 0x44, struct.pack('<I', case['range'])); mu.mem_write(source + 0x70, struct.pack('<I', case['firingSound']))
        mu.mem_write(source + 0x9c, struct.pack('<I', 1)); mu.mem_write(source + 0x124, struct.pack('<I', CODES))
        mu.mem_write(source + 0x1d4, struct.pack('<4i', 0, 0, case['typehotx'], case['typehoty']))
        mu.mem_write(CODES + case['sourceFrame'] * 4, bytes([case['side'], 0, 0, 0]))
        mu.mem_write(TYPES + 71 * self.type_stride + 0x1d4, struct.pack('<2i', 2, 3))
        mu.mem_write(display + 0x70, struct.pack('<II', case['displaySound'], case['impactSound']))
        mu.mem_write(display + 0x110, struct.pack('<ii', case['directions'], case['frames']))
        mu.mem_write(TYPES + 136 * self.type_stride + 0x74, struct.pack('<I', case['impactSound']))
        mu.mem_write(HEADER + 0x620, struct.pack('<hh', case['hotx'], case['hoty']))
        # 같은 판본의 현재 전역 번호이며 실제 산술/프레임 분기를 대체하지 않는다.
        for address, value in ((p['capacity'], 188), (p['checking'], 0), (p['sun'], 128), (p['disc'], 83), (p['direct'], 71), (p['process'], 28), (p['impactType'], 136), (0x541268, 132), (0x541260, 130), (0x541288, 140), (0x5412a0, 146), (0x54daa4, 0)):
            mu.mem_write(address, struct.pack('<I', value))
        mu.mem_write(p['scaleX'], struct.pack('<ff', 16, 11))
        if kind == 0:
            mu.mem_write(p['now'], struct.pack('<d', 10.25)); corner = (case['endx'], case['endy']) if case['corner'] else (bits(-10000), bits(-10000))
            args = [50, 60, bits(20), 132, 0, case['endx'], case['endy'], *corner, 0, bits(-10000), bits(-10000)]; entry = p['ctor']; this = OBJECT
        elif kind == 1:
            struct.pack_into('<8I', fields, 0, p['vtable'], 81, 90, 0, case['passed'], 50, 60, case['corner'])
            struct.pack_into('<4I2f', fields, 32, 0, 0, 0, 0, 20, 0)
            struct.pack_into('<6I', fields, 56, case['x'], case['y'], case['endx'], case['endy'], case['endx'], case['endy'])
            struct.pack_into('<4f2d2f', fields, 88, -10000, -10000, -10000, -10000, 12, 10, 50, 89 if case['previous'] else -10000)
            mu.mem_write(OBJECT, bytes(fields)); mu.mem_write(p['now'], struct.pack('<d', 10 + case['fraction'] * .5))
            args = []; entry = p['frame']; this = OBJECT
        elif kind == 2:
            args = [OUTPUT]; entry = self.read(0x50bad0 + 0xb4); this = self.slot(50)
        else:
            args = [OUTPUT, case['x'], case['y'], case['endx'], case['endy'], 50]; entry = p['ray']; this = 0
        self.write_ranges = [(0, 4), (OBJECT, OBJECT + 128), (OUTPUT, OUTPUT + 8)] + [(self.slot(sid), self.slot(sid) + 50) for sid in (50, 60, 90)]
        saved = {register: 0x12340000 + index for index, register in enumerate((UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP))}
        # 비휘발/일반 레지스터를 이전 관찰과 독립인 입력으로 설정한다.
        for register, value in saved.items():
            mu.reg_write(register, value)
        mu.reg_write(UC_X86_REG_EAX, 0); mu.reg_write(UC_X86_REG_EDX, 0); mu.reg_write(UC_X86_REG_ECX, this); mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2); mu.reg_write(UC_X86_REG_FPCW, self.control); mu.reg_write(UC_X86_REG_FPSW, 0); mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 2), STOP, *args, 0xc0dec0de))
        mu.emu_start(entry, STOP, count=100000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + (len(args) * 4 if kind != 3 else 0)
        assert all(mu.reg_read(register) == value for register, value in saved.items())
        assert mu.reg_read(UC_X86_REG_FPCW) == self.control and mu.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0 and mu.reg_read(UC_X86_REG_FPTAG) == 0xffff and self.read(0) == 0
        assert self.read(STACK + 4 + len(args) * 4) == 0xc0dec0de
        if kind in (0, 2, 3):
            assert mu.reg_read(UC_X86_REG_EAX) == (OBJECT if kind == 0 else OUTPUT)
        self.returns += 1
        return [';'.join(self.events), bytes(mu.mem_read(OBJECT, 128)).hex(), b''.join(bytes(mu.mem_read(self.slot(sid), 50)) for sid in (50, 60, 90)).hex(), bytes(mu.mem_read(OUTPUT, 8)).hex()]


def main():
    """기본/비교 정밀도 원문을 보존하거나 전체 근거 SHA와 입력 고유성을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); parser.add_argument('--smoke', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 기존 결과를 재계산하지 않고 전체 파일 바이트 핀을 확인한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == report['uniqueInputs'] and len({tuple(row[:len(KEYS)]) for row in rows}) == len(rows)
        assert all(len(row) == len(KEYS) + 4 for row in rows)
        assert dict(Counter(row[0] for row in rows)) == report['inputKinds']
        print('썬더 Missile SHA/고유 입력 감사:', len(rows)); return
    inputs = cases()
    if args.smoke:
        inputs = [next(case for case in inputs if case['kind'] == kind) for kind in range(4)]
    oracles = [Oracle(0x027f), Oracle(0x037f)]; rows = []; different = []
    # 각 기대 출력은 실제 원본 CPU에서만 읽고 두 정밀도의 차이도 보존한다.
    for index, case in enumerate(inputs):
        try:
            output = [oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:
            raise RuntimeError(f'썬더 입력{index} {case}: {error}') from error
        if output[0] != output[1]:
            different.append(dict(index=index, secondary=output[1]))
        rows.append('\t'.join([str(case[key]) for key in KEYS] + output[0]))
    if args.smoke:
        print('썬더 Missile 실제4종 smoke 정상 반환'); return
    FIXTURE.write_text('# 실제 TYPE131 포구/사정선/Missile 생성/프레임. SHP 자료와 외부 월드 효과는 명시 경계다.\n' + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'originals/netstorm.tarc', 'extracted/refined/originals/functions.tsv', 'tools/decomp_thunder_missile.py', 'tools/decomp_crossbow_missile.py', 'tools/decomp_gunmissile_oracle.py', 'tools/decomp_owner_oracle.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', uniqueInputs=len(inputs), inputKinds=dict(Counter(str(case['kind']) for case in inputs)), returnsPerPrecision=len(inputs), precisionDifferentInputs=different, nativeCalls=dict(oracles[0].native_calls), boundaries=dict(oracles[0].boundaries), sha256={path: digest(ROOT / path) for path in paths}, limitations=['포구/사정선/방향/타입·치수/중심과 Missile 수치 몸체는 실제 PE 코드다.', 'SHP hotspot 자료·자산 가상 생성/소유자/공간·소리/피해/갱신/일반 충돌 상자는 명시 외부 경계다.', '독립 합성 입력이다. 실제 Session 완주/장치 픽셀/원본 실제 게임 실행 증거로 합산하지 않는다.'])
    # 실행 관찰과 구별해 같은 PE의 표시 생성자/가상 삭제와 정수 여유를 정적 근거로 기록한다.
    assert bytes(oracles[0].mu.mem_read(0x46a6c0, 40)).find(struct.pack('<I', 0x50b570)) >= 0
    report['staticPEEvidence'] = dict(displayConstructor='0046a6c0', displayVtable='0050b570',
        lifetimeSlots={f'{offset:02x}': f'{oracles[0].read(0x50b570 + offset):08x}' for offset in (0x10, 0x14, 0x18, 0x24)},
        rangeMargin=oracles[0].read(0x540bb4), thunderRangeMargin=oracles[0].read(0x540bb8))
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('썬더 Missile:', len(inputs), '고유 입력, 두 정밀도 정상 반환', sum(oracle.returns for oracle in oracles), '차이', len(different))


if __name__ == '__main__':
    main()
