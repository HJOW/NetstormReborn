#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 모양·점유·끝점 평가·8개 예산 검색의 실제 x86 반환과 전체 바이트를 동결한다."""
import collections
import csv
import hashlib
import json
import random
import socket
import struct

from decomp_bridge_oracle import BridgeOracle, ROOT, OBJECT, MAP, GLOBALS, codes, float_bits
from decomp_oracle import STACK

ENTRIES = {"Shape": 0x40F400, "Clear": 0x40F880, "Score": 0x40F910, "Search": 0x4144B0, "Side": 0x425C00}  # 실제 검토한 하위 함수다.
FIXTURES = ROOT / "cpppj/tests/fixtures"  # 원본 관측만 기록하며 C++ 기대를 계산하지 않는다.
REPORT = ROOT / "cpppj/ai-bridge-search-20261022.json"  # 보호 원본/도구/실행 범위의 출처다.
SHAPE, PLACEMENT, RESULT = OBJECT + 0x1000, OBJECT + 0x2000, OBJECT + 0x2100  # 겹치지 않는 caller 버퍼다.


def put(raw, offset, value):
    """caller의 원문 DWORD를 인접 바이트 보존으로 설치한다."""
    struct.pack_into("<I", raw, offset, value & 0xFFFFFFFF)


def digest(path):
    """현재 파일의 SHA-256을 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class SearchOracle(BridgeOracle):
    """게임·OS·assert 대체 없이 검토된 실제 decoder/CRT/난수까지 실행한다."""

    def __init__(self):
        """기존 decoder 실행기에 다섯 실제 몸체와 caller 버퍼 쓰기만 추가한다."""
        super().__init__("originals")
        self.stubs = {}
        with (ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8") as stream:
            facts = {int(row["entry"], 16): row for row in csv.DictReader(stream, delimiter="\t")}
        for name, entry in ENTRIES.items():  # 현재 호스트의 정밀 범위만 허용 목록에 더한다.
            self.entries[name] = entry
            for part in facts[entry]["ranges"].split(";"):
                low, high = [int(word, 16) for word in part.split("-")]
                self.allowed.append((low, high + 1))
        self.write_ranges.append((OBJECT, OBJECT + 0x2200))
        self.frames = codes("originals", "bridge")
        self.setup_type(self.frames)
        self.mu.mem_write(0x5C7C44, struct.pack("<I", MAP))
        self.visits = collections.Counter()

    def on_instruction(self, mu, address, size, data):
        """실제 하위 진입 횟수를 기록하며 허용 밖 명령은 부모가 거부한다."""
        if address in self.entries.values():
            self.visits[address] += 1
        super().on_instruction(mu, address, size, data)

    def shape(self, pattern, direction, fill):
        """초기 grid/미사용 끝점 바이트를 caller 입력으로 보존해 전체0xbc를 읽는다."""
        self.mu.mem_write(SHAPE, bytes([fill]) * 0xBC)
        self.call("Shape", [pattern, direction], SHAPE, 8)
        return bytes(self.mu.mem_read(SHAPE, 0xBC))

    def clear(self, pattern, direction, xbits, ybits, mode):
        """비대칭 점유 지도와 실제 CanonDecoder를 공급해 첫 점유 종료를 관측한다."""
        spots = bytes((1 if mode == 1 else (0x80 if (x + 3*y) % 11 == 0 else 0) if mode == 2 else (0x40 if x == 50 else 0) if mode == 3 else 0) for y in range(256) for x in range(256))
        self.mu.mem_write(MAP, spots)
        self.mu.mem_write(PLACEMENT, struct.pack("<4I", xbits, ybits, pattern, direction))
        return self.call("Clear", [], PLACEMENT)

    def score(self, shape, placement, target, excluded, fill):
        """실제 x87 중간 정밀도/절삭·방향 벌점·미갱신 출력 바이트를 읽는다."""
        self.mu.mem_write(SHAPE, shape)
        self.mu.mem_write(PLACEMENT, struct.pack("<4I", *placement))
        self.mu.mem_write(RESULT, bytes([fill]) * 8)
        score = self.call("Score", [*target, SHAPE, excluded, RESULT], PLACEMENT, 20)
        return score, bytes(self.mu.mem_read(RESULT, 8)).hex()

    def search(self, raw, state, scratch, calls, mode):
        """같은 검색 본체를 재개하며 각 호출의 실제8개 예산/난수/임시 출력을 보존한다."""
        self.mu.mem_write(OBJECT, bytes(raw))
        self.mu.mem_write(GLOBALS["originals"][5], struct.pack("<I", state))
        spots = bytes(0x80 if mode == 1 else (0x40 if (x + 3*y) % 11 == 0 else 0) if mode == 2 else 0 for y in range(256) for x in range(256))
        self.mu.mem_write(MAP, spots)
        observed = []
        for _ in range(calls):  # 동일 입력의 다음 호출에도 갱신된 실제 본체/난수를 사용한다.
            self.mu.mem_write(0x30000000, bytes([0xA5]) * 0x10000)
            self.mu.mem_write(STACK - 0xD0, scratch)
            complete = self.call("Search", [], OBJECT + 0x5F0)
            scratch = bytes(self.mu.mem_read(STACK - 0xD0, 8))
            state = struct.unpack("<I", self.mu.mem_read(GLOBALS["originals"][5], 4))[0]
            observed.append((complete, state, scratch.hex(), bytes(self.mu.mem_read(OBJECT, 0xB08)).hex()))
        return observed


def main():
    """모양104×미초기화2종·점유416·경계 평가·검색 재개의 원문만 새 파일에 쓴다."""
    paths = [FIXTURES / f"ai-bridge-{name}-x86.tsv" for name in ("shape", "clear", "score", "search")]
    assert not REPORT.exists() and all(not path.exists() for path in paths)
    oracle = SearchOracle()
    shapes, clears, scores, searches = [], [], [], []  # 각 행은 입력과 실제 native 결과를 나란히 보존한다.
    for pattern in range(26):  # 보호 PE의 실제26개 다리 모양을 모두 순회한다.
        for direction in (0, 2, 4, 6):  # 원본 검색의 네 회전 순서다.
            for fill in (0, 0xA5):  # helper가 지우지 않는 caller grid와 끝점 여백도 검사한다.
                raw = oracle.shape(pattern, direction, fill)
                shapes.append((pattern, direction, fill, raw.hex()))
            for mode in range(4):  # 빈 지도·전체 점유·비대칭 간격·단일 세로열 입력이다.
                xbits, ybits = float_bits(49.75), float_bits(63.25)
                clears.append((pattern, direction, xbits, ybits, mode, oracle.clear(pattern, direction, xbits, ybits, mode)))
    rng = random.Random(0x4144B0)  # 원본 난수와 별개인 재현 가능한 caller 입력 생성기다.
    points = ((50.25, 63.75), (50.999996, 63.000004), (16777216.0, -16777216.0), (-0.5, 0.5), (0.0, -0.0))  # x와 y의 서로 다른 float 저장 경계다.
    for index in range(640):  # 동률·최소6 벌점·거리2 금지·각 방향/대각선과 큰 좌표를 공급한다.
        pattern, direction = index % 26, (index // 26 % 4) * 2
        shape = oracle.shape(pattern, direction, 0)
        placement = (*[float_bits(value) for value in points[index % len(points)]], pattern, direction)
        target = (rng.choice((0, 48, 50, 51, 52, 53, 56, 61, 63, 64, 65, 70, 0x7FFFFFFF, 0x80000000)), rng.choice((0, 48, 50, 51, 52, 53, 56, 61, 63, 64, 65, 70, 0x7FFFFFFF, 0x80000000)))
        excluded, fill = rng.choice((0, 1, 2, 0xFFFFFFFF)), 0xA5
        score, output = oracle.score(shape, placement, target, excluded, fill)
        scores.append((pattern, direction, *placement[:2], *target, excluded, fill, score, output))
    for index in range(72):  # 최대8개 재고/6개 후보·잔여 회전·예산 재개·공유 난수 입력이다.
        raw = bytearray([0x5A]) * 0xB08
        available, count = index % 9, (index // 9 % 6) + 1
        for slot in range(8):  # 실제 caller의 다리 재고8DWORD만 채운다.
            put(raw, 0x608 + slot*4, (index*7 + slot*3) % 26)
        for slot in range(6):  # 입력 연결점만 채우고 결과 후보는 계산하지 않는다.
            for offset, value in enumerate((50 + slot*7, 63 + slot*3, (index + slot) % 4*2)):
                put(raw, 0x648 + slot*12 + offset*4, value)
        for offset, value in ((0x628, available), (0x640, (30, 50, 80)[index % 3]), (0x644, (35, 63, 90)[index // 3 % 3]), (0x690, count), (0x6A8, 0x10000), (0x6BC, 0xC61C4000), (0x6C0, 0xC61C4000), (0x6C4, (0, 2, 6, 8)[index % 4]), (0x6C8, available+1 if index % 5 == 0 else (available-1)), (0x6CC, 1 if index % 7 == 0 else 0)):
            put(raw, offset, value)
        state, scratch, mode = (0, 1, 0xFFFFFFFF)[index % 3], bytes.fromhex("00001cc600001cc6"), index // 24
        observed = oracle.search(raw, state, scratch, 4, mode)
        searches.append((index, mode, state, scratch.hex(), raw.hex(), ";".join(",".join(map(str, row)) for row in observed)))
    for path, rows in zip(paths, (shapes, clears, scores, searches)):  # 기존 관측을 덮지 않고 UTF-8로 쓴다.
        with path.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write("# 실제10.78 x86 명령의 caller 입력과 반환/전체 버퍼 관측\n")
            stream.writelines("\t".join(map(str, row)) + "\n" for row in rows)
    report = {"host": socket.gethostname(), "decompile_host": "VM-W11-Codex", "original_sha256": digest(ROOT / "originals/Netstorm.exe"), "cd_sha256": digest(ROOT / "originalCD/NETSTORM.EXE"), "tool_sha256": digest(ROOT / "tools/decomp_ai_bridge_search.py"), "frames_hex": oracle.frames.hex(), "rows": dict(zip(("shape", "clear", "score", "search"), map(len, (shapes, clears, scores, searches)))), "search_returns": sum(len(row[5].split(";")) for row in searches), "actual_entries": {hex(address): count for address, count in sorted(oracle.visits.items())}, "stubs": [], "game_started": False, "fixture_sha256": {path.name: digest(path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"rows": report["rows"], "returns": report["search_returns"], "stubs": report["stubs"]}))


if __name__ == "__main__":
    main()
