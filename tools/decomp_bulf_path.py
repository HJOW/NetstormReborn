#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 Bulf41 생성자/전체 프레임/회전 helper를 명시 외부 입력으로 격리 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import zlib
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, TYPES, VTABLE, IMAGE
from decomp_particle_gradient import digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP
from decomp_ai_initial_update import Oracle as AiInitialOracle, TYPES as AI_TYPES

HEAP, SIZE = 0x14500000, 0x186f0  # 실제 경로의100080바이트 할당과 동일한 격리 영역이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bulf-path-x86.tsv'  # 기존 이동 기대값은 수정하지 않는 독립 관측이다.
REPORT = ROOT / 'cpppj/bulf-path-20261022.json'  # 현재 호스트/보호 원문/실제 실행 범위를 보존한다.
THEME_FIXTURE = ROOT / 'cpppj/tests/fixtures/ai-initial-update-theme5-x86.tsv'  # TEST02의전체기술에서드러난별도theme5관측이다.
UNPOP, POP = STOP + 0x500, STOP + 0x520  # 부모의 두 가상 공간 효과를 구분하는 불투명 주소다.


def bits(value):
    """원본 float 입력을 반올림한 DWORD로 만든다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """출력 예상 없이 미초기화 heap·이동 각도·현재 프레임·진입 관문을 교차한다."""
    result = []
    # 생성자 할당 실패와 서로 다른 원래 heap 바이트도 보존한다.
    for fill, allocation in itertools.product((0, 0xa5, 0xff), (0, 1)):
        result.append((0, fill, allocation, 0, 0, 0, 0, 0, 0, 4000, 0))
    # 열은 op/이전각도/새각도/현재frame/방향/이동dx/dy/shape간격/관문/난수/Pop좌표변경이다.
    for frame, direction in itertools.product((0, 2, 3, 7, 8, 15, 16, 23, 24, 31, 32, 39, 40, 47), (0, 2, 4, 6)):
        angle = direction * 45.0
        result.append((1, bits(angle), bits(angle), frame, direction, bits(1), bits(0), bits(.25), 0, 4000, 0))
    # 다른 회전 방향/180도 경계와 동일한 비직각 각도에서는 보행을 갱신하지 않는 분기다.
    for old, new in itertools.product((0., 90., 180., 270., 45.), (0., 22.5, 45., 90., 180., 180.00002, 270., 359.99997)):
        result.append((1, bits(old), bits(new), 3, 0, bits(1), bits(0), bits(.25), 0, 4000, 0))
    # immobile/finished/Halt/Step 완료와 걷기 간격의 엄격한 경계다.
    for state, dx in itertools.product(range(5), (0., .25, .25001, 1.)):
        result.append((1, 0, 0, 2, 0, bits(dx), 0, bits(.25), state, 4000, 0))
    # 소리 파일 번호와 Pop 뒤 현재 좌표 재조회를 별도 입력으로 관측한다.
    for random, mutate in itertools.product((1000, 3999, 5999), (0, 1)):
        result.append((1, 0, 0, 2, 0, bits(1), 0, bits(.25), 0, random, mutate))
    # helper는 프레임 범위 진단 밖에서 signed64 절삭/NaN/경계 float도 그대로 실행한다.
    for angle, counter in itertools.product((0., -.0, .5, 22.5, 90., 179.9, 180., 270., 359.99997, 360., -1., -90., float('inf'), float('nan')), (0, 1)):
        result.append((2, 0, bits(angle), 0, counter, 0, 0, 0, 0, 4000, 0))
    profiled = []  # profile0은 합성8/8, profile1은 실제 Bulf처럼 보행8/회전24다.
    # 생성자는 한 번만, 나머지는 서로 다른 회전 구간 길이로 각각 실행한다.
    for row in result:
        for profile in ((0,) if not row[0] else (0, 1)):
            profiled.append((*row, profile))
    # 실제 E의 끝과 F의 시작/끝에서도 A~D 복귀를 확인한다.
    for frame, direction in itertools.product((55, 56, 79), (0, 2, 4, 6)):
        profiled.append((1, bits(direction*45.), bits(direction*45.), frame, direction, bits(1), 0, bits(.25), 0, 4000, 0, 1))
    return list(dict.fromkeys(profiled))


class Oracle(BaseOracle):
    """실제48bf00/48b2c0/48b760/48ba30과 CRT를 실행한다. Step/메타/큰 장치는 명시 경계다."""
    def __init__(self):
        """원본 메모리 이미지와 격리된 heap/메타만 준비한다."""
        super().__init__()
        self.mu.mem_map(HEAP, 0x19000)
        self.allowed = []  # 현재 PC의 정밀 함수표가 지정한 원문 구간이다.
        ranges(self, (0x48bf00, 0x48b2c0, 0x48b760, 0x48ba30, 0x43fa50))
        self.allowed.append((0x4e4900, 0x500000))  # 원본 fmod/ftol CRT와 그 하위 계산만 실행한다.
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.calls = collections.Counter()  # 실제 세 상위 몸체의 진입 수다.
        self.instructions = 0  # 실제 실행한 명령 수다.
        self.value = ()  # 현재 명시 입력이며 출력 계산에 사용하지 않는다.
        self.events = []  # 현재 입력에서 요청한 외부 효과의 전체 순서다.

    def instruction(self, mu, address, size, _):
        """하위 입력·가상 효과만 분리하고 실제 프레임 산술과 분기는 실행한다."""
        self.instructions += 1
        sp = mu.reg_read(UC_X86_REG_ESP)
        raw = POOL + 50 * 50
        if address in (0x48bf00, 0x48b2c0, 0x48b760, 0x48ba30): self.calls[f'{address:08x}'] += 1
        if address == 0x4e4391:
            assert self.get(sp + 4) == SIZE
            self.event('N', SIZE); self.ret(HEAP if self.value[2] else 0); return
        if address == 0x4287a0: self.event('I'); self.ret(int(self.value[8] == 1)); return
        if address == 0x489ff0: self.event('H'); self.ret(int(self.value[8] == 3)); return
        if address == 0x48a330:
            self.event('S')
            self.put(self.get(sp + 4), bits(10 + struct.unpack('<f', struct.pack('<I', self.value[5]))[0]))
            self.put(self.get(sp + 4) + 4, bits(20 + struct.unpack('<f', struct.pack('<I', self.value[6]))[0]))
            self.put(self.get(sp + 8), self.value[2]); self.put(HEAP + 0x186c4, self.value[2])
            self.ret(int(self.value[8] == 4), 8); return
        if address == 0x48b340: self.event('A'); self.put(HEAP + 0x186c8, 1); self.ret(); return
        if address == 0x41bfb0: self.ret(raw); return
        if address == 0x4ac550: self.ret(TYPES); return
        if address == 0x419850: self.ret(IMAGE, 8); return
        if address == 0x488f80: self.ret(self.value[4], 4); return
        if address == 0x4ad710:
            frame = self.get(raw + 36)
            self.ret(frame // 8 if not self.value[11] or frame < 32 else 4 if frame < 56 else 5); return
        if address == UNPOP: self.event('U', self.get(sp + 4)); self.ret(0, 4); return
        if address == POP:
            self.event('P', self.get(sp + 4), self.get(sp + 8), self.get(sp + 12))
            self.put(raw + 14, bits(33) if self.value[10] else self.get(sp + 4))
            self.put(raw + 18, bits(44) if self.value[10] else self.get(sp + 8))
            self.ret(0, 12); return
        if address == 0x4558f0:
            self.event('R', self.get(sp + 4), self.get(sp + 8)); self.ret(self.value[9]); return
        if address == 0x4a89c0:
            assert self.string(self.get(sp + 8)) == 'bulfWalk%d.wav'
            name = f'bulfWalk{self.get(sp + 12)}.wav'.encode() + b'\0'
            self.mu.mem_write(self.get(sp + 4), name); self.ret(self.get(sp + 4)); return
        if address == 0x4a9d70:
            self.event('W', self.get(sp + 4), self.get(sp + 8), self.string(self.get(sp + 12)), self.get(sp + 16))
            self.ret(); return
        if address == 0x4e0620: raise RuntimeError(f'Bulf 원본 진단 {self.get(sp+12):x}, input={self.value}')
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'Bulf 범위 밖 명령 {address:x}')

    def write(self, mu, access, address, size, value, _):
        """원문 쓰기를 현재 heap/부모50바이트·실제 스택에만 허용한다."""
        if any(low <= address and address + size <= high for low, high in ((HEAP, HEAP + SIZE),
               (POOL + 2500, POOL + 2550), (STACK - 0x7f000, STACK + 0x1000))): return
        raise RuntimeError(f'Bulf 범위 밖 쓰기 {address:x}+{size}')

    def read(self, mu, access, address, size, value, _):
        """원문 읽기도 보호 이미지/명시 메타/스택으로 제한한다."""
        if any(low <= address and address + size <= high for low, high in ((0x400000, 0x600000),
               (HEAP, HEAP + SIZE), (POOL, POOL + 0x10000), (TYPES, TYPES + 0x20000),
               (VTABLE, VTABLE + 0x1000), (IMAGE, IMAGE + 0x1000), (STACK - 0x7f000, STACK + 0x1000))): return
        raise RuntimeError(f'Bulf 범위 밖 읽기 {address:x}+{size}')

    def sample(self, value, precision):
        """한 독립 입력의 전체 경로 체크섬·부모 원문·효과 순서·helper 반환을 보존한다."""
        self.value = value; self.events = []
        raw = POOL + 2500
        self.mu.mem_write(HEAP, bytes([value[1] if not value[0] else 0]) * SIZE)
        self.mu.mem_write(raw, bytes(50))
        self.put(0x5c8464, POOL)
        self.put(raw, VTABLE); self.put(raw + 10, 135, 1); self.put(raw + 14, bits(10)); self.put(raw + 18, bits(20)); self.put(raw + 36, value[3])
        self.put(VTABLE + 0x48, UNPOP); self.put(VTABLE + 0x90, POP)
        self.put(HEAP, 0x50e354); self.put(HEAP + 8, 50); self.put(HEAP + 0x186c4, value[1] if value[0] else 0)
        self.put(HEAP + 0x186c8, int(value[8] == 2)); self.put(HEAP + 0x186c0, 0)
        self.put(HEAP + 0x10, 1); self.put(HEAP + 0x13898, value[4])
        self.put(TYPES + 0x114, 80 if value[11] else 48); self.put(IMAGE + 16, value[7]); self.put(IMAGE + 20, 0)
        # 합성8/8과 실제 Bulf의8/24 구간을 각각 명시 입력으로 공급한다.
        for side in range(6):
            first = side * 8 if not value[11] or side < 5 else 56
            self.put(TYPES + 0x130 + side * 4, first); self.put(TYPES + 0x170 + side * 4, 24 if value[11] and side >= 4 else 8)
        entry = (0x48bf00, 0x48b760, 0x48ba30)[value[0]]
        args = struct.pack('<II', value[2], value[4]) if value[0] == 2 else b''
        if not value[0]: self.mu.mem_write(HEAP, bytes([value[1]]) * SIZE)
        self.call(entry, HEAP, args, 8 if args else 0, precision)
        returned = self.mu.reg_read(UC_X86_REG_EAX) if value[0] == 2 else 0
        self.returns += 1
        return ';'.join(self.events) or '-', str(zlib.adler32(bytes(self.mu.mem_read(HEAP, SIZE)))), bytes(self.mu.mem_read(raw, 50)).hex(), str(returned)


class ThemeOracle(AiInitialOracle):
    """기존 실제AI초기분류 관측에 type1의theme4/5만 명시 입력으로 바꾼다."""
    def __init__(self):
        """기존 원문 호출/하위 경계는 보존하고 새 메타 입력만 보관한다."""
        super().__init__()
        self.theme = 5  # 실제 본문 진입 전에 공급할 타입1의 원소 인덱스다.

    def instruction(self, mu, address, size, data):
        """원문 진입 시 메타 입력을 쓰며 분류/stack/원소 결과는 고치지 않는다."""
        if address == 0x414710: self.put(AI_TYPES + 500 + 0x98, self.theme)
        super().instruction(mu, address, size, data)


def theme_cases():
    """기존선행count와중립battery·여러같은원소를교차하여공유stack칸을관측한다."""
    rows = []; oracle = ThemeOracle()
    # theme4는이전유효경계, theme5는counts[5]와sun battery칸을공유하는현재문제입력이다.
    for theme, pattern, prior in itertools.product((4, 5), (1, 4, 5, 7), (0, 2)):
        oracle.theme = theme
        rows.append((str(theme), *oracle.run((pattern, 1, 1, 0, 1, prior, 19))))
    THEME_FIXTURE.write_text('# theme input body builders players born effects\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    return len(rows)


def main():
    """실제 두 x87 정밀도로 새 원문을 만들거나 저장된 원문/해시만 재확인한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 관측 도구/보호 PE/fixture의 원문을 각각 다시 확인한다.
        for path, expected in report['sha256'].items(): assert digest(ROOT / path) == expected, path
        # 저장 입력과 두 정밀도 결과 수를 실제 생성 목록과 맞춘다.
        rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows)*2
        assert [row[0] for row in rows] == [','.join(map(str, value)) for value in cases()]
        assert all(row[1:5] == row[5:9] for row in rows)
        assert len([line for line in THEME_FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]) == report['theme5Inputs']
        print('Bulf41 evidence hashes: PASS'); return
    oracle = Oracle(); rows = []
    # 기대 출력은 C++ 계산을 호출하지 않고 보호 PE에서만 얻는다.
    for value in cases():
        first, second = [oracle.sample(value, control) for control in (0x27f, 0x37f)]
        assert first == second, value
        rows.append((','.join(map(str, value)), *first, *second))
    FIXTURE.write_text('# input effects53 adler53 raw53 result53 effects64 adler64 raw64 result64\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    theme_count = theme_cases()
    paths = ('originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'originals/d/TEST02.fort', 'originals/d/TEST02.english',
             'tools/decomp_bulf_path.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_particle_gradient.py',
             'tools/decomp_cursor_position.py', 'extracted/refined/originals/functions.tsv', 'cpppj/tests/fixtures/bulf-path-x86.tsv',
             'cpppj/src/o/RawBulfFrame.cpp', 'cpppj/src/o/RawBulfFrame.h', 'cpppj/src/o/RawPathRuntime.cpp', 'cpppj/src/o/RawPathRuntime.h',
             'cpppj/src/o/RawPathProcess.cpp', 'cpppj/src/o/RawPathProcess.h', 'cpppj/src/o/RawPathStart.cpp',
             'cpppj/src/client/RawCampaignPathBindings.cpp', 'cpppj/src/client/RawCampaignPathBindings.h',
             'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/tests/BulfPathTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
             'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/src/o/RawAiBrainInitialUpdate.cpp',
             'cpppj/tests/AiBrainInitialUpdateTests.cpp', 'cpppj/tests/fixtures/ai-initial-update-theme5-x86.tsv',
             'tools/decomp_ai_initial_update.py', 'tools/decomp_player_technology.py', 'tools/decomp_priestmenu.py',
             'extracted/ai-initial-update/originals/functions.tsv', 'extracted/player-technology/originals/functions.tsv',
             'extracted/refined/originals/Netstorm.c', 'originals/netstorm.tarc', 'cpppj/src/client/GameWorld.cpp',
             'cpppj/src/client/RawPriestWorldBindings.cpp', 'cpppj/src/o/RawCarrierMove.cpp')
    image = pefile.PE(str(ROOT / paths[0])).get_memory_mapped_image()
    # 보호 이미지의 실제41 가상 표와 부모135의 이동 관련 다섯 슬롯을 직접 읽는다.
    tables = {hex(table): {hex(offset): hex(struct.unpack_from('<I', image, table - 0x400000 + offset)[0])
              for offset in offsets} for table, offsets in ((0x50e354, range(0, 52, 4)), (0x503e50, (0x38, 0x3c, 0x40, 0x44, 0xc8)))}
    # 각 의존 원문을 변환 없이 해시하여 실제 실행 출처를 보존한다.
    report = dict(host=socket.gethostname(), lastFullDecompileHost='VM-W11-Codex', lastFullDecompileDate='2026-10-22',
                  uniqueInputs=len(rows), normalReturns=oracle.returns, instructions=oracle.instructions, nativeCalls=dict(oracle.calls), vtables=tables, theme5Inputs=theme_count,
                  boundaries='48bf00/48b2c0/48b760/48ba30 및 실제 CRT. new/Step/메타 조회/immobile/Halt/큰 도착/공간/난수/음향은 명시 외부 경계. 전체100080BYTE/부모50BYTE/효과/GP/스택/canary/x87/접근 범위를 검사한다.',
                  sha256={path: digest(ROOT / path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}))


if __name__ == '__main__': main()
