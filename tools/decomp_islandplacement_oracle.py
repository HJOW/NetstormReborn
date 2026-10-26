#!/usr/bin/env python3
"""편집기 CanPlace의 두 실제 PE 반환·진단·읽기 전용 저장소를 독립 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_islandbuilder_oracle import (IslandBuilderOracle, ROOT, TYPES, BUILDER, CUSTOM,
    CHUNK_BYTES, CODES, GEOMETRY, CONTROLS, frame_codes, digest)

# 기존 Place 관찰 자료는 바꾸지 않고 별도 fixture/report를 소유한다.
FIXTURE = ROOT / "cpppj/tests/fixtures/island-placement-x86.tsv"
REPORT = ROOT / "cpppj/island-placement.json"
ENTRIES = {"originals": 0x46de50, "originalCD": 0x490a50}


def cases():
    """모양·방향·우측/하단·충돌과 빈/누락 프레임을 기대 계산 없이 교차한다."""
    result = []
    # 위치 0은 완전 내부, 13/15는 원본 배타 우측/하단을 지난다.
    for shape, direction, offset, obstacle in itertools.product(range(64), range(4), (0, 13, 15), range(3)):
        result.append((shape | direction << 6, offset | offset << 4, 0, obstacle, 1, 1, 15, 15))
    # 빈/부분누락·역순프레임과 음수 왼쪽·현재폭보다 큰 범위를 별도 원본 입력한다.
    for shape, direction, profile, bounds in itertools.product((0, 3, 31, 63), range(4), (1, 2, 3),
            ((-1, -1, 15, 15), (-2, -2, 15, 15), (0, 0, 17, 17))):
        result.append((shape | direction << 6, 0, profile, 0, *bounds))
    return list(dict.fromkeys(result))


class PlacementOracle(IslandBuilderOracle):
    """현재 호스트 정밀 함수 표의 CanPlace와 기존 디코더 body만 추가 허용한다."""
    def __init__(self, edition):
        """기존 CPU/x87 장치를 재사용하고 이번 함수의 정확한 불연속 범위를 읽는다."""
        super().__init__(edition)
        self.s = dict(self.s)
        self.s["lines"] = dict(self.s["lines"])
        self.s["lines"][0xa8 if edition == "originals" else 0xa4] = "B"
        self.entry = ENTRIES[edition]
        export = ROOT / f"extracted/refined/{edition}/functions.tsv"
        with export.open(encoding="utf-8") as stream:
            selected = [row for row in csv.DictReader(stream, delimiter="\t") if int(row["entry"], 16) == self.entry]
        if len(selected) != 1:
            raise RuntimeError("현재 PC의 CanPlace 원문 범위 누락")
        self.entries.add(self.entry)
        for span in selected[0]["ranges"].split(";"):
            low, high = (int(value, 16) for value in span.split("-"))
            self.allowed[low-0x400000:high+1-0x400000] = b"\x01"*(high+1-low)
        self.exports.extend([export, ROOT/f"extracted/refined/{edition}/Netstorm.c"])

    def observe(self, values, control):
        """명시 지도·프레임·Territory를 준비하고 두 저장소가 불변인지 원문 실행으로 확인한다."""
        self.kind, self.values, self.events, self.assertions = "CanPlace", values, [], 0
        shape, offset, profile, obstacle, left, top, right, bottom = values
        chunk = bytearray(struct.pack("<III", 1, 16, 16) +
            (bytes((127, ord('.')))+b"\xa5"*6+struct.pack("<I", 0x12345678))*256)
        # 한 점 충돌과 전체 점유는 임의 입력이며 패턴의 유효 칸을 계산하지 않는다.
        for x in range(16):
            for y in range(16):
                if obstacle == 2 or (obstacle == 1 and x == y == 1):
                    chunk[12+(x*16+y)*12] = 0
        self.mu.mem_write(CUSTOM, bytes(chunk))
        self.call(self.s["constructor"], [], control)
        self.call(self.s["attach"], [CUSTOM], control)
        self.mu.mem_write(BUILDER+4, struct.pack("<4I", *[v & 0xffffffff for v in (left, top, right, bottom)]))
        self.put(self.spec["globals"]["types"], TYPES)
        for address, value in zip(GEOMETRY[self.edition]["patterns"], (107, 82, 94, 157, 131, 129, 140, 142), strict=True):
            self.put(address, value)
        codes = frame_codes(profile)
        if profile == 3:
            codes = b"".join(codes[i:i+4] for i in range(0, len(codes), 4) if codes[i] != ord('G'))
        typ = TYPES+107*self.type_stride
        self.mu.mem_write(typ, bytes(self.type_stride))
        self.put(typ+0x114, len(codes)//4)
        self.put(typ+0x124, CODES)
        self.mu.mem_write(CODES, bytes(4096))
        self.mu.mem_write(CODES, codes)
        if self.edition == "originals":
            self.put(0x5e4794, 0)
        before = bytes(self.mu.mem_read(CUSTOM, CHUNK_BYTES))+bytes(self.mu.mem_read(BUILDER, 24))
        result = self.call(self.entry, [shape | 0x0100 | offset << 16 | 0x55000000, 0xbbaa], control)
        after = bytes(self.mu.mem_read(CUSTOM, CHUNK_BYTES))+bytes(self.mu.mem_read(BUILDER, 24))
        if after != before:
            raise RuntimeError("CanPlace 원문이 입력 저장소를 변경함")
        self.returns += 1
        return result, ";".join(self.events) or "-"


def main():
    """기본 전체 생성과 무저장 표본 실행·현재 출처 감사를 구별한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding="utf-8"))
        rows = [line.split("\t") for line in FIXTURE.read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]
        if len(rows) != report["total"] or report["controls"] != list(CONTROLS):
            raise RuntimeError("CanPlace 입력 수 또는 x87 제어어 불일치")
        expected = {",".join(map(str, values)) for values in cases()}
        # 양판본의 실제 입력 집합과 두 번씩 정상 반환한 기록을 각각 확인한다.
        for edition, counts in report["editions"].items():
            selected = [row for row in rows if row[0] == edition]
            if {row[1] for row in selected} != expected or len(selected) != counts["unique"] or counts["returns"] != 2*len(selected):
                raise RuntimeError("CanPlace 판본별 입력 또는 반환 기록 불일치")
        # 모든 출처의 현재 바이트를 감사하며 오래된 fixture로 현재 제품을 통과시키지 않는다.
        for path, expected in report["files"].items():
            if digest(ROOT/path) != expected:
                raise RuntimeError(f"CanPlace 출처 불일치: {path}")
        print(f"CanPlace 출처감사 통과: {report['total']} 고유입력", flush=True)
        return
    rows, counts = [], {}
    sources = {Path(__file__), ROOT/"tools/decomp_islandbuilder_oracle.py",
        ROOT/"cpppj/src/o/RawIslandPlacement.h", ROOT/"cpppj/src/o/RawIslandPlacement.cpp",
        ROOT/"cpppj/tests/IslandPlacementTests.cpp"}
    # 재사용 디코더·CPU 초기화·패턴/청크 및 합성 입력 생성기의 출처도 함께 고정한다.
    for name in ("tools/decomp_owner_oracle.py", "tools/decomp_priestgeometry_oracle.py",
            "tools/decomp_canontype_oracle.py", "tools/decomp_damageablepredestroy_oracle.py",
            "cpppj/src/o/RawIslandBuilder.h", "cpppj/src/o/CanonDecoder.h", "cpppj/src/o/CanonDecoder.cpp",
            "cpppj/src/o/CanonDecoderTables.inc", "cpppj/src/o/RiftType.h", "cpppj/src/o/RiftType.cpp",
            "cpppj/src/o/ChunkMap.h", "cpppj/src/o/ChunkMap.cpp"):
        sources.add(ROOT/name)
    selected = cases()
    if args.smoke:
        selected = selected[:12]+selected[-12:]
    for edition in ENTRIES:
        oracle = PlacementOracle(edition)
        for values in selected:
            observed = [oracle.observe(values, control) for control in CONTROLS]
            if observed[0] != observed[1]:
                raise RuntimeError(f"CanPlace x87 출력 차이: {edition} {values}")
            rows.append((edition, ",".join(map(str, values)), *observed[0]))
        counts[edition] = {"unique": len(selected), "returns": oracle.returns, "native_calls": dict(oracle.native)}
        sources.update(oracle.exports)
        sources.add(ROOT/oracle.spec["binary"])
        print(f"{edition}: CanPlace {len(selected)} 고유입력 / {oracle.returns} 정상 반환", flush=True)
    if args.smoke:
        return
    FIXTURE.write_bytes(("# 실제 두 PE CanPlace: 입력/반환/진단; 지도·builder 전체 불변 확인\n"+
        "\n".join("\t".join(map(str, row)) for row in rows)+"\n").encode("utf-8"))
    sources.add(FIXTURE)
    report = {"host": "HJOW-Athlon", "date": "2026-10-26", "total": len(rows), "controls": list(CONTROLS),
        "editions": counts, "game_runs": 0, "os_calls": 0,
        "limits": ["원문 CanPlace의 읽기 전용 판정만 완료; 실제 AddIsland GUI·배치·지형·저장은 별도 미완료다."],
        "files": {path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(sources)}}
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2)+"\n").encode("utf-8"))


if __name__ == "__main__":
    main()
