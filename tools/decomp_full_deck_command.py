#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 Deck 명령 부모의 실제 Deck·난수 하위와 UserInput 관문을 제품 작성 전에 고정한다."""
import argparse
import copy
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_deck_restore import Oracle as DeckOracle, ROOT, DECK, RANDOM, TYPE_COUNT, EXCLUDED_A, EXCLUDED_B, TYPES, STACK, STOP, initial_deck, digest
from decomp_game_menu_actions import Oracle as MenuOracle, cases as menu_cases, packed, GLOBALS, SCHEMA
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

FULL, OWNER, DIRTY, PARENT, GET = 0x54db14, 0x540cac, 0x55a4a4, 0x44f9d0, 0x44f700  # 원문 부모의 실제 전역과 두 함수 주소다.
PARENT_FIXTURE = ROOT / 'cpppj/tests/fixtures/full-deck-command-x86.tsv'  # 실제 부모와 실제 하위 관측이다.
INPUT_FIXTURE = ROOT / 'cpppj/tests/fixtures/full-deck-input-x86.tsv'  # 실제 Guide·Gump·UserInput 관문의 관측이다.
REPORT = ROOT / 'cpppj/full-deck-command-native-20261026.json'  # 두 실행 범위와 명시 외부 장치를 구별한다.


class ParentOracle(DeckOracle):
    """기존 여섯 Deck 몸체에 명령 부모를 더하고 GetDeck 반환 메모리만 명시 장치로 둔다."""
    def __init__(self):
        """같은 보호 이미지와 검증된 Deck 실행기의 메모리·원문 범위를 사용한다."""
        self.parent_trace = []
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader((ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        self.parent_ranges = []
        for part in facts[PARENT]['ranges'].split(';'):  # 정밀본의 실제 부모 범위만 추가한다.
            low, high = (int(value, 16) for value in part.split('-'))
            self.parent_ranges.append((low, high + 1))

    def guard(self, mu, access, address, size, value, data):
        """추가 두 DWORD store를 실제 순서로 기록하고 기존 Deck·난수·스택 보호를 유지한다."""
        if address in (FULL, DIRTY) and size == 4:
            self.parent_trace.append(('F' if address == FULL else 'D') + ':' + str(value))
            return
        super().guard(mu, access, address, size, value, data)

    def mutate(self, boundary):
        """TYPE 조회가 부모 전역을 바꾸는 명시 재진입 입력도 같은 메모리에 반영한다."""
        if self.case[-1] == boundary + '-full0':
            self.put(FULL, 0)
            return
        super().mutate(boundary)

    def instruction(self, mu, address, size, data):
        """부모·FullDeck·난수는 실제 실행하고 선택된 GetDeck만 주소 반환 장치로 실행한다."""
        if any(low <= address < high for low, high in self.parent_ranges):
            self.instructions += 1
            self.total += 1
            return
        if address == GET:
            self.parent_trace.append('G:' + str(self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]))
            change = self.case[-1]
            if change == 'deck1-stop':
                self.stopped = True
                mu.emu_stop()
                return
            if change == 'deck1-full0':
                self.put(FULL, 0)
            elif change == 'deck1-owner8':
                self.put(OWNER, 8)
            elif change == 'deck1-dirty1234':
                self.put(DIRTY, 1234)
            self.ret(DECK)
            return
        if address == 0x44f680:
            self.parent_trace.append('L')
        super().instruction(mu, address, size, data)

    def sample_parent(self, values, control):
        """명시 초기 입력에서 부모 전체를 실행하고 Deck352BYTE·난수·현재 전역을 관측한다."""
        random, owner, full, dirty, count, profile, mutation = values
        self.case = (2, random, 0, 0, 0, 0, 0, 4, count, profile, 2, 4, mutation)
        self.trace, self.calls, self.parent_trace = [], Counter(), []
        self.instructions, self.stopped, self.entry, self.current_type = 0, False, DECK, TYPES
        self.mu.mem_write(DECK, initial_deck(4))
        for address, value in ((RANDOM, random), (OWNER, owner), (FULL, full), (DIRTY, dirty), (TYPE_COUNT, count), (EXCLUDED_A, 2), (EXCLUDED_B, 4)):  # 기대 계산 없이 원시 전역만 설치한다.
            self.put(address, value)
        self.mu.mem_write(TYPES, bytes(260 * 500))
        for index in range(260):  # 기존 독립 primitive와 같은 TYPE 명시 입력이다.
            self.put(TYPES + index * 500 + 0x9c, 10 if profile != 1 or index % 2 else 9)
            self.put(TYPES + index * 500 + 0xec, 256 if profile == 2 else 0)
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # CPU 입력을 매 관측 독립화한다.
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(PARENT, STOP)
        completed = not self.stopped
        self.normal += int(completed)
        self.stops += int(not completed)
        if completed and self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('전체 Deck 부모의 실제 스택 복구 불일치')
        globals_ = [self.words(address, 1)[0] for address in (FULL, OWNER, DIRTY, RANDOM, TYPE_COUNT)]
        return '\t'.join((str(control), ','.join(map(str, values)), str(int(completed)), ','.join(map(str, globals_)), bytes(self.mu.mem_read(DECK, 352)).hex(), ';'.join(self.trace) or '-', ';'.join(self.parent_trace), str(self.instructions)))


class InputOracle(MenuOracle):
    """전체 UserInput 현재 사건 경로를 실행하며 부모 호출만 별도 관측된 효과 경계로 둔다."""
    def instruction(self, mu, address, size, data):
        """Guide/Gump/release/Editor·mission 관문 뒤 실제 선택된 부모 호출만 기록한다."""
        if address == PARENT:
            self.event('fullDeck')
            self.returned()
            return
        super().instruction(mu, address, size, data)


def main():
    """기존 fixture를 덮어쓰지 않고 실제 CPU 관측 또는 최초 SHA 검증을 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in report['sha256'].items():  # 최초 원문·도구·자료의 동일성을 확인한다.
            if digest(ROOT / name) != expected:
                raise RuntimeError('전체 Deck 최초 SHA 불일치: ' + name)
        print(json.dumps({'verified': True, 'parentObservations': report['parentObservations'], 'inputObservations': report['inputObservations']}))
        return
    if any(path.exists() for path in (PARENT_FIXTURE, INPUT_FIXTURE, REPORT)):
        raise RuntimeError('최초 전체 Deck 증거 덮어쓰기 금지')
    parent = ParentOracle()
    values = [(random, owner, 0x80000000, 0xfedcba98, count, profile, 'none') for random, owner, count, profile in itertools.product((0, 1, 0x12345678), (0, 1, 8), (-1, 0, 6, 66), range(3))]  # 정상 값과 비영 DWORD를 교차한다.
    values += [(1, 1, 0xffffffff, 77, 6, 1, mutation) for mutation in ('deck1-stop', 'deck1-full0', 'deck1-owner8', 'deck1-dirty1234', 'type1-stop', 'type1-full0', 'type1-count0')]  # 콜백 복귀·부분 중단 뒤 현재 메모리를 확인한다.
    lines = ['# control / random,owner,full,dirty,typeCount,profile,mutation / completed / full,owner,dirty,random,typeCount / deck352 / typeTrace / parentTrace / instructions']
    for value in values:  # 각 입력을 두 x87 정밀도로 독립 실행한다.
        for control in (0x27f, 0x37f):
            lines.append(parent.sample_parent(value, control))
    source_case = menu_cases()[0]
    input_ = InputOracle()
    command = input_.number(0x542a28)
    input_lines = ['# control\t' + '\t'.join(SCHEMA)]
    input_count = 0
    for editor, mission, flag, guide, gump in itertools.product((0, 1, 0x80000000, 0xffffffff), (0, 1, 0x80000000, 0xffffffff), (0, 0x80000000, 0x38000000), (0, 1), (0, 1)):  # 실제 명령 관문을 전부 원본으로 실행한다.
        case = copy.copy(source_case)
        globals_ = list(map(int, case['globals'].split(':')))
        globals_[0], globals_[3] = editor, mission
        case.update(name=f'full-{editor}-{mission}-{flag}-{guide}-{gump}', event=packed((command | flag, 23, 99)), globals=packed(globals_), guideResult=guide, gumpResult=gump)
        for control in (0x27f, 0x37f):
            output = {**case, **input_.run(case, control)}
            input_lines.append(str(control) + '\t' + '\t'.join(str(output[key]) for key in SCHEMA))
            input_count += 1
    PARENT_FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    INPUT_FIXTURE.write_text('\n'.join(input_lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_deck_restore.py', 'tools/decomp_production_shortcuts.py', 'tools/decomp_game_menu_actions.py', 'tools/decomp_full_deck_command.py', 'cpppj/tests/fixtures/full-deck-command-x86.tsv', 'cpppj/tests/fixtures/full-deck-input-x86.tsv')
    result = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'parentObservations': len(values) * 2, 'inputObservations': input_count, 'normalParentReturns': parent.normal, 'explicitParentStops': parent.stops, 'parentRanges': parent.parent_ranges, 'nativeInputCommand': command, 'actualFullDeckAndRandomExecuted': True, 'getDeckIsExplicitBoundary': True, 'inputParentIsExplicitBoundary': True, 'originalGameExecuted': False, 'osExecuted': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result[key] for key in ('parentObservations', 'inputObservations', 'normalParentReturns', 'explicitParentStops')}))


if __name__ == '__main__':
    main()
