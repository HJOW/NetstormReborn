#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 사제 메뉴·행·선택·테마·프레임의 실제 기계어를 격리 관찰한다. 게임/OS 진입은 허용하지 않는다."""
import csv
import hashlib
import json
import random
import re
import struct
import sys
from pathlib import Path

# 실제 저장소와 격리 Python 의존성의 기준 경로다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 가상 Pool/Player/TYPE/문자열/스택/정상반환 주소는 원본 이미지와 겹치지 않는다.
POOL, PLAYERS, TYPES, TEXT, STACK, STOP = 0x12000000, 0x12200000, 0x12300000, 0x12400000, 0x3001f000, 0x20000000
# 메뉴가 사용하는 실제 타입 순서다. 입력 mask의 비트와 일대일 대응한다.
ORDER = [123, 124, 125, 85, 86, 87, 88, 126, 165, 164]


def digest(path):
    """근거/소스/fixture의 실제 바이트 SHA-256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fbits(value):
    """SP float를 십진 문자열의 반올림 없이 보존한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


class Oracle:
    """검토된 함수/CRT 범위만 실행하고 실제 효과 경계의 인자·순서를 기록한다."""
    def __init__(self):
        """보호 PE를 읽기 전용으로 복사하고 서로 분리된 가상 저장소를 만든다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        for address, size in ((POOL, 0x100000), (PLAYERS, 0x10000), (TYPES, 0x20000), (TEXT, 0x10000), (STACK - 0x1f000, 0x20000), (STOP, 0x1000)):
            self.mu.mem_map(address, size)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        addresses = [0x494870, 0x461cf0, 0x461bd0, 0x493ac0, 0x4ad710, 0x491af0, 0x491b10, 0x491a40, 0x4e49c0]
        self.allowed = []
        for address in addresses:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)
        self.instructions = 0

    def put(self, address, value, width=4):
        """명시 입력의 little endian 정수 필드를 쓴다."""
        self.mu.mem_write(address, int(value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))

    def words(self, address, count):
        """현재 스택 인자를 원본 DWORD 순서로 읽는다."""
        return struct.unpack('<' + 'I' * count, self.mu.mem_read(address, count * 4))

    def string(self, address):
        """명시 문자열의 NUL 종료 바이트를 읽으며 비정상 길이를 거부한다."""
        result = bytearray()
        for offset in range(8192):
            byte = self.mu.mem_read(address + offset, 1)[0]
            if not byte:
                return bytes(result)
            result.append(byte)
        raise ValueError('NUL 없는 메뉴 문자열')

    def ret(self, value=0, purge=0):
        """명시 경계의 실제 cdecl/thiscall 정리 규약으로 복귀한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        target = self.words(esp, 1)[0]
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, mu, address, size, data):
        """게임 계산은 실제 명령으로 실행하고 외부효과/자료getter만 관찰 경계로 둔다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        ecx = mu.reg_read(UC_X86_REG_ECX)
        args = self.words(esp + 4, 8)
        if address == 0x49a840:
            self.ret(TYPES + args[0] * 500)
        elif address == 0x49af80:
            self.ret(TEXT + ((ecx - TYPES) // 500) * 64)
        elif address == 0x4de9a0:
            self.ret(args[0], 4)
        elif address in (0x4a89c0, 0x4a89e0):
            target, source = args[:2]
            pattern = self.string(source).decode('ascii')
            index = 2
            values = []
            for match in re.finditer(r'%(?:%|s|d)', pattern):
                code = match[0]
                if code == '%%':
                    continue
                word = args[index]
                index += 1
                values.append(self.string(word).decode('ascii') if code == '%s' else struct.unpack('<i', struct.pack('<I', word))[0])
            result = pattern % tuple(values)
            if address == 0x4a89e0:
                result = self.string(target).decode('ascii') + result
            mu.mem_write(target, result.encode('ascii') + b'\0')
            self.ret(target)
        elif address == 0x4c2400:
            self.ret(int(bool(self.allowed_mask & (1 << ORDER.index(args[0])))))
        elif address == 0x41c7a0:
            self.ret(int(bool(self.excluded_mask & (1 << ORDER.index(args[0])))), 4)
        elif address == 0x40ec50:
            money = self.money[min(self.money_calls, len(self.money) - 1)]
            self.money_calls += 1
            self.put(args[1], money)
            self.ret()
        elif address == 0x48fc60:
            self.ret(20)
        elif address == 0x4afb40:
            self.ret(self.blocked if args[0] == 0x493 else self.building if args[0] == 0x25d else 0, 4)
        elif address == 0x427030:
            self.ret(self.immobile)
        elif address == 0x4acac0:
            self.ret(args[1] if (ecx == POOL + 20 * 50 and self.contents & 1) or (ecx == POOL + 50 * 50 and self.contents & 2) else 0, 12)
        elif address == 0x493ac0 and not self.real_theme:
            self.ret(int(bool(self.theme_mask & (1 << args[0]))))
        elif address == 0x4b2270:
            self.cursor = 0
            self.put(ecx + 8, self.theme_nodes[0] if self.theme_nodes else 0)
            self.ret(purge=4)
        elif address == 0x4b1b20:
            self.cursor += 1
            self.put(ecx + 8, self.theme_nodes[self.cursor] if self.cursor < len(self.theme_nodes) else 0)
            self.ret()
        elif address == 0x477ae0:
            self.events.append('F');self.ret()
        elif address == 0x477bb0:
            assert args[0] == 0
            self.events.append('V');self.ret()
        elif address == 0x477ee0:
            self.events.append('T,' + self.string(args[0]).hex());self.ret()
        elif address == 0x475c00:
            self.taken += 1;self.ret(100 + self.taken)
        elif address == 0x478160:
            self.events.append('S,' + self.string(args[0]).hex() + ',' + ','.join(map(str, args[1:4])));self.ret()
        elif address == 0x477cb0:
            self.events.append('I,' + self.string(args[0]).hex() + ',' + ','.join(map(str, args[1:6])));self.ret()
        elif address == 0x478260:
            self.events.append('X');self.ret()
        elif address == 0x4288f0:
            self.events.append('B,' + str((ecx - POOL) // 50));self.ret()
        elif address == 0x4cf960:
            self.events.append('D,' + self.string(args[0]).hex());self.ret()
        elif address == 0x4afa70:
            assert args[1:5] == (0, 0x20, 0x10, 0)
            self.events.append('C,' + str((ecx - POOL) // 50) + ',' + str(args[0]));self.ret(70, 20)
        elif address == 0x447000:
            self.events.append('P,' + ','.join(map(str, args[:2])));self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'미등록 명령 {address:08x}'
            self.instructions += 1

    def prepare(self, values):
        """판단결과를 계산하지 않고 독립 raw 입력과 실제 TYPE 프레임을 준비한다."""
        owner, local, self.blocked, self.building, self.immobile, frame, modes, player, self.allowed_mask, self.theme_mask, self.contents, amount, money, self.excluded_mask = values
        self.events, self.taken, self.money_calls, self.real_theme = [], 0, 0, False
        self.money = [money]
        self.mu.mem_write(POOL, bytes(0x10000));self.mu.mem_write(PLAYERS, bytes(9 * 180));self.mu.mem_write(TYPES, bytes(188 * 500))
        self.put(0x5c8464, POOL);self.put(0x5c847c, 1000);self.put(0x59534c, PLAYERS);self.put(0x540c70, local)
        for address, bit in ((0x594fbc, 1), (0x55a480, 2), (0x5c85a4, 4), (0x594fa4, 8)):
            self.put(address, int(bool(modes & bit)))
        self.put(POOL + 50 * 50 + 34, owner, 1);self.put(POOL + 50 * 50 + 10, 158, 1)
        self.put(TYPES + 158 * 500 + 0x124, TEXT + 0x8000)
        self.put(TEXT + 0x8000, frame + 65, 1)
        for offset, bit in ((0x7c, 1), (0x84, 2), (0x8c, 4), (0x94, 8)):
            self.put(PLAYERS + owner * 180 + offset, int(bool(player & bit)))
        self.put(PLAYERS + local * 180 + 0x71, 3, 1)
        for type_number in range(188):
            self.put(0x59ab50 + type_number * 4, amount + type_number * 23)
            self.put(TYPES + type_number * 500 + 0x98, type_number % 4)
            self.mu.mem_write(TEXT + type_number * 64, ('Type' + str(type_number)).encode() + b'\0')

    def call(self, address, args=(), ecx=0):
        """보존 레지스터/스택/x87제어어를 확인하며 원본 함수의 정상반환만 인정한다."""
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_ECX, ecx)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        self.mu.emu_start(address, STOP, count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x027f
        return self.mu.reg_read(UC_X86_REG_EAX), ';'.join(self.events) or '-'


def main():
    """고정시드 경계입력을 실제 PE로 관찰하고 C++ fixture·바이트계보 보고서를 저장한다."""
    if '--verify' in sys.argv:
        report = json.loads((ROOT / 'cpppj/priestmenu.json').read_text(encoding='utf-8'))
        for path, expected in report['sha256'].items():
            assert digest(ROOT / path) == expected, f'근거 SHA 불일치: {path}'
        with (ROOT / 'cpppj/tests/fixtures/priestmenu-x86.tsv').open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        counts = {kind: sum(row[0] == kind for row in rows) for kind in report['counts']}
        assert len(rows) == report['observations'] and counts == report['counts']
        input_columns = {'Open': 15, 'Select': 7, 'Theme': 4, 'Frame': 2}
        assert len({tuple(row[:input_columns[row[0]]]) for row in rows}) == len(rows), '중복 입력 관찰'
        print(json.dumps({'verified': True, 'observations': len(rows), 'counts': counts}, ensure_ascii=False))
        return
    oracle = Oracle();rows = [];rng = random.Random(0x494870)
    base = [1, 1, 0, 0, 0, 0, 1, 0, 1023, 15, 1, 400, fbits(3000), 0]
    # 소유/493·신전/알타·모드/기술·상태·부족금액의 독립 조합이다.
    cases = [base.copy()]
    for column, choices in ((0, [0, 2]), (2, [1]), (3, [1]), (4, [1]), (5, [-1, 9, 10, 14, 15]), (6, range(16)), (7, range(16)), (8, [0, 7, 8, 120, 128, 256, 1023]), (9, range(16)), (10, range(4)), (12, [fbits(0), fbits(399), fbits(400), fbits(float('nan'))]), (13, [1, 128, 256, 1023])):
        for value in choices:
            case = base.copy();case[column] = value;cases.append(case)
    for index in range(256):
        case = [rng.choice([1, 1, 2]), 1, rng.randrange(4) == 0, rng.randrange(2), rng.randrange(2), rng.choice([0, 9, 10, 14, 15]), rng.randrange(16), rng.randrange(16), rng.randrange(1024), rng.randrange(16), rng.randrange(4), rng.choice([0, 1, 400, 16777217]), rng.choice([fbits(0), fbits(400), fbits(3000), fbits(16777216)]), rng.randrange(1024)]
        cases.append(list(map(int, case)))
    cases = list(dict.fromkeys(tuple(case) for case in cases))
    for case in cases:
        oracle.prepare(case);result, trace = oracle.call(0x494870, ecx=POOL + 50 * 50);rows.append(['Open', *case, trace])
    # 원본 첫float저장/둘째정수비교·같은시점이아닌두SP·NaN/음수·editor우회를 확인한다.
    for amount in [-3, 0, 400, 16777217, 2147483647]:
        for first, second in [(fbits(399), fbits(400)), (fbits(400), fbits(399)), (fbits(400), fbits(400)), (fbits(16777216), fbits(16777216)), (fbits(float('nan')), fbits(3000)), (fbits(3000), fbits(float('nan')))]:
            for mode in [0, 4]:
                case = base.copy();case[11] = amount;case[12] = first;case[6] = mode
                oracle.prepare(case);oracle.money = [first, second]
                result, trace = oracle.call(0x461bd0, (123, 50));rows.append(['Select', amount, first, second, mode, 0, 0, oracle.money_calls, trace])
    # 워크샾은실제선택의테마관문도거친다.
    for editor in [0, 4]:
        for theme in [0, 1]:
            case = base.copy();case[6] = editor;case[9] = theme << 3;oracle.prepare(case);oracle.put(TYPES + 123 * 500 + 0xec, 0x4000)
            result, trace = oracle.call(0x461bd0, (123, 50));rows.append(['Select', 400, fbits(3000), fbits(3000), editor, 1, theme, oracle.money_calls, trace])
    # 실제HasTheme몸체에독립내용물커서출력을주어flags4/테마/flags2800교차를확인한다.
    for mask in range(16):
        for banned in [0, 1, 8, 15]:
            for theme in range(4):
                oracle.prepare(base);oracle.real_theme = True;oracle.theme_nodes = [21, 22, 23, 24]
                for index in range(4):
                    oracle.put(POOL + (21 + index) * 50 + 18, 170 + index, 2)
                    oracle.put(POOL + (21 + index) * 50 + 20, 4 if mask & (1 << index) else 0, 2)
                    oracle.put(TYPES + (170 + index) * 500 + 0x98, index)
                    oracle.put(TYPES + (170 + index) * 500 + 0xec, 0x800 if banned & (1 << index) else 0)
                result, trace = oracle.call(0x493ac0, (theme,));rows.append(['Theme', mask, banned, theme, result])
    # 물리프레임코드첫바이트의전체signed-char범위를원본getter에서읽는다.
    for side in range(256):
        oracle.prepare(base);oracle.put(TEXT + 0x8000, side, 1)
        result, trace = oracle.call(0x4ad710, ecx=POOL + 50 * 50)
        rows.append(['Frame', side, struct.unpack('<i', struct.pack('<I', result))[0]])
    fixture = ROOT / 'cpppj/tests/fixtures/priestmenu-x86.tsv'
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n');writer.writerow(['kind', 'input_and_native_trace']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/priestmenu/originals/creation.c', 'extracted/priestmenu/originals/functions.tsv', 'tools/ghidra/priestmenu-functions.json', 'tools/decomp_priestmenu.py', 'cpppj/src/client/RawPriestMenu.cpp', 'cpppj/src/client/RawPriestMenu.h', 'cpppj/tests/PriestMenuTests.cpp', 'cpppj/tests/fixtures/priestmenu-x86.tsv']
    counts = {kind: sum(row[0] == kind for row in rows) for kind in ['Open', 'Select', 'Theme', 'Frame']}
    report = {'host': 'VM-W11-Codex', 'edition': 'PATCH10.78', 'observations': len(rows), 'counts': counts, 'instructions': oracle.instructions, 'boundaries': ['현재허용/비용/SP/TYPE설명/번역서식', 'Open/Select의테마query·내용물query와Theme실행의내용물커서', '실제HP불가/사건조회', '메뉴생성/작성/기본객체메뉴', 'ContainedCreate/실제커서취득'], 'limits': ['실제메뉴저장소/픽셀/게임커서/건설실행은이oracle관찰범위밖', 'CD10.72대응분석은미수행'], 'sha256': {path: digest(ROOT / path) for path in paths}}
    (ROOT / 'cpppj/priestmenu.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'observations': len(rows), 'counts': counts}, ensure_ascii=False))


if __name__ == '__main__':
    main()
