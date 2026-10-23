#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 10.78 Money 부모·난수·float32·일반 stream 쓰기를 독립 x86로 동결한다."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import socket
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호 입력과 새 출력의 저장소 루트다.
PE = ROOT / 'originals/Netstorm.exe'  # OS/게임 진입 없이 읽기만 하는 10.78 PE다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 검토한 보호 PE 전체 식별자다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-money-save-x86.tsv'  # 최초 관측을 덮어쓰지 않을 독립 fixture다.
REPORT = ROOT / 'cpppj/fort-money-save-native.json'  # 실행 범위와 보호/도구/fixture SHA 보고서다.
BASE, HEAP, STACK, STOP = 0x400000, 0x14000000, 0x15008000, 0x1400F000  # PE·독립 물리 장치·스택·반환 주소다.
RANDOM, OWNER = 0x532710, 0x540CAC  # 실제 공유 게임난수와 독립 부모가 읽지 않는 isMine 전역이다.
STREAMS, BUFFERS = (HEAP + 0x100, HEAP + 0x120), (HEAP + 0x1000, HEAP + 0x1100)  # 두 실제16BYTE stream과 각64BYTE 버퍼다.
RANGES = {'money': (0x4832C0, 0x4832F2), 'parent': (0x4BEEC0, 0x4BEED4),
          'random': (0x4558C0, 0x4558E8), 'write': (0x44CCE0, 0x44CD3E)}  # 허용한 온전한 네 원문 함수다.
REGISTERS = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)  # 매 입력 초기화할 정수 레지스터다.
CONTROLS = (0x27F, 0x37F)  # 실제 FILD/FADD/FSTP를 두 x87 정밀도로 반복한다.
SCHEMA = ('name', 'kind', 'seed', 'slot', 'owner', 'manager', 'selected', 'position', 'end', 'capacity', 'mutation',
          'completed', 'randomout', 'managerout', 'afields', 'bfields', 'abuffer', 'bbuffer', 'trace', 'stackdelta', 'nativeinstructions')  # 앞11열은 입력이며 뒤10열은 실제 PE 관측이다.


def digest(path):
    """파일 전체 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """실제 난수로 모든1010개 잔여와 부모 슬롯/현재 stream/부분 중단을 관측한다."""
    rows = [dict(seed=seed) for seed in range(1, 1011)]  # 각 결과의 기대 float는 계산하지 않고 실제 PE에 입력한다.
    rows.extend(dict(seed=seed) for seed in (0, 0xFFFFFFFF, 0x80000000, 0x12345678, 0x0BAD0BAD))
    for slot in (-2, -1, 0, 1, 8, 9, 10):  # 배열 밖 this도 부모 자체에는 owner clamp/isMine 진단이 없음을 구별한다.
        for owner in (0, 1, 8, 0xFFFFFFFF):
            rows.append(dict(kind='parent', slot=slot, owner=owner, manager=0 if owner & 1 else 0x12345678, selected=slot & 1))
    for kind in ('money', 'parent'):  # 일반 쓰기 진단과 실제 필드 재조회는 두 진입에서 모두 실행한다.
        for position, end, capacity in ((0, 0, 0), (1, 0, 4), (4, 40, 64), (31, 0x80000000, 64), (0, 0, 0xFFFFFFFF)):
            rows.append(dict(kind=kind, position=position, end=end, capacity=capacity))
        for event in (('manager1', 'create1') if kind == 'parent' else ()) + ('write1', 'assert1'):
            prefix = dict(kind=kind, capacity=0 if event == 'assert1' else 64)
            for target, value in (('random', 0), ('manager', 0), ('a.pos', 7), ('a.end', 0x80000000),
                                  ('a.cap', 64), ('a.data', BUFFERS[1]), ('b.pos', 13)):
                rows.append(dict(prefix, mutation=f'{event}/{target}/{value}'))
            rows.append(dict(prefix, mutation=f'{event}/stop/1'))
        rows.extend((dict(kind=kind, capacity=0, mutation=f'assert1/a.pos/9;assert1/a.data/{BUFFERS[1]};assert1/a.end/3;assert1/random/0'),
                     dict(kind=kind, selected=1, position=7, capacity=0, mutation='assert1/b.pos/11;assert1/b.end/4294967295')))
    result, seen = [], set()  # 앞11열 입력만으로 중복을 제거한다.
    for row in rows:  # 생략된 명시 입력을 고정하며 기대값은 제품에서 가져오지 않는다.
        item = dict(kind='money', seed=0x12345678, slot=1, owner=1, manager=0x12345678,
                    selected=0, position=0, end=0, capacity=64, mutation='-')
        item.update(row)
        key = tuple(item.values())
        if key not in seen:
            seen.add(key);item['name'] = f'money-{len(result):04}';result.append(item)
    return result


class Oracle:
    """실제 네 함수와 명시 Create/진단만 허용하는 격리 x86 장치다."""
    def __init__(self):
        """보호 PE 읽기와 별도 메모리만 준비하며 OS 함수를 실행하지 않는다."""
        if digest(PE) != PE_SHA:
            raise RuntimeError('보호 PE SHA 불일치')
        self.pe = pefile.PE(str(PE));self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 보호 입력 파서와 독립 CPU다.
        self.mu.mem_map(BASE, (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.mu.mem_write(BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x10000);self.mu.mem_map(STACK - 0x8000, 0x10000)
        self.original = {start: bytes(self.mu.mem_read(start, end - start)) for start, end in RANGES.values()}  # 함수 원문 불변 확인용이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.total_instructions = 0;self.normal_returns = 0;self.boundaries = Counter();self.values = set()  # 실제 명령/정상 반환/하위 경계/float 비트 관측이다.

    def number(self, address, value=None):
        """raw DWORD를 읽거나 명시 입력/효과로 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))
        return int.from_bytes(self.mu.mem_read(address, 4), 'little')

    def boundary(self, event, text):
        """실제 인자를 먼저 캡처한 뒤 해당 명시 외부효과만 적용한다."""
        self.calls[event] += 1;self.boundaries[event] += 1;self.trace.append(text)
        trigger = event + str(self.calls[event])  # 동일 경계의1기반 호출 번호다.
        for selected, target, value in self.effects:  # 여러 효과를 입력의 순서대로 적용한다.
            if selected != trigger:
                continue
            if target == 'stop':
                if value != 1:
                    raise RuntimeError('명시 중단 값 오류')
                self.stopped = True;self.mu.emu_stop()
            elif target in ('random', 'manager'):
                self.number(RANDOM if target == 'random' else self.template, value)
            else:
                stream, field = target.split('.')  # 현재 해당 stream의 실제 필드를 바꾼다.
                self.number(STREAMS[0 if stream == 'a' else 1] + {'data': 0, 'pos': 4, 'end': 8, 'cap': 12}[field], value)

    def returned(self, answer, cleanup=0):
        """명시 하위의 실제 복귀주소와 thiscall 인자 해제만 수행한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP);target = self.number(esp)  # CPU가 쓴 하위의 실제 스택이다.
        self.mu.reg_write(UC_X86_REG_EAX, answer & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup);self.mu.reg_write(UC_X86_REG_EIP, target)

    def write(self, mu, access, address, size, value, user):
        """원문 쓰기를 격리 스택/stream/버퍼/공유난수로 제한한다."""
        if STACK - 0x1000 <= address and address + size <= STACK + 16:
            return
        if address == RANDOM and size == 4:
            return
        if any(stream <= address and address + size <= stream + 16 for stream in STREAMS):
            return
        if any(buffer <= address and address + size <= buffer + 64 for buffer in BUFFERS):
            return
        raise RuntimeError(f'허용하지 않은 원문 쓰기 {address:08x}/{size}')

    def instruction(self, mu, address, size, user):
        """난수/변환/일반쓰기는 원문에 맡기고 Create/진단의 명시 효과만 제공한다."""
        if address == STOP:
            self.completed = True;mu.emu_stop();return
        if any(start <= address < end for start, end in RANGES.values()):
            self.instructions += 1;self.total_instructions += 1
            if address == 0x4BEEC2:
                value = mu.reg_read(UC_X86_REG_ECX)  # 직전 MOV가 캡처한 manager 값이다.
                self.boundary('manager', f'manager:{self.case["slot"]}:{value}')
            elif address == 0x44CCE0:
                esp = mu.reg_read(UC_X86_REG_ESP);stream = mu.reg_read(UC_X86_REG_ECX)  # 실제 쓰기 진입 인자다.
                if self.number(esp + 8) != 4:
                    raise RuntimeError('Money 실제 쓰기 길이 불일치')
                bits = self.number(self.number(esp + 4));self.values.add(bits)
                self.boundary('write', f'write:{stream}:{bits}:{self.number(RANDOM)}')
            return
        esp = mu.reg_read(UC_X86_REG_ESP)  # 원문 CALL의 실제 인자다.
        if address == 0x44D1B0:
            pointer = self.number(esp + 4)
            if bytes(mu.mem_read(pointer, 6)) != b'Money\0':
                raise RuntimeError('Money 실제 Create 이름 불일치')
            manager = mu.reg_read(UC_X86_REG_ECX);answer = STREAMS[self.case['selected']]  # 캡처한 manager와 명시 Create 반환이다.
            self.boundary('create', f'create:{manager}:Money:{answer}:{self.number(RANDOM)}')
            if not self.stopped:
                self.returned(answer, 4)
        elif address == 0x4E0620:
            line = self.number(esp + 12)  # 실제 memstream.h capacity 진단만 허용한다.
            expression = bytes(mu.mem_read(self.number(esp + 4), 21))
            if line != 0x24 or not expression.startswith(b'pos+length <= alloc\0'):
                raise RuntimeError('Money 일반 쓰기 진단 불일치')
            self.boundary('assert', f'assert:capacity:{self.number(RANDOM)}')
            if not self.stopped:
                self.returned(0)
        else:
            raise RuntimeError(f'허용하지 않은 코드 진입 {address:08x}')

    def sample(self, case, control):
        """각 입력/CPU를 초기화하고 정상 RET 또는 명시 부분 중단을 관측한다."""
        self.case = case;self.calls = Counter();self.trace = [];self.completed = False;self.stopped = False;self.instructions = 0  # 현재 독립 실행의 상태다.
        self.effects = [] if case['mutation'] == '-' else [(event, target, int(value)) for event, target, value in (row.split('/') for row in case['mutation'].split(';'))]
        self.template = 0x5C89F0 + case['slot'] * 20  # 실제 Template 배열의 현재 this 주소다.
        self.number(self.template, case['manager']);self.number(OWNER, case['owner']);self.number(RANDOM, case['seed'])
        for stream, buffer in zip(STREAMS, BUFFERS):  # 두 stream과 독립 가드 버퍼를 매번 초기화한다.
            self.mu.mem_write(stream, struct.pack('<IIII', buffer, case['position'], case['end'], case['capacity']))
            self.mu.mem_write(buffer, bytes(range(64)))
        self.mu.mem_write(STACK - 0x1000, bytes([0xCC]) * 0x1020)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, STREAMS[case['selected']]))
        initial = {}  # 정상 반환의 callee-save 확인용 원시 레지스터다.
        for index, register in enumerate(REGISTERS):  # 앞선 입력의 정수 CPU 상태를 제거한다.
            value = 0x10111213 + index * 0x10101010;initial[register] = value;self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, self.template);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.emu_start(RANGES[case['kind']][0], STOP + 1, count=10000)
        if not self.completed and not self.stopped:
            raise RuntimeError('Money 정상 반환/명시 중단 전 명령 상한 도달')
        delta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - 4  # cdecl/thiscall 무인자 부모의 최종RET4만 정규화한다.
        if self.completed:
            self.normal_returns += 1
            if delta or any(self.mu.reg_read(register) != initial[register] for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)):
                raise RuntimeError('Money 정상 반환 스택/callee-save 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('Money x87 제어어/스택 잔여 변경')
        for address, original in self.original.items():  # 네 실제 몸체의 보호 바이트는 매번 같아야 한다.
            if bytes(self.mu.mem_read(address, len(original))) != original:
                raise RuntimeError('Money 보호 코드 변경')
        return [int(self.completed), self.number(RANDOM), self.number(self.template),
                *[bytes(self.mu.mem_read(stream, 16)).hex() for stream in STREAMS],
                *[bytes(self.mu.mem_read(buffer, 64)).hex() for buffer in BUFFERS],
                ';'.join(self.trace) or '-', delta, self.instructions]


def main():
    """최초 실행만 파일을 만들고 verify는 보호/도구/fixture 전체 SHA를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, default=FIXTURE)
    parser.add_argument('--report', type=Path, default=REPORT)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(args.report.read_text(encoding='utf-8'))
        for name, expected in record['sha256'].items():  # 최초 근거 전체를 독립 재확인한다.
            if digest(ROOT / name) != expected:
                raise RuntimeError('Money 최초 근거 SHA 불일치: ' + name)
        print('Money 최초 x86 근거 확인', record['inputs']);return
    if not args.dry_run and (args.fixture.exists() or args.report.exists()):
        raise RuntimeError('최초 Money 원본 fixture/보고서 덮어쓰기 금지')
    oracle = Oracle();inputs = cases();lines = ['# ' + '\t'.join(SCHEMA)]  # 제품 C++나 실행 파일은 읽지 않는다.
    for case in inputs:  # 두 실제 x87 결과가 같을 때만 한 독립 입력으로 기록한다.
        outputs = [oracle.sample(case, control) for control in CONTROLS]
        if outputs[0] != outputs[1]:
            raise RuntimeError('Money 정밀도별 관측 불일치: ' + case['name'])
        lines.append('\t'.join(map(str, [case[field] for field in SCHEMA[:11]] + outputs[0])))
    if len(oracle.values) != 1010:
        raise RuntimeError('Money 실제1010개 float 관측 누락')
    record = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon / 2026-10-23', inputs=len(inputs), executions=len(inputs) * len(CONTROLS),
                  normalReturns=oracle.normal_returns, instructions=oracle.total_instructions, boundaries=dict(oracle.boundaries), distinctMoneyBits=len(oracle.values),
                  ranges={name: f'{start:08x}-{end-1:08x}' for name, (start, end) in RANGES.items()}, gameExecuted=False, osExecuted=False,
                  scope='10.78 Money 부모/공유난수/FILD/FADD333/FSTP DWORD/일반 stream writer 실제 실행. Create와capacity진단은 명시 경계다. 단독 부모에는isMine/slot clamp/manager진단이 없으며 전체Snapshot 부모는 실행하지 않는다.')
    if not args.dry_run:
        args.fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
        paths = [PE, ROOT / 'extracted/refined/originals/Netstorm.c', Path(__file__).resolve(), args.fixture.resolve()]
        record['sha256'] = {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in paths}
        args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(record, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
