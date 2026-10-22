#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본0040fe90의 나선24칸·최대6개 후보를 실제 그래프/프레임/빈 이웃 helper와 실행한다."""
import itertools
import json
import socket
import struct
from pathlib import Path

from decomp_ai_bridge_initialize import BridgeInitializeOracle, ROOT, BODY, STACK, STOP, SIZE, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP

ENTRY = 0x40FE90  # 실제 후보 준비의 thiscall 진입점이다.
POOL = 0x20000000  # 원본50BYTE 슬롯들의 격리 주소다.
SURFACES = 0x21000000  # 원본 hash0의65536 WORD 입력이다.
SPOTS = 0x22001000  # 원본65536 BYTE 점유 입력이며 앞뒤 보호 바이트도 매핑한다.
TYPES = 0x23000000  # 실제500stride TYPE 레코드 입력이다.
FRAMES = 0x24000000  # source50의 현재4BYTE 프레임 코드다.
FIXTURE = ROOT / "cpppj/tests/fixtures/ai-bridge-candidates-x86.tsv"  # 새 독립 전체 본체 기대값이다.
REPORT = ROOT / "cpppj/ai-bridge-candidates-20261022.json"  # 원본/도구/관측 범위를 보존한다.
RANGES = ((0x40FE90, 0x4100F4), (0x40FA60, 0x40FA9E), (0x4AD390, 0x4AD3DC), (0x4AD680, 0x4AD6BE), (0x49A840, 0x49A853))  # 실제 준비·빈 이웃·그래프·현재 코드·TYPE 조회 명령만 허용한다.


class CandidateOracle(BridgeInitializeOracle):
    """게임/OS/창 없이 현재 입력 지도를 원본 준비 함수가 직접 읽게 한다."""

    def __init__(self):
        """기존 원본 매핑에 실제 지도·풀·TYPE 입력을 추가한다."""
        super().__init__()
        self.mu.mem_map(POOL, 0x200000)
        self.mu.mem_map(SURFACES, 0x20000)
        self.mu.mem_map(SPOTS - 0x1000, 0x12000)
        self.mu.mem_map(TYPES, 0x18000)
        self.mu.mem_map(FRAMES, 0x1000)
        self.visits = 0  # 실제 후보 셀 처리 횟수다.
        self.entries = {}  # 실제 하위 helper별 호출 횟수다.
        # 원본 getter가 사용하는 현재 전역 포인터만 명시 입력 주소에 연결한다.
        for address, value in ((0x5C8464, POOL), (0x5C84BC, SURFACES), (0x5C7C44, SPOTS), (0x59AB20, TYPES), (0x5411A0, 82), (0x5411C0, 90), (0x541288, 140)):
            self.mu.mem_write(address, struct.pack("<I", value))

    def instruction(self, mu, address, size, data):
        """허용한 실제 helper를 대체하지 않고 호출·24칸 예산 진입만 관측한다."""
        if address == 0x40FF2C:
            self.visits += 1
        if address in (0x40FA60, 0x4AD390, 0x4AD680, 0x49A840):
            self.entries[hex(address)] = self.entries.get(hex(address), 0) + 1
        if not any(low <= address < high for low, high in RANGES):
            raise RuntimeError(f"검토 범위 밖 명령 {address:08x}")

    def sample(self, values):
        """명시 입력으로 시작해 전체 본체/반환/지도 불변/ABI를 기록한다."""
        mode, kind, side, x, y, stage, count, direction, length, graph, owner, calls, pattern = values
        self.visits = 0
        self.entries = {}
        self.mu.mem_write(BODY, bytes([pattern]) * SIZE)
        # state+8/c는 캐시할 그래프/owner이고 a0 이후는 현재 나선 입력이다.
        for offset, value in ((0x5F8, graph), (0x5FC, owner), (0x690, count), (0x694, direction), (0x698, stage), (0x69C, length), (0x6A0, x), (0x6A4, y)):
            self.mu.mem_write(BODY + offset, struct.pack("<I", value & 0xFFFFFFFF))
        surface = bytearray(0x20000)  # 입력 모드가 정한 현재 hash0이다.
        spots = bytearray(65536)  # 입력 모드가 정한 현재 점유 바이트다.
        # 기대값을 만들지 않고 지형/점유 입력만 행·열 순서로 작성한다.
        for cy in range(256):
            for cx in range(256):
                present = mode != 0 and (mode != 4 or (cx == x and cy == y))
                if mode == 3:
                    present = (cx + cy) % 3 != 0
                if present:
                    struct.pack_into("<H", surface, (cy * 256 + cx) * 2, 50)
                if mode == 2:
                    spots[cy * 256 + cx] = 1
                elif mode == 3:
                    spots[cy * 256 + cx] = 0x80 if (cx + cy) % 3 == 0 else 0
                elif mode == 5:
                    spots[cy * 256 + cx] = 0x40 if (cx == x and cy == y - 1) or (cx == x + 1 and cy == y) else 0
        self.mu.mem_write(SURFACES, bytes(surface))
        self.mu.mem_write(SPOTS, bytes(spots))
        source = bytearray(50)  # source50은 동일 입력 지도에서 실제 getter가 읽는 슬롯이다.
        source[10], source[30], source[34] = kind, 7, 2
        self.mu.mem_write(POOL + 50 * 50, bytes(source))
        record = bytearray(500)  # 명시 source TYPE의 surface flag와 실제 코드 포인터다.
        struct.pack_into("<I", record, 0xE8, 0x800)
        struct.pack_into("<I", record, 0x124, FRAMES)
        self.mu.mem_write(TYPES + kind * 500, bytes(record))
        self.mu.mem_write(FRAMES, bytes((side, 65, 0, 0)))
        result = 0  # 마지막 실제 원본 반환만 기록한다.
        # 같은 본체를 다음 호출도 이어 읽어 예산 중단 뒤 재개를 확인한다.
        for repeat in range(calls):
            self.mu.mem_write(STACK, struct.pack("<I", STOP))
            self.mu.reg_write(UC_X86_REG_ESP, STACK)
            self.mu.reg_write(UC_X86_REG_ECX, BODY + 0x5F0)
            self.mu.reg_write(UC_X86_REG_EBX, 0x12345678)
            self.mu.reg_write(UC_X86_REG_EBP, 0x456789AB)
            self.mu.reg_write(UC_X86_REG_ESI, 0x23456789)
            self.mu.reg_write(UC_X86_REG_EDI, 0x3456789A)
            self.mu.emu_start(ENTRY, STOP, count=50000)
            assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert self.mu.reg_read(UC_X86_REG_EBX) == 0x12345678 and self.mu.reg_read(UC_X86_REG_EBP) == 0x456789AB
            assert self.mu.reg_read(UC_X86_REG_ESI) == 0x23456789 and self.mu.reg_read(UC_X86_REG_EDI) == 0x3456789A
            result = self.mu.reg_read(UC_X86_REG_EAX)
        assert bytes(self.mu.mem_read(SURFACES, len(surface))) == surface
        assert bytes(self.mu.mem_read(SPOTS, len(spots))) == spots
        assert bytes(self.mu.mem_read(POOL + 50 * 50, 50)) == source
        return result, bytes(self.mu.mem_read(BODY, SIZE)).hex(), self.visits


def main():
    """연결 A~P/다리·육지/지도·집계·signed 경계/예산 재개를 새 독립 fixture로 동결한다."""
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle = CandidateOracle()  # 원본 입력 전용 실행기다.
    cases = []  # 모든 값은 caller/지도 입력이며 기대값은 계산하지 않는다.
    # 여섯 지도 모드와 다리/육지의16방향 코드를 모두 교차한다.
    for mode, kind, side in itertools.product(range(6), (82, 97), range(65, 81)):
        cases.append((mode, kind, side, 50, 50, 2, 0, 0, 1, 7, 2, 1, 0xA5))
    # 실제 조기 종료·최대6개·진행 예산·DWORD 비교의 경계를 독립 입력으로 추가한다.
    for x, y, stage, count, direction, length, graph, owner, calls in ((10,50,2,0,0,1,7,2,1),(11,50,2,0,0,1,7,2,1),(245,50,2,0,0,1,7,2,1),(246,50,2,0,0,1,7,2,1),(50,10,2,0,0,1,7,2,1),(50,246,2,0,0,1,7,2,1),(50,50,24,0,0,1,7,2,1),(50,50,8,3,0,1,7,2,1),(50,50,7,3,0,1,7,2,1),(50,50,2,5,2,1,7,2,1),(50,50,2,6,4,1,7,2,1),(50,50,2,0xffffffff,6,1,7,2,1),(50,50,2,0,0,25,7,2,1),(50,50,2,0,0,0,7,2,1),(50,50,2,0,0,1,0x107,2,1),(50,50,2,0,0,1,7,0x102,1),(50,50,2,0,0,1,7,2,3),(50,50,2,0,0,1,7,2,9)):
        cases.append((1, 82, 65, x, y, stage, count, direction, length, graph, owner, calls, 0x71))
    rows = []  # 원본 전체 본체 결과다.
    returns = 0  # 실제 원본 함수의 정상 반환 횟수다.
    visited = 0  # 실제 검사한 셀 수다.
    helpers = {}  # 실제 helper 호출 합계다.
    # caller별 시작 상태를 분리하고 마지막 결과를 원문으로 기록한다.
    for values in cases:
        result, body, visits = oracle.sample(values)
        rows.append((*values, result, body, visits))
        returns += values[11]
        visited += visits
        for name, count in oracle.entries.items():
            helpers[name] = helpers.get(name, 0) + count
    FIXTURE.write_text("# mode type side x y stage count direction length graph owner calls pattern result fullBodyHex visits\n" + "\n".join("\t".join(map(str, row)) for row in rows) + "\n", encoding="utf-8", newline="\n")
    report = dict(host=socket.gethostname(), lastDecompile="VM-W11-Codex / 2026-10-22", entry="0040fe90", scenarios=len(rows), normalReturns=returns, visitedCells=visited, helpers=helpers, sha256={str(path.relative_to(ROOT)):digest(path) for path in (ROOT/"originals/Netstorm.exe", Path(__file__), ROOT/"tools/decomp_ai_bridge_initialize.py", FIXTURE)}, limitations=["후보 준비와 실제 그래프/프레임/빈 이웃/TYPE getter의 전체 본체를 검증한다. 경로 검색·자동 AI·플레이는 미완료다.", "게임·OS·창을 실행하지 않는다."])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"원본 다리 후보 {len(rows)}장면/{returns}정상 반환/{visited}칸 관측")


if __name__ == "__main__":
    main()
