#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 Badges CRT와count reset을실제독립x86로실행해최초장부fixture를고정한다."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import socket
import struct

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호원문/새검증자료의저장소루트다.
PE = ROOT / 'originals/Netstorm.exe'  # OS진입없이읽기만하는10.78 PE다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 현재보호PE전체SHA다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-badges-globals-x86.tsv'  # 제품작성전에고정할최초독립관측이다.
REPORT = ROOT / 'cpppj/fort-badges-globals-native.json'  # 원본범위/명령/전체SHA를남길새보고서다.
BASE, STACK, STOP = 0x400000, 0x15008000, 0x1400F000  # PE/독립스택/정상반환주소다.
TABLE, LENGTH = 0x593160, 6160  # 28×220BYTE장부의실제전체범위다.
RANGES = {'crt': (0x4FCBE0, 0x4FEA10), 'reset': (0x47D860, 0x47D883)}  # RET까지포함한두온전한몸체다.
CONTROLS = (0x27F, 0x37F)  # 정수초기화도두x87제어어를그대로보존해야한다.
REGISTERS = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)  # 입력간CPU잔여를제거할레지스터다.
STATIC_CRT_SHA = '40e455f3941361b21cc236838aa108416bf859a5214aa76dbd7cbb23d159cbc8'  # 별도로수행한정적상수대입기대이며실행결과를만드는용도로사용하지않는다.
SCHEMA = ('name', 'kind', 'profile', 'completed', 'output', 'stagehashes', 'stackdelta', 'nativeinstructions', 'tablewrites', 'writtenbytes')  # 앞3열은입력이며뒤7열은실제원문관측이다.


def digest(path):
    """전체파일바이트의SHA를구한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def initial(profile):
    """기대결과를만들지않고비0/변화패턴의명시6160BYTE입력만구성한다."""
    if profile == 0:
        return bytes(LENGTH)
    if profile in (1, 2):
        return bytes([255 if profile == 1 else 0xA5]) * LENGTH
    if profile in (3, 4, 5):
        return bytes((index if profile == 3 else 255 - index if profile == 4 else index * 37 + 19) & 255 for index in range(LENGTH))
    if profile == 6:
        return b''.join(struct.pack('<I', (index * 0x10003 + 0x80000000) & 0xFFFFFFFF) for index in range(LENGTH // 4))
    if profile == 7:
        return bytes(1 + index % 255 for index in range(LENGTH))
    raise RuntimeError('Badges명시입력프로필범위오류')


class Oracle:
    """외부경계없이CRT/reset원문만실행하는독립CPU다."""
    def __init__(self):
        """보호PE메모리와별도스택만준비하고게임/OS를시작하지않는다."""
        if digest(PE) != PE_SHA:
            raise RuntimeError('Badges보호PE SHA불일치')
        self.pe = pefile.PE(str(PE));self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 읽기전용파서와격리CPU다.
        self.mu.mem_map(BASE, (self.pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.mu.mem_write(BASE, self.pe.get_memory_mapped_image());self.mu.mem_map(STOP & ~4095, 4096)
        self.mu.mem_map(STACK - 0x8000, 0x10000)
        self.original = {start: bytes(self.mu.mem_read(start, end - start)) for start, end in RANGES.values()}  # 실행바이트불변검사용이다.
        self.pe_initial = bytes(self.mu.mem_read(TABLE, LENGTH))  # OS로더전개후CRT전가상0값이다.
        if any(self.pe_initial):
            raise RuntimeError('Badges PE초기가상0영역불일치')
        self.mu.hook_add(UC_HOOK_CODE, self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.instructions = 0;self.returns = 0;self.counts = Counter();self.zero_crt = None  # 실제전체관측누계와BSS0의CRT후관측이다.

    def instruction(self, mu, address, size, user):
        """실제한몸체밖의코드진입을거부하며외부CALL을대체하지않는다."""
        if address == STOP:
            self.completed = True;mu.emu_stop();return
        start, end = RANGES[self.stage]  # 이번원문함수하나의허용범위다.
        if not start <= address < end:
            raise RuntimeError(f'Badges허용밖코드진입{address:08x}')
        self.instructions += 1;self.sample_instructions += 1;self.counts[self.stage] += 1

    def write(self, mu, access, address, size, value, user):
        """원문쓰기를실제장부와분리한스택으로만제한한다."""
        if TABLE <= address and address + size <= TABLE + LENGTH:
            self.tablewrites += 1;self.written.update(range(address - TABLE, address - TABLE + size));return
        if STACK - 0x100 <= address and address + size <= STACK + 8:
            return
        raise RuntimeError(f'Badges허용밖쓰기{address:08x}/{size}')

    def sample(self, kind, profile, control):
        """같은장부에지정순서의실제함수를실행하고전체바이트/정상ABI를관측한다."""
        self.mu.mem_write(TABLE, initial(profile));self.sample_instructions = 0;self.tablewrites = 0;self.written = set();hashes = []  # 입력마다독립적인전체장부와실제쓰기누계다.
        for stage in kind.split('-'):  # 합성부모없이정상반환한원문을지정순서로이어붙인다.
            self.stage = stage;self.completed = False;self.mu.mem_write(STACK - 0x100, bytes([0xCC]) * 0x120);self.mu.mem_write(STACK, struct.pack('<I', STOP))
            before = {}  # 함수정상RET의callee-save확인용이다.
            for index, register in enumerate(REGISTERS):  # 이전함수의레지스터잔여를제거한다.
                value = 0x10111213 + index * 0x10101010;before[register] = value;self.mu.reg_write(register, value)
            self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
            self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPSW, 0)
            self.mu.emu_start(RANGES[stage][0], STOP + 1, count=20000)
            delta = self.mu.reg_read(UC_X86_REG_ESP) - STACK - 4  # 무인자cdecl RET4만정규화한다.
            if not self.completed or delta or any(self.mu.reg_read(register) != before[register] for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)):
                raise RuntimeError('Badges정상RET/스택/callee-save불일치')
            if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
                raise RuntimeError('Badges x87제어어/스택변경')
            raw = bytes(self.mu.mem_read(TABLE, LENGTH));hashes.append(stage + ':' + hashlib.sha256(raw).hexdigest());self.returns += 1
            if kind == 'crt' and profile == 0:
                self.zero_crt = raw
            for address, original in self.original.items():  # 두함수원문이실행뒤에도불변이어야한다.
                if bytes(self.mu.mem_read(address, len(original))) != original:
                    raise RuntimeError('Badges보호원문바이트변경')
        return [1, raw.hex(), ';'.join(hashes), delta, self.sample_instructions, self.tablewrites, len(self.written)]


def main():
    """원문실행후최초fixture를만들고기존결과덮어쓰기는거부한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, default=FIXTURE);parser.add_argument('--report', type=Path, default=REPORT)
    parser.add_argument('--verify', action='store_true');parser.add_argument('--dry-run', action='store_true');args = parser.parse_args()
    if args.verify:
        record = json.loads(args.report.read_text(encoding='utf-8'))
        for name, expected in record['sha256'].items():  # 최초보호/도구/fixture전체바이트를재확인한다.
            if digest(ROOT / name) != expected:
                raise RuntimeError('Badges최초관측SHA불일치:' + name)
        print('Badges CRT/reset최초근거확인', record['inputs']);return
    if not args.dry_run and (args.fixture.exists() or args.report.exists()):
        raise RuntimeError('Badges최초원본fixture/보고서덮어쓰기금지')
    oracle = Oracle();lines = ['# ' + '\t'.join(SCHEMA)];count = 0  # 제품코드/자료를읽거나기대결과로사용하지않는다.
    for kind in ('crt', 'reset', 'crt-reset', 'reset-crt'):  # 단독과두실제함수의합성순서를구분한다.
        for profile in range(8):  # 최초0과모든BYTE가비0인장부도같은실제몸체에공급한다.
            results = [oracle.sample(kind, profile, control) for control in CONTROLS]
            if results[0] != results[1]:
                raise RuntimeError('Badges두x87제어어관측불일치')
            lines.append('\t'.join(map(str, [f'badges-global-{count:03}', kind, profile] + results[0])));count += 1
    actual = hashlib.sha256(oracle.zero_crt).hexdigest()
    if actual != STATIC_CRT_SHA:
        raise RuntimeError('Badges실제CRT결과와선행정적전개기대가다름')
    record = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon / 2026-10-23', inputs=count, executions=count * len(CONTROLS),
                  normalFunctionReturns=oracle.returns, instructions=oracle.instructions, stageInstructions=dict(oracle.counts),
                  peInitialSha256=hashlib.sha256(oracle.pe_initial).hexdigest(), nativeCrtZeroSha256=actual,
                  staticExpectedSha256=STATIC_CRT_SHA, nativeMatchesStatic=True, gameExecuted=False, osExecuted=False, externalCalls=0,
                  ranges={name: f'{start:08x}-{end-1:08x}' for name, (start, end) in RANGES.items()},
                  scope='CRT/reset본체전체·전체6160BYTE·허용쓰기·스택/callee-save/x87불변을실제실행. 로더/저장/Client/GUI/OS미실행.')
    if not args.dry_run:
        args.fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
        paths = [PE, ROOT / 'extracted/refined/originals/Netstorm.c', Path(__file__).resolve(), args.fixture.resolve()]
        record['sha256'] = {str(path.relative_to(ROOT)).replace('\\', '/'): digest(path) for path in paths}
        args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(record, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
