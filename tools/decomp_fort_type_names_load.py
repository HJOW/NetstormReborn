#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TypeNames004bd800의실제314BYTE와별도SEH cleanup15BYTE를제품전에독립고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TYPES, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, STREAMS = POOL + 0x100, (POOL + 0x200, POOL + 0x220)  # 실제고정9헤더와별개인두16BYTE stream이다.
BUFFERS, BUFFER_BYTES = (POOL + 0x1000, POOL + 0x2000), 1088  # 최대255hash와명시pos를분리하는물리입력이다.
BLOCKS = {0: 0, 11: POOL + 0x4000, 22: POOL + 0x4200, 33: POOL + 0x4400}  # 기존/새할당/교체255BYTE의실제주소다.
DEPTH, CONVERSION, TYPE_COUNT = 0x5c89b8, 0x5c89bc, 0x541348  # 매원문위치에서현재값을읽는세전역이다.
NAMES = (b'Alpha', b'Beta', b'', b'\xffA\x80\x7f', b'Alpha', b'LongNameWithMoreThanTwentyCharacters123', b'Case', b'case')  # 중복/빈이름/signedchar/20자초과/대소문자를구별한다.
RANGES = ((0x4bd800, 0x4bd93a), (0x4fb820, 0x4fb828), (0x4bcc50, 0x4bcc57))  # 본문및별도cleanup실제명령만허용한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-type-names-load-x86.tsv'  # 제품작성전에최초고정한다.
REPORT = ROOT / 'cpppj/fort-type-names-load-native.json'  # 범위/부분상태/별도cleanup과근거SHA다.


def name_hash(name):
    """저장입력hash만독립구성하며기대변환표는계산하지않는다."""
    result = 0
    # 실제signedchar입력의합을32비트로감는다.
    for index, byte in enumerate(name):
        result = (result + ((byte if byte < 128 else byte - 256) << ((index & 3) * 8))) & 0xffffffff
    return result


def hashes(mode):
    """첫일치/중복파일칸/불일치인저장쪽원시DWORD입력이다."""
    return tuple(name_hash(NAMES[index]) for index in (0, 1, 2, 3)) if mode == 0 else (name_hash(NAMES[0]), name_hash(NAMES[0]), name_hash(NAMES[1]), 0) if mode == 1 else (0xffffffff, 0x12345678, 0x80000000, 0x7fffffff)


def cases():
    """빈stream/기존pointer/signedcount/pos/end/현재callback효과를독립교차한다."""
    rows = []
    # 빈data와end0은기존pointer를해제하지않고0으로덮는경로다.
    for data, end, old, count, depth, stored in itertools.product((0, 1), (0, 1, 20, 0xffffffff), (0, 11), (-1, 0, 1, 7), (0, 0xffffffff), (0, 4)):
        rows.append((1, 11, 0, data, end, 0, stored, 0, count, old, 22, depth, 'none'))
    # 고정this와select반환은현재owner라는편의캐시를도입하지않는다.
    for slot, manager, selected in itertools.product((0, 1, 8), (0, 11), (0, 1)):
        rows.append((slot, manager, selected, 1, 1, 0, 4, 1, 16, 11, 22, 3, 'none'))
    # DWORD주소감김/실제물리경계와signed end비교를구별한다.
    for position, end, selected in itertools.product((7, 1087, 1088, 0xffffffff), (1, 0xffffffff), (0, 1)):
        rows.append((1, 11, selected, 1, end, position, 4, 0, 7, 0, 22, 1, 'none'))
    # 타입낮은BYTE wrap과storedCount255도원문계산으로관측한다.
    for stored, count, mode in itertools.product((1, 4, 255), (16, 260), (0, 1, 2)):
        rows.append((1, 11, 0, 1, 1, 0, stored, mode, count, 0, 22, 1, 'none'))
    mutations = ('select-stream1', 'select-manager0', 'select-depth7', 'select-conversion22', 'select-stop', 'assert-conversion0', 'assert-data1', 'assert-pos9', 'assert-depth7', 'assert-count0', 'assert-stop', 'allocate-conversion11', 'allocate-data1', 'allocate-pos9', 'allocate-endffffffff', 'allocate-count0', 'allocate-depthffffffff', 'allocate-stop', 'record1-conversion11', 'record1-conversion0', 'record1-count0', 'record1-count260', 'record1-depth7', 'record1-name', 'record1-bufferhash', 'record1-data1', 'record1-stop', 'record2-stop')
    # 한하위효과만바꾸며각뒤재조회는PE가결정한다.
    for mutation in mutations:
        rows.append((1, 11, 0, 1, 1, 0, 4, 0, 7, 11, 22, 1, mutation))
    rows.extend(((1, 11, 0, 1, 1, 0, 4, 0, 7, 11, 0, 1, 'none'), (1, 11, 0, 1, 1, 0, 4, 0, 7, 11, 33, 1, 'none')))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """Select/할당/TYPE/진단만명시경계로두고검색/zero/실제전역·stream쓰기는PE가한다."""
    def __init__(self):
        """FS한페이지만추가하고보호PE를별도가상메모리에서읽는다."""
        super().__init__()
        self.mu.mem_map(0, 4096)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total_instructions = self.normal_returns = self.cleanup_returns = 0
        self.boundaries, self.stops = Counter(), Counter()

    def guard(self, mu, access, address, size, value, data):
        """원문CPU쓰기와실제깊이관측을FS/stack/두stream/현재전역/255블록으로한정한다."""
        ranges = ((0, 4), (STACK - 0x100, STACK + 8), (DEPTH, CONVERSION + 4), *((base, base + 16) for base in STREAMS), *((base, base + 255) for key, base in BLOCKS.items() if key))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'TypeNames CPU쓰기범위밖 {address:08x}/{size}')
        if address == DEPTH:
            self.events.append('L:' + str(value & 0xffffffff))

    def record(self, kind, text):
        """원문실제진입순서와1기반순번을기록한다."""
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        self.events.append(text)
        return self.calls[kind]

    def stop(self, cause):
        """SEH전개와다른명시callback/물리중단표식이다."""
        self.cause = cause
        self.mu.emu_stop()

    def mutate(self, boundary):
        """원문분기나기대변환표를대신하지않고명시현재필드만쓴다."""
        mutation = self.case[-1]
        stream = STREAMS[self.selected]
        if mutation == 'select-stream1' and boundary == 'select':
            self.selected = 1
        elif mutation == 'select-manager0' and boundary == 'select':
            self.put(HEADERS + self.case[0] * 20, 0)
        elif mutation == boundary + '-depth7':
            self.put(DEPTH, 7)
        elif mutation == boundary + '-depthffffffff':
            self.put(DEPTH, 0xffffffff)
        elif mutation in (boundary + '-conversion0', boundary + '-conversion11', boundary + '-conversion22'):
            self.put(CONVERSION, BLOCKS[int(mutation.split('conversion')[1])])
        elif mutation == boundary + '-data1':
            self.put(stream, BUFFERS[1])
        elif mutation == boundary + '-pos9':
            self.put(stream + 4, 9)
        elif mutation == boundary + '-endffffffff':
            self.put(stream + 8, 0xffffffff)
        elif mutation == boundary + '-count0':
            self.put(TYPE_COUNT, 0)
        elif mutation == boundary + '-count260':
            self.put(TYPE_COUNT, 260)
        elif mutation == 'record1-name' and boundary == 'record1':
            self.mu.mem_write(TYPES + 4, b'Beta\0')
        elif mutation == 'record1-bufferhash' and boundary == 'record1':
            self.put(BUFFERS[0] + 1, name_hash(b'Beta'))
        elif mutation == boundary + '-stop':
            self.stop(boundary)

    def instruction(self, mu, address, size, data):
        """실제314BYTE/별도cleanup만실행하고물리주소접근직전도달상태를보존한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 3000000:
            raise RuntimeError('TypeNames 명령상한초과')
        # 원문은끝을검사하지않는다.호스트물리거부를원문진단으로가장하지않는다.
        if address in (0x4bd850, 0x4bd8f9):
            target = (mu.reg_read(UC_X86_REG_EAX) + mu.reg_read(UC_X86_REG_ECX)) & 0xffffffff if address == 0x4bd850 else (mu.reg_read(UC_X86_REG_ECX) + mu.reg_read(UC_X86_REG_EAX) * 4) & 0xffffffff
            width = 1 if address == 0x4bd850 else 4
            if not any(base <= target and target + width <= base + BUFFER_BYTES for base in BUFFERS):
                self.stop('count-read' if width == 1 else 'hash-read')
                return
        if address == 0x4bd893 and not mu.reg_read(UC_X86_REG_EDI):
            self.stop('allocation0')
            return
        if address == 0x4bd90f:
            target = (mu.reg_read(UC_X86_REG_EDX) + mu.reg_read(UC_X86_REG_EAX)) & 0xffffffff
            if not any(base <= target < base + 255 for key, base in BLOCKS.items() if key):
                self.stop('conversion-write')
                return
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 3)
        if address == 0x44ceb0:
            assert self.string(args[0]) == b'TypeNames' and args[1] == 1
            self.record('select', 'S:' + str(mu.reg_read(UC_X86_REG_ECX)) + ':TypeNames:1')
            self.mutate('select')
            if not self.cause:
                self.ret(STREAMS[self.selected], 8)
        elif address == 0x4e0620:
            assert args[2] == 608
            self.record('assert', 'A:608')
            self.mutate('assert')
            if not self.cause:
                self.ret()
        elif address == 0x4e4391:
            assert args[0] == 255
            self.record('allocate', 'N:255')
            self.mutate('allocate')
            if not self.cause:
                self.ret(BLOCKS[self.case[10]])
        elif address == 0x49a840:
            index = args[0]
            call = self.record('record', 'R:' + str(index) + ':' + self.string(TYPES + index * 500 + 4).hex())
            self.mutate('record' + str(call))
            if not self.cause:
                self.ret(TYPES + index * 500)
        else:
            if address in (0x4bd8b0, 0x4bd912):
                self.events.append('C:' + str(struct.unpack('<i', mu.mem_read(TYPE_COUNT, 4))[0]))
            if not any(low <= address < high for low, high in RANGES):
                raise RuntimeError('TypeNames 허용본문밖: ' + hex(address))

    def state(self):
        """전역/명시호출횟수는그시점의현재값만읽는다."""
        pointer = next(key for key, base in BLOCKS.items() if base == self.words(CONVERSION, 1)[0])
        return ','.join(map(str, (self.words(DEPTH, 1)[0], pointer, struct.unpack('<i', self.mu.mem_read(TYPE_COUNT, 4))[0], self.selected, self.calls['select'], self.calls['assert'], self.calls['allocate'], self.calls['record'])))

    def raw(self):
        """고정header/stream/data/세255블록/변경가능이름두개를NUL뒤도모두관측한다."""
        return ':'.join((bytes(self.mu.mem_read(HEADERS, 180)).hex(), *(bytes(self.mu.mem_read(base, 16)).hex() for base in STREAMS), *(bytes(self.mu.mem_read(base, BUFFER_BYTES)).hex() for base in BUFFERS), *(bytes(self.mu.mem_read(base, 255)).hex() for key, base in BLOCKS.items() if key), *(bytes(self.mu.mem_read(TYPES + index * 500 + 4, 128)).hex() for index in range(2))))

    def sample(self, case, control):
        """최초본문부분상태뒤필요시별도원문cleanup을실행하여두범위를분리한다."""
        self.case = case
        slot, manager, self.selected, present, end, position, stored, mode, count, old, allocated, depth, mutation = case
        self.calls, self.events, self.cause = Counter(), [], ''
        self.instructions = 0
        self.put(0, 0x12345678)
        self.put(DEPTH, depth)
        self.put(CONVERSION, BLOCKS[old])
        self.put(TYPE_COUNT, count)
        self.mu.mem_write(HEADERS, bytes((0x70 + index) & 255 for index in range(180)))
        self.put(HEADERS + slot * 20, manager)
        # 실제stream은같은초기필드를가지되data배열주소는서로다르다.
        for index, base in enumerate(STREAMS):
            self.mu.mem_write(base, struct.pack('<4I', BUFFERS[index] if present else 0, position, end, BUFFER_BYTES))
            self.mu.mem_write(BUFFERS[index], bytes((0xa0 + offset + index * 7) & 255 for offset in range(BUFFER_BYTES)))
            if position < BUFFER_BYTES:
                self.put(BUFFERS[index] + position, stored, 1)
                # 저장쪽hash입력만쓰며예상변환표는원문이결정한다.
                for number in range(stored):
                    target = position + 1 + number * 4
                    if target + 4 <= BUFFER_BYTES:
                        self.put(BUFFERS[index] + target, hashes(mode)[(number + index) % 4])
        # 할당/기존포인터의NUL과무관한255BYTE전체초기입력을보존한다.
        for key, base in BLOCKS.items():
            if key:
                self.mu.mem_write(base, bytes((0x60 + key + offset) & 255 for offset in range(255)))
        # 타입count가260으로늘어나도실제500BYTE기록은먼저모두존재한다.
        for index in range(262):
            self.mu.mem_write(TYPES + index * 500 + 4, b'\xa7' * 128)
            self.mu.mem_write(TYPES + index * 500 + 4, NAMES[index % len(NAMES)] + b'\0')
        # 각관측은이전CPU레지스터나FS상태를빌리지않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, HEADERS + slot * 20)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(0x4bd800, STOP)
        body, raw, trace = self.state(), self.raw(), ';'.join(self.events)
        cause = self.cause
        cleanup = '-'
        if cause:
            self.stops[cause] += 1
            self.events, self.cause = [], ''
            self.mu.reg_write(UC_X86_REG_ESP, STACK)
            self.mu.mem_write(STACK, struct.pack('<I', STOP))
            self.mu.emu_start(0x4fb820, STOP)  # OS의CxxFrameHandler전체가아니라확인한funclet/helper만독립실행한다.
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            cleanup = self.state() + '/' + ';'.join(self.events)
            self.cleanup_returns += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 and self.words(0, 1)[0] == 0x12345678
            self.normal_returns += 1
        return '\t'.join((str(control), ','.join(map(str, case)), cause or '1', body, raw, trace, cleanup))


def main():
    """제품작성전최초결과를고정하고그뒤에는SHA만검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 최초입력/근거/관측을제품에맞춰갱신하지못하게함께확인한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('TypeNames 최초SHA불일치: ' + name)
        print('TypeNames 최초원문보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('TypeNames 최초fixture/report덮어쓰기금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / slot,manager,selected,data,end,pos,stored,hashMode,typeCount,oldConversion,allocated,depth,mutation / body-return-or-stop / state8 / raw / trace / separate-cleanup-state-and-trace']
    # x87두정밀도는같은정수본문도각각독립실행한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_type_names_load.py', 'cpppj/tests/fixtures/fort-type-names-load-x86.tsv')
    report = {'status': '신규 독립 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004bd800', 'bodyBytes': 314, 'cleanupBytes': 15, 'ranges': [[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'normalReturns': oracle.normal_returns, 'explicitStops': dict(oracle.stops), 'separateCleanupReturns': oracle.cleanup_returns, 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제TypeNames314BYTE의depth/pos/end/현재conversion255zero/hash/첫일치·현재count와별도예외funclet8BYTE/helper7BYTE만실행. Select/allocate/TYPE/진단은명시경계. 본문부분중단과별도cleanup결과를구별한다. 데이터끝/할당0/현재conversion0의물리거부는원문자체의검사가아니다. OS SEH/CxxFrameHandler/게임/GUI/파일조작은실행하지않음.', 'gameExecuted': False, 'osExecuted': False, 'filesMutated': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'separateCleanupReturns', 'instructions')}))


if __name__ == '__main__':
    main()
