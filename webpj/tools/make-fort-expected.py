# -*- coding: utf-8 -*-
"""
webpj 의 `.fort` 판독기(www/assets/fort.js) 검사용 기대값 표를 만든다.

기준은 독립 구현인 tools/fort.py(Python 판독기)이다. www/ 구현의 출력으로 이 표를 다시 만들지 않는다.
각 `.fort` 파일의 머리·구독자·Storm Power·기술/덱 개수·청크 영역별 청크 수와 오브젝트 수, 그리고 모든 저장 오브젝트(위치·타입·선택 필드·내용물)를
정해진 한 줄 서식으로 이은 글의 SHA-256 을 한 줄에 적는다. webpj/tests/fort.test.mjs 가 JS 판독기의 같은 서식 결과와 줄 단위로 대조한다.

사용법 (저장소 루트에서):
    python webpj/tools/make-fort-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/fort-expected.tsv]

(2026-11-01 WB03-U: `.fort` 판독기 검사를 위해 추가)
"""
import argparse
import glob
import hashlib
import os
import struct
import sys

# 저장소의 tools/ 폴더를 불러오기 경로에 넣는다(fort.py·taff.py·shp.py 를 그대로 쓴다).
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import fort as fortpy  # noqa: E402
from taff import TaffArchive  # noqa: E402

# 로더가 읽는 섹션 수이다(원본 이름 목록 35개, 그 뒤 섹션은 읽지 않는다).
SECTION_LIMIT = 35


def fmt_contents(items):
    """내용물 목록을 `이름[:qa=값][:qb=값][:ct=[…]]` 항목을 쉼표로 이은 글로 만든다. qb 는 부호 없는 16비트로 적는다."""
    parts = []
    # 목록의 항목마다 이름과 선택 필드, 중첩 내용물을 이어 붙인다.
    for item in items:
        text = item["type"]
        if "qa" in item:
            text += ":qa=%d" % item["qa"]
        if "qb" in item:
            text += ":qb=%d" % item["qb"]
        if "contents" in item:
            text += ":ct=[%s]" % fmt_contents(item["contents"])
        parts.append(text)
    return ",".join(parts)


def fmt_object(obj):
    """청크 안 오브젝트 하나를 `x,y|타입|필드…` 서식 한 줄로 만든다. 필드 순서는 고정이다."""
    f = obj.fields
    text = "%d,%d|%s" % (obj.cell[0], obj.cell[1], obj.type_name)
    # 고정 순서로 있는 선택 필드만 적는다.
    for key, label in (("frame", "f"), ("qa", "qa"), ("qb", "qb"), ("bridge", "br"), ("factory", "fa"), ("owner", "ow"), ("skip", "sk")):
        if key in f:
            text += "|%s=%d" % (label, f[key])
    if "contents" in f:
        text += "|ct=[%s]" % fmt_contents(f["contents"])
    return text


def describe(data, catalog):
    """`.fort` 한 파일의 표 한 줄(탭으로 나눈 칸 목록, 이름 칸 제외)을 만든다."""
    sections, _end = fortpy.split_sections(data)
    sections = sections[:SECTION_LIMIT]
    # 35번째 섹션 끝의 위치이다(그 뒤 바이트는 읽지 않는 꼬리이다).
    end = 2 + sum(len(s) + 2 for s in sections)
    named = {fortpy.SECTION_NAMES[i]: s for i, s in enumerate(sections)}
    lines = []
    # 구독자: 소유자 ID(4바이트) + 이름(길이 2바이트 + 바이트).
    sub = named.get("Subscriber", b"")
    sub_id, sub_name = "-", "-"
    if len(sub) >= 6:
        sub_id = str(struct.unpack_from("<I", sub, 0)[0])
        length, = struct.unpack_from("<H", sub, 4)
        sub_name = sub[6:6 + length].hex() or "-"
    # Storm Power: 32비트 실수의 바이트를 그대로 16진수로 적는다.
    money = named.get("Money", b"")
    money_text = money[:4].hex() if len(money) >= 4 else "-"
    conv = catalog.conversion(named.get("TypeNames", b""))
    # 기술 목록.
    technology = fortpy.read_technology(named.get("Technology", b""), conv, catalog)
    lines.append("tech %d" % len(technology))
    # 기술 항목마다 저장 타입 번호·이름·수량·목록 플래그·내용물을 적는다.
    for item in technology:
        text = "T|%d|%s" % (item["typeNumber"], item["type"])
        for key in ("qa", "qb"):
            if key in item:
                text += "|%s=%d" % (key, item[key])
        text += "|lf=%d" % item["listFlags"]
        if "contents" in item:
            text += "|ct=[%s]" % fmt_contents(item["contents"])
        lines.append(text)
    # 덱: 개수 + (타입, chance, power(부호 있음), 남은 횟수) 4바이트 항목.
    deck = named.get("Deck", b"")
    deck_count = 0
    if deck:
        deck_count = deck[0]
        # 항목마다 변환한 타입 이름(모르면 '-')과 세 값을 적는다.
        for i in range(deck_count):
            stored, chance, power, remaining = struct.unpack_from("<BBbB", deck, 1 + i * 4)
            lines.append("D|%s|%d|%d|%d" % (conv.get(stored, "-"), chance, power, remaining))
    counts = []
    # 청크가 있는 섹션(Chaff, Terr00~19)을 이름 순서로 해석한다.
    for name in fortpy.SECTION_NAMES:
        section = named.get(name, b"")
        if not (name == "Chaff" or (name.startswith("Terr") and name[4:].isdigit())) or len(section) <= 1:
            continue
        version, chunks = fortpy.read_chunks(section, conv, catalog)
        lines.append("S|%s|v%d|%d" % (name, version, len(chunks)))
        total = 0
        # 청크마다 오브젝트 수와 오브젝트 줄을 적는다.
        for index, objects in enumerate(chunks):
            lines.append("C|%d|%d" % (index, len(objects)))
            total += len(objects)
            for obj in objects:
                lines.append(fmt_object(obj))
        counts.append("%s=%d/%d" % (name, len(chunks), total))
    digest = hashlib.sha256(("\n".join(lines) + "\n").encode("ascii")).hexdigest()
    return [
        "%02x" % data[1], str(len(sections)), str(len(data) - end), sub_id, sub_name, money_text,
        str(len(technology)), str(deck_count), ",".join(counts) or "-", digest,
    ]


def main():
    """명령줄 인자를 읽어 모든 `.fort` 의 표를 쓴다."""
    parser = argparse.ArgumentParser(description="webpj .fort 판독기 기대값 표 생성")
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"))
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "fort-expected.tsv"))
    args = parser.parse_args()
    catalog = fortpy.TypeCatalog(args.orig)
    rows = []
    # 느슨한 파일: d/*.fort 를 이름순으로.
    for path in sorted(glob.glob(os.path.join(args.orig, "d", "*.fort")), key=lambda p: os.path.basename(p).lower()):
        rows.append(["loose:" + os.path.basename(path)] + describe(open(path, "rb").read(), catalog))
    archive = TaffArchive(os.path.join(args.orig, "netstorm.tarc"))
    # 아카이브 안 .fort 를 이름 인덱스 표 순서로.
    for entry in archive.entries:
        if entry.name.lower().endswith(".fort"):
            rows.append(["tarc:" + entry.name] + describe(archive.read(entry), catalog))
    header = [
        "# `.fort` 기대값(칸: 출처 / 둘째 바이트 / 읽은 섹션 수 / 읽지 않는 꼬리 바이트 / 구독자 ID / 구독자 이름(16진수) / Money(float32 16진수) / 기술 항목 수 / 덱 항목 수 / 청크 영역별 청크 수·오브젝트 수 / 전체 저장 내용의 SHA-256).",
        "# 생성: webpj/tools/make-fort-expected.py 가 tools/fort.py(Python, JS 판독기와 독립 구현)의 결과로 assets/game-data 를 읽어 만들었다(2026-11-01 WB03-U).",
        "# 이 파일은 webpj/tests/fort.test.mjs 가 www/assets/fort.js 의 판독 결과와 대조한다. www/ 구현의 출력으로 다시 만들지 않는다.",
    ]
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(header) + "\n")
        # 파일마다 한 줄을 쓴다.
        for row in rows:
            handle.write("\t".join(row) + "\n")
    print("쓴 줄 수:", len(rows), "->", args.out)


if __name__ == "__main__":
    main()
