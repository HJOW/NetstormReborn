#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Config.cpp의 객체 층·치환·줄 삭제·값 쓰기 대상 선택·섹션 검색을 x86으로 실행해 기대값을 만든다.

원본 게임 프로세스·OS API를 시작하지 않는다. 두 판본의 결과가 같은 입력만 기대값으로 저장한다.
대체한 것: ASCII toupper/strnicmp/strchr, getenv(고정 표), sprintf("%c%s%c" 계열), assert 보고,
값 쓰기의 서식·이어 붙이기(004402f0 ↔ CD 0042acd0 — 인자만 기록한다).
의존성: tools/requirements-decomp-oracle.txt (extracted/oracle-python에 격리 설치).

python tools/decomp_config_oracle.py
"""
import argparse
import hashlib
import importlib.metadata
import json
import random
import struct
from pathlib import Path

from decomp_oracle import Oracle, ROOT, SCRATCH, STOP, STACK
from unicorn import UcError
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                               UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
                               UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 검토한 함수 쌍. 호출 규약과 ret N은 역어셈블로 확인했다.
PAIRS = [
    {'name': 'Substitute', 'originals': '00440b60', 'originalCD': '0042a960', 'purge': 8},
    {'name': 'Get', 'originals': '004409d0', 'originalCD': '0042a930', 'purge': 8},
    {'name': 'RemoveKey', 'originals': '00440240', 'originalCD': '0042ac50', 'purge': 4},
    {'name': 'Set', 'originals': '004404e0', 'originalCD': '0042af30', 'purge': 8},
    {'name': 'FindSection', 'originals': '0043fe60', 'originalCD': '0042a320', 'purge': 0},
    {'name': 'Section', 'originals': '00440570', 'originalCD': '0042afd0', 'purge': 8},
]
# 위 함수가 부르는 원본 함수(그대로 실행한다): 괄호 해석, 조회, 환경 변수, 키 찾기, 원시 값, 스택 확보 등.
HELPERS = {
    'originals': [0x440a00, 0x440760, 0x440110, 0x43fd70, 0x440150, 0x4e4a40],
    'originalCD': [0x42a540, 0x42a6b0, 0x42ab10, 0x42a270, 0x42ab70, 0x42a240, 0x42a400, 0x4f1bd0],
}
# 대체하는 CRT·보고 함수의 주소.
STUBS = {
    'originals': {0x4e5f7b: 'toupper', 0x4e66ed: 'strnicmp', 0x4e63e0: 'strchr', 0x4e72c1: 'getenv',
                  0x4e58e9: 'sprintf', 0x4e0620: 'assert', 0x4402f0: 'append'},
    'originalCD': {0x4f24c0: 'toupper', 0x4f3fb0: 'strnicmp', 0x4f1bb0: 'strchr', 0x4f2760: 'getenv',
                   0x4f2830: 'sprintf', 0x490040: 'assert', 0x42acd0: 'append'},
}
# 설정 객체 포인터 배열과 개수의 전역 주소(패치 1000칸, CD 40칸).
GLOBALS = {'originals': (0x557e00, 0x558da0), 'originalCD': (0x5183c8, 0x518468)}
# 가상 메모리 배치. 모두 에뮬레이터 내부 주소다.
OBJECTS = SCRATCH            # 설정 객체(0x20바이트 간격)
NAMES = SCRATCH + 0x400      # 객체 이름("이름.")
SOURCE = SCRATCH + 0x1000    # 치환 입력·키
VALUE = SCRATCH + 0x2800     # 값 쓰기의 값
HEADER = SCRATCH + 0x3000    # 섹션 머리글
NAME_OUT = SCRATCH + 0x3800  # 섹션 이름 출력
ENV_OUT = SCRATCH + 0x4000   # getenv 결과
LENGTH_OUT = SCRATCH + 0x4800  # Section의 길이 출력
OUTPUT = SCRATCH + 0x8000    # 치환 출력
TEXTS = SCRATCH + 0x20000    # 객체별 설정 버퍼
TEXT_SPAN = 0x10000          # 객체 하나의 버퍼 간격
MAX_OBJECTS = 12             # 한 입력의 최대 객체 수
# 깊은 치환에 쓰는 추가 스택(기본 스택 바로 아래).
EXTRA_STACK = 0x2ff00000
# 한 호출의 명령 수 한도. 실제 설정 버퍼(약 19KB)를 여러 번 훑는 조회도 끝나게 한다.
MAX_CONFIG_INSTRUCTIONS = 20000000
# 재현 가능한 입력 시드.
INPUT_SEED = 0x436f6e66
# getenv 대체가 아는 환경 변수(이름은 ASCII 대소문자 무시). C++ 검사도 같은 표를 쓴다.
ENVIRONMENT = {'NSENV': b'env{A}val', 'NSEMPTY': b''}


class ConfigOracle(Oracle):
    """설정 객체 배열을 가상 메모리에 만들고 원본 함수를 실행한다."""
    def __init__(self, edition):
        """PE는 읽기만 하고, 넓은 스택·버퍼·가짜 TEB(FS:[0])를 준비한다."""
        super().__init__(edition, PAIRS, helpers=HELPERS[edition])
        self.stubs = STUBS[edition]
        self.array, self.count = GLOBALS[edition]
        self.mu.mem_map(TEXTS, TEXT_SPAN * MAX_OBJECTS)
        self.mu.mem_map(EXTRA_STACK, 0x100000)
        self.mu.mem_map(0, 0x1000)  # 예외 처리 목록(FS:[0]) 읽기·쓰기를 받는 빈 페이지.
        self.appended = None

    def on_instruction(self, mu, address, size, data):
        """추가 CRT 대체를 처리하고 나머지는 기본 검사(허용 몸체·toupper·strnicmp·assert)에 맡긴다."""
        stub = self.stubs.get(address)
        if stub in ('strnicmp', 'strchr', 'getenv', 'sprintf', 'append'):
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, a, b, c, d, e = struct.unpack('<IIIIII', mu.mem_read(esp, 24))
            if stub == 'strnicmp':
                # 긴 실제 설정 버퍼에서도 비교 길이만큼만 읽는다(기본 대체는 NUL까지 최대 8KB를 읽는다).
                left, right = self.read_bounded(a, c).lower(), self.read_bounded(b, c).lower()
                value = (left > right) - (left < right)
            elif stub == 'strchr':
                text = self.read_string(a)
                index = text.find(bytes([b & 0xff]))
                value = a + index if index >= 0 else 0
            elif stub == 'getenv':
                found = ENVIRONMENT.get(self.read_string(a).decode('ascii').upper())
                if found is None:
                    value = 0
                else:
                    mu.mem_write(ENV_OUT, found + b'\0')
                    value = ENV_OUT
            elif stub == 'sprintf':
                fmt = self.read_string(b)
                if fmt == b'%c%s%c':
                    text = bytes([c & 0xff]) + self.read_string(d) + bytes([e & 0xff])
                elif fmt == b'%c%s':
                    text = bytes([c & 0xff]) + self.read_string(d)
                else:
                    raise RuntimeError(f'대체하지 않은 sprintf 서식: {fmt!r}')
                mu.mem_write(a, text + b'\0')
                value = len(text)
            else:
                # 값 쓰기의 서식·이어 붙이기(cdecl: this, 서식, 키, 값). 인자만 기록한다.
                self.appended = (a, self.read_string(b), self.read_string(c), self.read_string(d))
                value = 0
            mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        super().on_instruction(mu, address, size, data)

    def read_bounded(self, address, count):
        """가상 주소에서 최대 count바이트를 읽되 NUL 앞에서 끊는다."""
        data = bytes(self.mu.mem_read(address, count)) if count else b''
        return data.split(b'\0')[0]

    def read_string(self, address):
        """NUL 종료 문자열을 읽는다. 실제 설정 버퍼(수십 KB)도 읽을 수 있게 조각 단위로 읽는다."""
        result = bytearray()
        # 256바이트씩 읽어 NUL을 찾는다. 상한은 객체 하나의 버퍼 간격이다.
        while len(result) < TEXT_SPAN:
            chunk = bytes(self.mu.mem_read(address + len(result), 256))
            if b'\0' in chunk:
                return bytes(result + chunk.split(b'\0')[0])
            result += chunk
        raise RuntimeError('NUL 종료 없는 oracle 문자열')

    def setup(self, objects):
        """객체 목록 [(이름 또는 None, 버퍼 또는 None, 환경 변수 허용)]을 전역 배열에 올린다."""
        if len(objects) > MAX_OBJECTS:
            raise RuntimeError('oracle 객체 수 초과')
        self.mu.mem_write(OBJECTS, bytes(0x400))
        # 객체마다 원본 0x18바이트 배치를 만든다.
        for index, (name, text, env) in enumerate(objects):
            text_address = 0
            if text is not None:
                if len(text) + 64 > TEXT_SPAN:
                    raise RuntimeError('oracle 버퍼 길이 초과')
                text_address = TEXTS + index * TEXT_SPAN
                # 버퍼 뒤를 0으로 채워, 끝 너머를 읽는 원본 경로가 이전 입력에 영향받지 않게 한다.
                self.mu.mem_write(text_address, text + bytes(64))
            prefix = b'' if name is None else name + b'.'
            self.mu.mem_write(NAMES + index * 0x40, prefix + b'\0')
            self.mu.mem_write(OBJECTS + index * 0x20, struct.pack(
                '<IIIIII', text_address, 0, int(env), NAMES + index * 0x40, len(prefix), 0))
            self.mu.mem_write(self.array + index * 4, struct.pack('<I', OBJECTS + index * 0x20))
        self.mu.mem_write(self.count, struct.pack('<I', len(objects)))

    def call(self, name, args, this=0, purge=0):
        """기본 호출과 같되, 수십 KB의 실제 설정 버퍼를 훑을 수 있게 명령 수 한도를 넓힌다."""
        # 일반 레지스터를 매번 초기화해 이전 호출의 흔적이 결과에 영향을 주지 않게 한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        words = [STOP] + [arg & 0xffffffff for arg in args]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        self.mu.emu_start(self.entries[name], STOP, count=MAX_CONFIG_INSTRUCTIONS)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f'{name}: 제한 내에 반환하지 않음')
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{name}: 스택 복구 크기 불일치')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self, name, args, this=0):
        """원본이 스택 넘침 등으로 끝나는 입력은 None으로 돌려 기대값에서 뺀다."""
        purge = next(pair['purge'] for pair in PAIRS if pair['name'] == name)
        try:
            return self.call(name, args, this, purge)
        except UcError:
            return None

    def substitute(self, objects, this, source):
        """Substitute(this, 출력, 입력)의 출력 문자열."""
        self.setup(objects)
        self.mu.mem_write(SOURCE, source + b'\0')
        self.mu.mem_write(OUTPUT, bytes(0x8000))
        if self.run('Substitute', [OUTPUT, SOURCE], OBJECTS + this * 0x20) is None:
            return None
        return self.read_string(OUTPUT)

    def get(self, objects, this, key):
        """Get(this, 키, 출력)의 반환값과 출력 문자열."""
        self.setup(objects)
        self.mu.mem_write(SOURCE, key + b'\0')
        self.mu.mem_write(OUTPUT, bytes(0x8000))
        found = self.run('Get', [SOURCE, OUTPUT], OBJECTS + this * 0x20)
        if found is None:
            return None
        return found, self.read_string(OUTPUT)

    def remove(self, text, key):
        """RemoveKey(this, 키)의 반환값과 지운 뒤의 버퍼."""
        self.setup([(None, text, False)])
        self.mu.mem_write(SOURCE, key + b'\0')
        found = self.run('RemoveKey', [SOURCE], OBJECTS)
        return found, self.read_string(TEXTS)

    def set(self, objects, this, key, value):
        """Set의 반환값, 고른 대상 객체 번호, 서식에 넘긴 키·값, 대상 버퍼(삭제 뒤)."""
        self.setup(objects)
        self.mu.mem_write(SOURCE, key + b'\0')
        self.mu.mem_write(VALUE, value + b'\0')
        self.appended = None
        existed = self.run('Set', [SOURCE, VALUE], OBJECTS + this * 0x20)
        target_address, fmt, out_key, out_value = self.appended
        if fmt != b'%s = "%s"':
            raise RuntimeError(f'값 쓰기 서식이 다름: {fmt!r}')
        target = (target_address - OBJECTS) // 0x20
        changed = [struct.unpack('<I', self.mu.mem_read(OBJECTS + i * 0x20 + 0x14, 4))[0] for i in range(len(objects))]
        if changed != [int(i == target) for i in range(len(objects))]:
            raise RuntimeError('변경 표시가 대상 객체와 다름')
        after = None if objects[target][1] is None else self.read_string(TEXTS + target * TEXT_SPAN)
        return existed, target, out_key, out_value, after

    def find_section(self, text, header, want_name):
        """FindSection(버퍼, 머리글, 이름 출력)의 다음 줄 위치(없으면 -1)와 이름."""
        self.setup([(None, text, False)])
        self.mu.mem_write(HEADER, header + b'\0')
        self.mu.mem_write(NAME_OUT, bytes(0x800))
        pointer = self.run('FindSection', [TEXTS, HEADER, NAME_OUT if want_name else 0])
        if pointer - TEXTS > len(text):
            return None  # `]` 없는 머리글 등에서 원본이 버퍼 끝 너머를 읽은 입력(정의되지 않은 동작).
        # 원본 호출자처럼 돌려받은 위치의 글자가 NUL이면 찾지 못한 것으로 본다.
        found = self.mu.mem_read(pointer, 1)[0] != 0
        return (pointer - TEXTS if found else -1), (self.read_string(NAME_OUT) if want_name and found else b'')

    def section(self, text, name):
        """Section(this, 이름, 길이 출력)의 본문 위치와 길이. 없으면 (-1, 0)."""
        self.setup([(None, text, False)])
        self.mu.mem_write(HEADER, name + b'\0')
        self.mu.mem_write(LENGTH_OUT, bytes(4))
        pointer = self.run('Section', [HEADER, LENGTH_OUT], OBJECTS)
        if pointer == 0:
            return -1, 0
        return pointer - TEXTS, struct.unpack('<I', self.mu.mem_read(LENGTH_OUT, 4))[0]


def lines(*items):
    """설정 줄을 CRLF로 잇는다(입력 구성용)."""
    return b'\r\n'.join(items) + b'\r\n'


def substitute_inputs():
    """치환·조회 규칙의 경계 입력. (객체 목록, this 번호, 입력)"""
    main = lines(b'A="x"', b'B="{A}y"', b'W="A"', b'S="v  "', b'Q="a//b" // c', b'U=unquoted value  // c',
                 b'quick help="h"', b'E="x`{y`}"', b'D="a`"b"', b'N=""', b'P="{C|{A}}"', b'T="  lead"')
    one = [(None, main, False)]
    cases = []
    sources = [b'{B}', b'{C|def}', b'{C|}', b'{C}', b'{c}', b'{{W}}', b'a`nb`tc`rd`{e`}``f`', b'  x  ', b' ', b'  ',
               b'x \t', b'\t', b'[{S}]', b'{S}', b'{S} ', b'{Q}', b'{U}|', b'{quick help}', b'{QUICK HELP}', b'{E}', b'{D}',
               b'{N}', b'{N|d}', b'{P}', b'{C|a|b}', b'{A', b'{A|x', b'{', b'}', b'{}', b'{|x}', b'{{Z}}', b'{T}', b'x{T}',
               b'{@abc}x', b'{@}', b'{A}{A}{A}', b'{ A }', b'{A }', b'{b}', b'`', b'``', b'`{A`}', b'{A`}', b'{C|`}}',
               b'{C| d }', b'{C|d  }  ', b'-{C|  }-', b'{_NSENV}', b'{_missing|d}', b'']
    cases += [(one, 0, source) for source in sources]
    # 환경 변수 허용 여부에 따른 차이.
    env = [(None, main, True)]
    cases += [(env, 0, source) for source in (b'{_NSENV}', b'{_nsenv}', b'{_NSEMPTY}', b'{_NSEMPTY|d}', b'{_missing|d}', b'{_missing}')]
    # 서명이 붙은 첫 줄은 조회되지 않는다.
    cases.append(([(None, lines(b'mQdsTA="1"', b'B="2"'), False)], 0, b'{A|dead}{B}'))
    # 이름 있는 객체와 등록 순서.
    local = [(None, lines(b'A="main"', b'spec="{DataDir}\\{local.1}.{local.2|x}"', b'DataDir="\\D"', b'1="main1"'), False),
             (b'local', lines(b'1="one"', b'2="two"', b'A="localA"'), False)]
    cases += [(local, 0, source) for source in (b'{local.1}-{LOCAL.2}-{local.3|d}', b'{spec}', b'{1}', b'{A}', b'{local.A}',
                                               b'{local.}', b'{local}', b'{localx.1}', b'{loca.1}')]
    cases += [([(None, b'A=1\r\n', False), (None, b'A=2\r\n', False)], 0, b'{A}'),
              ([(None, b'A=1\r\n', False), (None, b'B=2\r\n', False)], 1, b'{A}{B}'),
              ([(None, b'A=1\r\n', False), (b'local', None, False)], 0, b'{A}{local.1|none}'),
              ([(None, None, False), (None, b'A=1\r\n', False)], 0, b'{A}'),
              ([(None, None, False), (None, b'A=1\r\n', False)], 1, b'{A}'),
              ([(b'm', b'A="named"\r\n', False), (None, b'A="plain"\r\nm.A="dotted"\r\n', False)], 1, b'{m.A}{A}'),
              ([(None, b'A="plain"\r\nm.A="dotted"\r\n', False), (b'm', b'A="named"\r\n', False)], 0, b'{m.A}{A}'),
              ([(None, b'A="plain"\r\nm.A="dotted"\r\n', False), (b'm', b'B="named"\r\n', False)], 0, b'{m.A}{m.B}')]
    rng = random.Random(INPUT_SEED)
    keys = [b'K0', b'K1', b'K2', b'K3', b'K4', b'K5']
    pieces = [b'x', b'y ', b' ', b'`n', b'`{', b'``', b'|', b'//', b'"', b'z', b'  ', b'}', b'.']
    # 시드 고정 임의 입력: 키 i의 값은 더 큰 번호의 키만 참조하므로 대부분 유한하다.
    for _ in range(420):
        objects = []
        # 객체 1~4개. 일부는 이름이 있다.
        for _object in range(rng.randrange(1, 5)):
            name = rng.choice([None, None, b'local', b'm'])
            rows = []
            # 키마다 0~2줄의 정의를 만든다.
            for index, key in enumerate(keys):
                # 같은 키를 0~2번 정의한다.
                for _row in range(rng.choice([0, 1, 1, 2])):
                    value = b''
                    # 값은 조각과 뒤 번호 키의 참조로 이룬다.
                    for _piece in range(rng.randrange(0, 5)):
                        if rng.random() < 0.45 and index + 1 < len(keys):
                            ref = rng.choice(keys[index + 1:])
                            form = rng.randrange(5)
                            value += (b'{' + ref + b'}' if form < 2 else b'{' + ref + b'|d' + rng.choice(pieces) + b'}' if form == 2
                                      else b'{local.' + ref + b'}' if form == 3 else b'{{' + ref + b'}}')
                        else:
                            value += rng.choice(pieces)
                    quote = b'"' if rng.random() < 0.7 else b''
                    rows.append(rng.choice([b'', b' ', b'\t']) + rng.choice([key, key.lower()]) + rng.choice([b'=', b' = '])
                                + quote + value + quote)
            rng.shuffle(rows)
            objects.append((name, lines(*rows) if rows else (None if rng.random() < 0.3 else b''), rng.random() < 0.2))
        source = b''
        # 입력도 조각과 참조를 섞는다.
        for _piece in range(rng.randrange(1, 6)):
            source += (b'{' + rng.choice(keys + [b'local.K1', b'm.K2', b'NOPE', b'_NSENV']) + rng.choice([b'}', b'|d}', b''])
                       if rng.random() < 0.6 else rng.choice(pieces))
        cases.append((objects, rng.randrange(len(objects)), source))
    return cases


def get_inputs():
    """Get의 경계 입력. (객체 목록, this 번호, 키)"""
    main = [(None, lines(b'A="x"', b'B="{A}y "', b'Spec="{D}\\{local.1}.{L}"', b'D="\\D"', b'L="english"'), False),
            (b'local', lines(b'1="tutorial1"'), False)]
    keys = [b'A', b'a', b'B', b'Spec', b'missing', b'', b'local.1', b'LOCAL.1', b'local.2', b'@abc', b'_NSENV', b'A ']
    cases = [(main, 0, key) for key in keys]
    cases += [([(None, None, False), (None, b'A=1\r\n', False)], 0, b'A'),
              ([(None, None, False), (None, b'A=1\r\n', False)], 1, b'A'),
              ([(None, b'A="x"\r\nE="{_NSENV}"\r\n', True)], 0, b'E'), ([(None, b'A="x"\r\nE="{_NSENV}"\r\n', False)], 0, b'E'),
              ([(None, b'A="x"\r\nE="{_NSENV}"\r\n', True)], 0, b'_NSENV'), ([(None, b'A="x"\r\nE="{_NSENV}"\r\n', True)], 0, b'_nsenv')]
    return cases


def text_inputs():
    """줄 삭제·섹션 검색에 쓰는 설정 버퍼. 손으로 고른 것과 시드 고정 임의 버퍼."""
    texts = [b'A="1"\r\nB="2"\r\nA="3"\r\n', b'  A=1\r\n\r\n   B=2\r\n', b'A=1', b'A=1\nA=2\nA=3', b'B=1\r\n', b'',
             b'[one]\r\nA=1\r\n[two]\r\nB=2\r\n[END]\r\n', b'[one]\r\n[two]\r\nB=2\r\n', b'x [one]\r\nA=1\r\n',
             b'  [one] tail\r\nA=1\r\n', b'[a][one]\r\nA=1\r\n', b'[one]', b'[one]\r\n', b'[one]\n\rA=1\r\n',
             b'[one\r\nA]=1\r\n[two]\r\n', b'[ONE]\r\nA=[x]\r\n[two]\r\nB\r[three]\r\nC\r\n']
    rng = random.Random(INPUT_SEED + 1)
    alphabet = ['A', 'B', 'a', '=', ' ', '\t', '\r\n', '\n', '\r', '[', ']', 'one', 'two', '"', '1']
    # 구분 문자가 자주 나오는 짧은 버퍼를 만든다.
    for _ in range(120):
        texts.append(''.join(rng.choice(alphabet) for _ in range(rng.randrange(1, 40))).encode('ascii'))
    return texts


def set_inputs():
    """값 쓰기의 대상 객체·키 선택 입력. (객체 목록, this 번호, 키, 값)"""
    stacks = [
        [(None, lines(b'A="1"', b'm.A="2"'), False)],
        [(None, lines(b'A="1"', b'B="2"', b'A="3"'), False), (b'local', lines(b'1="one"', b'A="la"'), False)],
        [(b'local', lines(b'1="one"'), False), (None, lines(b'A="1"', b'1="main1"'), False)],
        [(None, lines(b'A="1"'), False), (b'm', None, False), (b'local', lines(b'A="x"'), False)],
        [(b'm', lines(b'A="1"'), False), (b'local', lines(b'1="x"'), False)],
        [(None, None, False)],
    ]
    keys = [b'A', b'a', b'B', b'New', b'local.1', b'LOCAL.1', b'local.A', b'm.A', b'x.y', b'a.b.c', b'.', b'A.', b'.A']
    cases = []
    # 모든 객체 구성 × this × 키 조합.
    for stack in stacks:
        # this 로 쓸 객체를 바꿔 가며 만든다.
        for this in range(len(stack)):
            # 키마다 입력 하나.
            for key in keys:
                cases.append((stack, this, key, b'value'))
    return cases


def real_configuration(edition):
    """실제 설정 파일을 시작 순서대로 이은 버퍼를 입력으로 만든다(결과 모델이 아니라 입력 구성이다)."""
    key = b'mydoghasfleas'
    parts = [b'[ARGS]', b'MAJOR_VERSION=10\r\nMINOR_VERSION=78\r\nversion="v10.78"']
    names = ['options.cfg', 'setup.cfg'] if edition == 'originals' else ['setup.cfg']
    # 인코딩된 파일은 서명 `mQdsT`를 포함한 채로 섹션 줄 뒤에 붙는다.
    for name in names:
        raw = (ROOT / edition / ('d' if edition == 'originals' else 'D') / name).read_bytes()
        plain = bytes(b ^ key[i % len(key)] for i, b in enumerate(raw))
        parts += [b'[' + name.encode('ascii') + b']', plain.rstrip(b'\r\n')]
    parts += [b'[END]', b'InstallDir = "C:\\NS"', b'CDDir = "C:\\NS"']
    return lines(*parts)


def real_keys(text):
    """실제 버퍼에서 값에 치환 괄호가 있는 키를 고른다."""
    keys = []
    # 줄마다 `키 = 값` 꼴이고 값에 `{`가 있으면 대상으로 삼는다.
    for line in text.split(b'\n'):
        line = line.strip()
        if b'=' in line and not line.startswith(b'//') and b'{' in line.split(b'=', 1)[1]:
            name = line.split(b'=', 1)[0].strip()
            if name and name not in keys and all(32 <= c < 127 for c in name):
                keys.append(name)
    return keys


def encode(value):
    """TSV 한 칸: 빈 문자열은 '-', 없음(None)은 '~', 나머지는 16진수."""
    if value is None:
        return '~'
    return value.hex() or '-'


def encode_objects(objects, defined):
    """객체 목록을 TSV 칸으로 만든다. 긴 버퍼는 Def 줄의 번호(@번호)로 가리킨다."""
    cells = [str(len(objects))]
    # 객체마다 이름·버퍼·환경 변수 허용을 적는다.
    for name, text, env in objects:
        cells += [encode(name), ('@%d' % defined[text]) if text in defined else encode(text), str(int(env))]
    return cells


def main():
    """두 판본의 기계어 결과가 같을 때만 기대값과 실행 근거를 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'cpppj/tests/fixtures/config-x86.tsv')
    parser.add_argument('--report', type=Path, default=ROOT / 'cpppj/recovery-config-evidence.json')
    args = parser.parse_args()
    patch, cd = ConfigOracle('originals'), ConfigOracle('originalCD')
    rows = ['# 원본 x86 기계어에서 생성. 두 판본 동일 결과만 기록. 칸: \'-\' 빈 문자열, \'~\' 없음, \'@n\' Def 줄의 버퍼.']
    counts, skipped, external, undefined = {}, 0, 0, 0
    defined = {}

    def both(method, *values):
        """두 판본에서 같은 입력을 실행하고 결과가 같은지 확인한다."""
        a, b = getattr(patch, method)(*values), getattr(cd, method)(*values)
        if a != b:
            raise RuntimeError(f'두 판본 불일치: {method} {values!r}: {a!r} / {b!r}')
        return a

    def add(kind, cells):
        """기대값 한 줄을 더한다."""
        rows.append('\t'.join([kind, *cells]))
        counts[kind] = counts.get(kind, 0) + 1

    # 실제 설정 버퍼는 한 번만 적고 번호로 가리킨다.
    real = {edition: real_configuration(edition) for edition in ('originals', 'originalCD')}
    # 버퍼마다 Def 줄 하나를 적는다.
    for index, text in enumerate(real.values()):
        defined[text] = index
        rows.append('\t'.join(['Def', str(index), text.hex()]))
    # 치환.
    for objects, this, source in substitute_inputs():
        result = both('substitute', objects, this, source)
        if result is None:
            skipped += 1
            continue
        add('Subst', [str(this), *encode_objects(objects, defined), encode(source), encode(result)])
    # 최상위 조회.
    for objects, this, key in get_inputs():
        result = both('get', objects, this, key)
        if result is None:
            raise RuntimeError(f'Get 입력이 끝나지 않음: {key!r}')
        found, result = result
        add('Get', [str(this), *encode_objects(objects, defined), encode(key), str(found), encode(result)])
    # 실제 설정에서 경로 지정값 등 치환이 있는 모든 키를 local.1~3과 함께 조회한다.
    local = (b'local', lines(b'1="TEST01"', b'2="2"', b'3="three"'), False)
    # 판본의 실제 버퍼마다 조회한다.
    for edition, text in real.items():
        objects = [(None, b'', False), (None, text, True), local]
        # 값에 치환 괄호가 있는 키마다.
        for key in real_keys(text):
            if any(marker in ConfigRawValue(text, key) for marker in (b'{@', b'{&')):
                continue  # 미션 파일·레지스트리를 읽는 경로는 OS·파일 접근이라 실행하지 않는다.
            try:
                result = both('get', objects, 1, key)
            except RuntimeError as error:
                if '허용하지 않은 실행 주소' not in str(error):
                    raise
                external += 1  # 다른 키를 거쳐 파일·레지스트리 경로로 들어간 입력.
                continue
            if result is None:
                skipped += 1
                continue
            add('Get', ['1', *encode_objects(objects, defined), encode(key), str(result[0]), encode(result[1])])
    texts = text_inputs()
    # 줄 삭제.
    for text in texts:
        # 두 키로 각각 지워 본다.
        for key in (b'A', b'B'):
            found, after = both('remove', text, key)
            add('Remove', [encode(text), encode(key), str(found), encode(after)])
    # 값 쓰기.
    for objects, this, key, value in set_inputs():
        existed, target, out_key, out_value, after = both('set', objects, this, key, value)
        add('Set', [str(this), *encode_objects(objects, defined), encode(key), encode(value),
                    str(existed), str(target), encode(out_key), encode(out_value), encode(after)])
    # 섹션 검색.
    for text in texts:
        # 머리글 꼴마다 찾는다.
        for header in (b'[one]', b'[one', b'[', b'[two]'):
            # 이름 출력을 받지 않는 경우와 받는 경우.
            for want_name in (False, True):
                result = both('find_section', text, header, want_name)
                if result is None:
                    undefined += 1
                    continue
                offset, name = result
                add('FindSection', [encode(text), encode(header), str(int(want_name)), str(offset), encode(name)])
        # 섹션 이름 꼴마다 본문 범위를 구한다.
        for name in (b'one', b'[two', b'END'):
            offset, length = both('section', text, name)
            add('Section', [encode(text), encode(name), str(offset), str(length)])
    payload = '\n'.join(rows) + '\n'
    report = {'schema': 1, 'method': 'Unicorn x86 32-bit; OS/API execution forbidden',
              'unicorn_version': importlib.metadata.version('unicorn'),
              'binary_sha256': {'originals': patch.sha256, 'originalCD': cd.sha256},
              'functions': PAIRS, 'helpers': {key: ['%08x' % address for address in value] for key, value in HELPERS.items()},
              'seed': INPUT_SEED, 'cases': counts, 'total_cases': sum(counts.values()),
              'skipped_nonterminating': skipped, 'skipped_external_access': external,
              'skipped_reads_past_buffer': undefined,
              'stubbed': ['ASCII toupper', 'ASCII strnicmp', 'strchr', 'getenv (fixed table)',
                          'sprintf (%c%s%c, %c%s)', 'assert reporting', 'formatted append 004402f0/0042acd0 (arguments recorded)'],
              'assert_report_calls': {'originals': patch.assertions, 'originalCD': cd.assertions},
              'fixture_sha256': hashlib.sha256(payload.encode('utf-8')).hexdigest(),
              'limitations': ['게임 전체 실행 검증 아님', 'CRT는 ASCII 로케일만 검증',
                              '{@미션.키}·{&레지스트리}는 파일·OS 접근이라 x86으로 실행하지 않음',
                              '값 쓰기의 서식·이어 붙이기는 인자만 기록(원본 256바이트 서식 버퍼 미검증)',
                              '원본이 끝나지 않는 자기 참조 입력은 기대값에서 제외',
                              'C++ 검증은 ctest에서 별도로 실행']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(payload, encoding='utf-8', newline='\n')
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'두 판본 x86 동일 결과 {sum(counts.values())}개(제외: 끝나지 않음 {skipped}, 외부 접근 {external}, '
          f'버퍼 밖 읽기 {undefined}) → {args.output}')


def ConfigRawValue(text, key):
    """입력 선별용: 버퍼에서 `키 =` 로 시작하는 첫 줄의 나머지를 돌려준다."""
    # 줄을 훑어 첫 일치를 찾는다.
    for line in text.split(b'\n'):
        stripped = line.strip()
        if stripped.lower().startswith(key.lower()) and stripped[len(key):].lstrip().startswith(b'='):
            return stripped
    return b''


if __name__ == '__main__':
    main()
