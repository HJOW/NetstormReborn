# -*- coding: utf-8 -*-
"""
_shapes.shp (VFX 셰이프 데이터베이스) 추출 도구

포맷 명세: docs/formats/shp.md

사용법:
    python tools/shp.py info   [--orig originals]
    python tools/shp.py export [--orig originals] [--out extracted/shapes] [--type sunCannon]

export 결과 (타입마다 폴더 하나):
    extracted/shapes/058_sunCannon/
        N00_L0.png ...   클러스터 이름 + 레이어 번호(L0 = 본체, L1 = 그림자 등)
        sheet.png        전체 프레임 모음
        meta.json        프레임별 기준점·영역 정보
"""
import argparse
import json
import os
import re
import struct
import sys
from dataclasses import dataclass, field

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from taff import TaffArchive  # noqa: E402  (같은 폴더의 TAFF 도구 재사용)

# VFX 셰이프 블록 매직 ("1.10", little-endian u32 로 읽으면 0x30312E31)
SHAPE_MAGIC = b"1.10"

# 프레임 헤더 크기 (bounds u16×2, origin u16×2, xmin/ymin/xmax/ymax i32×4)
FRAME_HEADER_SIZE = 24

# 이미지가 아닌 특수 레코드 판별 기준: xmin 이 이 값 이상이면 좌표가 아닌 다른 데이터를 담고 있다
SPECIAL_RECORD_THRESHOLD = 0x7FFF0000

# 게임 팔레트 파일 (setup.cfg: fortPal/battlePal/chalPal = "gifcloud", GamePalSpec = "{DataDir}\{local.1}.COL")
PALETTE_FILE = os.path.join("d", "GIFCLOUD.COL")

# Animator Pro .COL 팔레트 헤더 크기 (파일 크기 u32 + 매직 0xB123 u16 + 버전 u16)
COL_HEADER_SIZE = 8

# 타입 로딩 순서. Netstorm.exe 0x540DD0 의 포인터 배열(116개)을 옮긴 것.
# 게임은 이 순서로 .type 을 읽으며, i 번째 타입에 _shapes.shp 의 i 번째 블록을 배정한다.
TYPE_LOAD_ORDER = [
    "dude", "sunArcher", "sunAviary", "sunFlyer", "banner", "windBattery", "rainBattery",
    "thunderBattery", "sunBlocker", "thunderBlocker", "blankMissile", "bolt", "bridge", "sunDisc",
    "emptyGeyser", "sunFactory", "windFactory", "rainFactory", "thunderFactory", "residence",
    "fencemark", "flag", "fortGump", "icon", "island", "islandStalag", "playerBanner", "isle",
    "isleBig", "geyserBrightener", "treeTwo", "treeThree", "fringe", "mana", "range", "manabolt",
    "flare", "puzzlePiece", "playerIsland", "battleIsland", "particlePlaceHolder", "sunBalloon",
    "buried", "bombExplodeSmall", "bombExplodeMedium", "bombExplodeLarge", "bombHeal",
    "bombInvisible", "bombParalyze", "bombHardener", "bombTreason", "edgeFarm", "geyser",
    "windVortex", "rainVortex", "thunderVortex", "outpost", "mcloud", "sunCannon", "rainCannon",
    "rainCannonMissile", "thunderCannon", "thunderCannonMissile", "sunWalker", "teleportEffect",
    "bulf", "sunFence", "windWalker", "windFlyer", "windAviary", "windArcher", "windBalloon",
    "windBlocker", "rainAviary", "rainFlyer", "rainFence", "rainBalloon", "rainBlocker",
    "thunderArcher", "thunderFence", "growingRainBlocker", "platform", "player", "mog", "nugget",
    "bridgeConnector", "rainWalker", "noIsland", "priest", "lightning", "anim", "challengeIsland",
    "fakeThreeByThreeSurface", "sunFlyerBomb", "altar", "dais", "rune", "forceField",
    "bombSpecialOne", "daisExtraFrames", "fenceShield", "bombTwister", "bombIITwister",
    "bombIIITwister", "GravitationEffect", "bombGraviton", "HealEffect", "MeteorEffect",
    "BOMBmeteor", "bombIMano", "bombIIMano", "bombIIIMano", "bombLightingZap",
    "bombLightingwave", "RUIN", "Monument",
]

# .type 파일의 클러스터 정의 줄: "이름 : 플래그 : "그림.gif" #번호 [: "그림.gif" #번호] ;"
CLUSTER_LINE_RE = re.compile(r"^\s*([A-Za-z]+\d+)\s*:([^:]*):(.*)$")

# 클러스터 줄 안의 그림 참조 ("파일.gif" #번호)
IMAGE_REF_RE = re.compile(r'"([^"]+)"\s*#\s*(\d+)')


@dataclass
class Frame:
    """셰이프 프레임 하나"""
    offset: int          # 파일 내 절대 위치
    bounds: tuple        # 헤더의 bounds (u16, u16) — 원본 그림 크기로 추정
    origin: tuple        # 헤더의 origin (u16, u16) — 원본 그림 안의 기준점으로 추정
    rect: tuple          # 기준점 대비 픽셀 영역 (xmin, ymin, xmax, ymax), 양 끝 포함
    special: bool = False  # 이미지가 아닌 특수 레코드 여부

    @property
    def width(self) -> int:
        """픽셀 영역 폭"""
        return self.rect[2] - self.rect[0] + 1

    @property
    def height(self) -> int:
        """픽셀 영역 높이"""
        return self.rect[3] - self.rect[1] + 1


@dataclass
class Block:
    """타입 하나에 대응하는 셰이프 블록 ("1.10" 헤더 + 프레임 테이블)"""
    index: int
    offset: int
    frames: list = field(default_factory=list)


def read_palette(path: str) -> list:
    """Animator Pro .COL 팔레트(256×RGB)를 읽어 [r,g,b,...] 리스트로 돌려준다"""
    with open(path, "rb") as f:
        data = f.read()
    return list(data[COL_HEADER_SIZE:COL_HEADER_SIZE + 768])


def read_blocks(data: bytes) -> list:
    """파일 앞부분에 연속으로 놓인 셰이프 블록 헤더를 모두 읽는다"""
    blocks = []
    pos = 0
    # "1.10" 매직이 이어지는 동안 블록 헤더를 차례로 읽는다
    while data[pos:pos + 4] == SHAPE_MAGIC:
        count, = struct.unpack_from("<I", data, pos + 4)
        block = Block(len(blocks), pos)
        # 프레임 테이블 [블록 기준 오프셋 u32][컬러맵 오프셋 u32(항상 0)] 을 읽는다
        for i in range(count):
            rel, _colormap = struct.unpack_from("<II", data, pos + 8 + 8 * i)
            block.frames.append(read_frame_header(data, pos + rel))
        blocks.append(block)
        pos += 8 + 8 * count
    return blocks


def read_frame_header(data: bytes, ofs: int) -> Frame:
    """프레임 헤더를 읽는다"""
    b0, b1, o0, o1, xmin, ymin, xmax, ymax = struct.unpack_from("<HHHHiiii", data, ofs)
    special = xmin >= SPECIAL_RECORD_THRESHOLD or xmax < xmin or ymax < ymin
    return Frame(ofs, (b0, b1), (o0, o1), (xmin, ymin, xmax, ymax), special)


def decode_frame(data: bytes, frame: Frame) -> list:
    """RLE 픽셀 데이터를 풀어 행 단위 리스트(투명 = None, 그 외 = 팔레트 인덱스)로 돌려준다.

    행마다 토큰을 읽는다:
      0x00           : 행 끝 (남은 픽셀은 투명)
      0x01, n        : 투명 픽셀 n 개 건너뛰기
      홀수 t (>1)    : 뒤따르는 (t>>1) 바이트를 그대로 복사
      짝수 t (>0)    : 다음 바이트 1개를 (t>>1) 번 반복
    """
    w, h = frame.width, frame.height
    rows = []
    pos = frame.offset + FRAME_HEADER_SIZE
    # 행 수만큼 반복
    for _ in range(h):
        row = [None] * w
        x = 0
        # 행 끝 토큰(0)이 나올 때까지 토큰을 처리
        while True:
            t = data[pos]
            pos += 1
            if t == 0:
                break
            if t == 1:
                x += data[pos]
                pos += 1
            elif t & 1:
                n = t >> 1
                row[x:x + n] = data[pos:pos + n]
                pos += n
                x += n
            else:
                n = t >> 1
                row[x:x + n] = [data[pos]] * n
                pos += 1
                x += n
        rows.append(row[:w])
    return rows


def frame_to_image(data: bytes, frame: Frame, palette: list) -> Image.Image:
    """프레임을 투명 배경 RGBA 이미지로 만든다"""
    img = Image.new("RGBA", (frame.width, frame.height), (0, 0, 0, 0))
    px = img.load()
    # 디코딩한 행·열을 순회하며 팔레트 색을 칠한다
    for y, row in enumerate(decode_frame(data, frame)):
        for x, idx in enumerate(row):
            if idx is not None:
                px[x, y] = (palette[idx * 3], palette[idx * 3 + 1], palette[idx * 3 + 2], 255)
    return img


def read_clusters(type_text: str) -> list:
    """.type 텍스트에서 클러스터 정의를 순서대로 읽는다. [(이름, 플래그, [(gif, 번호), ...]), ...]"""
    clusters = []
    # 줄마다 주석을 떼고 클러스터 정의인지 검사
    for line in type_text.splitlines():
        line = line.split("//")[0]
        m = CLUSTER_LINE_RE.match(line)
        if not m:
            continue
        refs = [(g, int(n)) for g, n in IMAGE_REF_RE.findall(m.group(3))]
        clusters.append((m.group(1), m.group(2).strip(), refs))
    return clusters


def load_type_texts(orig: str) -> dict:
    """TAFF 아카이브에서 .type 텍스트를 모두 읽는다 (키: 소문자 타입 이름)"""
    arc = TaffArchive(os.path.join(orig, "netstorm.tarc"))
    texts = {}
    # .type 엔트리만 골라 복호화
    for e in arc.entries:
        if e.name.lower().endswith(".type"):
            name = e.name.replace("\\", "/").split("/")[-1][:-5].lower()
            texts[name] = arc.read(e).decode("latin-1")
    return texts


def frame_names(block: Block, clusters: list) -> list:
    """블록 프레임마다 "클러스터_L레이어" 이름을 붙인다.
    프레임 배치: 번호 = 레이어 × 클러스터 수 + 클러스터 순번 (레이어 0 = 본체, 1 = 그림자 등)"""
    n = len(block.frames)
    if not clusters:
        return [f"F{i:03d}" for i in range(n)]
    stride = len(clusters)
    names = []
    # 프레임 번호를 (레이어, 클러스터 순번)으로 나눠 이름을 만든다
    for i in range(n):
        layer, ci = divmod(i, stride)
        cname = clusters[ci][0] if ci < len(clusters) else f"X{ci:03d}"
        names.append(f"{cname}_L{layer}")
    return names


def cmd_info(args):
    """블록·타입 대응 요약 출력"""
    data = open(os.path.join(args.orig, "d", "_shapes.shp"), "rb").read()
    blocks = read_blocks(data)
    texts = load_type_texts(args.orig)
    print(f"블록 {len(blocks)}개, 프레임 {sum(len(b.frames) for b in blocks)}개")
    # 블록마다 대응 타입과 클러스터·레이어 수를 출력
    for b in blocks:
        tname = TYPE_LOAD_ORDER[b.index]
        clusters = read_clusters(texts.get(tname.lower(), ""))
        layers = max([len(c[2]) for c in clusters] or [0])
        special = sum(f.special for f in b.frames)
        mark = "" if len(clusters) * layers == len(b.frames) else "  (클러스터×레이어 불일치)"
        print(f"{b.index:3d} {tname:24s} 프레임 {len(b.frames):4d}  클러스터 {len(clusters):4d} × 레이어 {layers}"
              f"  특수 {special}{mark}")


def cmd_export(args):
    """프레임 PNG·시트·메타데이터 내보내기"""
    data = open(os.path.join(args.orig, "d", "_shapes.shp"), "rb").read()
    palette = read_palette(os.path.join(args.orig, PALETTE_FILE))
    blocks = read_blocks(data)
    texts = load_type_texts(args.orig)
    # 블록(=타입)마다 폴더를 만들어 내보낸다
    for b in blocks:
        tname = TYPE_LOAD_ORDER[b.index]
        if args.type and args.type.lower() != tname.lower():
            continue
        clusters = read_clusters(texts.get(tname.lower(), ""))
        names = frame_names(b, clusters)
        outdir = os.path.join(args.out, f"{b.index:03d}_{tname}")
        os.makedirs(outdir, exist_ok=True)
        meta = {"type": tname, "block": b.index, "frames": []}
        images = []
        # 프레임을 하나씩 이미지로 변환해 저장
        for fr, name in zip(b.frames, names):
            entry = {"name": name, "bounds": fr.bounds, "origin": fr.origin, "rect": fr.rect,
                     "special": fr.special}
            meta["frames"].append(entry)
            if fr.special:
                continue
            img = frame_to_image(data, fr, palette)
            img.save(os.path.join(outdir, f"{name}.png"))
            images.append(img)
        with open(os.path.join(outdir, "meta.json"), "w", encoding="utf-8") as f:
            json.dump(meta, f, ensure_ascii=False, indent=1)
        save_sheet(images, os.path.join(outdir, "sheet.png"))
    print(f"내보내기 완료 → {args.out}")


def save_sheet(images: list, path: str, max_width: int = 1024):
    """프레임 이미지들을 한 장에 격자 형태로 모아 저장한다"""
    if not images:
        return
    x = y = row_h = 0
    positions = []
    # 폭을 넘으면 다음 줄로 넘기며 위치를 계산
    for img in images:
        if x + img.width > max_width and x > 0:
            x, y, row_h = 0, y + row_h + 2, 0
        positions.append((x, y))
        x += img.width + 2
        row_h = max(row_h, img.height)
    sheet = Image.new("RGBA", (max_width, y + row_h), (48, 48, 64, 255))
    # 계산한 위치에 각 프레임을 붙인다
    for img, pos in zip(images, positions):
        sheet.paste(img, pos, img)
    sheet.save(path)


def main():
    """명령줄 인자 처리"""
    parser = argparse.ArgumentParser(description="_shapes.shp 도구")
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("info", help="블록·타입 대응 요약")
    p.add_argument("--orig", default="originals")
    p.set_defaults(func=cmd_info)
    p = sub.add_parser("export", help="PNG 내보내기")
    p.add_argument("--orig", default="originals")
    p.add_argument("--out", default=os.path.join("extracted", "shapes"))
    p.add_argument("--type", help="이 타입만 내보내기")
    p.set_defaults(func=cmd_export)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    sys.exit(main())
