#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH Guide Run/Advance의 실제 기계어를 격리 관찰하며 외부 상태/UI/가상 단계는 명시 입력으로 공급한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원본 이미지와 분리한 두 실제 크기 미션/가상 표/Guide 입력 주소다.
FIRST, SECOND, VTABLE, GUIDE = POOL, POOL + 0x1000, POOL + 0x2000, POOL + 0x10000
SIZE = 0xd0  # 전체 미션 입력을 읽어 원래 다섯 상태 필드 밖의 stale BYTE도 보존한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/mission-guide-x86.tsv'  # 독립 실제 원문 출력과 경계 호출 순서다.
REPORT = ROOT / 'cpppj/mission-guide.json'  # 새 관찰과 C++ 빌드 대기를 구분한다.


def initial(identifier, flags, sequence):
    """기대 동작을 계산하지 않고 독립 원문 입력과 다섯 상태 필드를 준비한다."""
    seed = 0x29 if identifier == 1 else 0xa7
    data = bytearray((index * 37 + seed) & 255 for index in range(SIZE))
    for offset, value in ((0, VTABLE), (0x54, flags), (0x58, sequence), (0xa0, 0x12345678 + identifier), (0xc0, 0x89abcdef - identifier), (0xc4, 0x13579bdf + identifier)):
        struct.pack_into('<I', data, offset, value)
    return bytes(data)


def inputs():
    """현재포인터/flags/단계/짧은평가/외부 변경의 명시 입력을 교차한다. 기대 결과는 PE에서만 얻는다."""
    cases = set()
    # 입력 순서는 active,flags1,seq1,flags2,seq2,pending,dialogs,trigger,action,value,ordinal,budget,runs다.
    def add(kind='R', active=1, flags=0, seq=0, flags2=0x10000000, seq2=2, pending=0, dialogs=1, trigger=0, action=0, value=0, ordinal=1, budget=1, runs=1):
        """중복 없는 원문 입력 행만 수집한다. 정상/오류 분기나 예상 출력은 계산하지 않는다."""
        cases.add((kind, active, flags, seq, flags2, seq2, pending, dialogs, trigger, action, value, ordinal, budget, runs))
    # 빈 Guide와0..8/최대DWORD, 취소/advance/높은flags, pending 짧은평가를 교차한다.
    for active, seq, flags, pending, dialogs in itertools.product((0, 1), (*range(9), 0xffffffff), (0, 1, 8, 9, 0x80000001, 0xffffffff), (0, 1, 0x80000000), (0, 1)):
        add(active=active, seq=seq, flags=flags, pending=pending, dialogs=dialogs)
    # 실제00482c80의 감김과flags의 bit1만 해제되는 범위를 별도로 관찰한다.
    for seq, flags in itertools.product((*range(9), 0xfffffffe, 0xffffffff), (0, 1, 2, 8, 0x80000000, 0x80000001, 0xfffffffe, 0xffffffff)):
        add(kind='A', seq=seq, flags=flags)
    # 가상 stage의 실제 Advance/flag요청/취소/명시단계/종료/다른 active 교체다.
    for seq, flags, action, runs in itertools.product(range(7), (0, 1), range(1, 9), (1, 2)):
        values = (0, 1, 5, 6, 7, 0xffffffff) if action in (4, 8) else (0,)
        for value in values:add(seq=seq, flags=flags, action=action, value=value, runs=runs)
    # 연속 단계 전환도6이 되면 그 Run을 종료하고 다음 Run에서만 stage6/release에 도달한다.
    for seq, runs in itertools.product(range(6), (1, 2)):add(seq=seq, action=1, budget=8, runs=runs)
    # pending/dialog 외부효과의 현재 미션 변경을 다음 원문 읽기에서 반영한다. 중간NULL역참조 입력은 제외한다.
    for trigger, action, pending, dialogs in itertools.product((1, 2), (1, 2, 3, 4, 6, 8), (0, 1), (0, 1)):
        add(trigger=trigger, action=action, value=7, pending=pending, dialogs=dialogs)
    # postpone/진단 뒤에도 같은 객체가 아니라 현재 포인터/단계를 다시 읽어 반복 여부를 결정한다.
    for trigger, seq, action, runs in itertools.product((3, 4), (0, 5, 7, 0xffffffff), range(1, 9), (1, 2)):
        add(trigger=trigger, seq=seq, action=action, value=6, pending=int(trigger == 3), runs=runs)
    # release는 명시 외부효과다. 기존 객체 해제 뒤 다른 미션이 설치되는 경계도 입력으로 관찰한다.
    for flags, seq, action in itertools.product((0, 8), (0, 6), (0, 6)):
        add(flags=flags, seq=seq, trigger=5, action=action, runs=2)
    # 실제 Guide+800의 각 읽기 위치에서 변경되는 getter 입력은 C++ 경계 횟수와도 직접 대조한다.
    for ordinal, action in itertools.product(range(1, 9), (2, 3, 4, 6, 8)):
        add(trigger=6, ordinal=ordinal, action=action, value=7, runs=2)
    # NULL은 최초/반복 뒤 포인터 검사의 정상 반환 위치에만 공급한다.
    for ordinal in (1, 5):add(trigger=6, ordinal=ordinal, action=5, runs=2)
    return sorted(cases)


class GuideOracle(MemoryOracle):
    """Run/Advance만 PE로 실행한다. pending/dialog/가상단계/release/assert는 명시 외부 효과다."""
    def __init__(self):
        """현재 PC 전체 정밀 함수표의 두 불연속 몸체만 허용하고 쓰기·포인터읽기를 감시한다."""
        super().__init__()
        self.allowed = []  # 미검토 함수/OS 진입은 허용하지 않는다.
        self.calls = {}  # 실제 Run/Advance의 누적 진입 횟수다.
        self.effects = {}  # 명시 대체 외부 경계의 누적 진입 횟수다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) in (0x463bb0, 0x482c80):
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        # 가상 주소는 실행하지 않고 해당 진입에서 명시 외부 경계로만 복귀한다.
        for index in range(8):self.put(VTABLE + 4 + index * 4, STOP + 0x100 + index * 16)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def active(self):
        """외부 효과가 참조하는 현재 입력 식별자를 읽는다. 이 호스트 읽기는 원본 getter 진입이 아니다."""
        pointer = self.words(GUIDE + 0x800, 1)[0]
        if pointer not in (0, FIRST, SECOND):raise RuntimeError('Guide current 입력 범위 오류')
        return 0 if pointer == 0 else 1 if pointer == FIRST else 2

    def select(self, identifier):
        """명시 외부 입력만 현재 Guide 필드에 쓴다. 원본 Run 계산을 대신하지 않는다."""
        self.put(GUIDE + 0x800, (0, FIRST, SECOND)[identifier])

    def mutate(self, trigger, target):
        """호출자가 지정한 외부 효과를 한정 횟수 적용하고 Advance는 실제 PE 몸체에 tailcall한다."""
        if trigger != self.trigger or self.budget == 0 or trigger == 6 and self.getters != self.ordinal:return None
        self.budget -= 1
        pointer = (0, FIRST, SECOND)[target]
        if self.action == 1 and pointer:return pointer
        if self.action in (2, 3, 4, 8) and pointer:
            flags = self.words(pointer + 0x54, 1)[0]
            if self.action in (2, 8):self.put(pointer + 0x54, flags | 1)
            if self.action == 3:self.put(pointer + 0x54, flags | 8)
            if self.action in (4, 8):self.put(pointer + 0x58, self.value)
        elif self.action == 5:self.select(0)
        elif self.action == 6:self.select(1 if target == 2 else 2)
        return None

    def complete_effect(self, trigger, target, result=0):
        """명시 외부 반환값을 보존하며 선택된 Advance 효과는 실제 함수로 실행한다."""
        pointer = self.mutate(trigger, target)
        if pointer:
            self.mu.reg_write(UC_X86_REG_EAX, result);self.mu.reg_write(UC_X86_REG_ECX, pointer);self.mu.reg_write(UC_X86_REG_EIP, 0x482c80)
        else:self.ret(result)

    def read(self, mu, access, address, size, value, data):
        """실제 Guide+800의 원문 읽기마다 current getter의 명시 입력과 호출 순서를 관찰한다."""
        if address != GUIDE + 0x800:return
        if size != 4:raise RuntimeError('Guide current 읽기 폭 오류')
        self.getters += 1
        pointer = self.mutate(6, self.active())
        if pointer:raise RuntimeError('Guide getter 입력은 실제 Advance 호출을 대신하지 않음')
        self.events.append('C,' + str(self.active()))

    def guard(self, mu, access, address, size, value, data):
        """실제 명령은 두 미션의flags/sequence와 한정된 스택 이외에 쓰지 못한다."""
        if STACK - 4096 <= address and address + size <= STACK + 4:return
        if any(pointer + 0x54 <= address and address + size <= pointer + 0x5c for pointer in (FIRST, SECOND)):return
        raise RuntimeError(f'Guide 원문 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """원본 두 몸체만 계산하며 외부 경계의 실제 호출 위치·this·진단 인자를 확인한다."""
        ecx = mu.reg_read(UC_X86_REG_ECX)
        if address in (0x463bb0, 0x482c80):self.calls[address] = self.calls.get(address, 0) + 1
        if address in (0x4b32e0, 0x4c8e90, 0x463ab0, 0x4e0620) or STOP + 0x100 <= address <= STOP + 0x170:
            self.effects[address] = self.effects.get(address, 0) + 1
            if address == 0x4b32e0:self.events.append('P');self.complete_effect(1, self.active(), self.pending);return
            if address == 0x4c8e90:self.events.append('D');self.complete_effect(2, self.active(), self.dialogs);return
            if address == 0x463ab0:
                if ecx != GUIDE:raise RuntimeError('Guide release this 오류')
                target = self.active();self.events.append('L');self.select(0);self.complete_effect(5, target);return
            if address == 0x4e0620:
                if self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 3) != (0x50161c, 0x50abe0, 0xaa):raise RuntimeError('Guide 원본 진단 인자 오류')
                self.events.append('A');self.complete_effect(4, self.active());return
            index = (address - STOP - 0x100) // 16
            if ecx not in (FIRST, SECOND):raise RuntimeError('Guide 가상 this 오류')
            target = 1 if ecx == FIRST else 2
            self.events.append(('H,' + str(target)) if index == 7 else f'S,{target},{index}')
            self.complete_effect(3 if index == 7 else 0, target);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):raise RuntimeError(f'Guide 미검토 명령 {address:08x}')

    def sample(self, case):
        """명시 입력을 쓰고 모든 Run/Advance의 정상 반환·스택·보존레지스터와 두 전체 객체를 읽는다."""
        kind, active, flags, seq, flags2, seq2, self.pending, self.dialogs, self.trigger, self.action, self.value, self.ordinal, self.budget, runs = case
        self.events = [];self.getters = 0
        self.mu.mem_write(FIRST, initial(1, flags, seq));self.mu.mem_write(SECOND, initial(2, flags2, seq2));self.select(active)
        preserved = {UC_X86_REG_EBX:0x11112222, UC_X86_REG_ESI:0x33334444, UC_X86_REG_EDI:0x55556666, UC_X86_REG_EBP:0x77778888}
        # 별도 Run 호출마다 정상 반환 주소와 비영 보존 값을 다시 설치한다.
        for unused in range(runs):
            for register, value in preserved.items():self.mu.reg_write(register, value)
            self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_EDX, 0xaaaabbbb)
            self.mu.reg_write(UC_X86_REG_ECX, GUIDE if kind == 'R' else FIRST);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.put(STACK, STOP)
            self.mu.emu_start(0x463bb0 if kind == 'R' else 0x482c80, STOP, count=100000)
            if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:raise RuntimeError('Guide 정상 반환/스택 오류')
            for register, value in preserved.items():
                if self.mu.reg_read(register) != value:raise RuntimeError('Guide 보존레지스터 오류')
        return self.active(), bytes(self.mu.mem_read(FIRST, SIZE)).hex(), bytes(self.mu.mem_read(SECOND, SIZE)).hex(), ';'.join(self.events) or '-'


def main():
    """새 native 입력을 보존하거나 기존 입력/현재 SHA만 감사한다. 공용 C++ 빌드는 실행하지 않는다."""
    if sys.argv[1:] == ['--verify']:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in facts['sha256'].items():
            if digest(ROOT / name) != expected:raise RuntimeError('Guide SHA 불일치: ' + name)
        rows = list(csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t'))[1:]
        if len(rows) != facts['observations'] or len({tuple(row[:2]) for row in rows}) != len(rows):raise RuntimeError('Guide 고유입력 수 오류')
        print('Guide native/SHA verified:', len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_mission_guide.py [--verify]')
    oracle = GuideOracle();rows = []
    for case in inputs():rows.append((case[0], ','.join(map(str, case[1:])), *oracle.sample(case)))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('kind', 'input', 'active', 'first', 'second', 'trace'));writer.writerows(rows)
    sources = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_mission_guide.py', 'cpppj/src/o/RawMissionGuide.h', 'cpppj/src/o/RawMissionGuide.cpp', 'cpppj/src/o/RawMissionStart.h', 'cpppj/tests/MissionGuideTests.cpp', 'cpppj/tests/fixtures/mission-guide-x86.tsv')
    facts = {'host':'HJOW-Athlon', 'date':'2026-10-15', 'status':'native-observed-product-unbuilt', 'observations':len(rows), 'run_inputs':sum(row[0] == 'R' for row in rows), 'advance_inputs':sum(row[0] == 'A' for row in rows), 'native_calls':{f'{address:08x}':count for address, count in oracle.calls.items()}, 'explicit_external_boundaries':{f'{address:08x}':count for address, count in oracle.effects.items()}, 'normal_return':True, 'stack_and_preserved_registers_checked':True, 'native_write_guard':'two mission flags/sequence fields and bounded stack only', 'game_gui_os_calls':0, 'sha256':{name:digest(ROOT / name) for name in sources if (ROOT / name).exists()}}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2)+'\n', encoding='utf-8');print('Guide native observations:', len(rows))


if __name__ == '__main__':main()
