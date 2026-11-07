"""cpppj의 포함 링크 가상 표(.inc·원문 상수)에서 C# 자료(RawContainedLinkTables.Data.cs)를 만든다.

실행 (저장소 루트에서 실행):
  python -X utf8 dotnetpj/tools/gen_contained_link_tables.py            자료를 다시 만든다
  python -X utf8 dotnetpj/tools/gen_contained_link_tables.py --check    자료 파일이 최신 생성 결과와 같은지 확인한다 (다르면 종료 코드 1)

입력인 ContentNoticeVtables.inc와 SquidProcess.cpp의 상수 줄은 읽기만 한다.
생성 자료 151개(패치 83·CD 68)의 정확도는 RawContainedLinkTests(개수·앞 항목·허용 목록 소속)가 대조한다.
(2026-11-07 F2d 포함 링크 이식에서 추가)
"""

from pathlib import Path
import argparse
import re
import sys

# 저장소 루트. 이 파일은 dotnetpj/tools 아래에 있으므로 두 단계 위다.
ROOT = Path(__file__).resolve().parents[2]
# 허용 목록 항목이 들어 있는 cpppj 원문 표 (읽기 전용).
NOTICES = ROOT / "cpppj" / "src" / "o" / "ContentNoticeVtables.inc"
# 네 Form 생성자 주소가 들어 있는 cpppj 원문 (읽기 전용).
PROCESS = ROOT / "cpppj" / "src" / "o" / "RawSquidContainedUnpop.cpp"
# 만드는 C# 자료 파일.
OUTPUT = ROOT / "dotnetpj" / "src" / "Netstorm.Core" / "Original" / "RawContainedLinkTables.Data.cs"
# 패치 허용 목록 항목 수.
PATCH_COUNT = 83
# CD 허용 목록 항목 수.
CD_COUNT = 68


def parse_notices(text):
    """허용 목록 두 배열의 (table, entry) 쌍을 순서대로 읽는다."""
    patch, cd = [], []
    current = None
    for line in text.splitlines():
        if "kPatchContentNotices" in line:
            current = patch
        elif "kCdContentNotices" in line:
            current = cd
        else:
            match = re.match(r"\s*\{0x([0-9a-fA-F]+)U,0x([0-9a-fA-F]+)U\},?", line)
            if match and current is not None:
                current.append((int(match.group(1), 16), int(match.group(2), 16)))
    return patch, cd


def parse_forms(text):
    """네 Form 생성자 주소 두 줄을 읽는다."""
    patch, cd = [], []
    for line in text.splitlines():
        match = re.match(r".*kPatchForms\{([0-9a-fA-FxU, ]+)\}.*", line)
        if match:
            patch = [int(v.strip().rstrip("U"), 16) for v in match.group(1).split(",")]
        match = re.match(r".*kCdForms\{([0-9a-fA-FxU, ]+)\}.*", line)
        if match:
            cd = [int(v.strip().rstrip("U"), 16) for v in match.group(1).split(",")]
    return patch, cd


def render(patch_forms, cd_forms, patch, cd):
    """C# 자료 파일 본문을 만든다."""
    lines = []
    lines.append("// 포함 링크 가상 표 허용 목록이다. 도구가 만들며 직접 고치지 않는다.")
    lines.append("// 근거: cpppj/src/o/ContentNoticeVtables.inc (83+68 항목), cpppj/src/o/RawSquidContainedUnpop.cpp (네 Form 주소).")
    lines.append("// (2026-11-07 F2d 포함 링크 이식에서 추가)")
    lines.append("namespace Netstorm.Core.Original;")
    lines.append("")
    lines.append("// 내용물 통지의 가상 표 허용 목록 한 항목이다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.")
    lines.append("public readonly record struct ContentNoticeEntry(uint Table, uint Entry);")
    lines.append("")
    lines.append("// 포함 링크 배치 선택이 읽는 원문 주소 표다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.")
    lines.append("public static class ContainedLinkTables")
    lines.append("{")

    def array(name, values):
        items = ", ".join(f"0x{v:08x}u" for v in values)
        lines.append(f"    /// <summary>원문 {name} 주소 표다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>")
        lines.append(f"    public static readonly uint[] {name} = [{items}];")

    array("PatchForms", patch_forms)
    array("CdForms", cd_forms)

    def entries(name, pairs):
        lines.append(f"    /// <summary>원문 {name} 허용 목록이다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>")
        lines.append(f"    public static readonly ContentNoticeEntry[] {name} =")
        lines.append("    [")
        for table, entry in pairs:
            lines.append(f"        new(0x{table:08x}u, 0x{entry:08x}u),")
        lines.append("    ];")

    entries("PatchNotices", patch)
    entries("CdNotices", cd)
    lines.append("}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    patch, cd = parse_notices(NOTICES.read_text(encoding="utf-8"))
    patch_forms, cd_forms = parse_forms(PROCESS.read_text(encoding="utf-8"))
    assert len(patch) == PATCH_COUNT, len(patch)
    assert len(cd) == CD_COUNT, len(cd)
    assert len(patch_forms) == 4 and len(cd_forms) == 4
    text = render(patch_forms, cd_forms, patch, cd)
    if args.check:
        if OUTPUT.read_text(encoding="utf-8") != text:
            print("contained link data differs; rerun gen_contained_link_tables.py")
            return 1
        print("contained link data matches")
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    print(f"wrote {OUTPUT} ({len(patch) + len(cd)} entries)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
