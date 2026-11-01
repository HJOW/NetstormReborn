# -*- coding: utf-8 -*-
"""
webpj 의 영역 패턴 표(www/assets/territory-patterns.js)를 만든다.

원본 실행 파일 0x5300f0 의 영역 패턴 68개(각 72바이트: 첫 값·폭·높이 + 셀 4바이트 15개)를 cpppj 가 옮겨 둔 `cpppj/src/o/CanonDecoderTables.inc` 에서 읽어
셀마다 (변형 글자, 방향 글자, 번호 글자)를 문자열 세 개로 펼쳐 JS 모듈로 쓴다. 이 값은 모양 번호(6비트, 0~63)로 고르는 영역 배치에 쓰인다.
검사(tests/territory.test.mjs)는 실행 파일에서 따로 뽑은 dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json 과 대조하므로, 두 추출이 같아야 한다.

사용법 (저장소 루트에서):
    python webpj/tools/make-territory-patterns.py [--check]

--check 는 파일을 쓰지 않고 현재 파일이 이 도구의 결과와 같은지만 확인한다.

(2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계 영역 배치를 위해 추가)
"""
import argparse
import os
import re
import sys

# 저장소 루트와 입출력 경로이다.
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SOURCE = os.path.join(ROOT, "cpppj", "src", "o", "CanonDecoderTables.inc")
TARGET = os.path.join(ROOT, "webpj", "www", "assets", "territory-patterns.js")

# 패턴 수와 한 패턴의 최대 셀 수이다(원본 레코드 72바이트 = 정수 3개 + 셀 15개).
PATTERN_COUNT = 68
MAX_CELLS = 15

# 영역 패턴 한 줄: `{첫 값, 폭, 높이, {{{셀}, {셀}, …}}},  // 번호`.
PATTERN_LINE = re.compile(r"^\s*\{(\d+), (\d+), (\d+), \{\{(.*)\}\}\},\s*// (\d+)\s*$")
# 셀 하나: `{0x31,0x4c,0x00,0x00}`.
CELL = re.compile(r"\{0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2})\}")


def read_patterns():
    """cpppj 표에서 영역 패턴(앞의 68개 줄)을 읽어 [(폭, 높이, 방향 글자, 변형 글자, 번호 글자)] 로 돌려준다."""
    with open(SOURCE, encoding="utf-8") as stream:
        lines = stream.read().splitlines()
    patterns = []
    # 영역 패턴 배열은 파일 앞쪽이다. 다리 패턴 줄은 번호 주석이 있어도 앞에서 이미 68개를 채운 뒤라 읽지 않는다.
    for line in lines:
        if len(patterns) == PATTERN_COUNT:
            break
        matched = PATTERN_LINE.match(line)
        if not matched:
            continue
        first, width, height, cells_text, number = matched.groups()
        if int(number) != len(patterns):
            raise ValueError("패턴 번호가 순서와 다릅니다: %s" % number)
        cells = CELL.findall(cells_text)
        if len(cells) != MAX_CELLS:
            raise ValueError("패턴 %s 의 셀 수가 15가 아닙니다" % number)
        width, height = int(width), int(height)
        if not 0 < width * height <= MAX_CELLS:
            raise ValueError("패턴 %s 의 크기가 잘못되었습니다" % number)
        sides, variations, labels = [], [], []
        # 폭 × 높이만큼의 셀만 쓴다(나머지는 0).
        for variation, side, _unused, label in cells[: width * height]:
            variations.append(chr(int(variation, 16)))
            sides.append(chr(int(side, 16)))
            labels.append("-" if label == "00" else chr(int(label, 16)))
        patterns.append((width, height, "".join(sides), "".join(variations), "".join(labels)))
    if len(patterns) != PATTERN_COUNT:
        raise ValueError("영역 패턴이 %d개뿐입니다" % len(patterns))
    return patterns


def js_string(text):
    """JS 작은따옴표 문자열 리터럴로 만든다(패턴 글자에는 따옴표·역슬래시가 없음을 확인한다)."""
    if "'" in text or "\\" in text:
        raise ValueError("문자열에 따옴표나 역슬래시가 있습니다: %r" % text)
    return "'" + text + "'"


def render(patterns):
    """JS 모듈 본문을 만든다. 줄바꿈은 LF 이다."""
    out = []
    out.append("// 영역(Territory) 패턴 표이다. 원본 `Territory` 레코드의 모양 번호(첫 바이트 하위 6비트)가 고르는 패턴이며, 한 패턴은 폭 × 높이 칸의 청크 배치 틀이다.")
    out.append("// 원본 실행 파일 0x5300f0 의 68개(각 72바이트)를 cpppj `CanonDecoderTables.inc` 에서 옮긴 것이다. 모양 번호는 6비트라 앞의 64개만 고를 수 있으나 원본 표를 그대로 둔다.")
    out.append("// 이 파일은 webpj/tools/make-territory-patterns.py 가 만든다. 직접 고치지 않는다(고치면 `--check` 와 tests/territory.test.mjs 가 알려 준다).")
    out.append("// 칸은 줄 우선 순서(왼쪽 위에서 오른쪽, 아래 줄)이다. 칸마다 세 글자가 한 위치에 대응한다:")
    out.append("//  - sides: 청크의 방향(모서리) 글자 'A'~'P', 빈 칸은 '.'  - variations: 변형 숫자 글자('1' 등, 빈 칸은 '.')  - labels: 번호 글자('a'~), 없으면 '-'.")
    out.append("// (2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계 영역 배치를 위해 추가)")
    out.append("")
    out.append("// 원본 패턴 표의 항목 수이다(모양 번호로 고를 수 있는 것은 앞의 64개). (2026-11-01 WB03-V)")
    out.append("export const TERRITORY_PATTERN_COUNT = %d;" % len(patterns))
    out.append("")
    out.append("// 영역 패턴 표이다. [폭, 높이, sides, variations, labels] 를 모양 번호 순서로 늘어놓고 아래에서 객체로 바꾼다. (2026-11-01 WB03-V)")
    out.append("const RAW_PATTERNS = [")
    # 패턴마다 한 줄이다(번호는 주석).
    for number, (width, height, sides, variations, labels) in enumerate(patterns):
        out.append("    [%d, %d, %s, %s, %s], // %d" % (width, height, js_string(sides), js_string(variations), js_string(labels), number))
    out.append("];")
    out.append("")
    out.append("// 영역 패턴 객체의 목록이다: { width, height, sides, variations, labels }. 모양 번호로 바로 고른다(예: TERRITORY_PATTERNS[7]). (2026-11-01 WB03-V)")
    out.append("export const TERRITORY_PATTERNS = Object.freeze(RAW_PATTERNS.map(([width, height, sides, variations, labels]) => Object.freeze({ width, height, sides, variations, labels })));")
    out.append("")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument("--check", action="store_true", help="파일을 쓰지 않고 현재 파일과 같은지만 확인한다")
    args = parser.parse_args()
    text = render(read_patterns())
    if args.check:
        with open(TARGET, encoding="utf-8", newline="") as stream:
            current = stream.read()
        if current.replace("\r\n", "\n") != text:
            print("다릅니다: %s" % TARGET)
            return 1
        print("같습니다: %s" % TARGET)
        return 0
    with open(TARGET, "w", encoding="utf-8", newline="\n") as stream:
        stream.write(text)
    print("%d개 패턴 저장: %s" % (PATTERN_COUNT, TARGET))
    return 0


if __name__ == "__main__":
    sys.exit(main())
