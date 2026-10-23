#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 CheatAllTechnology의 실제 128바이트를 독립 실행하고 getter/contained 경계를 기록한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TYPES, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

# 최초 x86 관측을 보존할 fixture와 실행 근거 보고서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/theme-technology-x86.tsv'
REPORT = ROOT / 'cpppj/theme-technology-native.json'


def inputs():
    """필터·타입0·음수/빈 count와 세 외부효과의 현재 값 변경 입력을 교차한다."""
    rows = []
    # 고정 입력에서 선택 theme5가 항상 제외되는지와 이미 있는 항목을 관측한다.
    for theme, count, variant, pattern in itertools.product(range(-1, 7), (-1, 0, 1, 7, 16), range(3), range(2)):
        rows.append((theme, count, variant, pattern, 0, 0))
    # callback이 부모·개수를 바꾼 경우에도 원본이 현재 getter/전역을 다시 읽게 한다.
    for theme, variant, pattern, mutation, count_mode in itertools.product(range(-1, 7), range(3), range(2), range(5), range(4)):
        rows.append((theme, 9, variant, pattern, mutation, count_mode))
    return list(dict.fromkeys(rows))


def type_input(index, variant):
    """기대 기술 목록을 계산하지 않고 독립 TYPE 기록의 두 DWORD만 준비한다."""
    if variant == 0:
        return (10 if index % 5 == 0 else index % 6, index % 8 - 1)
    if variant == 1:
        return (10 if index % 7 == 6 else index * 3 % 11, (index + 3) % 8 - 1)
    return (10 if index % 4 == 2 else index % 3, (0, 1, 2, 3, 4, 5, -1, 6)[index % 8])


class Oracle(MemoryOracle):
    """00492ae0 본체만 실제 기계어로 실행하고 네 하위 경계는 명시 입력으로 둔다."""
    def __init__(self):
        """보호 PE를 격리 가상 메모리에 읽으며 OS/게임 진입을 열지 않는다."""
        super().__init__()
        self.allowed = [(0x492ae0, 0x492b60)]
        self.total_instructions = 0
        self.normal_returns = 0

    def change_count(self, boundary, type_num):
        """한 지정 callback에서만 count를 바꾸며 본체의 다음 전역 읽기로 전달한다."""
        mode = self.case[5]
        if self.count_changed:
            return
        if mode == 1 and boundary == 'find':
            self.put(0x541348, type_num + 1)
        elif mode == 2 and boundary == 'create':
            self.put(0x541348, 12)
        elif mode == 3 and boundary == 'record':
            self.put(0x541348, 12)
        else:
            return
        self.count_changed = True

    def instruction(self, mu, address, size, data):
        """필터·반복·현재 getter 재조회는 원본에 맡기고 실제 호출 인자/순서를 보존한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 5000:
            raise RuntimeError('테마 기술 격리 명령 상한 초과')
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 6)
        if address in (0x492ae0, 0x492b53):
            value = struct.unpack('<i', mu.mem_read(0x541348, 4))[0]
            self.events.append('N,' + str(value))
        if address == 0x49a840:
            number = args[0]
            if number >= 32:
                raise RuntimeError('명시 TYPE 입력 범위 밖')
            group, theme = type_input(number, self.case[2])
            self.events.append(f'R,{number},{group},{theme}')
            self.change_count('record', number)
            self.ret(TYPES + number * 500)
        elif address == 0x48fc60:
            parent = self.parent
            self.events.append(f'G,{parent}')
            if self.case[4] == 4:
                self.parent = 50 + (self.parent - 49) % 3
            self.ret(parent)
        elif address == 0x4acac0:
            parent = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
            mask, number, index = args[:3]
            result = self.contained.get((parent, number), 0)
            self.events.append(f'F,{parent},{mask},{number},{index},{result}')
            self.change_count('find', number)
            if self.case[4] in (1, 3):
                self.parent = 50 + (self.parent - 49) % 3
            self.ret(result, 12)
        elif address == 0x4afa70:
            parent = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
            number, argument, flags, attach, quality = args[:5]
            result = 700 + len(self.born)
            self.events.append(f'C,{parent},{number},{argument},{flags},{attach},{quality},{result}')
            self.born.append((parent, number, result))
            self.contained[(parent, number)] = result
            self.change_count('create', number)
            if self.case[4] in (2, 3):
                self.parent = 50 + (self.parent - 49) % 3
            self.ret(result, 20)
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'검토하지 않은 테마 기술 명령 {address:08x}')

    def sample(self, case):
        """독립 입력만 준비하고 정상 반환 뒤 실제 trace·현재 부모/count·생성 경계를 읽는다."""
        self.case = case
        self.parent = 50
        self.events = []
        self.born = []
        self.contained = {}
        self.instructions = 0
        self.count_changed = False
        self.put(0x541348, case[1])
        self.put(0x5c8464, POOL)
        # 세 부모와 타입32개의 초기 존재 여부를 외부 Find 경계의 명시 입력으로 둔다.
        for parent, number in itertools.product(range(50, 53), range(32)):
            if case[3] and (parent + number) % 3 == 0:
                self.contained[(parent, number)] = 1000 + parent * 32 + number
        # 두 원본 DWORD 이외에는 이번 본체가 타입 기록을 사용하지 않는다.
        for number in range(32):
            group, theme = type_input(number, case[2])
            self.put(TYPES + number * 500 + 0x9c, group)
            self.put(TYPES + number * 500 + 0x98, theme)
        # 저장된 CPU 상태가 입력 간 전달되지 않게 실제 정수 레지스터를 초기화한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case[0] & 0xffffffff))
        self.mu.emu_start(0x492ae0, STOP)
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('테마 기술 cdecl 반환 스택 불일치')
        self.normal_returns += 1
        count = struct.unpack('<i', self.mu.mem_read(0x541348, 4))[0]
        born = ';'.join(','.join(map(str, row)) for row in self.born) or '-'
        return '\t'.join((','.join(map(str, case)), ';'.join(self.events), str(count), str(self.parent), born))


def main():
    """최초 원본 결과를 덮어쓰지 않고 보존하거나 모든 근거 SHA를 읽기 전용 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(REPORT.read_text(encoding='utf-8'))
        # 최초 PE와 원문·도구·fixture 중 하나라도 바뀌면 기존 관측을 재사용하지 않는다.
        for name, expected in record['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('최초 테마 기술 근거 SHA 불일치: ' + name)
        rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows) == record['inputs']
        assert len({row.split('\t')[0] for row in rows}) == len(rows)
        print('테마 기술 최초 x86 입력/SHA 보존 확인:', len(rows))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 원본 fixture/보고서 덮어쓰기 금지')
    oracle = Oracle()
    cases = inputs()
    lines = ['# theme,count,variant,pattern,mutation,countMode / 실제 순서 / 최종 count / 현재 부모 / 생성 인자·반환이다.']
    # 제품 소스와 기대 목록을 읽지 않고 보호 PE의 정상 반환을 한 번씩 관측한다.
    for case in cases:
        lines.append(oracle.sample(case))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_theme_technology.py', 'cpppj/tests/fixtures/theme-technology-x86.tsv')
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '00492ae0', 'range': '00492ae0-00492b5f', 'inputs': len(cases), 'normalReturns': oracle.normal_returns, 'instructions': oracle.total_instructions, 'scope': '실제 128바이트의 타입0부터 현재 count 재조회/필터/Current 재조회/Find/Create 호출. 타입 getter49a840·Current48fc60·Find4acac0·Create4afa70은 명시 외부 경계다. contained 하위/실제 게임/OS/UI 종료는 실행하지 않는다.', 'gameExecuted': False, 'osExecuted': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('inputs', 'normalReturns', 'instructions')}))


if __name__ == '__main__':
    main()
