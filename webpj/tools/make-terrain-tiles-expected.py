# -*- coding: utf-8 -*-
"""
webpj 의 지면 타일 선택(www/assets/terrain-tiles.js) 검사용 기대값 표를 만든다.

JS 구현과 겹치지 않게 다음을 따로 계산한다.
  * 본섬 칸 마스크: tools/terrain_mask.py(원본 Terrainbuilder 디컴파일·어셈블리에서 따로 옮긴 Python 재현)의 generate_mask.
  * 클러스터 표: 아카이브의 isle.type 텍스트를 이름·플래그·이미지 파일 이름 줄로 직접 읽는다(JS 의 .type 분석기를 쓰지 않는다). 원소 구간은 원본 산술(후보 수의 4분의 1)로 구하고,
    그림 파일 이름(RA*=비, TH*=천둥, WI*=바람, 그 밖=태양)으로 묶은 블록에서 첫 클러스터를 뺀 것과 같은지를 따로 적는다 — 실제로 쓰이는 쌍에서 같아야 "4분의 1 구간 = 원소" 가정이 맞다.
  * 안쪽/가장자리 구분: 원본 MarkEdges 처럼 여덟 이웃 중 하나라도 허공/월드 밖/다른 영역이면 가장자리('=')로 센다(JS 는 방향 글자 AA 로 구분한다).
  * 안쪽 칸 3 × 3 변형: MSVC rand 를 ctypes 로 실제 CRT(ucrtbase/msvcrt)에서 불러올 수 있으면 공식 구현과 대조하고, 아니면 공식으로 계산한다.
표에는 세 종류의 줄이 있다(탭으로 나눔, `#` 로 시작하는 줄은 설명).
  pair   방향 글자 쌍 256가지: 쌍 / 후보를 찾은 단계 / 후보 클러스터 번호(쉼표) / 원소 0~3 의 유효 후보(쉼표, 범위가 비면 `!`, 안쪽 AA 쌍은 `-`) / 그림 파일 이름 블록과 같은가(same|differs|-)
  core   안쪽 3 × 3 그림: 방식(stamp|paint0..3) / 256 × 256 월드의 프레임 번호(int32 little endian)의 SHA-256
  map    파일마다: 출처 / 마스크 SHA-256 / 지면 칸 수 / 안쪽 칸 수 / 가장자리 칸 수 / 방향 글자 쌍별 칸 수(`쌍:칸수,…`) / (x, y, 방향 글자, 변형 글자) 4바이트를 y·x 순서로 이은 글의 SHA-256
webpj/tests/terrain-tiles.test.mjs 가 JS 결과를 줄 단위로 대조한다. www/ 구현의 출력으로 이 표를 다시 만들지 않는다.

사용법 (저장소 루트에서, Pillow 필요, 약 4분):
    python webpj/tools/make-terrain-tiles-expected.py [--orig assets/game-data] [--out webpj/tests/fixtures/terrain-tiles-expected.tsv]

(2026-11-02 WB03-X: 정적 맵 화면(M2) 3단계(b) 지면 타일 선택 검사를 위해 추가)
"""
import argparse
import glob
import hashlib
import json
import os
import re
import struct
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

# 월드 한 변의 칸 수와 허공 표시이다.
WORLD = 256
EMPTY = terrain_mask.EMPTY

# 원본 비트→글자 표(0x52f8fc), 글자→연결 비트 표(0x52f910)를 이 도구에서 직접 적었다.
BIT_TO_LETTER = "PNOILJFBMHKEGDCA"
LETTER_TO_BITS = [15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0]
# 북부터 시계 방향 여덟 이웃과 켜는 비트이다(짝수 번호 직선, 홀수 번호 대각선).
NEIGHBORS = [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)]
NEIGHBOR_BITS = [1, 2, 2, 4, 4, 8, 8, 1]
LETTERS = "ABCDEFGHIJKLMNOP"
THEMES = ["sun", "thunder", "wind", "rain"]


def read_isle(archive):
    """아카이브의 isle.type 에서 클러스터 줄 [(이름, 플래그 단어 목록, 그림 파일 이름 목록)] 을 읽는다. 주석(`//`)과 속성 블록은 건너뛴다."""
    for entry in archive.entries:
        if entry.name.lower().endswith("isle.type"):
            text = archive.read(entry).decode("latin-1")
            break
    else:
        raise SystemExit("아카이브에 isle.type 이 없다")
    clusters = []
    pattern = re.compile(r'^\s*([A-Z][A-Z]\d+)\s*:\s*([^:]*?)\s*:\s*"([^"]+)"\s*#\s*\d+')
    # 줄마다 `이름 : 플래그… : "그림.gif" #번호 ;` 모양을 찾는다(여러 그림 층은 isle.type 에 없다).
    for line in text.splitlines():
        line = line.split("//")[0]
        found = pattern.match(line)
        if not found:
            continue
        clusters.append((found.group(1), found.group(2).split(), found.group(3)))
    return clusters


def image_theme(image):
    """그림 파일 이름에서 원소 번호를 구한다(RA*=비 3, TH*=천둥 1, WI*=바람 2, 나머지=태양 0)."""
    head = image[:2].upper()
    return {"RA": 3, "TH": 1, "WI": 2}.get(head, 0)


def normalize(side, variant):
    """원본 0041cd20 의 방향 정규화를 이 도구에서 따로 적었다: 양쪽 직선 이웃이 받쳐 주는 대각선만 남기고 직선 연결을 다시 만든다."""
    cardinal = LETTER_TO_BITS[LETTERS.index(side)]
    diagonal = LETTER_TO_BITS[LETTERS.index(variant)]
    supported = 0
    # 대각선 비트 i 는 직선 비트 i 와 앞 직선 비트(i + 3 & 3)가 모두 있어야 유효하다.
    for i in range(4):
        pair = (1 << i) | (1 << ((i + 3) & 3))
        if cardinal & pair == pair:
            supported |= 1 << i
    diagonal &= supported
    result = 0
    for i in range(4):
        if diagonal & (1 << i):
            result |= (1 << i) | (1 << ((i + 3) & 3))
    return BIT_TO_LETTER[result], BIT_TO_LETTER[diagonal]


def find(clusters, side, variant):
    """방향 글자(와 변형 글자)가 맞는 클러스터 번호를 선언 순서대로 최대 256개 모은다."""
    out = []
    for index, (name, _flags, _image) in enumerate(clusters):
        if name[0] == side and (variant is None or name[1] == variant) and len(out) < 256:
            out.append(index)
    return out


def pair_candidates(clusters, side, variant):
    """한 글자 쌍의 (단계, 후보 번호 목록): 정확히 → 정규화 → (방향, A) → 방향만 순서이다."""
    out = find(clusters, side, variant)
    if out:
        return "exact", out
    nside, nvariant = normalize(side, variant)
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


def block_sets(clusters, candidates):
    """후보 목록을 그림 파일 이름의 원소로 나눈 블록에서 각 블록의 첫 클러스터를 뺀 유효 후보 4개(원소 0~3)를 돌려준다. 블록이 네 개로 나뉘지 않거나 비면 None 이다."""
    blocks = [[], [], [], []]
    for index in candidates:
        blocks[image_theme(clusters[index][2])].append(index)
    sets = [block[1:] for block in blocks]
    if any(len(block) == 0 for block in blocks) or len({len(block) for block in blocks}) != 1 or any(len(s) == 0 for s in sets):
        return None
    return sets


def quartile_sets(candidates):
    """원본 ChooseTerrainFrame 의 산술: q = 후보 수 ÷ 4(내림), 원소 t 의 유효 후보는 인덱스 [q × t + 1, q × (t + 1)) 이다. 범위가 비면 None 이다."""
    quarter = len(candidates) // 4
    sets = []
    for theme in range(4):
        low, high = quarter * theme + 1, quarter * (theme + 1)
        if high <= low:
            return None
        sets.append(candidates[low:high])
    return sets


def crt_rand_factory(seed):
    """MSVC rand 를 돌려준다. ctypes 로 실제 CRT 를 불러올 수 있으면 srand/rand 를 쓰고, 아니면 공식(상태 × 214013 + 2531011, 상위 16비트의 15비트)을 쓴다. 두 구현이 같은지 앞 500개로 확인한다."""
    state = [seed & 0xFFFFFFFF]

    def formula():
        state[0] = (state[0] * 0x343FD + 0x269EC3) & 0xFFFFFFFF
        return (state[0] >> 16) & 0x7FFF

    try:
        import ctypes
        runtime = None
        for name in ("ucrtbase", "msvcrt"):
            try:
                runtime = ctypes.CDLL(name)
                break
            except OSError:
                continue
        if runtime is None:
            return formula, "공식"
        runtime.srand(ctypes.c_uint(seed & 0xFFFFFFFF))
        # 실제 CRT 와 공식이 앞 500개에서 같은지 본다.
        reference = [runtime.rand() for _ in range(500)]
        state[0] = seed & 0xFFFFFFFF
        mine = [formula() for _ in range(500)]
        if reference != mine:
            raise SystemExit("MSVC rand 공식이 실제 CRT 와 다르다")
        runtime.srand(ctypes.c_uint(seed & 0xFFFFFFFF))
        return (lambda: runtime.rand()), "실제 CRT"
    except Exception as error:  # ctypes 를 쓸 수 없는 환경은 공식으로 간다.
        if isinstance(error, SystemExit):
            raise
        state[0] = seed & 0xFFFFFFFF
        return formula, "공식"


def stamp_table():
    """첫 도장의 변형 표 99개: 실제 CRT 로 시드 0x38d535, 103개를 버린 뒤 99개를 뽑고 앞 항목과 하위 2비트가 같으면 1 올린다."""
    rand, how = crt_rand_factory(0x38D535)
    for _ in range(103):
        rand()
    values = []
    for index in range(99):
        value = rand()
        if index and values[index - 1] % 4 == value % 4:
            value += 1
        values.append(value)
    return values, how


def paint_table():
    """지역 도장의 변형 표 99개: 항목마다 시드 0 의 정수 난수 Next(65536) 을 새로 뽑는다(앞 항목과 하위 2비트가 같으면 1 올린다)."""
    values = []
    for index in range(99):
        state = (0x0BAD0BAD * 0x10003 + 3) & 0xFFFFFFFF
        value = (state >> 16) % 65536
        if index and values[index - 1] % 4 == value % 4:
            value += 1
        values.append(value)
    return values


def core_frame(start, table, x, y, element):
    """안쪽 3 × 3 그림의 프레임 번호이다: 시작 + 원소 × 36 + 변형 × 9 + (y % 3) × 3 + x % 3. 변형 = (표[(표[x ÷ 3 % 99] + 표[y ÷ 3 % 99]) % 99] + y ÷ 3) % 4 이다(첫 도장과 지역 도장의 식이 같고 표와 원소만 다르다)."""
    bx, by = (x // 3) % 99, (y // 3) % 99
    total = (table[bx] + table[by]) % 99
    variant = (table[total] + y // 3) % 4
    return start + element * 36 + variant * 9 + (y % 3) * 3 + x % 3


def describe_map(source, data, patterns):
    """`.fort` 한 개의 map 줄을 만든다."""
    sections, _end = fortpy.split_sections(data)
    named = {fortpy.SECTION_NAMES[i]: s for i, s in enumerate(sections[:SECTION_LIMIT])}
    rows = named.get("Territory", b"")
    rows = bytes(rows[:TERRITORY_BYTES]) if len(rows) >= TERRITORY_BYTES else bytes(TERRITORY_BYTES)
    land = terrain_mask.generate_mask(rows, patterns)
    histogram = {}
    interior = edge = total = 0
    digest = hashlib.sha256()
    # 월드를 y 바깥·x 안쪽 순서로 훑는다.
    for y in range(WORLD):
        for x in range(WORLD):
            region = land[y * WORLD + x]
            if region == EMPTY:
                continue
            straight = diagonal = 0
            same = 0
            # 북부터 시계 방향 여덟 이웃을 본다(범위 밖·허공·다른 영역은 켜지 않는다).
            for number, (dx, dy) in enumerate(NEIGHBORS):
                nx, ny = x + dx, y + dy
                if 0 <= nx < WORLD and 0 <= ny < WORLD and land[ny * WORLD + nx] == region:
                    same += 1
                    if number % 2:
                        diagonal |= NEIGHBOR_BITS[number]
                    else:
                        straight |= NEIGHBOR_BITS[number]
            side, variant = BIT_TO_LETTER[straight], BIT_TO_LETTER[diagonal]
            # 원본 MarkEdges: 여덟 이웃이 모두 같은 영역이어야 안쪽('x')이다.
            if same == 8:
                interior += 1
            else:
                edge += 1
            total += 1
            key = side + variant
            histogram[key] = histogram.get(key, 0) + 1
            digest.update(struct.pack("BBBB", x, y, ord(side), ord(variant)))
    return [
        "map", source, hashlib.sha256(bytes(land)).hexdigest(), str(total), str(interior), str(edge),
        ",".join("%s:%d" % (k, histogram[k]) for k in sorted(histogram)) or "-", digest.hexdigest(),
    ]


def main():
    """명령줄 인자를 읽어 pair·core·map 줄을 쓴다."""
    parser = argparse.ArgumentParser(description="webpj 지면 타일 선택 기대값 표 생성")
    parser.add_argument("--orig", default=os.path.join(ROOT, "assets", "game-data"))
    parser.add_argument("--out", default=os.path.join(ROOT, "webpj", "tests", "fixtures", "terrain-tiles-expected.tsv"))
    args = parser.parse_args()
    with open(PATTERNS, encoding="utf-8") as handle:
        patterns = json.load(handle)
    archive = TaffArchive(os.path.join(args.orig, "netstorm.tarc"))
    clusters = read_isle(archive)
    lines = []

    # pair 줄: 256가지 글자 쌍. 안쪽 AA 쌍은 첫 도장/지역 도장이 3 × 3 그림으로 처리하므로 원소 구간이 해당 없다(`-`).
    # 원소 구간은 원본 산술(후보 수의 4분의 1)로 구하고, 마지막 칸에 "그림 파일 이름으로 나눈 블록(첫 클러스터 제외)과 같은가"를 same/differs 로 적는다(AA·후보 없음은 `-`).
    for side in LETTERS:
        for variant in LETTERS:
            stage, candidates = pair_candidates(clusters, side, variant)
            if side == "A" and variant == "A":
                sets, blocks = "-\t-\t-\t-", "-"
            elif not candidates:
                sets, blocks = "!\t!\t!\t!", "-"
            else:
                quarters = quartile_sets(candidates)
                sets = "\t".join((",".join(str(i) for i in quarters[t]) if quarters else "!") for t in range(4))
                blocks = "same" if quarters is not None and quarters == block_sets(clusters, candidates) else "differs"
            lines.append("pair\t%s%s\t%s\t%s\t%s\t%s" % (side, variant, stage, ",".join(str(i) for i in candidates) or "-", sets, blocks))

    # core 줄: 3 × 3 그림 시작 번호(JJ00 다음)와 월드 전체 프레임 번호의 해시이다. 먼저 원소 순서가 그림 파일 이름과 맞는지 본다.
    start = [name for name, _f, _i in clusters].index("JJ00") + 1
    for element in range(4):
        for offset in range(36):
            name, _flags, image = clusters[start + element * 36 + offset]
            if name != "AA00" or image_theme(image) != element:
                raise SystemExit("3 × 3 그림 묶음이 원소 순서와 다르다: %d %d" % (element, offset))
    stamp, how = stamp_table()
    paint = paint_table()
    grid = bytearray()
    for y in range(WORLD):
        for x in range(WORLD):
            grid += struct.pack("<i", core_frame(start, stamp, x, y, 0))
    lines.append("core\tstamp\t%s" % hashlib.sha256(bytes(grid)).hexdigest())
    for element in range(4):
        grid = bytearray()
        for y in range(WORLD):
            for x in range(WORLD):
                grid += struct.pack("<i", core_frame(start, paint, x, y, element))
        lines.append("core\tpaint%d\t%s" % (element, hashlib.sha256(bytes(grid)).hexdigest()))

    # map 줄: 느슨한 파일 → 아카이브 순서(terrain 표와 같다).
    maps = []
    for path in sorted(glob.glob(os.path.join(args.orig, "d", "*.fort")), key=lambda p: os.path.basename(p).lower()):
        maps.append(describe_map("loose:" + os.path.basename(path), open(path, "rb").read(), patterns))
    for entry in archive.entries:
        if entry.name.lower().endswith(".fort"):
            maps.append(describe_map("tarc:" + entry.name, archive.read(entry), patterns))
    header = [
        "# 지면 타일 선택 기대값(탭 구분). pair: 글자 쌍 / 찾은 단계 / 후보 클러스터 번호 / 원소 0~3 의 유효 후보(`!` 범위 없음, `-` 해당 없음).",
        "# core: stamp|paint0..3 / 256 × 256 월드의 3 × 3 그림 프레임 번호(int32 little endian)의 SHA-256 (stamp 표 생성: %s)." % how,
        "# map: 출처 / 마스크 SHA-256 / 지면 칸 수 / 안쪽 칸 수 / 가장자리 칸 수 / 방향 글자 쌍별 칸 수 / (x, y, 방향 글자, 변형 글자) 4바이트열 SHA-256.",
        "# 생성: webpj/tools/make-terrain-tiles-expected.py 가 isle.type 텍스트와 tools/terrain_mask.py(원본에서 따로 옮긴 Python 재현, JS 구현과 독립)로 assets/game-data 를 읽어 만들었다(2026-11-02 WB03-X).",
        "# 이 파일은 webpj/tests/terrain-tiles.test.mjs 가 www/assets/terrain-tiles.js 의 결과와 대조한다. www/ 구현의 출력으로 다시 만들지 않는다.",
    ]
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(header) + "\n")
        for line in lines:
            handle.write(line + "\n")
        for row in maps:
            handle.write("\t".join(row) + "\n")
    print("pair/core 줄 수:", len(lines), "map 줄 수:", len(maps), "stamp 표:", how, "->", args.out)


if __name__ == "__main__":
    main()
