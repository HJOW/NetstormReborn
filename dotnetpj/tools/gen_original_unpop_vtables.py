"""cpppj 의 Unpop 표시 지원 표(.inc)에서 10.78 부분만 골라 C# 데이터 파일(OriginalUnpopDisplay.Data.cs)을 만든다.

사용법 (저장소 루트에서 실행):
  python -X utf8 dotnetpj/tools/gen_original_unpop_vtables.py            파일을 다시 만든다
  python -X utf8 dotnetpj/tools/gen_original_unpop_vtables.py --check    만든 결과가 현재 파일과 같은지만 확인한다 (다르면 종료 코드 1)

입력은 cpppj/src/o/UnpopVtables.inc 이며 읽기만 한다. CD 10.72 표(kCdDisplayVtables)는 기준 판본이 아니므로 옮기지 않는다.
결과 표의 정확성은 원본 기계어 기대값 unpop-vtables.tsv(10.78 83행)를 읽는 C# 검사가 확인한다.
(2026-10-16 W02 Unpop 조각으로 추가. gen_original_constructors.py 와 같은 방식이다.)
"""

from pathlib import Path
import argparse
import hashlib
import re
import sys

# 저장소 루트. 이 도구는 dotnetpj/tools 아래에 있으므로 두 단계 위다.
ROOT = Path(__file__).resolve().parents[2]
# 판본별 공통 표시 가상 함수 지원 표를 담은 cpppj 원본 목록 파일 (읽기 전용).
UNPOP_VTABLES = ROOT / "cpppj" / "src" / "o" / "UnpopVtables.inc"
# 만들어 낼 C# 데이터 파일.
OUTPUT = ROOT / "dotnetpj" / "src" / "Netstorm.Core" / "Original" / "OriginalUnpopDisplay.Data.cs"
# 10.78 표의 항목 수. 입력의 길이가 이와 다르면 입력이 바뀐 것이므로 중단한다.
PATCH_ENTRY_COUNT = 83


def sha256_lf(path: Path) -> str:
    """파일의 CRLF 를 LF 로 바꾼 바이트의 SHA-256 을 소문자 16진수로 돌려준다."""
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def parse_patch_entries(text: str) -> list[tuple[int, int]]:
    """kPatchDisplayVtables 의 (가상 표 주소, 지원 비트) 목록을 읽는다. 지원 비트는 1(일반)·2(소용돌이 경로)의 합이다."""
    start = text.index("kPatchDisplayVtables")
    begin = text.index("{{", start)
    end = text.index("}}", begin)
    entries = [(int(address, 16), int(mask))
               for address, mask in re.findall(r"\{(0x[0-9A-Fa-f]+)U,(\d+)\}", text[begin:end])]
    if len(entries) != PATCH_ENTRY_COUNT:
        raise ValueError(f"10.78 표시 지원 표가 {len(entries)}개다 (기대 {PATCH_ENTRY_COUNT})")
    return entries


def render() -> str:
    """읽은 표로 C# 데이터 파일의 전체 내용을 만든다 (LF, UTF-8)."""
    entries = parse_patch_entries(UNPOP_VTABLES.read_text(encoding="utf-8"))
    lines = [
        "// 이 파일은 dotnetpj/tools/gen_original_unpop_vtables.py 가 만든다. 직접 고치지 말고 도구를 다시 실행한다.",
        f"// 입력(cpppj, 읽기 전용): UnpopVtables.inc sha256={sha256_lf(UNPOP_VTABLES)}",
        "// 10.78 부분만 옮겼다. 결과는 원본 기계어 기대값 unpop-vtables.tsv 로 검사한다 (W02 Unpop 조각, 2026-10-16).",
        "namespace Netstorm.Core.Original;",
        "",
        "/// <summary>Unpop 의 공통 표시 경로를 쓰는 가상 표 주소와 지원 비트(10.78)를 담는 데이터 전용 클래스다. RawUnpop 이 Supports 조회에 쓴다. 값은 이 도구가 만들며 직접 고치지 않는다. (W02 Unpop 조각 2026-10-16 생성, 2026-10-20 summary 추가)</summary>",
        "public static partial class OriginalUnpopDisplay",
        "{",
        f"    /// <summary>공통 표시 경로(update88/update8c)를 쓰는 가상 표 {PATCH_ENTRY_COUNT}개(10.78)와 지원 비트(1 일반, 2 소용돌이 경로). 호스트 포인터가 아니라 클래스 식별 값이다.</summary>",
        "    private static readonly DisplayVtable[] Table =",
        "    [",
    ]
    # 항목마다 한 줄씩 쓴다.
    for address, mask in entries:
        lines.append(f"        new(0x{address:08X}, {mask}),")
    lines += ["    ];", "}", ""]
    return "\n".join(lines)


def main() -> int:
    """파일을 만들거나(--check 이면 비교만) 종료 코드를 돌려준다."""
    parser = argparse.ArgumentParser(description="cpppj Unpop 표시 지원 표에서 C# 데이터 파일을 만든다")
    parser.add_argument("--check", action="store_true", help="현재 파일과 같은지만 확인한다")
    args = parser.parse_args()
    text = render()
    if args.check:
        same = OUTPUT.exists() and OUTPUT.read_bytes().replace(b"\r\n", b"\n") == text.encode("utf-8")
        print("같다" if same else "다르다: 도구를 다시 실행한다")
        return 0 if same else 1
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"저장했다: {OUTPUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
