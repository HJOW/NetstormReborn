#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE Walker38 생성자/전체 프레임과 실제 애니메이션 helper를 격리 관찰한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import zlib
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, TYPES, VTABLE, IMAGE
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP

HEAP, SIZE = 0x14500000, 0x186f0  # 원본 new100080의 격리 메모리다.
UNPOP, POP = STOP + 0x500, STOP + 0x520  # 현재 부모의 두 가상 공간 효과 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/walker-path-x86.tsv'  # 기존 이동 기대값과 분리한 새 원문이다.
REPORT = ROOT / 'cpppj/walker-path-20261022.json'  # 출처/호스트/실행 경계를 보존한다.


def bits(value):
    """명시 float 입력을 실제 DWORD로 반올림한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def as_float(value):
    """각도/상대 좌표의 원래 float 비트를 읽는다."""
    return struct.unpack('<f', struct.pack('<I', value))[0]


def next_time(code):
    """예약과 현재10초의 직전/동일/직후 및 unordered 입력을 공급한다."""
    return (0., 10., 10.000000000000002, float('nan'), 9.999999999999998,
            float('inf'), -float('inf'))[code]


def cases():
    """예상 출력 없이 직각/회전/예약/관문/공유 counter를 교차한다."""
    rows = []
    # 생성자는 할당 실패와 미초기화 세 패턴을 포함한다.
    for fill, allocation in itertools.product((0, 0xa5, 0xff), (0, 1)):
        rows.append((0, fill, allocation, 137, 0, 0, 0, 0, 0, 0, 0, 0))
    # 각 열은 op/fill/allocation/type/angle/frame/counter/dx/dy/gate/next/profile이다.
    for angle, frame, counter in itertools.product((0., -0., 45., 90., 180., 270., 359.99997),
                                                   (0, 1, 3, 15, 34), (0, 3, 0xffffffff)):
        rows.append((1, 0, 1, 137, bits(angle), frame, counter, bits(1), 0, 0, 0, 0))
    # float 저장 및 CRT 경계의 정상/비정상 회전 입력도 원문에서 직접 관찰한다.
    for angle in (-90., -1., .00001, 22.5, 179.99998, 180.00002, 269.99997, 360., 720., float('inf'), float('nan')):
        rows.append((1, 0, 1, 137, bits(angle), 0, 17, 0, 0, 0, 0, 0))
    # 레인은 A/B 구간의 처음/끝/다른 구간 및 시각 분기를 교차한다.
    for angle, frame, code, profile in itertools.product((0., 90., 180., 270., 45.),
                                                       (0, 7, 8, 15, 0xffffffff), (0, 1, 2, 3), (0, 1)):
        rows.append((1, 0, 1, 156, bits(angle), frame, 3, bits(1), 0, 0, code, profile))
    # 완료/정지/낙하를 실제 프레임 본문이 어느 순서로 확인하는지 보존한다.
    for kind, gate, dx in itertools.product((137, 156), range(6), (0., 1.)):
        rows.append((1, 0, 1, kind, bits(0), 1, 0, bits(dx), 0, gate, 0, 0))
    for code in (4, 5, 6):
        rows.append((1, 0, 1, 156, bits(90), 7, 0, 0, bits(1), 0, code, 0))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """원문 산술/분기/프레임 helper는 실행하며 Step/지면/가상 효과만 명시 경계로 둔다."""
    def __init__(self):
        """실제 함수 구간과 읽기/쓰기 가드를 설치한다."""
        super().__init__()
        self.mu.mem_map(HEAP, 0x19000)
        self.allowed = []  # 정밀 디컴파일 함수표의 실제 바이트 범위다.
        ranges(self, (0x48bed0, 0x48b2c0, 0x48b080, 0x447790, 0x43fa50, 0x40e850, 0x40eab0, 0x4e49c0))
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.calls = collections.Counter()  # 실제 본문/helper 진입 횟수다.
        self.instructions = 0  # 실행 명령 수다.
        self.value = ()  # 현재 외부 입력이다.
        self.events = []  # 순서가 있는 외부 효과 원문이다.

    def instruction(self, mu, address, size, _):
        """큰 하위만 격리하며 CRT/helper 반환은 대체하지 않는다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        raw = POOL + 2500
        if address in (0x48bed0, 0x48b2c0, 0x48b080, 0x447790, 0x43fa50, 0x40e850, 0x40eab0, 0x4e49c0):
            self.calls[f'{address:08x}'] += 1
        if address == 0x4e4391:
            assert self.get(sp+4) == SIZE
            self.event('N', SIZE); self.ret(HEAP if self.value[2] else 0); return
        if address == 0x4287a0: self.event('I'); self.ret(int(self.value[9] == 1)); return
        if address == 0x489ff0: self.event('H'); self.ret(int(self.value[9] == 3)); return
        if address == 0x48a330:
            self.event('S')
            self.put(self.get(sp+4), bits(10+as_float(self.value[7])))
            self.put(self.get(sp+4)+4, bits(20+as_float(self.value[8])))
            self.put(self.get(sp+8), self.value[4]); self.put(HEAP+0x186c4, self.value[4])
            self.ret(int(self.value[9] == 4), 8); return
        if address == 0x48a7c0:
            self.event('G', self.get(sp+4), self.get(sp+8)); self.ret(int(self.value[9] == 5), 8); return
        if address == 0x48b340: self.event('A'); self.put(HEAP+0x186c8, 1); self.ret(); return
        if address == 0x4ac550: self.ret(TYPES); return
        if address == UNPOP: self.event('U', self.get(sp+4)); self.ret(0, 4); return
        if address == POP:
            self.event('P', self.get(sp+4), self.get(sp+8), self.get(sp+12))
            self.put(raw+14, self.get(sp+4)); self.put(raw+18, self.get(sp+8)); self.ret(0, 12); return
        if address == 0x4e0620: raise RuntimeError(f'Walker 원본 진단 {self.get(sp+12):x}')
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Walker 범위 밖 실행 {address:x}')

    def write(self, mu, access, address, size, value, _):
        """현재 heap/부모/공유 counter/스택 밖 쓰기를 차단한다."""
        if any(low <= address and address+size <= high for low, high in ((HEAP, HEAP+SIZE),
               (POOL+2500, POOL+2550), (0x5951c4, 0x5951c8), (STACK-0x7f000, STACK+0x1000))): return
        raise RuntimeError(f'Walker 범위 밖 쓰기 {address:x}+{size}')

    def read(self, mu, access, address, size, value, _):
        """보호 이미지/명시 메타/경로/스택만 읽게 한다."""
        if any(low <= address and address+size <= high for low, high in ((0x400000, 0x600000),
               (HEAP, HEAP+SIZE), (POOL, POOL+0x10000), (TYPES, TYPES+0x20000),
               (VTABLE, VTABLE+0x1000), (IMAGE, IMAGE+0x1000), (STACK-0x7f000, STACK+0x1000))): return
        raise RuntimeError(f'Walker 범위 밖 읽기 {address:x}+{size}')

    def sample(self, value, precision):
        """전체 경로/부모/공유 전역/효과를 변환 없이 캡처한다."""
        self.value = value; self.events = []
        raw = POOL+2500
        self.mu.mem_write(HEAP, bytes([value[1]])*SIZE); self.mu.mem_write(raw, bytes(50))
        self.put(raw, VTABLE); self.put(raw+10, value[3], 1)
        self.put(raw+14, bits(10)); self.put(raw+18, bits(20)); self.put(raw+36, value[5])
        self.put(VTABLE+0x48, UNPOP); self.put(VTABLE+0x90, POP)
        self.put(0x5c8464, POOL); self.put(0x54127c, 137); self.put(0x5412c8, 156)
        self.put(0x5951c4, value[6]); self.mu.mem_write(0x55b4d0, struct.pack('<d', 10.))
        if value[0]:
            self.put(HEAP, 0x50e320); self.put(HEAP+8, 50); self.put(HEAP+0x10, 1)
            self.put(HEAP+0x186c8, int(value[9] == 2))
            self.mu.mem_write(HEAP+0x186b8, struct.pack('<d', next_time(value[10])))
        # TYPE의 두 연속 구간은 실제447790이 직접 읽는다.
        for index, first, count in ((0, 3, 5), (1, 11, 7)) if value[11] else ((0, 0, 8), (1, 8, 8)):
            self.put(TYPES+0x130+index*4, first); self.put(TYPES+0x170+index*4, count)
        self.call(0x48b080 if value[0] else 0x48bed0, HEAP, b'', 0, precision); self.returns += 1
        return (';'.join(self.events) or '-', str(zlib.adler32(bytes(self.mu.mem_read(HEAP, SIZE)))),
                bytes(self.mu.mem_read(raw, 50)).hex(), str(self.get(0x5951c4)))


def main():
    """두 x87 정밀도 원문을 생성하거나 동결한 출처 해시만 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 제품 변경 후에는 원문 fixture를 재생성하지 않고 해시 불일치를 드러낸다.
        for path, expected in {**report['sha256'], **report.get('implementationSha256', {})}.items():
            assert digest(ROOT/path) == expected, path
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows)*2
        assert [row[0] for row in rows] == [','.join(map(str, value)) for value in cases()]
        assert all(row[1:5] == row[5:9] for row in rows)
        print('Walker38 근거 SHA/원문/고유 입력: PASS', len(rows)); return
    oracles = [Oracle(), Oracle()]; rows = []
    # C++ 계산 없이 각 정밀도의 보호 원문에서만 기대값을 얻는다.
    for value in cases():
        outputs = [oracle.sample(value, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], value
        rows.append((','.join(map(str, value)), *outputs[0], *outputs[1]))
    FIXTURE.write_text('# input effects53 adler53 raw53 counter53 effects64 adler64 raw64 counter64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_walker_path.py',
             'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_particle_gradient.py',
             'cpppj/tests/fixtures/walker-path-x86.tsv')
    implementation = ('cpppj/src/o/RawWalkerFrame.h', 'cpppj/src/o/RawWalkerFrame.cpp',
                      'cpppj/src/o/RawPathRuntime.h', 'cpppj/src/o/RawPathRuntime.cpp',
                      'cpppj/src/o/RawPathProcess.h', 'cpppj/src/o/RawPathProcess.cpp', 'cpppj/src/o/RawPathStart.cpp',
                      'cpppj/src/client/RawCampaignPathBindings.h', 'cpppj/src/client/RawCampaignPathBindings.cpp',
                      'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/RawCarrierSteal.h', 'cpppj/src/o/RawCarrierSteal.cpp',
                      'cpppj/tests/WalkerPathTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
                      'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt')  # C++ 대조에 사용한 제품/검사 원문이다.
    image = pefile.PE(str(ROOT/paths[0])).get_memory_mapped_image()
    tables = {hex(table): {hex(offset): hex(struct.unpack_from('<I', image, table-0x400000+offset)[0])
              for offset in offsets} for table, offsets in ((0x50e320, range(0,52,4)), (0x503d80, (0x38,0x3c,0x40,0x44,0xc8)), (0x503f20, (0x38,0x3c,0x40,0x44,0xc8)))}
    report = dict(host=socket.gethostname(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-22',
                  uniqueInputs=len(rows), normalReturns=sum(oracle.returns for oracle in oracles),
                  instructions=sum(oracle.instructions for oracle in oracles), nativeCalls=dict(sum((oracle.calls for oracle in oracles), collections.Counter())),
                  vtables=tables, boundaries='48bed0/48b2c0/48b080/447790/43fa50/40e850/40eab0/CRT4e49c0을 실제 실행. 할당/immobile/Halt/Step/지면/도착/TYPE getter/공간은 명시 경계. 전체100080BYTE/부모50BYTE/공유counter/외부효과/GP/스택/canary/x87/접근범위를 검사.',
                  sha256={path: digest(ROOT/path) for path in paths}, implementationSha256={path: digest(ROOT/path) for path in implementation})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs','normalReturns','instructions','nativeCalls')}))


if __name__ == '__main__': main()
