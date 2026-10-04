# -*- coding: utf-8 -*-
"""
원본 Netstorm.exe 의 마우스 커서 리소스(RT_CURSOR)를 그림으로 풀고, ns_driver.py 가 실행 중에 찍은 커서 그림과 맞춰 본다.

원본은 상황마다 Windows 커서(SetCursor)를 바꾼다. 화면 캡처에는 커서가 안 찍히므로 ns_driver.py 의 커서 기록기가
GetCursorInfo 로 현재 커서 핸들을 읽어 그림으로 저장하고(cursors/<핸들>.png: 왼쪽 = 검은 배경, 오른쪽 = 흰 배경),
이 도구가 exe 리소스(extracted/res/Netstorm/RT_CURSOR_N.bin, RT_GROUP_CURSOR_N.bin)와 같은 그림을 찾아 그룹 번호로 이름을 붙인다.

사용법:
    python tools/cursor_catalog.py catalog -o extracted/screens/cursor-catalog.png
    python tools/cursor_catalog.py match <ns_driver 작업 폴더>/cursors
리소스 추출은 docs/formats/README.md 의 추출 순서를 따른다 (extracted/res/Netstorm).
"""
import argparse
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

# 저장소 루트와 exe 에서 풀어 둔 리소스 폴더
REPO = Path(__file__).resolve().parents[1]
RES = REPO / "extracted" / "res" / "Netstorm"
# 커서 그림을 맞출 때 쓰는 캔버스 한 변 (ns_driver.CANVAS 와 같다)
CANVAS = 64


def parse_cursor(path: Path):
    """RT_CURSOR 리소스 하나(핫스팟 4바이트 + DIB 머리 + 색 표 + XOR 비트맵 + AND 마스크)를 해석한다.
    (핫스팟, 너비, 높이, XOR 색 배열, AND 마스크 배열)을 돌려준다"""
    data = path.read_bytes()
    hot_x, hot_y = struct.unpack_from("<HH", data, 0)
    header = data[4:]
    size, width, height2, _planes, bpp, _comp, _img, _xp, _yp, colors, _imp = struct.unpack_from("<IiiHHIIiiII", header, 0)
    height = height2 // 2
    offset = size
    palette = []
    # 색 표(1·4·8비트 커서)를 읽는다
    for i in range(colors if colors else (1 << bpp if bpp <= 8 else 0)):
        b, g, r, _ = struct.unpack_from("<BBBB", header, offset + 4 * i)
        palette.append((r, g, b))
    offset += 4 * len(palette)
    row_bytes = ((width * bpp + 31) // 32) * 4
    xor = np.zeros((height, width, 3), dtype=int)
    # XOR 비트맵은 아래 줄부터 저장되어 있다
    for y in range(height):
        row = header[offset + y * row_bytes: offset + (y + 1) * row_bytes]
        for x in range(width):
            if bpp == 1:
                color = palette[(row[x // 8] >> (7 - x % 8)) & 1]
            elif bpp == 4:
                color = palette[(row[x // 2] >> (4 if x % 2 == 0 else 0)) & 15]
            elif bpp == 8:
                color = palette[row[x]]
            elif bpp == 24:
                color = (row[3 * x + 2], row[3 * x + 1], row[3 * x])
            else:
                color = (row[4 * x + 2], row[4 * x + 1], row[4 * x])
            xor[height - 1 - y, x] = color
    offset += row_bytes * height
    mask_bytes = ((width + 31) // 32) * 4
    mask = np.zeros((height, width), dtype=int)
    # AND 마스크도 아래 줄부터 저장되어 있다
    for y in range(height):
        row = header[offset + y * mask_bytes: offset + (y + 1) * mask_bytes]
        for x in range(width):
            mask[height - 1 - y, x] = (row[x // 8] >> (7 - x % 8)) & 1
    return (hot_x, hot_y), width, height, xor, mask


def groups():
    """RT_GROUP_CURSOR 번호 → 커서 이미지 리소스 번호 표를 만든다 (그룹 하나에 이미지 하나인 커서만)"""
    table = {}
    # 그룹 리소스를 번호순으로 읽는다
    for path in sorted(RES.glob("RT_GROUP_CURSOR_*.bin"), key=lambda p: int(p.stem.split("_")[-1])):
        data = path.read_bytes()
        _res, _typ, count = struct.unpack_from("<HHH", data, 0)
        ids = [struct.unpack_from("<HHHHIH", data, 6 + 14 * i)[5] for i in range(count)]
        table[int(path.stem.split("_")[-1])] = ids[0]
    return table


def render_on_black(width, height, xor, mask):
    """AND 마스크가 0 인 곳은 XOR 색, 1 이고 XOR 가 0 이 아닌 곳은 반전(검은 배경에서는 흰색)으로 그린다"""
    out = np.zeros((height, width, 3), dtype=int)
    # 모든 픽셀을 훑는다
    for y in range(height):
        for x in range(width):
            if mask[y, x] == 0:
                out[y, x] = xor[y, x]
            elif xor[y, x].any():
                out[y, x] = (255, 255, 255)
    return out


def cmd_catalog(args):
    """모든 커서를 4배 확대해 그룹 번호·핫스팟과 함께 한 장으로 만든다"""
    table = groups()
    scale = 4
    columns = 6
    cell_w, cell_h = 32 * scale + 10, 32 * scale + 34
    sheet = Image.new("RGB", (columns * cell_w, ((len(table) + columns - 1) // columns) * cell_h), (70, 70, 90))
    draw = ImageDraw.Draw(sheet)
    # 그룹 번호순으로 한 칸씩 그린다
    for i, (group, image_id) in enumerate(table.items()):
        hot, width, height, xor, mask = parse_cursor(RES / f"RT_CURSOR_{image_id}.bin")
        picture = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        pixels = picture.load()
        # 마스크 0 인 곳은 색, 반전 픽셀은 분홍으로 표시한다
        for y in range(height):
            for x in range(width):
                if mask[y, x] == 0:
                    pixels[x, y] = tuple(int(v) for v in xor[y, x]) + (255,)
                elif xor[y, x].any():
                    pixels[x, y] = (255, 0, 255, 200)
        big = picture.resize((width * scale, height * scale), Image.NEAREST)
        ox, oy = (i % columns) * cell_w + 5, (i // columns) * cell_h + 5
        sheet.paste(big, (ox, oy), big)
        draw.rectangle([ox + hot[0] * scale, oy + hot[1] * scale, ox + hot[0] * scale + scale, oy + hot[1] * scale + scale], outline=(255, 255, 0))
        draw.text((ox, oy + 32 * scale + 3), f"group{group} id{image_id} hot{hot}", fill=(255, 255, 255))
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    sheet.save(args.output)
    print(args.output, sheet.size)


def cmd_match(args):
    """작업 폴더의 cursors/*.png(검은 배경 쪽 64x64)를 exe 커서와 비교해 그룹 번호를 출력한다"""
    table = groups()
    resources = {}
    # exe 커서를 검은 배경 그림으로 만들어 둔다
    for group, image_id in table.items():
        hot, width, height, xor, mask = parse_cursor(RES / f"RT_CURSOR_{image_id}.bin")
        resources[group] = (hot, render_on_black(width, height, xor, mask))
    # 캡처한 커서마다 가장 같은 그림을 찾는다
    for path in sorted(Path(args.folder).glob("*.png")):
        image = np.asarray(Image.open(path).convert("RGB")).astype(int)[:, :CANVAS]
        best = None
        # 왼쪽 위에 그려지므로 같은 크기 영역만 비교한다
        for group, (hot, picture) in resources.items():
            h, w, _ = picture.shape
            diff = np.abs(image[:h, :w] - picture).sum()
            if best is None or diff < best[0]:
                best = (diff, group, hot)
        print(f"{path.stem}: group {best[1]} hotspot {best[2]} 차이 {best[0]}")


def main():
    """명령줄 진입점"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    catalog = sub.add_parser("catalog")
    catalog.add_argument("-o", "--output", required=True)
    match = sub.add_parser("match")
    match.add_argument("folder")
    args = parser.parse_args()
    {"catalog": cmd_catalog, "match": cmd_match}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
