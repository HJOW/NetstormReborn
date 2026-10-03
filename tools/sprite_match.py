# -*- coding: utf-8 -*-
"""원본 녹화 프레임에서 오브젝트가 그리는 셰이프 클러스터를 템플릿 매칭으로 찾는다. 원본 게임을 실행하지 않는다.

`tools/shp.py export` 로 내보낸 `extracted/shapes/<번호>_<타입>/` 의 PNG·meta.json 을 쓴다.
녹화가 Windows 화면 배율(예: 125%)로 캡처된 경우 스프라이트를 같은 배율로 키워 비교한다.
점수는 스프라이트의 불투명 픽셀에서 잰 평균 RGB 절대차이며 작을수록 잘 맞는다.

같은 이름의 클러스터가 여러 개인 타입은 "이름#순번" 으로 구분한다. 카메라 위치를 모르는 장면은 --locate 로
프레임 전체에서 건물 하나의 기준점을 먼저 찾고, 그 값으로 다른 오브젝트의 기준점을 계산한다.

사용 예:
  python tools/sprite_match.py playingVideos/<세션ID> --type thunderFactory --clusters B --locate --times 345
  python tools/sprite_match.py playingVideos/<세션ID> --type geyser --anchor 936,518 --times 40,40.5,41
  python tools/sprite_match.py playingVideos/<세션ID> --type windVortex --anchor 1036,257 --start 40 --end 42 --clusters A
"""
import argparse
import glob
import json
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import recordplay_frames as rf  # noqa: E402
import shp  # noqa: E402

# 원본 게임 폴더 (같은 이름 클러스터를 순번으로 읽을 때 셰이프·팔레트를 직접 연다).
ORIGINALS_DIR = "originals"

# 내보낸 셰이프 폴더 (tools/shp.py export 의 기본 출력).
SHAPES_DIR = "extracted/shapes"
# 불투명으로 보는 알파 하한 (확대 보간 뒤 가장자리의 반투명 픽셀을 뺀다).
OPAQUE_ALPHA = 200
# 기준점 주변에서 위치를 다시 찾는 범위(캡처 픽셀). 카메라·반올림 오차를 흡수한다.
SEARCH_RADIUS = 3
# --locate 전체 탐색에서 프레임·스프라이트를 줄이는 배수.
COARSE_FACTOR = 4
# --locate 전체 탐색에서 원래 크기로 다시 맞춰 볼 후보 위치 수.
COARSE_KEEP = 12


def shape_folder(type_name):
    """타입 이름으로 내보낸 셰이프 폴더를 찾는다 (대소문자 무시)."""
    for path in glob.glob(os.path.join(SHAPES_DIR, "*_*")):
        if os.path.basename(path).split("_", 1)[1].lower() == type_name.lower():
            return path
    raise SystemExit(f"셰이프 폴더가 없습니다: {type_name} (tools/shp.py export 로 먼저 내보내세요)")


def load_sprites(type_name, scale, layer=0, prefix=None):
    """클러스터 이름 → (확대한 RGB 배열, 불투명 마스크, 기준점 대비 왼쪽 위 오프셋) 사전을 만든다."""
    folder = shape_folder(type_name)
    meta = json.load(open(os.path.join(folder, "meta.json"), encoding="utf-8"))
    sprites = {}
    # 같은 이름의 클러스터가 여러 개인 타입(예: windWalker 의 A01 36개)은 PNG 가 서로 덮어써져 구분할 수 없으므로
    # 원본 셰이프에서 순번대로 직접 읽고 "이름#순번" 으로 구분한다.
    names = [frame["name"] for frame in meta["frames"]]
    duplicated = len(set(names)) != len(names)
    shape_data = shape_block = palette = None
    if duplicated:
        shape_data = open(os.path.join(ORIGINALS_DIR, "d", "_shapes.shp"), "rb").read()
        shape_block = shp.read_blocks(shape_data)[meta["block"]]
        palette = shp.read_palette(os.path.join(ORIGINALS_DIR, shp.PALETTE_FILE))
    # meta 의 프레임마다 그림을 읽어 캡처 배율로 키운다
    for number, frame in enumerate(meta["frames"]):
        name = frame["name"]
        if frame.get("special") or not name.endswith(f"_L{layer}"):
            continue
        cluster = name[:-3]
        if prefix and not cluster.startswith(prefix):
            continue
        if duplicated:
            image = shp.frame_to_image(shape_data, shape_block.frames[number], palette)
            cluster = f"{cluster}#{number}"
        else:
            path = os.path.join(folder, name + ".png")
            if not os.path.exists(path):
                continue
            image = Image.open(path).convert("RGBA")
        width = max(1, round(image.width * scale))
        height = max(1, round(image.height * scale))
        big = np.asarray(image.resize((width, height), Image.BILINEAR)).astype(np.int32)
        mask = big[..., 3] >= OPAQUE_ALPHA
        if mask.sum() < 12:
            continue
        xmin, ymin = frame["rect"][0], frame["rect"][1]
        sprites[cluster] = (big[..., :3], mask, (xmin * scale, ymin * scale))
    return sprites


def score(frame, sprite, anchor, radius=SEARCH_RADIUS):
    """기준점 주변을 조금씩 옮겨 가며 가장 작은 평균 절대차와 그 위치 보정을 돌려준다."""
    rgb, mask, (ox, oy) = sprite
    height, width = mask.shape
    best = (1e9, 0, 0)
    # 기준점에서 radius 픽셀 안의 모든 위치를 시험한다
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            x0 = int(round(anchor[0] + ox)) + dx
            y0 = int(round(anchor[1] + oy)) + dy
            if x0 < 0 or y0 < 0 or x0 + width > frame.shape[1] or y0 + height > frame.shape[0]:
                continue
            patch = frame[y0:y0 + height, x0:x0 + width]
            diff = np.abs(patch[mask] - rgb[mask]).mean()
            if diff < best[0]:
                best = (float(diff), dx, dy)
    return best


def best_clusters(frame, sprites, anchor, top=3, radius=SEARCH_RADIUS):
    """모든 후보 클러스터의 (점수, 이름, 위치 보정 x, y)를 구해 잘 맞는 순서로 돌려준다."""
    results = []
    # 후보마다 가장 잘 맞는 위치와 점수를 구한다
    for name, sprite in sprites.items():
        value, dx, dy = score(frame, sprite, anchor, radius)
        results.append((value, name, dx, dy))
    return sorted(results)[:top]


def locate(frame, sprite, coarse=COARSE_FACTOR, keep=COARSE_KEEP):
    """프레임 전체에서 스프라이트가 가장 잘 맞는 기준점을 찾는다 (카메라 위치를 모를 때).

    먼저 프레임과 스프라이트를 coarse 배로 줄여 모든 위치의 평균 절대차를 구하고,
    잘 맞는 keep 곳만 원래 크기에서 다시 맞춘다. (점수, 기준점 x, 기준점 y)를 돌려준다.
    """
    rgb, mask, (ox, oy) = sprite
    height, width = mask.shape
    small_h, small_w = height // coarse, width // coarse
    if small_h < 2 or small_w < 2:
        raise SystemExit("스프라이트가 너무 작아 전체 탐색을 할 수 없습니다.")
    # 스프라이트를 coarse×coarse 칸 평균으로 줄인다 (칸 전체가 불투명한 곳만 비교에 쓴다)
    crop_rgb = rgb[:small_h * coarse, :small_w * coarse].reshape(small_h, coarse, small_w, coarse, 3).mean(axis=(1, 3))
    crop_mask = mask[:small_h * coarse, :small_w * coarse].reshape(small_h, coarse, small_w, coarse).all(axis=(1, 3))
    frame_h, frame_w = frame.shape[0] // coarse, frame.shape[1] // coarse
    small = frame[:frame_h * coarse, :frame_w * coarse].reshape(frame_h, coarse, frame_w, coarse, 3).mean(axis=(1, 3))
    ys, xs = np.nonzero(crop_mask)
    if len(ys) < 8:
        raise SystemExit("줄인 스프라이트에 불투명 칸이 너무 적습니다.")
    target = crop_rgb[ys, xs]
    rows, cols = frame_h - small_h + 1, frame_w - small_w + 1
    total = np.zeros((rows, cols))
    # 불투명 칸마다 그 칸이 놓일 프레임 영역 전체와의 차이를 더한다 (위치별 반복 대신 칸별 반복)
    for index in range(len(ys)):
        window = small[ys[index]:ys[index] + rows, xs[index]:xs[index] + cols]
        total += np.abs(window - target[index]).sum(axis=2)
    order = np.argsort(total, axis=None)[:keep]
    best = (1e9, 0.0, 0.0)
    # 후보 위치마다 원래 크기에서 coarse 픽셀 범위를 다시 맞춘다
    for flat in order:
        top, left = divmod(int(flat), cols)
        anchor = (left * coarse - ox, top * coarse - oy)
        value, dx, dy = score(frame, sprite, anchor, coarse)
        if value < best[0]:
            best = (value, anchor[0] + dx, anchor[1] + dy)
    return best


def main():
    """명령줄 인자를 해석해 시각별 최적 클러스터를 출력한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session", help="녹화 세션 폴더 (video-*.avi 와 frames.csv 가 있는 곳)")
    parser.add_argument("--type", required=True, help="타입 이름 (예: geyser)")
    parser.add_argument("--anchor", help="오브젝트 기준점의 캡처 화면 좌표 x,y (--locate 를 쓰면 생략)")
    parser.add_argument("--locate", action="store_true",
                        help="기준점을 모를 때: 프레임 전체에서 후보 클러스터마다 가장 잘 맞는 기준점을 찾아 출력한다")
    parser.add_argument("--scale", type=float, default=1.25, help="캡처 배율 (기본 1.25 = Windows 125%%)")
    parser.add_argument("--layer", type=int, default=0, help="맞출 레이어 (0 = 본체, 1 = 그림자)")
    parser.add_argument("--clusters", help="후보를 이 접두어로 시작하는 클러스터로 제한")
    parser.add_argument("--times", help="쉼표로 나눈 경과 초 목록")
    parser.add_argument("--start", type=float, help="연속 구간 시작 초")
    parser.add_argument("--end", type=float, help="연속 구간 끝 초")
    parser.add_argument("--step", type=int, default=1, help="연속 구간에서 몇 프레임마다 볼지")
    parser.add_argument("--top", type=int, default=3, help="시각마다 출력할 후보 수")
    parser.add_argument("--radius", type=int, default=SEARCH_RADIUS,
                        help="기준점 주변에서 위치를 다시 찾는 범위(캡처 픽셀). 기준점 규칙을 모를 때 크게 준다")
    args = parser.parse_args()
    frames = rf.load_frames(args.session)
    reader = rf.FrameReader(frames)
    if not args.locate and not args.anchor:
        raise SystemExit("--anchor 또는 --locate 가 필요합니다.")
    anchor = tuple(float(v) for v in args.anchor.split(",")) if args.anchor else None
    sprites = load_sprites(args.type, args.scale, args.layer, args.clusters)
    if args.times:
        numbers = [reader.nearest(float(t)) for t in args.times.split(",")]
    else:
        numbers = [n for n, f in enumerate(frames) if args.start <= f[2] <= args.end][::args.step]
    # 고른 프레임마다 가장 잘 맞는 클러스터를 출력한다
    for number in numbers:
        frame = np.asarray(reader.image(number)).astype(np.int32)
        if args.locate:
            # 후보 클러스터마다 전체 탐색을 하고 잘 맞는 순서로 기준점을 출력한다
            found = sorted((locate(frame, sprite) + (name,)) for name, sprite in sprites.items())[:args.top]
            text = "  ".join(f"{name}:{value:.1f}@({x:.0f},{y:.0f})" for value, x, y, name in found)
            print(f"{frames[number][2]:8.3f}s g{number}  {text}")
            continue
        ranked = best_clusters(frame, sprites, anchor, args.top, args.radius)
        text = "  ".join(f"{name}:{value:.1f}({dx:+d},{dy:+d})" for value, name, dx, dy in ranked)
        print(f"{frames[number][2]:8.3f}s g{number}  {text}")


if __name__ == "__main__":
    main()
