#!/usr/bin/env python3
"""영상의 지정 시각들을 자막 시각이 붙은 PNG 한 장으로 모은다. 원본 게임을 실행하지 않는다.

기존 videoframes.py의 방송 영상 전용 잘라내기를 적용하지 않는다.
예: python tools/video_contact.py "playingVideos/[Youtube] 3-1 to 3-5.mp4" --start 0 --step 120 --count 18 -o extracted/contact.png
"""
import argparse
import io
import math
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw

# 축소 프레임 아래 시각을 적을 여백(픽셀).
LABEL_HEIGHT = 24


def contact(video, start, step, count, columns, width, crop, output):
    """각 시각으로 탐색해 프레임을 가져오고 위치·시각을 보존한 관찰표를 저장한다."""
    frames = []
    # 원하는 시각만 디코딩하므로 긴 영상 전체를 순차 해독하지 않는다.
    for index in range(count):
        seconds = start + step * index
        filters = f"crop={crop}," if crop else ""
        result = subprocess.run([
            "ffmpeg", "-v", "error", "-ss", str(seconds), "-i", str(video),
            "-frames:v", "1", "-vf", filters + f"scale={width}:-1",
            "-f", "image2pipe", "-vcodec", "png", "-",
        ], check=True, capture_output=True)
        if not result.stdout:
            raise ValueError(f"영상 범위를 벗어난 시각: {seconds}초")
        frames.append((seconds, Image.open(io.BytesIO(result.stdout)).convert("RGB")))
    cell_height = frames[0][1].height + LABEL_HEIGHT
    sheet = Image.new("RGB", (columns * width, math.ceil(count / columns) * cell_height), "#202020")
    draw = ImageDraw.Draw(sheet)
    # 프레임을 왼쪽 위부터 시간순으로 배치한다.
    for index, (seconds, frame) in enumerate(frames):
        x, y = index % columns * width, index // columns * cell_height
        sheet.paste(frame, (x, y))
        label = f"{int(seconds // 3600):02}:{int(seconds // 60) % 60:02}:{seconds % 60:05.2f}"
        draw.text((x + 8, y + frame.height + 4), label, fill="white")
    output.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output)
    print(output)


def main():
    """명령행 인자를 검사해 관찰표를 만든다. crop은 FFmpeg의 폭:높이:x:y 형식이다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("video", type=Path)
    parser.add_argument("--start", type=float, default=0)
    parser.add_argument("--step", type=float, default=120)
    parser.add_argument("--count", type=int, default=18)
    parser.add_argument("--columns", type=int, default=3)
    parser.add_argument("--width", type=int, default=420)
    parser.add_argument("--crop", help="폭:높이:x:y (생략하면 전체 화면)")
    parser.add_argument("-o", "--output", type=Path, required=True)
    args = parser.parse_args()
    if args.start < 0 or args.step <= 0 or min(args.count, args.columns, args.width) < 1:
        parser.error("시작은 0 이상, 간격·개수·열·폭은 양수여야 합니다.")
    contact(args.video, args.start, args.step, args.count, args.columns, args.width, args.crop, args.output)


if __name__ == "__main__":
    main()
