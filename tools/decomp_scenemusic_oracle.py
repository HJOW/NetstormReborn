#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""장면별 배경음악 감독(결과 곡 잠금·곡 요청·다음 곡·프레임 확인·시작·날씨 효과)을 세 실제 PE에서 정상 반환까지 실행한다.

실제 명령으로 실행하는 것: 위 여섯 함수, 곡 길이 조회, 시계 가산(패치 `00424930`), 난수, ASCII 대소문자 무시 비교.
명시 기록 대체: 실시간 시계, 곡 선택(현재 이름 갱신과 곡 길이 설치를 입력대로 흉내), 희생/대기실 판정, 화면 갱신, 설정 조회,
팔레트 경로 조립/적용, 번개 효과, 날씨 효과음. 패치 CRT의 스레드 로캘 선택은 C 로캘의 ASCII 비교 본문으로 넘긴다.
게임/OS/창/소리 장치는 실행하지 않는다. --verify는 저장된 SHA/입력/진입/반환 근거를 감사한다.

python -X utf8 tools/decomp_scenemusic_oracle.py            # 관찰 fixture·근거 기록 생성
python -X utf8 tools/decomp_scenemusic_oracle.py --smoke    # 일부 입력만 실행(저장하지 않음)
python -X utf8 tools/decomp_scenemusic_oracle.py --verify   # 저장된 기록 감사
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, STOP, STACK, digest, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 두 x87 정밀도에서 같은 관찰이어야 한다.
CONTROLS = (0x027f, 0x037f)
# 경로 조립이 돌려주는 가짜 문자열 객체와, 두 디렉터리 전역에 넣는 가짜 포인터다(원본은 문자열 객체/포인터를 해석하지 않는 경계다).
PATH_OBJECT, DIR_A, DIR_B = 0x16300000, 0x16200000, 0x16200100
# 원본 이름 저장 공간(256바이트)과 곡 요청 인자 문자열을 올릴 작업 메모리다.
NAMES, WORK = 0x15100000, 0x15200000
# 기본 전역 값(원본 초기값)과 원소 곡별 날씨 색의 시험 값이다. 색 표는 원본에서 실행 중에 채워지므로 서로 다른 값을 준다.
DEFAULT_TINTS = (0x1111, 0x2222, 0x3333, 0x4444)
# 판본별 실제 몸체·대체 경계·전역이다.
BODY = {
 'originals': dict(lock=0x469d30, request=0x469db0, next=0x469f00, frame=0x469f60, start=0x469fc0, weather=0x469c80,
    duration=0x4aa5b0, assign=0x424930, rng_fn=0x4558f0, stricmp=0x4e65ed, ascii=0x4ef510,
    stubs=dict(wall=0x460d70, select=0x435200, sacrificing=0x449220, waiting=0x42cbd0, refresh=0x43dad0, option=0x441470,
        join=0x459c60, apply=0x4a4850, thunder=0x470fa0, sound=0x4a9cb0),
    g=dict(index=0x5409c4, fanfare=0x565e58, defeat=0x565e60, song=0x565e68, result=0x5c85ac, battle=0x594fbc, players=0x595344,
        local=0x540c70, name=0x54dd60, tint=0x540cdc, tints=0x5b5da8, dirty=0x59a8b0, duration=0x5c7b78, rng=0x532710,
        dir_a=0x540cec, dir_b=0x540cf8)),
 'originalCD': dict(lock=0x436a10, request=0x436a90, next=0x436c40, frame=0x436ca0, start=0x436d00, weather=0x436960,
    duration=0x4398f0, assign=None, rng_fn=0x48cc10, stricmp=None, ascii=0x4f2980,
    stubs=dict(wall=0x4011b0, select=0x484aa0, sacrificing=0x484400, waiting=0x48ebe0, refresh=0x4ee9d0, option=0x4a98d0,
        join=0x49c1a0, apply=0x424760, thunder=0x4b10d0, sound=0x438b50),
    g=dict(index=0x51a19c, fanfare=0x5650a0, defeat=0x565098, song=0x5650a8, result=0x518910, battle=0x540a20, players=0x50f828,
        local=0x50f6c8, name=0x52eb38, tint=0x5203a0, tints=0x549b40, dirty=0x52039c, duration=0x5650d0, rng=0x5308ec,
        dir_a=0x510940, dir_b=0x51094c)),
}
BODY['original1037'] = dict(BODY['originalCD'])
FIXTURE = ROOT / 'cpppj/tests/fixtures/scenemusic-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-scenemusic-evidence.json'
# 호출 연산과 그 연산이 실제로 진입하는 원본 몸체의 키다.
CALLS = {'lock': 'lock', 'req': 'request', 'next': 'next', 'frame': 'frame', 'start': 'start', 'weather': 'weather'}


def bits(value):
    """배정도 입력의 비트(16진수 16자리)."""
    return f'{struct.unpack("<Q", struct.pack("<d", value))[0]:016x}'


def named(text):
    """이름을 16진수로 적어 구분 문자와 섞이지 않게 한다. 빈 이름은 '-'다."""
    return text.encode('latin-1').hex() or '-'


NAN, INF = '7ff8000000012345', bits(float('inf'))
SONGS = ('wind22.mus', 'rain22.mus', 'thu22.mus', 'sun22.mus')


def ops(*items):
    """연산 목록을 한 문자열로 잇는다."""
    return '|'.join(items)


def lock_scripts():
    """결과 곡 잠금: 현재 곡 이름과 두 끝 시각(지남·같음·앞·비교 불가)을 교차한다. 시계는 1000.0이다."""
    scripts = []
    ends = (bits(999.0), bits(1000.0), bits(1001.0), NAN)
    # 대소문자가 섞인 이름과 빈 이름, 결과 곡이 아닌 이름을 넣는다.
    for current, fanfare, defeat in itertools.product(('', 'fanfare.mus', 'FANFARE.MUS', 'defeat.mus', 'Defeat.Mus', 'wind22.mus'), ends, ends):
        scripts.append(ops(f'cur:{named(current)}', f'f:fanfare:{fanfare}', f'f:defeat:{defeat}', f'clk:{bits(1000.0)}', 'lock'))
    return scripts


def request_scripts():
    """곡 요청: 현재 곡·요청 곡·결과 상태·결과 곡 잠금·곡 길이를 교차한다."""
    scripts = []
    currents = ('', 'ser22.mus', 'WIND22.MUS', 'fanfare.mus', 'defeat.mus', 'anticipation.mus', 'sacrifice.mus', 'rain22.mus')
    requests = ('fanfare.mus', 'FANFARE.MUS', 'defeat.mus', 'anticipation.mus', 'ANTICIPATION.MUS', 'wind22.mus', 'sacrifice.mus', 'ser22.mus', 'rain22.mus')
    # 끝 시각 쌍: 둘 다 지남, 승리 곡만 앞, 패배 곡만 앞.
    locks = ((bits(999.0), bits(999.0)), (bits(1001.0), bits(999.0)), (bits(999.0), bits(1001.0)))
    # 곡 길이는 30초 경계 양쪽 두 값만 쓴다. 길이 전수는 아래 별도 묶음이 맡는다.
    for current, request, result, (fanfare, defeat), duration in itertools.product(currents, requests, (0, 1, 2), locks, (29.5, 214.1)):
        scripts.append(ops(f'cur:{named(current)}', f'i:result:{result}', f'f:fanfare:{fanfare}', f'f:defeat:{defeat}', f'f:song:{bits(5.0)}',
            f'clk:{bits(1000.0)};{bits(1000.5)};{bits(1001.25)}', f'durs:{bits(duration)}', f'req:{named(request)}'))
    durations = (0.0, 29.5, 30.0, 30.0000001, 214.1, float('nan'), -3.0, float('inf'), 1e-300)
    clocks = (f'{bits(1000.0)};{bits(1000.5)}', f'{bits(123456.789)};{bits(123456.790)}', f'{NAN};{NAN}')
    # 곡 길이 전수: 길이를 잘못 믿거나 시계를 한 번 덜 읽으면 달라지는 경계들이다.
    for request, duration, clock in itertools.product(('wind22.mus', 'fanfare.mus', 'defeat.mus'), durations, clocks):
        scripts.append(ops('cur:-', f'i:result:{1}', f'clk:{clock}', f'durs:{bits(duration) if duration == duration else NAN}', f'req:{named(request)}'))
    return scripts


def next_scripts():
    """다음 곡: 색인·플레이어 준비·희생 여부·로컬 플레이어·날씨 설정·현재 곡을 교차한다."""
    scripts = []
    # 색인은 0~3과 감김 경계(7, 0x7fffffff)를 쓴다. 음수는 원본이 표 앞을 읽으므로 넣지 않는다.
    for index, players, sacrificing, local, option, current in itertools.product((0, 1, 2, 3, 4, 7, 0x7fffffff), (0, 1, 0x100), (0, 1), (0, 3, 0x100), (0, 1), ('ser22.mus', 'wind22.mus')):
        scripts.append(ops(f'cur:{named(current)}', f'i:index:{index}', f'i:players:{players}', f'i:local:{local}', f'sac:{sacrificing}', f'opt:{option}',
            f'clk:{bits(1000.0)};{bits(1000.5)}', f'durs:{bits(214.1)}', 'next'))
    return scripts


def frame_scripts():
    """프레임 확인: 곡 끝 시각과 시계(지남·같음·앞·비교 불가)·전투/대기실 여부를 교차한다."""
    scripts = []
    ends = (bits(999.0), bits(1000.0), bits(1001.0), NAN)
    # 곡 끝이 지난 경우의 분기(다음 곡/대기실/메뉴)와 아직인 경우의 무동작을 모두 본다.
    for end, wall, battle, waiting, current, index in itertools.product(ends, (bits(1000.0), NAN), (0, 1, 0x100), (0, 1), ('ser22.mus', 'sacrifice.mus', 'anticipation.mus'), (3, 1)):
        scripts.append(ops(f'cur:{named(current)}', f'i:index:{index}', f'i:battle:{battle}', f'wait:{waiting}', f'sac:1', f'opt:1', f'f:song:{end}',
            f'clk:{wall};{bits(1000.5)};{bits(1001.0)}', f'durs:{bits(232.8)}', 'frame'))
    return scripts


def start_scripts():
    """시작: 전투 여부·난수 상태·희생·설정·현재 곡·곡 끝 시각을 교차한다."""
    scripts = []
    # 난수 상태 0은 원본이 고정 시드로 바꾼다. 곡 끝이 이미 지난 경우는 Frame이 다음 곡으로 한 번 더 넘어간다.
    for battle, rng, players, sacrificing, option, current, end in itertools.product((0, 1), (0, 1, 0x0bad0bad, 0x12345678, 0xffffffff, 7), (0, 1), (0, 1), (0, 1),
            ('', 'ser22.mus', 'rain22.mus'), (bits(0.0), bits(1e9))):
        scripts.append(ops(f'cur:{named(current)}', f'i:battle:{battle}', f'rng:{rng}', f'i:players:{players}', f'sac:{sacrificing}', f'opt:{option}',
            f'f:song:{end}', f'clk:{bits(1000.0)};{bits(1000.5)};{bits(1001.0)};{bits(1200.0)}', f'durs:{bits(214.1)};{bits(225.0)}', 'start'))
    return scripts


def weather_scripts():
    """날씨 효과: 색인·설정·초기 색·갱신 표시를 교차한다."""
    # 색인마다 설정·초기 색·초기 갱신 카운터를 바꿔 날씨 효과 한 번을 실행하는 입력을 만든다.
    return [ops(f'i:index:{index}', f'i:tint:{tint}', f'i:dirty:{dirty}', f'opt:{option}', 'weather')
            for index, option, tint, dirty in itertools.product((0, 1, 2, 3), (0, 1), (167, 0xffffffff), (0, 5))]


def sequence_scripts():
    """여러 호출을 이은 흐름: 시계를 곡 길이만큼씩 올리며 전투 순환·희생 전환·결과 곡·메뉴 복귀를 만든다. 입력만 정하고 기대값은 없다."""
    rng = random.Random(0x469fc0)
    scripts = []
    choices = ('start', 'frame', 'frame', 'frame', 'next', 'weather', 'req:ser22.mus', 'req:fanfare.mus', 'req:defeat.mus', 'req:anticipation.mus',
               'req:sacrifice.mus', 'req:wind22.mus', 'req:RAIN22.MUS', 'lock')
    # 서로 다른 시드의 열 단계 흐름을 만든다.
    for _ in range(240):
        steps = []
        clock, now = [], 1000.0
        # 시계는 단조 증가시키되 가끔 길게 건너뛰거나 비교 불가 값을 넣는다.
        for _ in range(48):
            now += rng.choice((0.0, 0.5, 3.0, 100.0, 214.1, 300.0))
            clock.append(NAN if rng.random() < 0.03 else bits(now))
        durations = [rng.choice((bits(214.1), bits(232.8), bits(225.0), bits(206.8), bits(139.9), bits(21.6), bits(29.0), NAN, 'keep', bits(0.0))) for _ in range(24)]
        steps += [f'cur:{named(rng.choice(("", "ser22.mus", "wind22.mus", "fanfare.mus")))}', f'i:battle:{rng.choice((0, 1, 1))}', f'i:players:{rng.choice((0, 1, 1))}',
                  f'i:result:{rng.choice((0, 1, 1, 2))}', f'i:local:{rng.choice((0, 3))}', f'rng:{rng.choice((0, 7, 0x12345678, 0xdeadbeef))}',
                  f'f:song:{bits(rng.choice((0.0, 1000.0, 1e9)))}', f'clk:{";".join(clock)}', f'durs:{";".join(durations)}',
                  f'sac:{";".join(str(rng.choice((0, 0, 1))) for _ in range(8))}', f'wait:{rng.choice((0, 1))}', f'opt:{";".join(str(rng.choice((0, 1))) for _ in range(4))}']
        # 호출 사이에 전역을 바꾸는 연산(결과 상태·전투 여부)도 끼워 넣는다.
        for _ in range(rng.randint(5, 10)):
            if rng.random() < 0.2: steps.append(f'i:{rng.choice(("result", "battle", "players"))}:{rng.choice((0, 1, 2))}')
            action = rng.choice(choices)
            steps.append(f'req:{named(action[4:])}' if action.startswith('req:') else action)
        scripts.append(ops(*steps))
    return scripts


def inputs():
    """고정 순서의 전체 입력."""
    groups = (lock_scripts(), request_scripts(), next_scripts(), frame_scripts(), start_scripts(), weather_scripts(), sequence_scripts())
    return [('Script', script) for group in groups for script in group]


class SceneMusicOracle(OwnerOracle):
    """실제 장면 음악 몸체만 실행하고 곡 선택·시계·월드·화면·설정·효과음은 기록 대체한다."""
    def __init__(self, edition):
        """새 내보내기의 불연속 몸체만 실행을 허용한다."""
        super().__init__(edition)
        self.b = BODY[edition]
        self.g = self.b['g']
        # SEH 프레임(fs:[0])·이름 인자·작업 메모리다.
        for address, size in ((0, 0x1000), (NAMES, 0x2000), (WORK, 0x1000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/scenemusic/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 구간 사이 임의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        # 대체 경계: 주소 -> (이름, 처리기).
        self.stubs = {address: (name, getattr(self, 'stub_' + name)) for name, address in self.b['stubs'].items()}
        self.returns = collections.Counter()
        self.substitutions = collections.Counter()
        self.calls = 0

    def u32(self, address):
        """비정렬 DWORD 읽기."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def text(self, address):
        """NUL로 끝나는 문자열 읽기."""
        data = bytearray()
        # 한 바이트씩 NUL까지 읽는다.
        while True:
            byte = self.mu.mem_read(address + len(data), 1)[0]
            if not byte: return data.decode('latin-1')
            data.append(byte)

    def leave(self, mu, value, purge=0):
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

    def scripted(self, key):
        """목록 입력을 하나씩 소비한다. 목록이 끝나면 마지막 값을 반복하고 비어 있으면 기본값을 쓴다."""
        values, index = self.lists[key]
        value = values[min(index, len(values) - 1)] if values else self.defaults[key]
        self.lists[key] = (values, index + 1)
        return value

    def stub_wall(self, mu):
        """실시간 시계(cdecl, 출력 버퍼 하나): 입력 시각을 쓰고 버퍼 주소를 돌려준다."""
        target, = self.arguments(mu, 1)
        mu.mem_write(target, bytes.fromhex(self.scripted('clk'))[::-1])
        self.events.append('T')
        self.leave(mu, target)

    def stub_select(self, mu):
        """곡 선택(cdecl, 이름 하나): 현재 이름을 strncpy처럼 바꾸고 곡 길이를 설치한다. 실제 선택 본체는 별도 단계에서 대조했다."""
        name = self.text(self.arguments(mu, 1)[0])
        self.events.append('S:' + (name.encode('latin-1').hex() or '-'))
        raw = name.encode('latin-1')[:255]
        mu.mem_write(self.g['name'], raw + bytes(255 - len(raw)))
        duration = self.scripted('durs')
        if duration != 'keep': mu.mem_write(self.g['duration'], bytes.fromhex(duration)[::-1])
        self.leave(mu, 0xcafe)

    def stub_sacrificing(self, mu):
        """희생 의식 판정(cdecl, 플레이어 하나)."""
        player, = self.arguments(mu, 1)
        self.events.append(f'C:{player}')
        self.leave(mu, int(self.scripted('sac')))

    def stub_waiting(self, mu):
        """멀티플레이 대기실 판정(cdecl)."""
        self.events.append('W')
        self.leave(mu, int(self.scripted('wait')))

    def stub_refresh(self, mu):
        """화면 갱신 요청(cdecl)."""
        self.events.append('R')
        self.leave(mu, 0xcafe)

    def stub_option(self, mu):
        """설정 조회(cdecl 이름, 기대값): 이름이 ascendancyPalette이고 기대값이 1인지 확인한다."""
        name, expected = self.arguments(mu, 2)
        if self.text(name) != 'ascendancyPalette' or expected != 1: raise RuntimeError('설정 조회 인자 오류')
        self.events.append('O')
        self.leave(mu, int(self.scripted('opt')))

    def stub_join(self, mu):
        """팔레트 경로 조립(cdecl 여덟 인자): 디렉터리 두 개와 팔레트 이름을 확인하고 가짜 문자열 객체를 돌려준다."""
        values = self.arguments(mu, 8)
        if (values[0], values[2], values[4], values[6], values[7]) != (0xfffffffe, 0xfffffffe, 0xfffffffd, 0, 0) or values[1] != DIR_A or values[3] != DIR_B:
            raise RuntimeError('경로 조립 인자 오류')
        self.events.append('J:' + self.text(values[5]).encode('latin-1').hex())
        self.leave(mu, PATH_OBJECT)

    def stub_apply(self, mu):
        """팔레트 적용(cdecl 경로 객체, 0): 조립이 돌려준 객체가 그대로 오는지 확인한다."""
        path, extra = self.arguments(mu, 2)
        if path != PATH_OBJECT or extra != 0: raise RuntimeError('팔레트 적용 인자 오류')
        self.events.append('A')
        self.leave(mu, 0xcafe)

    def stub_thunder(self, mu):
        """번개 효과(cdecl, 0)."""
        if self.arguments(mu, 1)[0] != 0: raise RuntimeError('번개 효과 인자 오류')
        self.events.append('K')
        self.leave(mu, 0xcafe)

    def stub_sound(self, mu):
        """날씨 효과음(cdecl 여섯 인자): 이름 뒤의 인자는 모두 0이어야 한다."""
        values = self.arguments(mu, 6)
        if any(values[1:]): raise RuntimeError('효과음 인자 오류')
        self.events.append('N:' + self.text(values[0]).encode('latin-1').hex())
        self.leave(mu, 0xcafe)

    def on_instruction(self, mu, address, size, data):
        """대체 경계는 기록하고 나머지는 허용 범위 안의 실제 명령만 실행한다."""
        stub = self.stubs.get(address)
        if stub:
            self.substitutions[stub[0]] += 1
            stub[1](mu)
        elif address == self.spec['assert_report']:
            raise RuntimeError('장면 음악 몸체에서 assert 보고')
        elif address == self.b.get('stricmp'):
            # 패치 CRT는 스레드 로캘을 OS에서 얻는다. C 로캘의 ASCII 비교 본문으로 넘긴다.
            self.substitutions['locale'] += 1
            mu.reg_write(UC_X86_REG_EIP, self.b['ascii'])
        else:
            super().on_instruction(mu, address, size, data)

    def call(self, entry, stack, purge, control):
        """보존 레지스터·스택·SEH·x87을 확인하며 한 함수를 정상 반환까지 실행하고 EAX를 돌려준다."""
        saved = ((UC_X86_REG_EBX, 0x11223344), (UC_X86_REG_ESI, 0x22334455), (UC_X86_REG_EDI, 0x33445566), (UC_X86_REG_EBP, 0x44556677))
        # 비영 callee 보존 레지스터를 확인한다.
        for register, value in saved: self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0x0badf00d)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(0, struct.pack('<I', 0x12345678))
        self.mu.mem_write(STACK, struct.pack(f'<{len(stack) + 1}I', STOP, *stack))
        self.mu.emu_start(entry, STOP, count=2000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'정상 반환/스택 오류: {self.edition} {entry:08x}')
        if any(self.mu.reg_read(register) != value for register, value in saved) or self.u32(0) != 0x12345678:
            raise RuntimeError('보존 레지스터/SEH 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어/TOP 오류')
        self.calls += 1
        return self.mu.reg_read(UC_X86_REG_EAX)

    def put(self, name, value):
        """판본의 전역 주소에 DWORD를 쓴다."""
        self.mu.mem_write(self.g[name], struct.pack('<I', value & 0xffffffff))

    def reset(self):
        """스크립트 시작 상태: 원본 초기값과 합성 입력. 이름 저장 공간과 곡 길이는 0이다."""
        g = self.g
        # 정수 전역을 원본 초기값으로 되돌린다.
        for name, value in dict(index=3, result=0, battle=0, players=0, local=0, tint=167, dirty=0, rng=0).items(): self.put(name, value)
        self.put('dir_a', DIR_A)
        self.put('dir_b', DIR_B)
        # 배정도 전역 네 개(승리·패배 곡 끝, 지금 곡 끝, 곡 길이)를 0.0으로 되돌린다.
        for name in ('fanfare', 'defeat', 'song', 'duration'): self.mu.mem_write(g[name], bytes(8))
        self.mu.mem_write(g['name'], bytes(256))
        # 날씨 색 표는 실행 중에 채워지는 값이므로 서로 다른 시험 값을 준다.
        self.mu.mem_write(g['tints'], struct.pack('<4I', *DEFAULT_TINTS))
        self.lists = dict(clk=([], 0), durs=([], 0), sac=([], 0), wait=([], 0), opt=([], 0))
        self.defaults = dict(clk=bits(1000.0), durs='keep', sac='0', wait='0', opt='0')
        write = lambda name, size: (self.g[name], self.g[name] + size)
        self.write_ranges = [(0, 4), write('index', 4), write('fanfare', 8), write('defeat', 8), write('song', 8), write('tint', 4), write('dirty', 4), write('rng', 4)]

    def run_script(self, row, control):
        """연산 목록을 차례로 실행한다. 호출 연산마다 "사건=반환"을, 끝에 전역 스냅샷을 관찰한다."""
        self.reset()
        g, tokens = self.g, []
        # 연산마다 입력 설정 한 번 또는 실제 진입 한 번이다.
        for operation in row[1].split('|'):
            op = operation.split(':')
            self.events, result = [], '-'
            if op[0] == 'i': self.put(op[1], int(op[2]))
            elif op[0] == 't': self.mu.mem_write(g['tints'] + 4 * int(op[1]), struct.pack('<I', int(op[2])))
            elif op[0] == 'f': self.mu.mem_write(g[op[1]], bytes.fromhex(op[2])[::-1])
            elif op[0] == 'cur':
                raw = b'' if op[1] == '-' else bytes.fromhex(op[1])
                self.mu.mem_write(g['name'], raw + bytes(256 - len(raw)))
            elif op[0] == 'rng': self.put('rng', int(op[1]))
            elif op[0] in ('clk', 'durs', 'sac', 'wait', 'opt'): self.lists[op[0]] = (op[1].split(';') if op[1] else [], 0)
            elif op[0] in CALLS:
                if op[0] == 'req':
                    raw = b'' if op[1] == '-' else bytes.fromhex(op[1])
                    self.mu.mem_write(NAMES, raw + b'\0')
                    value = self.call(self.b['request'], (NAMES,), 0, control)
                else:
                    value = self.call(self.b[CALLS[op[0]]], (), 0, control)
                    if op[0] == 'lock': result = str(value)
                self.returns[op[0]] += 1
                tokens.append((';'.join(self.events) or '-') + '=' + result)
                continue
            else: raise RuntimeError(f'알 수 없는 연산 {operation}')
        name = bytes(self.mu.mem_read(g['name'], 256))
        end = name.index(b'\0') if b'\0' in name else 256
        snapshot = [self.u32(g['index']), bytes(self.mu.mem_read(g['fanfare'], 8))[::-1].hex(), bytes(self.mu.mem_read(g['defeat'], 8))[::-1].hex(),
            bytes(self.mu.mem_read(g['song'], 8))[::-1].hex(), self.u32(g['tint']), self.u32(g['dirty']), self.u32(g['rng']),
            name[:end].hex() or '-', bytes(self.mu.mem_read(g['duration'], 8))[::-1].hex(), self.u32(g['result'])]
        return ['|'.join(tokens), *snapshot]

    def run(self, row, control):
        """입력 종류별 실행."""
        return self.run_script(row, control)


def generate(smoke=False):
    """세 판본의 두 정밀도 관찰이 같은 입력만 fixture로 저장한다."""
    cases = inputs()[::37] if smoke else inputs()
    rows, editions = [], {}
    paths = {Path(__file__), FIXTURE, ROOT / 'tools/ghidra/scenemusic-functions.json', ROOT / 'tools/decomp_owner_oracle.py'}
    # 판본별 PE와 실제 몸체는 독립으로 읽는다.
    for edition in SPECS:
        oracle = SceneMusicOracle(edition)
        # Python에서 곡 선택/시각/반환의 기대값을 계산하지 않고 원본의 관찰끼리 비교한다.
        for case in cases:
            first, second = (oracle.run(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'두 x87 정밀도 관찰 불일치: {edition} {case[1][:200]}')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=dict(oracle.returns), calls=oracle.calls, native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 장면 음악 {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 장면 음악 감독(결과 곡 잠금·곡 요청·다음 곡·프레임 확인·시작·날씨 효과). 시계/선택/월드/화면/설정/효과음은 입력대로 흉내 낸 경계.\n'
        '# edition Script ops | 호출별 "사건=반환" index fanfareEnd defeatEnd songEnd tint dirty rng name duration result\n'
        '#   ops: i:전역:값 t:색인:값 f:fanfare|defeat|song:비트 cur:이름hex rng:값 clk|durs|sac|wait|opt:목록 lock|req:이름hex|next|frame|start|weather\n'
        '#   사건: T 시계, S 선택, C 희생 판정, W 대기실 판정, R 화면 갱신, O 설정 조회, J 팔레트 경로, A 팔레트 적용, K 번개, N 날씨 효과음\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, host='HJOW-Athlon', decompile_date='2026-10-10', total=len(rows), controls=list(CONTROLS),
        editions=editions, os_calls=0, default_tints=list(DEFAULT_TINTS),
        stubbed=['실시간 시계', '곡 선택(현재 이름 갱신과 곡 길이 설치를 입력대로 흉내)', '희생 의식 판정', '대기실 판정', '화면 갱신', '설정 조회',
                 '팔레트 경로 조립/적용', '번개 효과', '날씨 효과음', '패치 CRT 스레드 로캘 선택(C 로캘 ASCII 비교 본문은 실제 실행)'],
        limitations=['색인이 0~3을 벗어나는 입력(원본은 표 앞 메모리를 읽음) 제외', '실제 곡 선택/재생/월드/화면은 이 대조의 범위 밖', '날씨 색 표는 시험 값'],
        files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(paths)}), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA/정확한 입력/정상 반환/실제 몸체 진입과 OS 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    expected = inputs()
    if report['schema'] != 1 or report['controls'] != list(CONTROLS) or report['os_calls'] or set(report['editions']) != set(SPECS) \
            or report['default_tints'] != list(DEFAULT_TINTS):
        raise RuntimeError('감사 스키마/정밀도/판본/OS 오류')
    # PE/실행기/모든 내보내기/fixture의 바이트를 확인한다.
    for name, value in report['files'].items():
        if digest(ROOT / name) != value: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != len(expected) * 3 or report['total'] != len(rows): raise RuntimeError('행 개수 오류')
    entered = collections.Counter(operation.split(':')[0] for case in expected for operation in case[1].split('|') if operation.split(':')[0] in CALLS)
    # 판본마다 누락/중복 없이 전체 입력을 같은 순서로 실행해야 한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]
        body = BODY[edition]
        if [row[1:1 + len(case)] for row, case in zip(selected, expected)] != [list(map(str, case)) for case in expected] or item['cases'] != len(expected):
            raise RuntimeError(f'입력 오류: {edition}')
        if item['assertions']: raise RuntimeError(f'assert 오류: {edition}')
        # 모든 호출 연산이 두 정밀도에서 정상 반환해야 하고 직접 진입한 몸체는 그 수 이상 실행돼야 한다(다른 몸체가 부르는 경우가 더해진다).
        for operation, key in CALLS.items():
            if item['returns'].get(operation) != entered[operation] * 2 or item['native_calls'].get(f'{body[key]:08x}', 0) < entered[operation] * 2:
                raise RuntimeError(f'진입/반환 오류: {edition}/{operation}')
        if not item['native_calls'].get(f'{body["duration"]:08x}') or not item['native_calls'].get(f'{body["rng_fn"]:08x}'):
            raise RuntimeError(f'곡 길이/난수 진입 누락: {edition}')
    print(f'scenemusic 검증 통과: {len(rows)}개')


def main():
    """새 원본 관찰 생성/저장 감사/소수 입력 점검을 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
