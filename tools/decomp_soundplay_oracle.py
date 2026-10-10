#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소리 재생 계층(Sound.cpp)의 재생/정지/재생 여부와 화면 위치 계산을 세 실제 PE에서 정상 반환까지 실행한다.

실제 명령으로 실행하는 것: 전역 재생, 위치 반복 재생, 위치 한 번 재생, 정지, 재생 여부, 이름 기반 재생 두 가지,
버퍼 상태 조회/정지 도우미, 월드→화면 좌표, 화면 안 판정, 좌우/음량 계산, 이름 표 조회/이름 복사, ASCII 비교, CRT 정수 변환.
명시 기록 대체: 장치 버퍼의 COM 호출(GetStatus/Play/SetCurrentPosition/SetVolume/SetPan/Stop)과 DuplicateSoundBuffer,
wav 읽기(항목 적재), 기록 출력, assert 보고(기록 뒤 계속), 패치 CRT의 스레드 로캘 선택, 소리 장치 초기화의 표 확보/대입.
대체한 장치는 "재생하면 재생 중, 정지하면 멈춤"만 흉내 내는 상태표이며 C++ 검사도 같은 규칙의 장치를 쓴다.
게임/OS/창/소리 장치는 실행하지 않는다. --verify는 저장된 SHA/입력/진입/반환 근거를 감사한다.

python -X utf8 tools/decomp_soundplay_oracle.py            # 관찰 fixture·근거 기록 생성
python -X utf8 tools/decomp_soundplay_oracle.py --smoke    # 일부 입력만 실행(저장하지 않음)
python -X utf8 tools/decomp_soundplay_oracle.py --verify   # 저장된 기록 감사
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, STOP, STACK, digest, UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS)
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 소리 이름 표(C++ kSoundListBase와 같은 주소)와 외부 이름 문자열, 화면 좌표 계산용 작업 메모리다.
SOUNDS, NAMES, WORK = 0x15000000, 0x15100000, 0x15200000
# 가짜 장치 객체: IDirectSound 하나와 버퍼들. 버퍼 값은 BUFFERS + 16 × 순번이며 C++ 검사 장치도 같은 값을 만든다.
DEVICE, DEVICE_TABLE, BUFFER_TABLE, BUFFERS = 0x16000000, 0x16000100, 0x16000200, 0x16001000
# 가짜 가상 표가 가리키는 대체 진입 주소(코드는 없고 진입 순간에 가로챈다).
STUBS = STOP + 0x800
LIST_BYTES = 0x8000
# 두 x87 정밀도에서 같은 관찰이어야 한다.
CONTROLS = (0x027f, 0x037f)
# 소리 장치 초기화가 표에 처음 넣는 이름과, 버퍼가 없는 소리를 뜻하는 원본 표식 값이다.
FALLBACK_NAME, SILENT = 'nonexistant.wav', 0xffffffff
# 판본별 실제 몸체·대체 경계·전역이다.
BODY = {
 'originals': dict(play=0x4a9550, loop=0x4a9860, once=0x4a9c30, stop=0x4a97e0, playing=0x4a9810, byname=0x4a9cb0, nameat=0x4a9d70,
    screen=0x497220, visible=0x4c6b30, pan=0x4c6a40, volume=0x4c6ac0, lookup=0x4a8ee0, ascii=0x4ef510, stricmp=0x4e65ed,
    load=0x4a9170, log=0x4c2630, g=dict(initialized=0x5c7b08, device=0x5c7b0c, enabled=0x54daa0, max=0x5424a8, playing=0x5c7b38,
    master=0x5c7b20, serial=0x5c7b34, swap=0x54db8c, left=0x5ca9cc, top=0x5ca9d0, right=0x5ca9d4, bottom=0x5ca9d8,
    camx=0x59a91c, camy=0x59a920, silent=0x542494, list=0x5c7b44, free=0x5c7b48, fallback=0x5c7b4c)),
 'originalCD': dict(play=0x4383f0, loop=0x438800, once=0x438ae0, stop=0x4386f0, playing=0x4387a0, byname=0x438b50, nameat=0x438c00,
    screen=0x455c90, visible=0x4cdeb0, pan=0x4cdd00, volume=0x4cde00, lookup=0x437d20, ascii=0x4f2980,
    load=0x438000, log=0x48ca70, g=dict(initialized=0x51a6f0, device=0x51a6f4, enabled=0x52e824, max=0x51a73c, playing=0x51a738,
    master=0x51a720, serial=0x51a734, swap=0x52e934, left=0x583e20, top=0x583e24, right=0x583e28, bottom=0x583e2c,
    camx=0x565c34, camy=0x565c38, silent=0x51a6fc, list=0x51a74c, free=0x51a750, fallback=0x51a754)),
}
BODY['original1037'] = dict(BODY['originalCD'])
# 장치 대체: (이름, 가상 표 오프셋, stdcall이 정리하는 인자 바이트).
METHODS = (('status', 0x24, 8), ('start', 0x30, 16), ('seek', 0x34, 8), ('gain', 0x3c, 8), ('balance', 0x40, 8), ('halt', 0x48, 4))
FIXTURE = ROOT / 'cpppj/tests/fixtures/soundplay-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-soundplay-evidence.json'
# 실패/성공 반환값(HRESULT)과 스크립트가 시작할 때의 전역 기본값(원본 초기값 + 장치 준비 전 상태)이다.
FAIL, ODD = 0x80004005, 1
DEFAULTS = dict(initialized=0, device=0, enabled=0, max=8, playing=0, master=0, serial=0, swap=0, left=0, top=0, right=640, bottom=480, camx=0, camy=0)


def bits(value):
    """단정도 입력의 비트."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def named(text):
    """이름을 16진수로 적어 구분 문자와 섞이지 않게 한다."""
    return text.encode('latin-1').hex()


def signed(value):
    """DWORD를 부호 있는 수로 읽는다."""
    return value - 0x100000000 if value & 0x80000000 else value


def ready(*names):
    """장치가 준비된 표준 시작 상태와 이름 조회들이다. 조회 결과는 $6부터 차례로 쓴다."""
    return ['i', 'g:initialized:1', 'g:device:1', 'g:enabled:1', 'view:0:0:640:480', 'cam:0:0'] + ['n:' + named(name) for name in names]


def play_scripts():
    """전역 재생: 준비 여부·우선·동시 재생 한도·적재 결과·음량 계산·사슬 상태·복제 실패를 교차한다."""
    scripts = []
    # 준비/사용 여부·우선·한도 대 현재 재생 수. 실패해야 하는지는 원본이 정한다.
    for initialized, enabled, priority, (limit, count) in itertools.product((0, 1), (0, 1), (0, 1, 8), ((8, 0), (8, 7), (8, 8), (8, 9), (0, 50), (-1, 0))):
        scripts.append(['i', f'g:initialized:{initialized}', 'g:device:1', f'g:enabled:{enabled}', f'g:max:{limit}', f'g:playing:{count}',
                        'n:' + named('a.wav'), f'play:$6:0:0:0:{priority}:0', 'isp:$6', 'play:0:0:0:0:1:0'])
    # 음량 = 인자 + 전체 음량 − 파일 감쇠, [-10000, 0]으로 자른다. 좌우·반복 인자와 설정 실패 반환을 섞는다.
    for index, (attenuation, volume, master, result) in enumerate(itertools.product((0, 500, -300, 12000), (0, -2000, -20000, 3000, 0x7fffffff),
            (0, -1000, 700), (0, FAIL, ODD))):
        pan, loop = (0, -5000, 10000, 123)[index % 4], (0, 1, 8)[index % 3]
        scripts.append(ready('a.wav') + [f'ld:b:{attenuation}', f'g:master:{master & 0xffffffff}', f'hr:gain:{result}', f'hr:balance:{(0, FAIL)[index % 2]}',
                       f'play:$6:{loop}:{volume & 0xffffffff}:{pan & 0xffffffff}:0:0', 'isp:$6', 'stop:$6', 'isp:$6'])
    # 적재 결과: 버퍼·소리 없음 표식·0(장치 없음)과 복제 성공/실패.
    for kind, result in itertools.product('bsz', (0, FAIL, ODD)):
        scripts.append(ready('a.wav') + [f'ld:{kind}:250', f'hr:copy:{result}', 'play:$6:0:0:0:0:0', 'isp:$6', f'play:$6:1:{-100 & 0xffffffff}:0:0:0', 'stop:$6'])
    # 사슬: 같은 소리를 여러 번 재생해 복제 항목을 만들고, 일부를 멈추거나 버퍼를 지운 뒤 한도/복제 결과를 바꿔 다시 재생한다.
    for plays, change, limit, result, target in itertools.product((1, 2, 3), ('none', 'stop0', 'stop1', 'stoplast', 'clr1', 'clrlast', 'lost0'),
            (0, 1, 2, 3, -1), (0, FAIL), ('root', 'last')):
        script = ready('a.wav') + ['ld:b:100'] + ['play:$6:0:0:0:0:0'] * plays
        first, last = len(script) - plays, len(script) - 1
        if change == 'stop0': script.append('st:0:0')
        elif change == 'stop1': script.append(f'st:{min(1, plays - 1)}:0')
        elif change == 'stoplast': script.append(f'st:{plays - 1}:0')
        elif change == 'clr1': script += [f'st:{min(1, plays - 1)}:0', f'clr:${first + min(1, plays - 1)}']
        elif change == 'clrlast': script += [f'st:{plays - 1}:0', f'clr:${last}']
        elif change == 'lost0': script.append('st:0:3')
        script += [f'hr:copy:{result}', f'play:${6 if target == "root" else last}:1:{-50 & 0xffffffff}:20:0:{limit & 0xffffffff}', f'isp:${last}', 'isp:$6']
        scripts.append(script)
    # 버퍼가 지워진 복제 항목을 직접 재생하면 원본 버퍼에서 다시 복제한다. 원본까지 지워졌으면 다시 적재한다.
    for clear_root, result, kind in itertools.product((0, 1), (0, FAIL), 'bs'):
        script = ready('a.wav') + ['ld:b:40', 'play:$6:0:0:0:0:0', 'play:$6:0:0:0:0:0', 'st:0:0', 'st:1:0', 'clr:$9']
        if clear_root: script.append('clr:$6')
        scripts.append(script + [f'ld:{kind}:70', f'hr:copy:{result}', 'play:$9:0:0:0:0:0', 'isp:$9', 'isp:$6'])
    return scripts


def stop_scripts():
    """정지/재생 여부: null·버퍼 없음·소리 없음 표식·재생 중·멈춤·잃어버린 버퍼와 준비/사용 여부."""
    scripts = []
    # 적재 종류와 장치 상태 비트(1 재생, 2 잃음, 4 반복), 그리고 조회 쪽의 준비/사용 여부.
    for kind, status, initialized, enabled in itertools.product('bsn', (0, 1, 2, 3, 5), (0, 1), (0, 1)):
        script = ready('a.wav')
        if kind != 'n': script += [f'ld:{kind}:0', 'play:$6:1:0:0:0:0']
        if kind == 'b': script.append(f'st:0:{status}')
        scripts.append(script + [f'g:initialized:{initialized}', f'g:enabled:{enabled}', 'isp:$6', 'isp:0', 'stop:$6', 'stop:0', 'isp:$6', 'stop:$6'])
    return scripts


def loop_scripts():
    """위치 반복 재생: 화면 안팎·현재 항목의 종류와 상태·다른 이름·한도·음량/좌우 설정 실패·좌우 바꿈."""
    scripts = []
    inside, moved, outside = (bits(20.0), bits(21.0)), (bits(35.5), bits(40.25)), (bits(200.0), bits(21.0))
    # 준비 여부와 한도에 걸리는 경우.
    for initialized, device, enabled, (limit, count) in itertools.product((0, 1), (0, 1), (0, 1), ((8, 0), (8, 8), (0, 9))):
        scripts.append(['i', f'g:initialized:{initialized}', f'g:device:{device}', f'g:enabled:{enabled}', 'view:0:0:640:480', 'cam:0:0',
                        'n:' + named('a.wav'), f'g:max:{limit}', f'g:playing:{count}', f'loop:{inside[0]}:{inside[1]}:0:$6', 'isp:$9'])
    # 시작 → 이동 갱신 → 화면 밖 → 다시 화면 안의 흐름. 감쇠·전체 음량·설정 실패·좌우 바꿈을 섞는다.
    for index, (attenuation, master, gain, balance, swap) in enumerate(itertools.product((0, 800), (0, -1500), (0, FAIL, ODD), (0, FAIL), (0, 1))):
        scripts.append(ready('a.wav') + [f'ld:b:{attenuation}', f'g:master:{master & 0xffffffff}', f'hr:gain:{gain}', f'hr:balance:{balance}', f'g:swap:{swap}',
            f'loop:{inside[0]}:{inside[1]}:0:$6', f'loop:{moved[0]}:{moved[1]}:$12:$6', f'loop:{moved[0]}:{moved[1]}:$13:0',
            f'loop:{outside[0]}:{outside[1]}:$14:$6', f'loop:{outside[0]}:{outside[1]}:$15:$6', f'loop:{inside[0]}:{inside[1]}:$16:$6', 'isp:$6'])
    # 현재 항목이 멈춘 뒤의 재시작, 버퍼가 지워진 현재 항목, 소리 없음 표식, 적재 0과 복제 실패.
    for state, kind, result, place in itertools.product(('playing', 'stopped', 'cleared', 'lost'), 'bsz', (0, FAIL), (inside, outside)):
        script = ready('a.wav') + [f'ld:{kind}:60', f'hr:copy:{result}', f'loop:{inside[0]}:{inside[1]}:0:$6']
        if kind == 'b' and state != 'playing': script.append({'stopped': 'st:0:0', 'cleared': 'st:0:0', 'lost': 'st:0:3'}[state])
        if kind == 'b' and state == 'cleared': script.append('clr:$6')
        scripts.append(script + [f'loop:{place[0]}:{place[1]}:$9:$6', f'loop:{place[0]}:{place[1]}:$9:0', 'isp:$6'])
    # 다른 이름으로 바꾸면 현재 항목을 멈추고 새 소리를 시작한다. 같은 이름의 복제 항목은 같은 소리로 본다.
    for other, place, current_state in itertools.product(('b.wav', 'A.WAV'), (inside, outside), (1, 0)):
        script = ready('a.wav', other) + ['ld:b:0', f'loop:{inside[0]}:{inside[1]}:0:$6']
        if not current_state: script.append('st:0:0')
        scripts.append(script + [f'loop:{place[0]}:{place[1]}:$9:$7', 'isp:$6', 'isp:$7', f'loop:{place[0]}:{place[1]}:0:0'])
    # 이미 전역 재생 중인 소리와 그 복제 항목을 현재 항목으로 준 경우(같은 소리는 하나만 반복 재생한다).
    for plays, place, use in itertools.product((1, 2), (inside, outside), ('root', 'last')):
        script = ready('a.wav') + ['ld:b:0'] + ['play:$6:1:0:0:0:0'] * plays
        last = len(script) - 1
        scripts.append(script + [f'loop:{place[0]}:{place[1]}:0:$6', f'loop:{place[0]}:{place[1]}:${6 if use == "root" else last}:$6', 'st:0:0',
                                 f'loop:{place[0]}:{place[1]}:${last}:$6'])
    return scripts


def once_scripts():
    """위치 한 번 재생과 이름 기반 재생: 화면 안팎·반복/우선 인자·유일하지 않은 이름의 반복 재생."""
    scripts = []
    places = ((bits(20.0), bits(21.0)), (bits(1.0), bits(2.0)), (bits(42.4), bits(46.3)), (bits(-3.0), bits(21.0)), (bits(20.0), bits(47.0)), (bits(300.0), bits(300.0)))
    # 위치에 따른 음량/좌우와 반복·우선 인자, 좌우 바꿈, 카메라.
    for index, (place, loop, priority, swap, camera) in enumerate(itertools.product(places, (0, 1, 8), (0, 1), (0, 1), ((0, 0), (160, -40)))):
        scripts.append(ready('a.wav') + ['ld:b:300', f'g:swap:{swap}', f'cam:{camera[0] & 0xffffffff}:{camera[1] & 0xffffffff}', 'g:max:2', f'g:playing:{index % 3}',
            f'once:{place[0]}:{place[1]}:$6:{loop}:{priority}', 'isp:$6', f'at:{place[0]}:{place[1]}:{named("b.wav")}:{loop}', f'once:{place[0]}:{place[1]}:0:0:0'])
    # 이름 기반 전역 재생: 새 이름·기존 이름·대소문자, 반복 재생은 복제 항목이 있으면 보고한다.
    for name, loop, priority, duplicates in itertools.product(('a.wav', 'A.wav', 'new.wav'), (0, 1), (0, 1), (0, 1, 2)):
        script = ready('a.wav') + ['ld:b:0'] + ['play:$6:0:0:0:0:0'] * duplicates
        scripts.append(script + [f'name:{named(name)}:{loop}:{-700 & 0xffffffff}:300:{priority}:0', f'name:{named(name)}:{loop}:0:0:{priority}:1', 'isp:$6'])
    return scripts


def script_inputs():
    """스크립트 형식의 모든 입력."""
    return [('Script', '|'.join(script)) for script in play_scripts() + stop_scripts() + loop_scripts() + once_scripts()]


def point_inputs():
    """좌표 계산 입력: 월드→화면(좌표 비트, 카메라), 화면 안 판정/좌우/음량(화면 점, 화면 영역, 좌우 바꿈)."""
    rows = []
    worlds = [bits(v) for v in (0.0, -0.0, 1.0, 20.75, 21.9, 0.03125, 0.031249, -0.03125, -0.04, -1.5, 255.999, 256.0, 1000.5, 3.0e9, -3.0e9, 1.0e20)] + [0x7fc12345, 0x7f800000, 0xff800000]
    # 좌표 두 개와 카메라 원점.
    for x, y, camera in itertools.product(worlds, worlds[::3], ((0, 0), (160, 120), (-33, 0x7fffffff))):
        rows.append(('Screen', x, y, camera[0] & 0xffffffff, camera[1] & 0xffffffff))
    views = ((0, 0, 640, 480), (0, 0, 800, 600), (0, 0, 1024, 768), (16, 24, 657, 505), (-100, -50, 300, 200), (0, 0, 2, 2), (0, 0, 3, 480))
    xs = (-1000, -41, -40, -39, 0, 1, 159, 319, 320, 321, 639, 640, 679, 680, 681, 900, 40000, -40000, 46341, 100000)
    ys = (-31, -30, -29, 0, 239, 240, 241, 480, 509, 510, 511, 30000)
    # 화면 영역마다 경계 부근의 점과 32비트 곱셈이 넘치는 먼 점을 넣는다.
    for view, x, y, swap in itertools.product(views, xs, ys, (0, 1)):
        rows.append(('Point', x & 0xffffffff, y & 0xffffffff, *(v & 0xffffffff for v in view), swap))
    return rows


def inputs():
    """고정 순서의 전체 입력."""
    return script_inputs() + point_inputs()


class SoundPlayOracle(OwnerOracle):
    """실제 재생 계층 몸체만 실행하고 장치 버퍼·적재·기록은 상태표로 대체한다."""
    def __init__(self, edition):
        """새 내보내기의 불연속 몸체만 실행을 허용하고 가짜 장치의 가상 표를 만든다."""
        super().__init__(edition)
        self.b = BODY[edition]
        self.g = self.b['g']
        # SEH 프레임(fs:[0])·소리 표·이름·작업 메모리·가짜 장치 객체다.
        for address, size in ((0, 0x1000), (SOUNDS, 0x10000), (NAMES, 0x1000), (WORK, 0x1000), (DEVICE, 0x10000)):
            self.mu.mem_map(address, size)
        self.exports = [ROOT / f'extracted/soundplay/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 구간 사이 임의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        if self.u32(self.g['silent']) != SILENT or self.u32(self.g['max']) != 8:
            raise RuntimeError('소리 전역 초기값 오류')
        # 가짜 IDirectSound(+0x14 복제)와 버퍼 가상 표. 대체 진입은 주소 -> (이름, 정리할 인자 바이트)다.
        self.stubs = {STUBS: ('copy', 12)}
        self.mu.mem_write(DEVICE, struct.pack('<I', DEVICE_TABLE))
        self.mu.mem_write(DEVICE_TABLE + 0x14, struct.pack('<I', STUBS))
        # 버퍼 메서드마다 진입 주소를 하나씩 둔다.
        for index, (name, offset, purge) in enumerate(METHODS):
            self.stubs[STUBS + 0x10 * (index + 1)] = (name, purge)
            self.mu.mem_write(BUFFER_TABLE + offset, struct.pack('<I', STUBS + 0x10 * (index + 1)))
        self.stubs[self.b['load']] = ('load', 4)
        self.stubs[self.b['log']] = ('log', 0)
        self.returns = collections.Counter()
        self.substitutions = collections.Counter()

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

    def ordinal(self, buffer):
        """버퍼 값을 사건에 적는 순번으로 바꾼다. 순번이 없는 값(0 등)은 그대로 적는다."""
        index, rest = divmod(buffer - BUFFERS, 16)
        return f'#{index}' if 0 <= index < len(self.status) and rest == 0 else str(buffer)

    def create(self):
        """새 가짜 버퍼를 만든다. 처음 상태는 멈춤이다."""
        buffer = BUFFERS + 16 * len(self.status)
        self.mu.mem_write(buffer, struct.pack('<I', BUFFER_TABLE))
        self.status.append(0)
        return buffer

    def device(self, mu, name, purge):
        """장치/적재/기록 대체. 호출 사실과 인자를 적고 상태표를 갱신한다."""
        if name == 'log':
            # (채널, 형식, 가변 인자)의 cdecl이다. %s/%d만 쓰이므로 그 둘을 채워 한 줄로 적는다.
            channel, pattern = self.arguments(mu, 2)
            parts, taken, text = self.text(pattern).split('%'), 2, ''
            text = parts[0]
            # 변환 지정자마다 다음 가변 인자를 하나씩 채운다.
            for part in parts[1:]:
                value = self.arguments(mu, taken + 1)[taken]
                taken += 1
                text += (self.text(value) if part[0] == 's' else str(signed(value))) + part[1:]
            self.events.append(f'G:{channel}:{text.rstrip()}')
            return self.leave(mu, 0, 0)
        if name == 'load':
            # thiscall(항목, 이름): 설정한 적재 결과를 항목의 +0에 쓰고, 버퍼가 0이 아니면 감쇠를 +0xc에 쓴다.
            entry, (pointer,) = mu.reg_read(UC_X86_REG_ECX), self.arguments(mu, 1)
            if pointer != entry + 0x1c: raise RuntimeError('적재 이름 인자 오류')
            buffer = {'b': self.create, 's': lambda: SILENT, 'z': lambda: 0}[self.load[0]]()
            mu.mem_write(entry, struct.pack('<I', buffer))
            if buffer: mu.mem_write(entry + 0xc, struct.pack('<I', self.load[1] & 0xffffffff))
            self.events.append('F:' + self.text(pointer))
            return self.leave(mu, 0, purge)
        values = self.arguments(mu, purge // 4)
        if name == 'copy':
            # DuplicateSoundBuffer(장치, 원본, 결과 주소): 성공일 때만 새 버퍼를 결과에 쓴다.
            if values[0] != DEVICE: raise RuntimeError('복제 this 오류')
            self.events.append('D:' + self.ordinal(values[1]))
            if not self.results['copy'] & 0x80000000: mu.mem_write(values[2], struct.pack('<I', self.create()))
            return self.leave(mu, self.results['copy'], purge)
        index, rest = divmod(values[0] - BUFFERS, 16)
        if rest or not 0 <= index < len(self.status): raise RuntimeError(f'알 수 없는 버퍼 {values[0]:08x}')
        result = 0
        if name == 'status':
            mu.mem_write(values[1], struct.pack('<I', self.status[index]))
            self.events.append(f'Q:#{index}')
        elif name == 'start':
            self.status[index] = 1 | (4 if values[3] & 1 else 0)
            self.events.append(f'P:#{index}:{values[1]}:{values[2]}:{values[3]}')
        elif name == 'seek': self.events.append(f'C:#{index}:{values[1]}')
        elif name == 'halt':
            self.status[index] &= ~5
            self.events.append(f'X:#{index}')
        else:
            result = self.results[name]
            self.events.append(f'{"V" if name == "gain" else "N"}:#{index}:{signed(values[1])}')
        self.leave(mu, result, purge)

    def on_instruction(self, mu, address, size, data):
        """대체 경계와 assert 보고는 기록하고 나머지는 허용 범위 안의 실제 명령만 실행한다."""
        stub = self.stubs.get(address)
        if stub:
            self.substitutions[stub[0]] += 1
            self.device(mu, *stub)
        elif address == self.spec['assert_report']:
            # 원본 assert 보고는 (조건식, 파일, 줄)의 cdecl이며 보고 뒤 실행을 계속한다.
            self.substitutions['assert'] += 1
            self.events.append('!%d' % self.arguments(mu, 3)[2])
            self.leave(mu, 0, 0)
        elif address == self.b.get('stricmp'):
            # 패치 CRT는 스레드 로캘을 OS에서 얻는다. C 로캘의 ASCII 비교 본문으로 넘긴다.
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
        self.mu.emu_start(entry, STOP, count=2000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'정상 반환/스택 오류: {self.edition} {entry:08x}')
        if any(self.mu.reg_read(register) != value for register, value in saved) or self.u32(0) != 0x12345678:
            raise RuntimeError('보존 레지스터/SEH 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어/TOP 오류')
        self.calls += 1
        return self.mu.reg_read(UC_X86_REG_EAX)

    def put_global(self, name, value):
        """판본의 전역 주소에 DWORD를 쓴다. device는 가짜 장치 객체의 주소 또는 0이다."""
        if name == 'device': value = DEVICE if value else 0
        self.mu.mem_write(self.g[name], struct.pack('<I', value & 0xffffffff))

    def run_script(self, row, control):
        """연산 묶음: 연산별 사건/반환, 마지막 빈 위치·표 전체 Adler-32·재생 수·일련번호를 관찰한다."""
        g = self.g
        self.mu.mem_write(SOUNDS, bytes(0x10000))
        self.mu.mem_write(DEVICE + 0x1000, bytes(0xf000))
        self.mu.mem_write(g['list'], struct.pack('<III', SOUNDS, SOUNDS, 0))
        # 전역을 스크립트 시작 상태로 되돌린다.
        for name, value in DEFAULTS.items(): self.put_global(name, value)
        self.status, self.load, self.results = [], ('b', 0), dict(copy=0, gain=0, balance=0)
        self.write_ranges = [(0, 4), (SOUNDS, SOUNDS + LIST_BYTES), (g['free'], g['free'] + 4), (g['playing'], g['playing'] + 4), (g['serial'], g['serial'] + 4)]
        results, tokens = [], []

        def handle(text):
            """$k는 k번째 연산의 반환 항목, 0은 null이다."""
            return results[int(text[1:])] if text.startswith('$') else int(text)

        def lookup(text):
            """외부 이름으로 실제 조회를 실행한다."""
            self.mu.mem_write(NAMES, text + b'\0')
            return self.call(self.b['lookup'], 0, (NAMES,), 0, control)

        # 연산마다 실제 진입 한 번 또는 입력 설정 한 번이다.
        for operation in row[1].split('|'):
            op = operation.split(':')
            self.events, result, shown = [], 0, '-'
            if op[0] == 'i':
                result = lookup(FALLBACK_NAME.encode('latin-1'))
                self.mu.mem_write(g['fallback'], struct.pack('<I', result))
            elif op[0] == 'n': result = lookup(bytes.fromhex(op[1]))
            elif op[0] == 'g': self.put_global(op[1], int(op[2]))
            elif op[0] == 'view':
                # 화면 영역의 네 변을 차례로 쓴다.
                for name, value in zip(('left', 'top', 'right', 'bottom'), op[1:]): self.put_global(name, int(value))
            elif op[0] == 'cam':
                # 카메라 원점의 두 좌표를 쓴다.
                for name, value in zip(('camx', 'camy'), op[1:]): self.put_global(name, int(value))
            elif op[0] == 'st': self.status[int(op[1])] = int(op[2])
            elif op[0] == 'hr': self.results[op[1]] = int(op[2])
            elif op[0] == 'ld': self.load = (op[1], int(op[2]))
            elif op[0] == 'clr': self.mu.mem_write(handle(op[1]), bytes(4))
            elif op[0] == 'play': result = self.call(self.b['play'], 0, (handle(op[1]), *map(int, op[2:7])), 0, control)
            elif op[0] == 'loop': result = self.call(self.b['loop'], 0, (int(op[1]), int(op[2]), handle(op[3]), handle(op[4])), 0, control)
            elif op[0] == 'once': result = self.call(self.b['once'], 0, (int(op[1]), int(op[2]), handle(op[3]), int(op[4]), int(op[5])), 0, control)
            elif op[0] == 'stop': self.call(self.b['stop'], 0, (handle(op[1]),), 0, control)
            elif op[0] == 'isp':
                result = self.call(self.b['playing'], 0, (handle(op[1]),), 0, control)
                shown = str(result)
            elif op[0] == 'name':
                self.mu.mem_write(NAMES, bytes.fromhex(op[1]) + b'\0')
                result = self.call(self.b['byname'], 0, (NAMES, *map(int, op[2:7])), 0, control)
            elif op[0] == 'at':
                self.mu.mem_write(NAMES, bytes.fromhex(op[3]) + b'\0')
                result = self.call(self.b['nameat'], 0, (int(op[1]), int(op[2]), NAMES, int(op[4])), 0, control)
            else: raise RuntimeError(f'알 수 없는 연산 {operation}')
            if op[0] in ('i', 'n', 'play', 'loop', 'once', 'name', 'at'): shown = 'null' if result == 0 else str(result - SOUNDS)
            results.append(result)
            tokens.append((';'.join(self.events) or '-') + '=' + shown)
        if self.u32(g['list']) != SOUNDS: raise RuntimeError('표 전역 변경')
        return ['|'.join(tokens), self.u32(g['free']) - SOUNDS, zlib.adler32(bytes(self.mu.mem_read(SOUNDS, LIST_BYTES))), signed(self.u32(g['playing'])), self.u32(g['serial'])]

    def run_screen(self, row, control):
        """월드→화면: thiscall(결과 점; 사본 주소, x, y). 두 결과 점을 관찰한다."""
        self.events = []
        self.put_global('camx', row[3])
        self.put_global('camy', row[4])
        self.mu.mem_write(WORK, b'\xcc' * 32)
        self.write_ranges = [(0, 4), (WORK, WORK + 32)]
        self.call(self.b['screen'], WORK, (WORK + 16, row[1], row[2]), 12, control)
        first, second = struct.unpack('<ii', self.mu.mem_read(WORK, 8)), struct.unpack('<ii', self.mu.mem_read(WORK + 16, 8))
        if first != second: raise RuntimeError('두 결과 점 불일치')
        return [first[0], first[1]]

    def run_point(self, row, control):
        """화면 점 하나의 화면 안 판정·좌우·음량(thiscall, 점만 읽는다)."""
        self.events = []
        # 화면 영역과 좌우 바꿈 전역을 쓴다.
        for name, value in zip(('left', 'top', 'right', 'bottom', 'swap'), row[3:8]): self.put_global(name, value)
        self.mu.mem_write(WORK, struct.pack('<II', row[1], row[2]))
        self.write_ranges = [(0, 4)]
        return [self.call(self.b['visible'], WORK, (), 0, control), signed(self.call(self.b['pan'], WORK, (), 0, control)),
                signed(self.call(self.b['volume'], WORK, (), 0, control))]

    def run(self, row, control):
        """입력 종류별 실행."""
        result = getattr(self, 'run_' + row[0].lower())(row, control)
        self.returns[row[0]] += 1
        return result

    calls = 0


def generate(smoke=False):
    """세 판본의 두 정밀도 관찰이 같은 입력만 fixture로 저장한다."""
    cases = script_inputs()[::7] + point_inputs()[::41] if smoke else inputs()
    rows, editions = [], {}
    paths = {Path(__file__), FIXTURE, ROOT / 'tools/ghidra/soundplay-functions.json', ROOT / 'tools/decomp_owner_oracle.py'}
    # 판본별 PE와 실제 몸체는 독립으로 읽는다.
    for edition in SPECS:
        oracle = SoundPlayOracle(edition)
        # Python에서 재생 여부/음량/반환을 계산하지 않고 원본의 관찰끼리 비교한다.
        for case in cases:
            first, second = (oracle.run(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'두 x87 정밀도 관찰 불일치: {edition} {case}')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=dict(oracle.returns), calls=oracle.calls, native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 소리 재생 계층 {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 소리 재생 계층(전역/위치 재생·정지·재생 여부·이름 기반 재생)과 화면 위치 계산. 장치 버퍼·적재·기록은 상태표 대체.\n'
        '# edition Script ops | 연산별 "사건=반환" firstFree adler playing serial\n'
        '#   ops: i 초기화, n:이름 조회, g:전역:값, view:l:t:r:b, cam:x:y, st:버퍼순번:상태, hr:copy|gain|balance:반환, ld:b|s|z:감쇠, clr:$k,\n'
        '#        play:$k:반복:음량:좌우:우선:한도, loop:x:y:$현재:$소리, once:x:y:$소리:반복:우선, stop:$k, isp:$k, name:이름:반복:음량:좌우:우선:한도, at:x:y:이름:반복\n'
        '#   사건: F 적재, D 복제, Q 상태, P 재생, C 위치, V 음량, N 좌우, X 정지, G 기록, ! assert 줄. 버퍼 값 = 0x16001000 + 16 × 순번\n'
        '# edition Screen xbits ybits cameraX cameraY | x y\n'
        '# edition Point x y left top right bottom swap | visible pan volume\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, host='HJOW-Athlon', decompile_date='2026-10-10', total=len(rows), controls=list(CONTROLS),
        editions=editions, os_calls=0, sound_list_base=f'{SOUNDS:08x}', buffer_base=f'{BUFFERS:08x}',
        stubbed=['IDirectSoundBuffer GetStatus/Play/SetCurrentPosition/SetVolume/SetPan/Stop', 'IDirectSound DuplicateSoundBuffer', '항목 적재(wav 읽기)',
                 '기록 출력', 'assert 보고(기록 뒤 계속)', '패치 CRT 스레드 로캘 선택(C 로캘 ASCII 비교 본문은 실제 실행)', '소리 장치 초기화의 표 확보/0 채움/대체 소리 대입'],
        limitations=['장치는 재생/정지만 흉내 내는 상태표', '소리 장치(DirectSound)·실제 재생/정지·wav 읽기 제외', '버퍼가 0이거나 소리 없음 표식인 항목의 직접 정지·손상된 사슬 제외',
                     '변조 감지 때의 고의 고장 값(패치 DAT_005318ec == 7)은 초기값 그대로'],
        files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(paths)}), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA/정확한 입력/정상 반환/실제 몸체 진입과 OS 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    expected = inputs()
    if report['schema'] != 1 or report['controls'] != list(CONTROLS) or report['os_calls'] or set(report['editions']) != set(SPECS) \
            or report['sound_list_base'] != f'{SOUNDS:08x}' or report['buffer_base'] != f'{BUFFERS:08x}':
        raise RuntimeError('감사 스키마/정밀도/판본/OS 오류')
    # PE/실행기/모든 내보내기/fixture의 바이트를 확인한다.
    for name, value in report['files'].items():
        if digest(ROOT / name) != value: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != len(expected) * 3 or report['total'] != len(rows): raise RuntimeError('행 개수 오류')
    counts = collections.Counter(case[0] for case in expected)
    entered = collections.Counter(operation.split(':')[0] for case in expected if case[0] == 'Script' for operation in case[1].split('|'))
    # 판본마다 누락/중복 없이 전체 입력을 같은 순서로 실행해야 한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]
        body = BODY[edition]
        if [row[1:1 + len(case)] for row, case in zip(selected, expected)] != [list(map(str, case)) for case in expected] or item['cases'] != len(expected):
            raise RuntimeError(f'입력 오류: {edition}')
        if item['assertions'] or any(item['returns'][kind] != counts[kind] * 2 for kind in counts): raise RuntimeError(f'assert/반환 수 오류: {edition}')
        # 직접 진입한 실제 몸체는 연산 수 × 정밀도 수 이상 실행돼야 한다(다른 몸체가 부르는 경우가 더해진다).
        for operation, key in (('play', 'play'), ('loop', 'loop'), ('once', 'once'), ('stop', 'stop'), ('isp', 'playing'), ('name', 'byname'), ('at', 'nameat')):
            if item['native_calls'].get(f'{body[key]:08x}', 0) < entered[operation] * 2: raise RuntimeError(f'진입 수 오류: {edition}/{operation}')
        if item['native_calls'].get(f'{body["screen"]:08x}', 0) < counts['Screen'] * 2 or any(
                item['native_calls'].get(f'{body[key]:08x}', 0) < counts['Point'] * 2 for key in ('visible', 'pan', 'volume')):
            raise RuntimeError(f'좌표 계산 진입 수 오류: {edition}')
    print(f'soundplay 검증 통과: {len(rows)}개')


def main():
    """새 원본 관찰 생성/저장 감사/소수 입력 점검을 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
