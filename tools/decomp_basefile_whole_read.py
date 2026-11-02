#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문41baa0의 Probe/최소4·추가폭/출력길이/ctor완료·실제cleanup funclet을 고정한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, TYPES, TEXT, STACK, STOP, digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

ALLOCATION, KEY, OUTPUT = TYPES + 0x2000, TYPES + 0x100, TEXT + 0x200  # 격리 출력할당/고정키/선택length출력의 서로 다른 실제 물리다.


class BoundaryFailure(Exception):
    """명시 Open/new/fread/fclose 장치의 일반예외 진입을 구별한다."""


class WholeReadOracle(MemoryOracle):
    """41baa0·길이·읽기·소멸/funclet은 실제코드이며 Open/Probe/CRT/할당만 명시한다."""
    def __init__(self):
        """확인한 함수범위와 SEH 첫슬롯만 준비하고 원본이미지/파일은 쓰지 않는다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.allowed = []
        add_ranges(self, (0x41baa0, 0x41aa80, 0x41a7a0, 0x41b410, 0x41a680, 0x4f93a0))
        self.allowed.append((0x4e4a40, 0x4e4a7e))

    def fields(self):
        """같은 지역File의 여섯DWORD를 포인터 토큰까지 원문폭으로 읽는다."""
        return list(self.words(self.object, 6))

    def instruction(self, mu, address, size, data):
        """장치인자와 실제현재필드를 기록하고 실제파일/게임/통신은 호출하지 않는다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 4)
        if address == 0x41ab10:
            self.events.append('P:' + self.string(args[0]).hex())
            if self.scenario == 6:
                mu.mem_write(TEXT, b'changed.bin\0')
            self.ret(self.probe)
            return
        if address == 0x41b4c0:
            assert mu.reg_read(UC_X86_REG_ECX) == self.object and args[1:3] == (0, 1)
            self.events.append('O:' + self.string(args[0]).hex() + ':0:1:' + ','.join(str(x) for x in self.fields()))
            fields = [0 if self.scenario == 2 else PLAYERS, 0x80 | self.subfile, 37 if self.subfile else 0, self.length if self.subfile else 0, KEY if self.key else 0, 3 if self.key else 0]
            mu.mem_write(self.object, struct.pack('<6I', *fields))
            self.position = 37 if self.subfile else 0
            if self.scenario == 1:
                raise BoundaryFailure('open')
            self.ret(0, 12)
            return
        if address == 0x4e566c:
            self.events.append('T:' + str(args[0]))
            self.ret(self.position)
            return
        if address == 0x4e573c:
            self.events.append('S:' + ':'.join(str(x) for x in args[:3]))
            self.position = self.length if args[2] == 2 else args[1]
            self.ret(0)
            return
        if address == 0x4e4391:
            self.events.append('N:' + str(args[0]))
            if self.scenario == 3:
                raise BoundaryFailure('allocation')
            self.allocated = args[0]
            assert self.allocated <= 0x4000
            mu.mem_write(ALLOCATION, bytes([205]) * self.allocated)
            self.ret(0 if self.scenario == 7 else ALLOCATION)
            return
        if address == 0x4e586e:
            target, length, count, file = args
            if self.key and self.scratch is None:
                self.scratch = target
                mu.mem_write(target, bytes([101]) * 4096)  # 원문미정스택값을호스트와같다고가정하지않고부분암호읽기의명시물리seed만설치한다.
            actual = min(length * count, self.available)
            self.events.append(f'R:{file}:{length}:{count}:{actual}')
            if self.scenario == 4:
                raise BoundaryFailure('read')
            if actual:
                mu.mem_write(target, bytes(((self.used + n) * 29 + 7) & 255 for n in range(actual)))
            self.used += actual
            self.available -= actual
            self.position += actual
            self.ret(count if length and actual == length * count else 0)
            return
        if address == 0x4e54b9:
            self.events.append('C:' + str(args[0]))
            if self.scenario == 5:
                raise BoundaryFailure('close')
            self.ret(0)
            return
        if address == 0x4e4354:
            self.events.append('F:' + str(args[0]))
            self.ret()
            return
        if not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'전체읽기 명령 범위 밖 {address:08x}')

    def sample(self, case, precision):
        """입력/물리seed만 설치하고 원문이 기록한 할당/길이/필드/cleanup효과를 반환한다."""
        self.length, extra, self.key, self.subfile, output, check, self.probe, self.available, self.scenario = case
        self.object = STACK - 36
        self.events, self.allocated, self.used, self.scratch = [], 0, 0, None
        self.mu.mem_write(STACK - 0x10000, bytes([101]) * 0x10100)
        self.mu.mem_write(TEXT, bytes(256))
        self.mu.mem_write(TEXT, b'sample.bin\0')
        self.mu.mem_write(KEY, b'abc\0')
        self.put(OUTPUT, 0xdeadbeef)
        self.put(0, 0)
        self.mu.mem_write(STACK, struct.pack('<5I', STOP, TEXT, check, OUTPUT if output else 0, extra & 0xffffffff))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        outcome = 'ok'
        try:
            self.mu.emu_start(0x41baa0, STOP, count=300000)
        except BoundaryFailure as failure:
            outcome = str(failure)
            if self.words(STACK - 4, 1)[0] == 0:
                self.mu.reg_write(UC_X86_REG_EBP, STACK)
                self.mu.reg_write(UC_X86_REG_ESP, STACK + 0x100)
                self.put(STACK + 0x100, STOP)
                self.mu.emu_start(0x4f93a0, STOP, count=10000)
        result = self.mu.reg_read(UC_X86_REG_EAX) if outcome == 'ok' else 0
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        payload = bytes(self.mu.mem_read(ALLOCATION, self.allocated)).hex() if self.allocated else '-'
        return '\t'.join([str(x) for x in case] + [outcome, str(result), str(self.words(OUTPUT, 1)[0]), ','.join(str(x) for x in self.fields()), ';'.join(self.events), payload])


def main():
    """최초fixture 또는 읽기전용 재관측만 수행하며 기존파일/디컴파일을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    cases = [(n, extra, key, sub, output, 0, 2, n, 0) for n, extra, key, sub, output in itertools.product((0, 1, 3, 4, 4096, 4101), (0, 3), range(2), range(2), range(2))]
    cases += [(n, 0, key, sub, 1, 0, 2, n // 2, 0) for n, key, sub in itertools.product((1, 4, 4101), range(2), range(2))]
    cases += [(4, 0, key, sub, 1, 0, 2, 4, scenario) for scenario, key, sub in itertools.product((1, 2, 3, 4, 5), range(2), range(2))]
    cases += [(1, 0, 0, 0, 1, 1, 0, 1, 0), (1, 0, 0, 0, 1, 1, 2, 1, 6), (0, -1, 0, 0, 1, 0, 2, 0, 0), (3, -1, 0, 0, 1, 0, 2, 3, 0), (0, 0, 0, 0, 1, 0, 2, 0, 7)]
    oracle = WholeReadOracle()
    rows = ['# length extra key subfile lengthOut check probe available scenario outcome token outputLength fields events allocationHex']
    # 각 독립 입력을 두 x87 정밀도에서 실행하고 전체 결과가 같을 때만 보존한다.
    for case in cases:
        first = oracle.sample(case, 0x027f)
        assert first == oracle.sample(case, 0x037f)
        rows.append(first)
    payload = '\n'.join(rows) + '\n'
    if args.verify:
        assert args.fixture.read_text('utf-8') == payload
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text(payload, 'utf-8', newline='\n')
    args.output.write_text(json.dumps(dict(unit=763, host=socket.gethostname(), rows=len(cases), observations=len(cases) * 2, gameExecutions=0, multiplayerTests=0,
        boundary='실제41baa0/길이/전체읽기/소멸과명시예외뒤실제004f93a0 cleanup funclet. Open/Probe/FILE/new와최초암호scratch4096BYTE=101seed는명시경계. MSVC SEH dispatcher는실행하지않고marker0에서실제funclet을별도호출. 음수길이/할당밖원문미정쓰기는정상행에포함하지않음',
        sha256={str(p.relative_to(ROOT)): digest(p) for p in (ROOT / 'originals/Netstorm.exe', ROOT / 'extracted/refined/originals/functions.tsv')}), ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(rows=len(cases), observations=len(cases) * 2)))


if __name__ == '__main__':
    main()
