#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""palette전환114/캐시반납29/ColorSort이름59/dirty20BYTE를제품전에최초고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

PALETTE_NAME, SORT_NAME = 0x5c8910, 0x56b7a0  # 원문이고정주소에NUL까지쓰는두현재이름이다.
CACHE, COUNT, RENDERER, DIRTY = 0x569ffc, 0x540a80, 0x59a8bc, 0x59a8b0  # 같은부모/ColorSort/dirty가직접공유하는현재DWORD다.
INPUTS, PATHS = (TEXT, TEXT + 0x400), (TEXT + 0x1000, TEXT + 0x1200)  # 명시원문두개512BYTE와두경로256BYTE의실제할당이다.
TOKENS = {0: 0, 11: POOL + 0x1000, 22: POOL + 0x2000, 33: POOL + 0x3000}  # 실제할당/free하위는토큰장치로만분리한다.
RANGES = ((0x4b6100, 0x4b6172), (0x470c30, 0x470c4d), (0x471190, 0x47119d), (0x4711a0, 0x4711ce), (0x4973c0, 0x4973d4))  # 불연속59BYTE를포함한네실제본문이다.
ENTRIES = {'transition': 0x4b6100, 'cache': 0x470c30, 'sort': 0x471190, 'dirty': 0x4973c0}  # 각원문진입점은같은장치상태를독립초기화한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-palette-transition-x86.tsv'  # 제품전에고정하고이후덮어쓰기를거부한다.
REPORT = ROOT / 'cpppj/fort-palette-transition-native.json'  # 실제/명시경계와전체원시/SHA를보존한다.


def cases():
    """같은이름/빈이름/긴이름/alias·캐시·현재count·renderer와부분중단을독립교차한다."""
    rows = []
    # 같은이름도로그는항상호출하지만어떤캐시/파일/dirty효과도추가하지않는다.
    for name, cache, count, renderer, dirty in itertools.product(range(7), (0, 11), (0, 1, 3), (0, 1), (0, 0xffffffff)):
        rows.append(('transition', name, cache, count, renderer, dirty, 22, 0, 'none'))
    # 각작은하위도독립실행해미선택장치와직접물리copy를분리한다.
    for kind, name, cache, count, renderer in itertools.product(('cache', 'sort', 'dirty'), (0, 3, 4, 5, 6), (0, 11), (0, 3), (0, 1)):
        rows.append((kind, name, cache, count, renderer, 0x12345678, 22, 1, 'none'))
    mutations = ('log-argumentB', 'log-source', 'log-cache11', 'log-stop', 'compare-argumentB', 'compare-source', 'compare-palette', 'compare-cache0', 'compare-stop', 'free-cache33', 'free-source', 'free-palette', 'free-stop', 'build-count0', 'build-count3', 'build-cache0', 'build-argumentB', 'build-source', 'build-stop', 'step1-count0', 'step1-cache0', 'step1-argumentB', 'step1-source', 'step1-stop', 'step2-stop', 'path-return1', 'path-return0', 'path-source', 'path-renderer0', 'path-stop', 'load-renderer0', 'load-renderer1', 'load-dirty77', 'load-stop', 'maps-renderer0', 'maps-renderer1', 'maps-dirty77', 'maps-palette', 'maps-stop')
    # 변경된입력값의영향만주고재조회/분기/반복횟수는PE본문에맡긴다.
    for mutation in mutations:
        rows.append(('transition', 0, 11, 3, 0 if mutation.endswith('renderer1') else 1, 0x12345678, 22, 0, mutation))
    rows.extend((('transition', 0, 0, 0, 1, 0, 0, 0, 'none'), ('sort', 0, 0, 0, 1, 0, 0, 0, 'build-count3'), ('sort', 0, 11, 3, 1, 0, 22, 0, 'step1-count0'), ('transition', 0, 11, 0xffffffff, 1, 0, 22, 0, 'step3-stop'), ('sort', 0, 11, 0xffffffff, 1, 0, 22, 0, 'step3-stop')))
    # 물리범위를벗어나는실제BYTE접근은정상반환대신그명령직전에멈춘다.
    for kind, name in (('transition', 7), ('transition', 8), ('sort', 8), ('transition', 9), ('transition', 10), ('sort', 10)):
        rows.append((kind, name, 11, 3, 1, 0x12345678, 22, 0, 'none'))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """부모/이름copy/free뒤0/count루프/dirty는실제PE로실행하고장치계산은명시입력으로둔다."""
    def __init__(self):
        """기존보호PE/분리가상메모리에CPU쓰기범위guard만추가한다."""
        super().__init__()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total_instructions = 0
        self.boundaries, self.completions = Counter(), Counter()

    def guard(self, mu, access, address, size, value, data):
        """정확한두이름256BYTE/두전역/stack밖의CPU쓰기를거부한다."""
        allowed = ((STACK - 0x100, STACK + 8), (PALETTE_NAME, PALETTE_NAME + 256), (SORT_NAME, SORT_NAME + 256), (CACHE, CACHE + 4), (DIRTY, DIRTY + 4))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'palette전환CPU쓰기범위밖 {address:08x}/{size}')

    def record(self, name, text):
        """실제하위진입과해당하위의1기반순번을기록한다."""
        self.calls[name] += 1
        self.boundaries[name] += 1
        self.events.append(text)
        return self.calls[name]

    def finish(self, cause):
        """명시callback/물리중단은정상ret와따로보존한다."""
        self.cause = cause
        self.mu.emu_stop()

    def readable(self, address):
        """실제빌린이름/원문의끝을넘지않고남은물리길이만돌려준다."""
        regions = ((PALETTE_NAME - 1, 257), (SORT_NAME - 1, 257), *((base, 512) for base in INPUTS), *((base, 256) for base in PATHS))
        for low, length in regions:
            if low <= address < low + length:
                return low + length - address
        return 0

    def cstring(self, address, cause):
        """명시CRT/파일경계에서도없는주소나NUL없는물리를빈문자열로가장하지않는다."""
        length = self.readable(address)
        if not length:
            self.finish(cause)
            return None
        data = bytes(self.mu.mem_read(address, length))
        found = data.find(b'\0')
        if found < 0:
            self.finish(cause)
            return None
        return data[:found]

    def mutate(self, boundary):
        """캡처source내용/인자변수/현재cache/count/renderer의명시외부효과만쓴다."""
        mutation = self.case[-1]
        source = self.source
        if mutation == boundary + '-argumentB':
            self.put(STACK + 4, INPUTS[1])
        elif mutation == boundary + '-source':
            self.mu.mem_write(source, b'ChangedByChild\0')
        elif mutation == boundary + '-palette':
            self.mu.mem_write(PALETTE_NAME, b'ChangedPalette\0')
        elif mutation == boundary + '-cache11':
            self.put(CACHE, TOKENS[11])
        elif mutation == boundary + '-cache33':
            self.put(CACHE, TOKENS[33])
        elif mutation == boundary + '-cache0':
            self.put(CACHE, 0)
        elif mutation == boundary + '-count0':
            self.put(COUNT, 0)
        elif mutation == boundary + '-count3':
            self.put(COUNT, 3)
        elif mutation == boundary + '-renderer0':
            self.put(RENDERER, 0)
        elif mutation == boundary + '-renderer1':
            self.put(RENDERER, 1)
        elif mutation == boundary + '-dirty77':
            self.put(DIRTY, 77)
        elif mutation == 'path-return1' and boundary == 'path':
            self.path = 1
        elif mutation == 'path-return0' and boundary == 'path':
            self.path = None
        elif mutation == boundary + '-stop':
            self.finish(boundary)

    def instruction(self, mu, address, size, data):
        """네실제본문과허용명시하위만실행하며copy직전물리접근을관측한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 10000:
            raise RuntimeError('palette전환명령상한초과')
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 5)
        if address in (0x4b6130, 0x4711a0):
            if not self.readable(mu.reg_read(UC_X86_REG_EAX)):
                self.finish('palette-read' if address == 0x4b6130 else 'sort-read')
                return
        if address in (0x4b6132, 0x4711a2):
            target = (mu.reg_read(UC_X86_REG_EDX) + mu.reg_read(UC_X86_REG_EAX)) & 0xffffffff
            base = PALETTE_NAME if address == 0x4b6132 else SORT_NAME
            if not base <= target < base + 256:
                self.finish('palette-write' if address == 0x4b6132 else 'sort-write')
                return
        if address == 0x4c2630:
            assert args[0] == 29 and self.string(args[1]) == b'init palette\n'
            self.record('log', 'LOG:29:696e69742070616c657474650a')
            self.mutate('log')
            if not self.cause:
                self.ret()
        elif address == 0x4e65ed:
            assert args[0] == PALETTE_NAME
            self.source = args[1]
            self.record('compare', f'COMPARE:{args[0]}:{args[1]}')
            first, second = self.cstring(args[0], 'compare-physical'), self.cstring(args[1], 'compare-physical')
            if self.cause:
                return
            result = int(first.lower() != second.lower())
            self.mutate('compare')
            if not self.cause:
                self.ret(result)
        elif address == 0x4e4354:
            self.record('free', 'FREE:' + str(args[0]))
            self.mutate('free')
            if not self.cause:
                self.ret()
        elif address == 0x471040:
            self.record('build', 'BUILD:' + str(self.words(CACHE, 1)[0]) + ':' + str(self.words(COUNT, 1)[0]))
            self.put(CACHE, TOKENS[self.case[6]])
            self.mutate('build')
            if not self.cause:
                self.ret()
        elif address == 0x470b20:
            index = self.record('step', 'STEP:' + str(self.calls['step'] + 1) + ':' + str(self.words(CACHE, 1)[0]) + ':' + str(self.words(COUNT, 1)[0]))
            self.mutate('step' + str(index))
            if not self.cause:
                self.ret()
        elif address == 0x4410f0:
            assert self.string(args[0]) == b'GamePalSpec' and args[2:4] == (0, 0)
            self.record('path', 'PATH:' + str(args[1]) + ':0:0')
            name = self.cstring(args[1], 'path-physical')
            if name is None:
                return
            self.events.append('PATH-NAME:' + name.hex())
            self.mutate('path')
            if not self.cause:
                self.ret(0 if self.path is None else PATHS[self.path])
        elif address == 0x4a4850:
            assert args[1] == 1
            self.record('load', 'LOAD:' + str(args[0]) + ':1')
            path = self.cstring(args[0], 'load-physical')
            if path is None:
                return
            self.events.append('LOAD-PATH:' + path.hex())
            self.mutate('load')
            if not self.cause:
                self.ret()
        elif address == 0x43c010:
            self.record('maps', 'MAPS:' + str(self.words(RENDERER, 1)[0]) + ':' + str(self.words(DIRTY, 1)[0]))
            self.mutate('maps')
            if not self.cause:
                self.ret()
        elif not any(low <= address < high for low, high in RANGES):
            raise RuntimeError('palette전환원문범위밖 ' + hex(address))

    def sample(self, case, control):
        """두이름/원문/현재DWORD/stack을독립초기화하고전체부분효과를읽는다."""
        self.case = case
        kind, name, cache, count, renderer, dirty, built, path, mutation = case
        self.path, self.cause, self.calls, self.events, self.instructions = path, '', Counter(), [], 0
        self.mu.mem_write(PALETTE_NAME - 1, b'Z' + b'\xa1' * 256)
        self.mu.mem_write(SORT_NAME - 1, b'Y' + b'\xb2' * 256)
        self.mu.mem_write(PALETTE_NAME, b'gifcloud\0' if name in (1, 2) else b'OldPalette\0')
        self.mu.mem_write(SORT_NAME, b'LegacySort\0')
        # NUL뒤주변과미사용원문도별개입력으로둔다.
        for index, base in enumerate(INPUTS):
            self.mu.mem_write(base, bytes((0xc0 + index * 7 + byte) & 255 for byte in range(512)))
        self.mu.mem_write(INPUTS[0], b'GIFCLOUD\0' if name == 1 else b'\0' if name == 3 else b'LongPaletteName' * 14 + b'\0' if name == 6 else b'gifcloud\0')
        self.mu.mem_write(INPUTS[1], b'SecondPalette\0')
        if name == 8:
            self.mu.mem_write(INPUTS[0], b'Q' * 512)
        self.source = {4: PALETTE_NAME + 1, 5: SORT_NAME + 1, 7: PALETTE_NAME - 1, 9: SORT_NAME, 10: SORT_NAME - 1}.get(name, INPUTS[0])
        # 실제화면/파일은열지않고두PathSpec반환의주소와NUL내용을구별한다.
        for index, base in enumerate(PATHS):
            self.mu.mem_write(base, bytes((0xd0 + index * 5 + byte) & 255 for byte in range(256)))
            self.mu.mem_write(base, (b'Work/First.col\0', b'Work/Second.col\0')[index])
        self.put(CACHE, TOKENS[cache])
        self.put(COUNT, count)
        self.put(RENDERER, renderer)
        self.put(DIRTY, dirty)
        self.mu.mem_write(STACK - 0x100, b'\xcd' * 0x108)
        self.put(STACK, STOP)
        self.put(STACK + 4, self.source)
        # 이전관측의callee보존레지스터나정밀도에기대지않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.emu_start(ENTRIES[kind], STOP)
        if not self.cause:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        self.completions[self.cause or 'return'] += 1
        values = ','.join(map(str, (self.words(CACHE, 1)[0], self.words(COUNT, 1)[0], self.words(RENDERER, 1)[0], self.words(DIRTY, 1)[0], self.words(STACK + 4, 1)[0], -1 if self.path is None else self.path, self.calls['free'], self.calls['build'], self.calls['step'], self.calls['load'], self.calls['maps'])))
        raw = ':'.join((bytes(self.mu.mem_read(PALETTE_NAME - 1, 257)).hex(), bytes(self.mu.mem_read(SORT_NAME - 1, 257)).hex(), *(bytes(self.mu.mem_read(base, 512)).hex() for base in INPUTS), *(bytes(self.mu.mem_read(base, 256)).hex() for base in PATHS)))
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause or '1', values, raw, ';'.join(self.events)))


def main():
    """최초원문관측을제품전에고정하고이후에는근거SHA만검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 제품이나fixture수정으로최초관측을맞추지못하도록함께잠근다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('palette전환최초SHA불일치 ' + name)
        print('palette전환최초원문보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('palette전환최초fixture/report덮어쓰기금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / kind,nameCase,cache,count,renderer,dirty,built,path,mutation / return-or-stop / current11fields / raw / trace']
    # 네정수/문자열본문도두정밀도에서각각독립실행한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:], case
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_palette_transition.py', 'cpppj/tests/fixtures/fort-palette-transition-x86.tsv')
    report = {'status': '단위383다음준비·신규독립미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entries': {key: f'{value:08x}' for key, value in ENTRIES.items()}, 'bodyBytes': {'transition': 114, 'cache': 29, 'sort': 59, 'dirty': 20}, 'ranges': [[f'{lo:08x}', f'{hi:08x}'] for lo, hi in RANGES], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'completions': dict(oracle.completions), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제4b6100/470c30/471190/4973c0의로그뒤인자pointer캡처·이름비교결과분기·NUL포함전방BYTEcopy·alias/부분쓰기·free뒤현재cache무조건0·cache0일때만build·build뒤현재count캡처·step뒤count재조회없음·GamePalSpec4인자/LoadPalette(path,1)·maps뒤현재renderer/dirty. CRT비교/로그/free/471040/470b20/PathSpec/Screen arg2=1/43c010은명시필수하위이며실제장치/색표계산/파일/GUI/OS/SEH전체실행은아님. 음수DWORDcount는3번째step명시중단입력으로만관측.', 'gameExecuted': False, 'guiExecuted': False, 'osExecuted': False, 'filesMutated': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'completions', 'instructions')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
