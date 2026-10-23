#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Editor Player004904b0의 실제273BYTE와 이름/필드/직접 기술생성 순서를 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

PLAYER_BASES = (PLAYERS, PLAYERS + 0x2000)  # callback이현재전역을바꾸어도캡처된기록주소는유지한다.
HANDLES = (POOL + 0x8000, POOL + 0x8100)  # 현재Client이름의서로다른실제64BYTE쓰기입력이다.
POOLS = (POOL, POOL + 0x80000)  # ContainedCreate직전현재Pool기준주소를구별한다.
PLAYER_BYTES = 9 * 180  # 원본9개의실제Player기록전체를관측한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/editor-players-x86.tsv'  # 제품보다먼저확정하는독립원문이다.
REPORT = ROOT / 'cpppj/editor-players-native.json'  # 실제몸체와명시하위경계를구별한다.
KEYS = (b'Human Player', b'Player', b'Computer #%d')  # 원본이직접조회하는번역키순서다.
TRANSLATIONS = ((b'Human Player', b'Player', b'Computer #%d'), (b'', b'', b''), (b'J\xf6ueur', b'Moi', b'Bot [%d]'))  # NUL/빈번역/원시CP1252입력이다.


def types(index, variant):
    """독립 TYPE입력의group/theme만만들며기대생성목록은계산하지않는다."""
    if variant == 0:
        return (10 if index % 5 == 4 else index % 4, 5 if index % 4 == 3 else index % 4)
    if variant == 1:
        return (10 if index % 2 else index % 4, 5 if index % 3 == 0 else index % 4)
    return (10, 0)


def cases():
    """타입0/빈signedcount·모든제외·이름·상위flags보존과callback현재변경을교차한다."""
    result = []
    # 모든owner는count와무관하게초기화하며eligible은원문이판정한다.
    for count, variant, language, flags, parent in itertools.product((-1, 0, 1, 7, 16), range(3), range(3), (0, 0xffffffff, 0xa5a50104), range(2)):
        result.append((count, variant, language, flags, parent, 'none'))
    mutations = ('human-players1', 'player-handle1', 'technology2-players1', 'computer2-players1', 'format2-players1', 'format2-flags', 'technology2-count3', 'format2-count0', 'record1-count0', 'record1-count12', 'create1-count12', 'record1-eligible', 'create1-pool1', 'create1-parentrecord', 'human-stop', 'player-stop', 'technology3-stop', 'format2-stop', 'record1-stop', 'create1-stop', 'finish-stop')
    # 한 callback만외부효과를공급하고나머지순서는실제CPU가결정한다.
    for mutation, variant in itertools.product(mutations, range(3)):
        result.append((7, variant, 0, 0xa5a501fc, 0, mutation))
    return list(dict.fromkeys(result))


class Oracle(MemoryOracle):
    """번역/Player부모/서식/TYPE/Contained/마지막후처리만명시경계로두고원본문체를실행한다."""
    def __init__(self):
        """보호PE를별도주소공간에읽고쓰기허용범위를제한한다."""
        super().__init__()
        self.allowed = ((0x4904b0, 0x4905c1),)
        self.total_instructions = 0
        self.normal_returns = 0
        self.stops = Counter()
        self.boundaries = Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """CPU쓰기는지역스택/두Player표/두현재handle밖으로나갈수없다."""
        allowed = ((STACK - 0x100, STACK + 4), *((base, base + PLAYER_BYTES) for base in PLAYER_BASES), *((base, base + 64) for base in HANDLES))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'Editor Player CPU쓰기범위밖 {address:08x}/{size}')

    def record(self, text, kind):
        """원문하위진입과명시인자를순서대로기록한다."""
        self.events.append(text)
        self.boundaries[kind] += 1

    def mutate(self, boundary):
        """본문이재조회할현재값또는캡처레코드의필드만명시입력대로바꾼다."""
        mutation = self.case[-1]
        if mutation == boundary + '-players1':
            self.put(0x59534c, PLAYER_BASES[1])
        elif mutation == 'player-handle1' and boundary == 'player':
            self.put(0x540ca8, HANDLES[1])
        elif mutation == 'format2-flags' and boundary == 'format2':
            self.put(self.captured_player + 0x50, 0x5a5a01fc)
        elif mutation == 'technology2-count3' and boundary == 'technology2':
            self.put(0x541348, 3)
        elif mutation == 'format2-count0' and boundary == 'format2':
            self.put(0x541348, 0)
        elif mutation == 'record1-count0' and boundary == 'record1':
            self.put(0x541348, 0)
        elif mutation in ('record1-count12', 'create1-count12') and boundary == mutation.split('-')[0]:
            self.put(0x541348, 12)
        elif mutation == 'record1-eligible' and boundary == 'record1':
            self.put(TYPES + self.type_index * 500 + 0x9c, 0)
            self.put(TYPES + self.type_index * 500 + 0x98, 0)
        elif mutation == 'create1-pool1' and boundary == 'create1':
            self.put(0x5c8464, POOLS[1])
        elif mutation == 'create1-parentrecord' and boundary == 'create1':
            self.put(self.words(0x59534c, 1)[0] + self.owner * 180 + 0x9c, 999)
        elif mutation == boundary + '-stop':
            self.cause = boundary
            self.mu.emu_stop()

    def instruction(self, mu, address, size, data):
        """필드쓰기·이름복사·필터/현재count와고정parent선택은실제273BYTE가수행한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 15000:
            raise RuntimeError('Editor Player 명령상한초과')
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 5)
        if address in (0x490559, 0x49059b):
            count = struct.unpack('<i', mu.mem_read(0x541348, 4))[0]
            self.count_reads += 1
            self.record(f'N:{self.owner}:{count}', 'count-read')
        if address == 0x4de9a0:
            key = self.string(args[0])
            index = KEYS.index(key)
            value = TRANSLATIONS[self.case[2]][index]
            self.record(f'T:{index}:{value.hex()}', 'translate')
            if index == 2:
                self.captured_player = mu.reg_read(UC_X86_REG_ESI)
            self.mutate(('human', 'player', 'computer' + str(self.owner))[index])
            if not self.cause:
                self.mu.mem_write(TEXT + index * 128, value + b'\0')
                self.ret(TEXT + index * 128, 4)
        elif address == 0x490250:
            self.owner = args[0]
            self.parent_calls += 1
            parent = 0 if self.case[4] and self.owner == 2 else 500 + self.owner if self.case[4] else 100 + self.owner
            self.record(f'P:{self.owner}:{parent}', 'technology')
            self.put(self.words(0x59534c, 1)[0] + self.owner * 180 + 0x9c, parent)
            self.mutate('technology' + str(self.owner))
            if not self.cause:
                self.ret(parent)
        elif address == 0x4b9390:
            capacity, source, number = args[:3]
            value = self.string(source)
            self.record(f'M:{capacity}:{number}:{value.hex()}', 'format')
            self.mutate('format' + str(self.owner))
            if not self.cause:
                formatted = value.replace(b'%d', str(number).encode())[:capacity - 1]
                self.mu.mem_write(TEXT + 0x400, formatted + b'\0')
                self.ret(TEXT + 0x400)
        elif address == 0x49a840:
            self.type_index = args[0]
            if self.type_index >= 32:
                raise RuntimeError('Editor TYPE명시물리입력범위밖')
            self.records += 1
            group = self.words(TYPES + self.type_index * 500 + 0x9c, 1)[0]
            theme = self.words(TYPES + self.type_index * 500 + 0x98, 1)[0]
            self.record(f'R:{self.type_index}:{group}:{theme}', 'type')
            self.mutate('record' + str(self.records))
            if not self.cause:
                self.ret(TYPES + self.type_index * 500)
        elif address == 0x4afa70:
            self.creates += 1
            base = self.words(0x5c8464, 1)[0]
            parent = (mu.reg_read(UC_X86_REG_ECX) - base) // 50
            pool_index = POOLS.index(base)
            self.record('C:' + ':'.join(map(str, (pool_index, parent, *args))), 'create')
            self.mutate('create' + str(self.creates))
            if not self.cause:
                self.ret(700 + self.creates, 20)
        elif address == 0x46e980:
            self.finish += 1
            self.record('F', 'finish')
            self.mutate('finish')
            if not self.cause:
                self.ret()
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError('Editor Player 검토범위밖: ' + hex(address))

    def sample(self, case, control):
        """독립raw입력만준비하고본문의모든필드·주변바이트·현재값·trace를읽는다."""
        self.case = case
        count, variant, language, flags, parent, mutation = case
        self.instructions = self.owner = self.parent_calls = self.creates = self.finish = self.count_reads = self.records = self.type_index = 0
        self.events = []
        self.cause = ''
        self.captured_player = 0
        self.put(0x59534c, PLAYER_BASES[0])
        self.put(0x540ca8, HANDLES[0])
        self.put(0x5c8464, POOLS[0])
        self.put(0x541348, count)
        # 모든Player와handle의미변경BYTE도서로다른입력으로유지한다.
        for index, base in enumerate(PLAYER_BASES):
            mu_bytes = bytes((0xa5 + offset * 13 + index * 7) & 255 for offset in range(PLAYER_BYTES))
            self.mu.mem_write(base, mu_bytes)
            for owner in range(9):
                self.put(base + owner * 180 + 0x50, flags ^ (owner << 20) ^ (index << 28))
        for index, base in enumerate(HANDLES):
            self.mu.mem_write(base, bytes((0xd0 + offset + index * 7) & 255 for offset in range(64)))
        # TYPE getter가반환한같은기록의현재group/theme를본문이읽는다.
        for number in range(32):
            group, theme = types(number, variant)
            self.put(TYPES + number * 500 + 0x9c, group)
            self.put(TYPES + number * 500 + 0x98, theme)
        # 매관측마다정수레지스터와반환스택을새로만든다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(0x4904b0, STOP)
        if self.cause:
            self.stops[self.cause] += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            self.normal_returns += 1
        state = (struct.unpack('<i', self.mu.mem_read(0x541348, 4))[0], PLAYER_BASES.index(self.words(0x59534c, 1)[0]), HANDLES.index(self.words(0x540ca8, 1)[0]), POOLS.index(self.words(0x5c8464, 1)[0]), self.parent_calls, self.creates, self.finish, self.count_reads)
        raw = ':'.join(bytes(self.mu.mem_read(base, PLAYER_BYTES)).hex() for base in PLAYER_BASES) + ':' + ':'.join(bytes(self.mu.mem_read(base, 64)).hex() for base in HANDLES)
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause or '1', ','.join(map(str, state)), raw, ';'.join(self.events)))


def main():
    """최초관측을제품작성전동결하고이후에는근거SHA검증만허용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 제품변경에맞춰최초원문을재생성하지않도록모든근거를확인한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Editor Player 최초 SHA불일치: ' + name)
        print('Editor Player 최초 x86근거보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Editor Player 최초fixture/보고서덮어쓰기금지')
    oracle = Oracle()
    inputs = cases()
    lines = ['# control / count,variant,language,flags,parentMode,mutation / completed-or-stop / current8fields / twoPlayerTables+twoHandles / original-trace']
    # 두x87제어어를독립실행하고정수부모의동일관측을확인한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_editor_players.py', 'cpppj/tests/fixtures/editor-players-x86.tsv')
    report = {'status': '신규 독립 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004904b0', 'range': '004904b0-004905c0', 'bodyBytes': 273, 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'normalReturns': oracle.normal_returns, 'explicitStops': dict(oracle.stops), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제273BYTE의Human/현재handle이름복사,owner2..8의고정레코드+2c/이름/+50 flags,타입0/현재signedcount재조회/group/theme필터/직접ContainedCreate/마지막tail후처리. 번역/00490250/서식/TYPE/004afa70/0046e980은명시외부경계이며하위실제생성/전체Editor/파일/OS/게임은실행하지않음. 본문에는config/title/Find/Ensure호출이없음.', 'gameExecuted': False, 'osExecuted': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'instructions')}))


if __name__ == '__main__':
    main()
