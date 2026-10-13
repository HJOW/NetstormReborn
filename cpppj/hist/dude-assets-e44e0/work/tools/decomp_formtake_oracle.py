#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 실제 Form 생성/Take/Unpop/풀 초기화를 격리 실행해 C++ 대조 입력을 만든다.

사용법: formtake 읽기 전용 Ghidra 내보내기 후 python -X utf8 tools/decomp_formtake_oracle.py
게임/GUI/OS API는 실행하지 않는다. malloc의 확보 주소와 부모의 제거 통지만 명시 경계다.
"""
import csv
import hashlib
import json
import socket
import struct
import zlib
from decomp_sid_oracle import SidOracle, POOL, SPECS
from decomp_creation_oracle import CREATION, TYPES
from decomp_oracle import ROOT, STOP, STACK, SCRATCH
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 기존/성장 풀 크기와 외부 부모 알림용 합성 가상 표·함수 주소다.
CAPACITY, PARENT_TABLE, NOTICE = 32768, SCRATCH + 0x500, STOP + 0x100
# 내장 생성자·가상 표·공통 Unpop·malloc 경계는 실제 두 PE에서 읽은 주소다.
FORMS = {
    'originals': {'ctors': {1: 0x4ae760, 2: 0x4ae3d0, 3: 0x4ae9f0, 6: 0x4aea40},
        'vtables': {1: 0x512110, 2: 0x512098, 3: 0x512188, 6: 0x512208},
        'unpop': 0x4ae140, 'malloc': 0x43fa80},
    'originalCD': {'ctors': {1: 0x4af920, 2: 0x4af5c0, 3: 0x4afd60, 6: 0x4afe00},
        'vtables': {1: 0x505a08, 2: 0x505990, 3: 0x505a80, 6: 0x505b00},
        'unpop': 0x4af270, 'malloc': 0x48ef10},
}
# ABI가 보존해야 하는 네 레지스터에 설정하는 검사 값이다.
SAVED = {UC_X86_REG_EBX: 0x11223344, UC_X86_REG_ESI: 0x22334455,
    UC_X86_REG_EDI: 0x33445566, UC_X86_REG_EBP: 0x44556677}


def encode_slots(slots):
    """SID와 전체 raw 바이트를 TSV 한 열에 손실 없이 저장한다."""
    return ';'.join(f'{sid}:{raw.hex()}' for sid, raw in slots.items())


class FormOracle(SidOracle):
    """SID의 실제 코드 허용 범위에 Form의 새 Ghidra 범위를 추가한다."""
    def __init__(self, edition):
        """보호 PE를 읽고 실제 생성자 표만 합성 타입 입력에 놓는다."""
        super().__init__(edition)
        self.form_spec, self.creation = FORMS[edition], CREATION[edition]
        with (ROOT / f'extracted/formtake/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            # 불연속 함수 범위를 넓히지 않고 추가한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.facts.append(row)
                # 각 Ghidra 범위의 실제 명령만 허용한다.
                for span in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in span.split('-'))
                    self.allowed.append((low, high + 1))
        self.mu.mem_write(self.creation['type_global'], struct.pack('<I', TYPES))
        # 타입의 HP/zOrder는 쓰지 않고 실제 Form 생성자 주소만 배치한다.
        for number, ctor in self.form_spec['ctors'].items():
            self.mu.mem_write(TYPES + number * self.creation['type_stride'] + self.creation['ctor_offset'], struct.pack('<I', ctor))
        self.mu.mem_write(PARENT_TABLE + 0x24, struct.pack('<I', NOTICE))
        self.notices, self.allocations = [], []

    def on_instruction(self, mu, address, size, data):
        """외부 메모리 확보/부모 통지만 경계로 기록하고 나머지는 허용된 실제 명령을 실행한다."""
        if address == self.form_spec['malloc']:
            ret, size_arg = struct.unpack('<2I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP), 8))
            if size_arg != self.capacity * self.stride:
                raise RuntimeError('원본 malloc 요청 크기 불일치')
            self.allocations.append(size_arg)
            mu.reg_write(UC_X86_REG_EAX, POOL)
            mu.reg_write(UC_X86_REG_ESP, mu.reg_read(UC_X86_REG_ESP) + 4)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == NOTICE:
            esp = mu.reg_read(UC_X86_REG_ESP)
            ret, form, reason = struct.unpack('<3I', mu.mem_read(esp, 12))
            parent = (mu.reg_read(UC_X86_REG_ECX) - POOL) // self.stride
            if reason != 2:
                raise RuntimeError('ContentForm 부모 제거 통지 번호 불일치')
            self.notices.append(f'{parent}:{form}:{reason}:{self.slot(form).hex()}:{self.slot(parent).hex()}')
            mu.reg_write(UC_X86_REG_ESP, esp + 12)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        super().on_instruction(mu, address, size, data)

    def slot(self, sid):
        """원본 SID 위치의 전체 raw 바이트를 읽기 전용으로 저장한다."""
        return bytes(self.mu.mem_read(POOL + sid * self.stride, self.stride))

    def call_form(self, op, type_id, arg):
        """cdecl/thiscall의 정상 EIP/ESP와 보존 레지스터를 확인한다."""
        self.notices, self.allocations, self.instructions = [], [], 0
        # 호출자 입력은 매번 독립적으로 놓고 방향 플래그를 0으로 설정한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX):
            self.mu.reg_write(register, 0)
        # 피호출자가 보존할 레지스터는 0이 아닌 값으로 확인한다.
        for register, value in SAVED.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<3I', STOP, type_id, arg & 0xffffffff))
        if op == 'Unpop':
            address, expected_esp = self.form_spec['unpop'], STACK + 8
            self.mu.reg_write(UC_X86_REG_ECX, POOL + arg * self.stride)
        elif op == 'Reset':
            address, expected_esp = self.entries['Reset'], STACK + 4
            self.mu.mem_write(STACK + 4, struct.pack('<I', arg & 0xffffffff))
        else:
            address, expected_esp = self.creation['entries'][op], STACK + 4
        self.mu.emu_start(address, STOP, timeout=30000000, count=10000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != expected_esp:
            raise RuntimeError(f'{self.edition} {op}: 정상반환/ABI 오류')
        # 반환 뒤 각 보존 레지스터를 입력값에 직접 비교한다.
        for register, value in SAVED.items():
            if self.mu.reg_read(register) != value:
                raise RuntimeError(f'{op}: 보존 레지스터 오류')
        if op not in ('Create', 'Take'):
            return 0
        result = self.mu.reg_read(UC_X86_REG_EAX)
        if (result - POOL) % self.stride:
            raise RuntimeError('Form 반환 주소 정렬 오류')
        return (result - POOL) // self.stride

    def metadata(self):
        """전체 풀과 주요 카운터/머리/꼬리를 독립 결과로 저장한다."""
        glob = self.spec['globals']
        capacity, free, cursor = [struct.unpack('<I', self.mu.mem_read(glob[index], 4))[0] for index in (6, 5, 7)]
        client_head, server_head = [struct.unpack('<H', self.mu.mem_read(POOL + offset, 2))[0] for offset in (4, 18)]
        client_tail, server_tail = [(struct.unpack('<I', self.mu.mem_read(glob[index], 4))[0] - POOL) // self.stride for index in (2, 4)]
        return [capacity, free, cursor, client_head, client_tail, server_head, server_tail,
            zlib.adler32(self.mu.mem_read(POOL, capacity * self.stride))]


def generate():
    """실제 PE에서 생성/Take/링크 해제/크기 재확보의 600개 입력 결과를 저장한다."""
    rows, reports = ['# edition op type arg server inputs outputs return capacity free cursor ch ct sh st adler notices'], {}
    # 각 PE를 별도 주소 공간에서 실행하고 외부 경계를 섞지 않는다.
    for edition, source in FORMS.items():
        oracle = FormOracle(edition)
        count = 0
        # 권한 유무와 client/서버 번호 양쪽을 검증한다.
        for server in (False, True):
            oracle.begin(CAPACITY, server)
            oracle.step('Reset')
            baseline = bytes(oracle.mu.mem_read(POOL, CAPACITY * oracle.stride))
            globals_before = {address: bytes(oracle.mu.mem_read(address, 4)) for address in oracle.spec['globals']}

            def record(op, number, arg, inputs):
                """C++를 호출하지 않고 명시 입력과 실제 PE의 raw/통지/메타데이터를 한 행에 저장한다."""
                nonlocal count
                oracle.capacity = arg if op == 'Reset' and arg > 0 else CAPACITY
                oracle.mu.mem_write(POOL, baseline)
                # 매 루트 호출은 같은 원본 초기화 결과에서 시작한다.
                for address, value in globals_before.items():
                    oracle.mu.mem_write(address, value)
                oracle.write_ranges = [(POOL, POOL + oracle.capacity * oracle.stride), (0x30000000, 0x30010000)]
                oracle.write_ranges.extend((address, address + 4) for address in oracle.spec['globals'][:8])
                # fixture의 합성 입력만 직접 놓으며 원본 함수의 실행 결과와 구분한다.
                for sid, raw in inputs.items():
                    oracle.mu.mem_write(POOL + sid * oracle.stride, bytes(raw))
                result = oracle.call_form(op, number, arg)
                watch = list(inputs) if inputs else ([result] if op == 'Create' else [])
                output = {sid: oracle.slot(sid) for sid in watch if sid < oracle.capacity}
                rows.append('\t'.join(map(str, [edition, op, number, arg, int(server), encode_slots(inputs),
                    encode_slots(output), result, *oracle.metadata(), ';'.join(oracle.notices)])))
                if op == 'Reset' and (len(oracle.allocations) != int(arg > 0)):
                    raise RuntimeError('양수/기존 풀 초기화의 확보 횟수 불일치')
                count += 1

            # 네 내장 Form 생성자는 플래그별 실제 Allocate와 실제 빈 postCreate를 실행한다.
            for number in source['ctors']:
                # 권한 없는 서버 할당 flags 0은 제외한다.
                for flags in ((0, 1, 2, 3) if server else (1, 2, 3)):
                    record('Create', number, flags, {})
            # Free Take는 잘린 초기화가 아니며 예측 머리와 마지막 번호도 원본 범위 안이다.
            for number in source['ctors']:
                # free/dead/추가 상태 비트의 보존을 확인한다.
                for state in (1, 3, 0x89):
                    # 서버 머리·예측 머리·마지막 슬롯 모두 원본 Take 범위다.
                    for sid in (oracle.spec['server_first'], oracle.spec['predictable'], CAPACITY - 1):
                        raw = bytearray((number * 29 + index * 37) & 255 for index in range(oracle.stride))
                        raw[10], raw[11] = 90, state
                        record('Take', number, sid, {sid: raw})
            # live Take/Unpop은 머리/중간·Form/자산 혼합 링크·부모 free/dead를 직접 대조한다.
            for number in source['ctors']:
                # 살아 있는 부모와 free/dead 부모의 서로 다른 해제를 확인한다.
                for parent_state in (0, 1, 2):
                    # 자신이 머리인지 중간인지에 따른 head/previous 쓰기를 확인한다.
                    for has_previous in (False, True):
                        # 다음 항목의 이전 링크가 +14/+16 중 어느 위치인지 확인한다.
                        for asset_next in (False, True):
                            form, parent, previous, following = oracle.spec['server_first'] + 31, 5, oracle.spec['server_first'] + 32, oracle.spec['server_first'] + 33
                            inputs = {sid: bytearray((number * 17 + index * 37) & 255 for index in range(oracle.stride)) for sid in (form, parent, previous, following)}
                            # 네 슬롯만 합성 입력으로 오염시키고 전체 풀 체크섬도 확인한다.
                            for sid, type_id, vtable, state in [(form, number, source['vtables'][number], 0x88),
                                    (parent, 90, PARENT_TABLE, parent_state), (previous, 3, source['vtables'][3], 8),
                                    (following, 80 if asset_next else 3, oracle.creation['vtable'] if asset_next else source['vtables'][3], 8)]:
                                struct.pack_into('<I', inputs[sid], 0, vtable)
                                inputs[sid][10], inputs[sid][11] = type_id, state
                            struct.pack_into('<H', inputs[form], 4, following)
                            struct.pack_into('<H', inputs[form], 14, previous if has_previous else 0)
                            struct.pack_into('<H', inputs[form], 16, parent)
                            struct.pack_into('<H', inputs[parent], 6, previous if has_previous else form)
                            struct.pack_into('<H', inputs[previous], 4, form)
                            struct.pack_into('<H', inputs[following], 16 if asset_next else 14, form)
                            # 같은 입력의 수신 재생성과 단독 해제를 별도 실제 루트로 실행한다.
                            for op in ('Take', 'Unpop'):
                                record(op, number, form, inputs)
            # 양수는 확보 경계를 거치고 0/음수는 같은 크기에서 전체 raw를 초기화한다.
            for size in (0, -7, 32770, oracle.spec['predictable'] + 65):
                record('Reset', 0, size, {15: bytes([0xd7]) * oracle.stride})
        reports[edition] = {'sha256': oracle.sha256, 'cases': count, 'assertions': oracle.assertions,
            'boundaries': ['CRT malloc: 요청 크기를 기록하고 격리 풀을 반환', '부모 가상+24: raw/이유 2 기록'],
            'functions': oracle.facts}
    fixture = ROOT / 'cpppj/tests/fixtures/formtake-x86.tsv'
    fixture.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    report = {'last_decompile_host': socket.gethostname(), 'recorded_date': '2026-10-13',
        'method': '실제 두 PE의 Form/공통 Unpop/SID 명령을 정상 반환까지 실행. C++를 호출하지 않고 600개 입력 저장.',
        'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(), 'editions': reports}
    (ROOT / 'cpppj/formtake.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('Form 원본 관찰 완료:', {edition: report['cases'] for edition, report in reports.items()})


if __name__ == '__main__':
    generate()
