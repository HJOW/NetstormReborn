# -*- coding: utf-8 -*-
"""
webpj 의 영역 배치(www/assets/territory.js) 검사용 기대값 표를 만든다.

기준은 JS 구현과 겹치지 않는 두 자료이다: 판독기 tools/fort.py(Python)와, 실행 파일에서 따로 뽑은 영역 패턴 자료
dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json(방향 글자만 쓴다). www/ 구현의 출력으로 이 표를 다시 만들지 않는다.
파일마다 `FORT 출처` 줄, 영역마다 `T …` 줄(모양·방향·플래그·위치·저장된 청크 수·놓인 청크 좌표와 방향 글자), 마지막에 `O 개수 SHA-256` 줄을 적는다.
`O` 줄은 Chaff 의 청크 i 를 월드 청크 (i % 16, i ÷ 16) 로, 영역 청크 레코드를 놓인 청크의 y 바깥·x 안쪽 순서에 짝지어 모든 저장 오브젝트를
`월드x,월드y|타입` 으로 이은 글의 해시이다(월드 칸 = 청크 × 16 + 청크 안 칸). webpj/tests/territory.test.mjs 가 JS 의 결과를 줄 단위로 대조한다.

사용법 (저장소 루트에서):
    python webpj/tools/make-territory-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/territory-expected.tsv]

(2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계 영역 배치 검사를 위해 추가)
"""
import argparse
import glob
import hashlib
import json
import os
import sys

# 저장소의 tools/ 폴더를 불러오기 경로에 넣는다(fort.py·taff.py 를 그대로 쓴다).
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import fort as fortpy  # noqa: E402
from taff import TaffArchive  # noqa: E402

# 영역 패턴 독립 자료(tools/territory_patterns.py 가 실행 파일에서 뽑은 것)이다.
PATTERNS = os.path.join(ROOT, "dotnetpj", "src", "Netstorm.Assets", "TerritoryPatterns.json")
# 청크 지도의 한 변, 영역 수, 레코드 크기, 영역을 놓는 범위의 원점(원본 FUN_00431fc0 의 (1,1)), 청크 한 변의 칸 수이다.
MAP_SIDE = 16
TERRITORY_COUNT = 20
RECORD_BYTES = 6
ORIGIN = 1
CELLS = 16
# 로더가 읽는 섹션 수이다(원본 이름 목록 35개).
SECTION_LIMIT = 35


def describe(name, data, catalog, patterns):
    """`.fort` 한 개의 줄 목록(`FORT` 줄, `T` 줄들, `O` 줄)을 만든다."""
    sections, _end = fortpy.split_sections(data)
    named = {fortpy.SECTION_NAMES[i]: s for i, s in enumerate(sections[:SECTION_LIMIT])}
    lines = ["FORT " + name]
    conv = catalog.conversion(named.get("TypeNames", b""))
    territory = named.get("Territory", b"")
    owner_of = {}   # (x, y) → 영역 번호
    side_of = {}    # (x, y) → 방향 글자
    # 레코드를 번호순으로 놓는다. 나중에 놓은 영역이 겹친 청크를 차지한다.
    if len(territory) >= TERRITORY_COUNT * RECORD_BYTES:
        for index in range(TERRITORY_COUNT):
            shape_byte, flags, position = territory[index * RECORD_BYTES:index * RECORD_BYTES + 3]
            if not (flags & 1) or (flags & 2):
                continue
            if shape_byte >> 6:
                raise AssertionError("%s: 회전된 영역(방향 %d)은 독립 계산이 다루지 않는다" % (name, shape_byte >> 6))
            pattern = patterns[shape_byte & 0x3F]
            # 패턴 칸을 줄 우선으로 훑는다. '.' 은 빈 칸이다.
            for cell, side in enumerate(pattern["Cells"]):
                if side == ".":
                    continue
                x = min(ORIGIN + (position & 0xF) + cell % pattern["Width"], MAP_SIDE - 1)
                y = min(ORIGIN + (position >> 4) + cell // pattern["Width"], MAP_SIDE - 1)
                owner_of[(x, y)] = index
                side_of[(x, y)] = side
    objects = []
    # Chaff: 청크 i 는 월드 청크 (i % 16, i ÷ 16) 이다.
    chaff = named.get("Chaff", b"")
    if len(chaff) > 1:
        for index, chunk in enumerate(fortpy.read_chunks(chaff, conv, catalog)[1]):
            cx, cy = index % MAP_SIDE, index // MAP_SIDE
            for obj in chunk:
                objects.append("%d,%d|%s" % (cx * CELLS + obj.cell[0], cy * CELLS + obj.cell[1], obj.type_name))
    # 영역마다 한 줄: 레코드 값, 저장된 청크 레코드 수, 놓인 청크(y 바깥, x 안쪽).
    for index in range(TERRITORY_COUNT):
        section = named.get("Terr%02d" % index, b"")
        stored = fortpy.read_chunks(section, conv, catalog)[1] if len(section) > 1 else []
        chunks = [(x, y) for y in range(MAP_SIDE) for x in range(MAP_SIDE) if owner_of.get((x, y)) == index]
        if not chunks and not stored:
            continue
        # 레코드가 없는 파일(120바이트 미만)은 값을 0 으로 적는다.
        shape_byte, flags, position = territory[index * RECORD_BYTES:index * RECORD_BYTES + 3] if len(territory) >= TERRITORY_COUNT * RECORD_BYTES else (0, 0, 0)
        if chunks and len(chunks) != len(stored):
            raise AssertionError("%s 영역 %d: 놓인 청크 %d개, 저장된 청크 레코드 %d개" % (name, index, len(chunks), len(stored)))
        lines.append("T %d shape=%d dir=%d flags=%d pos=%d,%d stored=%d chunks=%s" % (
            index, shape_byte & 0x3F, shape_byte >> 6, flags, position & 0xF, position >> 4, len(stored),
            "".join("%d,%d%s;" % (x, y, side_of[(x, y)]) for x, y in chunks)))
        # 저장된 청크 레코드 k 는 놓인 k 번째 청크의 것이다.
        for (cx, cy), chunk in zip(chunks, stored):
            for obj in chunk:
                objects.append("%d,%d|%s" % (cx * CELLS + obj.cell[0], cy * CELLS + obj.cell[1], obj.type_name))
    digest = hashlib.sha256(("\n".join(objects) + "\n").encode("ascii")).hexdigest()
    lines.append("O %d %s" % (len(objects), digest))
    return lines


def main():
    """명령줄 인자를 읽어 모든 `.fort` 의 표를 쓴다."""
    parser = argparse.ArgumentParser(description="webpj 영역 배치 기대값 표 생성")
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"))
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "territory-expected.tsv"))
    args = parser.parse_args()
    with open(PATTERNS, encoding="utf-8") as handle:
        patterns = json.load(handle)
    catalog = fortpy.TypeCatalog(args.orig)
    rows = []
    # 느슨한 파일: d/*.fort 를 이름순으로.
    for path in sorted(glob.glob(os.path.join(args.orig, "d", "*.fort")), key=lambda p: os.path.basename(p).lower()):
        rows += describe("loose:" + os.path.basename(path), open(path, "rb").read(), catalog, patterns)
    archive = TaffArchive(os.path.join(args.orig, "netstorm.tarc"))
    # 아카이브 안 .fort 를 이름 인덱스 표 순서로.
    for entry in archive.entries:
        if entry.name.lower().endswith(".fort"):
            rows += describe("tarc:" + entry.name, archive.read(entry), catalog, patterns)
    header = [
        "# 영역 배치 기대값(파일마다 `FORT 출처`, 영역마다 `T 번호 shape 방향 플래그 위치 저장청크수 놓인청크(x,y+방향글자;…)`, 끝에 `O 오브젝트수 SHA-256`).",
        "# 생성: webpj/tools/make-territory-expected.py 가 tools/fort.py(Python 판독기)와 dotnetpj 의 영역 패턴 자료(JS 구현과 독립)로 assets/game-data 를 읽어 만들었다(2026-11-01 WB03-V).",
        "# 이 파일은 webpj/tests/territory.test.mjs 가 www/assets/territory.js 의 배치 결과와 대조한다. www/ 구현의 출력으로 다시 만들지 않는다.",
    ]
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(header + rows) + "\n")
    print("쓴 줄 수:", len(rows), "->", args.out)


if __name__ == "__main__":
    main()
