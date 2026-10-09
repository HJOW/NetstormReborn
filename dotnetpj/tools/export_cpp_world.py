"""게임·창 실행 없이 cpppj 콘솔 덤프(--dump-world)의 미션 시작 상태를 dotnetpj 회귀 자료로 내보낸다.

사용법 (저장소 루트에서):
    cmake -S cpppj -B cpppj/build
    cmake --build cpppj/build --config Release --target NetstormCpp
    python dotnetpj/tools/export_cpp_world.py [--exe <NetstormCpp.exe>] [--game-dir originals]

결과: dotnetpj/tests/Netstorm.Core.Tests/Fixtures/world-start-1078.tsv
    player <미션> <번호> <시작SP> <색 번호> <동맹 비트> <이름> <시작 지식> <저장 기술 수> <저장 덱 수>
    object <미션> <순번> <타입> <소유자> <영역(-1 = Chaff)> <칸x> <칸y> <프레임> <수량> <HP> <내용물 수>

--dump-world 는 원본 10.78 자료 폴더를 **읽기만** 하는 콘솔 명령이다 (docs/exe/cpp-world-reconstruction.md).
창을 만드는 --run 은 쓰지 않는다. cpppj 소스와 원본 폴더는 수정하지 않는다.
이 값은 원본 x86 실행 결과가 아니라 **cpppj 의 현재 월드 어댑터 출력**이다. 소유자 열은 cpppj 의
"저장 소유자 정규화 / geyser·buried·island 중립 / 저장하지 않는 타입은 1" 규칙을 적용한 값이다.
2026-10-10 추가 (LEFT_JOBS.dotnetpj.md 6-1·6-2).
"""

from pathlib import Path
import argparse
import hashlib
import subprocess

# 저장소 루트. 결과는 dotnetpj 안에만 쓴다.
ROOT = Path(__file__).resolve().parents[2]
# 대조하는 10.78 미션 (docs/exe/cpp-world-reconstruction.md 검사 결과 표와 같은 네 개).
MISSIONS = ("thewarbegins", "savetheisland", "tutorial1", "TEST01")
# cpppj `object` 줄에서 남기는 열: 순번·타입·소유자·영역·칸x·칸y·프레임·수량·HP·내용물 수.
# (실수 좌표·걷기·목표 칸은 시작 시점에 칸 좌표·0 과 같아 뺀다.)
OBJECT_COLUMNS = (1, 2, 3, 4, 5, 6, 9, 13, 14, 15)
# 출처를 남길 cpppj 소스 (소유자·시작 값·월드 조립).
SOURCES = ("cpppj/src/client/GameWorld.cpp", "cpppj/src/o/Player.cpp", "cpppj/src/o/Template.cpp")


def dump(exe: Path, game_dir: str, mission: str) -> list[str]:
    """미션 하나의 콘솔 덤프에서 player·object 줄만 골라 미션 이름을 붙여 돌려준다."""
    result = subprocess.run([str(exe), "--dump-world", game_dir, mission], check=True, capture_output=True,
                            text=True, encoding="utf-8", cwd=ROOT)
    rows = []
    # 덤프의 줄마다 종류를 보고 필요한 열만 남긴다.
    for line in result.stdout.splitlines():
        fields = line.split("\t")
        if fields[0] == "player":
            rows.append("\t".join(["player", mission] + fields[1:]))
        elif fields[0] == "object":
            rows.append("\t".join(["object", mission] + [fields[i] for i in OBJECT_COLUMNS]))
    if not any(row.startswith("player\t") for row in rows) or not any(row.startswith("object\t") for row in rows):
        raise ValueError(f"{mission}: 덤프에 player/object 줄이 없습니다.")
    return rows


def main():
    """네 미션을 덤프해 fixture 를 다시 쓴다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", default=str(ROOT / "cpppj/build/bin/Release/NetstormCpp.exe"))
    parser.add_argument("--game-dir", default="originals")
    args = parser.parse_args()
    exe = Path(args.exe)
    if not exe.exists():
        raise SystemExit(f"cpppj 실행 파일이 없습니다: {exe} (먼저 cpppj 를 Release 로 빌드한다)")
    header = "# x86 직접 출력이 아닌, cpppj --dump-world(콘솔, 창·게임 실행 없음)의 10.78 미션 시작 상태.\n"
    header += "# player 미션 번호 시작SP 색번호 동맹비트 이름 시작지식 저장기술수 저장덱수 (탭 구분)\n"
    header += "# object 미션 순번 타입 소유자 영역 칸x 칸y 프레임 수량 HP 내용물수 (탭 구분, 영역 -1 = Chaff)\n"
    header += "# 아래 SHA256 은 줄 끝을 LF 로 맞춘 내용의 값이다 (Windows 체크아웃의 CRLF 와 무관하게 같다).\n"
    # 월드 조립 소스의 SHA 를 남겨 어느 cpppj 상태의 출력인지 알 수 있게 한다.
    for name in SOURCES:
        normalized = (ROOT / name).read_bytes().replace(b"\r\n", b"\n")
        header += f"# SHA256 {name} {hashlib.sha256(normalized).hexdigest()}\n"
    rows = []
    # 미션 순서대로 덤프를 이어 붙인다.
    for mission in MISSIONS:
        rows += dump(exe, args.game_dir, mission)
    output = ROOT / "dotnetpj/tests/Netstorm.Core.Tests/Fixtures/world-start-1078.tsv"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(header + "\n".join(rows) + "\n", encoding="utf-8", newline="\n")
    players = sum(row.startswith("player\t") for row in rows)
    print(f"{output.relative_to(ROOT)}: player {players}줄, object {len(rows) - players}줄. 원본 게임 실행 없음.")


if __name__ == "__main__":
    main()
