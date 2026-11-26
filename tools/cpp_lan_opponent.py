#!/usr/bin/env python3
"""두 실제 cpppj 클라이언트를 MCP로 조작하는 로컬 TCP/IP 상대 봇이다.

예: python tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe
    --game extracted/cpp-reset-all-1325/game --output extracted/lan-run
실제 창 두 개를 실행한다. AGENTS.md의 실행 허용 범위에서 사용한다.
전용 사용자 루트와 새 Fort를 쓰며 원본/설치 자산과 실제 사용자 저장소를 수정하지 않는다.
게임 프로토콜을 모방하거나 ACK/월드/참가자 상태를 주입하지 않는다.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import queue
import subprocess
import threading
import time


class Peer:
    """실제 Netstorm.exe 한 개와 제한 시간이 있는 MCP 입출력을 소유한다."""

    def __init__(self, exe, game, output, role, lobby_port, game_port, trace=False):
        """새 사용자 디렉터리로 시작하며 생성된 Fort/설정/로그를 결과 옆에 보존한다."""
        self.output = output / role  # 이 참가자의 관측·사용자 파일만 저장한다.
        self.output.mkdir()
        self.role = role  # 사람이 읽을 역할이며 게임 Player 번호를 대신하지 않는다.
        self.next_id = 0  # 현재 stdio 연결의 단조 증가 요청 번호다.
        self.replies = queue.Queue()  # 읽기 스레드가 받은 실제 응답 또는 EOF다.
        self.transcript = []  # 큰 이미지 본문을 제외한 도구/응답 요약이다.
        self.exit_code = None  # 최종 실제 프로세스 종료 코드다.
        settings = (f"welcomeGiven=1;tellTips=0;autoDemo=0;intro=1;edgeScroll=0;"
                    f"SCREENW=1024;SCREENH=768;currentLanguage=english;"
                    f"registeredSubId={0x610001 if role == 'host' else 0x610002};"
                    f"lastMultiplayerFort=LanBot{role};lanServerAddress=127.0.0.1;"
                    f"lanLobbyPort={lobby_port};gameServerPort={game_port}")  # 표준 옵션으로 새 Fort·서로 다른 오프라인 등록ID·로컬 주소를 선택한다.
        if trace:
            settings += ";tracelevel=29"  # 원문1d 결과 로그를 표준 필터 옵션으로 선택한다.
        command = [str(exe), "--run", str(game), "--mcp", "--window", "--no-audio", "--set", settings]  # 고정 시각/자동 전투 옵션을 사용하지 않는다.
        self.stderr = (self.output / "stderr.log").open("wb")  # 게임 진단 stdout은 MCP가 stderr로 분리한다.
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=self.stderr, env=dict(os.environ, NETSTORM_USER_DATA=str(self.output / "user")),
                                        creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
        self.reader = threading.Thread(target=self._read, daemon=True)  # stdout의 큰 PNG 응답도 계속 비운다.
        self.reader.start()

    def _read(self):
        """한 줄 JSON-RPC를 대기열로 전달하며 종료도 즉시 알린다."""
        try:
            for line in self.process.stdout:  # 응답 하나마다 실제 JSON 경계를 유지한다.
                self.replies.put(json.loads(line))
        except Exception as error:
            self.replies.put(error)
        finally:
            self.replies.put(None)

    def request(self, method, params=None, timeout=30):
        """실제 응답의 ID/오류를 검사하고 정지한 자식을 무한 대기하지 않는다."""
        self.next_id += 1
        message = dict(jsonrpc="2.0", id=self.next_id, method=method)  # 현재 요청의 독립 봉투다.
        if params is not None:
            message["params"] = params
        self.process.stdin.write((json.dumps(message) + "\n").encode("utf8"))
        self.process.stdin.flush()
        reply = self.replies.get(timeout=timeout)  # OS 시계에 따른 제한이며 게임 시계를 변경하지 않는다.
        if isinstance(reply, Exception):
            raise reply
        if reply is None:
            raise RuntimeError(f"{self.role}: MCP EOF, process={self.process.poll()}")
        if reply.get("id") != self.next_id or "error" in reply:
            raise RuntimeError(reply)
        result = reply["result"]  # 프로토콜 성공과 도구의 isError를 별도로 검사한다.
        compact = dict(result)  # 이미지 바이트는 별도 PNG로만 보존한다.
        if "content" in result:
            compact["content"] = [{k: v for k, v in item.items() if k != "data"} for item in result["content"]]
        self.transcript.append(dict(request=message, result=compact, seconds=time.time()))
        if result.get("isError"):
            raise RuntimeError(result)
        return result

    def call(self, name, **arguments):
        """노출된 MCP 도구에만 요청을 전달한다."""
        return self.request("tools/call", dict(name=name, arguments=arguments))

    def action(self, operation, argument, frames=5):
        """같은 게임 입력 큐로 한 동작을 전달하고 프레임 완료를 기다린다."""
        return self.call("netstorm_input", actions=[dict(op=operation, arg=argument)], wait_frames=frames, screenshot=False)

    def report(self):
        """현재 실제 월드/로그인/화면의 읽기 전용 보고를 반환한다."""
        return self.call("netstorm_report")["content"][0]["text"]

    def observe(self, name):
        """같은 참가자의 보고와 실제 PNG를 파일로 보존한다."""
        text = self.report()  # 현재 원문 상태를 관측하며 성공값을 생성하지 않는다.
        (self.output / (name + ".tsv")).write_text(text, encoding="utf8")
        content = self.call("netstorm_screenshot")["content"]  # 화면은 런타임이 생성한 PNG를 그대로 저장한다.
        picture = next(item["data"] for item in content if item["type"] == "image")
        (self.output / (name + ".png")).write_bytes(base64.b64decode(picture))
        return text

    def wait(self, predicate, description, seconds=30):
        """실제 보고가 조건에 도달할 때까지 기다리며 실패 자료를 남긴다."""
        deadline = time.monotonic() + seconds  # 각 작업의 실제 제한 시간이다.
        while time.monotonic() < deadline:  # 두 게임 프로세스는 기다리는 동안에도 자연 진행한다.
            text = self.report()
            if predicate(text):
                return text
            self.call("netstorm_wait_frames", frames=6)
        self.observe("timeout")
        raise TimeoutError(f"{self.role}: {description}")

    def prepare(self):
        """일반 메뉴와 새 Fort/이름 입력을 실제 UI로 완료한다."""
        self.request("initialize", dict(protocolVersion="2025-06-18", capabilities={}, clientInfo=dict(name="lan-opponent", version="1")))
        self.wait(lambda text: "phase\t0\n" in text, "메인 메뉴")
        self.action("click", "Multiplayer", 30)
        self.observe("name-prompt")
        self.action("text", "LocalHost" if self.role == "host" else "OpponentBot")
        self.action("click", "screen:480,420", 15)  # 1024×768 기본 이름 입력창의 실제 OK 버튼이다.
        self.observe("fort")
        self.action("click", "screen:40,746")  # 전용 옵션 돌 버튼은 라벨 조회 대상이 아니므로 실제 하단 Connect 좌표를 누른다.
        self.action("click", "Host Local Battle" if self.role == "host" else "Join Local Battle", 12)

    def close(self):
        """정상 종료를 우선하고 제한 시간 뒤 자기 자식만 정리한다."""
        try:
            if self.process.poll() is None:
                try:
                    self.call("netstorm_quit")
                except (OSError, RuntimeError):
                    self.process.wait(timeout=2)  # EOF와 프로세스 종료 통지 사이의 짧은 경합은 실제 종료 코드로 수집한다.
            self.exit_code = self.process.wait(timeout=15)
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.exit_code = self.process.wait(timeout=15)
            if self.exit_code is None:
                self.exit_code = self.process.poll()
            try:
                self.process.stdin.close()  # 오류 종료한 게임의 깨진 파이프도 Python 종료 전에 명시 반납한다.
            except OSError:
                pass  # 이미 결과에 보존한 실제 자식 종료 코드를 바꾸지 않는다.
            self.reader.join(timeout=1)
            self.process.stdout.close()
            self.stderr.close()
            (self.output / "mcp.json").write_text(json.dumps(self.transcript, ensure_ascii=False, indent=2), encoding="utf8")


def fields(report, key):
    """탭 보고의 지정 행을 원문 열 순서대로 읽는다."""
    return [line.split("\t")[1:] for line in report.splitlines() if line.startswith(key + "\t")]


def battle_ready(report):
    """실제 일반 로그인14와 로컬 PlayerKnown 비영을 함께 확인한다."""
    inputs, sessions = fields(report, "raw_input"), fields(report, "raw_session")  # 화면 단계와 구별한 실제 Session 관측이다.
    return (any(len(row) > 2 and row[2] == "14" for row in inputs)
            and any(len(row) > 1 and int(row[0]) in range(1, 9) and int(row[1]) != 0 for row in sessions))


def player_identity(report):
    """실제180BYTE Player 기록에서 두 참가자의 ID·이름·참여/로그인/Ready 비트를 읽는다."""
    result = []  # 카메라·로컬 표시 비트와 구별한 두 공통 참가자 기록이다.
    for row in fields(report, "raw_player_record"):  # 일반 LAN의 실제 두 슬롯만 관측한다.
        if int(row[0]) not in (1, 2):
            continue
        record = bytes.fromhex(row[1])  # 보고서의 원문 바이트를 그대로 읽는다.
        result.append(dict(owner=int(row[0]), subscriber=int.from_bytes(record[44:48], "little"),
                           name=record[12:44].split(b"\0")[0].decode("ascii"), flags=int.from_bytes(record[80:84], "little") & 0x43))
    return result


def owned_objects(report, asset, owner=1):
    """실제 객체 보고에서 지정 자산/소유자의 현재 SID·좌표·HP만 선택한다."""
    return [dict(sid=int(row[0]), asset=row[1], owner=int(row[2]),
                 x=float(row[6]), y=float(row[7]), hp=int(row[13]))
            for row in fields(report, "object") if row[1].lower() == asset.lower() and (owner is None or int(row[2]) == owner)]


def select_legal_site(peer, expected_type, name):
    """현재 커서에 실제 마우스 이동을 보내고 원본 배치 허용을 통과한 화면 위치를 고른다."""
    candidates = [(600, 420), (560, 420), (560, 450), (600, 450), (450, 420), (500, 450),
                  (620, 380), (580, 360), (480, 380), (640, 460), (520, 440),
                  (720, 400), (700, 500), (500, 540), (640, 540), (440, 500), (580, 560)]  # 고정1024×768 화면의 자기 섬 주변 후보이며 성공값을 만들지 않는다.
    for x, y in candidates:  # 각 후보의 실제 Query/표시/소스 허용을 모두 읽은 뒤 선택한다.
        peer.action("pointer", f"0,{x},{y}", 8)
        rows = fields(peer.report(), "raw_cursor")
        if rows and rows[0][1] == "1" and rows[0][2] == str(expected_type) and rows[0][8:11] == ["1", "1", "1"]:
            peer.observe(name)
            return x, y
    peer.observe(name + "-rejected")
    raise AssertionError(f"TYPE{expected_type} 실제 허용 배치 위치를 찾지 못했습니다")


def produce_golem(peers):
    """실제 신전 건설과 생산 아이콘 입력으로 골렘 한 개를 배치하고 두 TCP 수신 결과를 대조한다."""
    host, bot = peers  # 호스트만 조작하며 상대는 실제 소켓으로 생산/배치 결과를 받는다.
    initial = host.observe("production-initial")
    if owned_objects(initial, "sunWalker"):
        raise AssertionError("새 Fort에 이미 골렘이 있어 신규 생산을 구별할 수 없습니다")
    host.action("right", "world:priest", 10)
    host.action("click", "Construct", 10)
    host.action("click", "Temple", 10)
    menu = host.observe("production-temple-menu")
    choices = [row for row in fields(menu, "raw_menu_item") if row[5] == "1" and "Wind" in row[6]]  # 현재 실제 보이는 신전 하위 메뉴에서만 항목을 고른다.
    if len(choices) != 1:
        raise AssertionError("실제 Wind 신전 메뉴 항목을 하나로 구별하지 못했습니다")
    choice = choices[0]  # 명령/인자는 보고된 실제 항목의 클릭 위치 조회에만 사용한다.
    host.action("click", f"menu-command:{int(choice[3]):x},{int(choice[4]):x}", 10)
    temple_site = select_legal_site(host, 123, "production-temple-preview")
    host.action("click", f"screen:{temple_site[0]},{temple_site[1]}", 120)
    for peer in peers:  # 완성된 실제 신전과 전송 수신 뒤에만 생산 아이콘을 사용한다.
        peer.wait(lambda text: any(row["hp"] == 5000 for row in owned_objects(text, "windVortex")), "실제 Wind 신전 완성", seconds=60)
    host.wait(lambda text: any(row[2] == "133" for row in fields(text, "raw_production_item")), "신전의 골렘 생산 아이콘", seconds=30)
    before = [peer.observe("production-before-unit") for peer in peers]  # 비용과 신규 SID 대조의 실제 시작 상태다.
    before_sp = [list(map(int, fields(text, "raw_sp")[0][1:])) for text in before]
    if before_sp[0] != before_sp[1] or before_sp[0][0] != 1500:
        raise AssertionError("신전 비용 차감 뒤 두 실제 SP가1500으로 일치하지 않습니다")
    host.action("click", "production:133", 10)  # 실제 아이콘의 불투명 픽셀을 클릭하며 생산/재고 경로를 우회하지 않는다.
    unit_site = select_legal_site(host, 133, "production-unit-preview")
    host.action("click", f"screen:{unit_site[0]},{unit_site[1]}", 120)
    for peer in peers:  # 새 객체가 양쪽 월드에 실제 등록될 때까지 자연 프레임을 기다린다.
        peer.wait(lambda text: bool(owned_objects(text, "sunWalker")), "실제 신규 골렘", seconds=30)
    after = [peer.observe("production-unit-settled") for peer in peers]
    units = [owned_objects(text, "sunWalker") for text in after]
    after_sp = [list(map(int, fields(text, "raw_sp")[0][1:])) for text in after]
    if len(units[0]) != 1 or units[0] != units[1] or units[0][0]["hp"] != 50:
        raise AssertionError("신규 골렘의 양쪽 SID/자산/owner/좌표/HP 불일치")
    expected_sp = before_sp[0].copy()  # 실제 비용 기본값400은 원본 TYPE 후처리에서 적용된다.
    expected_sp[0] -= 400
    if after_sp != [expected_sp, expected_sp]:
        raise AssertionError("골렘 비용400의 실제 SP 차감 또는 두 수신 결과 불일치")
    saved = host.output / "user/Saves/!state.dat"
    image = saved.read_bytes()  # 커서 배치가 실제 작성한 호스트 복구 파일이다.
    if int.from_bytes(image[260:264], "little") != 1 or int.from_bytes(image[268:272], "little") != expected_sp[0]:
        raise AssertionError("실제 복구 저장의 owner/SP 불일치")
    (host.output / "production-after-unit-state.dat").write_bytes(image)
    return dict(templeSite=temple_site, unitSite=unit_site, beforeSp=before_sp[0], afterSp=after_sp[0],
                units=units, saved=dict(bytes=len(image), owner=1, sp=expected_sp[0], sha256=hashlib.sha256(image).hexdigest()))


def first_bridge(report):
    """실제 첫 다리 아이콘의 내용물 SID와 원본 패턴/품질을 대응시킨다."""
    items = [row for row in fields(report, "raw_production_item") if row[2] == "82"]
    if not items:
        return None
    return next((row for row in fields(report, "raw_production_bridge") if row[0] == items[0][1]), None)


def workshop_finished(report):
    """같은 Factory TYPE의 건설 중/완성을 최대HP와 원본 abstract bit 해제로 구별한다."""
    extras = {int(row[0]): int(row[2]) for row in fields(report, "raw_asset_fields")}  # 현재 실제 자산 확장 플래그이며 보고서 값을 수정하지 않는다.
    return any(row["hp"] == 3000 and row["sid"] in extras and not (extras[row["sid"]] & 1)
               for row in owned_objects(report, "sunFactory"))


def build_extra(peers):
    """실제 사제 메뉴로 썬 워크샾을 완성하고 자연 공급된 단일 다리를 정상 클릭으로 배치한다."""
    host, bot = peers  # 상태는 호스트의 실제 명령이 전송한 결과로만 비교한다.
    initial = [peer.observe("extra-before-workshop") for peer in peers]
    before_sp = [list(map(int, fields(text, "raw_sp")[0][1:])) for text in initial]
    if before_sp[0] != before_sp[1] or before_sp[0][0] != 1100:
        raise AssertionError("추가 건설 전 실제 두 SP1100 불일치")
    host.action("right", "world:priest", 10)
    host.action("click", "Construct", 10)
    host.action("click", "Workshop", 10)
    menu = host.observe("extra-workshop-menu")
    choices = [row for row in fields(menu, "raw_menu_item") if row[5] == "1" and "Sun Workshop" in row[6]]
    if len(choices) != 1:
        raise AssertionError("현재 보이는 썬 워크샾 건설 항목 불일치")
    choice = choices[0]  # 읽은 항목의 실제 클릭점을 조회한다. 직접 건설 명령을 호출하지 않는다.
    host.action("click", f"menu-command:{int(choice[3]):x},{int(choice[4]):x}", 10)
    site = select_legal_site(host, 85, "extra-workshop-preview")
    host.action("click", f"screen:{site[0]},{site[1]}", 120)
    for peer in peers:  # 일반 건설/수신 본체가 같은 워크샾을 등록해야 한다.
        peer.wait(workshop_finished, "실제 썬 워크샾 완성/최대HP/abstract 해제", seconds=60)
    built = [peer.observe("extra-workshop-settled") for peer in peers]
    workshops = [owned_objects(text, "sunFactory") for text in built]
    after_sp = [list(map(int, fields(text, "raw_sp")[0][1:])) for text in built]
    expected_sp = before_sp[0].copy()  # 명시 비용800을 원래 SP 차감 결과와 대조한다.
    expected_sp[0] -= 800
    if len(workshops[0]) != 1 or workshops[0] != workshops[1] or workshops[0][0]["hp"] != 3000 or after_sp != [expected_sp, expected_sp]:
        raise AssertionError("썬 워크샾 실제 객체/HP3000/비용800의 두 수신 결과 불일치")
    host.wait(lambda text: (row := first_bridge(text)) is not None and row[1:3] == ["0", "2"], "자연 공급된 단일 다리의 품질2", seconds=30)
    before = host.observe("extra-before-bridge")
    bridge = first_bridge(before)  # 실제 내용물의 패턴0·품질2를 강제로 수정하지 않고 읽는다.
    existing = {int(row[0]) for row in fields(before, "object") if row[1].lower() == "bridge"}
    priest = owned_objects(before, "priest")[0]  # 원래 사제 좌표 주변의 지도 칸을 탐색 목표로 정한다.
    host.action("click", "production:82", 10)
    host.action("click", f"placement-reach:{int(priest['x']) + 12},{int(priest['y'])}", 120)  # 현재 커서의 실제 허용 위치만 탐색하고 생성은 정상 클릭이 수행한다.
    for peer in peers:  # 새 다리의 네트워크 수신도 실제 객체 목록으로 확인한다.
        peer.wait(lambda text: any(int(row[0]) not in existing for row in fields(text, "object") if row[1].lower() == "bridge"), "실제 신규 다리", seconds=30)
    settled = [peer.observe("extra-bridge-settled") for peer in peers]
    bridges = [[row for row in fields(text, "object") if row[1].lower() == "bridge" and int(row[0]) not in existing] for text in settled]
    bridge_sp = [list(map(int, fields(text, "raw_sp")[0][1:])) for text in settled]
    if len(bridges[0]) != 1 or bridges[0] != bridges[1] or bridge_sp != [expected_sp, expected_sp]:
        raise AssertionError("새 단일 다리의 양쪽 전체 객체 행 또는0 SP 비용 불일치")
    image = (host.output / "user/Saves/!state.dat").read_bytes()  # 워크샾 비용 이후의 실제 복구 저장을 확인한다.
    if int.from_bytes(image[260:264], "little") != 1 or int.from_bytes(image[268:272], "little") != expected_sp[0]:
        raise AssertionError("추가 건설 뒤 복구 저장의 실제 owner/SP 불일치")
    (host.output / "extra-after-build-state.dat").write_bytes(image)
    return dict(workshopSite=site, workshops=workshops, beforeSp=before_sp[0], afterSp=expected_sp,
                sourceBridge=bridge, bridges=bridges, saved=dict(bytes=len(image), owner=1, sp=expected_sp[0], sha256=hashlib.sha256(image).hexdigest()))


def main():
    """새 검증 디렉터리에 호스트와 상대 봇을 생성해 로비/전투를 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--lobby-port", type=int, default=16800)
    parser.add_argument("--game-port", type=int, default=16799)
    parser.add_argument("--lobby-only", action="store_true")
    parser.add_argument("--steady-seconds", type=float, default=4, help="두 참가자의 시작 후 실제 OS 시계 대기 초")
    parser.add_argument("--takeover", action="store_true", help="시작 검사 후 이 도구의 호스트만 강제 종료해 상대의 서버 인수를 검사")
    parser.add_argument("--produce-golem", action="store_true", help="실제 Wind 신전과 골렘 생산/배치 뒤 양쪽 객체·SP·복구 저장을 대조")
    parser.add_argument("--extra-build", action="store_true", help="골렘 생산 뒤 실제 썬 워크샾·단일 다리 건설을 추가 검증")
    args = parser.parse_args()  # 사용자 선택은 실제 포트·자산·출력 경로로만 제한한다.
    if not (0 < args.lobby_port < 65536 and 0 < args.game_port < 65536 and args.lobby_port != args.game_port):
        parser.error("서로 다른1~65535 포트가 필요합니다.")
    if not 0 <= args.steady_seconds <= 3600:
        parser.error("실시간 대기는0~3600초 범위여야 합니다.")
    if args.takeover and args.lobby_only:
        parser.error("서버 인수는 실제 전투 시작 검사와 함께 사용해야 합니다.")
    if args.produce_golem and (args.lobby_only or args.takeover):
        parser.error("골렘 생산은 일반 전투 시작 검증에서만 단독 선택합니다.")
    if args.extra_build and not args.produce_golem:
        parser.error("추가 건설은 --produce-golem과 함께 선택해야 합니다.")
    args.exe, args.game, args.output = args.exe.resolve(strict=True), args.game.resolve(strict=True), args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    peers = []  # 이 호출이 실행한 두 자식만 종료 대상으로 보관한다.
    result = dict(passed=False, exe=str(args.exe), exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(), peers=[])
    try:
        for role in ("host", "bot"):  # 봇도 실제 두 번째 클라이언트/등록표/프로토콜을 사용한다.
            peer = Peer(args.exe, args.game, args.output, role, args.lobby_port, args.game_port, trace=args.takeover)
            peers.append(peer)
            peer.prepare()
        for peer in peers:  # 두 이름이 실제 수신된 명단에 함께 나타나야 한다.
            peer.wait(lambda text: "LocalHost" in text and "OpponentBot" in text, "두 참가자 로비 명단")
            peer.observe("roster")
        if not args.lobby_only:
            peers[0].action("click", "Start Battle", 10)
            for peer in peers:  # 실제 phase14/PlayerKnown과 게임 입력 설치를 확인한다.
                peer.wait(battle_ready, "전투 로그인14/PlayerKnown", seconds=45)
                peer.observe("battle")
            started = time.monotonic()  # 고정 게임 시각 대신 실제 OS 경과를 측정한다.
            while time.monotonic() - started < args.steady_seconds:  # 양쪽 프레임을 작은 구간으로 기다려 종료/오류도 즉시 수집한다.
                for peer in peers:  # 상대도 계속 실제 패킷을 수신하며 현재 제품 프레임으로 실행된다.
                    peer.call("netstorm_wait_frames", frames=120)
            result["steadySeconds"] = time.monotonic() - started
            steady = [peer.observe("steady") for peer in peers]  # 자연 대기 뒤 명단·월드와 로그인창 소멸을 독립 비교한다.
            identities = [player_identity(text) for text in steady]  # 원격 수신 뒤의 실제 Player9 기록이다.
            expected = [dict(owner=1, subscriber=0x610001, name="LocalHost", flags=0x43),
                        dict(owner=2, subscriber=0x610002, name="OpponentBot", flags=0x43)]  # 새 격리 프로필의 명시 등록/이름 입력과 원문 Ready 비트다.
            if identities != [expected, expected] or any(fields(text, "raw_mission_loading") for text in steady):
                raise AssertionError("두 참가자/Ready 또는 일반 시작 안내 소멸 불일치")
            if fields(steady[0], "world") != fields(steady[1], "world"):
                raise AssertionError("두 초기 월드의 객체/타일 수 불일치")
            result["identity"] = identities[0]
            result["world"] = fields(steady[0], "world")
            if args.produce_golem:
                result["production"] = produce_golem(peers)
            if args.extra_build:
                result["extraBuild"] = build_extra(peers)
            if args.takeover:
                host, bot = peers  # 이 호출이 직접 만든 실제 두 자식만 대상으로 삼는다.
                if fields(steady[1], "raw_network_role") != [["0", "0"]]:
                    raise AssertionError("서버 인수 전 상대의 실제 client 권한 불일치")
                host.process.kill()  # 호스트 장애를 실제 소켓 단절로 재현하고 게임 상태를 변경하지 않는다.
                result["hostTermination"] = dict(requested=True, exitCode=host.process.wait(timeout=15))
                bot.wait(lambda text: battle_ready(text) and fields(text, "raw_network_role") == [["1", "1"]],
                         "실제 서버 인수와 재접속14", seconds=45)
                bot.call("netstorm_wait_frames", frames=120)
                taken = bot.observe("takeover")  # 실제 재접속 수신과 권한 전환 뒤의 상태/화면이다.
                if not battle_ready(taken) or fields(taken, "raw_network_role") != [["1", "1"]]:
                    raise AssertionError("인수 후 실제 권한/로그인 불일치")
                result["takeover"] = dict(role=fields(taken, "raw_network_role"), session=fields(taken, "raw_session"), world=fields(taken, "world"))
        result["passed"] = True
    except Exception as error:
        result["error"] = repr(error)
        print(result["error"], flush=True)
        for peer in peers:  # 대기실 거부 등 양쪽 상태가 다른 실패도 각 실제 화면/보고를 보존한다.
            if peer.process.poll() is None:
                try:
                    peer.observe("failure")
                except Exception as observation_error:
                    result.setdefault("observationErrors", []).append(repr(observation_error))
    finally:
        for peer in reversed(peers):  # 상대 봇부터 종료하며 자기 프로세스 밖의 서버/게임을 건드리지 않는다.
            try:
                peer.close()
            except Exception as error:
                result.setdefault("closeErrors", []).append(repr(error))
            result["peers"].append(dict(role=peer.role, exitCode=peer.exit_code))
        if any(peer.exit_code != 0 and not (peer.role == "host" and result.get("hostTermination", {}).get("requested")) for peer in peers) or result.get("closeErrors"):
            result["passed"] = False
        if result.get("takeover"):
            log = (peers[1].output / "stderr.log").read_text(encoding="utf8", errors="replace")  # 실제 정상 종료의 CRT flush 뒤 결과 로그를 검증한다.
            result["takeover"]["acceptedLog"] = "Reconnection accepted" in log
            if not result["takeover"]["acceptedLog"]:
                result["passed"] = False
                result["error"] = "정상 종료 후 실제 재접속 수락 로그 없음"
        (args.output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
        print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
