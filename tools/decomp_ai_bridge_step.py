#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본10.78의004167a0만 격리 실행하여 다리 탐색 단계의 독립 입력을 만든다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]  # 저장소 루트다.
BODY = 0x10000000  # 원본0xb08 본체의 격리 주소다.
SOURCE = BODY + 0x2000  # caller가 소유한 결과 XY다.
STACK = 0x11008000  # 격리 호출 스택이다.
STOP = 0x12000000  # 원본 정상 반환을 확인하는 주소다.
ENTRY = 0x4167A0  # 실행 대상인 원본 단계 함수다.
BOUNDARIES = {0x40FDE0: ("initialize", 16), 0x40FE90: ("prepare", 0), 0x4144B0: ("search", 0)}  # 명시 하위 경계와 thiscall 스택 정리 폭이다.


def digest(path):
    """보호 PE·도구·fixture의 실제 바이트 SHA를 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class BridgeStepOracle:
    """단계 분배 명령만 실행하며 하위 탐색의 완료/상태 변경은 입력으로 공급한다."""

    def __init__(self):
        """PE를 읽기 전용으로 매핑하고 실제 명령 범위를 제한한다."""
        self.pe = pefile.PE(str(ROOT / "originals/Netstorm.exe"))  # 실행하지 않고 매핑할 보호 PE다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # OS/게임 창이 없는 격리 x86 실행기다.
        base = self.pe.OPTIONAL_HEADER.ImageBase  # 실제 PE 이미지 기준 주소다.
        image = self.pe.get_memory_mapped_image()  # 원본 섹션 바이트다.
        self.mu.mem_map(base, (len(image) + 4095) & ~4095)
        self.mu.mem_write(base, image)
        self.mu.mem_map(BODY, 0x4000)
        self.mu.mem_map(STACK - 0x8000, 0x10000)
        self.mu.mem_map(STOP, 0x1000)
        self.events = []  # 실제 하위 진입 순서다.
        self.mode = 0  # 하위 false/true/현재 상태변경의 명시 입력이다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def put(self, address, value):
        """지정 DWORD의 raw 비트를 쓴다."""
        self.mu.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))

    def read(self, address):
        """현재 raw DWORD를 읽는다."""
        return struct.unpack("<I", self.mu.mem_read(address, 4))[0]

    def instruction(self, mu, address, size, data):
        """허용한 단계 함수와 명시 하위 이외의 실행을 거부한다."""
        if address in BOUNDARIES:
            name, purge = BOUNDARIES[address]
            self.events.append(name)
            assert mu.reg_read(UC_X86_REG_ECX) == BODY + 0x5F0
            sp = mu.reg_read(UC_X86_REG_ESP)
            if name == "initialize":
                assert tuple(self.read(sp + offset) for offset in (4, 8, 12, 16)) == (0x41200000, 0x41A00000, 0x41F00000, 0x42200000)
            if self.mode == 2:
                self.put(BODY + 0x5F0, 0xFFFFFFFF)
            mu.reg_write(UC_X86_REG_EAX, int(self.mode != 0))
            mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
            mu.reg_write(UC_X86_REG_EIP, self.read(sp))
            return
        if not ENTRY <= address < 0x41683E:
            raise RuntimeError(f"검토 범위 밖 명령 {address:08x}")

    def sample(self, state, wanted, available, mode):
        """입력 본체를 구성하고 원본 반환·상태·출력·하위 순서를 읽는다."""
        self.events = []
        self.mode = mode
        self.mu.mem_write(BODY, bytes([0xA5]) * 0xB08)
        self.put(BODY + 0x5F0, state)
        self.put(BODY + 0x604, wanted)
        self.put(BODY + 0x628, available)
        self.put(BODY + 0x6BC, 0x42CA0000)
        self.put(BODY + 0x6C0, 0x434A0000)
        self.put(SOURCE, 0x41200000)
        self.put(SOURCE + 4, 0x41A00000)
        self.mu.mem_write(STACK, struct.pack("<IIII", STOP, SOURCE, 0x41F00000, 0x42200000))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, BODY + 0x5F0)
        self.mu.emu_start(ENTRY, STOP, count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 16
        return (self.read(BODY + 0x5F0), self.mu.reg_read(UC_X86_REG_EAX), self.read(SOURCE), self.read(SOURCE + 4), ",".join(self.events) or "-")


def main():
    """signed 경계/한 단계 전이/하위 뒤 현재 상태 증가를 독립 fixture에 보존한다."""
    oracle = BridgeStepOracle()
    cases = list(itertools.product((0, 1, 2, 3, 4, 5, 6, 0xFFFFFFFF), (0, 1, 8, 0x80000000, 0xFFFFFFFF), (0, 7, 8, 0x7FFFFFFF, 0xFFFFFFFF), (0, 1, 2)))  # 입력만 열거하며 기대 결과는 계산하지 않는다.
    rows = []  # 실제 원본 반환을 저장한다.
    # 각 입력은 새 본체/스택에서 독립 실행한다.
    for values in cases:
        rows.append((*values, *oracle.sample(*values)))
    fixture = ROOT / "cpppj/tests/fixtures/ai-bridge-step-x86.tsv"  # C++가 재생하는 실제 원본 출력이다.
    fixture.write_text("# state wanted available mode after complete x y events\n" + "\n".join("\t".join(map(str, row)) for row in rows) + "\n", encoding="utf-8", newline="\n")
    report = dict(host=socket.gethostname(), samples=len(rows), normalReturns=len(rows), entry="004167a0", sha256=digest(ROOT / "originals/Netstorm.exe"), fixtureSha256=digest(fixture), toolSha256=digest(Path(__file__)), limitations=["단계 분배의 실제 x86 명령만 실행한다. 초기화/후보 준비/탐색은 명시 하위 입력이다.", "원본 게임·OS·창을 실행하지 않는다. 실제 경로 검색과 사용자 플레이 동일성은 검증하지 않는다."])
    (ROOT / "cpppj/ai-bridge-step-20261022.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"원본 단계 함수 {len(rows)}개 입력·정상 반환 확인")


if __name__ == "__main__":
    main()
