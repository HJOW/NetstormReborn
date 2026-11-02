#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""객체 덤프의 실제 빈prefix 작성과 성공한 재읽기/재쓰기 부모의 공유10칸을 관측한다."""
import argparse
import json
from pathlib import Path
import socket
import struct

from decomp_basefile_read_policy import Oracle as ReadOracle, OBJECT, PRIMARY, SECONDARY
from decomp_file_name_formatter import RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW


class DumpFileRingOracle(ReadOracle):
    """선택된 원문459c60/41b4c0 두부모만 CPU에서 실행하며 실제 파일/할당은 열지 않는다."""

    def instruction(self, mu, address, size, data):
        """일반 디스크 성공만 명시하며 mode0과mode2의 같은 경로/현재번호를 기록한다."""
        if address == 0x41a5b0:
            esp = mu.reg_read(UC_X86_REG_ESP)
            name, mode, create = self.words(esp + 4, 3)
            assert mu.reg_read(UC_X86_REG_ECX) == OBJECT and mode in (0, 2) and create == 0
            self.opens.append(dict(nameHex=self.string(name).hex(), mode=mode, index=self.words(INDEX, 1)[0]))
            self.ret(1, 12)
            return
        ReadOracle.instruction(self, mu, address, size, data)

    def invoke(self, address, arguments, precision, this=0):
        """원래 가변인자/thiscall 스택을 공급하고 정상 반환의 정확한 정리 폭을 확인한다."""
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1), STOP, *arguments))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.emu_start(address, STOP, count=30000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        expected = STACK + (4 if address == 0x459c60 else 16)
        assert self.mu.reg_read(UC_X86_REG_ESP) == expected and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        return self.mu.reg_read(UC_X86_REG_EAX)

    def snapshot(self):
        """현재번호와 첫NUL 뒤를 포함한 원문2560BYTE를 결과로 고정한다."""
        return dict(index=self.words(INDEX, 1)[0], ringHex=bytes(self.mu.mem_read(RING, 2560)).hex())

    def sample(self, root, initial, precision):
        """004d6020이 실제 선택하는 세 호출을 같은 고리/논리이름 주소로 연속 수행한다."""
        self.instructions = 0
        self.opens = []
        self.trace = []
        self.thrown = False
        self.mu.mem_write(STACK - 4096, bytes(8192))
        self.mu.mem_write(OBJECT, bytes(64))
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, initial)
        self.put(0, 0)
        self.put(0x532728, 92, 1)
        self.mu.mem_write(PRIMARY, bytes(256))
        self.mu.mem_write(SECONDARY, bytes(256))
        self.mu.mem_write(PRIMARY, root + b'\0')
        self.mu.mem_write(SECONDARY, b'D:\\disc\0')
        self.mu.mem_write(TEXT, b'\0')
        self.mu.mem_write(TEXT + 512, b'squids.txt\0')
        name = self.invoke(0x459c60, [0xfffffffe, TEXT, 0xfffffffd, TEXT + 512, 0], precision)
        logical = self.string(name)
        snapshots = [self.snapshot()]
        self.invoke(0x41b4c0, [name, 0, 0], precision, OBJECT)
        snapshots.append(self.snapshot())
        self.invoke(0x41b4c0, [name, 2, 0], precision, OBJECT)
        snapshots.append(self.snapshot())
        assert logical == b'.\\squids.txt' and self.string(name) == logical
        assert [row['index'] for row in snapshots] == [(initial + n) % 10 for n in (1, 3, 5)]
        return dict(rootHex=root.hex(), initial=initial, logicalHex=logical.hex(), snapshots=snapshots, opens=list(self.opens))


def main():
    """새 보고서만 쓰며 기존 최초fixture/PE/디컴파일 산출물은 변경하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    oracle = DumpFileRingOracle()
    rows = []
    for root in (b'C:\\Game', b'C:/Game', b'\\\\server\\share\\Game'):  # native·현대 slash·UNC의 원래 입력을 각각 공급한다.
        for initial in range(10):  # 모든 현재 고리 번호에서 동일 세 호출을 두 정밀도로 관측한다.
            result = oracle.sample(root, initial, 0x027f)
            assert result == oracle.sample(root, initial, 0x037f)
            rows.append(result)
    report = dict(unit=755, prepared=False, host=socket.gethostname(), rows=rows, observations=len(rows) * 2,
        boundary='실제459c60과41b4c0 mode0/mode2의정상기본디스크성공.004d6020전체부모/41baa0할당읽기/OS FILE/예외unwind는범위밖이며상대쓰기실패의TAFF/보조폴백은검사하지않음',
        gameExecutions=0, multiplayerTests=0,
        sha256={str(p.relative_to(ROOT)): digest(p) for p in (ROOT / 'originals/Netstorm.exe', ROOT / 'tools/decomp_basefile_read_policy.py')})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(rows=len(rows), observations=len(rows) * 2)))


if __name__ == '__main__':
    main()
