#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""숫자/Shift 기호의 실제 저장 카메라10칸·언어 표·뷰포트 중심을 최초 동결한다(719)."""
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys
from decomp_oracle import Oracle as BaseOracle, STACK, SCRATCH
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 같은 호스트의 보호 PE/정밀 디컴파일만 사용한다.
SAVED, LANGUAGE, RENDERER = 0x5caeb0, 0x542384, SCRATCH + 0x1000  # 실제80BYTE/공개 언어와 격리된 Renderer64BYTE다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/saved-camera-key-x86.tsv'  # 최초 성공 이후 기대값을 재생성하지 않는다.
REPORT = ROOT / 'cpppj/saved-camera-key-native-20261102.json'  # 원문 관측 범위/정밀도 차이를 구별한다.
WORK = ROOT / 'extracted/cpp-saved-camera-719'  # 큰 원시 관측은 Git에서 제외한다.
INVALID, NO_LANGUAGE_CHANGE = 0xc61c4000, 0xfffffffe  # 원문 무효 Point 비트와 검사 입력의 변경 없음 표식이다.


def digest(path):
    """최초 원문과 동결 자료의 물리 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """입력용 단정도만 원시 DWORD로 바꾸며 결과를 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def points_image(points):
    """열 좌표를 순서대로80BYTE 입력으로 만든다."""
    return struct.pack('<20I', *(word for point in points for word in point)).hex()


def view_image(x=976, y=793, cell_x=16.0, cell_y=11.0):
    """사용되는 Renderer 필드만 지정하고 나머지도 구별 가능한 입력으로 채운다."""
    words = [0x12340000 + i for i in range(16)]  # 계산이 쓰지 않는 필드의 보존도 원문 이미지에 남긴다.
    words[7:10] = [x & 0xffffffff, y & 0xffffffff, bits(cell_x)]
    words[10] = bits(cell_y)
    return struct.pack('<16I', *words).hex()


def rectangle(values):
    """지도 LTRB signed DWORD 네 개의 입력 비트를 저장한다."""
    return struct.pack('<4I', *(value & 0xffffffff for value in values)).hex()


class PartialStop(Exception):
    """소리/뷰포트 읽기/중심 경계에서 의도한 중단이다."""


class Oracle(BaseOracle):
    """일반 키 부모·기호 표 조회·Point·실제 Renderer 중심만 격리 실행한다."""
    def __init__(self):
        """실제 허용 코드와 전역/스택의 쓰기 범위를 고정한다."""
        super().__init__('originals', [dict(name='unused', originals='004d62b0')], helpers=[])
        self.allowed = [(0x4d96b6, 0x4d9bbb), (0x4d53f0, 0x4d5430), (0x4d5020, 0x4d5067), (0x497e50, 0x497eb2), (0x439928, 0x43994c)]
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def dword(self, address, value=None):
        """원시 네 바이트만 읽고 입력/명시 장치 변경만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def guard(self, mu, access, address, size, value, data):
        """CPU의 효과는 실제 스택/저장80BYTE 밖으로 나가지 못한다."""
        assert STACK - 0x100 <= address and address + size <= STACK + 0x1000 or SAVED <= address and address + size <= SAVED + 80, (hex(address), size)

    def returned(self, value=0):
        """cdecl 장치의 반환 주소만 소비하며 인자 정리는 원문에 맡긴다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EIP, self.dword(sp))

    def on_instruction(self, mu, address, size, data):
        """strchr/정지/소리/즉시 카메라만 장치이며 나머지 계산은 실제 기계어다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e63e0:
            first, character = self.dword(sp + 4), self.dword(sp + 8) & 255
            text = self.read_string(first) + b'\0'
            at = text.find(bytes([character]))
            self.returned(0 if at < 0 else first + at)
            return
        if address == 0x460de0:
            self.trace.append('P:' + str(self.case['paused']))
            if self.case['pauseLanguage'] != NO_LANGUAGE_CHANGE:
                self.dword(LANGUAGE, self.case['pauseLanguage'])
            self.returned(self.case['paused'])
            return
        if address == 0x4a9cb0:
            assert self.read_string(self.dword(sp + 4)) == b'select.wav'
            assert all(self.dword(sp + offset) == 0 for offset in (8, 12, 16, 20, 24))
            self.trace.append('S:select.wav:0:0:0:0:0')
            if self.case['mutateSound']:
                mu.mem_write(RENDERER, bytes.fromhex(view_image(-87, -61, 8.0, 5.5)))
                mu.mem_write(0x5ca9cc, bytes.fromhex(rectangle([82, 19, 1000, 747])))
                self.dword(SAVED, bits(150.5))
                self.dword(SAVED + 4, bits(151.5))
            if self.case['stop'] == 'Sound':
                raise PartialStop()
            self.returned()
            return
        if address == 0x497e50:
            self.trace.append('V')
            if self.case['stop'] == 'Capture':
                raise PartialStop()
        if address == 0x433aa0:
            self.trace.append(f'C:{self.dword(sp + 4)}:{self.dword(sp + 8)}')
            if self.case['stop'] == 'Center':
                raise PartialStop()
            self.returned()
            return
        return super().on_instruction(mu, address, size, data)

    def run(self, case, control):
        """원문의 전체 저장 장부·현재 언어·호출 순서와 x87 상태를 관측한다."""
        self.case, self.trace = case, []
        self.mu.mem_write(SAVED, bytes.fromhex(case['points']))
        self.mu.mem_write(RENDERER, bytes.fromhex(case['view']))
        self.mu.mem_write(0x5ca9cc, bytes.fromhex(case['viewport']))
        self.dword(LANGUAGE, case['language'])
        self.dword(0x59a8bc, RENDERER)
        self.dword(0x545370, INVALID)
        self.dword(0x545374, INVALID)
        self.mu.mem_write(STACK - 0x100, bytes(0x1100))
        self.dword(STACK + 0x14, case['event'])
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        start, end = (0x439928, 0x43994c) if case['kind'] == 'Init' else (0x4d96b6, 0x4d9bbb)
        completed = True
        try:
            self.mu.emu_start(start, end, timeout=1000000, count=4000)
        except PartialStop:
            completed = False
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        if completed:
            assert self.mu.reg_read(UC_X86_REG_EIP) == end and self.mu.reg_read(UC_X86_REG_ESP) == STACK
        return dict(points=bytes(self.mu.mem_read(SAVED, 80)).hex(), language=self.dword(LANGUAGE), trace=';'.join(self.trace) or '-', completed=int(completed), fpsw=self.mu.reg_read(UC_X86_REG_FPSW), stackDelta=self.mu.reg_read(UC_X86_REG_ESP) - STACK)


def inputs():
    """언어/실제 문자/수정키와 좌표 경계·뷰포트·장치 중단 입력만 조합한다."""
    valid = [(bits(20 + i * 17.25), bits(30 + i * 9.5)) for i in range(10)]  # 호출할 칸을 서로 다른 위치로 구별한다.
    base = dict(kind='Key', event=ord('1'), language=0, points=points_image(valid), view=view_image(), viewport=rectangle([82, 0, 1024, 768]), paused=0, pauseLanguage=NO_LANGUAGE_CHANGE, mutateSound=0, stop='None')
    rows = []
    characters = sorted(set(b'1234567890!@#$%^&*()' + bytes([34, 163, 167, 47, 61])))
    for language, character, modifiers, paused in itertools.product((0, 1, 3, 0xffffffff), characters, range(8), (0, 1)):  # 세 실제 표와 기본 분기를 모든 수정 비트 조합으로 관측한다.
        flags = ((modifiers & 1) * 0x20000000) | (((modifiers >> 1) & 1) * 0x10000000) | (((modifiers >> 2) & 1) * 0x08000000)
        rows.append(dict(base, language=language, event=character | flags, paused=paused))
    edges = [(0, 0), (0x80000000, bits(1)), (1, 1), (0x437fffff, bits(1)), (bits(256), bits(1)), (bits(-1), bits(1)), (0x7f800000, bits(1)), (0xff800000, bits(1)), (0x7fc12345, bits(1)), (bits(1), 0x7fc12345), (bits(1), bits(256))]
    for index, point in itertools.product(range(10), edges):  # 저장 호출은 Tab과 달리0을 포함하는 실제 Point predicate를 사용한다.
        points = list(valid)
        points[index] = point
        rows.append(dict(base, event=ord('1234567890'[index]), points=points_image(points)))
    geometries = [(976, 793, 16.0, 11.0, [82, 0, 1024, 768]), (-87, -61, 8.0, 5.5, [82, 19, 1000, 747]), (-1, -1, 16.0, 11.0, [-9, -7, 2, 4]), (0x7fffffff, -0x80000000, 3.0, 7.0, [0x7fffffff, -0x80000000, 6, -7])]
    geometries.extend((0, 0, x, y, [0, 0, 1024, 768]) for x, y in ((0.0, -0.0), (float('inf'), 11.0), (float('nan'), 11.0), (1e-40, -1e-40), (-16.0, -11.0)))
    for x, y, cx, cy, viewport in geometries:  # signed 합/나눗셈 절삭과FILD→FDIV→FSTP를 실제 본문에 맡긴다.
        rows.append(dict(base, event=0x20000021, view=view_image(x, y, cx, cy), viewport=rectangle(viewport)))
    for event, mutate, stop in itertools.product((0x20000021, 0x20000040, ord('1')), (0, 1), ('None', 'Sound', 'Capture', 'Center')):  # 선택된 실제 경계에서만 의도한 중단이 발생한다.
        rows.append(dict(base, event=event, mutateSound=mutate, stop=stop))
    for before, after, symbol in ((0, 1, 34), (1, 0, 64), (0, 3, 167), (3, 0, 64)):  # 정지 판정 뒤 소비하는 현재 공개 언어를 관측한다.
        rows.append(dict(base, language=before, pauseLanguage=after, event=0x20000000 | symbol))
    for pattern in (valid, [(INVALID, INVALID)] * 10, edges[:10]):  # 최초80BYTE 초기화는 이전 장부의 모든 칸을 덮어쓴다.
        rows.append(dict(base, kind='Init', points=points_image(pattern)))
    return rows


def main():
    """제품 수정 전 최초 관측을 한 번 저장하고 이후에는 해시만 확인한다."""
    if '--verify' in sys.argv:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():  # 현재 제품에 맞춰 원문 기대값을 덮어쓰지 않는다.
            assert digest(ROOT / name) == expected, name
        print('숫자 키 저장 카메라 최초 원문 보존 확인')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    WORK.mkdir(parents=True, exist_ok=True)
    oracle, rows, observations = Oracle(), [], []
    for case in inputs():  # PC027f 기본 관측을 두 번 반복하고037f와 결과/부분 효과를 비교한다.
        outputs = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert outputs[0] == outputs[2], case
        assert outputs[0] == outputs[1], case
        result = outputs[0]
        rows.append([case[name] for name in ('kind', 'event', 'language', 'points', 'view', 'viewport', 'paused', 'pauseLanguage', 'mutateSound', 'stop')] + [result[name] for name in ('points', 'language', 'trace', 'completed', 'fpsw', 'stackDelta')])
        observations.append(dict(input=case, observations=outputs))
    FIXTURE.write_text('# kind event language points view viewport paused pauseLanguage mutateSound stop outPoints outLanguage trace completed fpsw stackDelta\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows), 'utf-8', newline='\n')
    (WORK / 'native-observations.json').write_text(json.dumps(observations, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    paths = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/raw.c', 'tools/decomp_saved_camera_key.py', 'tools/decomp_oracle.py', FIXTURE.relative_to(ROOT).as_posix()]
    record = dict(unit=719, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(rows), observations=len(rows) * 3, normalReturns=sum(row[13] for row in rows), partialStops=sum(not row[13] for row in rows), precisionOrRepeatDifferences=0, gameExecutions=0, multiplayerTests=0, ranges=['004d96b6..004d9bba', '004d53f0..004d542f', '004d5020..004d5066', '00497e50..00497eb1', '00439928..0043994b'], boundaries=['strchr/정지/선택음/즉시 중심만 명시 경계이며 언어 표·Point·현재 Renderer 뷰포트 중심은 실제 x86이다.', '해제/Console/Guide/Gump 등 상위 입력 관문·전체 GUI/OS/통신은 실행하지 않는다.', '음향/캡처/중심 명시 중단의 선행 부분 효과와x87조건/NaN을 정상 반환과 구별한다.'], sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps({key: value for key, value in record.items() if key not in ('sha256', 'boundaries')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
