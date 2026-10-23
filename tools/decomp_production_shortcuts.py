#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 입력 분기와 실제 명령 큐에서 다리 단축키를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ROOT = Path(__file__).resolve().parents[1]  # 허용한 새 산출물의 루트다.
PE_PATH = ROOT / 'originals/Netstorm.exe'  # 변경하지 않는 보호 입력이다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 원문 식별자다.
FIXTURE_PATH = ROOT / 'cpppj/tests/fixtures/production-shortcuts-x86.tsv'  # 실제 관측 행이다.
REPORT_PATH = ROOT / 'extracted/cpp-production-shortcuts-20261023-native.json'  # 범위/경계/해시 기록이다.
BASE, HEAP, STACK = 0x400000, 0x1000000, 0x2100000  # 보호 이미지와 격리 장치/스택이다.
ENTRY, END = 0x4D6E61, 0x4DA477  # Guide 호출 준비부터 현재 사건의 종료 라벨 직전까지다.
GUIDE = 0x562CA0  # 원본이 직접 사용하는 현재 Guide 주소다.
MISSION, TABLE, VIRTUAL_INPUT = HEAP, HEAP + 0x1000, HEAP + 0x2000  # Guide의 명시 하위 입력이다.
READ, WRITE, QUEUE = 0x55A508, 0x55A50C, 0x55A510  # 실제40칸 입력 큐다.
COMMAND, PAUSE, CONSOLE, CURSOR = 0x542AC4, 0x55B4B0, 0x565DD4, 0x5325C8  # 현재 재조회 전역이다.
CONTROLS = (0x27F, 0x37F)  # 정밀도별 명령 경로도 독립 실행한다.
RANGES = ((ENTRY, END), (0x463E40, 0x463F62), (0x460DE0, 0x460DEF),
          (0x445330, 0x44533E), (0x452FE0, 0x45303C),
          (0x4E63D0, 0x4E63D5), (0x4E63E0, 0x4E649E))  # 마지막 두 범위는 비대상 키 fallback의 실제CRT strchr다.
SCHEMA = ('name', 'events', 'guideResult', 'gumpResult', 'pauseDepth', 'console',
          'command', 'read', 'filled', 'completed', 'readout', 'writeout',
          'queueout', 'physicalqueue', 'trace', 'stackdelta', 'nativeinstructions')  # 앞9열 입력/뒤8열 관측이다.


def digest(path):
    """보호 원문과 새 자료의 SHA256을 변경 없이 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """현재 사건 처리와 하위 Guide/정지/큐 명령을 실제 x86으로 실행한다."""

    def __init__(self):
        """읽기 전용 이미지와 고정 장치만 메모리에 준비한다."""
        self.pe = pefile.PE(str(PE_PATH))  # 입력 파서다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 원문 명령 실행기다.
        self.image_size = (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095  # 페이지 경계다.
        self.image = self.pe.get_memory_mapped_image()  # 최초 한 번 적재하고 CPU 변경을 금지할 원문이다.
        self.mu.mem_map(BASE, self.image_size)
        self.mu.mem_write(BASE, self.image)
        self.mu.mem_map(HEAP, 0x4000)
        self.mu.mem_map(STACK - 0x10000, 0x20000)
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.original = {low: bytes(self.mu.mem_read(low, high-low)) for low, high in RANGES}  # 본체 불변 근거다.
        self.boundaries, self.writes, self.totals, self.visited = Counter(), Counter(), {}, set()  # 독립 실행 집계다.

    def number(self, address, value=None, width=4):
        """비정렬 원시 DWORD를 읽고 명시 입력만 쓴다."""
        if value is not None:
            self.mu.mem_write(address, (value & ((1 << (width*8))-1)).to_bytes(width, 'little'))
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def returned(self, value=0, cleanup=0):
        """명시 외부 CALL의 실제 복귀주소/thiscall 인자를 소비한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 실제 경계 진입 스택이다.
        target = self.number(esp)  # CPU가 쓴 복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def event(self, name, values=()):
        """경계 이름과 실제 전달값을 실행 순서로 기록한다."""
        self.boundaries[name] += 1
        self.trace.append(':'.join((name, *(str(value) for value in values))))

    def guard(self, mu, access, address, size, value, data):
        """실제 CPU 쓰기를 스택과 원본 입력 큐로 제한한다."""
        if STACK-0x8000 <= address and address+size <= STACK+0x3000:
            category = 'stack'
        elif READ <= address and address+size <= QUEUE+40*12:
            category = 'queue'
        else:
            raise RuntimeError(f'미명시 원문 쓰기 {address:08x}/{size}')
        self.writes[category] += 1

    def instruction(self, mu, address, size, data):
        """분기/점프표/실제 큐를 실행하며 명시 UI 장치만 대체한다."""
        if address == END:
            self.completed = True
            mu.emu_stop()
            return
        if address == 0x463E40:
            esp = mu.reg_read(UC_X86_REG_ESP)  # 현재 Guide가 받은 실제 사건 포인터다.
            pointer = self.number(esp+4)
            self.event('guide', tuple(self.number(pointer+offset) for offset in (0, 4, 8)))
        elif address == 0x460DE0:
            self.event('paused', (self.number(PAUSE),))
        elif address == 0x445330:
            self.event('cursor', (self.number(CURSOR),))
        elif address == 0x452FE0:
            esp = mu.reg_read(UC_X86_REG_ESP)  # 실제 push한 명령/슬롯/셋째값이다.
            self.event('queue', tuple(self.number(esp+offset) for offset in (4, 8, 12)))
        if any(low <= address < high for low, high in RANGES):
            self.instructions += 1
            self.visited.add(address)
            return
        if address == VIRTUAL_INPUT:
            pointer = self.number(mu.reg_read(UC_X86_REG_ESP)+4)  # 실제 Guide 가상+24의 사건이다.
            self.event('mission', tuple(self.number(pointer+offset) for offset in (0, 4, 8)))
            self.returned(self.case['guideResult'], 4)
            return
        if address == 0x466200:
            esp = mu.reg_read(UC_X86_REG_ESP)  # 원본이 값으로 복사한 사건12BYTE와 마지막0이다.
            self.event('gump', tuple(self.number(esp+offset) for offset in (4, 8, 12, 16)))
            self.returned(self.case['gumpResult'])
            return
        if address == 0x413D50:
            self.event('general')
            self.returned()
            return
        raise RuntimeError(f'미명시 원문 실행 {address:08x}')

    def run(self, case, control):
        """기대 매핑 없이 입력 사건열을 실행하고 활성/전체 물리 큐를 읽는다."""
        self.case, self.trace, self.instructions = case, [], 0  # 현재 명시 장치와 관측이다.
        # CPU 쓰기는 큐/스택만 허용하므로 해당 입력과 명시 장치 전역만 초기화한다.
        self.number(GUIDE+0x800, MISSION)
        self.number(MISSION, TABLE)
        self.number(TABLE+0x24, VIRTUAL_INPUT)
        self.number(COMMAND, case['command'])
        self.number(PAUSE, case['pauseDepth'])
        self.number(CONSOLE, case['console'])
        self.number(CURSOR, 1)
        self.number(READ, case['read'])
        self.number(WRITE, (case['read']+case['filled']) % 40)
        # 활성 밖 오래된 슬롯도 실제 포화 쓰기와 구별하도록 고유 입력을 쓴다.
        for index in range(40):
            self.mu.mem_write(QUEUE+index*12, struct.pack('<3I', 0x2000000+0x300+index, 100+index, 200+index))
        deltas = []  # 각 원문 사건의 스택 균형을 모은다.
        self.completed = False
        # 각 입력은 OS/커서 폴링을 끝낸 원본의 현재 사건 프레임에서 시작한다.
        for text in case['events'].split(';'):
            code, first, second = (int(value) for value in text.split(':'))  # 입력 세 DWORD다.
            self.mu.mem_write(STACK-0x4000, bytes(0x8000))
            self.mu.mem_write(STACK+0x14, struct.pack('<3I', code, first, second))
            # 원본의 EDI=0 및 이미 확보한 지역 프레임을 명시한다.
            for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                             UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
                self.mu.reg_write(register, 0)
            self.mu.reg_write(UC_X86_REG_ESP, STACK)
            self.mu.reg_write(UC_X86_REG_FPCW, control)
            self.mu.reg_write(UC_X86_REG_FPSW, 0)
            self.completed = False
            self.mu.emu_start(ENTRY, END+1, count=300000)
            if not self.completed:
                raise RuntimeError('현재 사건의 원문 종료 라벨 미도달')
            if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) != 0:
                raise RuntimeError('정수 입력 분기의 x87 상태 변경')
            deltas.append(str(self.mu.reg_read(UC_X86_REG_ESP)-STACK))
        current, last = self.number(READ), self.number(WRITE)  # 기대 계산 없이 실제 커서만 읽는다.
        active = []  # 두 실제 커서 사이의 유효 항목이다.
        while current != last:
            active.append(':'.join(str(self.number(QUEUE+current*12+offset)) for offset in (0, 4, 8)))
            current = (current+1) % 40
        output = {'completed': int(self.completed), 'readout': self.number(READ), 'writeout': last,
                  'queueout': ';'.join(active) or '-', 'physicalqueue': bytes(self.mu.mem_read(QUEUE, 40*12)).hex(),
                  'trace': ';'.join(self.trace) or '-', 'stackdelta': ';'.join(deltas), 'nativeinstructions': self.instructions}  # 원문 최종 관측이다.
        # 명시 효과가 보호 본체를 바꾸지 않았는지 모든 실행에서 확인한다.
        for low, expected in self.original.items():
            if bytes(self.mu.mem_read(low, len(expected))) != expected:
                raise RuntimeError('보호 명령 본체 변경')
        total = self.totals.setdefault(f'{control:03x}', {'runs': 0, 'instructions': 0})  # 제어어별 합계다.
        total['runs'] += 1
        total['instructions'] += self.instructions
        return output


def cases():
    """문자/모든 상위 입력 플래그와 실제 정지/소비/현재 명령/포화를 교차한다."""
    rows, seen = [], set()  # 중복 입력을 제거할 저장소다.

    def add(name, events, **changes):
        """관측 기대 없이 명시 장치와 사건열만 등록한다."""
        case = dict(name=name, events=';'.join(':'.join(str(value & 0xFFFFFFFF) for value in event) for event in events),
                    guideResult=0, gumpResult=0, pauseDepth=0, console=0, command=0x020000F4, read=0, filled=0)
        case.update(changes)
        key = tuple(case[field] for field in SCHEMA[:9] if field != 'name')  # 결과를 포함하지 않는 입력 중복 키다.
        if key not in seen:
            seen.add(key)
            rows.append(case)

    keys = 'QqWwAaSsZzXxYyEe'  # 이 문자들의 출력 슬롯은 Python에서 계산하지 않는다.
    flags = tuple(sum(bit << (26+index) for index, bit in enumerate(bits)) for bits in itertools.product((0, 1), repeat=6))  # outside/alt/ctrl/shift/pressed/release 전체 조합이다.
    # 모든 문자와 상위 플래그를 정지0/1에서 교차한다.
    for key, flag, pause in itertools.product(keys, flags, (0, 1)):
        add(f'flags-{ord(key)}-{flag}-{pause}', [(ord(key)|flag, 23, 47)], pauseDepth=pause)
    # Guide/Gump의 비영 반환과 signed 정지 깊이의 순서를 구별한다.
    for key, guide, gump, pause, flag in itertools.product(keys, (0, 1, 0xFFFFFFFF), (0, 1, 0xFFFFFFFF), (0, 1, 0xFFFFFFFF), (0, 0x80000000)):
        add(f'gate-{ord(key)}-{guide}-{gump}-{pause}-{flag}', [(ord(key)|flag, 23, 47)], guideResult=guide, gumpResult=gump, pauseDepth=pause)
    # 현재 명령의 마스킹과 가득 찬 고리/감기는 고리에서 실제 쓰기를 관측한다.
    for key, command, read, filled in itertools.product(keys, (0x020000F4, 0xABCDEF12, 0, 0xFFFFFFFF), (0, 17, 39), (0, 1, 34, 38, 39)):
        add(f'ring-{ord(key)}-{command}-{read}-{filled}', [(ord(key), 0x12345678, 0x87654321)], command=command, read=read, filled=filled)
    # 주변 비대상 문자는 동일 실제 분기에서 예약 없음인지 관측한다.
    for key, flag, pause in itertools.product('BbFfGgJjKkLlMmOoVv{~', (0, 0x38000000, 0x80000000), (0, 1)):
        add(f'near-{ord(key)}-{flag}-{pause}', [(ord(key)|flag, 23, 47)], pauseDepth=pause)
    # 콘솔 전역이 있으면 일반 단축키 분기보다 먼저 사건이 끝난다.
    for key, console in itertools.product(keys, (1, 0xFFFFFFFF)):
        add(f'console-{ord(key)}-{console}', [(ord(key), 23, 47)], console=console)
    # 큐 순서/동일 슬롯 반복/E의 여섯 예약/포화를 여러 실제 사건으로 이어 확인한다.
    for text, read, filled in itertools.product(('QWASZX', 'XZS AWQ'.replace(' ', ''), 'QqYyZz', 'EeQX'), (0, 17, 39), (0, 30, 34, 38, 39)):
        add(f'sequence-{text}-{read}-{filled}', [(ord(key), 23, 47) for key in text], read=read, filled=filled)
    return rows


def main():
    """원문 해시 확인 뒤 새 두 정밀도 관측과 범위 기록만 저장한다."""
    if digest(PE_PATH) != PE_SHA:
        raise RuntimeError('보호 PE SHA256 불일치')
    oracle, rows = Oracle(), []  # 원문 실행기와 TSV 행이다.
    inputs = cases()  # 기대 없는 입력 목록이다.
    # 각 입력을 같은 원문에서 두 번 독립 실행하며 출력 차이를 거부한다.
    for index, case in enumerate(inputs):
        previous = None
        for control in CONTROLS:
            try:
                output = oracle.run(case, control)
            except Exception as error:
                raise RuntimeError(f'{case["name"]}: {error}') from error
            if previous is not None and previous != output:
                raise RuntimeError('정밀도 차이: '+case['name'])
            previous = output
            values = dict(case, name=case['name']+f'-cw{control:03x}', **output)  # 실제 관측만 행에 합친다.
            rows.append('\t'.join(str(values[field]) for field in SCHEMA))
        if index % 1000 == 0:
            print('observed', index, flush=True)
    FIXTURE_PATH.write_text('# '+ '\t'.join(SCHEMA)+'\n'+'\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    record = {'host': socket.gethostname(), 'lastDecompileHost': 'VM-W11-Codex / 2026-10-23',
              'inputs': len(inputs), 'observations': len(rows), 'schema': SCHEMA, 'precisionDifferences': 0,
              'precisions': oracle.totals, 'boundaries': dict(oracle.boundaries), 'cpuWrites': dict(oracle.writes),
              'inputGroups': dict(Counter(case['name'].split('-', 1)[0] for case in inputs)),
              'nativeRanges': [[hex(low), hex(high)] for low, high in RANGES], 'visitedInstructionAddresses': len(oracle.visited),
              'scope': '실제004d62b0 안의004d6e61부터004da477 직전까지 현재 사건의 Guide/Gump/release/콘솔/정지/키 분기와 실제452fe0 큐 쓰기를 실행한다. Guide463e40/정지460de0/커서445330과 비대상 문자 fallback CRT strchr도 실제 본체다.',
              'limits': ['전체004d62b0 진입/OS 메시지/폴링/커서 초기화/실제큐Pop/다음 사건 반복/최종RET는 범위 밖이다. 원본 EDI0과 이미 확보한 지역 프레임을 공급한다.',
                         'Guide 하위 가상+24와 Gump466200의 소비 반환, 일반413d50은 명시 장치다. 실제 Gump 트리/생산43ec00/Take/Drop/음향/게임/GUI는 실행하지 않는다.',
                         'outside 플래그는 이미 큐에서 꺼낸 현재 사건 안의 마스킹만 검증한다. 그보다 앞의 창 활성/커서 폴링/큐 처리 진입 관문을 검증한 결과로 확대하지 않는다.',
                         '입력 사건열 사이 큐를 보존하며 출력에는 활성 큐와 비활성 슬롯을 포함한40칸 전체를 보존한다. 기대 슬롯 매핑은 Python에서 계산하지 않는다.'],
              'gameExecuted': False, 'guiExecuted': False,
              'sha256': {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in
                         (PE_PATH, ROOT/'extracted/refined/originals/Netstorm.c', Path(__file__), FIXTURE_PATH)}}  # 원문/도구/출력 식별자다.
    REPORT_PATH.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('inputs', 'observations', 'precisionDifferences')}), flush=True)


if __name__ == '__main__':
    main()
