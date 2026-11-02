#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 exists 소비자가 호출하는 실제41ab10의 출처/메타우선/전체FileSpec/같은10칸을 고정한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_config_path_load import LoadOracle, FIND, CLOSE
from decomp_file_name_leaf import add_ranges
from decomp_file_name_formatter import RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, PLAYERS, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW


class ProbeOracle(LoadOracle):
    """실제부모/FileSpec/Device/Format/CRT문자열과secondary_EXISTS를실행하고OS/metadata만명시한다."""
    def __init__(self):
        """기존FindFirst/Close와원문허용명령범위에실제존재부모만추가한다."""
        super().__init__()
        add_ranges(self, (0x41ab10, 0x41aae0, 0x459900))

    def instruction(self, mu, address, size, data):
        """payload fread/길이/키는허용하지않고실제metadata/open/close순서만기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 4)
        if address == FIND:
            self.events.append('find:' + self.string(args[0]).hex())
            if self.found:
                mu.mem_write(args[1] + 44, b'chosen.gif\0')
            self.ret(0x1234 if self.found else 0xffffffff, 8)
            return
        if address == CLOSE:
            self.events.append('searchClose:' + str(args[0]))
            self.ret(1, 4)
            return
        if address == 0x41a340:
            secondary = mu.reg_read(UC_X86_REG_ECX) == 0x544f80
            self.events.append('A:' + str(int(secondary)) + ':' + self.string(args[0]).hex())
            self.ret(TEXT + 0x5000 if self.mask & (1 << (4 if secondary else 2)) else 0, 12)
            return
        if address == 0x4e545a:
            caller = self.words(esp, 1)[0]
            site = {0x41abbb: 1, 0x41ac97: 0, 0x41aaf0: 3}[caller]
            assert self.string(args[1]) == b'rb'
            self.events.append('D:' + str(site) + ':' + self.string(args[0]).hex())
            self.ret(PLAYERS if self.mask & (1 << site) else 0)
            return
        if address == 0x4e54b9:
            self.events.append('C:' + str(args[0]))
            self.ret(0)
            return
        LoadOracle.instruction(self, mu, address, size, data)

    def sample(self, name, mask, found, initial, precision):
        """원래입력/명시검색/출처mask만설치하고정상반환/전체첫장치칸/마지막칸을관측한다."""
        self.mask, self.found, self.events, self.thrown = mask, found, [], False
        self.mu.mem_write(STACK - 0x2000, bytes([101]) * 0x2100)
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, initial)
        self.put(0, 0)
        self.put(0x532728, 92, 1)
        self.mu.mem_write(0x544d80, b'C:\\base\0')
        self.mu.mem_write(0x544e80, b'D:\\disc\0')
        self.mu.mem_write(TEXT, name + b'\0')
        self.mu.mem_write(STACK, struct.pack('<2I', STOP, TEXT))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.emu_start(0x41ab10, STOP, count=30000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        index = self.words(INDEX, 1)[0]
        first = bytes(self.mu.mem_read(RING + ((initial + 1) % 10) * 256, 256)).hex()
        current = bytes(self.mu.mem_read(RING + index * 256, 256)).hex()
        return '\t'.join([name.hex() or '-', str(mask), str(int(found)), str(initial), str(self.mu.reg_read(UC_X86_REG_EAX)), ';'.join(self.events), str(index), first, current])


def main():
    """새fixture만생성하거나최초출력과읽기전용으로재관측을대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    names = (b'd\\a.gif', b'd/a.gif', b'C:\\base\\d\\a.gif', b'\\\\server\\share\\a.gif', b'\\root\\a.gif', b'd\\a\0ignored:C', b'', b'd\\*.gif', b'd/*.gif', b'C:\\base\\d\\*.gif')
    oracle = ProbeOracle()
    rows = ['# nameHex mask found initial result events index firstDeviceHex currentHex']
    # 열 가지 이름과 전체출처조합/검색반환을 두 정밀도로 관측한다.
    for name, mask, found in itertools.product(names, range(32), range(2)):
        initial = (len(rows) - 1) % 10
        row = oracle.sample(name, mask, found, initial, 0x027f)
        assert row == oracle.sample(name, mask, found, initial, 0x037f)
        rows.append(row)
    payload = '\n'.join(rows) + '\n'
    if args.verify:
        assert args.fixture.read_text('utf-8') == payload
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text(payload, 'utf-8', newline='\n')
    report = dict(unit=765, host=socket.gethostname(), lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'), rows=len(rows) - 1, observations=(len(rows) - 1) * 2, gameExecutions=0, multiplayerTests=0,
        boundary='실제41ab10/41aae0/FileSpec/Device/Format/CRT문자열. FindFirst/FindClose/fopen/fclose/TAFFmetadata만명시. fread/ftell/키장치는허용하지않아존재부모의payload미접근을검사. 세상위caller전체/OS I/O/SEH ABI는별도',
        sha256={str(p.relative_to(ROOT)): digest(p) for p in (ROOT / 'originals/Netstorm.exe', ROOT / 'extracted/refined/originals/functions.tsv')})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(rows=len(rows) - 1, observations=(len(rows) - 1) * 2)))


if __name__ == '__main__':
    main()
