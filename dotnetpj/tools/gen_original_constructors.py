"""cpppj 의 원본 생성자 표(.inc)에서 10.78 부분만 골라 C# 데이터 파일(OriginalConstructors.Data.cs)을 만든다.

사용법 (저장소 루트에서 실행):
  python -X utf8 dotnetpj/tools/gen_original_constructors.py            파일을 다시 만든다
  python -X utf8 dotnetpj/tools/gen_original_constructors.py --check    만든 결과가 현재 파일과 같은지만 확인한다 (다르면 종료 코드 1)

입력은 cpppj/src/o/TypeConstructors.inc 와 DerivedConstructors.inc 이며 읽기만 한다.
CD 10.72 표는 기준 판본이 아니므로 옮기지 않는다. 결과 표의 정확성은 원본 기계어 기대값
constructors-x86.tsv(주소 표)와 derived-x86.tsv(쓰기 결과)를 읽는 C# 검사가 확인한다.
(2026-10-16 W02 첫 조각으로 추가)
"""

from pathlib import Path
import argparse
import hashlib
import re
import sys

# 저장소 루트. 이 도구는 dotnetpj/tools 아래에 있으므로 두 단계 위다.
ROOT = Path(__file__).resolve().parents[2]
# 타입 번호별 생성자 주소 표를 담은 cpppj 원본 목록 파일 (읽기 전용).
TYPE_CONSTRUCTORS = ROOT / "cpppj" / "src" / "o" / "TypeConstructors.inc"
# 파생 생성자의 순서 있는 쓰기 표를 담은 cpppj 원본 목록 파일 (읽기 전용).
DERIVED_CONSTRUCTORS = ROOT / "cpppj" / "src" / "o" / "DerivedConstructors.inc"
# 만들어 낼 C# 데이터 파일.
OUTPUT = ROOT / "dotnetpj" / "src" / "Netstorm.Core" / "Original" / "OriginalConstructors.Data.cs"
# 10.78 타입 수. 주소 표의 길이가 이와 다르면 입력이 바뀐 것이므로 중단한다.
PATCH_TYPE_COUNT = 188
# 10.78 파생 생성자 수.
PATCH_RECIPE_COUNT = 82


def sha256_lf(path: Path) -> str:
    """파일의 CRLF 를 LF 로 바꾼 바이트의 SHA-256 을 소문자 16진수로 돌려준다."""
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def block_of(text: str, name: str) -> str:
    """`name` 변수의 중괄호 본문(바깥 `{`부터 짝이 맞는 `}`까지)을 잘라 돌려준다."""
    start = text.index(name)
    begin = text.index("{", start)
    depth = 0
    # 문자마다 중괄호 깊이를 세어 본문의 끝을 찾는다.
    for index in range(begin, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[begin + 1:index]
    raise ValueError(f"{name} 본문의 끝을 찾지 못했다")


def parse_int(token: str) -> int:
    """`0x00501dd0U` 나 `40` 같은 C++ 정수 표기를 값으로 바꾼다."""
    return int(token.rstrip("uUlL"), 0)


def parse_addresses(text: str) -> list[int]:
    """kPatchConstructors 의 타입 번호 순서 생성자 주소를 읽는다."""
    body = block_of(text, "kPatchConstructors")
    values = [parse_int(token) for token in re.findall(r"0x[0-9A-Fa-f]+U", body)]
    if len(values) != PATCH_TYPE_COUNT:
        raise ValueError(f"10.78 생성자 주소가 {len(values)}개다 (기대 {PATCH_TYPE_COUNT})")
    return values


def parse_recipes(text: str) -> list[tuple[int, int, list[tuple[int, int, bool, int]]]]:
    """kPatchRecipes 의 (주소, 최종 vtable, 순서 있는 쓰기 목록)을 읽는다. 쓰기는 (오프셋, 폭, OR 여부, 값)이다."""
    body = block_of(text, "kPatchRecipes")
    recipes = []
    # 항목은 한 줄에 하나다: {주소,vtable,개수,{{{쓰기},{쓰기}}}}
    for line in body.splitlines():
        match = re.match(r"\s*\{(0x[0-9A-Fa-f]+U),(0x[0-9A-Fa-f]+U),(\d+),\{\{(.*)\}\}\},?\s*$", line)
        if not match:
            continue
        writes = [(int(o), int(w), flag == "true", parse_int(v))
                  for o, w, flag, v in re.findall(r"\{(\d+),(\d+),(true|false),(0x[0-9A-Fa-f]+U|\d+U?)\}", match.group(4))]
        if len(writes) != int(match.group(3)):
            raise ValueError(f"쓰기 개수 불일치: {line.strip()}")
        recipes.append((parse_int(match.group(1)), parse_int(match.group(2)), writes))
    if len(recipes) != PATCH_RECIPE_COUNT:
        raise ValueError(f"10.78 파생 생성자가 {len(recipes)}개다 (기대 {PATCH_RECIPE_COUNT})")
    return recipes


def render() -> str:
    """읽은 표로 C# 데이터 파일의 전체 내용을 만든다 (LF, UTF-8)."""
    addresses = parse_addresses(TYPE_CONSTRUCTORS.read_text(encoding="utf-8"))
    recipes = parse_recipes(DERIVED_CONSTRUCTORS.read_text(encoding="utf-8"))
    lines = [
        "// 이 파일은 dotnetpj/tools/gen_original_constructors.py 가 만든다. 직접 고치지 말고 도구를 다시 실행한다.",
        f"// 입력(cpppj, 읽기 전용): TypeConstructors.inc sha256={sha256_lf(TYPE_CONSTRUCTORS)}",
        f"//                         DerivedConstructors.inc sha256={sha256_lf(DERIVED_CONSTRUCTORS)}",
        "// 10.78 부분만 옮겼다. 결과는 원본 기계어 기대값 constructors-x86.tsv·derived-x86.tsv 로 검사한다 (W02 첫 조각, 2026-10-16).",
        "namespace Netstorm.Core.Original;",
        "",
        "public static partial class OriginalConstructors",
        "{",
        f"    /// <summary>타입 번호 0~{PATCH_TYPE_COUNT - 1} 의 생성자 원본 주소(10.78). 0 은 기본 Squid 생성이다. 호스트 함수 주소로 쓰지 않는 식별 값이다.</summary>",
        "    private static readonly uint[] AddressTable =",
        "    [",
    ]
    # 주소 8개씩 한 줄로 늘어놓는다.
    for start in range(0, len(addresses), 8):
        lines.append("        " + ", ".join(f"0x{value:08X}" for value in addresses[start:start + 8]) + ",")
    lines += [
        "    ];",
        "",
        f"    /// <summary>지원하는 파생 생성자 {PATCH_RECIPE_COUNT}개(10.78)의 주소·최종 가상 표 값·순서 있는 필드 쓰기.</summary>",
        "    private static readonly ConstructorRecipe[] RecipeTable =",
        "    [",
    ]
    # 생성자마다 한 줄씩 쓴다.
    for address, vtable, writes in recipes:
        items = ", ".join(f"new({o}, {w}, {'true' if flag else 'false'}, 0x{v:08X})" for o, w, flag, v in writes)
        lines.append(f"        new(0x{address:08X}, 0x{vtable:08X}, [{items}]),")
    lines += ["    ];", "}", ""]
    return "\n".join(lines)


def main() -> int:
    """파일을 만들거나(--check 이면 비교만) 종료 코드를 돌려준다."""
    parser = argparse.ArgumentParser(description="cpppj 생성자 표에서 C# 데이터 파일을 만든다")
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
