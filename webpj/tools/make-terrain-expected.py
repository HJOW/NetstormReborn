# -*- coding: utf-8 -*-
"""
webpj 의 본섬 칸 마스크(www/assets/terrain-mask.js) 검사용 기대값 표를 만든다.

기준은 JS 구현과 겹치지 않는 독립 재현 도구 tools/terrain_mask.py(원본 Terrainbuilder 디컴파일·어셈블리에서 따로 옮긴 Python)의 generate_mask 이다.
영역 레코드는 tools/fort.py 로 읽고 영역 패턴은 dotnetpj 의 패턴 JSON(실행 파일에서 추출)을 쓴다. www/ 구현의 출력으로 이 표를 다시 만들지 않는다.
파일마다 한 줄: 출처 / 마스크 65,536바이트(인덱스 = y × 256 + x, 빈 칸 255)의 SHA-256 / 지면 칸 수 / 영역별 칸 수(`번호:칸수,…`).
webpj/tests/terrain.test.mjs 가 JS 의 buildIslandMask 결과를 줄 단위로 대조한다.

사용법 (저장소 루트에서, Pillow 필요):
    python webpj/tools/make-terrain-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/terrain-expected.tsv]

(2026-11-01 WB03-W: 정적 맵 화면(M2) 3단계의 첫 부분 검사를 위해 추가)
"""
import argparse
import glob
import hashlib
import json
import os
import sys

# 저장소의 tools/ 폴더를 불러오기 경로에 넣는다(terrain_mask.py·fort.py·taff.py 를 그대로 쓴다).
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import terrain_mask  # noqa: E402
import fort as fortpy  # noqa: E402
from taff import TaffArchive  # noqa: E402

# 영역 패턴 독립 자료와 로더가 읽는 섹션 수, 영역 레코드 섹션의 크기이다.
PATTERNS = os.path.join(ROOT, "dotnetpj", "src", "Netstorm.Assets", "TerritoryPatterns.json")
SECTION_LIMIT = 35
TERRITORY_BYTES = 120


def describe(source, data, patterns):
    """`.fort` 한 개의 표 한 줄(탭으로 나눈 칸 목록)을 만든다. 영역 레코드가 120바이트에 못 미치면 영역이 없는 것으로 본다."""
    sections, _end = fortpy.split_sections(data)
    named = {fortpy.SECTION_NAMES[i]: s for i, s in enumerate(sections[:SECTION_LIMIT])}
    rows = named.get("Territory", b"")
    rows = bytes(rows[:TERRITORY_BYTES]) if len(rows) >= TERRITORY_BYTES else bytes(TERRITORY_BYTES)
    land = terrain_mask.generate_mask(rows, patterns)
    counts = {}
    # 지면 칸을 영역 번호별로 센다(빈 칸 255 는 센다 제외).
    for value in land:
        if value != terrain_mask.EMPTY:
            counts[value] = counts.get(value, 0) + 1
    return [source, hashlib.sha256(bytes(land)).hexdigest(), str(sum(counts.values())), ",".join("%d:%d" % (k, counts[k]) for k in sorted(counts)) or "-"]


def main():
    """명령줄 인자를 읽어 모든 `.fort` 의 표를 쓴다."""
    parser = argparse.ArgumentParser(description="webpj 본섬 칸 마스크 기대값 표 생성")
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"))
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "terrain-expected.tsv"))
    args = parser.parse_args()
    with open(PATTERNS, encoding="utf-8") as handle:
        patterns = json.load(handle)
    rows = []
    # 느슨한 파일: d/*.fort 를 이름순으로.
    for path in sorted(glob.glob(os.path.join(args.orig, "d", "*.fort")), key=lambda p: os.path.basename(p).lower()):
        rows.append(describe("loose:" + os.path.basename(path), open(path, "rb").read(), patterns))
    archive = TaffArchive(os.path.join(args.orig, "netstorm.tarc"))
    # 아카이브 안 .fort 를 이름 인덱스 표 순서로.
    for entry in archive.entries:
        if entry.name.lower().endswith(".fort"):
            rows.append(describe("tarc:" + entry.name, archive.read(entry), patterns))
    header = [
        "# 본섬 칸 마스크 기대값(칸: 출처 / 마스크 65,536바이트(y × 256 + x, 빈 칸 255)의 SHA-256 / 지면 칸 수 / 영역별 칸 수 `번호:칸수,…`).",
        "# 생성: webpj/tools/make-terrain-expected.py 가 tools/terrain_mask.py(원본에서 따로 옮긴 Python 재현, JS 구현과 독립)로 assets/game-data 를 읽어 만들었다(2026-11-01 WB03-W).",
        "# 이 파일은 webpj/tests/terrain.test.mjs 가 www/assets/terrain-mask.js 의 결과와 대조한다. www/ 구현의 출력으로 다시 만들지 않는다.",
    ]
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(header) + "\n")
        # 파일마다 한 줄을 쓴다.
        for row in rows:
            handle.write("\t".join(row) + "\n")
    print("쓴 줄 수:", len(rows), "->", args.out)


if __name__ == "__main__":
    main()
