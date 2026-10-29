"""cpppj 원본 기계어 기대값(fixture)과 dotnetpj 검사의 동기화 상태를 확인하고 SHA-256 목록을 갱신한다.

사용법 (저장소 루트에서 실행):
  python dotnetpj/tools/cpp_sync.py                         상태 보고. 읽기 전용이며 연결 fixture·생성 자료가 어긋났으면 종료 코드 1
  python dotnetpj/tools/cpp_sync.py --update                연결 fixture 의 SHA-256·행 수를 현재 파일 기준으로 갱신
  python dotnetpj/tools/cpp_sync.py --update --reflect HEAD 갱신하면서 '반영한 cpppj 커밋'도 함께 기록
  python dotnetpj/tools/cpp_sync.py --update --add sid:Core 새 fixture(sid-x86.tsv)를 목록에 등록 (사용 프로젝트 Core 또는 Assets)
  (dotnetpj 가 직접 소유한 fixture 는 --add clockkernel:Core:dotnetpj 처럼 출처를 셋째 칸에 쓴다)
  (cpppj 최상위 기존 관찰은 --add recovery-menulayout-observations.tsv:Client:cpppj-root 로 원문을 연결한다)
  python dotnetpj/tools/cpp_sync.py --ports                 포트 맵(dotnetpj/PORT_MAP.tsv) 보고: 이식 뒤 cpppj 소스가 바뀐 대응 쌍과 사라진 파일. 읽기 전용
  python dotnetpj/tools/cpp_sync.py --ports-init            C# 주석의 cpppj 소스 경로로 포트 맵을 처음 만든다 (이미 있으면 거부)
  python dotnetpj/tools/cpp_sync.py --ports-reflect HEAD --ports-file RawSidPool.cs   지정한 C# 파일의 기준 커밋을 올린다 (재대조를 마친 뒤에만)
  python dotnetpj/tools/cpp_sync.py --ports-add cpppj/src/client/Screen.cpp:Netstorm.Client/Display/Screen.cs:port   새 이식 쌍을 포트 맵에 추가한다 (기준 커밋은 현재 HEAD)

목록 파일은 dotnetpj/tests/Shared/cpp-fixture-manifest.tsv 이며 두 테스트 프로젝트가 같은 SHA-256 계산으로 검사한다.
게임·창을 실행하지 않고 cpppj·원본 파일을 수정하지 않는다 (읽기 전용 git 명령과 목록 파일 쓰기만 한다).
--update 는 반드시 해당 fixture 를 쓰는 C# 검사를 다시 대조한 뒤에 실행한다. 기대값을 C# 결과로 덮어쓰지 않는다.
(2026-10-16 W01 동기화 장치로 추가, 계획: docs/dotnet-cpp-port-plan-20261016.md 7절)
(2026-10-28 R0-3 포트 맵 추적 추가: fixture 가 아니라 cpppj 소스 변경도 경고한다. 계획: LEFT_JOBS.dotnetpj.md 0절)
"""

from dataclasses import dataclass
from pathlib import Path
import argparse
import hashlib
import re
import subprocess
import sys

# 저장소 루트. 이 도구는 dotnetpj/tools 아래에 있으므로 두 단계 위다.
ROOT = Path(__file__).resolve().parents[2]
# SHA-256 목록 파일. 테스트 프로젝트가 X86/cpp-fixture-manifest.tsv 로 복사해 읽는다.
MANIFEST = ROOT / "dotnetpj" / "tests" / "Shared" / "cpp-fixture-manifest.tsv"
# cpppj 가 소유한 fixture 폴더. 이 도구는 읽기만 한다.
CPP_FIXTURES = ROOT / "cpppj" / "tests" / "fixtures"
# 기존 복원 관찰 TSV가 있는 cpppj 최상위.사본 생성 없이 읽기 전용 Link로 연결한다.
CPP_OBSERVATIONS = ROOT / "cpppj"
# dotnetpj 가 직접 소유한 fixture 폴더 (벽/게임 시계 관찰 자료).
LOCAL_FIXTURES = ROOT / "dotnetpj" / "tests" / "Netstorm.Core.Tests" / "Fixtures"
# 목록 파일에서 '반영한 cpppj 커밋'을 적는 주석 머리말.
REFLECT_KEY = "# reflected-commit:"
# 목록의 사용 프로젝트 이름. 테스트 어셈블리 Netstorm.<이름>.Tests 와 일치해야 한다.
CONSUMERS = ("Core", "Assets", "Client")
# 목록의 출처 이름. cpppj 는 읽기 전용, dotnetpj 는 이 저장소의 dotnetpj 폴더가 소유한다.
ORIGINS = ("cpppj", "cpppj-root", "dotnetpj")
# 보고서에 펼쳐 보여 주는 커밋·파일 목록의 최대 길이.
LIST_LIMIT = 40
# 포트 맵 파일. cpppj 소스 ↔ C# 파일 ↔ 이식 기준 커밋을 한 줄씩 적는다 (2026-10-28 R0-3).
PORT_MAP = ROOT / "dotnetpj" / "PORT_MAP.tsv"
# 포트 맵을 처음 만들 때 C# 주석에서 cpppj 소스 경로를 찾는 폴더들 (dotnetpj/src 기준).
PORT_SCAN_DIRS = ("Netstorm.Core", "Netstorm.Assets", "Netstorm.Client")
# C# 주석에 적힌 cpppj 소스 경로. 폴더는 o(원본 월드 코어) 또는 client(클라이언트)다.
PORT_PATTERN = re.compile(r"cpppj/src/(o|client)/([A-Za-z0-9_]+\.(?:cpp|h|inc))")
# 포트 맵의 기본 기준 커밋. 이 커밋까지는 cpppj 변경을 이미 검토했다고 본다(W01 반영 커밋과 같다).
PORT_FLOOR_KEY = "# floor-commit:"
# 포트 맵의 status 로 허용하는 값. L1 원본 fixture 대조, session 세션 연결, port 새 경로 직역 이식, retire 대체 예정.
PORT_STATUSES = ("L1", "session", "port", "retire")
# cpppj 의 원본 목록(.inc 등)에서 C# 자료를 만드는 생성 도구들. 각 도구의 --check 가 현재 C# 자료와 cpppj 원본의 일치를 확인한다.
GENERATORS = (
    "gen_original_constructors.py",
    "gen_original_unpop_vtables.py",
    "gen_original_pop_vtables.py",
)


@dataclass
class Entry:
    """목록의 한 줄. 파일 이름·SHA-256(소문자 16진수)·주석 제외 행 수·출처·사용 프로젝트를 담는다."""

    file: str
    sha256: str
    rows: int
    origin: str
    consumer: str


def normalized(data: bytes) -> bytes:
    """CRLF 를 LF 로 바꾼다. Windows 체크아웃의 줄바꿈 차이가 SHA-256 에 섞이지 않게 한다."""
    return data.replace(b"\r\n", b"\n")


def digest(path: Path) -> tuple[str, int]:
    """파일의 (줄바꿈 정규화 SHA-256, 주석·빈 줄을 뺀 행 수)를 계산한다. 행 수 기준은 X86Fixture.Read 와 같다."""
    data = normalized(path.read_bytes())
    rows = 0
    # 줄마다 주석('#')과 빈 줄이 아닌 것만 센다.
    for line in data.split(b"\n"):
        if line and not line.startswith(b"#"):
            rows += 1
    return hashlib.sha256(data).hexdigest(), rows


def source_path(entry: Entry) -> Path:
    """목록 출처별 실제 기대값 경로.최상위 기존 관찰도 복사하거나 수정하지 않고 원문을 읽는다."""
    return {"cpppj": CPP_FIXTURES, "cpppj-root": CPP_OBSERVATIONS, "dotnetpj": LOCAL_FIXTURES}[entry.origin] / entry.file


def load_manifest() -> tuple[str, list[Entry]]:
    """목록 파일을 읽어 (반영한 커밋, 항목들)을 돌려준다. 파일이 없으면 빈 목록이다."""
    reflected = ""
    entries: list[Entry] = []
    if not MANIFEST.exists():
        return reflected, entries
    # 줄마다 머리말 주석, 일반 주석, 데이터 행을 구분한다.
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if line.startswith(REFLECT_KEY):
            reflected = line[len(REFLECT_KEY):].strip()
        elif line and not line.startswith("#"):
            file, sha, rows, origin, consumer = line.split("\t")
            entries.append(Entry(file, sha, int(rows), origin, consumer))
    return reflected, entries


def save_manifest(reflected: str, entries: list[Entry]) -> None:
    """목록 파일을 LF·UTF-8(BOM 없음)로 쓴다. 항목은 파일 이름순으로 정렬해 diff 가 안정되게 한다."""
    lines = [
        "# cpppj 원본 기계어 기대값 fixture 의 SHA-256 목록 (dotnetpj W01 동기화 장치, 2026-10-16).",
        "# dotnetpj 테스트(CppFixtureManifestTests)가 이 목록과 실제 파일을 비교한다. 다르면 cpppj 가 fixture 를 다시 만든 것이므로",
        "# 해당 C# 검사를 재대조한 뒤 `python dotnetpj/tools/cpp_sync.py --update` 로 갱신한다. 기대값을 C# 결과로 덮어쓰지 않는다.",
        "# SHA-256 은 CRLF 를 LF 로 바꾼 바이트로 계산하고, rows 는 주석·빈 줄을 뺀 행 수다.",
        f"{REFLECT_KEY} {reflected}",
        "# 열: file\tsha256\trows\torigin\tconsumer",
    ]
    # 항목마다 한 줄씩 쓴다.
    for e in sorted(entries, key=lambda item: item.file):
        lines.append(f"{e.file}\t{e.sha256}\t{e.rows}\t{e.origin}\t{e.consumer}")
    MANIFEST.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def git(*args: str) -> str | None:
    """읽기 전용 git 명령을 실행해 표준 출력을 돌려준다. git 이 없거나 실패하면 None 이다."""
    try:
        done = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    except OSError:
        return None
    return done.stdout if done.returncode == 0 else None


def status_of(entry: Entry) -> str:
    """항목의 현재 상태: OK(일치) / 변경(SHA·행 수 불일치) / 없음(파일 없음)."""
    path = source_path(entry)
    if not path.exists():
        return "없음"
    sha, rows = digest(path)
    return "OK" if sha == entry.sha256 and rows == entry.rows else "변경"


def report() -> int:
    """동기화 상태를 출력한다. 연결 fixture·생성 자료가 어긋났으면 1, 아니면 0 을 돌려준다."""
    reflected, entries = load_manifest()
    head = (git("rev-parse", "HEAD") or "").strip()
    print("== cpppj 동기화 상태 ==")
    print(f"반영한 cpppj 커밋 : {reflected or '(기록 없음)'}")
    print(f"저장소 HEAD       : {head or '(git 사용 불가)'}")

    bad = 0
    print(f"\n[연결 fixture {len(entries)}개]")
    # 항목마다 현재 파일과 기록을 비교해 한 줄씩 보여 준다.
    for e in entries:
        state = status_of(e)
        bad += state != "OK"
        print(f"  {state:<3} {e.file:<34} {e.consumer:<6} {e.rows:>6}행 ({e.origin})")
    # 연결 범위를 한눈에 보도록 cpppj fixture 전체 대비 연결 개수를 함께 보여 준다.
    cpp_total = len(list(CPP_FIXTURES.glob("*-x86.tsv")))
    cpp_linked = sum(1 for e in entries if e.origin == "cpppj" and e.file.endswith("-x86.tsv"))
    print(f"  cpppj *-x86.tsv {cpp_total}개 중 {cpp_linked}개 연결")
    print(f"  cpppj 최상위 기존 관찰 {sum(e.origin == 'cpppj-root' for e in entries)}개 읽기 전용 연결")

    # 생성한 C# 자료가 cpppj 원본 목록과 아직 같은지 각 생성기의 --check 로 확인한다.
    print(f"\n[생성 자료 {len(GENERATORS)}개]")
    for generator in GENERATORS:
        done = subprocess.run([sys.executable, "-X", "utf8", str(Path(__file__).with_name(generator)), "--check"],
                              capture_output=True, text=True, encoding="utf-8", errors="replace")
        state = "OK" if done.returncode == 0 else "다름"
        bad += done.returncode != 0
        print(f"  {state:<3} {generator}" + ("" if done.returncode == 0 else "  <- cpppj 목록이 바뀌었다. 도구를 다시 실행하고 관련 검사를 재대조한다"))

    if reflected and head:
        since = git("log", "--oneline", "--no-decorate", f"{reflected}..HEAD", "--", "cpppj", "docs/exe", "LEFT_JOBS.md")
        if since is None:
            print("\n반영한 커밋을 git 에서 찾지 못했다. 목록의 reflected-commit 을 확인한다.")
        else:
            commits = [line for line in since.splitlines() if line]
            print(f"\n[반영 이후 cpppj 관련 커밋 {len(commits)}개]")
            # 최근 커밋부터 제한 개수만 보여 준다.
            for line in commits[:LIST_LIMIT]:
                print(f"  {line}")
            if len(commits) > LIST_LIMIT:
                print(f"  … 외 {len(commits) - LIST_LIMIT}개")
            print_fixture_changes(reflected, {e.file: e for e in entries})

    # 아직 커밋되지 않은 cpppj fixture 변경은 근거로 쓰지 않는다 (HEAD 만 쓴다는 계획 7절 2번).
    dirty = git("status", "--porcelain", "--", "cpppj/tests/fixtures")
    if dirty and dirty.strip():
        print("\n주의: cpppj fixture 에 커밋되지 않은 변경이 있다. 근거로 쓰지 않는다.")
        # 변경된 파일마다 한 줄씩 보여 준다.
        for line in dirty.splitlines()[:LIST_LIMIT]:
            print(f"  {line}")

    if bad:
        print(f"\n연결 fixture {bad}개가 목록과 다르다. 해당 C# 검사를 재대조한 뒤 --update 로 갱신한다.")
    return 1 if bad else 0


def print_fixture_changes(reflected: str, linked: dict[str, Entry]) -> None:
    """반영 커밋 이후 cpppj fixture 의 추가·수정·삭제를 연결 여부와 함께 보여 준다."""
    changes = git("diff", "--name-status", f"{reflected}..HEAD", "--", "cpppj/tests/fixtures")
    if changes is None:
        return
    added: list[str] = []
    edited: list[str] = []
    removed: list[str] = []
    # 변경 줄마다 상태 글자와 파일 이름을 나눠 분류한다.
    for line in changes.splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        name = parts[-1].rsplit("/", 1)[-1]
        if parts[0].startswith("A"):
            added.append(name)
        elif parts[0].startswith("D"):
            removed.append(name)
        else:
            edited.append(name)
    print(f"\n[반영 이후 cpppj fixture 변경] 추가 {len(added)} / 수정 {len(edited)} / 삭제 {len(removed)}")
    # 수정·삭제 중 연결된 것은 재대조가 필요하다.
    for name in edited + removed:
        mark = "!" if name in linked else " "
        print(f"  {mark} {'수정' if name in edited else '삭제'} {name}" + ("  <- 연결됨, 재대조 필요" if name in linked else ""))
    # 추가된 fixture 는 아직 연결 전이므로 이름만 보여 준다.
    for name in added[:LIST_LIMIT]:
        print(f"  + 추가 {name}")
    if len(added) > LIST_LIMIT:
        print(f"  + … 외 {len(added) - LIST_LIMIT}개")


def update(reflect: str | None, adds: list[str]) -> int:
    """목록의 SHA-256·행 수를 현재 파일 기준으로 갱신한다. --add 항목은 새로 등록하고 --reflect 는 커밋을 기록한다."""
    reflected, entries = load_manifest()
    known = {e.file for e in entries}
    # --add 이름:사용프로젝트[:출처] 마다 새 항목을 만든다. 출처를 생략하면 cpppj 다.
    for item in adds:
        parts = item.split(":")
        name, consumer = parts[0], parts[1] if len(parts) > 1 else ""
        origin = parts[2] if len(parts) > 2 else "cpppj"
        file = name if name.endswith(".tsv") else f"{name}-x86.tsv"
        if consumer not in CONSUMERS or origin not in ORIGINS:
            print(f"--add {item}: 사용 프로젝트는 {'/'.join(CONSUMERS)}, 출처는 {'/'.join(ORIGINS)} 중 하나여야 한다.")
            return 2
        if file in known:
            print(f"--add {item}: 이미 등록된 fixture 다.")
            return 2
        entries.append(Entry(file, "", 0, origin, consumer))
        known.add(file)

    # 항목마다 파일을 다시 읽어 SHA-256·행 수를 채운다.
    for e in entries:
        path = source_path(e)
        if not path.exists():
            print(f"파일이 없다: {path}")
            return 2
        sha, rows = digest(path)
        if (e.sha256, e.rows) != (sha, rows):
            print(f"  갱신 {e.file}: {e.rows}행 -> {rows}행, {e.sha256[:12] or '(신규)'} -> {sha[:12]}")
        e.sha256, e.rows = sha, rows

    if reflect:
        full = (git("rev-parse", reflect) or "").strip()
        if not full:
            print(f"--reflect {reflect}: git 에서 커밋을 찾지 못했다.")
            return 2
        reflected = full
    save_manifest(reflected, entries)
    print(f"목록을 저장했다: {MANIFEST.relative_to(ROOT)} (항목 {len(entries)}개, 반영 커밋 {reflected or '(기록 없음)'})")
    return 0


@dataclass
class PortRow:
    """포트 맵의 한 줄. cpppj 소스·C# 파일(dotnetpj/src 기준)·이식 기준 커밋·상태를 담는다."""

    cpp: str
    csharp: str
    base: str
    status: str


def is_ancestor(older: str, newer: str) -> bool:
    """older 가 newer 의 조상(또는 같은 커밋)이면 참이다. git 을 쓸 수 없으면 거짓이다."""
    done = subprocess.run(["git", "-C", str(ROOT), "merge-base", "--is-ancestor", older, newer], capture_output=True)
    return done.returncode == 0


def load_ports() -> tuple[str, list[PortRow]]:
    """포트 맵을 읽는다. 첫 값은 기본 기준 커밋(floor-commit)이다. 파일이 없으면 빈 목록이다."""
    floor, rows = "", []
    if not PORT_MAP.exists():
        return floor, rows
    # 줄마다 머리말 주석이나 데이터 행을 읽는다.
    for line in PORT_MAP.read_text(encoding="utf-8").splitlines():
        if line.startswith(PORT_FLOOR_KEY):
            floor = line[len(PORT_FLOOR_KEY):].strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) >= 4:
            rows.append(PortRow(parts[0], parts[1], parts[2], parts[3]))
    return floor, rows


def save_ports(floor: str, rows: list[PortRow]) -> None:
    """포트 맵을 저장한다. 머리말 설명 뒤에 cpp 경로 순으로 정렬한 행을 쓴다(LF, UTF-8)."""
    lines = [
        "# cpppj 소스 -> dotnetpj C# 파일 포트 맵 (dotnetpj R0-3, 2026-10-28). cpp_sync.py --ports 가 읽는다.",
        "# 열: cpp(저장소 기준 경로)	csharp(dotnetpj/src 기준 경로)	base(이식·재대조를 마친 cpppj 커밋)	status",
        "# status: L1(원본 fixture 대조, 세션 미연결) / session(세션 연결) / port(새 경로 직역 이식) / retire(대체 예정)",
        "# base 이후 cpp 파일이 바뀌면 --ports 가 경고한다. 재대조를 마친 뒤 --ports-reflect 로 base 를 올린다. 기대값을 C# 결과로 바꾸지 않는다.",
        f"{PORT_FLOOR_KEY} {floor}",
    ]
    # 정렬한 행마다 한 줄을 쓴다.
    for r in sorted(rows, key=lambda x: (x.cpp, x.csharp)):
        lines.append(f"{r.cpp}\t{r.csharp}\t{r.base}\t{r.status}")
    PORT_MAP.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def ports_init() -> int:
    """C# 주석의 cpppj 소스 경로를 모아 포트 맵을 처음 만든다. 기준 커밋은 C# 파일을 마지막으로 고친 시점의 cpppj 커밋과 W01 반영 커밋 중 늦은 쪽이다."""
    if PORT_MAP.exists():
        print("PORT_MAP.tsv 가 이미 있다. 지우고 다시 만들지 않는다. 행을 직접 고치거나 --ports-reflect 를 쓴다.")
        return 2
    floor_rev, _ = load_manifest()
    floor = (git("rev-parse", floor_rev) or "").strip() if floor_rev else ""
    if not floor:
        print("목록(cpp-fixture-manifest.tsv)의 reflected-commit 을 찾지 못했다.")
        return 2
    rows = []
    # 설정한 폴더마다 C# 파일을 훑는다.
    for name in PORT_SCAN_DIRS:
        folder = ROOT / "dotnetpj" / "src" / name
        if not folder.exists():
            continue
        # 폴더 안의 C# 파일마다 주석에서 cpppj 경로를 찾는다. 빌드 산출물 폴더는 건너뛴다.
        for cs in sorted(folder.rglob("*.cs")):
            if "/obj/" in cs.as_posix() or "/bin/" in cs.as_posix():
                continue
            refs = sorted(set(PORT_PATTERN.findall(cs.read_text(encoding="utf-8", errors="replace"))))
            if not refs:
                continue
            rel = cs.relative_to(ROOT / "dotnetpj" / "src").as_posix()
            # C# 파일을 마지막으로 고친 커밋을 찾고, 그 시점까지의 마지막 cpppj 소스 커밋을 이식 시점으로 본다.
            last_cs = (git("log", "-1", "--format=%H", "--", f"dotnetpj/src/{rel}") or "").strip()
            at_port = (git("log", "-1", "--format=%H", last_cs, "--", "cpppj/src") or "").strip() if last_cs else ""
            base = floor if (not at_port or is_ancestor(at_port, floor)) else at_port
            # 주석에 적힌 cpppj 경로마다 한 행을 만든다. 실제로 있는 파일만 적는다.
            for folder_name, file in refs:
                cpp = f"cpppj/src/{folder_name}/{file}"
                if (ROOT / cpp).exists():
                    rows.append(PortRow(cpp, rel, base, "L1" if name != "Netstorm.Client" else "port"))
    save_ports(floor, rows)
    print(f"포트 맵을 만들었다: {PORT_MAP.relative_to(ROOT)} (행 {len(rows)}개, 기본 기준 커밋 {floor[:12]})")
    return 0


def ports_report() -> int:
    """포트 맵을 읽어 이식 뒤 cpppj 소스가 바뀐 쌍과 사라진 파일을 보고한다. 파일이 사라졌거나 C# 이 없으면 1 을 돌려준다."""
    floor, rows = load_ports()
    if not rows:
        print("포트 맵이 없거나 비어 있다. --ports-init 로 만든다.")
        return 2
    head = (git("rev-parse", "HEAD") or "").strip()
    print("== 포트 맵 상태 ==")
    print(f"기본 기준 커밋 : {floor[:12]}   저장소 HEAD : {head[:12]}   행 {len(rows)}개")
    bad, stale = 0, []
    # 행마다 파일 존재와 기준 커밋 이후의 변경 커밋 수를 확인한다.
    for r in rows:
        problems = []
        if not (ROOT / r.cpp).exists():
            problems.append("cpp 파일 없음")
        if not (ROOT / "dotnetpj" / "src" / r.csharp).exists():
            problems.append("C# 파일 없음")
        if problems:
            bad += 1
            print(f"  오류 {r.cpp} -> {r.csharp}: {', '.join(problems)}")
            continue
        log = git("log", "--oneline", "--no-decorate", f"{r.base}..HEAD", "--", r.cpp)
        count = len([x for x in (log or "").splitlines() if x])
        if count:
            stale.append((r, count))
    print(f"\n[기준 이후 cpppj 소스가 바뀐 쌍 {len(stale)}개 / 전체 {len(rows)}개] (재대조 필요 신호이며 불일치의 증거는 아니다)")
    # 변경 커밋이 많은 쌍부터 제한 개수만 보여 준다.
    for r, count in sorted(stale, key=lambda x: (-x[1], x[0].csharp))[:LIST_LIMIT * 2]:
        print(f"  {count:>3}커밋 {r.csharp:<48} <- {r.cpp.removeprefix('cpppj/src/')}  [{r.status}]")
    if len(stale) > LIST_LIMIT * 2:
        print(f"  … 외 {len(stale) - LIST_LIMIT * 2}개")
    return 1 if bad else 0


def ports_add(items: list[str]) -> int:
    """새 이식 쌍(cpp:csharp[:status])을 포트 맵에 추가한다. 기준 커밋은 현재 HEAD이며, 이미 있는 쌍이거나 파일이 없으면 거부한다. 새 파일을 이식할 때마다 쓴다."""
    head = (git("rev-parse", "HEAD") or "").strip()
    floor, rows = load_ports()
    if not head or not floor:
        print("포트 맵이 없거나 git HEAD 를 읽지 못했다. --ports-init 를 먼저 실행한다.")
        return 2
    known = {(r.cpp, r.csharp) for r in rows}
    # 인자마다 cpp, C#, 상태(생략하면 port)를 나눠 검사한 뒤 행을 더한다.
    for item in items:
        parts = item.split(":")
        if len(parts) not in (2, 3):
            print(f"--ports-add {item}: cpp:csharp[:status] 형식이어야 한다.")
            return 2
        cpp, cs, status = parts[0], parts[1], parts[2] if len(parts) == 3 else "port"
        if status not in PORT_STATUSES:
            print(f"--ports-add {item}: status 는 {'/'.join(PORT_STATUSES)} 중 하나여야 한다.")
            return 2
        if not (ROOT / cpp).exists() or not (ROOT / "dotnetpj" / "src" / cs).exists():
            print(f"--ports-add {item}: cpp 파일 또는 C# 파일이 없다.")
            return 2
        if (cpp, cs) in known:
            print(f"--ports-add {item}: 이미 등록된 쌍이다.")
            return 2
        rows.append(PortRow(cpp, cs, head, status))
        known.add((cpp, cs))
    save_ports(floor, rows)
    print(f"{len(items)}개 쌍을 추가했다 (기준 커밋 {head[:12]}).")
    return 0


def ports_reflect(rev: str, files: list[str]) -> int:
    """지정한 C# 파일(이름 또는 dotnetpj/src 기준 경로 끝부분)에 해당하는 행의 기준 커밋을 rev 로 올린다. 재대조를 마친 뒤에만 쓴다."""
    full = (git("rev-parse", rev) or "").strip()
    if not full:
        print(f"--ports-reflect {rev}: git 에서 커밋을 찾지 못했다.")
        return 2
    if not files:
        print("--ports-file 로 대상 C# 파일을 하나 이상 지정한다. 전체를 한 번에 올리지 않는다.")
        return 2
    floor, rows = load_ports()
    touched = set()
    # 행마다 지정한 C# 파일과 경로 끝이 맞으면 기준 커밋을 올린다.
    for r in rows:
        if any(r.csharp == f or r.csharp.endswith("/" + f) for f in files):
            r.base = full
            touched.add(r.csharp)
    missing = [f for f in files if not any(t == f or t.endswith("/" + f) for t in touched)]
    if missing:
        print(f"포트 맵에 없는 C# 파일: {', '.join(missing)}")
        return 2
    save_ports(floor, rows)
    print(f"기준 커밋을 {full[:12]} 로 올렸다: {', '.join(sorted(touched))}")
    return 0


def main() -> int:
    """명령줄 인자를 읽어 보고 또는 갱신을 실행한다."""
    # 콘솔 인코딩이 한국어 문자를 표현하지 못해도 출력 때문에 죽지 않게 한다.
    sys.stdout.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(description="cpppj fixture 와 dotnetpj SHA-256 목록의 동기화 도구")
    parser.add_argument("--update", action="store_true", help="목록의 SHA-256·행 수를 현재 파일 기준으로 갱신한다")
    parser.add_argument("--reflect", metavar="REV", help="--update 와 함께: 반영한 cpppj 커밋을 기록한다 (예: HEAD)")
    parser.add_argument("--add", metavar="NAME:CONSUMER[:ORIGIN]", action="append", default=[], help="--update 와 함께: 새 fixture 를 등록한다")
    parser.add_argument("--ports", action="store_true", help="포트 맵(dotnetpj/PORT_MAP.tsv) 기준으로 이식 뒤 바뀐 cpppj 소스를 보고한다")
    parser.add_argument("--ports-init", action="store_true", help="C# 주석의 cpppj 소스 경로로 포트 맵을 처음 만든다")
    parser.add_argument("--ports-reflect", metavar="REV", help="--ports-file 로 지정한 C# 파일의 기준 커밋을 올린다 (재대조를 마친 뒤)")
    parser.add_argument("--ports-add", metavar="CPP:CSHARP[:STATUS]", action="append", default=[], help="새 이식 쌍을 포트 맵에 추가한다 (기준 커밋은 현재 HEAD)")
    parser.add_argument("--ports-file", metavar="CSHARP", action="append", default=[], help="--ports-reflect 의 대상 C# 파일 이름 또는 경로 끝부분")
    args = parser.parse_args()
    if args.ports_add:
        return ports_add(args.ports_add)
    if args.ports_init:
        return ports_init()
    if args.ports_reflect:
        return ports_reflect(args.ports_reflect, args.ports_file)
    if args.ports:
        return ports_report()
    if args.update:
        return update(args.reflect, args.add)
    if args.reflect or args.add:
        parser.error("--reflect 와 --add 는 --update 와 함께 써야 한다")
    return report()


if __name__ == "__main__":
    sys.exit(main())
