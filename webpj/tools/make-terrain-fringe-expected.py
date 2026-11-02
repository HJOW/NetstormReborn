# -*- coding: utf-8 -*-
"""
webpj 의 절벽 fringe(www/assets/terrain-fringe.js) 검사용 기대값 표를 만든다.

JS 구현·JS `.type` 분석기와 겹치지 않게, 아카이브의 `fringe.type`·`isle.type` 텍스트를 줄 단위 정규식으로 직접 읽는다.
  * 글자 쌍 256가지(방향 글자 × 변형 글자)마다 `fringe` 타입의 후보를 찾는다: 정확히 → 정규화 → (방향, A) → 방향만(지면 선택과 같은 4단계).
    정규화(원본 0041cd20)는 tools/make-terrain-tiles-expected.py 에서 따로 적어 둔 구현을 그대로 불러 쓴다(그 도구도 JS 와 독립이다).
  * 클러스터 이름 줄의 플래그 단어 `lit`/`unlit` 로 제외 플래그(0x10 `unlit` 만, 또는 0x18 `lit`+`unlit`)를 지킨 허용 후보를 적는다.
  * `isle.type` 에서 `fringe` 플래그 단어가 있는 클러스터(번호·이름)를 적는다(주석 처리된 줄은 제외).
표에는 두 종류의 줄이 있다(탭으로 나눔, `#` 로 시작하는 줄은 설명).
  pair   글자 쌍 / 후보를 찾은 단계 / 후보 번호(쉼표, 없으면 `-`) / 0x10 제외 후 허용 후보 / 0x18 제외 후 허용 후보
  isle   fringe 플래그가 있는 isle 클러스터: 번호 / 이름 / 0x10 제외 후 허용 후보 수
webpj/tests/terrain-fringe.test.mjs 가 JS 결과를 줄 단위로 대조한다. www/ 구현의 출력으로 이 표를 다시 만들지 않는다.

사용법 (저장소 루트에서, Pillow 불필요, 약 1초):
    python webpj/tools/make-terrain-fringe-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/terrain-fringe-expected.tsv]

(2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c) 절벽 fringe 검사를 위해 추가)
"""
import argparse
import importlib.util
import os
import re
import sys

# 저장소 루트와 tools/ 폴더를 불러오기 경로에 넣는다(taff.py 를 그대로 쓴다).
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from taff import TaffArchive  # noqa: E402

# 앞 단위의 독립 기대값 도구에서 정규화·글자 표를 불러온다(파일 이름에 하이픈이 있어 importlib 로 읽는다).
_SPEC = importlib.util.spec_from_file_location("tiles_expected", os.path.join(os.path.dirname(os.path.abspath(__file__)), "make-terrain-tiles-expected.py"))
tiles = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(tiles)

# 클러스터 플래그 단어와 비트이다(원본 로더가 프레임 코드에 반영하는 7종).
FLAG_BITS = {"fringe": 0x01, "suck": 0x02, "rim": 0x04, "lit": 0x08, "unlit": 0x10, "cracked": 0x20, "hard": 0x40}
LETTERS = "ABCDEFGHIJKLMNOP"
# 제외 플래그: 전투 맥락, 맥락 소유자 0.
EXCLUDE_BATTLE = 0x10
EXCLUDE_NONE_OWNER = 0x18


def read_type(archive, suffix):
    """아카이브의 `suffix`(예: fringe.type)로 끝나는 파일에서 클러스터 줄 [(이름, 플래그 비트)] 을 선언 순서대로 읽는다.

    주석(`//`) 뒤는 버리고, `이름 : 플래그 단어… : "그림" #번호 ;` 줄만 센다. 플래그 단어는 대소문자를 구분하지 않는다.
    """
    for entry in archive.entries:
        if entry.name.lower().endswith(suffix):
            text = archive.read(entry).decode("latin-1")
            break
    else:
        raise SystemExit("아카이브에 %s 가 없다" % suffix)
    clusters = []
    pattern = re.compile(r'^\s*([A-Z][A-Z]\d+)\s*:\s*([^:]*?)\s*:\s*"([^"]+)"\s*#\s*\d+')
    for line in text.splitlines():
        line = line.split("//")[0]
        found = pattern.match(line)
        if not found:
            continue
        bits = 0
        for word in found.group(2).split():
            bits |= FLAG_BITS.get(word.lower(), 0)
        clusters.append((found.group(1), bits))
    return clusters


def find(clusters, side, variant):
    """방향 글자(와 변형 글자)가 맞는 클러스터 번호를 선언 순서대로 최대 256개 모은다."""
    out = []
    for index, (name, _bits) in enumerate(clusters):
        if name[0] == side and (variant is None or name[1] == variant) and len(out) < 256:
            out.append(index)
    return out


def pair_candidates(clusters, side, variant):
    """한 글자 쌍의 (단계, 후보 번호 목록): 정확히 → 정규화 → (방향, A) → 방향만 순서이다."""
    out = find(clusters, side, variant)
    if out:
        return "exact", out
    nside, nvariant = tiles.normalize(side, variant)
    out = find(clusters, nside, nvariant)
    if out:
        return "normalized", out
    out = find(clusters, side, "A")
    if out:
        return "sideA", out
    out = find(clusters, side, None)
    if out:
        return "side", out
    return "none", []


def allowed(clusters, candidates, excluded):
    """후보 가운데 플래그가 제외 플래그와 겹치지 않는 것(부호 확장은 플래그 최대 0x40 이라 영향 없음)."""
    return [index for index in candidates if not (clusters[index][1] & excluded)]


def join(values):
    """번호 목록을 쉼표 글로, 비면 `-`."""
    return ",".join(str(value) for value in values) if values else "-"


def build(orig):
    """기대값 표의 줄 목록을 만든다."""
    archive = TaffArchive(os.path.join(orig, "netstorm.tarc"))
    fringe = read_type(archive, "fringe.type")
    isle = read_type(archive, "isle.type")
    lines = [
        "# webpj 절벽 fringe 기대값 표 — make-terrain-fringe-expected.py 가 fringe.type·isle.type 텍스트를 직접 읽어 만든다(JS 구현의 출력이 아님).",
        "# pair\\t글자 쌍\\t단계\\t후보\\t0x10 제외 후\\t0x18 제외 후   /   isle\\t클러스터 번호\\t이름\\t0x10 제외 후 허용 후보 수",
    ]
    for side in LETTERS:
        for variant in LETTERS:
            stage, candidates = pair_candidates(fringe, side, variant)
            lines.append("\t".join(["pair", side + variant, stage, join(candidates),
                                    join(allowed(fringe, candidates, EXCLUDE_BATTLE)), join(allowed(fringe, candidates, EXCLUDE_NONE_OWNER))]))
    for index, (name, bits) in enumerate(isle):
        if bits & 0x01:
            stage, candidates = pair_candidates(fringe, name[0], name[1])
            lines.append("\t".join(["isle", str(index), name[:2], str(len(allowed(fringe, candidates, EXCLUDE_BATTLE)))]))
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"), help="원본 자료 폴더(netstorm.tarc 가 있는 곳)")
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "terrain-fringe-expected.tsv"))
    args = parser.parse_args()
    lines = build(args.orig)
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")
    print("wrote %s (%d lines)" % (args.out, len(lines)))


if __name__ == "__main__":
    main()
