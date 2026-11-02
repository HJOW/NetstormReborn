#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제41b4c0 전체부모와 디스크/부분파일/키 몸체를 실행해 mode/create/실패/고리를 고정한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct

from decomp_basefile_read_policy import Oracle as ReadOracle, OBJECT, PRIMARY, SECONDARY
from decomp_file_name_leaf import add_ranges
from decomp_file_name_formatter import RING, INDEX
from decomp_priestmenu import ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

NEWKEY = TYPES + 0x4000  # 실제 새 키 할당에만 쓰는 격리 물리다.


class OpenFailure(Exception):
    """원문 EX_UNABLE_TO_OPEN_FILE 생성 직전의 비복귀 경계를 구별한다."""


class AllocationFailure(Exception):
    """키 operator_new가 예외를 던진 위치의 실제 부분 필드를 구별한다."""


class OpenOracle(ReadOracle):
    """기존 읽기부모 범위에 실제 일반열기/부분파일/키를 더하며 CRT/메타데이터만 명시한다."""

    def __init__(self):
        """원문 부모와 실제 하위 함수의 불연속 명령 범위만 추가한다."""
        super().__init__()
        add_ranges(self, (0x41a5b0, 0x41b460, 0x41a620))

    def instruction(self, mu, address, size, data):
        """FILE/메타데이터/할당의 현재 인자를 기록하며 본체의 필드 store는 실제 코드가 수행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 5)
        if address == 0x41a5b0:
            caller = self.words(esp, 1)[0]
            self.active = {0x41b53a: 1, 0x41b5cb: 3, 0x41b678: 0, 0x41b743: 3}[caller]
            return
        if address == 0x41b460:
            self.active = 5
            return
        if address == 0x4e545a:
            name, mode = self.string(args[0]), self.string(args[1])
            self.events.append('O:' + name.hex() + ':' + mode.decode('ascii'))
            success = self.archive_open if self.active == 5 else bool(self.mask & (1 << self.active)) or (self.create_alt and mode == b'w+b' and self.mode == 1 and self.create)
            self.ret(PLAYERS if success else 0)
            return
        if address == 0x41a340:
            secondary = mu.reg_read(UC_X86_REG_ECX) == 0x544f80
            self.events.append('A:' + str(int(secondary)) + ':' + self.string(args[0]).hex())
            site = 4 if secondary else 2
            self.put(args[1], 0x234 if secondary else 0x123)
            self.put(args[1] + 4, 0x456 if secondary else 0x345)
            self.put(args[2], self.encrypted)
            self.ret(TEXT + (0x5100 if secondary else 0x5000) if self.mask & (1 << site) else 0, 12)
            return
        if address == 0x4e573c:
            self.events.append('S:' + ':'.join(str(x) for x in args[:3]))
            self.ret(0xffffffff)
            return
        if address == 0x4e4354:
            self.events.append('F:' + str(args[0]))
            self.ret()
            return
        if address == 0x4e4391:
            self.events.append('N:' + str(args[0]))
            if self.allocate_fail:
                raise AllocationFailure()
            self.allocated = args[0]
            mu.mem_write(NEWKEY, bytes([205]) * self.allocated)
            self.ret(NEWKEY)
            return
        if address == 0x41a0d0:
            self.events.append('E:' + self.string(args[0]).hex())
            raise OpenFailure()
        ReadOracle.instruction(self, mu, address, size, data)

    def sample(self, case, precision):
        """명시 입력만 설치한 뒤 실제 최종여섯필드/고리/키/모든CRT인자를 반환한다."""
        name, self.mode, self.create, self.mask, self.archive_open, self.encrypted, initial, self.create_alt, self.allocate_fail = case
        self.events = []
        self.trace = []
        self.instructions = 0
        self.allocated = 0
        self.mu.mem_write(STACK - 0x2000, bytes(0x2100))
        self.mu.mem_write(RING, bytes(2560))
        self.mu.mem_write(OBJECT, struct.pack('<6I', 0x1001, 0x82, 7, 9, TYPES, 3))
        self.mu.mem_write(TYPES, b'old\0')
        self.put(INDEX, initial)
        self.put(0, 0)
        self.put(0x532728, 92, 1)
        self.mu.mem_write(PRIMARY, b'C:\\base\0')
        self.mu.mem_write(SECONDARY, b'D:\\disc\0')
        self.mu.mem_write(TEXT, name + b'\0')
        self.mu.mem_write(TEXT + 0x5000, b'primary.tarc\0')
        self.mu.mem_write(TEXT + 0x5100, b'secondary.tarc\0')
        self.mu.mem_write(STACK, struct.pack('<4I', STOP, TEXT, self.mode, self.create))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, OBJECT)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        outcome = 'ok'
        try:
            self.mu.emu_start(0x41b4c0, STOP, count=30000)
        except OpenFailure:
            outcome = 'unable'
        except AllocationFailure:
            outcome = 'allocation'
        if outcome == 'ok':
            assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 16
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        index = self.words(INDEX, 1)[0]
        final = ','.join(str(x) for x in self.words(OBJECT, 6))
        slot = bytes(self.mu.mem_read(RING + index * 256, 256)).hex()
        key = bytes(self.mu.mem_read(NEWKEY, self.allocated)).hex() if self.allocated else '-'
        return '\t'.join([name.hex() or '-'] + [str(x) for x in case[1:]] + [outcome, ';'.join(self.events) or '-', final, str(index), slot, key])


def main():
    """최초 관측은 지정한 새fixture/보고서에만 쓰며 준비본 밖 기존fixture를 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle = OpenOracle()
    cases = []
    names = (b'.\\d\\a', b'd/a', b'C:\\base\\d\\a', b'C:\\BASE\\d\\a', b'D:\\other\\d\\a', b'\\\\server\\share\\d\\a', b'd\\a\0ignored:C')
    for name, mode, create, mask, opened, encrypted in itertools.product(names, range(3), range(2), (0, 1, 2, 4, 8, 16, 7, 31), range(2), range(2)):  # 실제 모드/생성/출처 성공·부분파일·암호 조합을 독립 입력으로 만든다.
        cases.append((name, mode, create, mask, opened, encrypted, len(cases) % 10, 0, 0))
    for initial in range(10):  # 모든 고리 시작 번호에서 생성 재시도와 키 할당 실패도 확인한다.
        cases.append((b'.\\d\\a', 1, 1, 0, 1, 0, initial, 1, 0))
        cases.append((b'.\\d\\a', 2, 0, 4, 1, 1, initial, 0, 1))
    rows = ['# nameHex mode create mask archiveOpen encrypted initial createAlt allocateFail outcome events finalFields index slotHex keyHex']
    for case in cases:  # 같은 입력을 두 정밀도로 관측하고 한 행의 기대 바이트를 동결한다.
        result = oracle.sample(case, 0x027f)
        assert result == oracle.sample(case, 0x037f)
        rows.append(result)
    payload = '\n'.join(rows) + '\n'
    if args.verify:
        assert args.fixture.read_text('utf-8') == payload
    else:
        assert not args.fixture.exists()  # 기존 최초 관측은 verify로만 대조하고 덮어쓰지 않는다.
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text(payload, 'utf-8', newline='\n')
    report = dict(unit=757, date='2026-11-03', host=socket.gethostname(), lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'), rows=len(cases), observations=len(cases) * 2, gameExecutions=0, multiplayerTests=0,
        boundary='실제41b4c0/FileSpec/Device/Format/일반열기/부분파일/키setter. 실제OS fopen/fseek/free/new와archive metadata만명시경계. 실패는41a0d0진입또는operator_new예외에서중단하므로256BYTE예외객체/SEH unwind/OS FILE ABI는범위밖',
        sha256={str(p.relative_to(ROOT)): digest(p) for p in (ROOT / 'originals/Netstorm.exe', ROOT / 'extracted/refined/originals/functions.tsv')})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(rows=len(cases), observations=len(cases) * 2)))


if __name__ == '__main__':
    main()
