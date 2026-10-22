#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 맵 사각형에서 다리 시작점을 고르는 실제00410b10/0040faa0을 격리 검증한다."""
import itertools
import json
import socket
import struct

from decomp_ai_bridge_candidates import CandidateOracle, ROOT, BODY, STACK, STOP, SIZE, POOL, SURFACES, SPOTS, TYPES, FRAMES, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW

ENTRY = 0x410B10  # 실제 시작점 선택의 thiscall 진입점이다.
OUTPUT = BODY + 0x1000  # 전체 Brain 바깥의 caller 결과8BYTE다.
FIXTURE = ROOT / "cpppj/tests/fixtures/ai-bridge-start-x86.tsv"  # 새 독립 입력/출력 동결이다.
REPORT = ROOT / "cpppj/ai-bridge-start-20261022.json"  # 출처와 실행 범위다.
RANGES = ((0x410B10, 0x410D40), (0x40FAA0, 0x40FB28), (0x4AD390, 0x4AD3DC), (0x4AD680, 0x4AD6BE), (0x49A840, 0x49A853), (0x4E49C0, 0x4E4A35))  # 계산 스텁 없는 허용 명령이다.


def bits(value):
    """caller float를 원문DWORD로 보존한다."""
    return struct.unpack("<I", struct.pack("<f", value))[0]


class StartOracle(CandidateOracle):
    """실제 graph/현재 프레임/연결/CRT까지 실행하고 게임은 시작하지 않는다."""

    def instruction(self, mu, address, size, data):
        """실제 방문 수/하위 진입을 기록하고 검토 범위 밖은 거부한다."""
        if address == 0x410BD0:
            self.visits += 1
        if address in (0x40FAA0, 0x4AD390, 0x4AD680, 0x49A840, 0x4E49C0):
            self.entries[hex(address)] = self.entries.get(hex(address), 0) + 1
        if not any(low <= address < high for low, high in RANGES):
            raise RuntimeError(f"검토 범위 밖 명령 {address:08x}")

    def sample(self, values, precision):
        """순수 caller 입력을 설치하고 결과XY/전체본문 불변/ret12/보존 레지스터를 확인한다."""
        mode, kind, side, left, top, right, bottom, tx, ty, graph, owner, bridge = values
        body = bytearray([0xA5]) * SIZE
        struct.pack_into("<I", body, 0x298, graph)
        struct.pack_into("<I", body, 0x260, owner)
        self.mu.mem_write(BODY, bytes(body))
        self.mu.mem_write(0x544D14, struct.pack("<4I", left, top, right, bottom))
        self.mu.mem_write(0x5411A0, struct.pack("<I", bridge))
        surface, spots = bytearray(0x20000), bytearray(65536)  # 결과와 무관한 현재 지도 입력이다.
        for y in range(256):  # 실제 flat 지도의 입력만 채운다.
            for x in range(256):
                index = y*256+x
                if mode and (mode != 3 or (x + 3*y) % 5):
                    struct.pack_into("<H", surface, index*2, 50)
                if mode == 2:
                    spots[index] = 0x80
                elif mode == 3 and (x + 3*y) % 5 == 0:
                    spots[index] = 0x40
        self.mu.mem_write(SURFACES, bytes(surface))
        self.mu.mem_write(SPOTS - 0x1000, bytes(0x12000))
        self.mu.mem_write(SPOTS, bytes(spots))
        source = bytearray(50)
        source[10], source[30], source[34] = kind, 7, 2
        self.mu.mem_write(POOL + 50*50, bytes(source))
        record = bytearray(500)
        struct.pack_into("<I", record, 0xE8, 0x800)
        struct.pack_into("<I", record, 0x124, FRAMES)
        self.mu.mem_write(TYPES + kind*500, bytes(record))
        self.mu.mem_write(FRAMES, bytes((side, 65, 0, 0)))
        self.mu.mem_write(OUTPUT, bytes([0x5A]) * 8)
        self.mu.mem_write(STACK, struct.pack("<4I", STOP, OUTPUT, tx, ty))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, BODY)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        for register, value in ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_EBP, 0x456789AB), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789A)):
            self.mu.reg_write(register, value)
        self.mu.emu_start(ENTRY, STOP, count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 16
        assert self.mu.reg_read(UC_X86_REG_EAX) == OUTPUT and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        for register, value in ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_EBP, 0x456789AB), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789A)):
            assert self.mu.reg_read(register) == value
        assert bytes(self.mu.mem_read(BODY, SIZE)) == body
        assert bytes(self.mu.mem_read(SURFACES, len(surface))) == surface
        assert bytes(self.mu.mem_read(SPOTS, len(spots))) == spots
        assert bytes(self.mu.mem_read(POOL + 50*50, 50)) == source
        return bytes(self.mu.mem_read(OUTPUT, 8)).hex()


def main():
    """연결A~P·다리/육지·지도·거리동률·분수경계·DWORD비교를 두 정밀도로 동결한다."""
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle = StartOracle()
    bounds = tuple(map(bits, (49.75, 61.5, 55.25, 67.5)))
    cases = []
    for mode, kind, side, target in itertools.product(range(4), (82, 97), range(65, 81), ((52.9, 64.1), (48.9, 60.1), (60.9, 72.9))):  # 모든연결과순회동률을교차한다.
        cases.append((mode, kind, side, *bounds, *map(bits, target), 7, 2, 82))
    for graph, owner, bridge in ((0x107,2,82),(7,0x102,82),(7,2,0x152),(0,2,82),(7,0,82)):  #BYTE로축소하지않는현재전역/본체비교다.
        cases.append((1,82,ord('A'),*bounds,bits(52),bits(64),graph,owner,bridge))
    for boundary in ((55.5,61.5,55.25,67.5),(49.75,68.1,55.25,67.5),(49.75,61.5,float('nan'),67.5),(49.75,61.5,55.25,float('nan')),(-1.5,-1.5,0.5,0.5),(256,256,258,258),(float('nan'),61,0,62)):
        cases.append((0,97,ord('P'),*map(bits,boundary),bits(52),bits(64),7,2,82))
    for target in ((-1.5,-0.5),(float('nan'),float('inf')),(2147483648.0,-2147483648.0)):
        cases.append((1,97,ord('P'),*bounds,*map(bits,target),7,2,82))
    outputs = []
    for values in cases:  # 기대XY를 계산하지 않고 실제 명령의 결과만 채택한다.
        observed = [oracle.sample(values, precision) for precision in (0x27F, 0x37F)]
        assert observed[0] == observed[1]
        outputs.append(observed[0])
    with FIXTURE.open("x", encoding="utf-8", newline="\n") as stream:
        stream.write("# 실제00410b10:지도/타입/코드/경계4/목표2/graph/owner/bridge/결과XY\n")
        stream.writelines("\t".join(map(str, (*values, output))) + "\n" for values, output in zip(cases, outputs))
    report = {"host": socket.gethostname(), "decompile_host": "VM-W11-Codex", "cases": len(cases), "normal_returns": len(cases)*2, "x87": [0x27F,0x37F], "cells": oracle.visits, "helpers": oracle.entries, "stubs": [], "game_started": False, "sources": {str(path.relative_to(ROOT)): digest(path) for path in (ROOT/"originals/Netstorm.exe",ROOT/"originalCD/NETSTORM.EXE",ROOT/"tools/decomp_ai_bridge_start.py",FIXTURE)}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"cases":report["cases"],"returns":report["normal_returns"],"cells":report["cells"]}))


if __name__ == "__main__":
    main()
