#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tell의 두 맵 sentinel 기본블록만 실제 PE에서 실행해 독립 TSV로 관측한다.

행 작성004cb210·목록 갱신004cafd0·메뉴 시작00477bb0·CRT 비교004e65ed는
명시 외부 경계다. 이 도구는 그 하위 몸체나 실제 게임 실행을 검증하지 않는다.
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


ROOT = Path(__file__).resolve().parents[1]  # tools에서 올라간 저장소 루트다.
PE_PATH = ROOT / 'originals/Netstorm.exe'  # 읽기 전용 10.78 보호 PE다.
FIXTURE_PATH = ROOT / 'cpppj/tests/fixtures/tell-map-menus-x86.tsv'  # 새 독립 원문만 저장한다.
REPORT_PATH = ROOT / 'extracted/cpp-tell-map-menus-20261023-native.json'  # 새 실행 관측 보고서다.
IMAGE_BASE = 0x400000  # 보호 PE가 가정하는 원래 이미지 기준 주소다.
ENTRY = 0x4C86FE  # EBX=this, ESP+b4=label로 시작하는 실제 $Menu 분기다.
MAP_MERGE = 0x4C8B77  # 두 맵 경로의 정상 합류 직전 관측 종료점이다.
NOT_MAP = 0x4C88C5  # 두 비교가 모두 실패한 직후이며 다음 sentinel은 실행하지 않는다.
BEGIN_MENU = 0x477BB0  # BeginVertical(flags) 명시 cdecl 경계다.
REFRESH_LIST = 0x4CAFD0  # Refresh(mode) 명시 cdecl 경계다.
MAKE_ROW = 0x4CB210  # Row(index pointer) 명시 cdecl 경계다.
STRICMP = 0x4E65ED  # ASCII CRT 대소문자 비교의 명시 cdecl 경계다.
KEY = 0x5327AC  # 같은9BYTE를 덮어쓰는 실제 FileSpec 키 버퍼다.
DIRTY = 0x542678  # 실제 목록 dirty DWORD다.
COUNT = 0x5CAD20  # 실제 목록 count signed DWORD다.
HEAP = 0x1000000  # 호스트 입력 this를 두는 격리 영역이다.
STACK = 0x2100000  # 기존 함수 프레임을 대신 공급하는 격리 ESP다.
THIS = HEAP + 0x1000  # 원문이 EBX로 읽는 현재 Tell 객체의 고정 주소다.
MENU_OFFSET = 0x21C8  # 원문 Tell 객체의 현재 controlled 메뉴 개수 위치다.
LABEL_OFFSET = 0xB4  # 지정 기본블록 진입 시 label 문자열의 스택 위치다.
KEY_BYTES = 9  # fortSpec/SOLOSpec의8문자와 마지막 NUL을 포함한다.
MAX_ROWS = 100  # 잘못된 외부 반환 공급의 무한 반복을 명시 거부한다.
MAX_INSTRUCTIONS = 10000  # 각 입력의 비정상 제어 흐름을 성공으로 간주하지 않는다.
CONTROLS = (0x27F, 0x37F)  # 두 x87 제어어를 각 입력에서 별도로 실행한다.
COUNTS = (0, 1, 15, 16, 17, 31, 40, 50)  # 빈 목록·열 경계·원본 상한을 교차한다.
LABELS = ('*SOLOLIST*', '*sololist*', '*FORTLIST*', '*FoRtLiSt*', '*FORTS*', '*UNKNOWN*')  # 두 실제 이름과 비맵 입력이다.
INITIAL_KEYS = ('fortSpec', 'SOLOSpec', 'initial!')  # 호출 전 현재 키가 이미 달라도 원문 쓰기만 관측한다.
MUTATIONS = ('begin-menus', 'row-count', 'row-state')  # 외부 경계가 명시적으로 현재 자료를 바꾸는 세 입력이다.
SCHEMA = (
    'name', 'label', 'initialmenus', 'initialdirty', 'initialkey', 'count',
    'mutation', 'handled', 'menus', 'dirty', 'key', 'trace', 'stackdelta',
)  # 앞7열은 독립 입력이고 뒤6열은 실제 기본블록/명시 경계의 관측이다.
BOUNDARIES = {
    BEGIN_MENU: 'BeginVertical', REFRESH_LIST: 'Refresh', MAKE_ROW: 'Row', STRICMP: 'stricmp',
}  # 이 네 호출만 본체 대신 명시 장치로 반환한다.


def sha256(path):
    """파일 전체 SHA256을 읽기 전용으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fold_ascii(value):
    """현재 CRT 기본 로케일에 맞춰 ASCII 대문자만 접는다."""
    result = bytearray()
    for byte in value:  # 원본 C문자열의 BYTE 순서와 비ASCII 바이트를 보존한다.
        result.append(byte + 32 if 65 <= byte <= 90 else byte)
    return bytes(result)


class Oracle:
    """명시된 PE 기본블록과 실제 전역·스택 쓰기만 Unicorn으로 실행한다."""

    def __init__(self):
        """보호 PE를 복사한 메모리와 다른 격리 this/스택을 준비한다."""
        self.pe = pefile.PE(str(PE_PATH))  # 기존 파일은 수정하지 않는 입력 이미지다.
        if self.pe.OPTIONAL_HEADER.ImageBase != IMAGE_BASE:
            raise RuntimeError('보호 PE 원래 기준 주소 불일치')
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 원본32비트 명령 실행기다.
        self.image_size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095  # PE 매핑의 페이지 정렬 크기다.
        self.mu.mem_map(IMAGE_BASE, self.image_size)
        self.mu.mem_write(IMAGE_BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x10000)
        self.mu.mem_map(STACK - 0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.memory_write)
        self.original_code = bytes(self.mu.mem_read(ENTRY, NOT_MAP - ENTRY))  # 원문 바이트 불변 확인용이다.
        self.boundary_totals = Counter()  # 두 정밀도의 실제 경계 호출 횟수다.
        self.native_write_totals = Counter()  # CPU가 원문 명령으로 수행한 쓰기만 센다.
        self.precisions = {}  # 제어어마다 독립 실행/명령 수를 보관한다.

    def dword(self, address, value=None):
        """비정렬 little endian DWORD를 읽고 명시 입력일 때만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def c_string(self, address, limit=256):
        """현재 첫 NUL 앞까지 읽고 물리 문자열 한도 초과를 거부한다."""
        result = bytearray()
        for index in range(limit):  # 원본이 실제 보는 NUL 종료 바이트만 읽는다.
            byte = self.mu.mem_read(address + index, 1)[0]
            if not byte:
                return bytes(result)
            result.append(byte)
        raise RuntimeError('현재 원문 문자열의 NUL/물리 범위 누락')

    def write_key(self, key):
        """초기 입력/명시 Row 효과에서만 같은9BYTE 키를 제공한다."""
        raw = key.encode('ascii')
        if len(raw) != KEY_BYTES - 1:
            raise RuntimeError('명시 FileSpec 키8문자 계약 위반')
        self.mu.mem_write(KEY, raw + b'\0')

    def returned(self, value=0):
        """명시 cdecl 경계의 반환 주소만 소비하고 인자 정리는 원문에 맡긴다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 외부 호출의 실제 진입 스택이다.
        target = self.dword(esp)  # 원본 CALL이 저장한 반환 주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def memory_write(self, mu, access, address, size, value, data):
        """원문 CPU 쓰기를 지정 전역·Tell 메뉴수·격리 스택으로 제한한다."""
        if KEY <= address and address + size <= KEY + KEY_BYTES:
            category = 'key'
        elif DIRTY <= address and address + size <= DIRTY + 4:
            category = 'dirty'
        elif THIS + MENU_OFFSET <= address and address + size <= THIS + MENU_OFFSET + 4:
            category = 'menus'
        elif STACK - 64 <= address and address + size <= STACK + 0xB4:
            category = 'stack'
        else:
            raise RuntimeError(f'원문 기본블록의 예상하지 않은 쓰기 {address:08x}/{size}')
        self.native_write_totals[category] += 1

    def instruction(self, mu, address, size, data):
        """본체 분기를 실행하고 네 외부 효과와 두 종료점만 가로챈다."""
        self.instructions += 1
        if address in (MAP_MERGE, NOT_MAP):
            self.stop = address
            mu.emu_stop()
            return
        if ENTRY <= address < NOT_MAP:
            self.native_instructions += 1
            return
        if address not in BOUNDARIES:
            raise RuntimeError(f'격리 범위 밖의 미명시 코드 {address:08x}')
        self.boundary_totals[BOUNDARIES[address]] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)  # 외부 호출이 실제 전달한 스택 인자를 읽는다.
        first = self.dword(esp + 4)  # 첫cdecl 인자다.
        if address == STRICMP:
            second = self.dword(esp + 8)  # 두 번째 비교 문자열 포인터다.
            left = fold_ascii(self.c_string(first))  # 원문 label의 현재 바이트다.
            right = fold_ascii(self.c_string(second))  # 보호 PE의 실제 sentinel 리터럴이다.
            self.returned((left > right) - (left < right))
        elif address == BEGIN_MENU:
            self.trace.append(f'begin:{first}')
            self.begin_calls += 1
            if self.case['mutation'] == 'begin-menus' and self.begin_calls == 1:
                self.dword(THIS + MENU_OFFSET, 5)  # 명시 callback 효과이며 이후 증가 쓰기는 원문이 수행한다.
            self.returned()
        elif address == REFRESH_LIST:
            self.trace.append(f'refresh:{first}:{self.c_string(KEY).decode("ascii")}:{self.dword(DIRTY)}')
            self.refresh_calls += 1
            self.dword(COUNT, self.case['count'])  # 독립 입력인 외부 목록 결과다.
            self.dword(DIRTY, 0)  # 명시 Refresh 완료 효과이며 이후 sentinel 쓰기를 대신하지 않는다.
            self.returned(self.case['count'])
        elif address == MAKE_ROW:
            self.row_calls += 1
            if self.row_calls > MAX_ROWS:
                raise RuntimeError('명시 Row 호출100회 한도 초과')
            index = struct.unpack('<i', mu.mem_read(first, 4))[0]  # 원문이 초기화/증가한 실제 index다.
            count = struct.unpack('<i', mu.mem_read(COUNT, 4))[0]  # callback 시점의 현재 목록 개수다.
            self.trace.append(f'row:{index}')
            success = index < count  # 실제 Row 몸체 대신 명시 공급하는 반환 규칙이다.
            if success:
                self.successful_rows += 1
                if self.successful_rows == 1 and self.case['mutation'] == 'row-count':
                    self.dword(COUNT, 32)  # 첫 성공 뒤 count 변경을 원문 루프가 다시 읽게 한다.
                elif self.successful_rows == 1 and self.case['mutation'] == 'row-state':
                    self.write_key('changed!')  # 외부 Row의 현재 키 변경 효과다.
                    self.dword(DIRTY, 7)  # 외부 Row의 현재 dirty 변경 효과다.
            self.returned(int(success))

    def run(self, case, control):
        """한 입력/정밀도의 endpoint·현재 전역·호출 순서를 독립 관측한다."""
        self.case = case  # 결과를 포함하지 않은 독립 입력이다.
        self.trace = []  # 관측 순서대로만 채우는 명시 외부 이벤트다.
        self.stop = None  # 실제 endpoint에 도달해야 정상 관측이다.
        self.instructions = 0  # endpoint/외부 진입을 포함한 전체 hook 횟수다.
        self.native_instructions = 0  # 실제 지정 PE 본체 명령만 센다.
        self.begin_calls = 0  # 첫 Begin 효과를 다른 Begin과 구분한다.
        self.refresh_calls = 0  # 지정 영역에서 실제 발생한 Refresh 요청 수다.
        self.row_calls = 0  # terminal0 반환 요청도 포함하는 Row 호출 수다.
        self.successful_rows = 0  # 첫 성공에만 외부 메모리 변경을 공급한다.
        self.mu.mem_write(STACK - 64, bytes([0xCC]) * 0x400)
        self.mu.mem_write(THIS, bytes(0x2200))
        self.mu.mem_write(STACK + LABEL_OFFSET, case['label'].encode('ascii') + b'\0')
        self.dword(THIS + MENU_OFFSET, case['initialmenus'])
        self.dword(DIRTY, case['initialdirty'])
        self.dword(COUNT, case['count'])
        self.write_key(case['initialkey'])
        registers = {
            UC_X86_REG_EBX: THIS, UC_X86_REG_ESP: STACK,
            UC_X86_REG_EAX: 0x10111213, UC_X86_REG_ECX: 0x20212223,
            UC_X86_REG_EDX: 0x30313233, UC_X86_REG_ESI: 0x40414243,
            UC_X86_REG_EDI: 0x50515253, UC_X86_REG_EBP: 0x60616263,
            UC_X86_REG_FPCW: control, UC_X86_REG_FPSW: 0,
        }  # 온전한 함수 호출이 아닌 기본블록의 명시 현재 레지스터다.
        for register, value in registers.items():  # 이전 입력의 레지스터 잔여값을 분리한다.
            self.mu.reg_write(register, value)
        self.mu.emu_start(ENTRY, MAP_MERGE + 1, count=MAX_INSTRUCTIONS)
        if self.stop not in (MAP_MERGE, NOT_MAP):
            raise RuntimeError('실제 sentinel endpoint 미도달')
        if self.mu.reg_read(UC_X86_REG_EIP) != self.stop or self.mu.reg_read(UC_X86_REG_ESP) != STACK:
            raise RuntimeError('원문 기본블록 endpoint/스택 정리 불일치')
        if self.mu.reg_read(UC_X86_REG_EBX) != THIS:
            raise RuntimeError('원문 현재 Tell 객체 레지스터 변경')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('원문 기본블록 x87 제어어/스택 불일치')
        if bytes(self.mu.mem_read(ENTRY, NOT_MAP - ENTRY)) != self.original_code:
            raise RuntimeError('보호 PE 본체 바이트 변경')
        totals = self.precisions.setdefault(f'{control:03x}', {'runs': 0, 'native_instructions': 0, 'hook_events': 0})
        totals['runs'] += 1
        totals['native_instructions'] += self.native_instructions
        totals['hook_events'] += self.instructions
        return {
            'handled': int(self.stop == MAP_MERGE),
            'menus': self.dword(THIS + MENU_OFFSET), 'dirty': self.dword(DIRTY),
            'key': self.c_string(KEY).decode('ascii'), 'finalcount': self.dword(COUNT),
            'trace': ';'.join(self.trace), 'stackdelta': self.mu.reg_read(UC_X86_REG_ESP) - STACK,
        }


def cases():
    """일반 경계와 명시 변경을 기대 출력 없는 독립 입력으로 교차한다."""
    result = []  # 기대 출력 없이 준비하는 입력 목록이다.
    for index, values in enumerate(itertools.product(COUNTS, (0, 3), LABELS, (0, 9), INITIAL_KEYS)):  # 기본 상태/키/대소문자를 교차한다.
        count, menus, label, dirty, key = values
        result.append(dict(name=f'fixed-{index:04d}', label=label, initialmenus=menus,
                           initialdirty=dirty, initialkey=key, count=count, mutation='fixed'))
    for index, values in enumerate(itertools.product(COUNTS, (0, 3), LABELS, MUTATIONS)):  # 선택/미선택 callback 효과와 경계 개수를 교차한다.
        count, menus, label, mutation = values
        result.append(dict(name=f'mutation-{index:04d}', label=label, initialmenus=menus,
                           initialdirty=9, initialkey='initial!', count=count, mutation=mutation))
    return result


def main():
    """독립 입력마다 두 실제 실행을 수행한 뒤 새 세 파일 범위에만 기록한다."""
    before = sha256(PE_PATH)  # 실행 전 보호 PE 원문을 보존 확인한다.
    oracle = Oracle()  # 원본 주소를 유지한 격리 실행기다.
    fixture_rows = []  # 평문 TSV의 각 정밀도별 독립 관측이다.
    inputs = cases()  # 제품 계산이나 기존 fixture를 읽지 않는 입력이다.
    for case in inputs:  # 모든 입력은 두 x87 제어어에서 별도로 실행한다.
        observed = None  # 두 번째 정밀도 비교용 첫 실제 결과다.
        for control in CONTROLS:  # 정밀도별 행을 합치지 않고 각각 동결한다.
            output = oracle.run(case, control)
            if observed is not None and output != observed:
                raise RuntimeError(f'정밀도별 실제 관측 차이: {case["name"]}')
            observed = output
            values = dict(case, name=f'{case["name"]}-cw{control:03x}', **output)
            fields = []  # 입력/기대 열을 명시 스키마 순서로 나열한다.
            for column in SCHEMA:  # TSV에 숨은 JSON이나 escaped 줄바꿈을 넣지 않는다.
                fields.append(str(values[column]))
            fixture_rows.append('\t'.join(fields))
    if sha256(PE_PATH) != before:
        raise RuntimeError('보호 PE 입력 파일 SHA 변경')
    header = '# Tell 맵 sentinel 기본블록의 독립 원문 관측. 앞7열 입력/뒤6열 관측이며 하위 Row·Refresh·Begin·CRT는 명시 경계다.\n'
    header += '#' + '\t'.join(SCHEMA) + '\n'
    FIXTURE_PATH.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE_PATH.write_text(header + '\n'.join(fixture_rows) + '\n', encoding='utf-8', newline='\n')
    boundary_addresses = {}  # 같은 명시 경계의 실제 PE 주소를 보고서에 남긴다.
    for address, name in BOUNDARIES.items():  # 하위 몸체 실행으로 오해하지 않도록 경계를 별도 표기한다.
        boundary_addresses[f'{address:08x}'] = name
    hashes = {}  # 보호 PE/도구/생성 원문의 현재 SHA를 고정한다.
    for path in (PE_PATH, Path(__file__), FIXTURE_PATH):  # 자기 보고서의 순환 SHA 의존은 포함하지 않는다.
        hashes[str(path.relative_to(ROOT)).replace('\\', '/')] = sha256(path)
    report = {
        'host': socket.gethostname(), 'date': '2026-10-23', 'inputs': len(inputs),
        'precision_runs': len(fixture_rows), 'precision_observations': oracle.precisions,
        'precision_differences': 0, 'schema': list(SCHEMA), 'input_columns': list(SCHEMA[:7]),
        'observation_columns': list(SCHEMA[7:]), 'precision_row_names': 'name 끝의 -cw27f/-cw37f로 각 실행을 구별한다.',
        'entry': f'{ENTRY:08x}', 'endpoints': {'handled': f'{MAP_MERGE:08x}', 'not_map': f'{NOT_MAP:08x}'},
        'entry_contract': 'EBX=this; ESP+b4 is label bytes; this+21c8 is current menu count',
        'native_code': {'first': f'{ENTRY:08x}', 'end_exclusive': f'{NOT_MAP:08x}',
                        'bytes': len(oracle.original_code), 'sha256': hashlib.sha256(oracle.original_code).hexdigest()},
        'globals': {'key': f'{KEY:08x}/9BYTE', 'dirty': f'{DIRTY:08x}/DWORD', 'count': f'{COUNT:08x}/signed DWORD'},
        'boundaries': boundary_addresses,
        'boundary_calls': dict(oracle.boundary_totals), 'native_write_counts': dict(oracle.native_write_totals),
        'mutations': {
            'fixed': '외부 경계는 현재 자료를 추가 변경하지 않는다. Refresh만 입력 count/DIRTY0을 공급한다.',
            'begin-menus': '첫 실제 Begin 경계가 this+21c8을5로 쓴다. 후속 증가 쓰기는 원문이 수행한다.',
            'row-count': '첫 성공 Row 반환을 공급하기 전에 현재 count를32로 바꾼다.',
            'row-state': '첫 성공 Row 반환을 공급하기 전에 키를changed!/NUL,dirty를7로 바꾼다.',
        },
        'row_boundary': 'index pointer는 원문이 초기화/증가한다. 외부 반환은 현재 index<count이며 최대100회다. 004cb210 몸체는 실행하지 않는다.',
        'scope': '실제 PE 기본블록이 비교 분기·dirty/키 쓰기·메뉴 증가·현재 개수 재조회·분할·반복·정상 복구를 실행한다.',
        'limits': ['전체004c83b0/Tell·메뉴 생성·파일 목록·Row 몸체 검증이 아니다.',
                   '실제 게임/GUI/네트워크 실행이 아니며 새 디컴파일은 없다.',
                   '호스트 예외의 전역 복구 정책과 원본 SEH는 이 정상 경로 관측 범위 밖이다.'],
        'new_game_runs': 0, 'new_decompilations': 0,
        'sha256': hashes,
    }  # 자기 자신의 JSON SHA를 넣지 않아 보고서의 순환 의존을 피한다.
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'native inputs={len(inputs)} precision_runs={len(fixture_rows)} differences=0')
    print('schema=' + '\t'.join(SCHEMA))
    print('fixture=' + str(FIXTURE_PATH.relative_to(ROOT)))
    print('report=' + str(REPORT_PATH.relative_to(ROOT)))


if __name__ == '__main__':  # 직접 호출 때만 새 원문 관측 파일을 생성한다.
    main()
