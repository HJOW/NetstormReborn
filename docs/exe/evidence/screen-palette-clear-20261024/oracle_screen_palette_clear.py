#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사본 제품 전에 004a4923의 파일읽기 뒤 관문부터 SetPalette checkpoint까지 독립 PE 효과를 고정한다."""
import itertools
import json
from pathlib import Path
import sys
from collections import Counter

ROOT = Path(__file__).resolve().parents[2]  # 공유 파일은 읽기만 하고 출력은 이 독립 경로에만 만든다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_priestmenu import Oracle as MemoryOracle, POOL, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_FPCW

HERE = Path(__file__).resolve().parent  # 최초 fixture/report는 이곳에 고정한다.
FLAGS, DEPTH, DRAW = 0x5c78d4, 0x5c7920, 0x5c7948  # 현재 모드·중첩잠금·그리기표면 실제 DWORD다.
BUFFER, PITCH, VFX, LOADED = 0x59af88, 0x5c78ec, 0x5c78f8, 0x555e08  # inline unlock이 직접 비우는 실제 필드다.
WIDTH, HEIGHT, CLIP, VIEW = 0x531860, 0x531864, 0x5c7a44, 0x5c7a54  # 현재 크기와 별개의 clip/viewport다.
SURF, OTHER, PIXELS = POOL + 0x1000, POOL + 0x2000, POOL + 0x5000  # 장치 주소와 실제 8×4 BYTE 입력을 구별한다.
RANGES = ((0x4a4923, 0x4a49b0), (0x4a1fb0, 0x4a1fdd), (0x4a46d0, 0x4a4783), (0x4a16d0, 0x4a17b3))  # 파일부모 prefix와 세 실제 후처리 부모만 실행한다.


def cases():
    """clear DWORD·현재 flags·잠금 깊이·clip 및 callback 재조회를 명시 교차한다."""
    rows = [(clear, flags, depth, small, 'none') for clear, flags, depth, small in itertools.product((0, 1, 2), (0, 0x80, 0x82, 0x8a, 0x81, 0x84), (0, 1), (0, 1))]
    # 원문에 없는 callback은 실제 하위 장치가 복귀하며 바꿀 수 있는 효과로만 명시한다.
    for mutation in ('lock-size', 'lock-flags0', 'lock-stop', 'fill-draw', 'fill-flags1', 'fill-stop', 'unlock-buffer', 'unlock-stop', 'update-clip', 'update-flags4', 'update-stop'):
        rows.append((1, 0x8a, 0, 1, mutation))
    return rows


class Oracle(MemoryOracle):
    """원문 분기/inline unlock/clip 복구를 실행하고 OS/GDI 하위만 명시 장치로 둔다."""
    def __init__(self):
        super().__init__()
        self.total = 0
        self.completions = Counter()

    def current(self, address):
        """같은 실제 DWORD의 현재값을 매 경계 읽는다."""
        return self.words(address, 1)[0]

    def mutate(self, boundary):
        """명시 입력의 하위 변경과 중단만 적용한다."""
        mutation = self.case[-1]
        if mutation == boundary + '-stop':
            self.cause = boundary
            self.mu.emu_stop()
        elif mutation == 'lock-size' and boundary == 'lock':
            self.put(WIDTH, 6); self.put(HEIGHT, 3)
        elif mutation == 'lock-flags0' and boundary == 'lock':
            self.put(FLAGS, 0)
        elif mutation == 'fill-draw' and boundary == 'fill':
            self.put(DRAW, OTHER)
        elif mutation == 'fill-flags1' and boundary == 'fill':
            self.put(FLAGS, 0x81)
        elif mutation == 'unlock-buffer' and boundary == 'unlock':
            self.put(BUFFER, PIXELS + 16); self.put(PITCH, 99); self.put(VFX, PIXELS + 16)
        elif mutation == 'update-clip' and boundary == 'update':
            for index, value in enumerate((2, 2, 3, 3)):
                self.put(CLIP + index * 4, value)
        elif mutation == 'update-flags4' and boundary == 'update':
            self.put(FLAGS, 0x84)

    def instruction(self, mu, address, size, data):
        """네 선택 범위만 실행하며 명시 장치 밖의 게임/OS 코드는 거부한다."""
        self.total += 1
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 5)
        if address == 0x4a14e0:
            depth = self.current(DEPTH)
            self.events.append(f'LOCK:{depth}')
            self.put(DEPTH, depth + 1)
            if depth == 0:
                self.put(BUFFER, PIXELS); self.put(PITCH, 8); self.put(VFX, PIXELS)
            self.mutate('lock')
            if not self.cause: self.ret()
        elif address == 0x4a2060:
            rect = args[:4]
            clip = self.words(CLIP, 4)
            self.events.append('FILL:' + ','.join(map(str, rect)) + ':' + str(args[4]) + ':' + ','.join(map(str, clip)))
            # 실제 FillRect 하위는 기존 제품/검사의 소유다. 이 관측은 명시 물리 장치만 사용한다.
            if self.current(FLAGS) & 0x80 and self.current(BUFFER):
                left, top, right, bottom = tuple(max(clip[index % 2], min(value, clip[index % 2 + 2])) for index, value in enumerate(rect))
                for y in range(top, bottom):
                    self.mu.mem_write(self.current(BUFFER) + y * self.current(PITCH) + left, bytes([args[4] & 255]) * max(right - left, 0))
            self.mutate('fill')
            if not self.cause: self.ret()
        elif address == 0x4a0950:
            self.events.append('UNLOCK:' + str(mu.reg_read(UC_X86_REG_ECX)))
            self.mutate('unlock')
            if not self.cause: self.ret()
        elif address == 0x4a1fb0:
            self.events.append('WAIT:' + str(self.current(FLAGS)))
            if self.current(FLAGS) & 1:
                self.cause = 'directdraw-vblank'; self.mu.emu_stop()
        elif address == 0x4a46d0:
            self.events.append('PRESERVE:' + ','.join(map(str, self.words(CLIP, 4))))
        elif address == 0x4a1800:
            self.events.append('UPDATE:' + ','.join(map(str, args[:4])))
            for index, value in enumerate(self.words(VIEW, 4)):
                self.put(CLIP + index * 4, value)
            self.mutate('update')
            if not self.cause: self.ret()
        elif address == 0x4a16d0:
            self.events.append('FLIP:' + str(self.current(FLAGS)))
            if self.current(FLAGS) & 4:
                self.cause = 'directdraw-flip'; self.mu.emu_stop()
        elif address == 0x4a2640:
            assert args[:4] == (0, 256, 0, 1)
            self.events.append('APPLY:0,256,0,1')
            self.cause = 'checkpoint'; self.mu.emu_stop()
        elif not any(lo <= address < hi for lo, hi in RANGES):
            raise RuntimeError('팔레트 clear 허용 범위 밖 ' + hex(address))

    def sample(self, case, control):
        """정확한 prefix 진입 상태를 넣고 SetPalette 직전까지 부분 효과를 보존한다."""
        self.case, self.cause, self.events = case, '', []
        clear, flags, depth, small, mutation = case
        for address, value in ((FLAGS, flags), (DEPTH, depth), (DRAW, SURF), (BUFFER, PIXELS if depth else 0), (PITCH, 8 if depth else 0), (VFX, PIXELS if depth else 0), (LOADED, 77), (WIDTH, 8), (HEIGHT, 4)):
            self.put(address, value)
        for index, value in enumerate((1, 1, 7, 3) if small else (0, 0, 8, 4)):
            self.put(CLIP + index * 4, value)
        for index, value in enumerate((0, 0, 8, 4)):
            self.put(VIEW + index * 4, value)
        self.mu.mem_write(PIXELS, bytes(range(64)))
        self.mu.mem_write(STACK - 256, b'\xcd' * 512)
        self.put(STACK + 0x38, clear)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EDI, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.emu_start(0x4a4923, STOP)
        self.completions[self.cause] += 1
        values = ','.join(map(str, [self.current(x) for x in (FLAGS, DEPTH, DRAW, BUFFER, PITCH, VFX, LOADED, WIDTH, HEIGHT)] + list(self.words(CLIP, 4))))
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause, values, bytes(self.mu.mem_read(PIXELS, 64)).hex(), ';'.join(self.events)))


def main():
    """원문 관측을 제안 사본보다 먼저 생성하며 이후에는 덮어쓰지 않는다."""
    fixture, report = HERE / 'screen-palette-clear-first.tsv', HERE / 'screen-palette-clear-first.json'
    if '--verify' in sys.argv:
        data = json.loads(report.read_text(encoding='utf-8'))
        for name, expected in data['sha256'].items():
            assert digest(ROOT / name) == expected, name
        print('screen-palette-clear first SHA verified', data['observations']); return
    if fixture.exists() or report.exists(): raise RuntimeError('최초 palette clear 자료 덮어쓰기 금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / clear,flags,depth,smallClip,mutation / stop / current13fields / physical64 / trace']
    # 선택 prefix는 정수뿐이지만 두 정밀도에서 독립 초기화한다.
    for case in inputs:
        for control in (0x27f, 0x37f): lines.append(oracle.sample(case, control))
    fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    names = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', str(Path(__file__).relative_to(ROOT)).replace('\\', '/'), str(fixture.relative_to(ROOT)).replace('\\', '/'), 'cpppj/src/client/Screen.h', 'cpppj/src/client/Screen.cpp')
    result = {'status': '단위384 제안 사본 전 최초 관측', 'entry': '004a4923 after-file-palette-read', 'checkpoint': '004a2640 call before SetPalette body', 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'completions': dict(oracle.completions), 'instructions': oracle.total, 'scope': '파일읽기/색표계산 전체가 아닌 prefix와 실제inline unlock/46d0 clip복구. Lock/FillRect/표면Unlock/Update는 명시 장치. DirectDraw vblank/flip은 실제 부모 진입에서 명시중단. OS/GUI/원본게임 실행 없음.', 'sha256': {name: digest(ROOT / name) for name in names}}
    report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result[key] for key in ('uniqueInputs', 'observations', 'completions', 'instructions')}))


if __name__ == '__main__': main()
