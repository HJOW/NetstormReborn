#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소리 프로세스(SfxProcess)와 소리 이름 표 조회를 세 실제 PE에서 정상 반환까지 실행한다.

실제 명령으로 실행하는 것: 소리 프로세스 생성자·실행 몸체(vtable +0x18)·form 삭제 통지(vtable +0xc),
부모 타입 조회, 패치의 SID 범위 검사, 소리 이름 표의 찾기/추가와 이름 복사, ASCII 대소문자 무시 비교.
명시 기록 대체: BaseProcess 부착, 벽시계 조회, 세 재생 함수, 정지/재생 여부 조회, 프로세스 Kill, assert 보고,
패치 CRT의 스레드 로캘 선택(C 로캘의 ASCII 비교 본문으로 넘긴다), 소리 장치 초기화의 목록 확보/0 채움/대입.
게임/OS/창/소리 장치는 실행하지 않는다. --verify는 저장된 SHA/입력/진입/반환 근거를 감사한다.

python -X utf8 tools/decomp_soundprocess_oracle.py            # 관찰 fixture·근거 기록 생성
python -X utf8 tools/decomp_soundprocess_oracle.py --smoke    # 일부 입력만 실행(저장하지 않음)
python -X utf8 tools/decomp_soundprocess_oracle.py --verify   # 저장된 기록 감사
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, POOL, STOP, STACK, LISTS, TARGET, TYPES, OBJECT_TYPE,
    CAPACITY, digest, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 격리 메모리의 프로세스 객체, 소리 이름 표(0x8000바이트 + 경계 밖 이름용 여유), 외부 이름 문자열 주소다.
# SOUNDS는 C++ kSoundListBase와 같은 값이어야 표 전체 바이트(자기 참조 포함)를 그대로 비교할 수 있다.
PROCESS, SOUNDS, NAMES = LISTS + 0x8000, 0x15000000, 0x15100000
# 원본 표 크기, 프로세스 객체 크기, 부착된 form의 합성 SID다.
LIST_BYTES, OBJECT_BYTES, FORM = 0x8000, 0x28, 60
# 재생 함수가 역참조하지 않는 합성 소리 항목 값(기본·대체·재생 중·재생 함수 반환)이다.
SOUND, ALTERNATE, PLAYING, REPLY = 0x15000040, 0x15000080, 0x150000c0, 0x15000100
# 두 x87 정밀도에서 같은 관찰이어야 한다.
CONTROLS = (0x027f, 0x037f)
# 판본별 실제 몸체·대체 경계·전역·타입 레코드의 마지막 소리 프레임 위치다.
BODY = {
 'originals': dict(ctor=0x4ab240, destroy=0x4ab030, run=0x4ab070, robust=0x41be80, typeof=0x4ac550, lookup=0x4a8ee0,
    body=0x4a8d50, ascii=0x4ef510, attach=0x41bd20, clock=0x460d70, loop=0x4a9860, once=0x4a9c30, play=0x4a9550,
    stop=0x4a97e0, playing=0x4a9810, kill=0x41bf10, stricmp=0x4e65ed, table=0x511ff4, ptype=0x54112c, frame=0x55b4a8,
    debug=0x5e4794, capacity=0x5c847c, list=0x5c7b44, free=0x5c7b48, fallback=0x5c7b4c, stamp=0x1f0),
 'originalCD': dict(ctor=0x453800, destroy=0x4538b0, run=0x453900, typeof=0x4abf50, lookup=0x437d20,
    body=0x437b29, ascii=0x4f2980, attach=0x48f590, clock=0x4011b0, loop=0x438800, once=0x438ae0, play=0x4383f0,
    stop=0x4386f0, playing=0x4387a0, kill=0x48f7b0, table=0x502020, ptype=0x51ca1c, frame=0x50f248,
    list=0x51a74c, free=0x51a750, fallback=0x51a754, stamp=0x1d0),
}
BODY['original1037'] = dict(BODY['originalCD'])
# 결과 fixture/근거는 앞 단계 파일과 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/soundprocess-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-soundprocess-evidence.json'
# 소리 장치 초기화가 표에 처음 넣는 이름이다(004aa600 / CD 00437820).
FALLBACK_NAME = 'nonexistant.wav'


def bits(value):
    """단정도 입력의 비트."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def wide(value):
    """배정도 입력의 비트."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def named(text):
    """외부 이름 조회 연산. 바이트는 16진수로 적어 구분 문자와 섞이지 않게 한다."""
    return 'n:' + text.encode('latin-1').hex()


def run_inputs():
    """실행 몸체 입력. 재생 여부·순서·기록값의 기대는 만들지 않는다."""
    rows = []
    nan = 0x7ff8000000012345
    # (현재 벽시계, 프로세스 시작 시각): 지남·같음·아직·현재 NaN·시작 NaN.
    clocks = ((wide(10.0), wide(0.0)), (wide(10.0), wide(10.0)), (wide(10.0), wide(10.5)), (nan, wide(0.0)), (wide(10.0), nan))
    # 좌표는 재생 함수에 비트 그대로 전달된다. 부호 있는 0과 NaN 비트도 넣는다.
    places = ((bits(20.75), bits(21.9)), (0x80000000, 0x7fc12345), (bits(254.9), bits(-1.5)))
    # (프레임 카운터, 타입의 마지막 소리 프레임): 이미 재생·아직·초기값 0끼리 같음.
    stamps = ((7, 7), (7, 6), (0, 0))
    # flags 낮은 네 비트 전체와 현재 소리·부모 상태 단어·타입 프레임·재생 반환·시각·debug를 교차한다.
    for index, (flags, current, word, stamp, reply, clock, debug) in enumerate(itertools.product(
            range(16), (0, PLAYING), (0, 1, 0x8000), stamps, (0, REPLY), clocks, (0, 1))):
        x, y = places[index % 3]
        rows.append(('Run', flags, current, word, stamp[0], stamp[1], reply, f'{clock[0]:016x}', f'{clock[1]:016x}', debug, x, y, SOUND, ALTERNATE))
    # 낮은 네 비트 밖의 flags는 분기에 쓰이지 않는다.
    for flags, current, stamp, reply, debug in itertools.product((0x10, 0xfffffff0, 0xffffffff), (0, PLAYING), stamps[:2], (0, REPLY), (0, 1)):
        rows.append(('Run', flags, current, 1, stamp[0], stamp[1], reply, f'{clocks[0][0]:016x}', f'{clocks[0][1]:016x}', debug, *places[0], SOUND, ALTERNATE))
    return rows


def destroy_inputs():
    """form 삭제 통지 입력: 프로세스 flags·현재 소리·재생 여부 조회 반환·삭제 flags."""
    return [('Destroy', flags, current, reply, argument) for flags, current, reply, argument in itertools.product(
        (0, 1, 4, 5, 8, 0xc, 0xfffffffb, 0xffffffff), (0, PLAYING), (0, 1, 0x100), (0, 0x40, 0xffffffff))]


def ctor_inputs():
    """생성자 입력: 부모 SID·기본/대체 소리·flags·현재 프로세스 타입 전역."""
    return [('Ctor', parent, sound, alternate, flags, ptype) for parent, sound, alternate, flags, ptype in itertools.product(
        (5, 50, 127), (0, SOUND), (0, ALTERNATE), (0, 1, 2, 4, 8, 0xc, 0xffffffff), (47, 48))]


def lookup_inputs():
    """이름 표 연산 묶음. i=초기화, n=외부 이름, p=표 기준 포인터 오프셋, z=null. 결과는 원본이 정한다."""
    long_names = [named(str(i)[::-1].ljust(996, 'x') + '.wav') for i in range(31)]
    scripts = [
        # 보호막이 쓰는 이름과 대소문자, 같은 이름의 재조회, null, 초기 이름의 재조회.
        ['i', named('priestForceField.wav'), named('PRIESTFORCEFIELD.wav'), named('priestforcefield.wav'), named('priestFall.wav'),
         named('ourPriestImmobile.wav'), named('priestForceField.wav'), 'z', named(FALLBACK_NAME), named('NONEXISTANT.wav')],
        # 길이/접두가 다른 이름과 ASCII 대문자 범위 바로 밖의 글자, 상위 비트 글자는 서로 다른 이름이다.
        ['i', named('a.wav'), named('a.wa'), named('a.wavv'), named('A.wav'), named('@.wav'), named('`.wav'), named('[.wav'),
         named('{.wav'), named('Z.wav'), named('z.wav'), named('\xc1.wav'), named('\xe1.wav'), named('a.wav'), named('{.wav')],
        # 표 안을 가리키는 포인터는 표를 읽지 않고 고정 오프셋을 뺀다. 표 끝 바로 뒤는 외부 이름으로 읽는다.
        ['i', named('a.wav'), 'p:0', 'p:28', 'p:31', 'p:44', 'p:32767', 'p:32768', named('edge.wav'), 'z'],
        # 초기화 없이 빈 표에 처음 넣는 이름.
        [named('first.wav'), named('FIRST.wav'), named('second.wav'), named('first.wav')],
        # 마지막 글자가 v가 아닌 이름을 넣은 뒤의 순회(CD의 항목 검사 보고).
        ['i', named('upper.WAV'), named('noext'), named('upper.wav'), named('tail.wav'), named('NOEXT'), named('tail.wav')],
        # 마지막 추가가 정확히 한도(0x7fb0)에서 끝난 뒤의 새 이름·기존 이름·null.
        ['i', *long_names, named('y' * 712 + '.wav'), named('new.wav'), named(FALLBACK_NAME), long_names[0], long_names[30],
         named('Y' * 712 + '.wav'), named('other.wav'), 'z'],
        # 한도 바로 앞(0x7faf)에서 시작하는 마지막 추가와 그 뒤의 가득 찬 표.
        ['i', *long_names, named('y' * 711 + '.wav'), named('last.wav'), named('new.wav'), named('LAST.wav'), long_names[15], named('other.wav')],
    ]
    return [('Lookup', '|'.join(script)) for script in scripts]


def inputs():
    """고정 순서의 전체 입력."""
    return run_inputs() + destroy_inputs() + ctor_inputs() + lookup_inputs()


class SoundOracle(OwnerOracle):
    """실제 소리 프로세스/이름 표 몸체만 실행하고 장치·시계·부착 경계는 기록 대체한다."""
    def __init__(self, edition):
        """새 내보내기의 불연속 몸체만 실행을 허용하고 실제 가상 표의 대응을 확인한다."""
        super().__init__(edition)
        self.b = BODY[edition]
        # SEH 프레임(fs:[0])과 소리 표·이름 문자열용 메모리다.
        for address, size in ((0, 0x1000), (SOUNDS, 0x10000), (NAMES, 0x1000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/soundprocess/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 구간 사이 임의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        b = self.b
        self.vtable = {f'{offset:02x}': f'{self.u32(b["table"] + offset):08x}' for offset in (0x0c, 0x18)}
        if self.u32(b['table'] + 0x0c) != b['destroy'] or self.u32(b['table'] + 0x18) != b['run']:
            raise RuntimeError('실제 소리 프로세스 가상 표 오류')
        if self.u32(b['ptype']) != 47:
            raise RuntimeError('소리 프로세스 타입 전역 초기값 오류')
        # 대체 경계: 주소 -> (이름, 처리기).
        self.stubs = {b[name]: (name, getattr(self, 'stub_' + name)) for name in ('attach', 'clock', 'loop', 'once', 'play', 'stop', 'playing', 'kill')}
        self.returns = collections.Counter()
        self.substitutions = collections.Counter()
        # 실제 조회 진입 횟수(연산 수 × 정밀도 수)다.
        self.lookups = 0

    def u32(self, address):
        """비정렬 DWORD 읽기."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def leave(self, mu, value, purge):
        """대체한 함수에서 호출자로 돌아간다. 호출자가 보존을 기대할 수 없는 ECX/EDX는 흐린다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        mu.reg_write(UC_X86_REG_EAX, value)
        mu.reg_write(UC_X86_REG_ECX, 0xdeadbeef)
        mu.reg_write(UC_X86_REG_EDX, 0xfeedface)
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, self.u32(esp))

    def arguments(self, mu, count):
        """스택의 DWORD 인자."""
        return struct.unpack(f'<{count}I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP) + 4, 4 * count))

    def stub_attach(self, mu):
        """BaseProcess 부착(thiscall, 인자 4개): 타입·부모·두 flags를 기록한다."""
        if mu.reg_read(UC_X86_REG_ECX) != PROCESS: raise RuntimeError('부착 this 오류')
        self.events.append('B:' + ':'.join(map(str, self.arguments(mu, 4))))
        self.leave(mu, PROCESS, 16)

    def stub_clock(self, mu):
        """벽시계 조회(cdecl, 출력 버퍼 하나): 입력 시각을 쓰고 버퍼 주소를 돌려준다."""
        target, = self.arguments(mu, 1)
        mu.mem_write(target, bytes.fromhex(self.case['now'])[::-1])
        self.events.append('T')
        self.leave(mu, target, 0)

    def stub_loop(self, mu):
        """위치 반복 재생(cdecl x,y,현재,소리)."""
        self.events.append('L:' + ':'.join(map(str, self.arguments(mu, 4))))
        self.leave(mu, self.case['reply'], 0)

    def stub_once(self, mu):
        """위치 한 번 재생(cdecl x,y,소리,flags&8,0). 호출자는 반환값을 저장하지 않는다."""
        self.events.append('O:' + ':'.join(map(str, self.arguments(mu, 5))))
        self.leave(mu, self.case['reply'], 0)

    def stub_play(self, mu):
        """전역 재생(cdecl 소리,한 번 재생이면 0·아니면 1,0,0,flags&8,0). 인자의 뜻은 재생 몸체를 대조할 때 확정한다."""
        self.events.append('G:' + ':'.join(map(str, self.arguments(mu, 6))))
        self.leave(mu, self.case['reply'], 0)

    def stub_stop(self, mu):
        """정지(cdecl 소리)."""
        self.events.append('S:%d' % self.arguments(mu, 1))
        self.leave(mu, 0, 0)

    def stub_playing(self, mu):
        """재생 여부 조회(cdecl 소리)."""
        self.events.append('P:%d' % self.arguments(mu, 1))
        self.leave(mu, self.case['reply'], 0)

    def stub_kill(self, mu):
        """프로세스 Kill(thiscall flags)."""
        if mu.reg_read(UC_X86_REG_ECX) != PROCESS: raise RuntimeError('Kill this 오류')
        self.events.append('K:%d' % self.arguments(mu, 1))
        self.leave(mu, 0, 4)

    def on_instruction(self, mu, address, size, data):
        """대체 경계와 assert 보고는 기록하고 나머지는 허용 범위 안의 실제 명령만 실행한다."""
        stub = self.stubs.get(address)
        if stub:
            self.substitutions[stub[0]] += 1
            stub[1](mu)
        elif address == self.spec['assert_report']:
            # 원본 assert 보고는 (조건식, 파일, 줄)의 cdecl이며 보고 뒤 실행을 계속한다.
            self.substitutions['assert'] += 1
            self.events.append('!%d' % self.arguments(mu, 3)[2])
            self.leave(mu, 0, 0)
        elif address == self.b.get('stricmp'):
            # 패치 CRT는 스레드 로캘을 OS에서 얻는다. C 로캘의 ASCII 비교 본문으로 넘긴다(인자/반환 주소는 그대로).
            self.substitutions['locale'] += 1
            mu.reg_write(UC_X86_REG_EIP, self.b['ascii'])
        else:
            super().on_instruction(mu, address, size, data)

    def call(self, entry, ecx, stack, purge, control):
        """보존 레지스터·스택·SEH·x87을 확인하며 한 함수를 정상 반환까지 실행하고 EAX를 돌려준다."""
        saved = ((UC_X86_REG_EBX, 0x11223344), (UC_X86_REG_ESI, 0x22334455), (UC_X86_REG_EDI, 0x33445566), (UC_X86_REG_EBP, 0x44556677))
        # 비영 callee 보존 레지스터를 확인한다.
        for register, value in saved: self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0x0badf00d)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ECX, ecx)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(0, struct.pack('<I', 0x12345678))
        self.mu.mem_write(STACK, struct.pack(f'<{len(stack) + 1}I', STOP, *stack))
        self.mu.emu_start(entry, STOP, count=4000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'정상 반환/스택 오류: {self.edition} {entry:08x}')
        if any(self.mu.reg_read(register) != value for register, value in saved) or self.u32(0) != 0x12345678:
            raise RuntimeError('보존 레지스터/SEH 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어/TOP 오류')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def put_process(self, form, parent, current, sound, alternate, flags, time):
        """원본 객체 배치(+4 form, +8 부모, +0xc 현재 소리, +0x10/+0x14 소리, +0x18 flags, +0x20 시작 시각)를 쓴다."""
        raw = bytearray(b'\xcc' * OBJECT_BYTES)
        struct.pack_into('<7I', raw, 0, self.b['table'], form, parent, current, sound, alternate, flags)
        struct.pack_into('<Q', raw, 0x20, time)
        self.mu.mem_write(PROCESS, bytes(raw))

    def run_process(self, row, control):
        """실행 몸체: 사건 순서/인자, 프로세스 객체 전체, 타입의 마지막 소리 프레임을 관찰한다."""
        keys = ('flags', 'current', 'word', 'frame', 'stamp', 'reply', 'now', 'time', 'debug', 'x', 'y', 'sound', 'alternate')
        c = self.case = dict(zip(keys, row[1:]))
        b, stamp = self.b, TYPES + OBJECT_TYPE * self.type_stride + self.b['stamp']
        self.put_process(FORM, TARGET, c['current'], c['sound'], c['alternate'], c['flags'], int(c['time'], 16))
        # 부모는 타입·상태 단어(+0xc)·좌표만 읽힌다. form은 패치 debug의 범위 검사 대상이다.
        parent = bytearray(self.stride)
        parent[10] = OBJECT_TYPE
        struct.pack_into('<HII', parent, 12, c['word'], c['x'], c['y'])
        self.mu.mem_write(self.slot(TARGET), bytes(parent))
        self.mu.mem_write(self.slot(FORM), bytes(self.stride))
        self.mu.mem_write(TYPES + OBJECT_TYPE * self.type_stride, bytes(self.type_stride))
        self.mu.mem_write(stamp, struct.pack('<I', c['stamp']))
        self.mu.mem_write(b['frame'], struct.pack('<I', c['frame']))
        if 'debug' in b:
            self.mu.mem_write(b['debug'], struct.pack('<I', c['debug']))
            self.mu.mem_write(b['capacity'], struct.pack('<I', CAPACITY))
        self.events = []
        self.write_ranges = [(0, 4), (PROCESS, PROCESS + OBJECT_BYTES), (stamp, stamp + 4)]
        self.call(b['run'], PROCESS, (), 0, control)
        if bytes(self.mu.mem_read(self.slot(TARGET), self.stride)) != bytes(parent): raise RuntimeError('부모 슬롯 변경')
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(PROCESS, OBJECT_BYTES)).hex(), self.u32(stamp)]

    def run_destroy(self, row, control):
        """form 삭제 통지: 정지/재생 여부 조회/assert 사건과 객체 전체를 관찰한다."""
        c = self.case = dict(zip(('flags', 'current', 'reply', 'argument'), row[1:]))
        self.put_process(FORM, TARGET, c['current'], SOUND, ALTERNATE, c['flags'], 0)
        self.events = []
        self.write_ranges = [(0, 4), (PROCESS, PROCESS + OBJECT_BYTES)]
        self.call(self.b['destroy'], PROCESS, (c['argument'],), 4, control)
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(PROCESS, OBJECT_BYTES)).hex()]

    def run_ctor(self, row, control):
        """생성자: 쓰지 않는 바이트가 남는지 볼 수 있게 객체를 0xcc로 채우고 실행한다."""
        c = self.case = dict(zip(('parent', 'sound', 'alternate', 'flags', 'ptype'), row[1:]))
        self.mu.mem_write(PROCESS, b'\xcc' * OBJECT_BYTES)
        self.mu.mem_write(self.b['ptype'], struct.pack('<I', c['ptype']))
        self.events = []
        self.write_ranges = [(0, 4), (PROCESS, PROCESS + OBJECT_BYTES)]
        result = self.call(self.b['ctor'], PROCESS, (c['parent'], c['sound'], c['alternate'], c['flags']), 16, control)
        self.mu.mem_write(self.b['ptype'], struct.pack('<I', 47))
        if result != PROCESS: raise RuntimeError('생성자 반환 오류')
        raw = bytes(self.mu.mem_read(PROCESS, OBJECT_BYTES))
        if struct.unpack_from('<I', raw, 0)[0] != self.b['table']: raise RuntimeError('생성자 가상 표 오류')
        # 가상 표 주소는 판본마다 달라 확인만 하고 나머지 바이트를 저장한다.
        return [';'.join(self.events) or '-', raw[4:].hex()]

    def run_lookup(self, row, control):
        """이름 표 연산 묶음: 연산별 반환(표 기준 오프셋)·assert, 마지막 빈 위치, 표 전체 Adler-32를 관찰한다."""
        b = self.b
        self.case = {}
        self.mu.mem_write(SOUNDS, bytes(0x10000))
        # 표 끝 바로 뒤의 문자열은 표 밖 포인터 입력용이다.
        self.mu.mem_write(SOUNDS + LIST_BYTES, b'edge.wav\0')
        self.mu.mem_write(b['list'], struct.pack('<III', SOUNDS, SOUNDS, 0))
        self.write_ranges = [(0, 4), (SOUNDS, SOUNDS + LIST_BYTES), (b['free'], b['free'] + 4)]
        tokens = []
        # 연산마다 실제 조회 진입을 한 번 실행한다.
        for operation in row[1].split('|'):
            self.events = []
            if operation == 'z': pointer = 0
            elif operation.startswith('p:'): pointer = SOUNDS + int(operation[2:])
            else:
                text = FALLBACK_NAME.encode('latin-1') if operation == 'i' else bytes.fromhex(operation[2:])
                self.mu.mem_write(NAMES, text + b'\0')
                pointer = NAMES
            result = self.call(b['lookup'], 0, (pointer,), 0, control)
            self.lookups += 1
            # 초기화는 첫 조회 결과를 대체 소리 전역에 넣는다(대입은 Python 대체).
            if operation == 'i': self.mu.mem_write(b['fallback'], struct.pack('<I', result))
            tokens.extend(self.events)
            tokens.append('null' if result == 0 else str(result - SOUNDS))
        if self.u32(b['list']) != SOUNDS: raise RuntimeError('표 전역 변경')
        return [','.join(tokens), self.u32(b['free']) - SOUNDS, zlib.adler32(bytes(self.mu.mem_read(SOUNDS, LIST_BYTES)))]

    def run(self, row, control):
        """입력 종류별 실행."""
        result = getattr(self, 'run_' + {'Run': 'process', 'Destroy': 'destroy', 'Ctor': 'ctor', 'Lookup': 'lookup'}[row[0]])(row, control)
        self.returns[row[0]] += 1
        return result


def generate(smoke=False):
    """세 판본의 두 정밀도 관찰이 같은 입력만 fixture로 저장한다."""
    cases = inputs()[::37] + lookup_inputs()[:4] if smoke else inputs()
    rows, editions = [], {}
    paths = {Path(__file__), FIXTURE, ROOT / 'tools/ghidra/soundprocess-functions.json', ROOT / 'tools/decomp_owner_oracle.py'}
    # 판본별 PE와 실제 몸체는 독립으로 읽는다.
    for edition in SPECS:
        oracle = SoundOracle(edition)
        # Python에서 재생 여부/반환/표 내용을 계산하지 않고 원본의 관찰끼리 비교한다.
        for case in cases:
            first, second = (oracle.run(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'두 x87 정밀도 관찰 불일치: {edition} {case[:2]}')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=dict(oracle.returns), lookups=oracle.lookups, native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions), vtable=oracle.vtable, assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 소리 프로세스/이름 표 {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 소리 프로세스 생성자/실행/form 삭제 통지와 이름 표 조회. 부착/시계/재생/정지/Kill/assert 보고는 명시 경계.\n'
        '# edition Run flags current word frame stamp reply now time debug x y sound alternate | events object stampAfter\n'
        '# edition Destroy flags current reply argument | events object\n'
        '# edition Ctor parent sound alternate flags ptype | events object[4:]\n'
        '# edition Lookup script | results firstFree adler\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, host='HJOW-Athlon', decompile_date='2026-10-10', total=len(rows), controls=list(CONTROLS),
        editions=editions, os_calls=0, sound_list_base=f'{SOUNDS:08x}',
        stubbed=['BaseProcess 부착', '벽시계 조회', '위치 반복/위치 한 번/전역 재생', '정지/재생 여부 조회', '프로세스 Kill', 'assert 보고(기록 뒤 계속)',
                 '패치 CRT 스레드 로캘 선택(C 로캘 ASCII 비교 본문은 실제 실행)', '소리 장치 초기화의 표 확보/0 채움/대체 소리 대입'],
        limitations=['합성 프로세스 객체/부모 raw/타입 레코드', '소리 장치(DirectSound)·실제 재생/정지·파일 읽기 제외', '빈 이름·표 끝을 넘는 이름·손상된 표 제외'],
        files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(paths)}), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA/정확한 입력/정상 반환/실제 몸체 진입과 OS 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    expected = inputs()
    if report['schema'] != 1 or report['controls'] != list(CONTROLS) or report['os_calls'] or set(report['editions']) != set(SPECS) \
            or report['sound_list_base'] != f'{SOUNDS:08x}':
        raise RuntimeError('감사 스키마/정밀도/판본/OS 오류')
    # PE/실행기/모든 내보내기/fixture의 바이트를 확인한다.
    for name, value in report['files'].items():
        if digest(ROOT / name) != value: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != len(expected) * 3 or report['total'] != len(rows): raise RuntimeError('행 개수 오류')
    counts = collections.Counter(case[0] for case in expected)
    operations = sum(len(case[1].split('|')) for case in expected if case[0] == 'Lookup')
    debug_runs = sum(case[0] == 'Run' and case[9] == 1 for case in expected)
    # 판본마다 누락/중복 없이 전체 입력을 같은 순서로 실행해야 한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]
        body = BODY[edition]
        if [row[1:1 + len(case)] for row, case in zip(selected, expected)] != [list(map(str, case)) for case in expected] or item['cases'] != len(expected):
            raise RuntimeError(f'입력 오류: {edition}')
        if item['assertions'] or item['lookups'] != operations * 2: raise RuntimeError(f'assert/조회 수 오류: {edition}')
        # 두 정밀도 각각에서 모든 입력이 실제 몸체에 진입하고 정상 반환해야 한다.
        for kind, key in (('Run', 'run'), ('Destroy', 'destroy'), ('Ctor', 'ctor')):
            if item['returns'][kind] != counts[kind] * 2 or item['native_calls'].get(f'{body[key]:08x}') != counts[kind] * 2:
                raise RuntimeError(f'진입/반환 오류: {edition}/{kind}')
        if item['returns']['Lookup'] != counts['Lookup'] * 2 or item['native_calls'].get(f'{body["lookup"]:08x}') != operations * 2 \
                or item['native_calls'].get(f'{body["body"]:08x}') != operations * 2 or not item['native_calls'].get(f'{body["ascii"]:08x}'):
            raise RuntimeError(f'이름 표 진입 오류: {edition}')
        if item['substitutions'].get('attach') != counts['Ctor'] * 2 or not item['native_calls'].get(f'{body["typeof"]:08x}'):
            raise RuntimeError(f'부착/타입 조회 오류: {edition}')
        if 'robust' in body and item['native_calls'].get(f'{body["robust"]:08x}') != debug_runs * 2:
            raise RuntimeError(f'패치 debug 범위 검사 진입 오류: {edition}')
    print(f'soundprocess 검증 통과: {len(rows)}개')


def main():
    """새 원본 관찰 생성/저장 감사/소수 입력 점검을 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
