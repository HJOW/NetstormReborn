# -*- coding: utf-8 -*-
"""녹화 영상에서 한 버튼 영역이 "평소 모양"인지 "눌린 모양"인지 프레임마다 판정해 시간표로 보여 준다.

원본 게임을 실행하지 않고 analyzeManager 자동·사용자 녹화(`video-*.avi`, `*.frames.csv`)만 읽는다.
버튼 위에서 누른 채 커서를 옮기는 시험(바깥으로 나갔다 돌아오기 등)에서 눌림 표시가 언제 바뀌는지 읽는 데 쓴다.
기준 모양 두 개는 사용자가 시각으로 지정한다: 평소 모양(누르기 전)과 눌린 모양(버튼 위에서 길게 누르는 중).
각 프레임의 관심 영역과 두 기준의 평균 절대차를 구해 더 가까운 쪽을 고르고, 둘 다 멀면 "다름"이다.

사용 예:
  python tools/button_state.py extracted/analyzeManager/<세션ID>/recording --box 430,330,85,26 \
      --normal 112.0 --pressed 114.5 --start 131.5 --end 135.0
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import recordplay_frames as rf  # noqa: E402

# 두 기준과의 평균 절대차가 모두 이 값보다 크면 평소도 눌림도 아닌 "다름"으로 본다.
UNKNOWN_DIFF = 12.0


def region(reader, number, box):
    """프레임 번호의 관심 영역(x, y, 폭, 높이)을 RGB 정수 배열로 잘라 돌려준다."""
    x, y, w, h = box
    return np.asarray(reader.image(number).convert("RGB")).astype(int)[y:y + h, x:x + w]


def classify(current, normal, pressed):
    """현재 영역을 평소/눌림/다름으로 분류하고 두 차이값을 함께 돌려준다."""
    to_normal = float(np.abs(current - normal).mean())
    to_pressed = float(np.abs(current - pressed).mean())
    if min(to_normal, to_pressed) > UNKNOWN_DIFF:
        return "다름", to_normal, to_pressed
    return ("평소" if to_normal <= to_pressed else "눌림"), to_normal, to_pressed


def main():
    """명령줄 인자를 읽어 구간의 프레임마다 상태를 출력하고, 상태가 바뀐 시각만 따로 요약한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session", help="녹화 폴더")
    parser.add_argument("--box", required=True, help="관심 영역 x,y,폭,높이 (클라이언트 픽셀)")
    parser.add_argument("--normal", type=float, required=True, help="평소 모양인 영상 시각(초)")
    parser.add_argument("--pressed", type=float, required=True, help="눌린 모양인 영상 시각(초)")
    parser.add_argument("--start", type=float, required=True, help="판정 구간 시작(초)")
    parser.add_argument("--end", type=float, required=True, help="판정 구간 끝(초)")
    parser.add_argument("--all", action="store_true", help="모든 프레임을 출력 (생략하면 상태가 바뀐 프레임만)")
    args = parser.parse_args()
    box = tuple(int(v) for v in args.box.split(","))
    frames = rf.load_frames(args.session)
    reader = rf.FrameReader(frames)
    normal = region(reader, reader.nearest(args.normal), box)
    pressed = region(reader, reader.nearest(args.pressed), box)
    previous = None
    # 구간 안의 프레임을 시간 순서로 판정한다
    for number, frame in enumerate(frames):
        if not (args.start <= frame[2] <= args.end):
            continue
        state, to_normal, to_pressed = classify(region(reader, number, box), normal, pressed)
        if args.all or state != previous:
            print(f"{frame[2]:8.3f}s  {state:<3s}  평소와 {to_normal:5.1f}  눌림과 {to_pressed:5.1f}")
        previous = state


if __name__ == "__main__":
    main()
