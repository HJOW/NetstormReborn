#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 TypeNames/Deck writer의 실제 PE 실행을 새 독립 TSV로 보존한다.

Create/State/GetType/Refresh/GetDeck/헤더 Write/assert는 명시 외부 경계다.
이름 해시의 signed BYTE 계산과 인라인 DWORD/BYTE 쓰기는 실제 원문이 수행한다.
"""

import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EBP,
    UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW,
)


ROOT = Path(__file__).resolve().parents[1]  # tools의 상위 저장소 루트다.
PE_PATH = ROOT / 'originals/Netstorm.exe'  # 수정하지 않는 보호 PE다.
PE_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 확정한10.78 입력 식별자다.
FIXTURE_PATH = ROOT / 'cpppj/tests/fixtures/fort-snapshot-basics-x86.tsv'  # 새 독립 원문 TSV다.
REPORT_PATH = ROOT / 'extracted/cpp-fort-snapshot-basics-20261023-native.json'  # 명시 실행 범위/경계 보고서다.
IMAGE_BASE = 0x400000  # 원래 PE 이미지 기준 주소다.
ENTRIES = {'names': 0x4BD6F0, 'deck': 0x4BEFD0}  # 실제 두 writer 함수의 첫 명령이다.
NATIVE_RANGES = ((0x4BD6F0, 0x4BD7FC), (0x4BEFD0, 0x4BF188))  # 두 RET까지의 실제 범위다.
HEADERS = 0x5C89F0  # this는 이 기준 주소에서20BYTE 슬롯 간격이다.
MINE = 0x540CAC  # TypeNames의 isMine 비교가 현재 읽는 전역이다.
TYPE_COUNT = 0x541348  # 이름 개수 header/각 루프 경계에서 다시 읽는 signed DWORD다.
HEAP = 0x1000000  # 외부 장치/출력의 격리 메모리다.
NODE = HEAP + 0x1000  # 외부 Create가 반환하는16BYTE MemStream 헤더다.
PAYLOAD = HEAP + 0x2000  # 실제 원문이 쓰는 물리16KB 출력 버퍼다.
PHYSICAL_CAPACITY = 0x4000  # 논리 alloc 진단 후 정상 복귀도 관측할 수 있는 물리 출력 크기다.
DECK = HEAP + 0x7000  # 외부 GetDeck이 반환하는 원본352BYTE다.
DECK_BYTES = 0x160  # Deck의 전체 실제 크기다.
DECK_COUNT = DECK + 0x14C  # 실제 개수DWORD의 위치다.
DECK_ENTRIES = 66  # 실제5BYTE 항목의 물리 최대 개수다.
SLOTS = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15)  # 비소유자0·모든유효owner1..8·상한밖9/15를독립검사한다.
STATE = HEAP + 0x7800  # StateGet의 실제BYTE17을 공급하는 격리 저장소다.
TYPE_BASE = HEAP + 0x8000  # 원문 GetType 반환 레코드 여섯 개의 시작이다.
TYPE_STRIDE = 0x100  # +4 이름과 NUL 뒤 원문까지 분리하는 물리 간격이다.
NAMES = (b'', b'A', b'abcdE', b'\x80\xffAz', b'Netstorm', b'Q\0ignored')  # signed BYTE/순환 shift/NUL을 검증하는 명시 이름 원문이다.
STACK = 0x2100000  # 함수 진입 ESP와 격리 스택 기준이다.
RETURN = HEAP + 0xF000  # 함수 RET의 격리 종료점이다.
MAX_INSTRUCTIONS = 100000  # 현재typeCount가 무한 증가하는 입력은 허용하지 않는다.
CONTROLS = (0x27F, 0x37F)  # 두 x87 제어어로 각각 실제 실행한다.
BOUNDARIES = {0x44D1B0: 'create', 0x4BD550: 'state', 0x49A840: 'name',
              0x44F7C0: 'refresh', 0x44F700: 'get', 0x44CCE0: 'write', 0x4E0620: 'assert'}  # 이 하위 몸체만 명시 장치로 대체한다.
SCHEMA = (
    'name', 'kind', 'slot', 'manager', 'mine', 'version', 'typecount', 'deckcount',
    'position', 'end', 'capacity', 'mutation',
    'completed', 'managerout', 'mineout', 'versionout', 'typecountout', 'deckcountout',
    'current', 'positionout', 'endout', 'payload', 'trace', 'stackdelta',
)  # 앞12열은 기대 없는 독립 입력이며 뒤12열은 실제 실행 관측이다.


def sha256(path):
    """보호 입력/새 산출물 식별자를 읽기 전용으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """원래 PE 함수를 실행하고 현재 전역/실제 출력 바이트를 관측한다."""

    def __init__(self):
        """보호 PE와 분리된 입력/스택/출력 메모리를 준비한다."""
        self.pe = pefile.PE(str(PE_PATH))  # 읽기 전용 PE 입력이다.
        if self.pe.OPTIONAL_HEADER.ImageBase != IMAGE_BASE:
            raise RuntimeError('보호 PE 이미지 기준 주소 불일치')
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 원본32비트 명령 실행기다.
        self.image_size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095  # 페이지 정렬한 원문 크기다.
        self.mu.mem_map(IMAGE_BASE, self.image_size)
        self.mu.mem_write(IMAGE_BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x10000)
        self.mu.mem_map(STACK - 0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.memory_write)
        self.original = [(first, bytes(self.mu.mem_read(first, end - first))) for first, end in NATIVE_RANGES]  # 원문 바이트 불변 확인용이다.
        self.boundary_totals = Counter()  # 명시 경계의 실제 호출 합계다.
        self.native_write_totals = Counter()  # CPU의 원문 쓰기 합계다.
        self.precisions = {}  # 제어어별 실제 실행/명령 합계다.

    def dword(self, address, value=None):
        """비정렬 little endian DWORD를 읽고 명시 입력/장치 효과만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def c_string(self, address):
        """원본 C문자열의 첫 NUL 전까지 읽는다."""
        result = bytearray()  # 현재 관측한 문자열 바이트다.
        for offset in range(256):  # 보호 리터럴/명시 이름의 물리 한도를 지킨다.
            byte = self.mu.mem_read(address + offset, 1)[0]  # 현재 원문BYTE다.
            if not byte:
                return bytes(result)
            result.append(byte)
        raise RuntimeError('원문 문자열 NUL/범위 누락')

    def returned(self, value=0, cleanup=0):
        """명시 외부 호출의 반환주소와 thiscall 인자만 소비한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 실제 외부 CALL 진입 ESP다.
        target = self.dword(esp)  # CPU가 저장한 원문 반환주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def mutate(self, boundary):
        """명시된 callback 변경만 현재 저장소에 공급한다."""
        mutation = self.case['mutation']  # 결과를 포함하지 않는 독립 효과 선택값이다.
        prefix = boundary + '-'  # 현재 경계에서만 적용할 이름 접두사다.
        if mutation == prefix + 'manager22':
            self.dword(self.this, 22)
        elif mutation == prefix + 'mine8':
            self.dword(MINE, 8)
        elif mutation == prefix + 'version0':
            self.mu.mem_write(STATE + 17, b'\0')
        elif mutation == prefix + 'version1':
            self.mu.mem_write(STATE + 17, b'\1')
        elif mutation == prefix + 'typecount2':
            self.dword(TYPE_COUNT, 2)
        elif mutation == prefix + 'typecount0':
            self.dword(TYPE_COUNT, 0)
        elif mutation == prefix + 'deckcount2':
            self.dword(DECK_COUNT, 2)
        elif mutation == prefix + 'deckcount0':
            self.dword(DECK_COUNT, 0)
        elif mutation == prefix + 'capacity64':
            self.dword(NODE + 12, 64)

    def memory_write(self, mu, access, address, size, value, data):
        """CPU 원문 쓰기를 반환 스택·선택stream·물리 출력 범위로 제한한다."""
        if STACK - 0x100 <= address and address + size <= STACK + 4:
            category = 'stack'
        elif NODE <= address and address + size <= NODE + 16:
            category = 'stream'
        elif PAYLOAD <= address and address + size <= PAYLOAD + PHYSICAL_CAPACITY:
            category = 'payload'
        else:
            raise RuntimeError(f'원문 예상 밖 메모리 쓰기 {address:08x}/{size}')
        self.native_write_totals[category] += 1

    def instruction(self, mu, address, size, data):
        """실제 두 함수의 명령을 실행하고 명시 장치/끝점만 가로챈다."""
        self.instructions += 1
        if address == RETURN:
            self.completed = True
            mu.emu_stop()
            return
        if any(first <= address < end for first, end in NATIVE_RANGES):
            self.native_instructions += 1
            return
        if address not in BOUNDARIES:
            raise RuntimeError(f'격리 범위 밖의 미명시 코드 {address:08x}')
        boundary = BOUNDARIES[address]  # 실제 진입한 외부 장치 종류다.
        self.boundary_totals[boundary] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)  # 원문 실제 인자 스택이다.
        if boundary == 'assert':
            line = self.dword(esp + 12)  # assert의 원본 소스행 번호다.
            self.trace.append(f'assert:{line}')
            self.mutate('assert')
            if line == 0x24 and self.case['mutation'] == 'assert-stop':
                self.stopped_assert = True
                mu.emu_stop()
                return
            self.returned()
        elif boundary == 'create':
            manager = mu.reg_read(UC_X86_REG_ECX)  # 원문이 그 시점에 다시 읽은 *this다.
            section = self.c_string(self.dword(esp + 4)).decode('ascii')  # 실제 PE 섹션 이름 리터럴이다.
            self.trace.append(f'create:{manager}:{section}')
            self.current = 'Created'
            self.mutate('create')
            self.returned(NODE, cleanup=4)
        elif boundary == 'state':
            if mu.reg_read(UC_X86_REG_ECX) != self.this:
                raise RuntimeError('StateGet의 실제 Template this 변경')
            self.trace.append('state:' + str(self.case['slot']))
            self.current = 'State'
            self.mutate('state')
            self.returned(STATE)
        elif boundary == 'name':
            index = self.dword(esp + 4)  # 원문 loop가 현재 전달한 TYPE 번호다.
            self.trace.append('name:' + str(index))
            self.mutate('name' + str(index))
            self.returned(TYPE_BASE + (index % len(NAMES)) * TYPE_STRIDE)
        elif boundary in ('refresh', 'get'):
            owner = self.dword(esp + 4)  # 실제 slot 계산/clamp의 반환 인자다.
            self.trace.append(boundary + ':' + str(owner))
            self.mutate(boundary)
            self.returned(DECK if boundary == 'get' else 0)
        elif boundary == 'write':
            if mu.reg_read(UC_X86_REG_ECX) != NODE:
                raise RuntimeError('보관한 실제 stream 대신 current를 사용함')
            source = self.dword(esp + 4)  # stack의 실제 headerBYTE 주소다.
            length = self.dword(esp + 8)  # 이 두 함수의 경계 호출은1BYTE뿐이다.
            if length != 1:
                raise RuntimeError('미명시 헤더 외부 Write 길이')
            raw = bytes(mu.mem_read(source, length))  # 원문이 준비한 실제 낮은BYTE다.
            self.trace.append('write:1:' + raw.hex())
            position = self.dword(NODE + 4)  # 외부 장치가 읽는 현재 pos다.
            if position + length > self.dword(NODE + 12):
                raise RuntimeError('입력 계약 밖의 헤더 Write capacity 진단')
            mu.mem_write(PAYLOAD + position, raw)
            self.dword(NODE + 4, position + length)
            self.dword(NODE + 8, max(self.dword(NODE + 8), position + length))
            self.mutate('write')
            self.returned(cleanup=8)

    def run(self, case, control):
        """입력 하나의 전체 출력/부분쓰기/장치 호출 순서를 실제 PE에서 관측한다."""
        self.case = case  # 명시 외부 입력만 가진 기록이다.
        self.this = HEADERS + case['slot'] * 20  # 같은 실제 주소 차이로 slot을 계산하게 한다.
        self.trace = []  # 실제 장치 진입 시점만 순서대로 보존한다.
        self.current = 'old'  # 아직 Create/State가 선택하지 않은 현재 섹션이다.
        self.completed = False  # 실제 RET까지 도달해야 정상 완료다.
        self.stopped_assert = False  # 진단 중단은 정상 함수 복귀와 구별한다.
        self.instructions = 0  # 전체 hook 이벤트 수다.
        self.native_instructions = 0  # 실제 보호 PE 본체 명령 수다.
        self.mu.mem_write(STACK - 0x100, bytes([0xCC]) * 0x200)
        self.mu.mem_write(PAYLOAD, bytes([0xCC]) * PHYSICAL_CAPACITY)
        self.mu.mem_write(NODE, struct.pack('<IIII', PAYLOAD, case['position'], case['end'], case['capacity']))
        self.mu.mem_write(STATE, bytes([0xA5]) * 32)
        self.mu.mem_write(STATE + 17, bytes([case['version']]))
        raw_deck = bytearray([0xA5] * DECK_BYTES)  # 개수DWORD 외의 기존 raw 값도 별도로 공급한다.
        for index in range(DECK_ENTRIES):  # 실제5BYTE stride의 모든66항목을 채운다.
            for byte in range(5):  # writer가 건너뛰어야 하는 다섯째BYTE도 다른 값으로 만든다.
                raw_deck[index * 5 + byte] = (index * 19 + byte * 37 + 3) & 255
        self.mu.mem_write(DECK, bytes(raw_deck))
        self.dword(DECK_COUNT, case['deckcount'])
        self.dword(TYPE_COUNT, case['typecount'])
        self.dword(MINE, case['mine'])
        self.dword(self.this, case['manager'])
        for index, name in enumerate(NAMES):  # 이름 뒤NUL과 숨은 원문까지 외부 장치 입력으로 공급한다.
            self.mu.mem_write(TYPE_BASE + index * TYPE_STRIDE, bytes(4) + name + b'\0')
        self.dword(STACK, RETURN)
        registers = {
            UC_X86_REG_EAX: 0x10111213, UC_X86_REG_EBX: 0x20212223,
            UC_X86_REG_ECX: self.this, UC_X86_REG_EDX: 0x30313233,
            UC_X86_REG_ESI: 0x40414243, UC_X86_REG_EDI: 0x50515253,
            UC_X86_REG_EBP: 0x60616263, UC_X86_REG_ESP: STACK,
            UC_X86_REG_FPCW: control, UC_X86_REG_FPSW: 0,
        }  # 온전한 함수 진입의 명시 레지스터다.
        for register, value in registers.items():  # 이전 실행의 잔여 레지스터를 분리한다.
            self.mu.reg_write(register, value)
        self.mu.emu_start(ENTRIES[case['kind']], RETURN + 1, count=MAX_INSTRUCTIONS)
        stackdelta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - 4  # 정상 RET의 반환주소4BYTE를 정규화한다.
        if not self.completed and not self.stopped_assert:
            raise RuntimeError('정상 RET 또는 명시 진단 중단에 도달하지 않음')
        if self.completed and stackdelta:
            raise RuntimeError('정상 원문 함수의 스택 정리 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('원문 x87 제어어/스택 변경')
        for first, original in self.original:  # 실행한 원문 명령 바이트를 재확인한다.
            if bytes(self.mu.mem_read(first, len(original))) != original:
                raise RuntimeError('보호 PE 본체 바이트 변경')
        position = self.dword(NODE + 4)  # 최종 또는 진단 중단 시점의 현재 커서다.
        end = self.dword(NODE + 8)  # 비영 초기end도 원문 max 규칙으로 보존된다.
        if end > PHYSICAL_CAPACITY or position > PHYSICAL_CAPACITY:
            raise RuntimeError('원문 결과의 실제 물리 출력 범위 초과')
        totals = self.precisions.setdefault(f'{control:03x}', {'runs': 0, 'native_instructions': 0, 'hook_events': 0})  # 정밀도별 현재 합계다.
        totals['runs'] += 1
        totals['native_instructions'] += self.native_instructions
        totals['hook_events'] += self.instructions
        return {
            'completed': int(self.completed), 'managerout': self.dword(self.this),
            'mineout': self.dword(MINE), 'versionout': self.mu.mem_read(STATE + 17, 1)[0],
            'typecountout': self.dword(TYPE_COUNT), 'deckcountout': self.dword(DECK_COUNT),
            'current': self.current, 'positionout': position, 'endout': end,
            'payload': bytes(self.mu.mem_read(PAYLOAD, end)).hex(),
            'trace': ';'.join(self.trace), 'stackdelta': stackdelta,
        }


def cases():
    """원본 관문/슬롯 경계/입력byte와 callback 변경을 기대 없이 교차한다."""
    result = []  # 새 독립 입력 목록이다.
    base = dict(kind='names', slot=1, manager=11, mine=1, version=1, typecount=6,
                deckcount=3, position=0, end=0, capacity=4096, mutation='fixed')  # 정상 소규모 입력이다.
    for index, values in enumerate(itertools.product(SLOTS, (0, 11), (0, 1), (-1, 0, 1, 6, 9), ((0, 0), (3, 9), (7, 2)))):  # isMine/버전/개수/초기cursor를 교차한다.
        slot, manager, version, count, cursor = values  # 제품 결과를 포함하지 않는 조합이다.
        result.append(dict(base, name=f'names-{index:04d}', slot=slot, manager=manager, mine=slot,
                           version=version, typecount=count, position=cursor[0], end=cursor[1]))
    for index, values in enumerate(itertools.product(SLOTS, (0, 11), (-1, 0, 1, 3, 66), ((0, 0), (3, 9), (7, 2)))):  # Deck 슬롯clamp와signed 개수/물리 상한을 교차한다.
        slot, manager, count, cursor = values  # 실제5BYTE입력은 각 실행에서 같은 명시 식으로 공급한다.
        result.append(dict(base, name=f'deck-{index:04d}', kind='deck', slot=slot, manager=manager,
                           mine=slot, deckcount=count, position=cursor[0], end=cursor[1]))
    for index, count in enumerate((127, 188, 255, 256)):  # 이름 개수BYTE 감김과signed hash를 긴 루프에서도 관측한다.
        result.append(dict(base, name=f'large-names-{index:02d}', typecount=count))
    for index, mutation in enumerate(('create-manager22', 'create-mine8', 'create-version0', 'create-typecount2',
                                     'state-manager22', 'state-mine8', 'state-version0', 'state-version1',
                                     'state-typecount2', 'write-typecount2', 'write-typecount0',
                                     'name0-typecount2', 'name0-typecount0', 'name0-manager22')):  # State/GetType 효과 뒤 원문 현재값 재조회를 관측한다.
        result.append(dict(base, name=f'names-callback-{index:02d}', mutation=mutation, version=0 if mutation == 'state-version1' else 1))
    for index, mutation in enumerate(('refresh-manager22', 'refresh-mine8', 'refresh-deckcount2',
                                     'get-manager22', 'get-deckcount2', 'create-manager22', 'create-deckcount2',
                                     'write-deckcount2', 'write-deckcount0')):  # Refresh/Get/Create/헤더Write 뒤의현재manager/count를 확인한다.
        result.append(dict(base, name=f'deck-callback-{index:02d}', kind='deck', mutation=mutation))
    for index, values in enumerate(itertools.product(('names', 'deck'), ('assert-capacity64', 'assert-manager22', 'assert-stop', 'assert-typecount0', 'assert-deckcount2', 'assert-deckcount0'))):  # 진단 복귀 뒤 현재 필드와중단시부분쓰기를 관측한다.
        kind, mutation = values  # 두 writer의 첫 인라인capacity진단은 서로 다른부분쓰기뒤에 나타난다.
        result.append(dict(base, name=f'assert-{index:02d}', kind=kind, capacity=2, mutation=mutation))
    result.append(dict(base, name='mine-mismatch', mine=8))
    return result


def main():
    """허용한 새 TSV/보고서에 두 정밀도 실행 결과를 기록한다."""
    before = sha256(PE_PATH)  # 보호 원문 디스크 식별자다.
    if before != PE_SHA256:
        raise RuntimeError('확정한 보호 PE SHA256 불일치')
    oracle = Oracle()  # 실제 두 PE 함수를 실행하는 격리 장치다.
    inputs = cases()  # 독립 입력 목록이다.
    rows = []  # 두 정밀도별 평문 관측 행이다.
    for case in inputs:  # 같은 입력을 두 제어어에서 별도로 실행한다.
        first_observation = None  # 첫 실제 실행과 둘째 실행의 비교용이다.
        for control in CONTROLS:  # 원문 결과를 합치지 않고 각각 보존한다.
            output = oracle.run(case, control)  # 제품 코드/기존 fixture를 호출하지 않은 관측이다.
            if first_observation is not None and output != first_observation:
                raise RuntimeError('정밀도별 관측 불일치: ' + case['name'])
            first_observation = output
            values = dict(case, name=f'{case["name"]}-cw{control:03x}', **output)  # 입력과 원문 출력의 명시 합성이다.
            rows.append('\t'.join(str(values[column]) for column in SCHEMA))
    if sha256(PE_PATH) != before:
        raise RuntimeError('보호 PE 디스크 SHA256 변경')
    header = '# TypeNames/Deck writer의 실제 PE 관측. 앞12열 입력/뒤12열 관측. 하위장치만 명시경계이며 signed 이름hash/인라인쓰기는원문실행이다.\n#' + '\t'.join(SCHEMA) + '\n'  # 재사용할평문스키마다.
    FIXTURE_PATH.write_text(header + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    report = {
        'host': socket.gethostname(), 'date': '2026-10-23', 'inputs': len(inputs), 'precision_runs': len(rows),
        'precision_differences': 0, 'precision_observations': oracle.precisions,
        'schema': list(SCHEMA), 'input_columns': list(SCHEMA[:12]), 'observation_columns': list(SCHEMA[12:]),
        'entries': {kind: f'{address:08x}' for kind, address in ENTRIES.items()}, 'return': f'{RETURN:08x}',
        'entry_contract': 'ECX=005c89f0+slot*20。*this=manager。00540cac=mine。출력MemStream의초기position/end/capacity는외부Create의명시반환입력이다.',
        'names': [name.hex() for name in NAMES], 'name_device': 'GetType(index)는index%6의레코드를반환하고이름은+4다. 첫NUL까지만실제해시루프가읽으며해시는대체계산하지않는다.',
        'deck_device': '352BYTE중앞66개5BYTE항목은raw[i*5+j]=(i*19+j*37+3)&255。countDWORD는14c다. Refresh/Get의결과는명시입력이고몸체가첫4BYTE만순서대로쓴다.',
        'payload_device': '물리16384BYTE를cc로채우고현재end까지hex로출력한다. 헤더Write는원문의실제stackBYTE를복사하고외부장치에서pos/end를진행한다.',
        'current_device': '초기old。Create후Created。StateGet후State。현재선택이바뀌어도원문은보관한stream포인터로쓴다.',
        'boundaries': {f'{address:08x}': name for address, name in BOUNDARIES.items()}, 'boundary_calls': dict(oracle.boundary_totals),
        'native_write_counts': dict(oracle.native_write_totals),
        'native_ranges': [{'first': f'{first:08x}', 'end_exclusive': f'{end:08x}', 'sha256': hashlib.sha256(bytes(oracle.mu.mem_read(first, end - first))).hexdigest()} for first, end in NATIVE_RANGES],
        'mutations': 'create/state/refresh/get/write/name0/assert에서manager22,mine8,version0/1,typecount0/2,deckcount0/2,capacity64를명시공급한다.',
        'partial_failure': 'assert-stop은첫인라인capacity진단004e0620진입에서중단한다. completed0이며호출스택은아직복귀하지않았으므로stackdelta는음수다. 이는원본예외unwind관측이아니다.',
        'scope': '실제원문이Template진단/ownerclamp/State+17관문/개수BYTE/현재count재조회/이름hash/5BYTEstride중4BYTE직렬화/pos,end부분쓰기를수행한다.',
        'limits': ['Create/StateGet/타입조회/생산Refresh/GetDeck/헤더MemStream/assert하위몸체는명시외부경계다.',
                   '전체004bf9a0snapshot/실제파일장치/Editor월드/GUI/게임실행과원본SEHunwind검증이아니다.',
                   '물리출력은16384BYTE로제한하며논리capacity진단이복귀하는경우에도물리범위밖쓰기는허용하지않는다.'],
        'new_game_runs': 0, 'new_decompilations': 0,
        'sha256': {str(path.relative_to(ROOT)).replace('\\', '/'): sha256(path) for path in (PE_PATH, Path(__file__), FIXTURE_PATH)},
    }  # 관측과하위장치의미복원범위를분리한보고서다.
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'native inputs={len(inputs)} precision_runs={len(rows)} differences=0')
    print('schema=' + '\t'.join(SCHEMA))
    print('fixture=' + str(FIXTURE_PATH.relative_to(ROOT)))
    print('report=' + str(REPORT_PATH.relative_to(ROOT)))


if __name__ == '__main__':  # 직접실행할때만허용한새두산출물을생성한다.
    main()
