#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 10.78의 전체 Territory/Chaff 순회 두 몸체를 독립 x86 실행하여 새 fixture로 보존한다."""
import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호 원문과 새 검증 자료를 구별하는 저장소 루트다.
PE = ROOT / 'originals/Netstorm.exe'  # OS/게임 진입 없이 읽기만 하는 보호 PE다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 검토한10.78 PE의 고정 식별자다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-region-save-x86.tsv'  # 최초 실제 실행 결과를 보존할 새 경로다.
REPORT = ROOT / 'cpppj/fort-region-save-native.json'  # 실제 코드 범위와 경계·SHA를 남기는 보고서다.
BASE, HEAP, STACK, STOP = 0x400000, 0x14000000, 0x15008000, 0x1400F000  # 격리 PE/외부BYTE/스택/정상 반환 주소다.
WIDTH, HEIGHT, SCOPE = 0x54C8A4, 0x54C8A8, 0x5C89B0  # 실제 원문이 읽고 쓰는 공유 DWORD 주소다.
STREAM = 0x12345678  # 모든 청크 저장 CALL에서 그대로 유지해야 하는 명시 stream 토큰이다.
RANGES = {'territory': (0x4BE230, 0x4BE2B2), 'chaff': (0x4BE350, 0x4BE3B9)}  # 정상 RET까지 포함한 두 원문 몸체다.
READS = {0x4BE238: ('Y', UC_X86_REG_EAX), 0x4BE252: ('X', UC_X86_REG_EAX),
         0x4BE29A: ('X', UC_X86_REG_EAX), 0x4BE2A5: ('Y', UC_X86_REG_ECX),
         0x4BE358: ('Y', UC_X86_REG_EAX), 0x4BE376: ('X', UC_X86_REG_EAX),
         0x4BE39F: ('X', UC_X86_REG_EAX), 0x4BE3AD: ('Y', UC_X86_REG_ECX)}  # MOV 직후에만 관측해 이미 캡처한 읽기를 바꾸지 않는다.
REGISTERS = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)  # 입력 간 CPU 잔여를 제거할 정수 레지스터다.
CONTROLS = (0x27F, 0x37F)  # 두 x87 제어어에서도 정수 순회가 같은지 별도로 실행한다.
SCHEMA = ('name', 'kind', 'width', 'height', 'scope', 'owner', 'region', 'regions', 'decisions', 'mutation',
          'completed', 'widthout', 'heightout', 'scopeout', 'trace', 'stackdelta', 'nativeinstructions', 'scopewrites')  # 앞10열은 독립 입력, 뒤8열은 실제 PE 관측이다.


def digest(path):
    """보호 입력과 새 검증 파일의 전체 바이트 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def signed(value):
    """원본 signed DWORD 비교를 호스트 정수에서 해석한다."""
    return struct.unpack('<i', struct.pack('<I', value & 0xFFFFFFFF))[0]


def cases():
    """음수/빈 지도·모든 signed BYTE·현재 크기/공유scope 변경을 독립 입력으로 교차한다."""
    rows = []  # 아직 기대 trace나 선택 셀을 계산하지 않은 명시 입력 목록이다.
    for kind, width, height, decision in itertools.product(('territory', 'chaff'), (-1, 0, 1, 2, 3), (-1, 0, 1, 2, 3), (0, 1, 0x80000000)):
        rows.append(dict(kind=kind, width=width, height=height, decisions=str(decision)))  # 음수/0 반환과 비정상 양수비트도 원문 nonzero 분기로 관측한다.
    for kind, byte in itertools.product(('territory', 'chaff'), range(256)):
        rows.append(dict(kind=kind, width=1, height=1, regions=f'{byte:02x}', decisions='0,1,4294967295' if byte % 2 else '1,0',
                         owner=(0, 1, 8, 0xFFFFFFFF)[byte % 4], region=(-1, 0, 19, 0x7FFFFFFF)[byte % 4]))  # 각 BYTE를 원문 MOVSX에 직접 공급한다.
    for kind in ('territory', 'chaff'):  # 실제 외부 효과 뒤에만 현재 전역을 바꾸고 원문 다음 읽기를 기다린다.
        events = ('y1', 'x1', 'x2', 'x4', 'y2', 'cell1', 'cell3', 'chunk1', 'chunk3') + (('allow1', 'allow3') if kind == 'territory' else ())
        for event, field, value in itertools.product(events, ('width', 'height', 'scope'), (-1, 0, 1, 4)):
            rows.append(dict(kind=kind, width=3, height=3, mutation=f'{event}/{field}/{value}'))
        for event in events:  # 각 실제 경계의 명시 중단에서 scope의 부분 쓰기/현재 전역과 trace를 보존한다.
            rows.append(dict(kind=kind, width=3, height=3, mutation=f'{event}/stop/1'))
        rows.extend((dict(kind=kind, width=3, height=2, mutation='y2/width/0'),
                     dict(kind=kind, width=3, height=2, mutation='y2/width/4'),
                     dict(kind=kind, width=1, height=3, mutation='cell1/width/4;chunk1/height/2;cell3/scope/77'),
                     dict(kind=kind, width=0, height=3, mutation='x1/width/4;y2/height/1')))  # 빈 폭과 행 시작 cachedwidth 경계를 따로 보존한다.
    result = []  # 동일 입력은 한 번만 남겨 독립 fixture 이름을 안정적으로 만든다.
    seen = set()  # 직렬화한 입력의 중복만 제거하고 기대 결과는 보지 않는다.
    for row in rows:  # 모든 생략 입력에 명시 기본값을 붙인다.
        item = dict(kind='territory', width=2, height=2, scope=0x87654321, owner=0xFFFFFFFF, region=-7,
                    regions='007f80ff', decisions='1,0,2147483648,4294967295', mutation='-')
        item.update(row)
        key = tuple(item.values())
        if key not in seen:
            seen.add(key);item['name'] = f'region-{len(result):04}';result.append(item)
    return result


class Oracle:
    """두 실제 순회 몸체와 세 명시 외부 CALL만 허용하는 격리 x86 실행기다."""
    def __init__(self):
        """보호 PE를 가상 메모리에 읽고 변경할 수 있는 입력/스택만 따로 마련한다."""
        if digest(PE) != PE_SHA:
            raise RuntimeError('검토한 보호 PE SHA 불일치')
        self.pe = pefile.PE(str(PE))  # 실제 원문 바이트만 공급하는 읽기 전용 파서다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # OS/GUI/원본 게임을 시작하지 않는 CPU 실행기다.
        self.mu.mem_map(BASE, (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.mu.mem_write(BASE, self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP, 0x10000);self.mu.mem_map(STACK - 0x8000, 0x10000)
        self.original = {start: bytes(self.mu.mem_read(start, end - start)) for start, end in RANGES.values()}  # 실행 가능한 본체 전체의 불변 확인용이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.total_instructions = 0;self.boundaries = Counter();self.normal_returns = 0  # 실제 원문 실행과 명시 하위를 나누는 누계다.

    def number(self, address, value=None):
        """현재 raw DWORD를 읽거나 명시 입력/외부효과로만 변경한다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))
        return int.from_bytes(self.mu.mem_read(address, 4), 'little')

    def boundary(self, event, text):
        """캡처한 실제 호출 순서를 기록한 뒤 명시 효과만 적용한다."""
        self.calls[event] += 1;self.boundaries[event] += 1;self.trace.append(text)
        trigger = event + str(self.calls[event])  # 동일 종류의1기반 호출 순서다.
        for selected, target, value in self.effects:  # 같은 경계의 명시 변경을 입력 순서대로 적용한다.
            if selected != trigger:
                continue
            if target == 'stop':
                if value != 1:
                    raise RuntimeError('명시 중단 값 오류')
                self.stopped = True;self.mu.emu_stop()
            else:
                self.number({'width': WIDTH, 'height': HEIGHT, 'scope': SCOPE}[target], value)

    def returned(self, answer, cleanup=0):
        """실제 CALL 복귀주소와 thiscall 하위의 인자 해제만 수행한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 외부 CALL의 현재 실제 스택이다.
        target = self.number(esp)  # CPU가 쓴 정상 복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX, answer & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup);self.mu.reg_write(UC_X86_REG_EIP, target)

    def write(self, mu, access, address, size, value, user):
        """본체 쓰기를 격리 스택과 공유 scope 한 DWORD로 제한한다."""
        if STACK - 0x100 <= address and address + size <= STACK + 16:
            return
        if address == SCOPE and size == 4:
            self.scope_writes += 1
            if value != (1 if self.case['kind'] == 'territory' else 2):
                raise RuntimeError('원문 scope 쓰기 값 불일치')
            return
        raise RuntimeError(f'허용하지 않은 원문 쓰기 {address:08x}/{size}')

    def instruction(self, mu, address, size, user):
        """분기/반복/전역 읽기는 원문에 맡기고 CALL 인자와 캡처 뒤 효과만 제공한다."""
        if address == STOP:
            self.completed = True;mu.emu_stop();return
        start, end = RANGES[self.case['kind']]  # 이번 실행에 허용한 온전한 몸체다.
        if start <= address < end:
            self.instructions += 1;self.total_instructions += 1
            if address in READS:
                kind, register = READS[address]
                value = signed(mu.reg_read(register))  # 직전 MOV가 이미 캡처한 signed 값이다.
                self.boundary(kind.lower(), f'{kind}:{value}:{self.number(SCOPE)}')
            return
        esp = mu.reg_read(UC_X86_REG_ESP)  # 실제 CALL이 전달한 스택 인자를 읽는다.
        if address == 0x432180:
            if mu.reg_read(UC_X86_REG_ECX) != 0x54C8A0:
                raise RuntimeError('현재 청크 지도 this 주소 불일치')
            x, y = signed(self.number(esp + 4)), signed(self.number(esp + 8))
            if not (0 <= x < 8 and 0 <= y < 8):
                raise RuntimeError('명시 독립 지도 범위 초과')
            byte = self.regions[(y * 8 + x) % len(self.regions)]  # 하위 지도의 명시 BYTE 입력이며 필터 기대를 계산하지 않는다.
            self.boundary('cell', f'cell:{x}:{y}:{byte}:{self.number(SCOPE)}')
            self.mu.mem_write(HEAP, bytes([byte]))
            if not self.stopped:
                self.returned(HEAP, 8)
        elif address == 0x46F200:
            if mu.reg_read(UC_X86_REG_ECX) != 0x568B60:
                raise RuntimeError('현재 영역 판정 this 주소 불일치')
            byte, owner, region = signed(self.number(esp + 4)), self.number(esp + 8), signed(self.number(esp + 12))
            answer = self.decisions[self.calls['allow'] % len(self.decisions)]  # 외부 판정의 명시 DWORD 반환이다.
            self.boundary('allow', f'allow:{byte}:{owner}:{region}:{answer}:{self.number(SCOPE)}')
            if not self.stopped:
                self.returned(answer, 12)
        elif address == 0x4BE020:
            stream, x, y = self.number(esp + 4), signed(self.number(esp + 8)), signed(self.number(esp + 12))
            self.boundary('chunk', f'chunk:{stream}:{x}:{y}:{self.number(SCOPE)}')
            if not self.stopped:
                self.returned(0)
        else:
            raise RuntimeError(f'허용하지 않은 코드 진입 {address:08x}')

    def sample(self, case, control):
        """독립 입력을 매번 초기화하고 정상 RET 또는 명시 부분 중단의 실제 상태를 반환한다."""
        self.case = case;self.calls = Counter();self.trace = [];self.completed = False;self.stopped = False;self.instructions = 0;self.scope_writes = 0  # 한 입력의 원문 관측이다.
        self.effects = [] if case['mutation'] == '-' else [(event, target, int(value)) for event, target, value in (row.split('/') for row in case['mutation'].split(';'))]
        self.regions = bytes.fromhex(case['regions']);self.decisions = [int(value) for value in case['decisions'].split(',')]  # 명시 BYTE/판정 순서다.
        self.number(WIDTH, case['width']);self.number(HEIGHT, case['height']);self.number(SCOPE, case['scope'])
        self.mu.mem_write(STACK - 0x100, bytes([0xCC]) * 0x120)
        self.mu.mem_write(STACK, struct.pack('<IIII', STOP, STREAM, case['owner'], case['region'] & 0xFFFFFFFF))
        initial = {}  # 정상 반환에서 확인할 callee-save 원시 레지스터다.
        for index, register in enumerate(REGISTERS):  # 서로 다른 고정값으로 이전 실행의 CPU 상태를 제거한다.
            value = 0x10111213 + index * 0x10101010;initial[register] = value;self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.emu_start(RANGES[case['kind']][0], STOP + 1, count=100000)
        if not self.completed and not self.stopped:
            raise RuntimeError('정상 반환/명시 중단 전 원문 명령 상한 도달')
        delta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - 4  # cdecl 최상위 RET4만 정규화한다.
        if self.completed:
            self.normal_returns += 1
            if delta != 0 or any(self.mu.reg_read(register) != initial[register] for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)):
                raise RuntimeError('정상 반환의 스택/callee-save 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('x87 제어어/스택 잔여 변경')
        for address, original in self.original.items():  # 두 몸체 모두 실제 실행 뒤 원본 바이트가 같아야 한다.
            if bytes(self.mu.mem_read(address, len(original))) != original:
                raise RuntimeError('보호 코드 바이트 변경')
        return [int(self.completed), signed(self.number(WIDTH)), signed(self.number(HEIGHT)), self.number(SCOPE),
                ';'.join(self.trace) or '-', delta, self.instructions, self.scope_writes]


def main():
    """최초 fixture 덮어쓰기를 거부하고 검증 시 보호 입력/도구/fixture SHA를 다시 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, default=FIXTURE)
    parser.add_argument('--report', type=Path, default=REPORT)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--dry-run', action='store_true', help='파일을 만들지 않고 모든 독립 x86 입력만 실행한다')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(args.report.read_text(encoding='utf-8'))
        for name, expected in record['sha256'].items():  # 최초 관측의 보호 자료와 생성 도구/결과를 모두 재확인한다.
            if digest(ROOT / name) != expected:
                raise RuntimeError('최초 Region 원문 관측 SHA 불일치: ' + name)
        print('Region 최초 x86 근거 확인', record['inputs']);return
    if not args.dry_run and (args.fixture.exists() or args.report.exists()):
        raise RuntimeError('최초 Region 원본 fixture/보고서 덮어쓰기 금지')
    oracle = Oracle();inputs = cases();lines = ['# ' + '\t'.join(SCHEMA)]  # 결과를 계산하는 제품 소스/실행 파일은 읽지 않는다.
    for case in inputs:  # 같은 입력을 두 정밀도로 각각 실제 실행하고 반환·trace·부분 상태가 같은지 확인한다.
        outputs = [oracle.sample(case, control) for control in CONTROLS]
        if outputs[0] != outputs[1]:
            raise RuntimeError('Region 정밀도별 원문 관측 불일치: ' + case['name'])
        lines.append('\t'.join(map(str, [case[field] for field in SCHEMA[:10]] + outputs[0])))
    record = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon / 2026-10-23', inputs=len(inputs), executions=len(inputs) * len(CONTROLS),
                  normalReturns=oracle.normal_returns, instructions=oracle.total_instructions, boundaries=dict(oracle.boundaries),
                  ranges={name: f'{start:08x}-{end - 1:08x}' for name, (start, end) in RANGES.items()}, gameExecuted=False, osExecuted=False,
                  scope='원본 두 몸체의 크기 읽기/scope쓰기/signed BYTE/순회/분기/인자/현재 재조회. 432180/46f200/4be020은 명시 하위 장치다. 청크 저장·섬 판정·GUI·OS는 실행하지 않는다.')
    if not args.dry_run:
        args.fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
        paths = [PE, ROOT / 'extracted/refined/originals/Netstorm.c', Path(__file__).resolve(), args.fixture.resolve()]
        record['sha256'] = {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in paths}
        args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(record, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
