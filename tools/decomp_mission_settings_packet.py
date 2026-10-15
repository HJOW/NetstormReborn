#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 설정 송신·수신·고정헤더·자연 observer의 현재 읽기/쓰기와 전체bytes를 격리 관찰한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# payload의 실제 순서이며 현재 전역은 서로 다른 두 저장영역에 놓인다.
GLOBALS = (0x5326cc, 0x55a488, 0x55a48c, 0x55a490, 0x55a494, 0x5326d0, 0x5326d4)
PACKET = TEXT + 0x2000  # 수신 원문은 전역/실제 stack과 분리된32BYTE 입력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/mission-settings-packet-x86.tsv'  # 원본 전체 packet/설정/순서를 저장한다.
REPORT = ROOT / 'cpppj/mission-settings-packet.json'  # 실제 PE 범위와 현재효과 경계를 구분한다.


def values(profile, seed):
    """0/ffffffff/경계패턴/seed/각 필드onehot의 명시 현재 입력만 만든다."""
    if profile == 0:return [0] * 7
    if profile == 1:return [0xffffffff] * 7
    if profile == 2:return [0, 1, 0x80000000, 0x7fffffff, 0xffffffff, 3, 16]
    if profile == 3:return [(0x12345678 + index * 0x1010101 + seed) & 0xffffffff for index in range(7)]
    result = [0] * 7;result[(profile - 4) // 3] = (1, 0x80000000, 0xffffffff)[(profile - 4) % 3];return result


def mutate(words, field, step, seed):
    """관찰된 현재값 이외만 변경해 먼저 읽은 값의 보관과 후속 재조회를 구분한다."""
    # host입력변경은 기대packet을 작성하지 않으며 후속 PE 읽기의 현재 원문만 바꾼다.
    for index in range(7):
        if index != field:words[index] ^= (0x9e3779b9 + seed + step * 17 + index) & 0xffffffff


def cases():
    """작은독립격자와7DWORD bit경계를 송신/수신 각각 중복없이 공급한다."""
    # 일반4패턴은현재변경없음/최초변경/매번변경을 모두검사한다.
    for profile, seed, target, mode in itertools.product(range(4), (0, 7), (0, 1, 9, 0xffffffff), (0, 1, 2)):
        yield ('S', profile, seed, target, mode)
    # 각필드의고유bit입력은형식또는bool정규화를감지한다.
    for profile in range(4, 25):yield ('S', profile, 0, 9, 0)
    # 수신은사용하지않는sender를중복격자로늘리지않고같은현재변경3종을공급한다.
    for profile, seed, mode in itertools.product(range(4), (0, 7), (0, 1, 2)):yield ('R', profile, seed, 9, mode)
    for profile in range(4, 25):yield ('R', profile, 0, 9, 0)
    for mode in (0, 1):yield ('O', 3, 7, 9, mode)


class SettingsOracle(MemoryOracle):
    """실제5몸체만 실행하며 외부현재변경/송신과 실제global쓰기만 관찰한다."""
    def __init__(self):
        """현재PC 정밀함수범위와 독립 원본read/write guard를 설치한다."""
        super().__init__();self.allowed = [];self.calls = {};self.entries = (0x44dbc0, 0x44dc20, 0x40a700, 0x4df230, 0x40b380)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            # 현재5몸체의정밀range밖명령은실행하지않는다.
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) in self.entries:
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def change(self, field, packet=False):
        """원본현재효과경계가나머지저장소/packet을바꾸는명시입력이다."""
        self.step += 1
        if self.mode == 2 or self.mode == 1 and self.step == 1:
            current = list(self.words(PACKET + 4, 7)) if packet else [self.words(address, 1)[0] for address in GLOBALS];mutate(current, field, self.step, self.seed)
            # hook의현재입력변경은 실제제품출력/기대값생성과구분한다.
            for index, value in enumerate(current):self.put(PACKET + 4 + index * 4 if packet else GLOBALS[index], value)

    def read(self, mu, access, address, size, value, data):
        """송신의각실제전역load위치에서값을기록하고나머지현재필드를변경한다."""
        if self.kind == 'S' and address in GLOBALS:
            if size != 4:raise RuntimeError('설정송신DWORD폭오류')
            field = GLOBALS.index(address);self.events.append(f'R,{field},{self.words(address, 1)[0]}');self.change(field)

    def guard(self, mu, access, address, size, value, data):
        """한정stack과수신의실제7DWORD쓰기이외는모두거부한다."""
        if STACK - 4096 <= address and address + size <= STACK + 20:return
        if self.kind == 'R' and address in GLOBALS and size == 4:
            field = GLOBALS.index(address);self.events.append(f'W,{field},{value & 0xffffffff}');self.change(field, True);return
        raise RuntimeError(f'설정packet원문쓰기범위밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """설정/헤더/observer는모두실제기계어이며송신만명시실제연결gate다."""
        if address in self.entries:self.calls[address] = self.calls.get(address, 0) + 1
        if address == 0x4e2dc0:
            target, packet = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 2);self.events.append(f'S,{target}');self.output = bytes(mu.mem_read(packet, 32));self.ret();return
        if not any(low <= address and address + size <= high for low, high in self.allowed):raise RuntimeError(f'설정packet미검토명령 {address:08x}')

    def sample(self, case):
        """독립PE정상반환/스택/보존4레지스터와전체현재출력을확인한다."""
        self.kind, profile, self.seed, target, self.mode = case;self.events = [];self.step = 0;self.output = b''
        current = values(profile, self.seed);self.mu.mem_write(PACKET, bytes((index * 37 + 11) & 255 for index in range(32)))
        # 현재설정과수신입력은독립명시원문이며패킷기대출력은계산하지않는다.
        for index, address in enumerate(GLOBALS):self.put(address, current[index] if self.kind != 'R' else 0xcccc0000 + index)
        if self.kind != 'S':self.mu.mem_write(PACKET + 4, struct.pack('<7I', *current))
        preserved = {UC_X86_REG_EBX:0x11112222, UC_X86_REG_ESI:0x33334444, UC_X86_REG_EDI:0x55556666, UC_X86_REG_EBP:0x77778888}
        for register, value in preserved.items():self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_EDX, 0xaaaabbbb);self.mu.reg_write(UC_X86_REG_ECX, PACKET);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        if self.kind == 'S':entry, args, purge = 0x44dbc0, (target,), 0
        elif self.kind == 'R':entry, args, purge = 0x44dc20, (target, 0x12345678, PACKET), 0
        else:entry, args, purge = 0x40b380, (self.mode,), 4
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args));self.mu.emu_start(entry, STOP, count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:raise RuntimeError('설정packet정상반환/스택오류')
        for register, value in preserved.items():
            if self.mu.reg_read(register) != value:raise RuntimeError('설정packet보존레지스터오류')
        packet = self.output if self.kind == 'S' else bytes(self.mu.mem_read(PACKET, 32));settings = b''.join(bytes(self.mu.mem_read(address, 4)) for address in GLOBALS)
        return packet.hex(), settings.hex(), ';'.join(self.events) or '-'


def main():
    """원본전체결과와SHA만저장하며C++빌드/게임/OS는실행하지않는다."""
    if sys.argv[1:] == ['--verify']:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:raise RuntimeError('설정packet SHA불일치: ' + name)
        rows = list(csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t'))[1:]
        if len(rows) != report['observations'] or len({row[0] for row in rows}) != len(rows):raise RuntimeError('설정packet고유입력수오류')
        print('Mission settings packet SHA verified:', len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_mission_settings_packet.py [--verify]')
    oracle = SettingsOracle();rows = []
    for case in cases():rows.append((','.join(map(str, case)), *oracle.sample(case)))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('input', 'packet', 'settings', 'trace'));writer.writerows(rows)
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_mission_settings_packet.py', 'cpppj/src/o/RawPacketDispatch.h', 'cpppj/src/o/RawPacketDispatch.cpp', 'cpppj/src/o/RawPacketLengths.h', 'cpppj/src/o/RawMissionSettingsPacket.h', 'cpppj/src/o/RawMissionSettingsPacket.cpp', 'cpppj/tests/MissionSettingsPacketTests.cpp', 'cpppj/tests/fixtures/mission-settings-packet-x86.tsv', 'docs/exe/cpp-mission-settings-packet-20261015.md')
    report = {'host':'HJOW-Athlon', 'last_full_refined_host':'HJOW-Athlon', 'date':'2026-10-15', 'status':'native-observed-product-unbuilt', 'observations':len(rows), 'kinds':{kind:sum(row[0].startswith(kind+',') for row in rows) for kind in 'SRO'}, 'native_calls':{f'{address:08x}':count for address, count in oracle.calls.items()}, 'normal_return_stack_callee_saved':True, 'native_write_guard':True, 'send_read_order':[1,2,0,4,5,3,6], 'receive_write_order':list(range(7)), 'explicit_boundaries':['current DWORD getter/store effects', 'actual connection-gate send'], 'crt_replacements':0, 'game_gui_os_calls':0, 'sha256':{name:digest(ROOT / name) for name in paths if (ROOT / name).exists()}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8');print('Mission settings packet native observations:', len(rows))


if __name__ == '__main__':main()
