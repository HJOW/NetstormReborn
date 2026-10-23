#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Territory회복167BYTE/동일manager writer93BYTE/일반쓰기96BYTE를제품전에고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, OWNER = 0x5c89f0, 0x540cac  # 현재9×20BYTE헤더와매하위뒤재조회하는owner다.
MANAGERS = {0: 0, 11: POOL + 0x1000, 22: POOL + 0x2000}  # manager+8의현재stream을별개주소로소유한다.
STREAMS = (POOL + 0x3000, POOL + 0x3020, POOL + 0x3040)  # Create반환과현재stream이달라도구별한다.
BUFFERS = (POOL + 0x4000, POOL + 0x4200, POOL + 0x4400)  # 실제256BYTE출력세개다.
SOURCES, INPUT, STATES = POOL + 0x5000, POOL + 0x6000, POOL + 0x7000  #9×120입력/직접writer입력120/9×28State다.
LOCAL = STACK - 120  # 회복부모의zero120지역버퍼실제주소다.
MESSAGE = b'Territory recovered from Battle!\n'  # 원문29번로그의고정문자열이다.
RANGES = ((0x4b5ec0, 0x4b5f0d), (0x4b5f10, 0x4b5f6a), (0x4bd600, 0x4bd65d), (0x44cce0, 0x44cd40))  # 불연속부모167/writer93/Write96만실행한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-territory-recover-x86.tsv'  # 제품전에최초확정하고덮어쓰지않는다.
REPORT = ROOT / 'cpppj/fort-territory-recover-native.json'  # 근거SHA와실제/명시하위범위를구별한다.


def records(slot, pattern):
    """각6BYTE입력의flags만지정하고나머지5BYTE는독립입력패턴으로둔다."""
    data = bytearray((byte * 13 + 7 + slot * 11) & 255 for byte in range(120))
    # 모든다른flags비트를보존한채bit4조합만공급한다.
    for index in range(20):
        at = index * 6 + 1
        data[at] &= 0xfb
        if pattern == 4:
            data[at] = 255
        elif pattern == 1 or pattern == 2 and index % 2 == 0 or pattern == 3 and index == 19:
            data[at] |= 4
    return bytes(data)


def cases():
    """실제원문이flags/진단/쓰기순서를결정하도록초기입력만교차한다."""
    rows = []
    # 회복은변경된기록이없어도writer와State를항상호출한다.
    for owner, manager, pattern, state, position, capacity in itertools.product((0, 1, 8), (0, 11), range(5), (0, 4, 255), (0, 4), (0, 256)):
        rows.append(('recover', owner, manager, pattern, state, position, 0, capacity, 0, 'none'))
    # 직접writer의두독립진단과원문NULL입력전달을분리한다.
    for manager, pattern, position, end, capacity, null in itertools.product((0, 11, 22), (0, 4), (0, 4), (0, 0xffffffff), (0, 256), (0, 1)):
        rows.append(('write', 1, manager, pattern, 255, position, end, capacity, null, 'none'))
    mutations = ('read-owner', 'read-manager22', 'read-partial-stop', 'log1-owner', 'log1-manager22', 'log1-byte1-80', 'log1-byte1-255', 'log1-byte7-0', 'log1-stop', 'log2-stop', 'create-owner', 'create-manager22', 'create-current2', 'create-stop', 'write-owner', 'write-manager22', 'write-byte1-255', 'write-stop', 'state-owner', 'state-returnother', 'state-byte0-255', 'state-zero', 'state-stop', 'assert36-data1', 'assert36-pos7', 'assert36-owner', 'assert36-stop')
    # callback한개가현재값/이미받은지역버퍼를바꾸며나머지는PE가진행한다.
    for mutation in mutations:
        rows.append(('recover', 1, 11, 1, 255, 0, 0, 0 if mutation.startswith('assert36') else 256, 0, mutation))
    rows.extend((('write', 1, 0, 1, 255, 0, 0, 256, 0, 'assert521-manager22'), ('write', 1, 0, 1, 255, 0, 0, 256, 1, 'assert521-stop'), ('write', 1, 11, 1, 255, 0, 0, 256, 1, 'assert522-manager22'), ('write', 1, 11, 1, 255, 0, 0, 256, 1, 'assert522-stop')))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """getter/State/로그/Create/진단만명시경계로두고회복·writer·Write는실제PE로실행한다."""
    def __init__(self):
        """기존보호PE읽기/가상메모리에쓰기감시만추가한다."""
        super().__init__()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total_instructions = self.normal_returns = 0
        self.boundaries, self.stops = Counter(), Counter()

    def guard(self, mu, access, address, size, value, data):
        """CPU쓰기범위를stack/실제stream·output/State로한정한다."""
        ranges = ((STACK - 0x180, STACK + 8), (STATES, STATES + 9 * 28), *((base, base + 16) for base in STREAMS), *((base, base + 256) for base in BUFFERS))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'Territory회복CPU쓰기범위밖 {address:08x}/{size}')

    def key(self, pointer):
        """원문manager주소를명시입력번호로만표시한다."""
        return next(key for key, base in MANAGERS.items() if base == pointer)

    def record(self, kind, text):
        """실제하위진입순서와1기반순번을기록한다."""
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        self.events.append(text)
        return self.calls[kind]

    def stop(self, cause):
        """원문정상반환과다른명시하위/물리중단이다."""
        self.cause = cause
        self.mu.emu_stop()

    def mutate(self, boundary):
        """캡처지역버퍼/고정writer헤더/현재owner의명시효과만쓴다."""
        mutation = self.case[-1]
        if mutation == boundary + '-owner':
            self.put(OWNER, self.other)
        elif mutation == boundary + '-manager22':
            slot = self.fixed if boundary in ('create', 'assert521', 'assert522') else self.words(OWNER, 1)[0]
            self.put(HEADERS + slot * 20, MANAGERS[22])
        elif mutation == 'create-current2' and boundary == 'create':
            manager = self.words(HEADERS + self.fixed * 20, 1)[0]
            if manager:
                self.put(manager + 8, STREAMS[2])
        elif mutation == 'log1-byte1-80' and boundary == 'log1':
            self.put(self.local + 1, 128, 1)
        elif mutation in ('log1-byte1-255', 'write-byte1-255') and boundary == mutation.split('-')[0]:
            self.put(self.local + 1, 255, 1)
        elif mutation == 'log1-byte7-0' and boundary == 'log1':
            self.put(self.local + 7, 0, 1)
        elif mutation == 'state-returnother' and boundary == 'state':
            self.return_state = self.other
        elif mutation == 'state-byte0-255' and boundary == 'state':
            self.put(STATES + self.return_state * 28, 255, 1)
        elif mutation == 'state-zero' and boundary == 'state':
            self.return_state = None
        elif mutation == 'assert36-data1' and boundary == 'assert36':
            self.put(self.output_stream, BUFFERS[1])
        elif mutation == 'assert36-pos7' and boundary == 'assert36':
            self.put(self.output_stream + 4, 7)
        elif mutation == boundary + '-stop' or mutation == 'read-partial-stop' and boundary == 'read':
            self.stop(boundary)

    def instruction(self, mu, address, size, data):
        """회복·flags재조회·current선택·Write의실제원문명령만허용한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 10000:
            raise RuntimeError('Territory회복명령상한초과')
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4)
        if address == 0x4bd600:
            self.fixed = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            self.events.append('F:' + str(self.fixed))
        if address in (0x4bd603, 0x4bd63f, 0x4bd64b):
            self.events.append('M:' + str(self.fixed) + ':' + str(self.key(self.words(HEADERS + self.fixed * 20, 1)[0])))
        if address == 0x4bd64d:
            manager = mu.reg_read(UC_X86_REG_EAX)
            if not manager:
                self.stop('manager0')
                return
            self.events.append('C:' + str(self.key(manager)) + ':' + str(STREAMS.index(self.words(manager + 8, 1)[0])))
        if address == 0x44cd1c and not mu.reg_read(UC_X86_REG_ESI):
            self.stop('source0')
            return
        if address == 0x4b5f5d and not mu.reg_read(UC_X86_REG_EAX):
            self.stop('state0')
            return
        if address == 0x4bd660:
            slot = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            self.local = args[0]
            self.record('read', 'R:' + str(slot) + ':' + bytes(mu.mem_read(self.local, 120)).hex())
            length = 37 if self.case[-1] == 'read-partial-stop' else 120
            mu.mem_write(self.local, bytes(mu.mem_read(SOURCES + slot * 120, length)))
            self.mutate('read')
            if not self.cause:
                self.ret(purge=4)
        elif address == 0x4c2630:
            assert args[0] == 29 and self.string(args[1]) == MESSAGE
            index = self.record('log', 'L:29:' + MESSAGE.hex())
            self.mutate('log' + str(index))
            if not self.cause:
                self.ret()
        elif address == 0x44d1b0:
            assert self.string(args[0]) == b'Territory'
            self.record('create', 'K:' + str(self.key(mu.reg_read(UC_X86_REG_ECX))) + ':Territory')
            self.mutate('create')
            if not self.cause:
                self.ret(STREAMS[2], 4)  # 원문writer는반환stream이아닌현재manager+8을다시읽는다.
        elif address == 0x44cce0:
            self.output_stream = mu.reg_read(UC_X86_REG_ECX)
            assert args[1] == 120
            source = bytes(mu.mem_read(args[0], 120)).hex() if args[0] else 'null'
            self.record('write', 'W:' + str(STREAMS.index(self.output_stream)) + ':120:' + source)
            self.mutate('write')
            if self.cause:
                return
        elif address == 0x4bd550:
            slot = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            self.return_state = slot
            self.record('state', 'S:' + str(slot))
            self.mutate('state')
            if not self.cause:
                self.ret(0 if self.return_state is None else STATES + self.return_state * 28)
        elif address == 0x4e0620:
            line = args[2]
            assert line in (521, 522, 36)
            self.record('assert', 'A:' + str(line))
            self.mutate('assert' + str(line))
            if not self.cause:
                self.ret()
        elif not any(low <= address < high for low, high in RANGES):
            raise RuntimeError('Territory회복원문범위밖: ' + hex(address))

    def sample(self, case, control):
        """모든지역/출력/현재값을독립초기화하고부분효과까지읽는다."""
        self.case = case
        kind, owner, manager, pattern, state, position, end, capacity, null, mutation = case
        self.other, self.fixed = (owner + 1) % 9, owner
        self.calls, self.events, self.cause = Counter(), [], ''
        self.instructions = 0
        self.local = LOCAL if kind == 'recover' else INPUT
        self.put(OWNER, owner)
        self.mu.mem_write(HEADERS, bytes((0x70 + index) & 255 for index in range(180)))
        # 현재/다른슬롯의manager를구별하고사용하지않는슬롯도명시0으로둔다.
        for slot in range(9):
            self.put(HEADERS + slot * 20, 0)
            self.mu.mem_write(SOURCES + slot * 120, records(slot, pattern))
            self.mu.mem_write(STATES + slot * 28, bytes((0x80 + slot + byte * 7) & 255 for byte in range(28)))
            self.put(STATES + slot * 28, state, 1)
        self.put(HEADERS + self.other * 20, MANAGERS[22])
        self.put(HEADERS + owner * 20, MANAGERS[manager])
        # Create반환은stream2지만각실제manager.current는처음stream0/1이다.
        for key, base in MANAGERS.items():
            if key:
                self.mu.mem_write(base, bytes((0x50 + key + byte) & 255 for byte in range(16)))
                self.put(base + 8, STREAMS[key // 11 - 1])
        for index, base in enumerate(STREAMS):
            self.mu.mem_write(base, struct.pack('<4I', BUFFERS[index], position, end, capacity))
            self.mu.mem_write(BUFFERS[index], bytes((0xb0 + index * 7 + byte) & 255 for byte in range(256)))
        self.mu.mem_write(INPUT, records(owner, pattern))
        self.mu.mem_write(LOCAL, b'\xcd' * 120)
        # 이전관측의정수레지스터/stack을빌리지않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_ECX, HEADERS + owner * 20)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<2I', STOP, 0 if null else INPUT))
        self.mu.emu_start(0x4b5ec0 if kind == 'recover' else 0x4bd600, STOP)
        if self.cause:
            self.stops[self.cause] += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + (4 if kind == 'recover' else 8)
            self.normal_returns += 1
        state_out = ','.join(map(str, (self.words(OWNER, 1)[0], self.calls['read'], self.calls['log'], self.calls['create'], self.calls['write'], self.calls['state'], self.calls['assert'])))
        raw = ':'.join((bytes(self.mu.mem_read(HEADERS, 180)).hex(), *(bytes(self.mu.mem_read(base, 16)).hex() for key, base in MANAGERS.items() if key), *(bytes(self.mu.mem_read(base, 16)).hex() for base in STREAMS), *(bytes(self.mu.mem_read(base, 256)).hex() for base in BUFFERS), bytes(self.mu.mem_read(STATES, 9 * 28)).hex(), bytes(self.mu.mem_read(INPUT, 120)).hex(), bytes(self.mu.mem_read(LOCAL, 120)).hex() if kind == 'recover' else '-'))
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause or '1', state_out, raw, ';'.join(self.events)))


def main():
    """최초fixture를제품전에고정하고그뒤에는보호SHA만검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 최초근거/입력/관측을제품에맞춰변경하지못하게함께확인한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Territory회복최초SHA불일치: ' + name)
        print('Territory회복최초원문보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Territory회복최초fixture/report덮어쓰기금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / kind,owner,manager,pattern,state,pos,end,capacity,nullInput,mutation / return-or-stop / current7fields / fullraw / trace']
    # 두정밀도는정수/문자열본문도따로실행한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_territory_recover.py', 'cpppj/tests/fixtures/fort-territory-recover-x86.tsv')
    report = {'status': '신규 독립 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entries': ['004b5ec0', '004bd600', '0044cce0'], 'bodyBytes': [167, 93, 96], 'ranges': [[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'normalReturns': oracle.normal_returns, 'explicitStops': dict(oracle.stops), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제회복167/writer93/일반Write96의지역zero120/flags·로그뒤재조회/현재owner·현재manager·current재조회/진단뒤계속/실제DWORD복사/State첫BYTE회복. Territory getter004bd660/State004bd550/Create/로그/진단은명시경계. manager0/NULLsource/State0의물리접근은명시중단이며전체getter/State/새섹션/OS예외/파일/게임/GUI실행이아님.', 'gameExecuted': False, 'osExecuted': False, 'filesMutated': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'instructions')}))


if __name__ == '__main__':
    main()
