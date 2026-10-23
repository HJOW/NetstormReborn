#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 Fort 선택 기본블록과 next1/6 요청을 격리 실행해 독립 원문을 만든다.

Pending/PathSpec/Refresh/Tell/현재 경로/LoadFort/Core/섬 수/Notify는 명시 경계다.
004b3560의 분기·현재 슬롯 재조회·BYTE 쓰기와 _ftol 본체는 실제 PE 명령을 실행한다.
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
PE_PATH = ROOT / 'originals/Netstorm.exe'  # 수정하지 않는 10.78 보호 PE다.
PE_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 분석 대상으로 확정한 보호 PE의 원문 SHA다.
FIXTURE_PATH = ROOT / 'cpppj/tests/fixtures/fort-selection-x86.tsv'  # 새 독립 관측 TSV다.
REPORT_PATH = ROOT / 'extracted/cpp-fort-selection-20261023-native.json'  # 실행/경계/한계 보고서다.
IMAGE_BASE = 0x400000  # 실제 PE 이미지 기준 주소다.
ENTRIES = {'ask': 0x4D7408, 'read': 0x4D745F, 'next6': 0x4B3550, 'next1': 0x4B3560}  # 네 실제 진입점이다.
MERGE = 0x4D7B1A  # c9/ca 공통 합류 직전 정상 종료점이다.
NEXT6_JUMP = (0x4D73FE, 0x4D7408)  # ca mode0의 실제 next6 호출/합류 JMP 범위다.
ASK_CODE = (0x4D7408, 0x4D7446)  # c9 비교부터 정상 JMP까지 실제 범위다.
READ_CODE = (0x4D745F, 0x4D7483)  # ca 비교부터 정상 JMP까지 실제 범위다.
REQUEST_CODE = (0x4B3550, 0x4B361F)  # next6/next1의 본체와 RET 범위다.
FTOL_CODE = (0x4E49C0, 0x4E4A35)  # CRT의 실제 x87 정수 변환 본체다.
THUNK_CODE = (0x4BD540, 0x4BD545)  # 마지막 Notify tail JMP의 원문 범위다.
NATIVE_RANGES = (NEXT6_JUMP, ASK_CODE, READ_CODE, REQUEST_CODE, FTOL_CODE, THUNK_CODE)  # 실행을 허용한 실제 PE 본체만 나열한다.
MODE = 0x5CAE84  # c9/ca의 현재 모드 DWORD다.
NEXT = 0x5C8878  # 기존 값과 무관하게 덮어쓰는 다음 State DWORD다.
BATTLE = 0x594FBC  # next1의 추가 저장 관문 전역이다.
STATE = 0x5C8758  # next1이14와 비교하는 현재 State다.
SLOT = 0x540CAC  # 각 하위 호출 직전에 다시 읽는 현재 Template 슬롯이다.
OWNER = 0x540C70  # 섬 개수 호출 직전에 읽는 현재 소유자다.
SECONDS = 0x55B4D0  # 실제 FLD가 읽는 게임시각 double이다.
MANAGERS = 0x5C89F0  # 슬롯당20BYTE인 Template manager 배열이다.
HEAP = 0x1000000  # 외부 장치가 공급하는 문자열/Core의 격리 주소다.
CORE_BASE = HEAP + 0x1000  # 슬롯0/1의 원문 Core 바이트를 두는 시작 주소다.
CORE_STRIDE = 0x40  # 두 Core의 별도 물리 간격이다.
CORE_BYTES = 32  # flags/time 외 바이트 불변도 검증하는 관측 길이다.
SELECTED = HEAP + 0x100  # ca EAX가 전달하는 event.y 이름 문자열이다.
LAST_EDIT = HEAP + 0x200  # PathSpec의 명시 lastEditFort 결과 문자열이다.
CURRENT_PATH = HEAP + 0x300  # 현재Fort 경로 getter의 명시 결과다.
STACK = 0x2100000  # 기본블록 입력의 ESP와 독립 함수 호출의 진입 ESP다.
RETURN = HEAP + 0xF000  # 독립 next 함수의 격리 반환 종료점이다.
MAX_INSTRUCTIONS = 1000  # 정상 범위를 벗어나는 실행은 실패시킨다.
CONTROLS = (0x27F, 0x37F)  # 원본 두 x87 정밀도에서 독립 실행한다.
BOUNDARIES = {
    0x4410F0: 'path', 0x460BB0: 'pending', 0x4CAFD0: 'refresh',
    0x4CF960: 'tell', 0x460680: 'current-path', 0x4BF550: 'load',
    0x4BD550: 'core', 0x46F430: 'islands', 0x4BD360: 'notify',
}  # 계산을 대체하는 외부 경계이며 _ftol은 포함하지 않는다.
SCHEMA = (
    'name', 'kind', 'mode', 'next', 'battle', 'state', 'slot', 'owner',
    'flags0', 'flags1', 'seconds', 'islands', 'mutation',
    'modeout', 'nextout', 'slotout', 'ownerout', 'core0', 'core1',
    'pending', 'trace', 'stackdelta',
)  # 앞13열은 독립 입력이고 뒤9열은 실제 원문 실행의 관측이다.


def sha256(path):
    """보호 입력/새 산출물의 SHA256을 읽기 전용으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """원래 PE 주소를 유지하고 명시 외부 장치에만 반환값을 공급한다."""

    def __init__(self):
        """보호 PE 복사와 격리 스택/Core를 준비하며 디스크 원문은 바꾸지 않는다."""
        self.pe = pefile.PE(str(PE_PATH))  # 읽기 전용 원문 이미지다.
        if self.pe.OPTIONAL_HEADER.ImageBase != IMAGE_BASE:
            raise RuntimeError('보호 PE 이미지 기준 주소 불일치')
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 실제32비트 기계어 실행기다.
        self.image_size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095  # 페이지 정렬한 PE 길이다.
        self.mu.mem_map(IMAGE_BASE, self.image_size)
        self.mu.mem_write(IMAGE_BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x10000)
        self.mu.mem_map(STACK - 0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.memory_write)
        self.original = [(first, bytes(self.mu.mem_read(first, end - first))) for first, end in NATIVE_RANGES]  # 지정 원문 불변 확인용이다.
        self.boundary_totals = Counter()  # 명시 외부 경계의 호출 합계다.
        self.native_write_totals = Counter()  # CPU 원문 명령으로 수행한 쓰기 합계다.
        self.precisions = {}  # 정밀도별 실제 실행/명령 수다.

    def dword(self, address, value=None):
        """같은 비정렬 DWORD를 읽고 입력/경계 효과일 때만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def c_string(self, address):
        """원문 C문자열의 현재 NUL 앞 ASCII 바이트를 읽는다."""
        result = bytearray()  # 첫 NUL 전까지의 실제 바이트다.
        for offset in range(256):  # 명시 격리 문자열의 물리 한도를 지킨다.
            byte = self.mu.mem_read(address + offset, 1)[0]  # 현재 주소의 실제BYTE다.
            if not byte:
                return result.decode('ascii')
            result.append(byte)
        raise RuntimeError('원문 문자열 NUL/범위 누락')

    def returned(self, value=0, cleanup=0):
        """외부 경계의 반환 주소와 명시 thiscall 인자만 소비한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 실제 CALL 진입 스택이다.
        target = self.dword(esp)  # 원문이 저장한 반환 주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def manager_slot(self):
        """실제 ECX this를 슬롯0/1로 정규화하고 잘못된 manager를 거부한다."""
        difference = self.mu.reg_read(UC_X86_REG_ECX) - MANAGERS  # 실제 lea 결과다.
        if difference not in (0, 20):
            raise RuntimeError('명시 두 슬롯 외의 manager this')
        return difference // 20

    def mutate(self, boundary):
        """선택된 외부 callback 효과만 실제 공유 전역에 공급한다."""
        mutation = self.case['mutation']  # 출력과 무관한 독립 입력이다.
        if mutation == boundary + '-mode0':
            self.dword(MODE, 0)
        elif mutation == boundary + '-mode2':
            self.dword(MODE, 2)
        elif mutation == boundary + '-slot1':
            self.dword(SLOT, 1)
        elif mutation == boundary + '-owner2':
            self.dword(OWNER, 2)
        elif mutation == boundary + '-next99':
            self.dword(NEXT, 99)

    def memory_write(self, mu, access, address, size, value, data):
        """CPU의 원문 쓰기를 예상 전역/두 Core/격리 스택으로 제한한다."""
        if address == MODE and size == 4:
            category = 'mode'
        elif address == NEXT and size == 4:
            category = 'next'
        elif any(CORE_BASE + slot * CORE_STRIDE <= address and address + size <= CORE_BASE + slot * CORE_STRIDE + CORE_BYTES for slot in (0, 1)):
            category = 'core'
        elif STACK - 0x100 <= address and address + size <= STACK + 4:
            category = 'stack'
        else:
            raise RuntimeError(f'원문 예상 밖의 메모리 쓰기 {address:08x}/{size}')
        self.native_write_totals[category] += 1

    def instruction(self, mu, address, size, data):
        """지정 기계어를 실행하고 명시 경계/종료점만 가로챈다."""
        self.instructions += 1
        if address in (MERGE, RETURN):
            self.stop = address
            mu.emu_stop()
            return
        if any(first <= address < end for first, end in NATIVE_RANGES):
            self.native_instructions += 1
            if address == FTOL_CODE[0]:
                self.trace.append('time')
            return
        if address not in BOUNDARIES:
            raise RuntimeError(f'격리 범위 밖의 미명시 코드 {address:08x}')
        boundary = BOUNDARIES[address]  # 실제 진입한 하위 호출 종류다.
        self.boundary_totals[boundary] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)  # 원문이 실제 전달한 스택이다.
        if boundary == 'path':
            key = self.c_string(self.dword(esp + 4))  # 실제 PE lastEditFort 리터럴이다.
            arguments = [self.dword(esp + offset) for offset in (8, 12, 16)]  # 원문 push한 세0 인자다.
            self.trace.append('path:' + key + ':' + ':'.join(map(str, arguments)))
            self.mutate('path')
            self.returned(LAST_EDIT)
        elif boundary == 'pending':
            self.pending = self.c_string(self.dword(esp + 4))  # 실제 EAX/PathSpec 반환 문자열을 복사한다.
            self.trace.append('pending:' + self.pending)
            self.mutate('pending')
            self.returned()
        elif boundary == 'refresh':
            self.trace.append('refresh:' + str(self.dword(esp + 4)))
            self.mutate('refresh')
            self.returned()
        elif boundary == 'tell':
            self.trace.append('tell:' + self.c_string(self.dword(esp + 4)))
            self.mutate('tell')
            self.returned()
        elif boundary == 'current-path':
            self.trace.append('current-path')
            self.mutate('path')
            self.returned(CURRENT_PATH)
        elif boundary == 'load':
            self.trace.append(f'load:{self.manager_slot()}:' + self.c_string(self.dword(esp + 4)))
            self.mutate('load')
            self.returned(cleanup=4)
        elif boundary == 'core':
            slot = self.manager_slot()  # ECX는 원문이 호출 직전 현재 전역으로 계산했다.
            self.trace.append(f'core:{slot}')
            self.core_calls += 1
            self.mutate('core' + str(self.core_calls))
            self.returned(CORE_BASE + slot * CORE_STRIDE)
        elif boundary == 'islands':
            if mu.reg_read(UC_X86_REG_ECX) != 0x568B60:
                raise RuntimeError('섬 개수의 실제 this 불일치')
            self.trace.append('islands:' + str(self.dword(esp + 4)))
            self.mutate('islands')
            self.returned(self.case['islands'], cleanup=4)
        elif boundary == 'notify':
            self.trace.append(f'notify:{self.manager_slot()}')
            self.mutate('notify')
            self.returned()

    def run(self, case, control):
        """입력 하나를 실제 PE로 실행하고 현재 상태/바이트/호출 순서를 관측한다."""
        self.case = case  # 기대 출력을 포함하지 않는 명시 입력이다.
        self.trace = []  # 원문 호출 진입 순서로만 채운다.
        self.pending = 'old'  # pending 장치의 독립 초기 이름이다.
        self.core_calls = 0  # Core callback 번호별 전역 변경을 선택한다.
        self.stop = None  # 실제 정상 endpoint에 도달해야 성공이다.
        self.instructions = 0  # 전체 hook 진입 횟수다.
        self.native_instructions = 0  # 지정 본체의 실제 실행 명령 수다.
        self.mu.mem_write(STACK - 0x100, bytes([0xCC]) * 0x200)
        for address, value in ((MODE, case['mode']), (NEXT, case['next']), (BATTLE, case['battle']),
                               (STATE, case['state']), (SLOT, case['slot']), (OWNER, case['owner'])):  # 현재 전역을 각 입력으로 독립 초기화한다.
            self.dword(address, value)
        self.mu.mem_write(SECONDS, struct.pack('<d', case['seconds']))
        for slot in (0, 1):  # 두 실제 Core를 구별해 부분 쓰기와 재조회를 함께 검증한다.
            raw = bytearray([0xA5] * CORE_BYTES)  # flags/time 외 보존할 명시 바이트다.
            raw[0] = case['flags' + str(slot)]
            struct.pack_into('<I', raw, 12, 0x11223344)
            self.mu.mem_write(CORE_BASE + slot * CORE_STRIDE, bytes(raw))
        for address, value in ((SELECTED, b'Selected\0'), (LAST_EDIT, b'LastEdit\0'), (CURRENT_PATH, b'CurrentPath\0')):  # 실제 C문자열 인자 수명을 고정한다.
            self.mu.mem_write(address, value)
        self.dword(STACK, RETURN)
        registers = {
            UC_X86_REG_EAX: SELECTED, UC_X86_REG_EBX: 0x10111213,
            UC_X86_REG_ECX: 0x20212223, UC_X86_REG_EDX: 0x30313233,
            UC_X86_REG_ESI: 0x40414243, UC_X86_REG_EDI: 0,
            UC_X86_REG_EBP: 0x020000C9 if case['kind'] == 'ask' else 0x020000CA,
            UC_X86_REG_ESP: STACK, UC_X86_REG_FPCW: control, UC_X86_REG_FPSW: 0,
        }  # c9/ca 기본블록의 명시 레지스터와 next함수의 동일 격리 호출 상태다.
        for register, value in registers.items():  # 이전 실행의 잔여 레지스터를 분리한다.
            self.mu.reg_write(register, value)
        entry = ENTRIES[case['kind']]  # 선택한 실제 PE 진입점이다.
        self.mu.emu_start(entry, MERGE + 1, count=MAX_INSTRUCTIONS)
        standalone = case['kind'] in ('next1', 'next6')  # 독립 함수는 RET가 반환 주소 한칸을 소비한다.
        expected_stop = RETURN if standalone else MERGE  # 정상 종료 종류다.
        stackdelta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - (4 if standalone else 0)  # 호출 규약 차이를 정규화한 잔여 스택이다.
        if self.stop != expected_stop or stackdelta:
            raise RuntimeError(f'실제 endpoint/스택 불일치 {case["name"]}: {self.stop}/{stackdelta}')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('실제 x87 제어어/스택 불일치')
        for first, original in self.original:  # 기계어 바이트의 원문 불변을 확인한다.
            if bytes(self.mu.mem_read(first, len(original))) != original:
                raise RuntimeError('보호 PE 본체 바이트 변경')
        totals = self.precisions.setdefault(f'{control:03x}', {'runs': 0, 'native_instructions': 0, 'hook_events': 0})  # 정밀도별 실제 누적 관측이다.
        totals['runs'] += 1
        totals['native_instructions'] += self.native_instructions
        totals['hook_events'] += self.instructions
        return {
            'modeout': self.dword(MODE), 'nextout': self.dword(NEXT),
            'slotout': self.dword(SLOT), 'ownerout': self.dword(OWNER),
            'core0': bytes(self.mu.mem_read(CORE_BASE, CORE_BYTES)).hex(),
            'core1': bytes(self.mu.mem_read(CORE_BASE + CORE_STRIDE, CORE_BYTES)).hex(),
            'pending': self.pending, 'trace': ';'.join(self.trace), 'stackdelta': stackdelta,
        }


def cases():
    """일반 관문·BYTE·기존next·시간 경계와 callback 변경을 독립 교차한다."""
    result = []  # 제품 코드나 기존 fixture를 읽지 않고 만드는 입력 목록이다.
    base = dict(kind='next1', mode=2, next=99, battle=1, state=14, slot=0, owner=7,
                flags0=255, flags1=6, seconds=123.75, islands=0, mutation='fixed')  # 정상 추가 저장 입력이다.
    for index, values in enumerate(itertools.product(ENTRIES, (0, 1, 2, 0xFFFFFFFF), (0, 6, 99),
                                                    ((0, 14), (1, 3), (1, 14)), (-1, 0, 1), (0, 2, 6, 255))):  # 다음 요청/관문/섬/부분flags를 교차한다.
        kind, mode, next_state, gate, islands, flags = values  # 기대 없는 입력 조합이다.
        result.append(dict(base, name=f'fixed-{index:04d}', kind=kind, mode=mode, next=next_state,
                           battle=gate[0], state=gate[1], islands=islands, flags0=flags))
    for index, values in enumerate(itertools.product(('read', 'next1'), (0, 1),
                                                    (-2147483648.75, -123.75, -1.75, -0.75, 0.0, 0.75, 1.75, 2147483647.75, 2147483648.75), (-1, 1))):  # 실제 _ftol의 부호/0/32비트 경계를 통과한다.
        kind, slot, seconds, islands = values  # 현재 슬롯과 lowDWORD 변환을 함께 관측한다.
        result.append(dict(base, name=f'time-{index:03d}', kind=kind, slot=slot, seconds=seconds, islands=islands))
    for index, values in enumerate(itertools.product(('pending-mode0', 'pending-mode2', 'pending-next99'), (0, 2))):  # ca의 pending 뒤 현재mode 재조회를 관측한다.
        mutation, mode = values  # callback이 바꾸기 전의 모드다.
        result.append(dict(base, name=f'pending-{index:02d}', kind='read', mode=mode, mutation=mutation))
    for index, mutation in enumerate(('path-mode0', 'pending-mode0', 'refresh-mode0', 'tell-next99')):  # c9는 외부 callback 뒤 mode=2/next를 다시 덮어쓰지 않는다.
        result.append(dict(base, name=f'ask-mutation-{index:02d}', kind='ask', mutation=mutation))
    for index, values in enumerate(itertools.product(('path-slot1', 'load-slot1', 'core1-slot1', 'core2-slot1',
                                                    'islands-slot1', 'load-next99', 'core1-next99', 'notify-next99',
                                                    'path-owner2', 'load-owner2', 'core1-owner2', 'core3-slot1', 'core3-next99'), (-1, 1))):  # 하위 callback 뒤 슬롯/소유자/next 재조회 여부를 관측한다.
        mutation, islands = values  # 분기 결과와 callback 효과를 별도로 입력한다.
        result.append(dict(base, name=f'callback-{index:02d}', mutation=mutation, islands=islands))
    return result


def main():
    """두 정밀도의 독립 관측을 새 TSV/보고서에만 기록한다."""
    before = sha256(PE_PATH)  # 보호 원문 디스크 SHA다.
    if before != PE_SHA256:
        raise RuntimeError('분석 대상으로 확정한 보호 PE SHA256 불일치')
    oracle = Oracle()  # 실제 PE 격리 실행기다.
    inputs = cases()  # 기대 없는 독립 입력 목록이다.
    rows = []  # 정밀도별 원문 관측 평문 행이다.
    for case in inputs:  # 입력마다 실제 본체를 새 상태에서 실행한다.
        first_observation = None  # 두 정밀도 간 동일성 확인용이다.
        for control in CONTROLS:  # x87 제어어별 관측을 별도 행으로 보존한다.
            output = oracle.run(case, control)  # 제품 계산 없이 얻은 실제 관측이다.
            if first_observation is not None and output != first_observation:
                raise RuntimeError('정밀도별 관측 불일치: ' + case['name'])
            first_observation = output
            values = dict(case, name=f'{case["name"]}-cw{control:03x}', **output)  # 입력/관측의 명시 합성이다.
            rows.append('\t'.join(str(values[column]) for column in SCHEMA))
    if sha256(PE_PATH) != before:
        raise RuntimeError('보호 PE 디스크 SHA 변경')
    header = '# Fort 선택 기본블록/next1·6의 실제 PE 관측. 앞13열 입력/뒤9열 관측이며 하위 장치는 명시 경계다.\n#' + '\t'.join(SCHEMA) + '\n'  # 독립 스키마를 평문으로 보존한다.
    FIXTURE_PATH.write_text(header + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    report = {
        'host': socket.gethostname(), 'date': '2026-10-23', 'inputs': len(inputs), 'precision_runs': len(rows),
        'precision_differences': 0, 'precision_observations': oracle.precisions,
        'schema': list(SCHEMA), 'input_columns': list(SCHEMA[:13]), 'observation_columns': list(SCHEMA[13:]),
        'entries': {name: f'{address:08x}' for name, address in ENTRIES.items()},
        'endpoints': {'commands': f'{MERGE:08x}', 'standalone': f'{RETURN:08x}'},
        'entry_contract': 'c9/ca: EBP=command, EDI=0, EAX=event.y 문자열 주소. standalone: ESP의 격리 반환 주소. 스택 차이는 standalone RET의4BYTE를 정규화한다.',
        'core_contract': '슬롯0/1 각각32BYTE, 초기a5 fill/byte0=flagsN/+c DWORD=11223344. manager005c89f0+slot*20. 모든 core 바이트를 hex로 출력한다.',
        'boundaries': {f'{address:08x}': name for address, name in BOUNDARIES.items()},
        'boundary_calls': dict(oracle.boundary_totals), 'native_write_counts': dict(oracle.native_write_totals),
        'native_ranges': [{'first': f'{first:08x}', 'end_exclusive': f'{end:08x}',
                           'sha256': hashlib.sha256(bytes(oracle.mu.mem_read(first, end - first))).hexdigest()} for first, end in NATIVE_RANGES],
        'mutations': '고정 입력 외 callback의 pending/path/refresh/tell/load/core1/core2/islands/notify 뒤 mode0/2,slot1,owner2,next99를 명시 공급한다. 본체가 후속 현재 전역을 재조회하는지 관측한다.',
        'ftol': '004e49c0~004e4a34를 실제 실행한다. double0055b4d0의FLD와 lowDWORD 쓰기도 본체가 수행한다. time trace는 실제CRT 진입 표식이며 대체 계산 경계가 아니다.',
        'scope': 'c9/ca 순서/next덮어쓰기/관문과 next1의 flags BYTE/time DWORD/현재 슬롯 재조회/마지막 Notify를 실제 PE로 관측한다.',
        'limits': ['전체004d62b0 입력 부모의 Guide/Gump/release 관문은 이 기본블록 진입 관측 밖이다.',
                   'Pending/PathSpec/Refresh/Tell/LoadFort/Core/소유 섬수/Notify 하위 몸체는 명시 외부 장치다.',
                   '전체 State1/6 설치·Editor 월드·GUI·게임 실행·원본 SEH 예외를 검증하지 않는다.'],
        'new_game_runs': 0, 'new_decompilations': 0,
        'sha256': {str(path.relative_to(ROOT)).replace('\\', '/'): sha256(path) for path in (PE_PATH, Path(__file__), FIXTURE_PATH)},
    }  # 관측과 경계/미검증 범위를 분리한 재사용 보고서다.
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'native inputs={len(inputs)} precision_runs={len(rows)} differences=0')
    print('schema=' + '\t'.join(SCHEMA))
    print('fixture=' + str(FIXTURE_PATH.relative_to(ROOT)))
    print('report=' + str(REPORT_PATH.relative_to(ROOT)))


if __name__ == '__main__':  # 직접 호출 때에만 허용된 새 두 관측 파일을 생성한다.
    main()
