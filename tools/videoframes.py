# -*- coding: utf-8 -*-
"""로컬 플레이 영상(playingVideos/)에서 게임 화면만 잘라 프레임을 뽑는다.

영상은 원본 1024×768 풀스크린을 16:9 모니터(1920×1080)에서 녹화한 것이라
좌우에 검은 여백이 있다. 가운데 4:3 영역(1440×1080)을 잘라 원하는 크기로 맞춘다.
AV1 디코딩은 ffmpeg(libdav1d)에 맡긴다.

사용 예:
  python tools/videoframes.py probe  "playingVideos/…mp4"
  python tools/videoframes.py frame  "playingVideos/…mp4" 00:15:00 -o extracted/videos/da-15m.png
  python tools/videoframes.py range  "playingVideos/…mp4" 00:15:00 3 --fps 60 -o extracted/videos/da-15m
"""
import argparse
import collections
import json
import subprocess
import sys
from pathlib import Path

import numpy as np

# 원본 게임 화면 크기 (options.cfg 최대 해상도).
GAME_WIDTH = 1024
GAME_HEIGHT = 768
# 녹화 영상에서 게임 화면이 차지하는 영역 (2026-09-28 프레임 측정: x 240~1679, 전체 높이).
VIDEO_GAME_LEFT = 240
VIDEO_GAME_TOP = 0
VIDEO_GAME_WIDTH = 1440
VIDEO_GAME_HEIGHT = 1080
# 영상 픽셀 → 게임 픽셀 배율 (1080 / 768).
VIDEO_SCALE = VIDEO_GAME_HEIGHT / GAME_HEIGHT
# cadence: 게임 화면 왼쪽 사이드바 폭 (스크린샷 측정 약 82px, 여유를 둠).
SIDEBAR_WIDTH = 90
# cadence: 압축 잡음보다 큰 밝기 변화로 보는 회색조 차이.
PIXEL_CHANGE = 12
# cadence: 맵 영역에서 이 비율 이상 픽셀이 바뀌면 카메라 스크롤로 보고 제외한다.
SCROLL_RATIO = 0.05
# cadence: 분석 칸 크기 (게임 좌표 px).
CELL = 32
# cadence: 칸 평균 차이가 이 값을 넘으면 그 칸의 애니메이션이 바뀐 것으로 본다.
CELL_CHANGE = 6


def video_to_game(x, y):
    """녹화 영상 좌표를 원본 1024×768 게임 좌표로 바꾼다."""
    return (x - VIDEO_GAME_LEFT) / VIDEO_SCALE, (y - VIDEO_GAME_TOP) / VIDEO_SCALE


def crop_filter(native):
    """게임 영역만 남기는 ffmpeg 필터. native 이면 1024×768 로 축소하지 않는다."""
    crop = f"crop={VIDEO_GAME_WIDTH}:{VIDEO_GAME_HEIGHT}:{VIDEO_GAME_LEFT}:{VIDEO_GAME_TOP}"
    return crop if native else f"{crop},scale={GAME_WIDTH}:{GAME_HEIGHT}:flags=area"


def run_ffmpeg(args):
    """ffmpeg 를 오류만 출력하도록 실행하고 실패하면 종료한다."""
    result = subprocess.run(["ffmpeg", "-v", "error", "-y", *args])
    if result.returncode != 0:
        sys.exit(result.returncode)


def probe(video):
    """영상 코덱·해상도·프레임 속도·길이를 출력한다."""
    output = subprocess.run(
        ["ffprobe", "-v", "error", "-show_entries",
         "format=duration:stream=codec_name,codec_type,width,height,r_frame_rate", "-of", "json", video],
        capture_output=True, text=True, check=True).stdout
    info = json.loads(output)
    # 스트림마다 핵심 정보를 한 줄로 출력한다.
    for stream in info["streams"]:
        size = f" {stream['width']}x{stream['height']} {stream['r_frame_rate']}" if stream["codec_type"] == "video" else ""
        print(f"{stream['codec_type']}: {stream['codec_name']}{size}")
    seconds = float(info["format"]["duration"])
    print(f"길이: {int(seconds // 60)}분 {seconds % 60:.1f}초")


def frame(video, time, output, native):
    """지정 시각의 프레임 하나를 게임 영역만 잘라 저장한다."""
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    # -ss 를 입력 앞에 두어 빠르게 찾아간 뒤 한 장만 디코딩한다.
    run_ffmpeg(["-ss", time, "-i", video, "-frames:v", "1", "-vf", crop_filter(native), output])


def frame_range(video, start, duration, fps, output_dir, native):
    """구간의 프레임을 번호 붙은 PNG 로 저장한다 (시간 측정용)."""
    folder = Path(output_dir)
    folder.mkdir(parents=True, exist_ok=True)
    # 원본 60fps 보다 낮은 fps 를 주면 일정 간격으로 솎아 낸다.
    vf = f"{crop_filter(native)},fps={fps}"
    run_ffmpeg(["-ss", start, "-t", str(duration), "-i", video, "-vf", vf, str(folder / "%05d.png")])
    print(f"{folder}: 프레임 번호 n 의 시각 = {start} + (n-1)/{fps}초")


def read_gray_frames(video, start, duration):
    """구간을 게임 영역 1024×768 회색조 프레임 배열(프레임 수 × 768 × 1024)로 읽는다."""
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-ss", start, "-t", str(duration), "-i", video,
         "-vf", crop_filter(False) + ",format=gray", "-f", "rawvideo", "-"],
        capture_output=True, check=True).stdout
    size = GAME_WIDTH * GAME_HEIGHT
    count = len(raw) // size
    return np.frombuffer(raw[:count * size], np.uint8).reshape(count, GAME_HEIGHT, GAME_WIDTH).astype(np.int16)


def cadence(video, start, duration, max_y, top):
    """
    카메라가 멈춘 프레임에서 자주 바뀌는 32×32 칸을 찾아, 칸마다 변화 시점 사이의 영상 프레임 간격 분포를 출력한다.
    간격 2·3 이 번갈아 나오면 약 24Hz(60/2.5), 5 면 12Hz 로 애니메이션이 진행된 것이다.
    """
    frames = read_gray_frames(video, start, duration)
    diff = np.abs(np.diff(frames, axis=0))
    # 사이드바(x < SIDEBAR_WIDTH)를 뺀 맵 영역에서 크게 바뀐 픽셀 비율이 작으면 카메라가 멈춘 프레임이다
    moving = (diff[:, :, SIDEBAR_WIDTH:] > PIXEL_CHANGE).mean(axis=(1, 2))
    still = moving < SCROLL_RATIO
    rows, cols = GAME_HEIGHT // CELL, GAME_WIDTH // CELL
    cells = diff.reshape(len(diff), rows, CELL, cols, CELL).mean(axis=(2, 4))
    cells[~still] = 0
    score = (cells > CELL_CHANGE).sum(axis=0)
    score[:, :SIDEBAR_WIDTH // CELL + 1] = 0
    score[max_y // CELL:, :] = 0
    order = np.argsort(score.ravel())[::-1][:top]
    print(f"카메라 정지 프레임 비율 {still.mean():.2f} (영상 {len(frames)}프레임)")
    # 변화가 많은 칸부터 변화 간격 분포를 출력한다
    for index in order:
        cy, cx = divmod(int(index), cols)
        events = np.where(cells[:, cy, cx] > CELL_CHANGE)[0]
        gaps = collections.Counter(np.diff(events).tolist()).most_common(6)
        print(f"칸 ({cx * CELL},{cy * CELL}) 변화 {len(events)}회, 간격(영상 프레임):횟수 {gaps}")


def main():
    """명령줄 인자를 해석해 하위 명령을 실행한다."""
    # Windows 콘솔·Git Bash 모두에서 한글 안내가 깨지지 않도록 UTF-8 로 출력한다.
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("probe", help="영상 형식 확인")
    p.add_argument("video")
    f = sub.add_parser("frame", help="한 시각의 프레임 저장")
    f.add_argument("video")
    f.add_argument("time", help="예: 00:15:00 또는 900.5")
    f.add_argument("-o", "--output", required=True)
    f.add_argument("--native", action="store_true", help="1440×1080 그대로 저장 (축소하지 않음)")
    r = sub.add_parser("range", help="구간 프레임 저장")
    r.add_argument("video")
    r.add_argument("start")
    r.add_argument("duration", type=float, help="초")
    r.add_argument("--fps", type=float, default=60)
    r.add_argument("-o", "--output", required=True, help="저장 폴더")
    r.add_argument("--native", action="store_true")
    c = sub.add_parser("cadence", help="애니메이션 변화 간격 측정")
    c.add_argument("video")
    c.add_argument("start")
    c.add_argument("duration", type=float, help="초 (10~20초 권장)")
    c.add_argument("--max-y", type=int, default=GAME_HEIGHT, help="이 y(게임 좌표) 아래 칸은 제외 (전투 영역 배제용)")
    c.add_argument("--top", type=int, default=8, help="출력할 칸 수")
    args = parser.parse_args()
    if args.command == "probe":
        probe(args.video)
    elif args.command == "frame":
        frame(args.video, args.time, args.output, args.native)
    elif args.command == "range":
        frame_range(args.video, args.start, args.duration, args.fps, args.output, args.native)
    else:
        cadence(args.video, args.start, args.duration, args.max_y, args.top)


if __name__ == "__main__":
    main()
