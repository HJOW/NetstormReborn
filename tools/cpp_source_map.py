# -*- coding: utf-8 -*-
"""
두 판본 exe 에 남은 원본 소스 경로 문자열(assert 메시지)을 모아
"원본 소스 파일 → cpppj 경로" 표(cpppj/SOURCE_MAP.md)를 만든다.

원본 exe 는 읽기만 한다. 게임을 실행하지 않는다.

사용법 (저장소 루트에서):
    python tools/cpp_source_map.py
"""
import os
import re
import sys

# 저장소 루트 (이 파일의 상위 폴더)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 판본 이름 → exe 경로. 패치판은 이름이 "Clientmain.cpp" 처럼 바뀌어 있고, CD판에는 원래 대소문자가 남아 있다.
EDITIONS = {
    "patch": os.path.join(ROOT, "originals", "Netstorm.exe"),
    "cd": os.path.join(ROOT, "originalCD", "NETSTORM.EXE"),
}

# 패치판 함수 수를 읽어 올 모듈 맵 (tools/ghidra/module_map.py 가 만든다)
MODULE_MAP_PATH = os.path.join(ROOT, "docs", "exe", "modules.md")

# 결과 파일과, 존재 여부를 확인할 cpppj 소스 폴더
OUTPUT_PATH = os.path.join(ROOT, "cpppj", "SOURCE_MAP.md")
CPP_SOURCE_DIR = os.path.join(ROOT, "cpppj", "src")

# exe 안의 인쇄 가능한 ASCII 문자열 (4글자 이상)
STRING_RE = re.compile(rb"[\x20-\x7e]{4,}")

# 문자열 끝의 "폴더 + 소스 파일 이름". 폴더는 원본 빌드 PC 의 경로 표기 그대로다.
SOURCE_PATH_RE = re.compile(
    r"(?P<dir>c:\\ns\\o\\|\\ns\\o\\|\.\.\\o\\|\\ns\\zacket\\|\.\\)?"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*\.(?:cpp|h))$",
    re.IGNORECASE,
)

# 폴더 없이 파일 이름만 남은 문자열(CD판 클라이언트 모듈)로 받아들일 최소 이름 길이.
# 코드 바이트가 우연히 "j.h" 같은 꼴이 되는 것을 걸러 낸다.
MIN_BARE_NAME_LENGTH = 6

# 원본 폴더 분류 → (표에 적을 원본 폴더, cpppj/src 아래 폴더)
FOLDERS = {
    "o": ("`\\Ns\\O\\`", "o"),
    "zacket": ("`\\Ns\\Zacket\\`", "zacket"),
    "client": ("`.\\` (클라이언트)", "client"),
}

# 모듈 맵 표의 한 줄 ("| Squid.cpp | 42 | ...")
MODULE_ROW_RE = re.compile(r"^\| ([A-Za-z0-9_]+\.(?:cpp|h)) \| (\d+) \|", re.IGNORECASE)


def classify(directory: str):
    """경로 표기에서 원본 폴더 분류를 정한다. 폴더가 없으면 None."""
    lowered = directory.lower()
    if "zacket" in lowered:
        return "zacket"
    if lowered.endswith("\\o\\"):
        return "o"
    if lowered == ".\\":
        return "client"
    return None


def scan_exe(path: str) -> dict:
    """exe 하나에서 소스 파일을 모은다. 돌려주는 값: 소문자 이름 → (표기, 폴더 분류)"""
    with open(path, "rb") as f:
        data = f.read()
    found = {}
    # exe 의 모든 문자열 가운데 소스 경로로 끝나는 것을 고른다
    for match in STRING_RE.finditer(data):
        text = match.group().decode("ascii")
        m = SOURCE_PATH_RE.search(text)
        if not m:
            continue
        name = m.group("name")
        folder = classify(m.group("dir") or "")
        if folder is None:
            # 폴더 없이 이름만 있는 경우: CD판의 클라이언트 .cpp 만 받아들인다
            if not name.lower().endswith(".cpp") or len(name) < MIN_BARE_NAME_LENGTH:
                continue
            # 이름 앞에 다른 글자가 붙어 있으면(코드 바이트) 한 글자까지만 허용한다
            if len(text) - len(name) > 1:
                continue
            folder = "client"
        key = name.lower()
        previous = found.get(key)
        # 같은 파일이 여러 표기로 나오면 공용 폴더(o) 쪽을 우선한다 ("..\o\squid.h" 와 ".\" 의 혼동 방지)
        if previous is None or (previous[1] != "o" and folder == "o"):
            found[key] = (name, folder)
    return found


def read_function_counts() -> dict:
    """모듈 맵에서 패치판의 소스 파일별 함수 수를 읽는다. 파일이 없으면 빈 사전."""
    counts = {}
    if not os.path.exists(MODULE_MAP_PATH):
        return counts
    with open(MODULE_MAP_PATH, encoding="utf-8") as f:
        # 표의 각 줄에서 파일 이름과 함수 수를 뽑는다
        for line in f:
            m = MODULE_ROW_RE.match(line)
            if m:
                counts[m.group(1).lower()] = int(m.group(2))
    return counts


def target_name(patch_name, cd_name) -> str:
    """cpppj 에서 쓸 파일 이름: CD판 표기(없으면 패치판 표기)의 첫 글자를 대문자로 한다."""
    name = cd_name or patch_name
    if name.startswith("_"):
        # "_p_" 로 시작하는 파일은 원본 표기를 그대로 둔다
        return name
    return name[0].upper() + name[1:]


def build_rows() -> list:
    """표의 행을 만든다. 각 행은 사전이다."""
    patch = scan_exe(EDITIONS["patch"])
    cd = scan_exe(EDITIONS["cd"])
    counts = read_function_counts()
    rows = []
    # 두 판본에 나온 모든 파일 이름(소문자)을 합쳐 한 행씩 만든다
    for key in sorted(set(patch) | set(cd)):
        patch_entry = patch.get(key)
        cd_entry = cd.get(key)
        # 폴더는 한 판본이라도 공용(o)으로 적혀 있으면 공용이다
        folders = {entry[1] for entry in (patch_entry, cd_entry) if entry}
        folder = "o" if "o" in folders else sorted(folders)[0]
        name = target_name(patch_entry[0] if patch_entry else None, cd_entry[0] if cd_entry else None)
        relative = f"src/{FOLDERS[folder][1]}/{name}"
        rows.append({
            "folder": folder,
            "name": name,
            "patch": patch_entry[0] if patch_entry else "",
            "cd": cd_entry[0] if cd_entry else "",
            "functions": counts.get(key),
            "path": relative,
        })
    # 헤더의 대소문자를 짝이 되는 .cpp 에 맞춘다 (rifttype.h 는 패치판에만 있어 "Rifttype.h" 가 되지만 짝은 RiftType.cpp 다)
    stems = {row["name"][:-4].lower(): row["name"][:-4] for row in rows if row["name"].lower().endswith(".cpp")}
    for row in rows:
        stem = row["name"][:-2]
        if row["name"].lower().endswith(".h") and stem.lower() in stems:
            row["name"] = stems[stem.lower()] + ".h"
            row["path"] = f"src/{FOLDERS[row['folder']][1]}/{row['name']}"
    # cpppj 에 그 경로의 파일이 이미 있는지 확인한다
    for row in rows:
        row["exists"] = os.path.exists(os.path.join(ROOT, "cpppj", row["path"].replace("/", os.sep)))
    return rows


def write_markdown(rows: list):
    """표를 Markdown 으로 저장한다."""
    lines = [
        "# 원본 소스 파일 → cpppj 경로 (자동 생성)",
        "",
        "> `python tools/cpp_source_map.py` 로 생성. 직접 수정하지 말 것.",
        "> 두 판본 exe 의 assert 문자열에 남은 원본 소스 경로를 모았다. assert 가 없는 소스 파일은 여기에 나오지 않는다.",
        "> **cpppj 경로**의 파일 이름은 CD판 표기(원래 대소문자가 남아 있다)의 첫 글자를 대문자로 한 것이다.",
        "> **함수 수**는 패치판에서 그 파일 이름을 참조하는 함수의 수다([모듈 맵](../docs/exe/modules.md)). 그 파일의 전체 함수 수가 아니다.",
        "> **상태**는 cpppj 에 그 경로의 파일이 있는지만 본다. 파일이 있어도 일부 함수만 옮긴 것일 수 있다.",
        "",
    ]
    # 폴더별로 표를 하나씩 쓴다
    for folder in ("o", "client", "zacket"):
        selected = [row for row in rows if row["folder"] == folder]
        done = sum(1 for row in selected if row["exists"])
        lines.append(f"## {FOLDERS[folder][0]} → `src/{FOLDERS[folder][1]}/` ({len(selected)}개, 파일 있음 {done}개)")
        lines.append("")
        lines.append("| cpppj 경로 | 패치판 표기 | CD판 표기 | 함수 수 | 상태 |")
        lines.append("|---|---|---|---|---|")
        # 파일 이름순으로 한 줄씩 쓴다
        for row in sorted(selected, key=lambda r: r["name"].lower()):
            functions = "" if row["functions"] is None else str(row["functions"])
            status = "있음" if row["exists"] else ""
            lines.append(
                f"| `{row['path']}` | {row['patch'] or '—'} | {row['cd'] or '—'} | {functions} | {status} |"
            )
        lines.append("")
    with open(OUTPUT_PATH, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def main():
    """표를 만들어 저장하고 요약을 출력한다."""
    rows = build_rows()
    write_markdown(rows)
    done = sum(1 for row in rows if row["exists"])
    print(f"원본 소스 파일 {len(rows)}개 (cpppj 에 파일 있음 {done}개) → {os.path.relpath(OUTPUT_PATH, ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
