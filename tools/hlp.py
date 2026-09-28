# -*- coding: utf-8 -*-
"""helpdeco로 원본 WinHelp 파일을 풀고 토픽별 UTF-8 텍스트를 만든다."""

import argparse
import json
import re
import subprocess
from pathlib import Path


# 본문이 아닌 RTF 서식표, 각주, 숨긴 링크 대상은 텍스트에서 제외한다.
SKIPPED_GROUPS = {"fonttbl", "colortbl", "stylesheet", "footnote", "info", "pict", "object", "v", "up"}

# 본문에 나타나는 RTF 특수 문자를 일반 문자로 바꾼다.
CONTROL_SYMBOLS = {"~": " ", "-": "", "_": "-"}

# helpdeco의 토픽 목록에서 번호와 제목을 찾는다.
TOPIC_TITLE = re.compile(r"^Topic (\d+): (.*)$", re.MULTILINE)


def read_topics(rtf: bytes) -> list[str]:
    """helpdeco가 생성한 RTF에서 서식을 제거하고 페이지별 본문을 반환한다."""
    source = rtf.decode("cp1252", errors="replace")
    topics = []
    current = []
    skipped = [False]
    position = 0
    # 그룹 중첩과 제어어를 차례로 읽으며 보이는 본문만 모은다.
    while position < len(source):
        char = source[position]
        if char == "{":
            marker = re.match(r"\\(\*|[A-Za-z]+)", source[position + 1:])
            skipped.append(skipped[-1] or (marker is not None and marker.group(1) in SKIPPED_GROUPS | {"*"}))
            position += 1
            continue
        if char == "}":
            if len(skipped) == 1:
                raise ValueError("RTF 그룹 닫기가 너무 많습니다")
            skipped.pop()
            position += 1
            continue
        if char != "\\":
            if not skipped[-1] and char not in "\r\n":
                current.append(char)
            position += 1
            continue
        if position + 1 >= len(source):
            break
        next_char = source[position + 1]
        if next_char in "\\{}":
            if not skipped[-1]:
                current.append(next_char)
            position += 2
            continue
        if next_char == "'" and position + 3 < len(source):
            if not skipped[-1]:
                current.append(bytes.fromhex(source[position + 2:position + 4]).decode("cp1252", errors="replace"))
            position += 4
            continue
        if not next_char.isalpha():
            if not skipped[-1]:
                current.append(CONTROL_SYMBOLS.get(next_char, ""))
            position += 2
            continue
        end = position + 1
        # 영문 제어어 이름과 뒤따르는 선택적 정수 인자를 읽는다.
        while end < len(source) and source[end].isalpha():
            end += 1
        word = source[position + 1:end]
        number_start = end
        if end < len(source) and source[end] == "-":
            end += 1
        # 길이·유니코드 제어어의 숫자 인자를 끝까지 읽는다.
        while end < len(source) and source[end].isdigit():
            end += 1
        number = source[number_start:end]
        if end < len(source) and source[end] == " ":
            end += 1
        position = end
        if skipped[-1]:
            continue
        if word == "page":
            topics.append("".join(current).strip())
            current = []
        elif word in ("par", "line", "row"):
            current.append("\n")
        elif word in ("tab", "cell"):
            current.append("\t")
        elif word == "u" and number:
            current.append(chr(int(number) & 0xFFFF))
            # RTF 유니코드 제어어의 뒤따르는 대체 문자 하나를 건너뛴다.
            if position < len(source) and source[position] not in "\\{}":
                position += 1
        elif word == "bin" and number:
            position += int(number)
    if len(skipped) != 1:
        raise ValueError("RTF 그룹이 닫히지 않았습니다")
    if current and "".join(current).strip():
        topics.append("".join(current).strip())
    return topics


def run_helpdeco(executable: Path, source: Path, output: Path, option: str) -> str:
    """한 HLP를 지정된 출력 폴더에서 helpdeco로 처리한다."""
    result = subprocess.run(
        [str(executable), str(source), option], cwd=output, capture_output=True,
        text=True, encoding="cp1252", errors="replace", check=False,
    )
    message = result.stdout + "\n" + result.stderr
    if result.returncode != 0:
        raise RuntimeError(f"{source.name} {option}: helpdeco 종료 코드 {result.returncode}\n{message}")
    return message


def extract_one(executable: Path, source: Path, output: Path) -> dict:
    """HLP 하나를 RTF·이미지로 풀고 토픽별 텍스트와 목록을 확인한다."""
    output.mkdir(parents=True, exist_ok=True)
    decompile_log = run_helpdeco(executable, source, output, "/y")
    list_log = run_helpdeco(executable, source, output, "/l")
    rtf_path = output / f"{source.stem}.rtf"
    if not rtf_path.is_file():
        raise FileNotFoundError(f"helpdeco가 {rtf_path}를 만들지 않았습니다")
    topics = read_topics(rtf_path.read_bytes())
    # 목록에 적힌 번호별 제목을 추출한 본문과 맞춰 본다.
    titles = {int(index): title.strip() for index, title in TOPIC_TITLE.findall(list_log)}
    # helpdeco /l은 모든 원본에서 RTF 마지막 페이지 뒤의 빈 가상 토픽을 한 개 더 보고한다.
    if len(titles) == len(topics) + 1 and titles.get(len(titles)) == "untitled:":
        titles.pop(len(titles))
    if len(topics) != len(titles):
        raise ValueError(f"{source.name}: RTF 본문 {len(topics)}개, helpdeco 목록 {len(titles)}개")
    lines = [f"# {source.name} — 추출한 도움말 텍스트", ""]
    # 원본 토픽 순서를 지키고 목록의 제목을 각 본문 앞에 붙인다.
    for index, body in enumerate(topics, 1):
        lines.extend((f"## {index}. {titles[index]}", "", body, ""))
    text_path = output / f"{source.stem}.txt"
    text_path.write_text("\n".join(lines), encoding="utf-8")
    (output / "helpdeco.log").write_text(decompile_log, encoding="utf-8")
    # 추출된 그림 수는 각 HLP에 고유한 출력 폴더의 BMP 파일로 센다.
    pictures = sum(1 for path in output.iterdir() if path.suffix.lower() == ".bmp")
    return {"file": source.name, "topics": len(topics), "pictures": pictures,
            "helpdecoWarning": "had problems with" in decompile_log}


def main() -> None:
    """원본 help 폴더의 모든 HLP를 추출하고 요약 파일을 쓴다."""
    parser = argparse.ArgumentParser(description="helpdeco로 WinHelp 토픽 텍스트 추출")
    parser.add_argument("--helpdeco", type=Path, required=True, help="빌드한 helpdeco.exe 경로")
    parser.add_argument("--orig", type=Path, default=Path("originals/help"), help="원본 HLP 폴더")
    parser.add_argument("--out", type=Path, default=Path("extracted/helpdeco"), help="추출 결과 폴더")
    args = parser.parse_args()
    executable = args.helpdeco.resolve()
    source_dir = args.orig.resolve()
    output_dir = args.out.resolve()
    if not executable.is_file():
        parser.error(f"helpdeco 실행 파일을 찾을 수 없습니다: {executable}")
    # 대소문자가 섞인 원본 파일 이름을 보존하며 HLP만 찾는다.
    files = sorted((path for path in source_dir.iterdir() if path.suffix.lower() == ".hlp"), key=lambda path: path.name.lower())
    if not files:
        parser.error(f"HLP 파일을 찾을 수 없습니다: {source_dir}")
    manifest = []
    # 원본의 HLP마다 별도 폴더를 사용해 동명 이미지와 RTF의 충돌을 막는다.
    for source in files:
        result = extract_one(executable, source, output_dir / source.stem.upper())
        manifest.append(result)
        print(f"{result['file']}: 토픽 {result['topics']}개, 그림 {result['pictures']}개")
    (output_dir / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
