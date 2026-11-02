#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제440e00/FileSpec/이름 getter의 인자 전환·검색 핸들 수명을 CPU로 관측한다."""
import argparse
import json
import socket
import struct
from pathlib import Path

from decomp_file_name_leaf import LeafOracle, add_ranges
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW
import pefile

FIXTURE = ROOT / 'cpppj/tests/fixtures/config-path-load-x86.tsv'  # 실제440e00의 콜백/검색/고리 순서 최초 기록이다.
FIND, CLOSE = STOP + 0x100, STOP + 0x200  # 실제 OS 대신 명시 응답만 제공하는 격리 경계다.


class LoadOracle(LeafOracle):
    """본문 설정 파서는 명시 경계이며 Load 부모/전체 FileSpec/이름 getter는 실제 코드다."""
    def __init__(self):
        """함수 범위와 두 실제 Win32 import만 제한된 OS 장치로 연결한다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.events = []  # 실제 진입 인자/순서만 기록한다.
        self.found = b''  # FindFirst의 명시 단일 반환 이름이며 빈 값이면 실패다.
        add_ranges(self, (0x440e00, 0x45a220, 0x459610, 0x4595f0, 0x4e7620))
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        connected = set()  # 다른 import를 성공으로 합성하지 않는다.
        for library in pe.DIRECTORY_ENTRY_IMPORT:  # 실제 IAT의 이름/주소를 읽어 지정한 두 함수만 교체한다.
            for item in library.imports:  # 이름 없는 ordinal은 이 경계에 포함하지 않는다.
                if item.name in (b'FindFirstFileA', b'FindClose'):
                    self.put(item.address, FIND if item.name == b'FindFirstFileA' else CLOSE)
                    connected.add(item.name)
        assert len(connected) == 2

    def guard(self, mu, access, address, size, value, data):
        """원문 SEH/변경 알림/환경 표식만 추가 허용하며 다른 전역 쓰기를 금지한다."""
        if address in (0, 0x558dd0, 0x558df0) and size == 4:
            return
        NameOracle.guard(self, mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """원문이 도달한 설정 경계와 실제 Find/Close 순서를 관측한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x43fd40:
            self.events.append('clear')
            self.ret()
            return
        if address in (0x440330, 0x440410, 0x440380):
            name = self.words(esp + 4, 1)[0]
            kind = {0x440330: 'section', 0x440410: 'inline', 0x440380: 'read'}[address]
            self.events.append(f'{kind}:{self.string(name).hex()}')
            self.ret(0, 4)
            return
        if address == 0x4c2630:
            self.ret()
            return
        if address == FIND:
            name, output = self.words(esp + 4, 2)
            self.events.append('find:' + self.string(name).hex())
            if self.found:
                mu.mem_write(output + 44, self.found + b'\0')
            self.ret(0x1234 if self.found else 0xffffffff, 8)
            return
        if address == CLOSE:
            handle = self.words(esp + 4, 1)[0]
            self.events.append(f'close:{handle}')
            self.ret(1, 4)
            return
        if address == 0x459b20:
            self.events.append('name')
        NameOracle.instruction(self, mu, address, size, data)

    def load(self, arguments, found, precision):
        """가변 C인자/NULL 종결을 설치하고 실제 정상 반환/보존 규약을 확인한다."""
        self.events.clear()
        self.found = found
        self.thrown = False
        self.put(0, 0)
        self.put(INDEX, 0)
        self.put(0x532728, 92, 1)
        self.mu.mem_write(RING, bytes(2560))
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        inputs = []  # 입력은 첫NUL 뒤 물리까지 불변이어야 한다.
        words = [STOP, 0]
        for index, value in enumerate(arguments):  # 가변 인자는 종료0까지 원래 순서로 공급한다.
            address = TEXT + index * 512
            self.mu.mem_write(address, value + b'\0')
            inputs.append((address, value + b'\0'))
            words.append(address)
        words.append(0)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * len(words), *words))
        for number, register in enumerate(REGISTERS):  # 정상 cdecl의 비영 레지스터 수명을 대조한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        before = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.emu_start(0x440e00, STOP, count=50000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert before == [self.mu.reg_read(register) for register in REGISTERS] and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert all(bytes(self.mu.mem_read(address, len(value))) == value for address, value in inputs)
        assert self.words(0, 1) == (0,)
        advances = self.events.count('name')
        assert self.words(INDEX, 1)[0] == advances % 10
        return dict(advances=advances, events=list(self.events))


def main():
    """전환/NUL/빈 파일과 검색 성공·실패 수명을 새 fixture에만 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    cases = [([], b''), ([b'a=1', b'b=2'], b''), ([b'a=1', b'@', b'', b'one.cfg', b'd/two.cfg'], b''),
             ([b'@one.cfg', b'@d\\two.cfg', b'x=3'], b''), ([b'@C:\\data\\one.cfg', b'\\\\server\\share\\two.cfg'], b''),
             ([b'@d\\\xe9cole.cfg', b'plain\0ignored'], b''), ([b'@\0ignored', b'next.cfg'], b''),
             ([b'@d\\*.cfg'], b'chosen.cfg'), ([b'@d\\*.cfg'], b''),
             ([b'@d/*.cfg'], b'chosen.cfg'), ([b'@*.cfg'], b'chosen.cfg'),
             ([b'@' + b'd\\' + b'x' * 253], b'')]
    oracle = LoadOracle()
    rows = ['# arguments_hex found_hex advances events']
    records = []
    for arguments, found in cases:  # 첫@ 뒤 일반 인자도 파일이 되는 원문 관문을 실행한다.
        result = oracle.load(arguments, found, 0x027f)
        assert result == oracle.load(arguments, found, 0x037f)
        encoded = ','.join(value.hex() or '-' for value in arguments) or '-'
        rows.append(f'{encoded}\t{found.hex() or "-"}\t{result["advances"]}\t{";".join(result["events"])}')
        records.append(dict(arguments=[value.hex() for value in arguments], found=found.hex(), **result))
    content = '\n'.join(rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, 'utf-8', newline='\n')
    report = dict(unit=731, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  fixtureRows=len(cases), observations=len(cases) * 2, rows=records,
                  boundary='실제440e00/45a220/459610/4595f0/459b20과실제CRT. 설정Clear/Section/Inline/Read·로그·FindFirst/FindClose는명시경계. 실제OS/파서/예외unwind를실행하지않음',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)): digest(path) for path in (
                      ROOT / 'originals/Netstorm.exe', Path(__file__), ROOT / 'tools/decomp_file_name_leaf.py', FIXTURE)})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(fixtureRows=len(cases), observations=len(cases) * 2, advances=[row['advances'] for row in records])))


if __name__ == '__main__':
    main()
