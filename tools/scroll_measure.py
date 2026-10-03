# -*- coding: utf-8 -*-
"""record-play/guide 녹화에서 화면(카메라) 이동 속도를 잰다. 원본 게임을 실행하지 않는다.

연속한 두 프레임 사이에 지도가 몇 픽셀 움직였는지를 위상 상관(FFT)으로 구하고,
같은 시각의 커서 위치·눌린 버튼·눌린 키를 입력 JSONL 에서 찾아 표로 만든다.
그다음 "카메라 속도 = 상수 x (커서 - 화면 중심)" 이라는 선형 모델을 최소제곱으로 맞춘다.
Alt 누름·가운데 버튼 누름처럼 커서 방향으로 화면이 움직이는 조작을 분석하는 데 쓴다.

주의: 화면 전체가 바뀌는 조작(미니맵 누르기, 화면 전환)에서는 위상 상관이 틀리므로 쓸 수 없다.
이런 구간은 q(상관 최고값)가 낮게 나오며 --min-q 로 걸러 낸다.

사용 예:
  python tools/scroll_measure.py playingVideos/<세션ID> --start 15.8 --end 23.2
  python tools/scroll_measure.py playingVideos/<세션ID> --start 277 --end 280 --table
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np

import recordplay_frames as rf

# 지도 영역에서 왼쪽 도구 막대(폭 약 105px)를 빼기 위한 왼쪽 여백(1280x960 캡처 기준 픽셀)
MAP_LEFT = 110
# 지도 영역 아래쪽 한계. 그 아래는 미니맵이 있는 구간이라 위상 상관에서 제외한다
MAP_BOTTOM = 860
# 회귀에 넣을 최소 상관 최고값. 이보다 낮으면 이동량 추정이 믿을 수 없다고 본다
DEFAULT_MIN_Q = 0.15


def first_frame_ms(session):
    """세션의 첫 영상 프레임의 sessionElapsedMs 를 돌려준다(영상 시각 0 의 기준)."""
    timing = sorted(Path(session).glob("video-*.frames.csv"))[0]
    # 첫 행만 읽으면 된다
    with timing.open(encoding="utf-8") as handle:
        return float(next(csv.DictReader(handle))["sessionElapsedMs"])


def load_inputs(session, base_ms):
    """모든 input-*.jsonl 을 (영상 초, 입력 딕셔너리) 목록으로 시간순으로 읽는다."""
    events = []
    # 입력 로그는 4 MB 단위로 나뉘어 있을 수 있으므로 번호 순서대로 모두 읽는다
    for path in sorted(Path(session).glob("input-*.jsonl")):
        # 한 줄이 이벤트 하나다
        for line in path.open(encoding="utf-8"):
            row = json.loads(line)
            events.append(((row["sessionElapsedMs"] - base_ms) / 1000, row["input"]))
    events.sort(key=lambda item: item[0])
    return events


class InputState:
    """시각을 앞으로만 진행시키며 커서 위치·눌린 버튼·눌린 키를 갱신한다."""

    def __init__(self, events):
        """정렬된 이벤트 목록을 받아 처음 상태(커서 없음)로 시작한다."""
        self.events = events
        self.index = 0
        self.cursor = None
        self.buttons = set()
        self.keys = set()

    def advance(self, seconds):
        """영상 초 seconds 까지의 이벤트를 모두 반영한다. 시각은 되돌릴 수 없다."""
        # 아직 반영하지 않은 이벤트 중 seconds 이하인 것을 차례로 적용한다
        while self.index < len(self.events) and self.events[self.index][0] <= seconds:
            data = self.events[self.index][1]
            self.index += 1
            if data["type"] == "mouse":
                if "x" in data:
                    self.cursor = (data["x"], data["y"])
                if data["action"] == "down":
                    self.buttons.add(data["button"])
                elif data["action"] == "up":
                    self.buttons.discard(data["button"])
            elif data["type"] == "keyboard":
                if data["action"] == "down":
                    self.keys.add(data["key"])
                else:
                    self.keys.discard(data["key"])


def map_gray(reader, number):
    """프레임의 지도 영역만 흑백 실수 배열로 읽는다(도구 막대·미니맵 제외)."""
    image = reader.image(number).convert("L")
    return np.asarray(image, dtype=np.float32)[0:MAP_BOTTOM, MAP_LEFT:]


def content_shift(before, after):
    """before 에서 after 로 지도 내용이 움직인 (dx, dy)와 상관 최고값 q 를 위상 상관으로 구한다."""
    # 가장자리 불연속이 만드는 잡음을 줄이려고 한 창(window)을 곱한다
    window = np.hanning(before.shape[0])[:, None] * np.hanning(before.shape[1])[None, :]
    spec = np.fft.fft2(before * window) * np.conj(np.fft.fft2(after * window))
    spec /= np.abs(spec) + 1e-6
    corr = np.fft.ifft2(spec).real
    peak_y, peak_x = np.unravel_index(np.argmax(corr), corr.shape)
    height, width = corr.shape
    # 반 바퀴를 넘는 값은 음수 이동으로 되돌린다
    dy = peak_y if peak_y < height // 2 else peak_y - height
    dx = peak_x if peak_x < width // 2 else peak_x - width
    return -dx, -dy, float(corr.max())


def fit_line(xs, ys):
    """y = k*(x - c) 직선을 최소제곱으로 맞춰 (k, c, 상관계수)를 돌려준다."""
    slope, intercept = np.polyfit(xs, ys, 1)
    return float(slope), float(-intercept / slope), float(np.corrcoef(xs, ys)[0, 1])


def measure(session, start, end, min_q):
    """구간의 프레임마다 (영상 초, 이동량, q, 이전 프레임의 커서, 버튼, 키) 행을 만든다."""
    frames = rf.load_frames(session)
    reader = rf.FrameReader(frames)
    state = InputState(load_inputs(session, first_frame_ms(session)))
    rows = []
    previous = None
    previous_state = None
    # 구간 안의 모든 프레임을 차례로 본다
    for number, (_, _, seconds) in enumerate(frames):
        if not start <= seconds <= end:
            continue
        gray = map_gray(reader, number)
        # 이번 프레임이 아니라 "이전 프레임 시각"의 입력이 이번 이동의 원인이므로 그 상태를 함께 저장한다
        if previous is not None:
            dx, dy, q = content_shift(previous, gray)
            rows.append((seconds, dx, dy, q, previous_state))
        state.advance(seconds)
        previous = gray
        previous_state = (state.cursor, frozenset(state.buttons), frozenset(state.keys))
    return rows, frames[1][2] - frames[0][2]


def main():
    """명령줄 인자를 해석하고 표와 회귀 결과를 출력한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session", help="playingVideos/<세션ID> 또는 recording 폴더")
    parser.add_argument("--start", type=float, required=True, help="시작 영상 초")
    parser.add_argument("--end", type=float, required=True, help="끝 영상 초")
    parser.add_argument("--min-q", type=float, default=DEFAULT_MIN_Q, help="회귀에 쓸 최소 상관 최고값")
    parser.add_argument("--table", action="store_true", help="프레임별 표도 출력")
    args = parser.parse_args()
    # Windows 콘솔의 기본 코드 페이지에서도 한글이 깨지지 않게 UTF-8 로 출력한다
    sys.stdout.reconfigure(encoding="utf-8")

    rows, frame_dt = measure(args.session, args.start, args.end, args.min_q)
    # 프레임 간격이 일정하지 않으므로 실제 시각 차이로 속도를 계산한다
    previous_time = None
    samples = {"middle": [], "alt": [], "any": []}
    # 프레임별 행을 훑으며 표를 찍고 회귀용 표본을 모은다
    for seconds, dx, dy, q, (cursor, buttons, keys) in rows:
        dt = frame_dt if previous_time is None else seconds - previous_time
        previous_time = seconds
        if args.table:
            print(f"{seconds:7.2f} d=({dx:5d},{dy:5d}) q={q:4.2f} cursor={cursor} buttons={sorted(buttons)} keys={sorted(keys)}")
        if cursor is None or q < args.min_q or dt <= 0:
            continue
        # 카메라 속도는 지도 내용 이동의 반대 방향이다(px/s)
        sample = (cursor[0], cursor[1], -dx / dt, -dy / dt)
        if "middle" in buttons and not keys:
            samples["middle"].append(sample)
        if any(key in ("LeftAlt", "RightAlt") for key in keys) and not buttons:
            samples["alt"].append(sample)
        if buttons or keys:
            samples["any"].append(sample)

    # 조작 종류별로 가로·세로 직선을 맞춘다
    for name, items in samples.items():
        if len(items) < 5:
            print(f"{name}: 표본 {len(items)}개 - 너무 적어 건너뜀")
            continue
        data = np.array(items, dtype=float)
        kx, cx, rx = fit_line(data[:, 0], data[:, 2])
        ky, cy, ry = fit_line(data[:, 1], data[:, 3])
        peak = float(np.abs(data[:, 2:]).max())
        print(f"{name}: n={len(items)} x: k={kx:.2f}/s 중심={cx:.0f} r={rx:.3f} | y: k={ky:.2f}/s 중심={cy:.0f} r={ry:.3f} | 최대 {peak:.0f}px/s")


if __name__ == "__main__":
    main()
