#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Contents writer004bcea0의 실제 명령과 부분 출력을 독립 관측한다.

Finder/Robust/Type/ParentType/assert만 명시 경계이며 예약 count와 모든
직렬화·재조회·재귀는 보호 PE 원문이 수행한다. 게임 프로세스는 실행하지 않는다.
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


ROOT = Path(__file__).resolve().parents[1]  # 허용한 새 산출물을 둘 저장소 루트다.
PE_PATH = ROOT / 'originals/Netstorm.exe'  # 수정하지 않는10.78 보호 PE다.
PE_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 원문 입력 식별자다.
FIXTURE_PATH = ROOT / 'cpppj/tests/fixtures/fort-content-save-x86.tsv'  # 새 독립 관측 TSV다.
REPORT_PATH = ROOT / 'extracted/cpp-fort-content-save-20261023-native.json'  # 범위와 경계를 밝힐 보고서다.
IMAGE_BASE = 0x400000  # 보호 PE의 실제 이미지 기준 주소다.
ENTRY = 0x4BCEA0  # Contents writer의 실제 첫 명령이다.
NATIVE_END = 0x4BD124  # RET를 포함한 실제 본체의 배타적 끝이다.
POOL_GLOBAL = 0x5C8464  # 각 Form 조회에서 다시 읽는 원본 Pool 기준 포인터다.
LIMIT_GLOBAL = 0x5C847C  # 후보 물리 경계에서 다시 읽는 원본 Pool 개수다.
HEAP = 0x1000000  # 명시 장치와 출력의 격리 메모리다.
NODE = HEAP + 0x100  # 고정 stream의16BYTE 헤더다.
PAYLOADS = (HEAP + 0x1000, HEAP + 0x6000)  # callback의 data 변경도 구별할 두 출력 버퍼다.
BUFFER_SIZE = 0x4000  # 각 물리 버퍼는16KB이고 논리 alloc과 별개다.
RETURN = HEAP + 0xF000  # 원문 최상위 RET의 격리 종료점이다.
ROBUST = HEAP + 0xF100  # 실제 가상60 CALL이 도달할 명시 Robust 경계다.
VTABLE = HEAP + 0xF200  # 원문 후보가 읽는 명시 Form 가상표다.
FORMS = HEAP + 0x20000  # 실제50BYTE stride로 읽는 Form 배열이다.
FORM_SLOTS = 16  # 모든 입력 Form과 변경은 이 물리 범위 안에 둔다.
TYPES = HEAP + 0x30000  # 입력 타입들의 실제+e8/+fc/+100 필드 저장소다.
TYPE_STRIDE = 0x200  # 각 타입의 독립 물리 간격이다.
STACK = 0x2100000  # 최상위 cdecl 진입의 명시 ESP다.
MAX_DEPTH = 12  # 명시 무순환 트리의 허용 재귀 깊이다.
MAX_INSTRUCTIONS = 300000  # 크기/커서 변화로 무한 실행하는 입력은 거부한다.
CONTROLS = (0x27F, 0x37F)  # 두 x87 제어어에서 별도로 실행한다.
BOUNDARIES = {0x4B2270: 'begin', 0x4B1B20: 'next', ROBUST: 'robust',
              0x49A840: 'type', 0x4AC550: 'parent', 0x4E0620: 'assert'}  # 이 하위 몸체만 대체한다.
SCHEMA = (
    'name', 'parent', 'poollimit', 'position', 'end', 'capacity', 'forms', 'types', 'children', 'mutation',
    'completed', 'positionout', 'endout', 'dataout', 'payload0', 'payload1', 'trace', 'stackdelta',
    'poollimitout', 'formsout', 'typesout', 'nativeinstructions',
)  # 앞10열은 기대 없는 입력, 뒤12열은 보호 PE 실행 관측이다.


def sha256(path):
    """파일을 변경하지 않고 입력/산출물 식별자를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def records(text):
    """세미콜론/콜론으로 나눈 십진수 원시 레코드를 읽는다."""
    return [] if text == '-' else [[int(value) for value in row.split(':')] for row in text.split(';')]


class Oracle:
    """실제 Contents 명령을 실행하고 경계 효과만 명시 공급한다."""

    def __init__(self):
        """읽기 전용 PE와 외부 저장소를 분리해 준비한다."""
        self.pe = pefile.PE(str(PE_PATH))  # 보호 입력의 읽기 전용 PE 파서다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 원문32비트 명령 실행기다.
        self.image_size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095  # 페이지 정렬된 이미지 크기다.
        if self.pe.OPTIONAL_HEADER.ImageBase != IMAGE_BASE:
            raise RuntimeError('원본 PE ImageBase 불일치')
        self.mu.mem_map(IMAGE_BASE, self.image_size)
        self.mu.mem_write(IMAGE_BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x50000)
        self.mu.mem_map(STACK - 0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.memory_write)
        self.original = bytes(self.mu.mem_read(ENTRY, NATIVE_END - ENTRY))  # 실제 명령 불변 확인용이다.
        self.boundary_totals = Counter()  # 모든 실행의 명시 경계 호출 합계다.
        self.write_totals = Counter()  # 원문 CPU 쓰기의 범위별 합계다.
        self.precisions = {}  # 제어어별 실행/명령 합계다.

    def number(self, address, width=4, value=None):
        """비정렬 little endian 원시 값을 읽고 명시 장치 효과만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, int(value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def returned(self, value=0, cleanup=0):
        """외부 CALL의 반환주소와 thiscall의 인자만 소비한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 실제 CALL 진입 스택 포인터다.
        target = self.number(esp)  # CPU가 저장한 실제 원문 복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def apply(self, trigger):
        """입력에 적힌 현재 경계의 변경만 현재 저장소에 반영한다."""
        for entry in self.mutations:  # 세미콜론으로 지정한 변경들을 입력 순서대로 적용한다.
            event, target, field, raw = entry  # 효과 시점/대상/필드/십진수 값이다.
            if event != trigger:
                continue
            value = int(raw)  # 변경할 명시 정수다.
            if target.startswith('form'):
                sid = int(target[4:])  # 바꿀 실제50BYTE Form의SID다.
                offset, width = {'quantity': (12, 2), 'kind': (18, 2), 'listflags': (20, 2)}.get(field, (0, 0))  # 원문 raw 필드 위치다.
                if field == 'robust':
                    self.robust_values[sid] = value
                elif width:
                    self.number(FORMS + sid * 50 + offset, width, value)
                else:
                    raise RuntimeError('미명시 Form 변경 필드')
            elif target.startswith('type'):
                kind = int(target[4:])  # 바꿀 명시 타입 번호다.
                offset = {'flags1': 0xE8, 'content': 0x100, 'container': 0xFC}[field]  # 실제 타입 필드 위치다.
                self.number(self.type_addresses[kind] + offset, value=value)
            elif target == 'stream':
                offset = {'data': 0, 'position': 4, 'end': 8, 'capacity': 12}[field]  # 현재 stream 필드다.
                self.number(NODE + offset, value=PAYLOADS[value] if field == 'data' else value)
            elif target == 'global' and field == 'poollimit':
                self.number(LIMIT_GLOBAL, value=value)
            elif target == 'finder' and field == 'current':
                self.number(self.frames[-1]['finder'] + 8, value=value)
            elif target == 'control' and field == 'stop' and value == 1:
                self.stopped = True
                self.mu.emu_stop()
            else:
                raise RuntimeError('미명시 callback 변경 대상')

    def memory_write(self, mu, access, address, size, value, data):
        """실제 CPU 쓰기를 스택·stream·두 출력 버퍼 안으로 제한한다."""
        if STACK - 0x4000 <= address and address + size <= STACK + 12:
            category = 'stack'
        elif NODE <= address and address + size <= NODE + 16:
            category = 'stream'
        elif any(first <= address and address + size <= first + BUFFER_SIZE for first in PAYLOADS):
            category = 'payload'
        else:
            raise RuntimeError(f'원문 예상 밖 쓰기 {address:08x}/{size}')
        self.write_totals[category] += 1

    def instruction(self, mu, address, size, data):
        """본체 명령을 실행하고 명시된 하위 장치만 순서대로 관측한다."""
        self.hook_events += 1
        if address == RETURN:
            self.completed = True
            mu.emu_stop()
            return
        if ENTRY <= address < NATIVE_END:
            self.native_instructions += 1
            if address == ENTRY:
                esp = mu.reg_read(UC_X86_REG_ESP)  # 현재 재귀 호출의 진입 스택이다.
                parent = self.number(esp + 4)  # 이번 호출이 받은 고정 부모SID다.
                if len(self.frames) >= MAX_DEPTH or parent in [frame['parent'] for frame in self.frames]:
                    raise RuntimeError('명시 깊이/무순환 트리 계약 위반')
                self.frames.append({'parent': parent, 'finder': None})
            elif address == 0x4BD123:
                self.frames.pop()
            return
        if address not in BOUNDARIES:
            raise RuntimeError(f'격리 범위 밖 미명시 코드 {address:08x}')
        boundary = BOUNDARIES[address]  # 현재 CALL의 명시 장치 이름이다.
        self.boundary_totals[boundary] += 1
        self.calls[boundary] += 1
        trigger = boundary + str(self.calls[boundary])  # 입력 변경이 사용하는1기반 호출 번호다.
        esp = mu.reg_read(UC_X86_REG_ESP)  # 외부 진입의 실제 인자 스택이다.
        if boundary == 'begin':
            parent = self.number(esp + 4)  # 이번 Finder가 받은 실제 부모SID다.
            finder = mu.reg_read(UC_X86_REG_ECX)  # 재귀마다 별도로 생성한 stack Finder 주소다.
            self.frames[-1]['finder'] = finder
            sequence = self.children.get(parent, [])  # 명시 Begin/Next 장치의 후보 순서다.
            self.finders[finder] = {'parent': parent, 'sequence': sequence, 'index': 0}
            self.number(finder + 8, value=sequence[0] if sequence else 0)
            self.trace.append(f'begin:{parent}')
            self.apply(trigger)
            self.returned(cleanup=4)
        elif boundary == 'next':
            finder = mu.reg_read(UC_X86_REG_ECX)  # 현재 재귀의 Finder this다.
            cursor = self.finders[finder]  # 같은 호출의 독립 후보 커서다.
            current = self.number(finder + 8)  # callback 변경 뒤 원문이 넘겨준 현재SID다.
            self.trace.append(f'next:{cursor["parent"]}:{current}')
            cursor['index'] += 1
            sequence = cursor['sequence']  # 명시 외부 장치가 고정한 후보 순서다.
            self.number(finder + 8, value=sequence[cursor['index']] if cursor['index'] < len(sequence) else 0)
            self.apply(trigger)
            self.returned()
        elif boundary == 'robust':
            sid = (mu.reg_read(UC_X86_REG_ECX) - FORMS) // 50  # 실제 가상60이 받은 후보SID다.
            answer = self.robust_values.get(sid, 0)  # callback 변경 전 반환값을 명시 캡처한다.
            self.trace.append(f'robust:{sid}')
            self.apply(trigger)
            self.returned(answer)
        elif boundary == 'type':
            kind = self.number(esp + 4)  # 원문이 push한 실제 kind 인자다.
            self.trace.append(f'type:{kind}')
            self.apply(trigger)
            self.returned(self.type_addresses[kind])
        elif boundary == 'parent':
            address = mu.reg_read(UC_X86_REG_ECX)  # Begin전에 원문이 보관한 부모 rawpointer다.
            sid = (address - FORMS) // 50  # 명시 고정Pool의 부모SID다.
            self.trace.append(f'parent:{sid}')
            self.apply(trigger)
            self.returned(self.type_addresses[self.number(address + 18, 2)])
        else:
            line = self.number(esp + 12)  # 원문 assert의 소스행 번호다.
            self.trace.append(f'assert:{line}')
            self.apply(trigger)
            if not self.stopped:
                self.returned()

    def output_forms(self):
        """현재 Form raw 필드와 명시 Robust 장치 상태를 평문으로 반환한다."""
        return ';'.join(':'.join(map(str, (sid, self.number(FORMS + sid * 50 + 18, 2),
                                         self.number(FORMS + sid * 50 + 12, 2),
                                         self.number(FORMS + sid * 50 + 20, 2), self.robust_values[sid])))
                        for sid in self.form_ids)

    def output_types(self):
        """현재 타입의 세 raw DWORD를 입력과 같은 순서로 반환한다."""
        return ';'.join(':'.join(map(str, (kind, self.number(self.type_addresses[kind] + 0xE8),
                                         self.number(self.type_addresses[kind] + 0x100),
                                         self.number(self.type_addresses[kind] + 0xFC)))) for kind in self.type_ids)

    def run(self, case, control):
        """독립 입력 하나를 온전한 cdecl 진입부터 실제 RET/진단 중단까지 실행한다."""
        self.case = case  # 결과를 포함하지 않는 독립 입력이다.
        self.frames = []  # 실제 재귀 진입마다 소유하는 Finder 프레임이다.
        self.finders = {}  # 명시 외부 장치의 독립 후보 커서들이다.
        self.calls = Counter()  # 현재 실행의 종류별 경계 호출 번호다.
        self.trace = []  # 모든 외부 CALL의 실제 순서다.
        self.mutations = [] if case['mutation'] == '-' else [entry.split('/') for entry in case['mutation'].split(';')]  # 명시 callback 효과다.
        self.completed = False  # 실제 최상위 RET 도달 여부다.
        self.stopped = False  # 명시 진단 중단 여부이며 SEH unwind를 뜻하지 않는다.
        self.native_instructions = 0  # 실제 보호 본체에서 실행한 명령 수다.
        self.hook_events = 0  # 경계/종료를 포함한 전체 코드 hook 수다.
        self.robust_values = {}  # 외부 Robust의 현재 반환 장치 상태다.
        self.mu.mem_write(STACK - 0x4000, bytes([0xCC]) * 0x4010)
        self.mu.mem_write(FORMS, bytes(FORM_SLOTS * 50))
        self.mu.mem_write(TYPES, bytes(0x10000))
        self.number(VTABLE + 0x60, value=ROBUST)
        for sid in range(FORM_SLOTS):  # 모든 물리 Form에 같은 명시 가상60 경계를 설치한다.
            self.number(FORMS + sid * 50, value=VTABLE)
        self.form_ids = []  # 출력에 보존할 명시 Form의 입력 순서다.
        for sid, kind, quantity, flags, robust in records(case['forms']):  # 입력 raw 필드를 실제stride에 배치한다.
            if not 0 <= sid < FORM_SLOTS:
                raise RuntimeError('입력 Form 물리 범위 초과')
            self.form_ids.append(sid)
            self.number(FORMS + sid * 50 + 12, 2, quantity)
            self.number(FORMS + sid * 50 + 18, 2, kind)
            self.number(FORMS + sid * 50 + 20, 2, flags)
            self.robust_values[sid] = robust
        self.type_ids = []  # 출력 타입의 입력 순서다.
        self.type_addresses = {}  # 명시 GetType이 반환할 raw 레코드 주소다.
        for index, (kind, flags1, content, container) in enumerate(records(case['types'])):  # 실제필드에 타입입력을 배치한다.
            address = TYPES + index * TYPE_STRIDE  # 이 입력 타입의 고정 rawpointer다.
            self.type_ids.append(kind)
            self.type_addresses[kind] = address
            self.number(address + 0xE8, value=flags1)
            self.number(address + 0x100, value=content)
            self.number(address + 0xFC, value=container)
        self.children = {}  # 명시 Begin/Next가 공급하는 작은 후보 트리다.
        if case['children'] != '-':
            for row in case['children'].split(';'):  # 부모별 후보를 입력 순서대로 분리한다.
                parent, children = row.split(':')  # 이 부모와 쉼표로 나눈 자식SID들이다.
                self.children[int(parent)] = [] if children == '-' else [int(sid) for sid in children.split(',')]
        for payload in PAYLOADS:  # data변경 전후 쓰기를 구별하도록 두 버퍼를 같은CC로 채운다.
            self.mu.mem_write(payload, bytes([0xCC]) * BUFFER_SIZE)
        self.mu.mem_write(NODE, struct.pack('<IIII', PAYLOADS[0], case['position'], case['end'], case['capacity']))
        self.number(POOL_GLOBAL, value=FORMS)
        self.number(LIMIT_GLOBAL, value=case['poollimit'])
        self.mu.mem_write(STACK, struct.pack('<III', RETURN, case['parent'], NODE))
        registers = {UC_X86_REG_EAX: 0x10111213, UC_X86_REG_EBX: 0x20212223,
                     UC_X86_REG_ECX: 0x30313233, UC_X86_REG_EDX: 0x40414243,
                     UC_X86_REG_ESI: 0x50515253, UC_X86_REG_EDI: 0x60616263,
                     UC_X86_REG_EBP: 0x70717273, UC_X86_REG_ESP: STACK,
                     UC_X86_REG_FPCW: control, UC_X86_REG_FPSW: 0}  # 명시 함수 진입 레지스터다.
        for register, value in registers.items():  # 이전 관측의 레지스터 잔여를 제거한다.
            self.mu.reg_write(register, value)
        self.mu.emu_start(ENTRY, RETURN + 1, count=MAX_INSTRUCTIONS)
        stackdelta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - 4  # cdecl RET의 반환주소만 정규화한다.
        if not self.completed and not self.stopped:
            raise RuntimeError('실제 RET/명시 중단 없이 명령 한도 도달')
        if self.completed and (stackdelta or self.frames):
            raise RuntimeError('정상 RET의 스택/재귀 프레임 불일치')
        if self.completed:
            for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 원문callee-save를확인한다.
                if self.mu.reg_read(register) != registers[register]:
                    raise RuntimeError('callee-save 레지스터 변경')
        if bytes(self.mu.mem_read(ENTRY, len(self.original))) != self.original:
            raise RuntimeError('원문 명령 바이트 변경')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어어/스택 변경')
        position = self.number(NODE + 4)  # 마지막 실제 쓰기의 현재 커서다.
        end = self.number(NODE + 8)  # 초기end와 모든부분쓰기를 보존한 논리끝이다.
        if max(position, end) > BUFFER_SIZE:
            raise RuntimeError('명시 물리 출력 범위 초과')
        totals = self.precisions.setdefault(f'{control:03x}', {'runs': 0, 'native_instructions': 0, 'hook_events': 0})  # 정밀도별합계다.
        totals['runs'] += 1
        totals['native_instructions'] += self.native_instructions
        totals['hook_events'] += self.hook_events
        return {'completed': int(self.completed), 'positionout': position, 'endout': end,
                'dataout': PAYLOADS.index(self.number(NODE)),
                'payload0': bytes(self.mu.mem_read(PAYLOADS[0], end)).hex(),
                'payload1': bytes(self.mu.mem_read(PAYLOADS[1], end)).hex(),
                'trace': ';'.join(self.trace), 'stackdelta': stackdelta,
                'poollimitout': self.number(LIMIT_GLOBAL), 'formsout': self.output_forms(),
                'typesout': self.output_types(), 'nativeinstructions': self.native_instructions}


def cases():
    """원시필드/플래그/물리 경계/작은 트리/콜백 변경을 기대값 없이 교차한다."""
    result = []  # 새 독립 입력 목록이다.
    base = dict(parent=1, poollimit=8, position=0, end=0, capacity=4096,
                forms='1:500:0:0:1;2:71:43981:32929:1;3:72:4660:65535:1',
                types='500:0:0:0;71:0:0:0;72:0:0:0;300:0:0:0;501:0:0:0',
                children='1:2', mutation='-')  # 한 후보와 별도변경대상을 가진 기본입력이다.
    for index, values in enumerate(itertools.product((0, 0x1000, 0x2000, 0x3000),
                                                    (0, 1, 0x8080, 0x10000),
                                                    (0, 0x80), (0, 1), ((0, 0), (3, 9), (7, 2)))):  # QA/보존flags/부모fallback/Robust/cursor를교차한다.
        flags1, content, container, robust, cursor = values  # 제품출력에서유도하지않은입력조합이다.
        result.append(dict(base, name=f'fields-{index:04d}',
                           forms=f'1:500:0:0:1;2:71:43981:32929:{robust};3:72:4660:65535:1',
                           types=f'500:0:0:{container};71:{flags1}:{content}:0;72:0:0:0;300:0:0:0;501:0:0:0',
                           position=cursor[0], end=cursor[1]))
    for index, children in enumerate(('-', '1:0', '1:8', '1:65535', '1:2,3', '1:2,2,3')):  # 빈 목록/경계 밖/일반 복수 후보를 관측한다.
        result.append(dict(base, name=f'candidates-{index:02d}', children=children))
    for count in (254, 255, 256, 257):  # countBYTE의 감김도 실제 CPU의 inc로 관측한다.
        result.append(dict(base, name=f'count-{count}', children='1:' + ','.join(['2'] * count)))
    for index, quantity in enumerate((0, 1, 127, 128, 255, 256, 32767, 32768, 65535)):  # BYTE/WORD의 주요 경계를 모두 공급한다.
        result.append(dict(base, name=f'quantity-{index:02d}', forms=f'1:500:0:0:1;2:300:{quantity}:32929:1;3:72:4660:65535:1',
                           types='500:0:0:0;71:0:0:0;72:0:0:0;300:12288:0:0;501:0:0:0'))
    tree = dict(base, forms='1:500:0:0:1;2:71:43981:128:1;3:72:4660:65535:1;4:300:32769:1:1;5:71:17:128:0',
                types='500:0:0:0;71:135168:128:0;72:8192:0:0;300:12288:0:0;501:0:0:0',
                children='1:2,4;2:3,5')  # 한 단계 재귀와 형제/Robust 불통과를 함께 가진 작은 트리다.
    for index, cursor in enumerate(((0, 0), (3, 15), (9, 2))):  # 재귀가같은stream을진행시키는모든부분출력을비교한다.
        result.append(dict(tree, name=f'tree-{index:02d}', position=cursor[0], end=cursor[1]))
    mutations = (
        'begin1/global/poollimit/2', 'begin1/finder/current/3', 'begin1/stream/data/1',
        'begin1/form1/kind/501;begin1/type501/container/128',
        'robust1/finder/current/3', 'robust1/form2/kind/72',
        'robust1/form2/quantity/65535', 'robust1/form2/listflags/128',
        'robust1/form2/robust/0', 'robust1/global/poollimit/2',
        'type1/finder/current/3', 'type1/form2/kind/72',
        'type1/form2/quantity/4660', 'type1/type71/flags1/12288',
        'type1/type71/content/128', 'type1/stream/data/1', 'type1/global/poollimit/2',
        'type2/finder/current/3', 'type2/form2/kind/72',
        'type2/form2/listflags/128', 'type2/type71/content/128',
        'type2/type71/flags1/131072',
        'parent1/finder/current/3', 'parent1/form1/kind/501;parent1/type501/container/128',
        'parent1/form2/listflags/128', 'parent1/stream/data/1',
        'next1/finder/current/2', 'next1/global/poollimit/3',
        'next1/form3/kind/300', 'next1/type72/flags1/12288',
    )  # 타입과 현재 커서의 callback 뒤 재조회/보관값을 구별하는 명시 효과다.
    for index, values in enumerate(itertools.product(mutations, (0, 0x1000, 0x2000, 0x3000))):  # 각 QA 조합에서 같은 callback의 재조회를 구별한다.
        mutation, flags1 = values  # 현재 경계 효과와 첫 타입 flags1의 독립 조합이다.
        result.append(dict(base, name=f'callback-{index:03d}', children='1:2,3', mutation=mutation,
                           types=f'500:0:0:0;71:{flags1}:0:0;72:0:0:0;300:0:0:0;501:0:0:0'))
    for index, mutation in enumerate(('assert1/stream/capacity/64', 'assert1/stream/data/1',
                                     'assert1/stream/position/5', 'assert1/stream/end/12',
                                     'assert1/form2/quantity/4660', 'assert2/form2/quantity/255',
                                     'assert1/finder/current/3', 'assert1/control/stop/1',
                                     'assert2/control/stop/1', 'assert3/control/stop/1',
                                     'assert4/control/stop/1')):  # 진단 전에 캡처한 값과 중단 시 부분 출력을 관측한다.
        result.append(dict(base, name=f'assert-{index:02d}', capacity=0,
                           types='500:0:0:0;71:12288:0:0;72:0:0:0;300:0:0:0;501:0:0:0', mutation=mutation))
    result.append(dict(base, name='header-no-capacity-check', capacity=0, children='-'))
    result.append(dict(tree, name='tree-current-change', mutation='robust2/finder/current/4'))
    result.append(dict(tree, name='tree-cached-container-flag', mutation='type2/type71/flags1/0'))
    result.append(dict(tree, name='tree-before-recursion-current-change', mutation='type2/finder/current/4'))
    result.append(dict(tree, name='tree-recursive-data-change', mutation='begin2/stream/data/1'))
    result.append(dict(tree, name='tree-child-capacity-stop', capacity=6, mutation='assert1/control/stop/1'))
    result.append(dict(base, name='kind-max-word', forms='1:500:0:0:1;2:65535:65535:65535:1;3:72:4660:65535:1',
                       types='500:0:0:0;65535:12288:0:0;72:0:0:0;300:0:0:0;501:0:0:0'))
    result.append(dict(base, name='robust-other-nonzero', forms='1:500:0:0:1;2:71:43981:32929:2;3:72:4660:65535:1'))
    return result


def main():
    """허용한 새 TSV/보고서만 생성하고 보호 원문 동일성을 검증한다."""
    before = sha256(PE_PATH)  # 실행 전 보호 PE의 읽기 전용 식별자다.
    if before != PE_SHA256:
        raise RuntimeError('확정한 보호 PE SHA256 불일치')
    oracle = Oracle()  # 실제 Contents 명령을 실행할 격리 장치다.
    inputs = cases()  # 결과를 포함하지 않는 새 독립 입력이다.
    rows = []  # 두 정밀도의 실제 관측 TSV 행이다.
    for case in inputs:  # 같은 독립 입력을 두 정밀도에서 실행한다.
        previous = None  # 첫 실행과둘째 실행의동일성비교용이다.
        for control in CONTROLS:  # 기대를합성하지않고각실행을온전히보존한다.
            try:
                output = oracle.run(case, control)  # 제품코드/기존fixture를호출하지않은실제PE관측이다.
            except Exception as error:
                raise RuntimeError(case['name']) from error
            if previous is not None and previous != output:
                raise RuntimeError('정밀도별 관측 차이: ' + case['name'])
            previous = output
            values = dict(case, name=f'{case["name"]}-cw{control:03x}', **output)  # 명시입력과실제관측을합친행이다.
            rows.append('\t'.join(str(values[column]) for column in SCHEMA))
    if sha256(PE_PATH) != before:
        raise RuntimeError('보호 PE 디스크 식별자 변경')
    header = '# Contents004bcea0 실제PE 관측. 앞10열입력/뒤12열관측. Finder/Robust/Type/ParentType/assert만명시경계다.\n#' + '\t'.join(SCHEMA) + '\n'  # 소비자가이름으로열을매핑할스키마다.
    FIXTURE_PATH.write_text(header + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    report = {
        'host': socket.gethostname(), 'date': '2026-10-23', 'inputs': len(inputs), 'precision_runs': len(rows),
        'precision_differences': 0, 'precision_observations': oracle.precisions,
        'schema': list(SCHEMA), 'input_columns': list(SCHEMA[:10]), 'observation_columns': list(SCHEMA[10:]),
        'entry': f'{ENTRY:08x}', 'end_exclusive': f'{NATIVE_END:08x}', 'native_sha256': hashlib.sha256(oracle.original).hexdigest(),
        'entry_contract': 'cdecl(parentSID,stream)。005c8464=고정FormPool。005c847c=현재물리limit。각Form은50BYTE이며+c=quantityWORD,+12=kindWORD,+14=listflagsWORD다.',
        'forms': 'sid:kind:quantityWORD:listflagsWORD:robust를세미콜론으로나눈다. 출력은같은입력순서이며변경후현재값이다.',
        'types': 'kind:flags1DWORD:contentFlagsDWORD:containerFlagsDWORD를세미콜론으로나눈다. 실제필드offset은e8/100/fc다.',
        'children': 'parent:child,child를세미콜론으로나눈다. 없는목록은-다. Begin/Next는재귀마다별도cursor를소유하며기본candidate순서는고정이다.',
        'mutation': 'trigger/target/field/value를세미콜론으로나눈다. trigger는종류별1기반호출번호(begin1,robust1,type2,parent1,next1,assert1)다. trace기록뒤명시효과를적용하고반환하며Robust반환값만효과전값을캡처한다. finder/current는현재재귀cursor의rawcurrentDWORD만바꾸며Next의후보목록index는바꾸지않는다.',
        'payloads': '두버퍼를cc로초기화한다. dataout은최종data의0/1이고payload0/1은각버퍼의최종end까지전체hex다. countBYTE의최초data+pos포인터가data변경뒤에도고정되는것을구별한다.',
        'trace': 'begin:parent;robust:sid;type:kind;parent:fixedParentSID;next:parent:current;assert:decimalSourceLine을실제CALL순서로기록한다.',
        'boundaries': {f'{address:08x}': name for address, name in BOUNDARIES.items()},
        'boundary_calls': dict(oracle.boundary_totals), 'native_write_counts': dict(oracle.write_totals),
        'scope': '원문이count예약/robust통과count증가/타입하위BYTE/QA BYTE·WORD/flags검증·부모fallback/현재cursor재조회/재귀/position,end를직접수행한다. 각실행의모든본체명령수를보존하고정상cdecl스택과callee-save/x87상태를확인한다.',
        'partial_failure': 'assert의control/stop/1효과는해당진입에서중단한다. completed0과음수stackdelta/부분payload는실제도달상태이나원본SEHunwind관측은아니다.',
        'limits': ['원문함수하나만관측하며상위Technology섹션생성/전체snapshot/실제파일/게임·GUI실행은포함하지않는다.',
                   f'물리Form{FORM_SLOTS}개/각출력{BUFFER_SIZE}BYTE/재귀깊이{MAX_DEPTH}/최대{MAX_INSTRUCTIONS}명령으로제한한다.',
                   '트리의재귀경로에동일parent가반복되면명시계약위반으로거부한다. 실제원문의순환방지나예외처리를구현한것이아니다.',
                   'Pool기준포인터는고정하고현재poollimit/currentForm/type필드/stream필드변경만공급한다.'],
        'new_game_runs': 0, 'new_decompilations': 0,
        'sha256': {str(path.relative_to(ROOT)).replace('\\', '/'): sha256(path) for path in (PE_PATH, Path(__file__), FIXTURE_PATH)},
    }  # 관측범위/명시경계/미검증부분을분리한보고서다.
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'native inputs={len(inputs)} precision_runs={len(rows)} differences=0')
    print('schema=' + '\t'.join(SCHEMA))
    print('fixture=' + str(FIXTURE_PATH.relative_to(ROOT)))
    print('report=' + str(REPORT_PATH.relative_to(ROOT)))


if __name__ == '__main__':  # 직접실행할때만허용한두새산출물을생성한다.
    main()
