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
import json
import subprocess
import sys
from pathlib import Path

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
    args = parser.parse_args()
    if args.command == "probe":
        probe(args.video)
    elif args.command == "frame":
        frame(args.video, args.time, args.output, args.native)
    else:
        frame_range(args.video, args.start, args.duration, args.fps, args.output, args.native)


if __name__ == "__main__":
    main()
