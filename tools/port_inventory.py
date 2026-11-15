# -*- coding: utf-8 -*-
"""cpppj 소스 대비 dotnetpj·webpj 의 남은 이식량을 영역별로 집계하는 읽기 전용 도구.

무엇을 하는가:
    cpppj/src 의 소스 단위(같은 디렉터리의 같은 이름 .cpp/.h/.inc 묶음)를 모두 세고,
    dotnetpj/PORT_MAP.tsv(cpp 파일 -> C# 파일)와 webpj/PORT_MAP.tsv(단위 -> 웹 모듈·상태)에 대조해
    "어느 영역에 얼마나 남았는가"를 표로 출력한다. 파일을 고치지 않는다.

사용법(저장소 루트에서):
    python -X utf8 tools/port_inventory.py                 # 영역별 요약 표(마크다운)
    python -X utf8 tools/port_inventory.py --families dn   # dotnetpj 대응표에 없는 단위를 영역·이름 계열별로
    python -X utf8 tools/port_inventory.py --families wb   # webpj 의 todo/partial 단위를 영역별로
    python -X utf8 tools/port_inventory.py --tsv out.tsv   # 단위별 원자료(영역, 디렉터리, 이름, 줄 수, dn 상태, wb 분류/상태)

주의:
    - 줄 수는 현재 작업 트리의 cpppj 소스 줄 수(주석 포함)다. webpj 문서의 "기준선 줄 수"와 다르다.
    - dotnetpj 의 "대응 있음"은 PORT_MAP 에 행이 있다는 뜻이며 그 파일을 끝까지 옮겼다는 뜻이 아니다.
    - 영역은 이름의 낱말로 정한 근사 분류다. 단계 배정의 근거 자료로만 쓰고 완료 판정에 쓰지 않는다.

2026-11-15 추가: LEFT_JOBS.dotnetpj.md·LEFT_JOBS.webpj.md 의 단계별 남은 양 표를 다시 만들 수 있게 하려고 넣었다.
"""
import argparse
import collections
import os
import re
import sys

# 저장소 루트(이 파일의 상위 디렉터리)와 cpppj 소스 루트.
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "cpppj", "src")

# 소스 단위로 세는 확장자.
SOURCE_EXTENSIONS = (".cpp", ".h", ".inc")

# 영역 이름. 키는 표의 정렬 순서와 문서의 영역 기호로 함께 쓴다.
AREAS = collections.OrderedDict([
    ("A", "자료·파일·설정"),
    ("B", "월드 코어(SID·슬롯·Pop·해시)"),
    ("C", "Fort·지형·섬 적재"),
    ("D", "세션·월드 조립·로그인·패킷·Client 전역"),
    ("E", "표시·렌더러·카메라·HUD·시각 효과"),
    ("F", "입력·커서·선택·게임 메뉴"),
    ("G", "UI 프레임워크·메뉴(Gump·HTML·Tell)"),
    ("H", "사제·운반·수확·경로"),
    ("I", "건설·생산·경제·다리"),
    ("J", "전투·비행·투사체·방어"),
    ("K", "AI"),
    ("L", "신전·제단·의식·승패"),
    ("M", "미션 수명·안내·대본·튜토리얼"),
    ("N", "소리·음악·동영상"),
    ("O", "채팅·메시지"),
    ("X", "요구사항 전용(4~14차)"),
    ("Y", "Edit·시나리오 편집"),
    ("Z", "멀티플레이·LAN·Outpost"),
    ("V", "root(루트 서버 독립 모듈)"),
    ("W", "호스트·플랫폼·개발 도구(app·platform)"),
])

# 영역별 낱말 규칙. 위에서부터 첫 일치를 쓴다. 이름을 CamelCase 낱말로 나눈 뒤 낱말 단위로 맞춘다
# (부분 문자열로 맞추면 ChatWindow 의 Wind 가 전투로 잘못 분류된다).
TOKEN_RULES = [
    ("Z", {"Lan", "Tcp", "Udp", "Meta", "Challenge", "Commander", "Lobby", "Reconnect", "Rank",
           "Outpost", "Spring3d", "Zsend", "Discovery"}),
    ("Y", {"Editor", "Scenario", "Existing"}),
    ("X", {"Mcp", "Fsr", "Gpu", "Korean", "Japanese", "Language", "Difficulty", "Skirmish", "Cd",
           "Autoplay", "Test"}),
    ("K", {"Ai"}),
    ("N", {"Sound", "Music", "Audio", "Movie", "Smacker", "Ambient"}),
    ("O", {"Chat", "Message", "Warning", "Dot"}),
    ("F", {"Cursor", "User", "Pick", "Selection", "Highlight", "Shortcut", "Input", "Recheck"}),
    ("L", {"Temple", "Altar", "Ritual", "Sacrifice", "Rune", "Vortex", "Capture", "Outcome", "Dais"}),
    ("J", {"Gun", "Bomb", "Fence", "Archer", "Crossbow", "Thunder", "Acid", "Ice", "Sun", "Wind",
           "Rain", "Hit", "Damage", "Damageable", "Burning", "Dissolve", "Shrapnel", "Lightning",
           "Fiery", "Smoke", "Flyer", "Aviary", "Balloon", "Missile", "Projectile", "Cannon",
           "Meltdown", "Devastation", "Detonate", "Motion", "Forcefield", "Evil", "Flare", "Mano",
           "Bulwark", "Spell", "Arc", "Special", "Board", "Snart", "Blast"}),
    ("H", {"Priest", "Carrier", "Path", "Geyser", "Nugget", "Bulf", "Prayer", "Panic", "Dude",
           "Walker", "Salvage", "Gem"}),
    ("I", {"Construction", "Production", "Factory", "Workshop", "Deck", "Energy", "Sp", "Money",
           "Wallet", "Reward", "Bridge", "Canon", "Placement", "Builder", "Research", "Upgrade",
           "Technology", "Knowledge", "Mana", "Theme", "Residence", "Building", "Storm"}),
    ("M", {"Mission", "Tutorial", "Guide", "Dialog", "Script", "Demo", "Briefing"}),
    ("E", {"Renderer", "Render", "Draw", "Sprite", "VFX", "Palette", "Color", "Camera", "Hud",
           "Minimap", "Kencloud", "Particle", "Particles", "Preview", "Display", "Screen", "Tile",
           "Collection", "Inventory", "Floating", "Shape", "Visual", "Edge", "Shadow", "Screenshot"}),
    ("G", {"Gump", "Html", "Tell", "Markup", "Style", "Quick", "Menu", "Button", "Scroll", "Text",
           "Font", "Uber", "Slider", "Widget", "Options", "Product", "Help", "Title", "Intro",
           "Loading", "Welcome"}),
    ("D", {"Session", "Campaign", "Surface", "World", "Packet", "Login", "Connection", "Transport",
           "Subscriber", "Player", "Alliance", "State", "Client", "Kernel", "Process", "Regular",
           "Form", "Clock", "Time", "Monitor", "Local", "Saved", "Game"}),
    ("C", {"Fort", "Terrain", "Island", "Islandbuilder", "Isle", "Farm", "Region", "Territory",
           "Template", "Graph"}),
    ("B", {"Squid", "Sid", "Hash", "Spatial", "Pop", "Unpop", "Content", "Contained", "Slot",
           "Type", "Derived", "Storage", "Finder", "Owner", "Checking", "Depend", "Create",
           "Destroy", "Frame", "Object", "Total", "Chunk", "Buried", "Allocation"}),
]

# CamelCase 이름을 낱말로 나누는 정규식(연속 대문자 약어, 대문자로 시작하는 낱말, 소문자·숫자 덩어리).
TOKEN_PATTERN = re.compile(r"[A-Z]+(?![a-z])|[A-Z][a-z0-9]*|[a-z0-9]+")

# 계열 이름을 만들 때 다음 낱말까지 붙이는 흔한 머리 낱말.
FAMILY_HEADS = {"Raw", "Client", "Campaign", "Sound", "Html", "Gump", "Squid", "Native", "Editor",
                "Quick", "Style", "Menu", "Local", "Game", "Uber", "Hud", "Mission"}


def tokens_of(stem):
    """이름을 CamelCase 낱말 목록으로 나눈다. 예: RawCarrierDrop -> [Raw, Carrier, Drop]."""
    return TOKEN_PATTERN.findall(stem)


def area_of(directory, stem, web_class):
    """단위 하나의 영역 기호를 정한다.

    directory 는 src 아래 첫 디렉터리, stem 은 확장자를 뺀 파일 이름, web_class 는 webpj 대응표의 분류
    (없으면 빈 문자열)다. webpj 가 멀티플레이·Edit 로 제외한 단위는 그 판정을 먼저 따른다.
    """
    if directory in ("app", "platform"):
        return "W"
    if directory == "root":
        return "V"
    if web_class == "exclude-mp":
        return "Z"
    if web_class == "exclude-edit":
        return "Y"
    toks = set(tokens_of(stem))
    # 시각 시스템은 이름에 전투·건설 낱말이 섞여 있어도 표시 영역으로 본다.
    if stem.endswith("VisualSystem") or stem.endswith("VisualRenderer"):
        return "E"
    # Native* 는 대부분 LAN·메타 맵 화면이다. 웹이 단독 경로로 분류한 것만 예외로 둔다.
    if stem.startswith("Native") and web_class not in ("port", "adapt"):
        return "Z"
    # Battle 낱말은 일반 전투(MP) 수명이다. 웹이 단독 경로로 분류한 것은 세션 영역으로 둔다.
    if "Battle" in toks:
        return "D" if web_class in ("port", "adapt") else "Z"
    # 규칙 표를 위에서부터 훑어 첫 일치를 쓴다.
    for key, words in TOKEN_RULES:
        if toks & words:
            return key
    return "A"


def family_of(stem):
    """이름 계열(같은 머리 낱말을 쓰는 단위 묶음)의 표시 이름을 만든다. 예: RawCarrierDrop -> RawCarrier."""
    toks = tokens_of(stem)
    if not toks:
        return stem
    if toks[0] in FAMILY_HEADS and len(toks) > 1:
        return toks[0] + toks[1]
    return toks[0]


def read_units():
    """cpppj/src 를 훑어 {(디렉터리, 이름): {"lines": 줄 수, "files": [경로]}} 를 만든다."""
    units = collections.OrderedDict()
    # src 바로 아래 디렉터리(app·client·o·platform·root·zacket)마다 훑는다.
    for directory in sorted(os.listdir(SRC)):
        top = os.path.join(SRC, directory)
        if not os.path.isdir(top):
            continue
        # 하위 디렉터리까지 모든 소스 파일을 센다.
        for base, _, files in os.walk(top):
            for name in sorted(files):
                stem, ext = os.path.splitext(name)
                if ext.lower() not in SOURCE_EXTENSIONS:
                    continue
                full = os.path.join(base, name)
                with open(full, "rb") as handle:
                    count = handle.read().count(b"\n")
                unit = units.setdefault((directory, stem), {"lines": 0, "files": []})
                unit["lines"] += count
                unit["files"].append(os.path.relpath(full, ROOT).replace("\\", "/"))
    return units


def read_dotnet_map():
    """dotnetpj/PORT_MAP.tsv 를 읽어 {cpp 파일 경로: {상태}} 를 만든다."""
    result = collections.defaultdict(set)
    with open(os.path.join(ROOT, "dotnetpj", "PORT_MAP.tsv"), encoding="utf-8") as handle:
        # 주석·머리 행을 뺀 자료 행마다 cpp 열과 status 열을 모은다.
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            cells = line.rstrip("\n").split("\t")
            if len(cells) < 4 or cells[0] == "cpp":
                continue
            result[cells[0]].add(cells[3])
    return result


def read_web_map():
    """webpj/PORT_MAP.tsv 를 읽어 {(디렉터리, 이름): (분류, 상태)} 를 만든다."""
    result = {}
    with open(os.path.join(ROOT, "webpj", "PORT_MAP.tsv"), encoding="utf-8") as handle:
        # 주석·머리 행을 뺀 자료 행마다 대표 파일의 단위 키로 분류와 상태를 적는다.
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            cells = line.rstrip("\n").split("\t")
            if len(cells) < 5 or cells[0] == "cpp":
                continue
            parts = cells[0].split("/")
            result[(parts[2], os.path.splitext(parts[-1])[0])] = (cells[1], cells[4])
    return result


def build_rows():
    """단위마다 (영역, 디렉터리, 이름, 줄 수, dotnetpj 상태, webpj 분류, webpj 상태) 행을 만든다."""
    units = read_units()
    dotnet = read_dotnet_map()
    web = read_web_map()
    rows = []
    # 단위마다 두 대응표의 상태를 붙인다.
    for (directory, stem), unit in units.items():
        states = set()
        # 단위에 속한 파일 중 하나라도 dotnetpj 대응표에 있으면 대응 있음으로 본다.
        for path in unit["files"]:
            states |= dotnet.get(path, set())
        if "session" in states:
            dn_state = "session"
        elif "L1" in states:
            dn_state = "L1"
        elif "port" in states:
            dn_state = "port"
        else:
            dn_state = "none"
        web_class, web_state = web.get((directory, stem), ("", "none"))
        rows.append((area_of(directory, stem, web_class), directory, stem, unit["lines"],
                     dn_state, web_class, web_state))
    return rows


def web_bucket(web_class, web_state):
    """webpj 한 단위를 집계 칸(port·partial·todo·exclude·outside) 중 하나로 나눈다."""
    if web_state == "none":
        return "outside"
    if web_class.startswith("exclude"):
        return "exclude"
    if web_state in ("port", "session", "oracle"):
        return "port"
    if web_state == "partial":
        return "partial"
    return "todo"


def print_summary(rows):
    """영역별 요약 표를 마크다운으로 출력한다(단위 수 / 줄 수)."""
    totals = collections.OrderedDict((key, collections.Counter()) for key in AREAS)
    # 단위마다 해당 영역의 칸에 단위 수와 줄 수를 더한다.
    for area, _, _, lines, dn_state, web_class, web_state in rows:
        counter = totals[area]
        counter["u"] += 1
        counter["l"] += lines
        mapped = "dn_yes" if dn_state != "none" else "dn_no"
        counter[mapped + "_u"] += 1
        counter[mapped + "_l"] += lines
        bucket = "wb_" + web_bucket(web_class, web_state)
        counter[bucket + "_u"] += 1
        counter[bucket + "_l"] += lines
    columns = ["dn_yes", "dn_no", "wb_port", "wb_partial", "wb_todo", "wb_exclude", "wb_outside"]
    print("| 영역 | cpppj 단위 / 줄 | dn 대응 있음 | dn 대응 없음 | wb port | wb partial | wb todo | wb 제외 | wb 대응표 밖 |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    grand = collections.Counter()
    # 영역 순서대로 한 행씩 출력하고 합계를 모은다.
    for key, title in AREAS.items():
        counter = totals[key]
        grand.update(counter)
        cells = ["%d / %s" % (counter[c + "_u"], format(counter[c + "_l"], ",")) for c in columns]
        print("| %s %s | %d / %s | %s |" % (key, title, counter["u"], format(counter["l"], ","), " | ".join(cells)))
    cells = ["%d / %s" % (grand[c + "_u"], format(grand[c + "_l"], ",")) for c in columns]
    print("| **합계** | %d / %s | %s |" % (grand["u"], format(grand["l"], ","), " | ".join(cells)))


def print_families(rows, project):
    """남은 단위를 영역·이름 계열별로 출력한다. project 는 dn(대응 없음) 또는 wb(todo·partial)."""
    groups = collections.OrderedDict()
    # 남은 단위만 골라 (영역, 디렉터리, 계열)별로 모은다.
    for area, directory, stem, lines, dn_state, web_class, web_state in rows:
        if project == "dn":
            if dn_state != "none":
                continue
            label = stem
        else:
            if web_bucket(web_class, web_state) not in ("todo", "partial"):
                continue
            label = stem + ("*" if web_state == "partial" else "")
        group = groups.setdefault((area, directory, family_of(stem)), [0, 0, []])
        group[0] += 1
        group[1] += lines
        group[2].append((lines, label))
    current = None
    # 영역 순서, 그 안에서는 줄 수가 큰 계열부터 출력한다.
    for (area, directory, family), (count, lines, items) in sorted(
            groups.items(), key=lambda item: (list(AREAS).index(item[0][0]), -item[1][1])):
        if area != current:
            current = area
            print("\n### %s %s" % (area, AREAS[area]))
        items.sort(reverse=True)
        names = ", ".join("%s %d" % (name, size) for size, name in items)
        print("- `%s/%s*` %d단위 %s줄: %s" % (directory, family, count, format(lines, ","), names))


def main():
    """명령줄 인자를 읽어 요약 표·계열 목록·원자료 TSV 중 요청한 것을 출력한다."""
    parser = argparse.ArgumentParser(description="cpppj 대비 dotnetpj·webpj 남은 이식량 집계(읽기 전용)")
    parser.add_argument("--families", choices=("dn", "wb"), help="남은 단위를 영역·계열별로 출력")
    parser.add_argument("--tsv", help="단위별 원자료를 이 경로에 TSV 로 저장")
    args = parser.parse_args()
    rows = build_rows()
    if args.tsv:
        with open(args.tsv, "w", encoding="utf-8", newline="\n") as handle:
            # 단위마다 한 줄씩 탭으로 구분해 적는다.
            for row in rows:
                handle.write("\t".join(str(cell) for cell in row) + "\n")
    if args.families:
        print_families(rows, args.families)
    else:
        print_summary(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
