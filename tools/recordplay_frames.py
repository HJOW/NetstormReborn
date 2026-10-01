# -*- coding: utf-8 -*-
"""record-play/guide 녹화 세션의 MJPEG AVI 에서 프레임을 꺼내거나 관찰표를 만든다. 원본 게임을 실행하지 않는다.

analyzeManager 녹화는 AVI 조각마다 프레임 번호가 1부터 다시 시작하고, 캡처가 잠시 멈추면
프레임 사이 간격이 0.1초보다 길어진다. 따라서 시각은 프레임 번호가 아니라 각 조각의
`video-000N.frames.csv` 의 `sessionElapsedMs` 로 계산한다. 시각 0 은 첫 조각의 첫 프레임이며
`tools/audiomatch.py` 의 영상 경과 시각과 같은 기준이다. FFmpeg 없이 RIFF `00dc` 청크를 직접 읽는다.

사용 예:
  python tools/recordplay_frames.py frames playingVideos/<세션ID> -o extracted/frames 660.5 g6596
  python tools/recordplay_frames.py sheet playingVideos/<세션ID> -o extracted/sheet --step 50
  python tools/recordplay_frames.py sheet playingVideos/<세션ID> -o extracted/altar --step 5 \
      --start 870 --end 885 --crop 400,180,300,200
"""
import argparse
import csv
import io
import math
from pathlib import Path

from PIL import Image, ImageDraw

# 관찰표 한 줄의 칸 수.
SHEET_COLUMNS = 5
# 관찰표 한 장의 줄 수.
SHEET_ROWS = 6
# 칸 아래 시각 글자를 적는 여백(픽셀).
LABEL_HEIGHT = 18


def load_frames(session):
    """세션 폴더의 모든 조각을 이어 (AVI 경로, 조각 안 순번, 경과 초) 목록을 만든다."""
    frames = []
    base = None
    # 조각 번호 순서대로 시각 파일을 읽는다
    for timing in sorted(Path(session).glob("video-*.frames.csv")):
        avi = timing.with_name(timing.name.replace(".frames.csv", ".avi"))
        # 조각 안의 프레임마다 경과 초를 계산한다
        for index, row in enumerate(csv.DictReader(timing.open(encoding="utf-8"))):
            ms = float(row["sessionElapsedMs"])
            base = ms if base is None else base
            frames.append((avi, index, (ms - base) / 1000))
    if not frames:
        raise SystemExit(f"video-*.frames.csv 가 없습니다: {session}")
    return frames


def read_jpegs(avi):
    """MJPEG AVI 의 영상 청크(00dc, JPEG 한 장씩)를 순서대로 돌려준다."""
    data = avi.read_bytes()
    pos = data.find(b"movi") + 4
    out = []
    # RIFF 청크를 차례로 따라가며 영상 청크만 모은다
    while pos + 8 <= len(data):
        tag = data[pos:pos + 4]
        size = int.from_bytes(data[pos + 4:pos + 8], "little")
        if tag == b"LIST":
            pos += 12
            continue
        if tag == b"idx1":
            break
        if tag == b"00dc":
            out.append(data[pos + 8:pos + 8 + size])
        pos += 8 + size + (size & 1)
    return out


class FrameReader:
    """최근에 연 AVI 조각 하나를 기억해 두고 전역 프레임 번호로 이미지를 꺼낸다."""

    def __init__(self, frames):
        """전역 프레임 목록을 받는다."""
        self.frames = frames
        self.avi = None
        self.jpegs = []

    def image(self, number):
        """전역 프레임 번호의 RGB 이미지를 돌려준다."""
        avi, index, _ = self.frames[number]
        if avi != self.avi:
            self.avi, self.jpegs = avi, read_jpegs(avi)
        return Image.open(io.BytesIO(self.jpegs[index])).convert("RGB")

    def nearest(self, seconds):
        """경과 초에 가장 가까운 전역 프레임 번호를 돌려준다."""
        return min(range(len(self.frames)), key=lambda k: abs(self.frames[k][2] - seconds))


def label(seconds, number):
    """관찰표·파일 이름에 쓰는 'mm:ss.s gN' 형식 글자."""
    return f"{int(seconds // 60):02d}:{seconds % 60:04.1f}  g{number}"


def cmd_frames(args):
    """지정한 경과 초(또는 g전역번호)의 프레임을 원본 크기 PNG 로 저장한다."""
    reader = FrameReader(load_frames(args.session))
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    # 요청한 시각마다 가장 가까운 프레임을 저장한다
    for spec in args.times:
        number = int(spec[1:]) if spec.startswith("g") else reader.nearest(float(spec))
        seconds = reader.frames[number][2]
        path = out / f"f{seconds:07.1f}-g{number}.png"
        reader.image(number).save(path)
        print(path)


def cmd_sheet(args):
    """step 프레임마다 뽑아 시각을 적은 관찰표 PNG 를 만든다(crop 이면 원본 픽셀로 잘라 붙인다)."""
    reader = FrameReader(load_frames(args.session))
    crop = tuple(int(v) for v in args.crop.split(",")) if args.crop else None
    picked = [n for n, f in enumerate(reader.frames)
              if n % args.step == 0 and args.start <= f[2] <= args.end]
    cells = []
    # 고른 프레임을 잘라 내거나 축소한다
    for number in picked:
        img = reader.image(number)
        if crop:
            img = img.crop((crop[0], crop[1], crop[0] + crop[2], crop[1] + crop[3]))
        else:
            img = img.resize((args.width, args.width * img.height // img.width))
        cells.append((number, reader.frames[number][2], img))
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    per_sheet = SHEET_COLUMNS * SHEET_ROWS
    # 관찰표 한 장씩 그려 저장한다
    for first in range(0, len(cells), per_sheet):
        chunk = cells[first:first + per_sheet]
        cell_w, cell_h = chunk[0][2].size
        sheet = Image.new("RGB", (SHEET_COLUMNS * cell_w,
                                  math.ceil(len(chunk) / SHEET_COLUMNS) * (cell_h + LABEL_HEIGHT)), "#202020")
        draw = ImageDraw.Draw(sheet)
        # 왼쪽 위부터 시간순으로 붙인다
        for k, (number, seconds, img) in enumerate(chunk):
            x = k % SHEET_COLUMNS * cell_w
            y = k // SHEET_COLUMNS * (cell_h + LABEL_HEIGHT)
            sheet.paste(img, (x, y))
            draw.text((x + 4, y + cell_h + 3), label(seconds, number), fill="yellow")
        path = out / f"sheet-{chunk[0][1]:07.1f}.png"
        sheet.save(path)
        print(path)


def main():
    """명령줄 인자를 해석해 하위 명령을 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("frames", help="지정 시각의 원본 크기 프레임 저장")
    p.add_argument("session")
    p.add_argument("-o", "--output", required=True)
    p.add_argument("times", nargs="+", help="경과 초(예: 660.5) 또는 g전역번호(예: g6596)")
    p.set_defaults(func=cmd_frames)
    p = sub.add_parser("sheet", help="시각이 적힌 관찰표 생성")
    p.add_argument("session")
    p.add_argument("-o", "--output", required=True)
    p.add_argument("--step", type=int, default=50, help="몇 프레임마다 뽑을지 (10 FPS 기준 50 = 5초)")
    p.add_argument("--start", type=float, default=0.0, help="시작 경과 초")
    p.add_argument("--end", type=float, default=1e9, help="끝 경과 초")
    p.add_argument("--width", type=int, default=320, help="crop 없을 때 축소 폭")
    p.add_argument("--crop", help="x,y,w,h — 원본 픽셀 영역만 잘라 붙인다")
    p.set_defaults(func=cmd_sheet)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
