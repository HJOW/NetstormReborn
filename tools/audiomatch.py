# -*- coding: utf-8 -*-
"""녹화한 게임 소리에서 원본 효과음·음악이 재생된 시점을 찾는다.

analyzeManager 의 record-play/guide 녹화는 Windows 기본 출력 장치의 소리를
별도 WAV(48kHz float 스테레오)로 남긴다. 이 도구는 녹음과 원본
`originals/sound/*.wav`·`originals/music/*.mus`(RIFF WAV)를 같은 표본율의
모노 신호로 바꾼 뒤, FFT 정규화 상호상관으로 일치 위치를 찾는다.

시각은 세션 폴더의 `video-0001.frames.csv` 첫 프레임 UTC 를 0 으로 하는
**영상 경과 시각**으로 출력한다(관찰 노트와 같은 기준).

세션 폴더 대신 **영상·소리 파일 하나**(예: analyzeManager `youtube_clip` 의 소리 포함 mp4)를 줄 수 있다.
이때 `--offset 초`(구간 시작 시각)를 더해 원본 영상 시각으로 출력한다. 방송 음성이 섞인 영상은
상관값이 낮아지므로 `--threshold` 를 0.3 안팎으로 낮추고 `--sounds` 로 대상을 좁혀 쓴다.

사용 예:
  python tools/audiomatch.py sfx   playingVideos/<세션ID> -o extracted/audio/<세션ID>-sfx.csv
  python tools/audiomatch.py music playingVideos/<세션ID> -o extracted/audio/<세션ID>-music.csv
  python tools/audiomatch.py level playingVideos/<세션ID>
  python tools/audiomatch.py sfx extracted/youtube/<ID>/clips/<구간>-a.mp4 --offset 800 --threshold 0.3 \
      --sounds originals/sound/forWind2.WAV originals/sound/itIsDone2.wav
"""
import argparse
import csv
import datetime as dt
import subprocess
import sys
from pathlib import Path

import numpy as np

# 분석 표본율. 원본 효과음 대부분이 22,050Hz 이므로 그 절반으로 줄여 계산량을 낮춘다.
RATE = 11025
# 원본 소리 폴더 (저장소 기준 경로).
SOUND_DIR = Path("originals/sound")
MUSIC_DIR = Path("originals/music")
# 효과음 앞뒤의 무음을 잘라 낼 때 쓰는 진폭 기준 (최대 진폭 대비 비율).
TRIM_RATIO = 0.02
# 너무 짧은 효과음은 우연 일치가 많으므로 이 길이(초) 미만은 건너뛴다.
MIN_TEMPLATE_SEC = 0.12
# 효과음 일치로 인정하는 정규화 상관 하한 (기본값, 명령줄로 바꿀 수 있다).
SFX_THRESHOLD = 0.55
# 녹음 구간 에너지가 이 값보다 작으면(거의 무음) 상관을 0 으로 본다.
ENERGY_FLOOR = 1e-6
# 음악 판정에 쓰는 녹음 조각 길이(초)와 간격(초).
MUSIC_WINDOW_SEC = 6.0
MUSIC_STEP_SEC = 6.0
# 음악 일치로 인정하는 정규화 상관 하한.
MUSIC_THRESHOLD = 0.45
# 소리 크기 요약(level)의 구간 길이(초).
LEVEL_STEP_SEC = 5.0
# 음악 구간 묶기: 이웃 조각의 "곡 시작 시각"(영상 시각 − 곡 안 위치) 차이 허용치(초).
SEGMENT_TOLERANCE_SEC = 0.6
# 음악 구간으로 인정할 최소 연속 조각 수 (상관이 낮아도 위치가 이어지면 같은 곡으로 본다).
SEGMENT_MIN_WINDOWS = 2


def decode_mono(path, force_wav=True):
    """ffmpeg 로 파일을 RATE Hz 모노 float32 배열로 디코딩한다.

    원본 소리(.wav·.mus)는 확장자와 무관하게 WAV 로 읽고(force_wav), 영상 파일은 형식을 자동 판별한다.
    """
    args = ["ffmpeg", "-v", "error"] + (["-f", "wav"] if force_wav else []) + ["-i", str(path),
            "-vn", "-ac", "1", "-ar", str(RATE), "-f", "f32le", "-"]
    result = subprocess.run(args, capture_output=True)
    if result.returncode != 0:
        sys.stderr.write(result.stderr.decode(errors="replace"))
        raise SystemExit(f"디코딩 실패: {path}")
    return np.frombuffer(result.stdout, dtype=np.float32).astype(np.float64)


def parse_utc(text):
    """녹화 기록의 ISO 8601 UTC 문자열(소수 7자리 허용)을 datetime 으로 바꾼다."""
    text = text.strip().replace("Z", "+00:00")
    head, _, tail = text.partition(".")
    if tail:
        # 소수부와 시간대(+00:00)를 나눠 마이크로초 6자리로 맞춘다.
        frac, sign, zone = tail[:tail.find("+")], "+", tail[tail.find("+") + 1:]
        text = f"{head}.{frac[:6].ljust(6, '0')}{sign}{zone}"
    return dt.datetime.fromisoformat(text)


def load_session(session):
    """세션 폴더의 녹음 조각을 이어 붙여 (신호, 영상 0초 기준 녹음 시작 오프셋 초) 를 돌려준다."""
    session = Path(session)
    with open(session / "video-0001.frames.csv", encoding="utf-8") as f:
        first = next(csv.DictReader(f))
    video_zero = parse_utc(first["utc"])
    pieces = []
    # 오디오 조각마다 시작 UTC 를 읽어 전체 타임라인에 놓는다.
    for start_file in sorted(session.glob("audio-*.start.txt")):
        name, stamp = start_file.read_text(encoding="utf-8").strip().split(",", 1)
        pieces.append((parse_utc(stamp), decode_mono(session / name)))
    if not pieces:
        raise SystemExit("audio-*.start.txt 가 없다")
    origin = pieces[0][0]
    total = max(int(round((t - origin).total_seconds() * RATE)) + len(s) for t, s in pieces)
    signal = np.zeros(total)
    # 조각을 시작 시각 위치에 복사한다(겹치면 나중 조각이 덮는다).
    for t, s in pieces:
        at = int(round((t - origin).total_seconds() * RATE))
        signal[at:at + len(s)] = s
    return signal, (origin - video_zero).total_seconds()


def load_input(args):
    """세션 폴더면 녹음 조각을, 파일이면 그 파일의 소리를 읽어 (신호, 시각 오프셋 초) 를 돌려준다."""
    source = Path(args.session)
    if source.is_dir():
        return load_session(source)
    return decode_mono(source, force_wav=False), args.offset


def trim(template):
    """효과음 앞뒤 무음을 잘라 낸다."""
    peak = np.max(np.abs(template)) if len(template) else 0.0
    if peak <= 0:
        return template[:0]
    loud = np.nonzero(np.abs(template) >= peak * TRIM_RATIO)[0]
    return template[loud[0]:loud[-1] + 1]


def next_pow2(n):
    """n 이상인 가장 작은 2의 거듭제곱."""
    return 1 << (int(n) - 1).bit_length()


class Correlator:
    """긴 신호 하나에 여러 짧은 틀을 정규화 상호상관으로 대 보는 계산기."""

    def __init__(self, signal, max_template):
        """신호 FFT 와 누적 에너지를 미리 계산한다."""
        self.signal = signal
        self.size = next_pow2(len(signal) + max_template)
        self.spectrum = np.fft.rfft(signal, self.size)
        # 구간 에너지 계산용 누적 제곱합 (앞에 0 하나).
        self.cumsq = np.concatenate(([0.0], np.cumsum(signal * signal)))

    def ncc(self, template):
        """모든 시작 위치에 대해 틀과 녹음 구간의 정규화 상관(-1~1)을 돌려준다."""
        n = len(template)
        t = template - template.mean()
        norm_t = np.sqrt(np.sum(t * t))
        spec_t = np.fft.rfft(t[::-1], self.size)
        full = np.fft.irfft(self.spectrum * spec_t, self.size)
        # 뒤집은 틀과의 합성곱에서 위치 k(시작) 의 값은 인덱스 k+n-1 에 있다.
        valid = len(self.signal) - n + 1
        raw = full[n - 1:n - 1 + valid]
        energy = self.cumsq[n:n + valid] - self.cumsq[:valid]
        denom = norm_t * np.sqrt(np.maximum(energy, 0.0))
        out = np.zeros(valid)
        ok = energy > ENERGY_FLOOR * n
        out[ok] = raw[ok] / denom[ok]
        return out


def pick_peaks(score, threshold, gap):
    """threshold 이상인 점 가운데 gap 표본 안에서 가장 큰 봉우리만 고른다."""
    idx = np.nonzero(score >= threshold)[0]
    peaks = []
    # 큰 값부터 고르고 가까운 후보는 버린다(비최대 억제).
    for i in idx[np.argsort(-score[idx])]:
        if all(abs(i - p) >= gap for p in peaks):
            peaks.append(int(i))
    return sorted(peaks)


def fmt_time(sec):
    """초를 mm:ss.s 형식으로 바꾼다."""
    sign = "-" if sec < 0 else ""
    sec = abs(sec)
    return f"{sign}{int(sec // 60):02d}:{sec % 60:04.1f}"


def cmd_sfx(args):
    """원본 효과음 전체를 녹음에 대 보고 일치 시점을 출력한다."""
    signal, offset = load_input(args)
    templates = []
    paths = [Path(p) for p in args.sounds] if args.sounds else sorted(
        p for p in SOUND_DIR.glob("*") if p.suffix.lower() == ".wav")
    # 대상 소리(기본: 원본 효과음 전체)를 읽고 앞뒤 무음을 자른다.
    for path in paths:
        t = trim(decode_mono(path))
        if len(t) >= MIN_TEMPLATE_SEC * RATE:
            templates.append((path.name, t))
    corr = Correlator(signal, max(len(t) for _, t in templates))
    rows = []
    # 효과음마다 상관 봉우리를 찾는다.
    for name, t in templates:
        score = corr.ncc(t)
        # 봉우리마다 영상 시각으로 바꿔 기록한다
        for p in pick_peaks(score, args.threshold, max(len(t) // 2, RATE // 4)):
            sec = p / RATE + offset
            rows.append((sec, name, float(score[p]), len(t) / RATE))
    rows.sort()
    write_rows(args.output, ["video_sec", "time", "sound", "ncc", "length_sec"],
               [(f"{s:.2f}", fmt_time(s), n, f"{c:.3f}", f"{l:.2f}") for s, n, c, l in rows])
    print(f"효과음 {len(templates)}개, 일치 {len(rows)}건", file=sys.stderr)


def cmd_music(args):
    """녹음을 일정 길이 조각으로 나눠 어느 음악의 몇 초 지점인지 찾는다."""
    signal, offset = load_input(args)
    tracks = [(p.name, decode_mono(p)) for p in sorted(MUSIC_DIR.glob("*.mus"))]
    window = int(MUSIC_WINDOW_SEC * RATE)
    step = int(MUSIC_STEP_SEC * RATE)
    correlators = [(name, Correlator(track, window)) for name, track in tracks]
    rows = []
    # 녹음 조각마다 모든 음악과 비교해 가장 잘 맞는 곡과 위치를 고른다.
    for start in range(0, len(signal) - window, step):
        piece = signal[start:start + window]
        if np.sum(piece * piece) <= ENERGY_FLOOR * window:
            rows.append((start / RATE + offset, "(무음)", 0.0, 0.0))
            continue
        best = ("(없음)", 0.0, 0.0)
        # 곡마다 조각을 틀로 삼아 곡 안의 위치를 찾는다.
        for name, c in correlators:
            score = c.ncc(piece)
            k = int(np.argmax(score))
            if score[k] > best[1]:
                best = (name, float(score[k]), k / RATE)
        label = best[0] if best[1] >= args.threshold else f"(불명:{best[0]})"
        rows.append((start / RATE + offset, label, best[1], best[2]))
    write_rows(args.output, ["video_sec", "time", "music", "ncc", "track_sec"],
               [(f"{s:.1f}", fmt_time(s), n, f"{c:.3f}", f"{t:.1f}") for s, n, c, t in rows])
    lengths = {name: len(track) / RATE for name, track in tracks}
    print("곡 구간 (곡 시작 시각은 영상 시각 − 곡 안 위치):", file=sys.stderr)
    # 곡 안 위치가 이어지는 조각 묶음을 곡 재생 구간으로 요약한다.
    for name, start, first, last, count in music_segments(rows):
        print(f"  {name:18s} 곡 시작 {fmt_time(start)}  관찰 {fmt_time(first)}~{fmt_time(last + args_window())}"
              f"  조각 {count}개  곡 끝 예정 {fmt_time(start + lengths[name])}", file=sys.stderr)


def args_window():
    """음악 조각 길이(초). 구간 끝 표시에 더한다."""
    return MUSIC_WINDOW_SEC


def music_segments(rows):
    """(영상 시각, 곡, 상관, 곡 위치) 목록에서 같은 곡·같은 시작 시각이 이어지는 묶음을 찾는다."""
    segments = []
    current = None
    # 조각을 시간순으로 보며 곡 이름(불명 표시 제거)과 곡 시작 시각이 같으면 이어 붙인다.
    for sec, label, score, pos in rows:
        name = label.split(":", 1)[-1].rstrip(")") if label.startswith("(") else label
        if score <= 0:
            current = None
            continue
        start = sec - pos
        if current and current[0] == name and abs(current[1] - start) <= SEGMENT_TOLERANCE_SEC:
            current[3] = sec
            current[4] += 1
            continue
        current = [name, start, sec, sec, 1]
        segments.append(current)
    return [tuple(s) for s in segments if s[4] >= SEGMENT_MIN_WINDOWS]


def cmd_level(args):
    """구간별 소리 크기(RMS, dBFS)를 출력한다. 무음·큰 소리 구간을 훑을 때 쓴다."""
    signal, offset = load_input(args)
    step = int(LEVEL_STEP_SEC * RATE)
    rows = []
    # 일정 구간마다 RMS 를 dB 로 바꾼다.
    for start in range(0, len(signal), step):
        piece = signal[start:start + step]
        rms = np.sqrt(np.mean(piece * piece)) if len(piece) else 0.0
        db = 20 * np.log10(rms) if rms > 0 else -120.0
        rows.append((f"{start / RATE + offset:.1f}", fmt_time(start / RATE + offset), f"{db:.1f}"))
    write_rows(args.output, ["video_sec", "time", "rms_dbfs"], rows)


def write_rows(output, header, rows):
    """CSV 를 파일(지정 시) 또는 표준 출력으로 쓴다."""
    if output:
        Path(output).parent.mkdir(parents=True, exist_ok=True)
        f = open(output, "w", encoding="utf-8", newline="")
    else:
        f = sys.stdout
    w = csv.writer(f)
    w.writerow(header)
    w.writerows(rows)
    if output:
        f.close()


def main():
    """명령줄 인자를 해석해 하위 명령을 실행한다."""
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("sfx", help="원본 효과음 재생 시점 찾기")
    p.add_argument("session", help="record-play 세션 폴더 또는 영상·소리 파일")
    p.add_argument("--offset", type=float, default=0.0, help="파일 입력일 때 더할 시각(초, 예: youtube_clip 의 start)")
    p.add_argument("--threshold", type=float, default=SFX_THRESHOLD)
    p.add_argument("--sounds", nargs="*", help="대상 소리 파일 (생략하면 originals/sound/*.wav 전체)")
    p.add_argument("-o", "--output")
    p.set_defaults(func=cmd_sfx)
    p = sub.add_parser("music", help="구간별 배경 음악과 곡 안 위치 찾기")
    p.add_argument("session", help="record-play 세션 폴더 또는 영상·소리 파일")
    p.add_argument("--offset", type=float, default=0.0, help="파일 입력일 때 더할 시각(초)")
    p.add_argument("--threshold", type=float, default=MUSIC_THRESHOLD)
    p.add_argument("-o", "--output")
    p.set_defaults(func=cmd_music)
    p = sub.add_parser("level", help="구간별 소리 크기")
    p.add_argument("session", help="record-play 세션 폴더 또는 영상·소리 파일")
    p.add_argument("--offset", type=float, default=0.0, help="파일 입력일 때 더할 시각(초)")
    p.add_argument("-o", "--output")
    p.set_defaults(func=cmd_level)
    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
