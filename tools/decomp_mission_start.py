#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 시작/AI설정/실제생성wrapper의 기계어를 격리 관찰한다. 원본 게임은 실행하지 않는다."""
import csv
import json
import re
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 설정/미션/brain 관찰 공간은 실제 PE와 분리한다.
SETTINGS, BRAIN, VTABLE = TYPES, TYPES + 0x10000, POOL + 0x10000
# 원본 능력 표의 실제 순서는 PE에서 읽으며 기대 설정값을 계산하지 않는다.
ABILITIES = ['FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS', 'BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED', 'ATTACH_TO_ENEMY_ISLAND', 'STOP_COLLECTING_FROM_DANGEROUS_GEYSERS', 'COLLECT_STORM_POWER', 'ENCIRCLE_ENEMY_ISLAND', 'CONCENTRATE_FIRE', 'REPEAT_SUCCESSFUL_DROPS', 'CAPTURE_ENEMY_PRIEST', 'BUILD_DAIS', 'SACRIFICE_ENEMY_PRIEST', 'USE_PRIEST_TO_COLLECT', 'BUILD_BRIDGES']


def signed(value):
    """DWORD의 실제 부호 해석이다."""
    return struct.unpack('<i', struct.pack('<I', value & 0xffffffff))[0]


def atol(text):
    """외부 CRT atol 경계의 입력 변환이며 원본 game 분기를 대신 계산하지 않는다."""
    match = re.match(r'\s*([+-]?\d+)', text)
    return int(match[1]) & 0xffffffff if match else 0


def item(value, index):
    """외부 ConfigListItem의 실제 세미콜론 분리다."""
    parts = value.split(';')
    return parts[index] if 0 <= index < len(parts) else ''


def pack_input(numbers, config):
    """명시 입력 숫자와 문자열 바이트를 TSV 한 셀에 보존한다."""
    return ','.join(map(str, numbers)) + '|' + '|'.join(key + '=' + value.encode().hex() for key, value in sorted(config.items()))


class Oracle(MemoryOracle):
    """원본 게임 계산은 실제 명령으로 실행하고 명시 외부 몸체만 관찰한다."""
    def __init__(self):
        """정밀 선택 함수의 실제 범위와 원본 ftol/AttachBrain 범위만 허용한다."""
        super().__init__();self.allowed = []
        paths = [('extracted/mission-start/originals/functions.tsv', None), ('extracted/refined/originals/functions.tsv', {0x4e49c0, 0x4911a0, 0x48f550})]
        for path, selected in paths:
            with (ROOT / path).open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    if selected is not None and int(row['entry'], 16) not in selected:
                        continue
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.mem_map(0, 0x1000)
        self.mu.mem_write(STOP + 0x100, b'\xdd\x05' + struct.pack('<I', TEXT + 0xff00) + b'\xc3')
        self.allowed.append((STOP + 0x100, STOP + 0x107))

    def text(self, value):
        """문자열 getter의 독립 결과를 새 가상 storage에 둔다."""
        data = value.encode() + b'\0';result = self.text_cursor;self.mu.mem_write(result, data);self.text_cursor += len(data) + 1;return result

    def instruction(self, mu, address, size, data):
        """현재 설정/세계/CRT/UI 요청을 기록하며 검토하지 않은 코드 진입은 거부한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);ecx = mu.reg_read(UC_X86_REG_ECX);args = self.words(esp + 4, 8)
        if address in (0x482c90, 0x482ce0, 0x482d50):
            key = self.string(args[0]).decode();self.events.append('G,' + key);value = self.config.get(key, '')
            if address == 0x482d50:
                value = item(value, signed(args[1]));purge = 8
            elif address == 0x482ce0:
                value = value or self.string(args[1]).decode();purge = 8
            else:
                purge = 4
            self.ret(self.text(value), purge)
        elif address == 0x4b9390:
            pattern = self.string(args[1]).decode();value = self.string(args[2]).decode() if '%s' in pattern else signed(args[2]);self.ret(self.text(pattern % value))
        elif address == 0x4e66de:
            self.ret(atol(self.string(args[0]).decode()))
        elif address == 0x4e4359:
            value = self.string(args[0]).decode();self.events.append('F,' + value.encode().hex());mu.mem_write(TEXT + 0xff00, struct.pack('<d', float(value)));mu.reg_write(UC_X86_REG_EIP, STOP + 0x100)
        elif address == 0x4a8890:
            value = self.string(args[1]).decode().lower();colors = ['none', 'blue', 'red', 'white', 'green', 'purple', 'yellow', 'lightblue', 'orange'];self.ret(colors.index(value) if value in colors else -1)
        elif address == 0x4e65ed:
            self.ret(int(self.string(args[0]).lower() > self.string(args[1]).lower()) - int(self.string(args[0]).lower() < self.string(args[1]).lower()))
        elif address == 0x4e0620:
            self.events.append('A');self.ret()
        elif address == 0x4c2630:
            self.events.append('L');self.ret()
        elif address == 0x440640:
            prefix = self.string(args[0]).decode();self.events.append('N,' + prefix);mu.mem_write(args[1], self.sections.encode() + b'\0');self.ret(purge=8)
        elif address == 0x4404e0:
            key, value = self.string(args[0]).decode(), self.string(args[1]).decode();self.events.append('W,' + key + ',' + value.encode().hex());self.config[key] = value;self.ret(purge=8)
        elif address == 0x40edb0:
            self.events.append('S,' + str(args[0]) + ',' + str(args[1]));self.ret()
        elif address == 0x43dad0:
            self.events.append('R');self.ret()
        elif address == 0x4903b0:
            self.events.append('C,' + str(args[0]));self.ret()
        elif address == 0x4915c0:
            self.events.append('T,' + str(args[0]) + ',' + self.string(args[1]).hex());self.ret()
        elif address == 0x4921f0:
            self.events.append('P,' + str(args[0]));self.ret((self.priests >> args[0]) & 1)
        elif address == 0x4c2d40 and self.kind == 'start':
            self.events.append('I,' + str(args[0]));self.ret(purge=4)
        elif address == 0x417e50 and self.kind == 'prepare':
            self.prepared = bytes(mu.mem_read(args[1], 600));self.ret()
        elif address == 0x42e440:
            self.events.append('Q');self.ret()
        elif address == 0x486440:
            self.events.append('Y');self.ret()
        elif address == 0x4340d0:
            self.events.append('Z');self.ret()
        elif address == 0x43ca70:
            self.events.append('O,' + str(args[0]) + ',' + str(args[1]));self.ret()
        elif address == 0x46f260:
            self.events.append('V,' + str(args[0]));self.ret(self.temple, 4)
        elif address == STOP + 0x200:
            self.events.append('D,' + str(args[0]));self.ret(purge=4)
        elif address == 0x4e4391:
            self.events.append('B,' + str(args[0]));self.ret(BRAIN)
        elif address == 0x416860:
            self.events.append('K,' + str(args[0]));self.ret(ecx, 8)
        elif address == 0x414710:
            self.events.append('U,' + str(ecx));self.ret(purge=4)
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'검토하지 않은 명령 진입 {address:08x}')
        self.instructions += 1
        if self.instructions > 150000:
            raise RuntimeError('원본 명령 상한 초과')

    def run(self, kind, numbers, config):
        """명시 입력을 가상 storage에만 쓰고 해당 함수의 원본 결과를 반환한다."""
        self.kind = kind;self.events = [];self.config = dict(config);self.instructions = 0;self.text_cursor = TEXT;self.sections = config.get('@sections', '')
        self.mu.mem_write(POOL, b'\xa5' * 0xd0);self.mu.mem_write(SETTINGS, bytes((index * 37 + numbers[0]) & 255 for index in range(600)));self.mu.mem_write(PLAYERS, b'\0' * 1620);self.put(0x59534c, PLAYERS)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<IIII', STOP, 0, 0, 0))
        result = '-'
        if kind in ('init', 'apply', 'prepare', 'ability'):
            fill, owner, refresh, money, checking = numbers[:5];self.put(0x5318d4, refresh);self.put(0x52f3c8, money);self.put(0x5e4794, checking, 1)
            if kind == 'init':entry = 0x40ef80;ecx = SETTINGS
            elif kind == 'ability':
                entry = 0x40f030;ecx = SETTINGS;self.mu.mem_write(STACK + 4, struct.pack('<II', self.text(config['name']), numbers[5]))
            elif kind == 'apply':
                entry = 0x4c2d90;ecx = POOL;self.mu.mem_write(STACK + 4, struct.pack('<II', SETTINGS, owner & 0xffffffff))
            else:
                entry = 0x4c2d40;ecx = POOL;self.mu.mem_write(STACK + 4, struct.pack('<I', owner & 0xffffffff))
            self.mu.reg_write(UC_X86_REG_ECX, ecx)
            # 실제 wrapper의 스택 local600 입력을 같은 caller 바이트 패턴으로 준비한다.
            if kind == 'prepare':
                self.mu.mem_write(STACK - 0x258, bytes(self.mu.mem_read(SETTINGS, 600)));self.prepared = None;self.mu.emu_start(entry, STOP)
            else:self.mu.emu_start(entry, STOP)
            raw = (self.prepared if kind == 'prepare' else bytes(self.mu.mem_read(SETTINGS, 600))).hex()
        elif kind == 'start':
            local, known, editor, demo, self.priests, initial, index = numbers
            for offset, value in [(0x54, 0xffffffff), (0x58, initial), (0x68, demo), (0x88, 0), (0xa0, index)]:self.put(POOL + offset, value)
            for offset in (0x70, 0x90, 0xa8):self.mu.mem_write(POOL + offset, struct.pack('<d', 77.25))
            for address, value in [(0x540c70, local), (0x595344, known), (0x5c85a4, editor)]:self.put(address, value)
            self.mu.mem_write(0x55b4d0, struct.pack('<d', 16777217.25));self.mu.reg_write(UC_X86_REG_ECX, POOL);self.mu.emu_start(0x4c2b20, STOP)
            values = [self.words(POOL + offset, 1)[0] for offset in (0x54, 0x58, 0xa0, 0xc0, 0xc4, 0x88)] + [self.words(address, 1)[0] for address in (0x595078, 0x59507c, 0x5453e4, 0x5950ac)]
            raw = ','.join(map(str, values)) + '|' + '|'.join(bytes(self.mu.mem_read(POOL + offset, 8)).hex() for offset in (0x70, 0x90, 0xa8))
        elif kind == 'create':
            owner, old, no_temple, color, self.temple, flags, subscriber = numbers;self.mu.mem_write(SETTINGS, b'\0' * 600);self.put(SETTINGS + 0x244, no_temple);self.put(SETTINGS + 0x250, color)
            for offset, value in [(0xb0, old), (0x78, 127), (0x50, flags), (0x2c, subscriber)]:self.put(PLAYERS + owner * 180 + offset, value)
            self.put(0x5c8464, POOL);self.put(POOL + self.temple * 50, VTABLE);self.put(VTABLE + 0x10, STOP + 0x200);self.mu.mem_write(STACK + 4, struct.pack('<II', owner, SETTINGS));self.mu.emu_start(0x417e50, STOP)
            raw = bytes(self.mu.mem_read(PLAYERS + owner * 180, 180)).hex();result = str(self.mu.reg_read(UC_X86_REG_EAX))
        else:raise ValueError(kind)
        return [kind, pack_input(numbers, config), raw, result, ';'.join(self.events) or '-']


def cases():
    """정상/경계의 고유 입력을 만들며 기대 분기를 합성하지 않는다."""
    for fill in (0, 1, 127, 255):
        for refresh, money in [(3, 2500), (0, 0), (-1, -4001), (2147483647, 2147483647), (-2147483648, -2147483648)]:yield 'init', [fill, 1, refresh, money, 0], {}
    for owner in (-1, 0, 1, 8):
        prefix = 'ai' + (str(owner) if owner > 0 else '')
        for suffix, value in [('Name', 'N' * 29), ('Tech', 'sunCannon;windWalker'), ('BridgeDrawRate', '-0.125'), ('StuffRefreshRate', '16777217'), ('TimeBetweenMoves', '1.17549435e-38'), ('StartMoney', '-2147483649tail'), ('GeyserAttachments', '33'), ('MaxInHand', '-5'), ('Collectors', '0'), ('Enemy', '8'), ('NoTemple', '2'), ('Off', '-1'), ('Color', 'LIGHTBLUE'), ('AllyList', '0;1;8;9;31;32;33;-1;2147483647;3;7'), ('Ability', '!BUILD_BRIDGES;!missing;COLLECT_STORM_POWER')]:yield 'apply', [19, owner, 3, 2500, 1], {prefix + suffix: value}
        yield 'apply', [93, owner, 3, 2500, 0], {}
    for index, name in enumerate(ABILITIES + ['missing', '', '!BUILD_BRIDGES']):
        for enabled in (0, 1):yield 'ability', [index, 1, 3, 2500, 1, enabled], {'name': name.lower()}
    for owner in (1, 8):
        for color in ('', 'none', 'blue', 'missing'):
            config = {'aiName': 'Common', 'aiTech': 'sunWalker', 'aiStartMoney': '0', 'aiAllyList': '1;32;33', 'aiAbility': '!BUILD_BRIDGES', 'aiColor': 'red', f'ai{owner}Name': '', f'ai{owner}AllyList': '8;31', f'ai{owner}Color': color, f'ai{owner}Ability': 'BUILD_BRIDGES;!COLLECT_STORM_POWER'}
            yield 'prepare', [73, owner, 3, 2500, 1], config
    for local in (1, 8):
        for known in (0, 1):
            for editor in (0, 1):
                for demo in (0, 1):
                    for index in (0, 1, 3):
                        yield 'start', [local, known, editor, demo, 0x122, 0xffffffff, index], {'denySalvage': '-1', 'denyAscend': '0', 'allowAnyCapture': '2', 'moreGeysers': str(demo), 'demoRestartTimer': str(0 if known == 0 else -2), '@sections': '@5;@-2;@30;', 'myStartMoney': '16777217', 'myTech': 'sunCannon;windWalker', 'randGeysers': str(editor)}
    for owner in (1, 8):
        for old in (0, 9001):
            for no_temple in (0, 1):
                for color in (0, 3):
                    for temple in (0, 7):yield 'create', [owner, old, no_temple, color, temple, 0x100, -1], {}


def main():
    """고유 입력/원본 결과와 모든 근거 SHA를 저장하거나 원형 재생성 없이 감사한다."""
    fixture = ROOT / 'cpppj/tests/fixtures/mission-start-x86.tsv';report = ROOT / 'cpppj/mission-start.json'
    sources = ['originals/Netstorm.exe', 'extracted/mission-start/originals/creation.c', 'extracted/mission-start/originals/functions.tsv', 'tools/ghidra/mission-start-functions.json', 'tools/decomp_mission_start.py', 'tools/decomp_priestmenu.py', 'cpppj/src/o/RawMissionAiSettings.h', 'cpppj/src/o/RawMissionAiSettings.cpp', 'cpppj/src/o/RawMissionStart.h', 'cpppj/src/o/RawMissionStart.cpp', 'cpppj/src/o/RawAiCreate.h', 'cpppj/src/o/RawAiCreate.cpp', 'cpppj/tests/MissionStartTests.cpp', 'cpppj/tests/fixtures/mission-start-x86.tsv']
    if '--verify' in sys.argv:
        prior = json.loads(report.read_text(encoding='utf-8'))
        for name, value in prior['sha256'].items():assert digest(ROOT / name) == value, name
        with fixture.open(encoding='utf-8') as stream:rows = list(csv.DictReader(stream, delimiter='\t'))
        assert len(rows) == prior['observations'];assert len({(row['kind'], row['input']) for row in rows}) == len(rows)
        print('MissionStart 원본 입력/근거 SHA 감사 통과:', len(rows));return
    oracle = Oracle();rows = [oracle.run(*case) for case in cases()]
    assert len({(row[0], row[1]) for row in rows}) == len(rows)
    with fixture.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(['kind', 'input', 'raw', 'result', 'trace']);writer.writerows(rows)
    facts = {'host': 'VM-W11-Codex', 'date': '2026-10-15', 'observations': len(rows), 'kinds': {kind: sum(row[0] == kind for row in rows) for kind in sorted({row[0] for row in rows})}, 'boundaries': ['Config/CRT getter/parse results', 'SP/technology/Gump/geyser/camera effects', 'real AI allocation/constructor/initial-update requests; no actual brain strategy'], 'sha256': {name: digest(ROOT / name) for name in sources}}
    report.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8');print('MissionStart 실제 원본 고유 입력:', len(rows))


if __name__ == '__main__':main()
