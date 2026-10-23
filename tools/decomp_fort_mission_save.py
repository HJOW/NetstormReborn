#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Mission 저장 00463a80/004bcbf0/00482c40과 실제 일반 MemStream 쓰기를 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

GUIDE = 0x562ca0  # 원본 Guide의 이름과+800 현재 Mission 포인터다.
MISSIONS = (0, POOL + 0x1000, POOL + 0x2000)  # 명시 Mission 두 개의 실제+54/+58 필드를 구별한다.
STREAM, TEMPLATE = POOL + 0x3000, POOL + 0x3500  # 실제16BYTE stream과 고정 Template 헤더다.
NAME = POOL + 0x20000  # WORD 길이 wrap 관측은 Guide 밖 독립 문자열 입력에서만 선택한다.
BUFFERS = (POOL + 0x40000, POOL + 0x60000)  # 서로 다른 현재 data 포인터 입력이다.
BUFFER_BYTES = 0x11000  # 가장 긴65537BYTE 원문과 주변 출력까지 보관한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-mission-save-x86.tsv'  # 제품보다 먼저 확정한 최초 독립 관측이다.
REPORT = ROOT / 'cpppj/fort-mission-save-native.json'  # 게임/OS 미실행과 명시 하위 경계를 기록한다.
RANGES = ((0x463a80, 0x463aab), (0x4bcbf0, 0x4bcc4c), (0x482c40, 0x482c79), (0x44cce0, 0x44cd40), (0x4bf94a, 0x4bf982), (0x4bfa71, 0x4bfaa9))  # 실제 CPU 실행만 허용할 PE 본체다.
ENTRIES = {'guide': 0x463a80, 'name': 0x4bcbf0, 'state': 0x482c40, 'battle': 0x4bf94a, 'fort': 0x4bfa71}  # 부모 두 개는 Mission 구간만 실행한다.


def fnv(data):
    """전체 물리 바이트를 고정64비트 FNV-1a로 관측한다. 예상 제품값을 생성하지 않는다."""
    value = 0xcbf29ce484222325
    # 길이에 관계없이 모든 원문 바이트를 순서대로 포함한다.
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return f'{value:016x}'


def text(case):
    """문자열 입력의 첫 NUL과 그 뒤의 살아 있는 메모리를 구별한다."""
    lengths = {0: 0, 1: 3, 3: 31, 4: 65535, 5: 65536, 6: 65537, 7: 0}
    if case == 2:
        return b'A\0BC\0'
    return bytes(65 + index % 23 for index in range(lengths[case])) + b'\0'


def cases():
    """현재 Mission 존재/이름/stream과 실제 선택 callback 변이를 독립 입력으로 교차한다."""
    rows = []
    # 일반 Battle도 Guide 현재가 있으면 같은 저장 부모를 실행한다.
    for mode, current, name, state in itertools.product(('guide', 'state'), (0, 1, 2), range(4), ((0, 0, 64), (3, 20, 64), (0, 0, 0), (0, 0xffffffff, 0xffffffff))):
        if mode == 'state' and not current:
            continue
        rows.append((mode, current, name, *state, 0x12345678, 0x89abcdef, 0xfedcba98, 0x76543210, 11, 'none'))
    # 길이 하위WORD와 실제 전체 길이는 독립 문자열 본체에서만65535 경계를 넘긴다.
    for name, state in itertools.product(range(8), ((0, 0, 0), (5, 10, BUFFER_BYTES))):
        rows.append(('name', 1, name, *state, 1, 2, 3, 4, 11, 'none'))
    mutations = ('write1-current2', 'write2-current2', 'write3-current2', 'write1-name', 'write2-name', 'write1-sequence', 'write3-flags', 'write4-flags', 'write2-current0', 'capacity-current', 'capacity-stop', 'write2-stop', 'write3-stop', 'write1-physical', 'create-current2', 'create-stop', 'assert-manager22', 'manager-stop')
    # callback 자체의 값만 바꾸며 재조회 대상과 조건/반복은 실제 명령이 결정한다.
    for mode, mutation in itertools.product(('guide',), mutations):
        rows.append((mode, 1, 1, 0, 0, 0 if mutation.startswith('capacity') else 64, 0x12345678, 0x89abcdef, 0xfedcba98, 0x76543210, 0 if mutation in ('assert-manager22', 'manager-stop') else 11, mutation))
    rows.append(('name', 1, 7, 0, 0, 64, 1, 2, 3, 4, 11, 'null-stop'))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """실제 Guide/문자열/필드 저장 및44cce0을 실행하며 Create/진단만 외부 경계로 둔다."""
    def __init__(self):
        """보호 PE를 격리 메모리에 복사하고 CPU 쓰기를 검토 범위로 제한한다."""
        super().__init__()
        self.allowed = RANGES
        self.total_instructions = 0
        self.normal_returns = 0
        self.stops = Counter()
        self.boundaries = Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """CPU는 스택/stream/두 물리출력만 쓸 수 있으며 callback 입력 변이는 별도로 공급한다."""
        allowed = ((STACK - 0x200, STACK + 24), (STREAM, STREAM + 16), *((base, base + BUFFER_BYTES) for base in BUFFERS))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'Mission CPU 쓰기 범위 밖 {address:08x}/{size}')

    def record(self, value):
        """실제 경계 진입 순서를 기록하고 명시 stop까지 앞선 효과를 보존한다."""
        self.events.append(value)

    def stop(self, cause):
        """정상 반환과 구별되는 명시 중단/물리 메모리 경계를 기록한다."""
        self.cause = cause
        self.mu.emu_stop()

    def mutate(self, boundary):
        """원문 실행 중 지정된 외부 장치 효과 한 개만 적용한다."""
        mutation = self.case[-1]
        if mutation == boundary + '-current2':
            self.put(GUIDE + 0x800, MISSIONS[2])
        elif mutation == boundary + '-current0':
            self.put(GUIDE + 0x800, 0)
        elif mutation == boundary + '-name':
            self.mu.mem_write(self.name, b'Z\0Y')
        elif mutation == 'write1-sequence' and boundary == 'write1':
            self.put(MISSIONS[1] + 0x58, 0x13579bdf)
        elif mutation == 'write3-flags' and boundary == 'write3':
            self.put(MISSIONS[1] + 0x54, 0x2468ace0)
        elif mutation == 'write4-flags' and boundary == 'write4':
            self.put(MISSIONS[1] + 0x54, 0x01020304)
        elif mutation == 'capacity-current' and boundary == 'capacity':
            self.mu.mem_write(STREAM, struct.pack('<4I', BUFFERS[1], 5, 2, BUFFER_BYTES))
            self.put(MISSIONS[1] + 0x58, 0x11223344)
        elif mutation == 'write1-physical' and boundary == 'write1':
            self.put(STREAM, 0)
        elif mutation == 'assert-manager22' and boundary == 'manager':
            self.put(TEMPLATE, 22)
        elif mutation == boundary + '-stop':
            self.stop(boundary)

    def instruction(self, mu, address, size, data):
        """일반 write는 hook 뒤 실제 기계어로 계속하여 진단/현재필드/복사 순서를 관측한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 400000:
            raise RuntimeError('Mission 원문 명령 상한 초과')
        if address in (0x4bf982, 0x4bfaa9):
            self.reached_end = True
            mu.emu_stop()
            return
        if address == 0x482c49 and mu.reg_read(UC_X86_REG_ESI) == 0:
            self.stop('mission0')
            return
        if address == 0x44cd15 and self.words(STREAM, 1)[0] == 0:
            self.stop('buffer0')
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 3)
        if address == 0x44cce0:
            self.writes += 1
            self.boundaries['write-observed-real-body'] += 1
            assert mu.reg_read(UC_X86_REG_ECX) == STREAM
            source = bytes(mu.mem_read(args[0], args[1]))
            self.record(f'W:{self.writes}:{args[1]}:{fnv(source)}')
            self.mutate('write' + str(self.writes))
        elif address == 0x4e0620:
            line = args[2]
            self.boundaries['assert'] += 1
            self.record('A:' + str(line))
            self.mutate({36: 'capacity', 155: 'null', 488: 'manager'}[line])
            if not self.cause:
                self.ret()
        elif address == 0x44d1b0:
            assert self.string(args[0]) == b'Mission'
            self.boundaries['create-explicit'] += 1
            self.record('C:' + str(mu.reg_read(UC_X86_REG_ECX)))
            self.mutate('create')
            if not self.cause:
                self.ret(STREAM, 4)
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError('Mission 원문 허용 범위 밖: ' + hex(address))

    def sample(self, case, control):
        """입력만 구성한 뒤 실제 원문의 종료·trace·전체 물리해시/주변바이트를 읽는다."""
        self.case = case
        mode, current, name_case, position, end, capacity, seq1, flags1, seq2, flags2, manager, mutation = case
        self.instructions = 0
        self.events = []
        self.writes = 0
        self.cause = ''
        self.reached_end = False
        self.name = NAME if mode == 'name' else GUIDE
        name = text(name_case)
        self.mu.mem_write(GUIDE, b'\0' * 0x804)
        self.mu.mem_write(self.name, name)
        self.put(GUIDE + 0x800, MISSIONS[current])
        self.put(MISSIONS[1] + 0x58, seq1)
        self.put(MISSIONS[1] + 0x54, flags1)
        self.put(MISSIONS[2] + 0x58, seq2)
        self.put(MISSIONS[2] + 0x54, flags2)
        self.put(TEMPLATE, manager)
        self.mu.mem_write(STREAM, struct.pack('<4I', BUFFERS[0], position, end, capacity))
        # 진단상 alloc보다 큰 물리출력도 명시 입력으로 남겨 진단 뒤 계속쓰기를 검증한다.
        for base in BUFFERS:
            self.mu.mem_write(base, b'\xcc' * BUFFER_BYTES)
        # 모든 입력마다 CPU 레지스터와 스택을 별도 초기화한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        if mode in ('fort', 'battle'):
            self.mu.reg_write(UC_X86_REG_ESI, TEMPLATE)
            self.mu.mem_write(STACK, struct.pack('<4I', 0x11223344, 0x55667788, STOP, 0))
        else:
            ecx = GUIDE if mode == 'guide' else STREAM if mode == 'name' else MISSIONS[current]
            argument = 0 if mode == 'name' and name_case == 7 else self.name if mode == 'name' else STREAM
            self.mu.reg_write(UC_X86_REG_ECX, ecx)
            self.mu.mem_write(STACK, struct.pack('<3I', STOP, argument, 0))
        try:
            self.mu.emu_start(ENTRIES[mode], STOP)
        except UcError:
            if mutation != 'write1-physical':
                raise
            self.cause = 'buffer0'
        completed = not self.cause
        if completed:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 8
            assert mode not in ('fort', 'battle') or self.reached_end
            self.normal_returns += 1
        else:
            self.stops[self.cause] += 1
        selected = MISSIONS.index(self.words(GUIDE + 0x800, 1)[0])
        state = [self.words(TEMPLATE, 1)[0], selected, self.words(MISSIONS[1] + 0x58, 1)[0], self.words(MISSIONS[1] + 0x54, 1)[0], self.words(MISSIONS[2] + 0x58, 1)[0], self.words(MISSIONS[2] + 0x54, 1)[0], self.writes]
        outputs = [bytes(self.mu.mem_read(base, BUFFER_BYTES)) for base in BUFFERS]
        raw = bytes(self.mu.mem_read(STREAM, 16)).hex() + '|' + '|'.join(fnv(output) + ':' + output[:64].hex() + ':' + output[-16:].hex() for output in outputs)
        return '\t'.join((str(control), ','.join(map(str, case)), '1' if completed else self.cause, ','.join(map(str, state)), raw, ';'.join(self.events)))


def main():
    """최초 fixture를 제품 작성 전에 고정하고 재실행은 SHA 검증만 허용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 기존 원문 근거를 제품 결과로 덮어쓰지 못하게 입력/도구/관측을 함께 확인한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Mission 최초 SHA 불일치: ' + name)
        print('Mission 최초 원문 SHA 보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 Mission fixture/보고서 덮어쓰기 금지')
    oracle = Oracle()
    inputs = cases()
    lines = ['# control / mode,current,name,pos,end,alloc,seq1,flags1,seq2,flags2,manager,mutation / completed-or-stop / current-state / stream+whole-buffer-fnv+surroundings / original-trace']
    # 두 x87 제어어는 서로 독립 실행하며 정수/문자열 부모의 차이가 없어야 한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    sections = []
    # 전체 부모는 병렬 별도 모듈이 소유하므로 이 기본블록 증거는 제품대조 fixture와 구별한다.
    for mode, manager, current, mutation in itertools.product(('fort', 'battle'), (0, 11), (0, 1), ('none', 'assert-manager22', 'create-current2')):
        case = (mode, current, 1, 0, 0, 64, 1, 2, 3, 4, manager, mutation)
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        sections.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_mission_save.py', 'cpppj/tests/fixtures/fort-mission-save-x86.tsv')
    report = {'status': '신규 독립 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entries': {key: f'{value:08x}' for key, value in ENTRIES.items()}, 'ranges': [[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'sectionObservations': len(sections), 'sectionEvidence': sections, 'normalReturnsIncludingSections': oracle.normal_returns, 'explicitStops': dict(oracle.stops), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제 Guide 현재관문/이름길이WORD+전체문자/문자열뒤Mission재조회/고정Mission sequence후flags/실제44cce0 진단·복사·pos/end. Fort/Battle Mission구간은보고서의별도관측이고제품대조fixture에합산하지않음. Create·진단callback은명시경계이며buffer0/Mission0은원본메모리접근직전명시물리중단. 전체snapshot/파일/OS/게임은실행하지않음.', 'gameExecuted': False, 'osExecuted': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'sectionObservations', 'normalReturnsIncludingSections', 'explicitStops', 'instructions')}))


if __name__ == '__main__':
    main()
