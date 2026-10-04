# -*- coding: utf-8 -*-
"""
자동 분석 녹화 영상에서 건물 설치 미리보기 테두리를 읽어 "어느 칸에 놓을 수 있었는지" 지도를 복원한다 (원본 게임을 실행하지 않는다).

원본은 건물을 커서에 붙이면 커서가 가장 가까운 칸에 붙은 112x88 픽셀 테두리를 그린다. 놓을 수 있으면 순백(255,255,255),
놓을 수 없으면 순적색(255,0,0)이다. 영상 프레임마다 그 테두리의 위치와 색을 읽으면 훑는 동안 커서가 지난 모든 칸의
가능 여부를 얻는다. (훑기 스크립트가 메모리에만 남겼던 결과를 영상에서 되살리는 용도다.)

사용법:
    python tools/preview_scan_from_video.py <녹화 폴더> --from "2026-10-04T07:08:38Z" --to "2026-10-04T07:17:10Z" -o out.json
출력: {"valid": [[col,row],...], "invalid": [[col,row],...], "frames": N} (col = 테두리 왼쪽/16 + 4, row = (테두리 위 + 34)/11)
"""
import argparse
import csv
import io
import json
import sys
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import recordplay_frames as rf  # noqa: E402

# 테두리 크기와 칸 크기, 칸 → 테두리 변환 상수 (docs/videos/auto-test02-construct-20261004.md 의 실측)
FRAME_W, FRAME_H = 112, 88
CELL_W, CELL_H = 16, 11
COLS_LEFT, TOP_OFFSET = 4, 34
# 세로 테두리선으로 인정하는 최소 픽셀 수 (흰 선은 뚜렷하고, 붉은 선은 JPEG 에서 더 흐려진다)
MIN_COUNT_WHITE = 60
MIN_COUNT_RED = 45
# 도구 막대(왼쪽 84픽셀)는 지도가 아니므로 이 x 아래의 선은 무시한다
MAP_LEFT = 90


def parse_utc(text: str) -> datetime:
    """ISO 8601 UTC 문자열(소수 7자리 허용)을 datetime 으로 바꾼다"""
    text = text.strip().replace("Z", "+00:00")
    head, _, tail = text.partition(".")
    if tail:
        frac, zone = (tail[:tail.find("+")], tail[tail.find("+"):]) if "+" in tail else (tail, "+00:00")
        text = f"{head}.{frac[:6].ljust(6, '0')}{zone}"
    return datetime.fromisoformat(text)


def read_frame(img: Image.Image):
    """프레임 한 장에서 설치 테두리의 (col, row, 가능 여부)를 읽는다. 테두리가 없으면 None.
    영상은 JPEG 라 순백·순적색이 흐려지므로 임계값으로 판정한다: 흰 선 = 세 채널 모두 215 초과, 붉은 선 = R 170 초과·G·B 110 미만.
    세로 테두리선 두 줄(간격 111픽셀)이 모두 MIN_COUNT 개 이상의 픽셀을 가지면 테두리로 본다"""
    arr = np.asarray(img).astype(int)
    white = arr.min(axis=2) > 215
    red = (arr[..., 0] > 170) & (arr[..., 1] < 110) & (arr[..., 2] < 110)
    # 흰색·붉은색 순서로 세로선 한 쌍을 찾는다
    for mask, ok, need in ((white, True, MIN_COUNT_WHITE), (red, False, MIN_COUNT_RED)):
        counts = mask[:, MAP_LEFT:].sum(axis=0)
        candidates = {x + MAP_LEFT for x in np.nonzero(counts >= need)[0]}
        # 왼쪽 선 후보마다 오른쪽 선이 111픽셀 옆에 있는지 본다
        for x in sorted(candidates):
            if x + FRAME_W - 1 in candidates:
                rows = np.nonzero(mask[:, x] & mask[:, x + FRAME_W - 1])[0]
                if len(rows) < need:
                    continue
                col = round(x / CELL_W) + COLS_LEFT
                row = round((int(rows.min()) + TOP_OFFSET) / CELL_H)
                return col, row, ok
    return None


def main():
    """명령줄 진입점"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session")
    parser.add_argument("--from", dest="start", required=True)
    parser.add_argument("--to", dest="end", required=True)
    parser.add_argument("-o", "--output", required=True)
    args = parser.parse_args()
    t0, t1 = parse_utc(args.start), parse_utc(args.end)
    seen = {}
    frames = 0
    # 영상 조각마다 시각 파일을 읽어 구간 안의 프레임만 해석한다
    for timing in sorted(Path(args.session).glob("video-*.frames.csv")):
        rows = list(csv.DictReader(timing.open(encoding="utf-8")))
        if not rows or parse_utc(rows[-1]["utc"]) < t0 or parse_utc(rows[0]["utc"]) > t1:
            continue
        jpegs = rf.read_jpegs(timing.with_name(timing.name.replace(".frames.csv", ".avi")))
        # 구간 안의 프레임마다 테두리를 읽는다
        for index, row in enumerate(rows):
            if not (t0 <= parse_utc(row["utc"]) <= t1):
                continue
            frames += 1
            result = read_frame(Image.open(io.BytesIO(jpegs[index])).convert("RGB"))
            if result:
                col, row_no, ok = result
                # 같은 칸에서 한 번이라도 흰색이면 가능으로 본다 (전환 프레임의 잘못된 색을 걸러 내기 위해 다수결은 쓰지 않는다)
                seen.setdefault((col, row_no), [0, 0])[0 if ok else 1] += 1
    valid = sorted([c, r] for (c, r), (w, rd) in seen.items() if w > 0)
    invalid = sorted([c, r] for (c, r), (w, rd) in seen.items() if w == 0)
    Path(args.output).write_text(json.dumps({"valid": valid, "invalid": invalid, "frames": frames,
                                             "counts": {f"{c},{r}": v for (c, r), v in seen.items()}}), encoding="utf-8")
    print(f"프레임 {frames}장, 가능 {len(valid)}칸, 불가 {len(invalid)}칸")


if __name__ == "__main__":
    main()
