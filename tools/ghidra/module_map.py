# -*- coding: utf-8 -*-
"""
디컴파일 결과(extracted/decomp/Netstorm.c)에서 assert 메시지에 남은 원본 소스 파일 이름을 모아
"소스 파일 → 함수 주소 목록" 모듈 맵을 Markdown 으로 만든다.

사용법:
    python tools/ghidra/module_map.py [입력 .c] [출력 .md]
    (기본값: extracted/decomp/Netstorm.c → docs/exe/modules.md)
"""
import collections
import os
import re
import sys

# 디컴파일 파일의 함수 구분 줄 ("// ==== FUN_004dad20 @ 004dad20")
FUNC_HEADER_RE = re.compile(r"^// ==== (\S+) @ ([0-9a-fA-F]+)")

# 문자열 상수 안의 소스 파일 이름 ("\\Ns\\O\\Squid.cpp", ".\\Screen.cpp")
SOURCE_FILE_RE = re.compile(r'"(?:[^"]*\\\\)?([A-Za-z0-9_]+\.(?:cpp|h|c))"')

# 소스 파일 하나당 표에 나열할 최대 함수 수
MAX_FUNCS_LISTED = 40


def build_map(decomp_path: str) -> dict:
    """소스 파일 이름 → [(함수 이름, 주소)] 사전을 만든다"""
    modules = collections.defaultdict(list)
    current = None
    seen = set()
    with open(decomp_path, encoding="utf-8", errors="replace") as f:
        # 디컴파일 파일을 한 줄씩 읽으며 현재 함수와 등장한 소스 파일을 짝짓는다
        for line in f:
            m = FUNC_HEADER_RE.match(line)
            if m:
                current = (m.group(1), m.group(2))
                continue
            if current is None:
                continue
            # 한 줄에 여러 소스 파일 이름이 나올 수 있으므로 모두 처리
            for src in SOURCE_FILE_RE.findall(line):
                key = (src.lower(), current)
                if key not in seen:
                    seen.add(key)
                    modules[src].append(current)
    return modules


def write_markdown(modules: dict, out_path: str):
    """모듈 맵을 Markdown 표로 저장한다"""
    lines = [
        "# Netstorm.exe 모듈 맵 (자동 생성)",
        "",
        "> `python tools/ghidra/module_map.py` 로 생성. 직접 수정하지 말 것.",
        "> assert 메시지에 남은 원본 소스 파일 이름을 기준으로, 그 파일 이름을 참조하는 함수를 모았다.",
        "> 함수가 다른 모듈의 인라인 코드를 포함할 수 있으므로 소속은 추정이다.",
        "",
        f"소스 파일 {len(modules)}개",
        "",
        "| 소스 파일 | 함수 수 | 함수 (주소) |",
        "|---|---|---|",
    ]
    # 함수가 많은 모듈부터 나열
    for src, funcs in sorted(modules.items(), key=lambda kv: (-len(kv[1]), kv[0].lower())):
        listed = ", ".join(f"`{addr}`" for _, addr in sorted(funcs, key=lambda x: x[1])[:MAX_FUNCS_LISTED])
        more = " …" if len(funcs) > MAX_FUNCS_LISTED else ""
        lines.append(f"| {src} | {len(funcs)} | {listed}{more} |")
    lines.append("")
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def main():
    """명령줄 인자 처리"""
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join("extracted", "decomp", "Netstorm.c")
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join("docs", "exe", "modules.md")
    modules = build_map(src)
    write_markdown(modules, out)
    print(f"소스 파일 {len(modules)}개 → {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
