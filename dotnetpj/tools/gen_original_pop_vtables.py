"""cpppj 의 Pop firstPop/postPop 지원 주소 표(.inc)에서 10.78 부분만 읽어 C# 원본 자료(OriginalPopDisplay.Data.cs)를 만든다.

실행 (저장소 루트에서 실행):
  python -X utf8 dotnetpj/tools/gen_original_pop_vtables.py            표를 다시 만든다
  python -X utf8 dotnetpj/tools/gen_original_pop_vtables.py --check    만든 결과와 현재 파일이 같은지만 확인한다 (다르면 종료 코드 1)

입력은 cpppj/src/o/PopVtables.inc 만 읽는다. CD 10.72 표(kCdPopVtables)는 기준 판본이 아니므로 옮기지 않는다.
출력 표의 정확성은 원본 기계어 기대값 pop-vtables.tsv(10.78 83행)를 읽는 C# 검사가 확인한다.
(2026-10-20 W02 Pop 조각에 추가. gen_original_unpop_vtables.py 와 같은 방식이다.)
"""

from pathlib import Path
import argparse
import hashlib
import re
import sys

# 저장소 루트. 이 파일은 dotnetpj/tools 아래에 있으므로 두 단계 위다.
ROOT = Path(__file__).resolve().parents[2]
# 판본별 지원 주소 표가 들어 있는 cpppj 원본 파일 (읽기 전용).
POP_VTABLES = ROOT / "cpppj" / "src" / "o" / "PopVtables.inc"
# 만들 C# 원본 파일.
OUTPUT = ROOT / "dotnetpj" / "src" / "Netstorm.Core" / "Original" / "OriginalPopDisplay.Data.cs"
# 10.78 표의 항목 수. 입력이 늘고 이 수와 다르면 입력이 바뀐 것이므로 중단한다.
PATCH_ENTRY_COUNT = 26


def sha256_lf(path: Path) -> str:
    """개행을 CRLF 에서 LF 로 바꾼 바이트의 SHA-256 을 소문자 16진수로 돌려준다."""
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def parse_patch_entries(text: str) -> list[int]:
    """kPatchPopVtables 에서 주소 26개를 읽는다. 마스크가 아니라 주소 표이므로 값만 있다."""
    start = text.index("kPatchPopVtables")
    begin = text.index("{{", start)
    end = text.index("}}", begin)
    entries = [int(address, 16)
               for address in re.findall(r"(0x[0-9A-Fa-f]+)U", text[begin:end])]
    if len(entries) != PATCH_ENTRY_COUNT:
        raise ValueError(f"10.78 Pop 지원 표가 {len(entries)}개이다 (예상 {PATCH_ENTRY_COUNT})")
    return entries


def render() -> str:
    """지원 주소 표 C# 원본 파일의 전체 내용을 만든다 (LF, UTF-8)."""
    entries = parse_patch_entries(POP_VTABLES.read_text(encoding="utf-8"))
    lines = [
        "// 이 파일은 dotnetpj/tools/gen_original_pop_vtables.py 가 만든다. 직접 고치지 말고 도구를 다시 실행한다.",
        f"// 입력(cpppj, 읽기 전용): PopVtables.inc sha256={sha256_lf(POP_VTABLES)}",
        "// 10.78 부분만 옮겼다. 결과는 원본 기계어 기대값 pop-vtables.tsv 로 검사한다 (W02 Pop 조각, 2026-10-20).",
        "namespace Netstorm.Core.Original;",
        "",
        "/// <summary>Pop 의 표시·파생 효과를 지원하는 가상 표 주소(10.78)를 담는 데이터 전용 클래스다. RawPop·RawPostPop 이 Supports 조회에 쓴다. 값은 이 도구가 만들며 직접 고치지 않는다. (W02 Pop 조각 2026-10-20 생성·summary 추가)</summary>",
        "public static partial class OriginalPopDisplay",
        "{",
        f"    /// <summary>Pop 의 파생 firstPop/postPop 효과를 지원하는 가상 표 {PATCH_ENTRY_COUNT}개(10.78). 호스트 포인터가 아니라 클래스 식별 값이다.</summary>",
        "    private static readonly uint[] Table =",
        "    [",
    ]
    # 항목마다 한 줄씩 적는다.
    for address in entries:
        lines.append(f"        0x{address:08X},")
    lines += ["    ];", "}", ""]
    return "\n".join(lines)


def main() -> int:
    """표시를 만들거나(--check 이면 비교만) 종료 코드를 돌려준다."""
    parser = argparse.ArgumentParser(description="cpppj Pop 지원 주소 표에서 C# 원본 파일을 만든다")
    parser.add_argument("--check", action="store_true", help="현재 파일과 같은지만 확인한다")
    args = parser.parse_args()
    text = render()
    if args.check:
        same = OUTPUT.exists() and OUTPUT.read_bytes().replace(b"\r\n", b"\n") == text.encode("utf-8")
        print("같다" if same else "다르다: 도구를 다시 실행한다")
        return 0 if same else 1
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"만들었다: {OUTPUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
