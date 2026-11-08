# -*- coding: utf-8 -*-
"""
webpj 의 타입별 Canon 패턴 표(www/assets/canon-type-patterns.js)를 만든다.

원본 10.78 실행 파일의 다리 패턴 26개(0x52f998, 첫 값은 추첨 가중치)와 섬(0x531410, 2개)·받침(0x5314a0, 1개)·공유 전투(0x5314e8, 1개) 패턴을
cpppj 가 옮겨 둔 `cpppj/src/o/CanonDecoderTables.inc`(다리)·`CanonTypePatterns.inc`(섬/받침/전투)에서 읽어, 셀마다 (방향 글자, 변형 글자, 번호 글자)를
문자열 세 개로 펼쳐 JS 모듈로 쓴다. 영역 패턴(territory-patterns.js)과 같은 모양이며 `first`(다리 추첨 가중치, 그 밖은 10)를 추가로 가진다.
CD(10.72) 다리 표는 범위 밖이라 읽지 않는다.

사용법 (저장소 루트에서):
    python webpj/tools/make-canon-type-patterns.py [--check]

--check 는 파일을 쓰지 않고 현재 파일이 이 도구의 결과와 같은지만 확인한다.

(2026-11-08 WB04-E136: 타입별 Canon 디코더 이식을 위해 추가)
"""
import argparse
import os
import re
import sys

# 저장소 루트와 입출력 경로이다.
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
BRIDGE_SOURCE = os.path.join(ROOT, "cpppj", "src", "o", "CanonDecoderTables.inc")
TYPE_SOURCE = os.path.join(ROOT, "cpppj", "src", "o", "CanonTypePatterns.inc")
TARGET = os.path.join(ROOT, "webpj", "www", "assets", "canon-type-patterns.js")

# 한 패턴의 최대 셀 수(원본 레코드 72바이트 = 정수 3개 + 셀 15개)와 다리 패턴 수, 10.78 다리 가중치 합이다.
MAX_CELLS = 15
BRIDGE_COUNT = 26
BRIDGE_WEIGHT_TOTAL = 287

# 패턴 한 줄: `{첫 값, 폭, 높이, {{{셀}, {셀}, …}}},` (다리 표는 뒤에 `// 번호` 주석이 붙는다).
PATTERN_LINE = re.compile(r"^\s*\{(\d+),\s*(\d+),\s*(\d+),\s*\{\{(.*)\}\}\},(?:\s*// (\d+))?\s*$")
# 셀 하나: `{0x31,0x4c,0x00,0x00}`.
CELL = re.compile(r"\{0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2})\}")


def parse_line(line):
    """패턴 줄 하나를 (첫 값, 폭, 높이, 방향 글자, 변형 글자, 번호 글자)로 바꾼다. 패턴 줄이 아니면 None 이다."""
    matched = PATTERN_LINE.match(line)
    if not matched:
        return None
    first, width, height, cells_text, _number = matched.groups()
    cells = CELL.findall(cells_text)
    if len(cells) != MAX_CELLS:
        raise ValueError("셀 수가 15가 아닙니다: %s" % line[:40])
    width, height = int(width), int(height)
    if not 0 < width * height <= MAX_CELLS:
        raise ValueError("패턴 크기가 잘못되었습니다: %s" % line[:40])
    sides, variations, labels = [], [], []
    # 폭 × 높이만큼의 셀만 쓴다(나머지는 0).
    for variation, side, _unused, label in cells[: width * height]:
        variations.append(chr(int(variation, 16)))
        sides.append(chr(int(side, 16)))
        labels.append("-" if label == "00" else chr(int(label, 16)))
    return (int(first), width, height, "".join(sides), "".join(variations), "".join(labels))


def read_block(path, start_marker, count):
    """파일에서 start_marker 로 시작하는 배열 선언 다음의 패턴 줄 count 개를 읽는다."""
    with open(path, encoding="utf-8") as stream:
        lines = stream.read().splitlines()
    for index, line in enumerate(lines):
        if line.startswith(start_marker):
            patterns = []
            # 선언 바로 뒤부터 닫는 `}};` 전까지의 패턴 줄을 읽는다.
            for body in lines[index + 1:]:
                if body.startswith("}};"):
                    break
                parsed = parse_line(body)
                if parsed is not None:
                    patterns.append(parsed)
            if len(patterns) != count:
                raise ValueError("%s 의 패턴이 %d개가 아니라 %d개입니다" % (start_marker, count, len(patterns)))
            return patterns
    raise ValueError("선언을 찾지 못했습니다: %s" % start_marker)


def read_all():
    """네 표를 읽어 (다리, 섬, 받침, 전투) 순서로 돌려준다. 다리 가중치 합은 287 이어야 한다."""
    bridge = read_block(BRIDGE_SOURCE, "constexpr std::array<CanonPattern, 26> kBridgePatterns", BRIDGE_COUNT)
    if sum(item[0] for item in bridge) != BRIDGE_WEIGHT_TOTAL:
        raise ValueError("다리 가중치 합이 287 이 아닙니다")
    island = read_block(TYPE_SOURCE, "constexpr std::array<CanonPattern,2> kIslandPatterns", 2)
    support = read_block(TYPE_SOURCE, "constexpr std::array<CanonPattern,1> kSupportPatterns", 1)
    battle = read_block(TYPE_SOURCE, "constexpr std::array<CanonPattern,1> kBattlePatterns", 1)
    return bridge, island, support, battle


def js_string(text):
    """JS 작은따옴표 문자열 리터럴로 만든다(패턴 글자에는 따옴표·역슬래시가 없음을 확인한다)."""
    if "'" in text or "\\" in text:
        raise ValueError("문자열에 따옴표나 역슬래시가 있습니다: %r" % text)
    return "'" + text + "'"


def render_table(out, name, comment, patterns):
    """패턴 표 하나를 [첫 값, 폭, 높이, sides, variations, labels] 줄 목록으로 쓴다."""
    out.append("// %s (2026-11-08 WB04-E136)" % comment)
    out.append("const RAW_%s = [" % name)
    # 패턴마다 한 줄이다(번호는 주석).
    for number, (first, width, height, sides, variations, labels) in enumerate(patterns):
        out.append("    [%d, %d, %d, %s, %s, %s], // %d" % (first, width, height, js_string(sides), js_string(variations), js_string(labels), number))
    out.append("];")
    out.append("")


def render(tables):
    """JS 모듈 본문을 만든다. 줄바꿈은 LF 이다."""
    bridge, island, support, battle = tables
    out = []
    out.append("// 타입별 Canon 패턴 표이다(원본 CanonDecoder 생성자 00425c20 이 타입 전역으로 고르는 다리·섬·받침·공유 전투 패턴, 10.78).")
    out.append("// 다리 26개는 0x52f998(첫 값 = 추첨 가중치, 합 287), 섬 2개는 0x531410, 받침 1개는 0x5314a0, 공유 전투 1개는 0x5314e8 이다(각 72바이트). 영역 패턴 68개는 territory-patterns.js 에 있다.")
    out.append("// 이 파일은 webpj/tools/make-canon-type-patterns.py 가 만든다. 직접 고치지 않는다(고치면 `--check` 와 tests/canon-type.test.mjs 가 알려 준다).")
    out.append("// 칸은 줄 우선 순서이고 칸마다 세 글자가 한 위치에 대응한다: sides(방향 'A'~'P', 빈 칸 '.')·variations(변형 숫자 글자, 빈 칸 '.')·labels(번호 'a'~, 없으면 '-'). first 는 다리 추첨 가중치(그 밖은 10)이다.")
    out.append("// (2026-11-08 WB04-E136: 타입별 Canon 디코더 이식을 위해 추가)")
    out.append("")
    render_table(out, "BRIDGE", "다리 패턴 26개이다: [가중치, 폭, 높이, sides, variations, labels].", bridge)
    render_table(out, "ISLAND", "섬 패턴 2개이다.", island)
    render_table(out, "SUPPORT", "받침 패턴 1개이다.", support)
    render_table(out, "BATTLE", "공유 전투 패턴 1개이다.", battle)
    out.append("// 패턴 객체로 바꾸는 도우미이다: { first, width, height, sides, variations, labels }. (2026-11-08 WB04-E136)")
    out.append("function build(rows) {")
    out.append("    return Object.freeze(rows.map(([first, width, height, sides, variations, labels]) => Object.freeze({ first, width, height, sides, variations, labels })));")
    out.append("}")
    out.append("")
    out.append("// 다리 패턴 객체 26개(가중치 합 287)이다. (2026-11-08 WB04-E136)")
    out.append("export const BRIDGE_PATTERNS = build(RAW_BRIDGE);")
    out.append("// 섬(타입 전역 세 번째) 패턴 객체 2개이다. (2026-11-08 WB04-E136)")
    out.append("export const ISLAND_PATTERNS = build(RAW_ISLAND);")
    out.append("// 받침(noIsland) 패턴 객체 1개이다. (2026-11-08 WB04-E136)")
    out.append("export const SUPPORT_PATTERNS = build(RAW_SUPPORT);")
    out.append("// 공유 전투 패턴 객체 1개이다. (2026-11-08 WB04-E136)")
    out.append("export const BATTLE_PATTERNS = build(RAW_BATTLE);")
    out.append("")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument("--check", action="store_true", help="파일을 쓰지 않고 현재 파일과 같은지만 확인한다")
    args = parser.parse_args()
    text = render(read_all())
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
    print("다리 %d + 섬 2 + 받침 1 + 전투 1개 패턴 저장: %s" % (BRIDGE_COUNT, TARGET))
    return 0


if __name__ == "__main__":
    sys.exit(main())
