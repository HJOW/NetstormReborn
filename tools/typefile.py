# -*- coding: utf-8 -*-
"""
.type (오브젝트 정의 스크립트) 파서 및 수치표 생성 도구

포맷 명세: docs/formats/type.md

사용법:
    python tools/typefile.py json  [--orig originals] [--out extracted/types.json]
    python tools/typefile.py table [--orig originals] [--out docs/gameplay/types.md]
"""
import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field, asdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from shp import TYPE_LOAD_ORDER, read_clusters, load_type_texts  # noqa: E402

# 주석 (// 부터 줄 끝까지)
COMMENT_RE = re.compile(r"//[^\n]*")

# 머리 줄: typename <이름> [constructor|client constructor]
TYPENAME_RE = re.compile(r"typename\s+(\S+)([^\n]*)")

# 플래그 줄: typeflags <플래그...> ;
TYPEFLAGS_RE = re.compile(r"typeflags\s+([^;]*);")

# 속성 블록 { ... }
BODY_RE = re.compile(r"\{(.*?)\}", re.S)

# 속성 한 개: 키 = 값 ;
PROPERTY_RE = re.compile(r"(\w+)\s*=\s*([^;]*);")

# 수치표에 넣을 속성 (표 머리글, 속성 키)
TABLE_COLUMNS = [
    ("설명", "description"), ("class", "class"), ("theme", "theme"), ("level", "level"),
    ("cost", "cost"), ("HP", "maxHitPoints"), ("range", "range"), ("hpPerSec", "hpPerSec"),
    ("delayBetweenShots", "delayBetweenShots"), ("constructionRate", "constructionRate"),
    ("speed", "speed"), ("foot", "foot"), ("manacost", "manacost"), ("casttime", "casttime"),
    ("threat", "threat"),
]


@dataclass
class TypeDef:
    """.type 파일 하나의 해석 결과"""
    name: str                                   # typename
    constructor: str = ""                       # typename 뒤의 수식어 (constructor / client constructor)
    flags: list = field(default_factory=list)   # typeflags
    props: dict = field(default_factory=dict)   # { } 안의 속성 (키는 원문 대소문자 유지)
    clusters: list = field(default_factory=list)  # [(이름, 플래그, [(gif, 번호), ...]), ...]
    load_index: int = -1                        # 로딩 순서 (= _shapes.shp 블록 번호), 없으면 -1


def parse_value(raw: str):
    """속성 값 문자열을 숫자/문자열로 변환한다 (따옴표 제거, 정수·실수 인식)"""
    v = raw.strip()
    if len(v) >= 2 and v[0] == '"' and v[-1] == '"':
        return v[1:-1]
    try:
        return int(v)
    except ValueError:
        pass
    try:
        return float(v)
    except ValueError:
        return v


def parse_type(text: str) -> TypeDef:
    """.type 텍스트를 해석한다"""
    clean = COMMENT_RE.sub("", text)
    m = TYPENAME_RE.search(clean)
    td = TypeDef(m.group(1) if m else "?", (m.group(2).strip() if m else ""))
    # typename 과 같은 줄에 typeflags 가 붙은 경우도 있으므로 전체에서 검색
    if "typeflags" in td.constructor:
        td.constructor = ""
    m = TYPEFLAGS_RE.search(clean)
    if m:
        td.flags = m.group(1).split()
    m = BODY_RE.search(clean)
    if m:
        # 속성을 하나씩 읽는다
        for k, v in PROPERTY_RE.findall(m.group(1)):
            td.props[k] = parse_value(v)
    td.clusters = read_clusters(text)
    return td


def get_prop(td: TypeDef, key: str):
    """대소문자를 무시하고 속성 값을 찾는다 (원본에 hotfootratiox 처럼 대소문자가 섞여 있음)"""
    low = key.lower()
    # 속성 목록에서 소문자 비교로 찾는다
    for k, v in td.props.items():
        if k.lower() == low:
            return v
    return None


def load_all(orig: str) -> list:
    """아카이브의 모든 .type 을 해석해 로딩 순서대로 돌려준다 (로딩 목록에 없는 타입은 뒤에 붙인다)"""
    texts = load_type_texts(orig)
    order = {n.lower(): i for i, n in enumerate(TYPE_LOAD_ORDER)}
    result = []
    # 타입 텍스트마다 해석
    for key, text in texts.items():
        td = parse_type(text)
        td.load_index = order.get(key, -1)
        result.append(td)
    result.sort(key=lambda t: (t.load_index < 0, t.load_index, t.name.lower()))
    return result


def cmd_json(args):
    """전체 타입 정의를 JSON 으로 저장"""
    types = load_all(args.orig)
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump([asdict(t) for t in types], f, ensure_ascii=False, indent=1)
    print(f"{len(types)}개 타입 → {args.out}")


def cmd_table(args):
    """클래스(class)가 있는 타입(플레이어가 다루는 건물·유닛·주문)의 수치표를 Markdown 으로 저장"""
    types = [t for t in load_all(args.orig) if get_prop(t, "class") is not None]
    types.sort(key=lambda t: (str(get_prop(t, "theme") or "~"), str(get_prop(t, "class")),
                              get_prop(t, "level") or 0, t.name.lower()))
    lines = [
        "# 오브젝트 수치표 (자동 생성)",
        "",
        "> `python tools/typefile.py table` 로 생성. 직접 수정하지 말 것.",
        "> 출처: `netstorm.tarc` 의 `.type` 파일 (패치판 10.7x 기준). 속성 의미는 [type.md](../formats/type.md) 참고.",
        "",
        "| 타입 | " + " | ".join(h for h, _ in TABLE_COLUMNS) + " | 플래그 |",
        "|---|" + "---|" * len(TABLE_COLUMNS) + "---|",
    ]
    # 타입마다 한 줄씩 표를 만든다
    for t in types:
        cells = []
        # 열마다 속성 값을 꺼낸다
        for _, key in TABLE_COLUMNS:
            if key == "foot":
                fx, fy = get_prop(t, "foot_x"), get_prop(t, "foot_y")
                cells.append(f"{fx}×{fy}" if fx is not None else "")
            else:
                v = get_prop(t, key)
                cells.append("" if v is None else str(v))
        lines.append(f"| {t.name} | " + " | ".join(cells) + f" | {' '.join(t.flags)} |")
    lines.append("")
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"{len(types)}개 타입 → {args.out}")


def main():
    """명령줄 인자 처리"""
    parser = argparse.ArgumentParser(description=".type 파서")
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("json", help="전체 타입을 JSON 으로")
    p.add_argument("--orig", default="originals")
    p.add_argument("--out", default=os.path.join("extracted", "types.json"))
    p.set_defaults(func=cmd_json)
    p = sub.add_parser("table", help="수치표 Markdown 생성")
    p.add_argument("--orig", default="originals")
    p.add_argument("--out", default=os.path.join("docs", "gameplay", "types.md"))
    p.set_defaults(func=cmd_table)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
