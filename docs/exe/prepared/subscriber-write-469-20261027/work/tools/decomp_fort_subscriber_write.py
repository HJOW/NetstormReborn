#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004bd400/등록helper/기존문자열/실제MemStream을게임·OS없이독립관측하고재실행검증한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
import sys
from collections import Counter
from pathlib import Path

# staging과 제품 양쪽에서 같은 저장소/읽기 전용 기반 도구를 찾는다.
HERE = Path(__file__).resolve()
ROOT = next(path for path in HERE.parents if (path / 'AGENTS.md').is_file())
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_priestmenu import Oracle as MemoryOracle, POOL, STACK, STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW)

OUT = ROOT if HERE.parent == ROOT / 'tools' else ROOT / 'extracted/cpp-fort-subscriber-write-469-staging/work'  # 미채택 단계의 fixture/보고서는 staging 안에만 쓴다.
FIXTURE = OUT / 'cpppj/tests/fixtures/fort-subscriber-write-x86.tsv'  # 최초 생성 뒤 제품 예상값으로 덮어쓰지 않는 관측이다.
REPORT = OUT / 'cpppj/fort-subscriber-write-native.json'  # PE/기반도구/생성기/fixture 해시와 실행 범위를 기록한다.
HEADER = POOL + 0x100  # 같은 고정 this가 가리키는 실제20BYTE 입력이다.
STREAMS = (POOL + 0x200, POOL + 0x220)  # Create 경계가 선택한 두 실제16BYTE stream이다.
NAMES = (POOL + 0x10000, POOL + 0x22000)  # 원래 캡처 포인터와 바뀐 global 포인터를 구별한다.
BUFFERS = (POOL + 0x40000, POOL + 0x60000)  # 실제 MemStream이 쓸 수 있는 두 물리출력이다.
PHYSICAL = 0x11000  # 길이WORD wrap과 주변 guard까지 포함한 물리 용량이다.
SUBSCRIBER, HANDLE = 0x540c74, 0x540ca8  # 등록helper가 쓰는현재ID와부모가이미캡처한뒤인global이름포인터다.
SELECTED = (0x4bd400, 0x4b38d0, 0x4bcbf0, 0x44cce0)  # 부모/실제등록/기존문자열/일반write만원문CPU로실행한다.


def digest(path):
    """현재 원문 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fnv(data):
    """출력 전체 바이트를 고정64비트 FNV1a로 관측하며 제품 결과를 입력하지 않는다."""
    value = 0xcbf29ce484222325
    for byte in data:  # 모든 물리 BYTE와 미사용 guard를 순서대로 포함한다.
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return f'{value:016x}'


def name_input(kind):
    """NUL/빈이름/255·256·65536길이와 원래 포인터의 물리 범위를 명시한다."""
    if kind == 3:
        return b'A\0BC\0'
    if kind in (4, 5, 6):
        length = {4: 255, 5: 256, 6: 65536}[kind]
        return bytes(65 + index % 23 for index in range(length)) + b'\0'
    return b'\0' if kind == 2 else b'Alpha\0'


def cases():
    """정상 경계와 의미 있는 재조회/현재값 변경/중단을 각각 독립 입력한다."""
    base = (1, 11, 0x12345678, 0x12345678, 0, 0, 0xffffffff, 9, 0, 0, 0, 64, 'none')
    variants = [('default', {})]
    for kind in range(1, 7):  # null 포인터와 살아 있는 빈 이름을 구별한다.
        variants.append(('name' + str(kind), {4: kind}))
    variants += [('id0', {2: 0}), ('id-max', {2: 0xffffffff}), ('register-read', {3: 0xffffffff, 5: 1, 6: 7}),
        ('register-generate', {3: 0xffffffff}), ('register-false-changed', {3: 0xffffffff, 6: 7}),
        ('generated-max', {3: 0xffffffff, 7: 0xffffffff}), ('stream1', {8: 1}),
        ('offset', {9: 3, 10: 20}), ('capacity', {11: 0}), ('manager0', {1: 0})]
    mutations = ('assert-manager22', 'assert-stop', 'register-manager22', 'register-id9',
        'register-pointer2', 'register-name', 'register-grow', 'register-stop', 'read-manager22',
        'read-id9', 'generate-name', 'config-name', 'create-manager0', 'create-name', 'create-stop',
        'write1-name', 'write1-grow', 'write1-pointer2', 'write1-id9', 'write1-stop',
        'write2-name', 'write2-grow', 'write2-stop', 'write3-name', 'write3-stop',
        'capacity-streamB', 'capacity-stop', 'null-stop')
    for mutation in mutations:  # 불필요한 전체 조합 대신 callback 위치마다 한 의미를 관측한다.
        change = {12: mutation}
        if mutation.startswith('assert'):
            change[1] = 0
        if mutation.startswith(('read', 'generate', 'config')):
            change[3] = 0xffffffff
        if mutation.startswith('capacity'):
            change[11] = 0
        if mutation == 'null-stop':
            change[4] = 1
        variants.append((mutation, change))
    result = []
    for label, changes in variants:  # 명시 입력만 바꾸며 효과/분기는 실제 CPU가 결정한다.
        value = list(base)
        for index, field in changes.items():  # 지정 필드 이외의 기본 입력은 유지한다.
            value[index] = field
        result.append((label, tuple(value)))
    return result


class Oracle(MemoryOracle):
    """선택한 PE 네 몸체를 실행하고 Create/Config/진단/새 ID 생성만 외부 장치로 둔다."""
    def __init__(self):
        """보호 PE를 격리 복사하고 실제 함수표와 CPU 쓰기 범위를 제한한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(source, delimiter='\t')}
        self.allowed = []  # 함수표가 허용한 실제 PE 명령 구간만 저장한다.
        for entry in SELECTED:  # 함수표의 불연속 실제 명령 범위만 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total = 0  # 모든 관측의 실제 CPU 명령 수다.
        self.boundaries = Counter()  # 실제 도달한 외부 장치 종류/순번의 누적 관측이다.
        self.statuses = Counter()  # 정상/명시 중단/Create-null 경계 결과를 구별한다.

    def guard(self, mu, access, address, size, value, data):
        """CPU는 stack/currentID/두stream/두물리출력 밖에 쓸 수 없다."""
        ranges = [(STACK - 0x1000, STACK + 16), (SUBSCRIBER, SUBSCRIBER + 4)]
        ranges += [(node, node + 16) for node in STREAMS]
        ranges += [(buffer, buffer + PHYSICAL) for buffer in BUFFERS]
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'SubscriberWrite CPU 쓰기 범위 밖 {address:08x}/{size}')

    def event(self, boundary, text):
        """실제 하위 진입 순서를 기록하고 지정한 외부 변경만 해당 시점에 적용한다."""
        self.events.append(text)
        self.boundaries[boundary] += 1
        mutation = self.case[-1]
        if '-' not in mutation:
            return
        trigger, action = mutation.split('-', 1)
        if trigger != boundary:
            return
        if action == 'stop':
            self.status = 'stop:' + boundary
            self.mu.emu_stop()
        elif action == 'manager22':
            self.put(HEADER, 22)
        elif action == 'manager0':
            self.put(HEADER, 0)
        elif action == 'id9':
            self.put(SUBSCRIBER, 9)
        elif action == 'pointer2':
            self.put(HANDLE, NAMES[1])
        elif action == 'name':
            self.mu.mem_write(NAMES[0], b'Z\0Y')
        elif action == 'grow':
            self.mu.mem_write(NAMES[0], b'Grown Word\0')
        elif action == 'streamB':
            node = STREAMS[self.case[8]]
            self.mu.mem_write(node, struct.pack('<4I', BUFFERS[1], 4, 2, PHYSICAL))
        else:
            raise RuntimeError('SubscriberWrite 변경 미해석:' + mutation)

    def instruction(self, mu, address, size, data):
        """부모/등록/문자열/일반write를 실제 실행하며 외부 하위만 명시 반환한다."""
        self.instructions += 1
        self.total += 1
        if self.instructions > 1000000:
            raise RuntimeError('SubscriberWrite 명령 상한')
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        args = self.words(esp + 4, 4)
        if address == 0x4b38d0:
            self.event('register', 'H')
        elif address == 0x44cce0:
            self.writes += 1
            self.event('write' + str(self.writes), f'W:{self.writes}:{args[1]}:{fnv(bytes(mu.mem_read(args[0], args[1])))}')
        elif address == 0x44d1b0:
            if self.string(args[0]) != b'Subscriber':
                raise RuntimeError('Subscriber 아닌 section 생성')
            self.event('create', 'C:' + str(ecx))
            if self.status == 'normal':
                if not ecx:
                    self.status = 'fault:manager0'; mu.emu_stop()
                else:
                    self.ret(STREAMS[self.case[8]], 4)
            return
        elif address == 0x4e0620:
            line = args[2]
            boundary = {0x1cf: 'assert', 0x9b: 'null', 0x24: 'capacity'}[line]
            self.event(boundary, 'A:' + str(line))
            if self.status == 'normal':
                self.ret()
            return
        elif address == 0x441270:
            if args[1] != SUBSCRIBER:
                raise RuntimeError('현재 Subscriber 참조가 아닌 ReadInt')
            self.put(SUBSCRIBER, self.case[6])
            self.event('read', 'R:' + self.string(args[0]).decode('ascii'))
            if self.status == 'normal':
                self.ret(self.case[5])
            return
        elif address == 0x4bc950:
            self.event('generate', 'G')
            if self.status == 'normal':
                self.ret(self.case[7])
            return
        elif address == 0x4e58e9:
            if self.string(args[1]) != b'registerdSubId = "%u"':
                raise RuntimeError('등록 helper 원문 설정 형식 차이')
            value = b'registerdSubId = "' + str(args[2]).encode('ascii') + b'"'
            mu.mem_write(args[0], value + b'\0'); self.ret(len(value)); return
        elif address == 0x441580:
            self.event('config', 'F:' + self.string(args[0]).decode('ascii'))
            if self.status == 'normal':
                self.ret()
            return
        if self.status == 'normal' and not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'SubscriberWrite 관측 범위 밖 {address:08x}')

    def sample(self, label, case, control):
        """독립 입력만 설치하고 출력전체hash/필드/원문이름/부분실패/trace를 관측한다."""
        self.case, self.status, self.events = case, 'normal', []  # 현재 입력/부분실패 상태/효과 순서를 같은 관측 동안 유지한다.
        self.instructions, self.writes = 0, 0  # 이번 CPU 명령 수와 실제 일반 write 진입 횟수다.
        self.mu.mem_write(HEADER, b'\0' * 20); self.put(HEADER, case[1])
        self.put(SUBSCRIBER, case[3]); self.put(HANDLE, NAMES[0])
        for index, name in enumerate(NAMES):  # 캡처 포인터와 교체 포인터의 내용을 구별한다.
            self.mu.mem_write(name, b'\xcc' * PHYSICAL)
            self.mu.mem_write(name, name_input(case[4]) if not index else b'Other\0')
        for index, node in enumerate(STREAMS):  # 두 반환 stream과 각 물리출력의 초기입력이다.
            self.mu.mem_write(node, struct.pack('<4I', BUFFERS[index], case[9], case[10], case[11]))
            self.mu.mem_write(BUFFERS[index], b'\xcc' * PHYSICAL)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # CPU 일반 레지스터는 입력마다 초기화한다.
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_ECX, HEADER); self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.mem_write(STACK, struct.pack('<III', STOP, case[2], 0 if case[4] == 1 else NAMES[0]))
        self.mu.emu_start(0x4bd400, STOP)
        if self.status == 'normal' and self.mu.reg_read(UC_X86_REG_ESP) != STACK + 12:
            raise RuntimeError('SubscriberWrite ret8 스택 정리 차이')
        words = []
        for node in STREAMS:  # 포인터만 두 독립버퍼 번호로 정규화한다.
            value = self.words(node, 4); words.extend((BUFFERS.index(value[0]) + 1, *value[1:]))
        outputs = [bytes(self.mu.mem_read(buffer, PHYSICAL)) for buffer in BUFFERS]
        state = (self.words(SUBSCRIBER, 1)[0], self.words(HEADER, 1)[0], NAMES.index(self.words(HANDLE, 1)[0]))
        self.statuses[self.status] += 1
        return '\t'.join((label, str(control), ','.join(map(str, case)), self.status, ','.join(map(str, state)), ','.join(map(str, words)), ':'.join(fnv(output) for output in outputs), ':'.join(output[:64].hex() for output in outputs), ';'.join(self.events) or '-'))


def observe():
    """두 x87 입력의 독립 관측을 매번 실제 PE로 재실행한다."""
    oracle, rows = Oracle(), ['# label/control/slot,manager,capturedID,globalID,name,readResult,readID,newID,stream,pos,end,alloc,mutation/status/globalID,manager,handleIndex/twoStreams8DWORD/twoFullFNV/twoPrefix64/trace']
    for label, case in cases():  # 제품/fixture의 예상 결과를 입력하지 않고 같은 원문만 두 번 실행한다.
        first, second = oracle.sample(label, case, 0x27f), oracle.sample(label, case, 0x37f)
        if first.split('\t')[2:] != second.split('\t')[2:]:
            raise RuntimeError('SubscriberWrite x87 차이')
        rows += [first, second]
    return '\n'.join(rows) + '\n', oracle


def main():
    """최초 fixture를 새로 고정하거나 같은 도구/PE/fixture를 재실행 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); options = parser.parse_args()
    sources = {'originals/Netstorm.exe': ROOT / 'originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv': ROOT / 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py': ROOT / 'tools/decomp_priestmenu.py', 'tools/decomp_fort_subscriber_write.py': HERE}
    if options.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, path in sources.items():  # 기존 관측 근거와 현재 도구 바이트를 먼저 검증한다.
            if digest(path) != report['sha256'][name]:
                raise RuntimeError('SubscriberWrite 근거 SHA 차이:' + name)
        if digest(FIXTURE) != report['fixtureSha256']:
            raise RuntimeError('SubscriberWrite 최초 fixture SHA 차이')
        generated, oracle = observe()
        if generated.encode('utf-8') != FIXTURE.read_bytes():
            raise RuntimeError('SubscriberWrite 실제 PE 재실행 관측 차이')
        print(json.dumps({'verified': len(cases()) * 2, 'differences': 0, 'instructions': oracle.total})); return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 SubscriberWrite fixture/보고서 덮어쓰기 금지')
    generated, oracle = observe(); FIXTURE.parent.mkdir(parents=True, exist_ok=True); FIXTURE.write_text(generated, encoding='utf-8', newline='\n')
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'uniqueInputs': len(cases()), 'observations': len(cases()) * 2, 'instructions': oracle.total, 'statuses': dict(oracle.statuses), 'boundaries': dict(oracle.boundaries), 'scope': '004bd400/004b38d0/004bcbf0/0044cce0 실제 CPU. Create/진단/ReadInt/ID생성/설정/sprintf는 명시 장치. 전체 물리출력 FNV와64BYTE prefix/두stream/전역/순서를 관측. 실제 게임·OS·파일 저장 아님.', 'gameExecuted': False, 'osExecuted': False, 'originalFilesMutated': False, 'sha256': {name: digest(path) for name, path in sources.items()}, 'fixtureSha256': digest(FIXTURE)}
    REPORT.parent.mkdir(parents=True, exist_ok=True); REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8'); print(json.dumps({'observations': report['observations'], 'instructions': oracle.total, 'statuses': report['statuses']}))


if __name__ == '__main__':
    main()
