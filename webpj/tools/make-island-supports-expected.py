# -*- coding: utf-8 -*-
"""
webpj 의 작은 받침 찾기(www/assets/island-supports.js) 검사용 기대값 표를 만든다.

JS 구현과 겹치지 않게 판독기 tools/fort.py(Python)와 영역 패턴 독립 자료(dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json)로 `.fort` 465개를 읽고, 원본이 요새를 읽으며 `noIsland` 칸을 만드는 순서
(Chaff 의 청크를 월드 y 바깥·x 안쪽으로, 이어서 영역 0~19 의 청크 레코드를 놓인 청크 순서로, 청크 안은 저장 순서)로 칸마다 글자를 다시 정한다:
서쪽 (x−1, y) 칸이 C→G, F→C, A→D, B→A, E→H, I→E 이면 그 글자, 아니면 북쪽 (x, y−1) 이 F→B, B→I 이면 그 글자, 그 밖에는 F. (이웃은 이미 만든 `noIsland` 칸만 본다)
글자가 F 인 칸이 받침 하나의 왼쪽 위 칸이고, 3 × 3 칸의 글자가 행 우선 `FCG/BAD/IEH` 이면 완전한 받침이다.
파일마다 한 줄: 출처 / noIsland 칸 수 / 받침 수(F 칸 수) / 불완전한 받침 수 / 같은 칸에 겹친 오브젝트 수 / 받침 목록(`x,y,저장 소유자,빠진 칸 수`를 줄바꿈으로 이은 글)의 SHA-256 / 소유자별 받침 수(`소유자:수,…`).
webpj/tests/island-supports.test.mjs 가 JS 의 결과를 줄 단위로 대조한다. www/ 구현의 출력으로 이 표를 다시 만들지 않는다.

사용법 (저장소 루트에서):
    python webpj/tools/make-island-supports-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/island-supports-expected.tsv]

(2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c) 작은 받침 검사를 위해 추가)
"""
import argparse
import collections
import glob
import hashlib
import importlib.util
import json
import os
import sys

# 저장소의 tools/ 폴더를 불러오기 경로에 넣는다(fort.py·taff.py 를 그대로 쓴다).
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import fort as fortpy  # noqa: E402
from taff import TaffArchive  # noqa: E402

# 영역 배치 독립 기대값 도구의 상수를 불러온다(파일 이름에 하이픈이 있어 importlib 로 읽는다).
_SPEC = importlib.util.spec_from_file_location("territory_expected", os.path.join(os.path.dirname(os.path.abspath(__file__)), "make-territory-expected.py"))
territory_tool = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(territory_tool)

# 받침 칸 타입 이름과 3 × 3 글자 배치이다.
NO_ISLAND = "noIsland"
ROWS = ["FCG", "BAD", "IEH"]
FROM_WEST = {"C": "G", "F": "C", "A": "D", "B": "A", "E": "H", "I": "E"}
FROM_NORTH = {"F": "B", "B": "I"}


def load_order(data, catalog, patterns, name):
    """`.fort` 한 개의 오브젝트를 원본이 만드는 순서로 [(월드 x, 월드 y, FortObject)] 로 돌려준다(영역 배치 도구와 같은 규칙)."""
    sections, _end = fortpy.split_sections(data)
    named = {fortpy.SECTION_NAMES[i]: s for i, s in enumerate(sections[:territory_tool.SECTION_LIMIT])}
    conv = catalog.conversion(named.get("TypeNames", b""))
    territory = named.get("Territory", b"")
    owner_of = {}
    # 영역 레코드를 번호순으로 놓는다. 나중에 놓은 영역이 겹친 청크를 차지한다.
    if len(territory) >= territory_tool.TERRITORY_COUNT * territory_tool.RECORD_BYTES:
        for index in range(territory_tool.TERRITORY_COUNT):
            shape_byte, flags, position = territory[index * 6:index * 6 + 3]
            if not (flags & 1) or (flags & 2):
                continue
            if shape_byte >> 6:
                raise AssertionError("%s: 회전된 영역은 독립 계산이 다루지 않는다" % name)
            pattern = patterns[shape_byte & 0x3F]
            for cell, side in enumerate(pattern["Cells"]):
                if side == ".":
                    continue
                x = min(territory_tool.ORIGIN + (position & 0xF) + cell % pattern["Width"], territory_tool.MAP_SIDE - 1)
                y = min(territory_tool.ORIGIN + (position >> 4) + cell // pattern["Width"], territory_tool.MAP_SIDE - 1)
                owner_of[(x, y)] = index
    ordered = []
    chaff = named.get("Chaff", b"")
    # Chaff: 청크 i 는 월드 청크 (i % 16, i ÷ 16) 이다.
    if len(chaff) > 1:
        for index, chunk in enumerate(fortpy.read_chunks(chaff, conv, catalog)[1]):
            cx, cy = index % territory_tool.MAP_SIDE, index // territory_tool.MAP_SIDE
            for obj in chunk:
                ordered.append((cx * 16 + obj.cell[0], cy * 16 + obj.cell[1], obj, -1))
    # 영역마다 저장된 청크 레코드를 놓인 청크(y 바깥, x 안쪽) 순서에 짝짓는다.
    for index in range(territory_tool.TERRITORY_COUNT):
        section = named.get("Terr%02d" % index, b"")
        stored = fortpy.read_chunks(section, conv, catalog)[1] if len(section) > 1 else []
        chunks = [(x, y) for y in range(territory_tool.MAP_SIDE) for x in range(territory_tool.MAP_SIDE) if owner_of.get((x, y)) == index]
        for (cx, cy), chunk in zip(chunks, stored):
            for obj in chunk:
                ordered.append((cx * 16 + obj.cell[0], cy * 16 + obj.cell[1], obj, index))
    return ordered


def describe(name, data, catalog, patterns):
    """`.fort` 한 개의 표 줄(탭으로 나눈 칸 목록)을 만든다."""
    letters = {}
    cells = 0
    duplicates = 0
    supports = []
    # 읽는 순서대로 noIsland 칸마다 글자를 정한다.
    for x, y, obj, _territory in load_order(data, catalog, patterns, name):
        if obj.type_name != NO_ISLAND:
            continue
        cells += 1
        west = letters.get((x - 1, y)) if x > 0 else None
        north = letters.get((x, y - 1)) if y > 0 else None
        if west in FROM_WEST:
            letter = FROM_WEST[west]
        elif north in FROM_NORTH:
            letter = FROM_NORTH[north]
        else:
            letter = "F"
        if (x, y) in letters:
            duplicates += 1
            continue
        letters[(x, y)] = letter
        if letter == "F":
            supports.append((x, y, obj.fields.get("owner")))
    lines = []
    incomplete = 0
    owners = collections.Counter()
    # 받침마다 3 × 3 글자가 FCG/BAD/IEH 인지 본다.
    for x, y, owner in supports:
        missing = sum(1 for dy in range(3) for dx in range(3) if letters.get((x + dx, y + dy)) != ROWS[dy][dx])
        incomplete += 1 if missing else 0
        owners["none" if owner is None else str(owner)] += 1
        lines.append("%d,%d,%s,%d" % (x, y, "none" if owner is None else owner, missing))
    digest = hashlib.sha256(("\n".join(lines) + "\n").encode("ascii")).hexdigest()
    return [name, str(cells), str(len(supports)), str(incomplete), str(duplicates), digest, ",".join("%s:%d" % (k, owners[k]) for k in sorted(owners))]


def main():
    """명령줄 인자를 읽어 모든 `.fort` 의 표를 쓴다."""
    parser = argparse.ArgumentParser(description="webpj 작은 받침 기대값 표 생성")
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"))
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "island-supports-expected.tsv"))
    args = parser.parse_args()
    with open(territory_tool.PATTERNS, encoding="utf-8") as handle:
        patterns = json.load(handle)
    catalog = fortpy.TypeCatalog(args.orig)
    rows = []
    # 느슨한 파일: d/*.fort 를 이름순으로.
    for path in sorted(glob.glob(os.path.join(args.orig, "d", "*.fort")), key=lambda p: os.path.basename(p).lower()):
        rows.append(describe("loose:" + os.path.basename(path), open(path, "rb").read(), catalog, patterns))
    archive = TaffArchive(os.path.join(args.orig, "netstorm.tarc"))
    # 아카이브 안 .fort 를 이름 인덱스 표 순서로.
    for entry in archive.entries:
        if entry.name.lower().endswith(".fort"):
            rows.append(describe("tarc:" + entry.name, archive.read(entry), catalog, patterns))
    header = [
        "# 작은 받침 기대값(파일마다 한 줄): 출처 / noIsland 칸 수 / 받침 수 / 불완전한 받침 수 / 같은 칸에 겹친 오브젝트 수 / 받침 목록(x,y,소유자,빠진 칸 수) SHA-256 / 소유자별 받침 수.",
        "# 생성: webpj/tools/make-island-supports-expected.py 가 tools/fort.py(Python 판독기)로 assets/game-data 를 읽어 원본의 칸 글자 규칙을 따로 구현해 만들었다(2026-11-02 WB03-Y).",
        "# 이 파일은 webpj/tests/island-supports.test.mjs 가 www/assets/island-supports.js 의 결과와 대조한다. www/ 구현의 출력으로 다시 만들지 않는다.",
    ]
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(header + ["\t".join(row) for row in rows]) + "\n")
    print("쓴 줄 수:", len(rows), "->", args.out)


if __name__ == "__main__":
    main()
