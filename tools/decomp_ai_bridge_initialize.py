#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본0040fde0과 실제 CRT 절삭을 격리 실행해 다리 탐색 초기화 전체 본체를 동결한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소 루트다.
BODY = 0x10000000  # 실제0xb08 본체의 격리 주소다.
STACK = 0x11008000  # caller의 원본 thiscall 인자를 보관하는 스택이다.
STOP = 0x12000000  # 정상 ret16을 확인하는 종료 주소다.
ENTRY = 0x40FDE0  # 이번에 복원할 실제 초기화 함수다.
SIZE = 0xB08  # 상위 Brain이 소유한 전체 메모리 폭이다.
FIXTURE = ROOT / "cpppj/tests/fixtures/ai-bridge-initialize-x86.tsv"  # 기존 관측을 덮지 않는 새 전체 본체 기대다.
REPORT = ROOT / "cpppj/ai-bridge-initialize-20261022.json"  # 원본·도구·관측의 출처를 보존한다.


def digest(path):
    """지정 파일의 실제 SHA-256을 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class BridgeInitializeOracle:
    """게임/OS/창 없이 초기화와 원본__ftol 명령만 실행한다."""

    def __init__(self):
        """보호 PE·전체 본체·스택을 읽기 전용 원본 바이트로 구성한다."""
        pe = pefile.PE(str(ROOT / "originals/Netstorm.exe"))  # 원본은 파일 읽기만 수행한다.
        image = pe.get_memory_mapped_image()  # 실제 섹션의 메모리 배치다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 격리된32비트 명령 실행기다.
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase, (len(image) + 4095) & ~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, image)
        self.mu.mem_map(BODY, 0x2000)
        self.mu.mem_map(STACK - 0x8000, 0x10000)
        self.mu.mem_map(STOP, 0x1000)
        self.calls = 0  # 실제 CRT 진입 횟수다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def instruction(self, mu, address, size, data):
        """기존 정밀 범위 밖 실행을 거부하고 CRT 몸체는 대체하지 않는다."""
        if address == 0x4E49C0:
            self.calls += 1
        if not (ENTRY <= address <= 0x40FE82 or 0x4E49C0 <= address <= 0x4E4A34):
            raise RuntimeError(f"검토 범위 밖 명령 {address:08x}")

    def sample(self, source, target, available, pattern, precision):
        """입력 비트를 쓰고 전체 본체·ABI 보존·정상 반환을 관측한다."""
        self.calls = 0
        self.mu.mem_write(BODY, bytes([pattern]) * SIZE)
        self.mu.mem_write(BODY + 0x628, struct.pack("<I", available))
        self.mu.mem_write(STACK, struct.pack("<5I", STOP, *source, *target))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, BODY + 0x5F0)
        self.mu.reg_write(UC_X86_REG_EBX, 0x12345678)
        self.mu.reg_write(UC_X86_REG_ESI, 0x23456789)
        self.mu.reg_write(UC_X86_REG_EDI, 0x3456789A)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        self.mu.emu_start(ENTRY, STOP, count=3000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 20
        assert self.mu.reg_read(UC_X86_REG_EAX) == 1 and self.calls == 4
        assert self.mu.reg_read(UC_X86_REG_EBX) == 0x12345678
        assert self.mu.reg_read(UC_X86_REG_ESI) == 0x23456789
        assert self.mu.reg_read(UC_X86_REG_EDI) == 0x3456789A
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        return bytes(self.mu.mem_read(BODY, SIZE)).hex()


def main():
    """좌표·signed 경계·count 감김과 두 x87 정밀도의 전체 본체를 독립 동결한다."""
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle = BridgeInitializeOracle()  # 원본 몸체와 실제 CRT만 사용한다.
    points = ((0x412C0000, 0x41A7FDF4), (0, 0x80000000), (0xBF800000, 0xBE800000), (0x3F000000, 0x437FFFFF), (1, 0x80000001), (0x4F000000, 0xCF000000), (0x5EFFFFFF, 0xDF000000), (0x5F000000, 0xDF000001), (0x7F800000, 0xFF800000), (0x7FC12345, 0xFFC12345), (0xC12C0000, 0xC1A7FDF4), (0x3F7FFFFF, 0xBF7FFFFF))  # caller가 전달하는 원문 float 비트다.
    targets = ((0x41F00000, 0x42200000), (0xBF000000, 0xC12C0000), (0x7FC00000, 0x7F800000))  # 정상·음수·비유한 목표 입력이다.
    counts = (0, 1, 0x80000000, 0xFFFFFFFF)  # 마지막 후보 인덱스의 signed 감김 경계다.
    rows = []  # C++ 결과를 읽지 않은 독립 관측 행이다.
    # 각 caller 입력을 두 x87 정밀도에서 실행해 차이가 없음을 먼저 확인한다.
    for index, (source, target, available) in enumerate(itertools.product(points, targets, counts)):
        pattern = (index * 73 + 0xA5) & 255  # 덮지 않은 주변 바이트가 각 행에서 보존되는지 확인하는 입력이다.
        first = oracle.sample(source, target, available, pattern, 0x27F)
        second = oracle.sample(source, target, available, pattern, 0x37F)
        assert first == second
        rows.append((*source, *target, available, pattern, first))
    FIXTURE.write_text("# sourceX sourceY targetX targetY available pattern fullBodyHex\n" + "\n".join("\t".join(map(str, row)) for row in rows) + "\n", encoding="utf-8", newline="\n")
    report = dict(host=socket.gethostname(), lastDecompile="VM-W11-Codex / 2026-10-22", entry="0040fde0", crt="004e49c0", inputs=len(rows), normalReturns=len(rows)*2, bytesCompared=SIZE*len(rows)*2, sha256={str(path.relative_to(ROOT)): digest(path) for path in (ROOT/"originals/Netstorm.exe", Path(__file__), FIXTURE)}, limitations=["초기화와 실제 CRT 절삭의 전체 본체만 검증한다. 후보 준비/탐색/자동 AI와 사용자 플레이는 미완료다.", "게임·OS·창을 실행하지 않는다."])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"원본 다리 초기화 {len(rows)}입력/{len(rows)*2}정상 반환·전체 본체 대조")


if __name__ == "__main__":
    main()
