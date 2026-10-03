# -*- coding: utf-8 -*-
"""입력 사건(마우스 누름·뗌, 키 누름) 주변의 소리를 원본 효과음과 대조해 "어느 사건에서 어느 소리가 났는지" 표로 만든다.

analyzeManager 의 자동 녹화·사용자 녹화는 입력 로그(`input-*.jsonl`)와 Windows 기본 출력 소리(`audio-*.wav`)를 함께 남긴다.
`tools/audiomatch.py sfx` 는 녹음 전체에서 소리를 찾지만, 이 도구는 **입력 사건 앞뒤의 짧은 창만** 검사하므로
사건과 소리의 시간차(누를 때 나는지, 뗄 때 나는지)를 밀리초 단위로 읽을 수 있고 짧은 효과음(0.1초대)도 찾는다.
시각은 모두 영상 0초(첫 프레임) 기준이며, 소리 시각은 녹음 시작 오프셋을 보정한 값이다.

사용 예:
  python tools/audio_events.py extracted/analyzeManager/<세션ID>/recording
  python tools/audio_events.py extracted/analyzeManager/<세션ID>/recording --start 110 --end 140 --threshold 0.4
  python tools/audio_events.py <녹화 폴더> --sounds originals/sound/button.wav originals/sound/opengump.wav
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audiomatch as am  # noqa: E402

# 사건 앞에서부터 검사하는 시간(초). 소리가 입력보다 먼저 나는 일은 없지만 시각 보정 오차를 흡수한다.
WINDOW_BEFORE = 0.25
# 사건 뒤로 검사하는 시간(초). 뗄 때 나는 소리와 창 열기 소리까지 담는다.
WINDOW_AFTER = 0.9
# 일치로 인정하는 정규화 상관 하한.
DEFAULT_THRESHOLD = 0.45
# 짧은 효과음도 틀로 쓰기 위한 최소 길이(초). select.wav(0.104초)보다 작게 둔다.
MIN_TEMPLATE_SEC = 0.09
# 한 사건 창에서 보여 줄 소리 후보 수의 상한.
MAX_SHOWN = 4
# 고역 강조 계수. 배경 음악의 저음이 짧은 효과음의 상관을 떨어뜨리므로 y[n] = x[n] − 계수 × x[n−1] 로 저음을 줄인다.
PRE_EMPHASIS = 0.95


def load_events(folder, start, end):
    """입력 로그에서 [start, end] 영상 시각 범위의 마우스 누름·뗌·키 입력을 (시각, 설명) 목록으로 읽는다."""
    with open(folder / "video-0001.frames.csv", encoding="utf-8") as f:
        zero = float(next(csv.DictReader(f))["sessionElapsedMs"]) / 1000
    events = []
    # 입력 로그 파일마다 한 줄씩 읽는다
    for path in sorted(folder.glob("input-*.jsonl")):
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            row = json.loads(line)
            item = row["input"]
            when = row["sessionElapsedMs"] / 1000 - zero
            if not (start <= when <= end):
                continue
            kind = item.get("type")
            if kind == "mouse" and item.get("action") in ("down", "up"):
                events.append((when, f"마우스 {item['button']} {item['action']} ({item['x']},{item['y']})"))
            elif kind == "keyboard" and item.get("action") in ("down", "up"):
                events.append((when, f"키 {item.get('key', item.get('vk', '?'))} {item['action']}"))
    return sorted(events)


def emphasize(signal):
    """저음을 줄이는 1차 고역 강조 필터를 적용한 새 배열을 돌려준다."""
    return np.concatenate(([signal[0]], signal[1:] - PRE_EMPHASIS * signal[:-1])) if len(signal) else signal


def load_templates(sounds):
    """대상 효과음(생략하면 원본 효과음 전체)을 읽어 앞뒤 무음을 자른 (이름, 신호) 목록으로 만든다."""
    paths = [Path(p) for p in sounds] if sounds else sorted(
        p for p in am.SOUND_DIR.glob("*") if p.suffix.lower() == ".wav")
    templates = []
    # 소리 파일마다 디코딩하고 너무 짧은 것은 건너뛴다
    for path in paths:
        template = am.trim(am.decode_mono(path))
        if len(template) >= MIN_TEMPLATE_SEC * am.RATE:
            templates.append((path.name, emphasize(template)))
    return templates


def probe(signal, offset, when, templates, threshold):
    """사건 시각 앞뒤 창에서 효과음마다 가장 잘 맞는 위치를 찾아 (사건 대비 지연 ms, 소리, 상관) 목록으로 돌려준다."""
    first = int(max(0.0, when - offset - WINDOW_BEFORE) * am.RATE)
    last = int((when - offset + WINDOW_AFTER) * am.RATE)
    window = emphasize(signal[first:last])
    found = []
    # 효과음마다 창 안의 상관 봉우리를 확인한다
    for name, template in templates:
        if len(window) < len(template):
            continue
        score = am.Correlator(window, len(template)).ncc(template)
        best = int(np.argmax(score))
        if score[best] >= threshold:
            at = (first + best) / am.RATE + offset
            found.append((round((at - when) * 1000), name, float(score[best])))
    found.sort(key=lambda row: -row[2])
    return found[:MAX_SHOWN]


def main():
    """명령줄 인자를 읽어 입력 사건마다 근처의 효과음을 출력한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("session", help="녹화 폴더 (video-0001.frames.csv·input-*.jsonl·audio-*.wav 가 있는 곳)")
    parser.add_argument("--start", type=float, default=0.0, help="검사할 영상 시각 범위의 시작(초)")
    parser.add_argument("--end", type=float, default=1e9, help="검사할 영상 시각 범위의 끝(초)")
    parser.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD, help="일치로 볼 정규화 상관 하한")
    parser.add_argument("--sounds", nargs="*", help="대상 소리 파일 (생략하면 originals/sound 전체)")
    args = parser.parse_args()
    folder = Path(args.session)
    signal, offset = am.load_session(folder)
    templates = load_templates(args.sounds)
    print(f"효과음 {len(templates)}개, 녹음 {len(signal) / am.RATE:.1f}초 (영상 대비 오프셋 {offset:+.3f}초)", file=sys.stderr)
    # 사건마다 한 줄 + 소리 후보 줄을 출력한다
    for when, text in load_events(folder, args.start, args.end):
        print(f"{when:8.3f}s {text}")
        for delay, name, ncc in probe(signal, offset, when, templates, args.threshold):
            print(f"           └ 사건 {delay:+5d}ms  {name:<22s} ncc {ncc:.3f}")


if __name__ == "__main__":
    main()
