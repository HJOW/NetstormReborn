#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재35섹션의실제DataManager SaveImage와sendFort/길이observer를격리관찰한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP
from taff import TaffArchive

# 실제PE와분리한두현재manager/연결리스트/섹션payload/이름표/최종이미지주소다.
MANAGERS = (POOL, POOL + 0x400)
NODE_BASES = (POOL + 0x1000, POOL + 0x4000)
DATA_BASES = (TYPES, TYPES + 0x8000)
IMAGE, NAMES, RETURNED = TYPES + 0x10000, TEXT + 0x1000, POOL + 0x8000
MISSIONS = ('BridgeTheGap', 'SecretWorkshop', 'CaptureThePriest', 'TacticalCombat', 'SubtleDefense', 'RawPower', 'TheWarBegins', 'MasterOfWhirligigs')
SECTION_NAMES = 'Subscriber State Mission CoreData Chaff Badges Territory TypeNames CompressedData Technology Money Deck Reserved2 Reserved3 Reserved4'.split() + [f'Terr{index:02d}' for index in range(20)]
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-image-packets-x86.tsv'  # 전체이미지/예약/전송과선택경계를보존한다.
REPORT = ROOT / 'cpppj/fort-image-packets.json'  # 실제몸체와명시CRT/선택/전송의범위를구분한다.


def protected(profile):
    """VFS의disk우선/보호아카이브를읽고raw길이섹션만명시입력으로분리한다."""
    name = MISSIONS[profile - 100] + '.fort';path = ROOT / 'originals/d' / name
    if path.exists():data = path.read_bytes()
    else:
        archive = TaffArchive(str(ROOT / 'originals/netstorm.tarc'))
        entry = next(item for item in archive.entries if item.name.replace('\\', '/').rsplit('/', 1)[-1].lower() == name.lower());data = archive.read(entry)
    result = [];position = 2
    while len(result) < 35 and position + 2 <= len(data):
        length = int.from_bytes(data[position:position + 2], 'little')
        if length < 2 or length > 0x7fff or position + length > len(data):break
        result.append(data[position + 2:position + length]);position += length
    return data[1], result


def sections(profile, seed, count):
    """현재raw섹션입력만만든다.이미지prefix/정렬/기대출력은PE에서관찰한다."""
    if profile >= 100:
        flag, result = protected(profile);result = list(result)
        if seed & 1:
            while len(result) <= 12:result.append(b'')
            result[12] = bytes((0xa0, profile & 255, seed & 255, 0, 0xfe))
        return result
    result = []
    for index in range(count):
        size = 0 if profile == 0 else (index * 7 + seed) % 17 if profile == 1 else 32696 if profile == 2 and index == 34 else 32695 if profile == 3 and index == 34 else 0
        result.append(bytes((byte * 29 + index * 13 + seed) & 255 for byte in range(size)))
    return result


def cases():
    """고정순서/빈tail/현재수정/경계/전송필드/observer를중복없는입력으로수집한다."""
    inputs = set()
    # kind/profile/seed/개수/flag/types/역연결/owner/target/predictable/watcher/초기길이/present/변경코드다.
    def add(kind='I', profile=1, seed=0, count=35, flag=0, types=188, reverse=0, owner=1, target=2, predictable=0, watcher=0, initial=0, present=1, mutation=0):
        """결과를계산하지않고명시값만모은다."""
        inputs.add((kind, profile, seed, count, flag, types, reverse, owner, target & 0xffffffff, predictable, watcher, initial, present, mutation))
    for profile, seed, count, flag, reverse in itertools.product((0, 1), (0, 7), (0, 1, 15, 35), (0, 0xfe, 0x7f), (0, 1)):
        add(profile=profile, seed=seed, count=count, flag=flag, reverse=reverse)
    for profile, types in itertools.product((2, 3), (188, 256)):add(profile=profile, types=types)
    for profile, seed in itertools.product(range(100, 108), (0, 1)):
        flag, unused = protected(profile);add(profile=profile, seed=seed, flag=flag, reverse=seed & 1)
        add(kind='S', profile=profile, seed=seed, flag=flag, owner=(profile - 100) % 8 + 1, predictable=0xffffffff, watcher=0xff)
    for owner, target, predictable, watcher, initial in itertools.product((0, 1, 8), (-1, 9), (0, 0xffffffff), (0, 1, 0xff), (0, 0xffff)):
        add(kind='S', owner=owner, target=target, predictable=predictable, watcher=watcher, initial=initial)
    for mutation in (1, 2, 3):add(kind='S', mutation=mutation)
    for present in (0, 1):add(kind='S', profile=0, present=present)
    # 직접observer는false의무읽기와DWORDffffffff/WORD감김을실제PE로대조한다.
    for size, write in itertools.product((0, 1, 17, 0xffff, 0xffffffff), (0, 1)):
        add(kind='F', predictable=size, mutation=write)
    return sorted(inputs)


class FortOracle(MemoryOracle):
    """실제Save/이름표/메모리stream/packethelper를실행하고CRT/heap/현재선택·전송만명시외부다."""
    def __init__(self):
        """현재PC전체정밀범위의15몸체만허용하고원본쓰기/반환/SEH를감시한다."""
        super().__init__();self.mu.mem_map(0, 0x1000);self.allowed = [];self.calls = {};self.crt = {}
        selected = (0x4bf520, 0x44d640, 0x44cc20, 0x44cce0, 0x4bcd80, 0x4bccb0, 0x4437f0, 0x4a8850, 0x4a8800, 0x4a7240, 0x40a580, 0x482ba0, 0x4df590, 0x4e4a40, 0x40a540)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) in selected:
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.selected = selected;self.mu.hook_add(UC_HOOK_MEM_READ, self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def read(self, mu, access, address, size, value, data):
        """상위현재Template선택과현재typeCount/초기길이읽기를원문위치에서관찰한다."""
        if address == self.template and self.kind == 'S':self.events.append('C,' + str(int(self.words(address, 1)[0] != 0)))
        if address == 0x541348:self.events.append('G,' + str(self.words(address, 1)[0]))
        if address == 0x544c48:
            self.events.append('H,' + str(self.words(address, 1)[0] & 0xffff))
            if self.mutation == 3:self.put(PLAYERS + self.owner * 0xb4 + 0x58, 0x7f, 1)

    def guard(self, mu, access, address, size, value, data):
        """실제쓰기는SEH/한정stack/단일image/이름표/manager선택필드/반환stream만허용한다."""
        regions = ((0, 4), (STACK - 0x1f000, STACK + 16), (IMAGE, IMAGE + 0x8000), (NAMES, NAMES + 0x200), (RETURNED, RETURNED + 16), (0x5c8aa4, 0x5c8aa8))
        if any(low <= address and address + size <= high for low, high in regions):return
        if any(manager + low <= address and address + size <= manager + high for manager in MANAGERS for low, high in ((8, 12), (0x10c, 0x114))):return
        raise RuntimeError(f'Fort 원문쓰기범위밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """전체이미지/예약은실제명령출력이고CRT와전송만별도로기록한다."""
        if address in self.selected:self.calls[address] = self.calls.get(address, 0) + 1
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp + 4, 5)
        if address == 0x4e4391:
            self.crt[address] = self.crt.get(address, 0) + 1
            if args[0] not in (0x200, 0x8000):raise RuntimeError('Fort 미검토할당크기')
            self.ret(NAMES if args[0] == 0x200 else IMAGE);return
        if address == 0x4a89c0:
            self.crt[address] = self.crt.get(address, 0) + 1
            if self.string(args[1]) != b'Terr%02d' or not 0 <= args[2] < 20:raise RuntimeError('Fort 미검토이름서식')
            mu.mem_write(args[0], (f'Terr{args[2]:02d}').encode() + b'\0');self.ret(args[0]);return
        if address == 0x4e65ed:
            self.crt[address] = self.crt.get(address, 0) + 1;a, b = self.string(args[0]).lower(), self.string(args[1]).lower();self.ret(int(a > b) - int(a < b));return
        if address == 0x4e0620:
            line = args[2]
            if line == 0xda:self.events.append('A,types')
            elif line == 0xbe:self.events.append('A,section')
            elif line == 0x24:self.events.append('A,stream')
            elif line == 0x672:self.events.append('A,packet')
            elif line == 0x57d:
                self.events.append('A,template')
                if self.mutation == 2:self.put(self.template, MANAGERS[1])
            else:raise RuntimeError(f'Fort 미검토진단 {line:x}')
            self.ret();return
        if address == 0x4a68e0:
            if self.string(args[0]) != b'sendFort from %d to %d\n':raise RuntimeError('Fort 로그서식오류')
            self.events.append('L')
            if self.mutation in (1, 2):self.put(self.template, MANAGERS[1] if self.mutation == 1 else 0)
            self.ret();return
        if address == 0x4df590:self.packet = args[4];self.reserve = args[3]
        if address == 0x4e2dc0:
            self.events.append('S,' + str(args[0]));self.packet = args[1];self.before = bytes(mu.mem_read(self.packet, self.reserve))
            # 실제전송경계에진입한뒤원본길이observer를실행하고같은caller로복귀한다.
            self.put(esp - 8, STOP + 0x200);self.put(esp - 4, 1);mu.reg_write(UC_X86_REG_ECX, self.packet);mu.reg_write(UC_X86_REG_ESP, esp - 8);mu.reg_write(UC_X86_REG_EIP, 0x40a540);return
        if address == STOP + 0x200:
            self.after = bytes(mu.mem_read(self.packet, self.reserve));length = self.words(self.packet + 2, 1)[0] & 0xffff;self.wire = bytes(mu.mem_read(self.packet, length));self.ret();return
        if address == 0x4e4354:
            if args[0] != IMAGE:raise RuntimeError('Fort 반납주소오류')
            self.ret();return
        if not any(low <= address and address + size <= high for low, high in self.allowed):raise RuntimeError(f'Fort 미검토명령 {address:08x}')

    def manager(self, identifier, profile, seed, count, flag, reverse):
        """현재linkedstream원문만준비한다.이미지의헤더/길이/복사는계산하지않는다."""
        manager, base, cursor = MANAGERS[identifier], NODE_BASES[identifier], DATA_BASES[identifier]
        self.mu.mem_write(manager, bytes(0x114));self.put(manager, 0x514690);self.put(manager + 0x110, int(flag == 0xfe))
        payloads = sections(profile, seed, count);order = list(range(35));order = list(reversed(order)) if reverse else order
        self.put(manager + 4, base + order[0] * 64)
        for ordinal, index in enumerate(order):
            node = base + index * 64;body = payloads[index] if index < len(payloads) else b'';self.mu.mem_write(node, bytes(64));self.put(node, cursor);self.put(node + 8, len(body));self.put(node + 12, len(body))
            name = SECTION_NAMES[index].swapcase() if reverse else SECTION_NAMES[index];self.mu.mem_write(node + 16, name.encode() + b'\0');self.put(node + 56, 0 if ordinal == 34 else base + order[ordinal + 1] * 64)
            if body:self.mu.mem_write(cursor, body)
            cursor += len(body)

    def sample(self, case):
        """실제정상반환/cdecl·thiscall스택/보존4레지스터/SEH와전체현재출력을읽는다."""
        self.kind, profile, seed, count, flag, types, reverse, self.owner, target, predictable, watcher, initial, present, self.mutation = case
        self.events = [];self.packet = 0;self.reserve = 0;self.before = self.after = self.wire = b'';self.template = 0x5c89f0 + self.owner * 20
        self.manager(0, profile, seed, count, flag, reverse);self.manager(1, profile, seed + 3, count, flag ^ 0xfe, 1 - reverse)
        self.mu.mem_write(PLAYERS, bytes(9 * 0xb4));self.put(PLAYERS + self.owner * 0xb4 + 0x58, watcher, 1);self.put(0x59534c, PLAYERS);self.put(self.template, MANAGERS[0] if present else 0)
        self.put(0x541348, types);self.put(0x5c8aa4, 0);self.put(0x544c48, initial, 2);self.put(0, 0x12345678);self.mu.mem_write(IMAGE, bytes([0xa5]) * 0x8000);self.mu.mem_write(RETURNED, bytes(16))
        preserved = {UC_X86_REG_EBX:0x11112222, UC_X86_REG_ESI:0x33334444, UC_X86_REG_EDI:0x55556666, UC_X86_REG_EBP:0x77778888}
        for register, value in preserved.items():self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_EDX, 0xaaaabbbb);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        if self.kind == 'I':entry = 0x4bf520;args = (RETURNED,);self.mu.reg_write(UC_X86_REG_ECX, self.template);purge = 4
        elif self.kind == 'F':
            entry = 0x40a540;args = (self.mutation,);self.mu.mem_write(RETURNED, bytes([0xa5]) * 20);self.put(RETURNED + 12, predictable);self.mu.reg_write(UC_X86_REG_ECX, RETURNED);purge = 4
        else:entry = 0x4a7240;args = (self.owner, target, predictable);purge = 0
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args));self.mu.emu_start(entry, STOP, count=1000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge or self.words(0, 1)[0] != 0x12345678:raise RuntimeError('Fort 정상반환/stack/SEH오류')
        for register, value in preserved.items():
            if self.mu.reg_read(register) != value:raise RuntimeError('Fort 보존레지스터오류')
        if self.kind == 'F':return '-', bytes(self.mu.mem_read(RETURNED, 20)).hex(), '-', '-', ';'.join(self.events) or '-'
        image = b''
        if self.kind == 'I' or self.packet:
            length = self.words(RETURNED + 8, 1)[0] if self.kind == 'I' else int.from_bytes(self.after[12:16], 'little')
            allocation = bytes(self.mu.mem_read(IMAGE, 0x8000));image = allocation[:length]
            if any(allocation[length:]):raise RuntimeError('Fort 실제할당의0tail불일치')
            if self.kind == 'I' and self.words(RETURNED, 4) != (IMAGE, 0, length, 0x8000):raise RuntimeError('Fort 반환stream계약오류')
        return image.hex() or '-', self.before.hex() or '-', self.after.hex() or '-', self.wire.hex() or '-', ';'.join(self.events) or '-'


def main():
    """현재독립원문출력/명시외부범위와SHA를보존한다.제품빌드나게임은실행하지않는다."""
    if sys.argv[1:] == ['--verify']:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in facts['sha256'].items():
            if digest(ROOT / name) != expected:raise RuntimeError('Fort 이미지/패킷 SHA 불일치: ' + name)
        rows = list(csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t'))[1:]
        if len(rows) != facts['observations'] or len({tuple(row[:2]) for row in rows}) != len(rows):raise RuntimeError('Fort 이미지 고유입력 수 오류')
        print('Fort image/packets native SHA verified:', len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_fort_image_packets.py [--verify]')
    oracle = FortOracle();rows = []
    for case in cases():rows.append((case[0], ','.join(map(str, case[1:])), *oracle.sample(case)))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('kind', 'input', 'image', 'before', 'after', 'wire', 'trace'));writer.writerows(rows)
    paths = ('originals/Netstorm.exe', 'originals/netstorm.tarc', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/taff.py', 'tools/decomp_fort_image_packets.py', 'cpppj/src/o/Template.h', 'cpppj/src/o/Template.cpp', 'cpppj/src/o/RawFortImage.h', 'cpppj/src/o/RawFortImage.cpp', 'cpppj/src/o/RawMissionLoginPackets.h', 'cpppj/src/o/RawMissionLoginPackets.cpp', 'cpppj/tests/FortImagePacketsTests.cpp', 'cpppj/tests/fixtures/fort-image-packets-x86.tsv', 'docs/exe/cpp-fort-image-packets-20261015.md')
    facts = {'host':'HJOW-Athlon', 'date':'2026-10-15', 'status':'native-observed-product-unbuilt', 'observations':len(rows), 'kinds':{kind:sum(row[0] == kind for row in rows) for kind in 'ISF'}, 'native_calls':{f'{address:08x}':count for address, count in oracle.calls.items()}, 'explicit_crt_substitutions':{f'{address:08x}':count for address, count in oracle.crt.items()}, 'normal_return_stack_callee_saved_seh':True, 'native_write_guard':True, 'whole_image_zero_tail_checked':True, 'selection_and_send':'explicit external boundaries; packet finalizer executes actual PE', 'game_gui_os_calls':0, 'sha256':{name:digest(ROOT / name) for name in paths if (ROOT / name).exists()}}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2)+'\n', encoding='utf-8');print('Fort image/packets native observations:', len(rows))


if __name__ == '__main__':main()
