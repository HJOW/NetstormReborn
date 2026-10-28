"""두 논리 화면 캡처(BMP)를 비교한다. cpppj 의 `--ui-script` snapshot 과 dotnetpj 의 같은 스크립트 결과를 L4a(픽셀 차이 0)로 판정하는 도구다.

사용법 (저장소 루트에서 실행):
  python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png] [--list 10]
      두 파일을 비교한다. 파일 바이트가 같으면 바로 일치다. 아니면 크기, 다른 픽셀 수, 다른 영역, 채널별 최대 차이, 처음 N개의 차이를 보여 준다.
      --diff 는 다른 픽셀을 빨강으로 표시한 PNG 를 쓴다.
  python dotnetpj/tools/cpp_frame_compare.py --dir 폴더A 폴더B [--list 5]
      두 폴더에서 이름이 같은 *.bmp 를 모두 비교한다. 한쪽에만 있는 파일도 보고한다.

종료 코드: 0 모두 일치, 1 차이 있음, 2 입력 오류. 게임·창을 실행하지 않고 읽기만 한다 (diff PNG 쓰기 제외).
지원 형식: cpppj WriteBitmap 과 같은 32비트 BMP(BITMAPV4HEADER, 음수 높이=위→아래). 다른 형식은 오류로 알린다.
(2026-10-28 R1-2 에서 추가, 계획: LEFT_JOBS.dotnetpj.md 0절 결정 4·0.6절)
"""

from dataclasses import dataclass
from pathlib import Path
import argparse
import hashlib
import struct
import sys
import zlib

# 파일 머리(14) + BITMAPV4HEADER(108) 크기. cpppj platform/Bitmap.cpp 의 kBitmapHeaderSize 와 같다.
HEADER_SIZE = 122


@dataclass
class Frame:
    """읽은 캡처 하나. 크기와 BGRA 화소 바이트(위→아래 행)를 담는다."""

    path: Path
    width: int
    height: int
    pixels: bytes
    raw: bytes


def read_frame(path: Path) -> Frame:
    """BMP 파일을 읽고 형식을 검증한다. 지원하지 않는 형식이면 ValueError 를 던진다."""
    raw = path.read_bytes()
    if len(raw) < HEADER_SIZE or raw[:2] != b"BM":
        raise ValueError(f"{path}: BMP 가 아니거나 머리가 짧다")
    data_offset, dib_size, width, height, planes, bits, compression = struct.unpack_from("<IIiiHHI", raw, 10)
    if dib_size != 108 or data_offset != HEADER_SIZE or planes != 1 or bits != 32 or compression != 3 or height >= 0 or width <= 0:
        raise ValueError(f"{path}: cpppj 캡처 형식(V4, 32비트, BITFIELDS, 위→아래)이 아니다")
    height = -height
    if len(raw) != HEADER_SIZE + width * height * 4:
        raise ValueError(f"{path}: 파일 크기가 머리의 크기와 맞지 않는다")
    return Frame(path, width, height, raw[HEADER_SIZE:], raw)


def compare_frames(a: Frame, b: Frame, list_limit: int) -> tuple[bool, list[str], list[tuple[int, int]]]:
    """두 캡처를 비교해 (일치 여부, 보고 줄들, 다른 픽셀 좌표)를 돌려준다. 크기가 다르면 화소는 비교하지 않는다."""
    lines = [f"A {a.path} {a.width}x{a.height} sha256 {hashlib.sha256(a.raw).hexdigest()[:16]}",
             f"B {b.path} {b.width}x{b.height} sha256 {hashlib.sha256(b.raw).hexdigest()[:16]}"]
    if (a.width, a.height) != (b.width, b.height):
        lines.append("차이: 크기가 다르다")
        return False, lines, []
    if a.raw == b.raw:
        lines.append("일치: 파일 바이트가 같다")
        return True, lines, []
    diffs: list[tuple[int, int]] = []
    channel_max = [0, 0, 0, 0]
    left, top, right, bottom = a.width, a.height, -1, -1
    # 화소마다 BGRA 네 바이트를 비교한다. 다른 화소의 좌표·영역·채널별 최대 차이를 모은다.
    for index in range(a.width * a.height):
        offset = index * 4
        pa, pb = a.pixels[offset:offset + 4], b.pixels[offset:offset + 4]
        if pa == pb:
            continue
        x, y = index % a.width, index // a.width
        diffs.append((x, y))
        left, top, right, bottom = min(left, x), min(top, y), max(right, x), max(bottom, y)
        for c in range(4):
            channel_max[c] = max(channel_max[c], abs(pa[c] - pb[c]))
    if not diffs:
        lines.append("일치: 화소는 같고 머리만 다르다(머리 바이트 차이를 확인한다)")
        return False, lines, []
    total = a.width * a.height
    lines.append(f"차이: 다른 픽셀 {len(diffs)}개 / {total} ({len(diffs) * 100.0 / total:.4f}%)")
    lines.append(f"영역: x {left}..{right}, y {top}..{bottom}   채널별 최대 차이 B{channel_max[0]} G{channel_max[1]} R{channel_max[2]} A{channel_max[3]}")
    # 처음 N개의 차이를 A/B 색과 함께 보여 준다.
    for x, y in diffs[:list_limit]:
        offset = (y * a.width + x) * 4
        pa, pb = a.pixels[offset:offset + 4], b.pixels[offset:offset + 4]
        lines.append(f"  ({x},{y}) A=#{pa[2]:02x}{pa[1]:02x}{pa[0]:02x} B=#{pb[2]:02x}{pb[1]:02x}{pb[0]:02x}")
    return False, lines, diffs


def write_diff_png(path: Path, a: Frame, diffs: list[tuple[int, int]]) -> None:
    """A 를 어둡게 깔고 다른 픽셀을 빨강으로 표시한 PNG 를 쓴다."""
    marked = set(diffs)
    rows = bytearray()
    # 행마다 필터 바이트 0 과 RGB 화소를 쓴다.
    for y in range(a.height):
        rows.append(0)
        for x in range(a.width):
            if (x, y) in marked:
                rows += b"\xff\x00\x00"
            else:
                o = (y * a.width + x) * 4
                rows += bytes((a.pixels[o + 2] // 3, a.pixels[o + 1] // 3, a.pixels[o] // 3))

    def chunk(kind: bytes, body: bytes) -> bytes:
        """PNG 청크 하나를 길이·종류·본문·CRC 순으로 만든다."""
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", a.width, a.height, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(rows), 6)) + chunk(b"IEND", b""))
    path.write_bytes(png)


def compare_dirs(a_dir: Path, b_dir: Path, list_limit: int) -> int:
    """두 폴더의 같은 이름 *.bmp 를 모두 비교한다. 모두 일치하면 0, 하나라도 다르거나 한쪽에만 있으면 1 이다."""
    a_files = {p.name: p for p in a_dir.glob("*.bmp")}
    b_files = {p.name: p for p in b_dir.glob("*.bmp")}
    bad = 0
    # 이름순으로 한 쌍씩 비교한다.
    for name in sorted(set(a_files) | set(b_files)):
        if name not in a_files or name not in b_files:
            print(f"한쪽에만 있음: {name} ({'A' if name in a_files else 'B'})")
            bad += 1
            continue
        same, lines, _ = compare_frames(read_frame(a_files[name]), read_frame(b_files[name]), list_limit)
        print(f"[{'OK' if same else '차이'}] {name}")
        if not same:
            bad += 1
            print("\n".join("    " + line for line in lines[2:]))
    print(f"\n비교 {len(set(a_files) | set(b_files))}개 중 불일치 {bad}개")
    return 1 if bad else 0


def main() -> int:
    """명령줄 인자를 읽어 파일 쌍 또는 폴더 쌍을 비교한다."""
    sys.stdout.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(description="cpppj/dotnetpj 논리 화면 캡처(BMP) 비교 도구")
    parser.add_argument("first", help="A 파일(또는 --dir 일 때 폴더)")
    parser.add_argument("second", help="B 파일(또는 --dir 일 때 폴더)")
    parser.add_argument("--dir", action="store_true", help="두 인자를 폴더로 보고 같은 이름의 *.bmp 를 모두 비교한다")
    parser.add_argument("--diff", metavar="PNG", help="다른 픽셀을 표시한 PNG 를 쓴다(파일 비교에서만)")
    parser.add_argument("--list", type=int, default=10, metavar="N", help="보고할 처음 차이 개수 (기본 10)")
    args = parser.parse_args()
    try:
        if args.dir:
            return compare_dirs(Path(args.first), Path(args.second), args.list)
        a, b = read_frame(Path(args.first)), read_frame(Path(args.second))
        same, lines, diffs = compare_frames(a, b, args.list)
        print("\n".join(lines))
        if args.diff and diffs:
            write_diff_png(Path(args.diff), a, diffs)
            print(f"차이 PNG: {args.diff}")
        return 0 if same else 1
    except (OSError, ValueError) as error:
        print(f"입력 오류: {error}")
        return 2


if __name__ == "__main__":
    sys.exit(main())
