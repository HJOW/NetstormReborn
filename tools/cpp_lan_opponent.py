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

    def __init__(self, exe, game, output, role, lobby_port, game_port):
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


def main():
    """새 검증 디렉터리에 호스트와 상대 봇을 생성해 로비/전투를 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--lobby-port", type=int, default=16800)
    parser.add_argument("--game-port", type=int, default=16799)
    parser.add_argument("--lobby-only", action="store_true")
    args = parser.parse_args()  # 사용자 선택은 실제 포트·자산·출력 경로로만 제한한다.
    if not (0 < args.lobby_port < 65536 and 0 < args.game_port < 65536 and args.lobby_port != args.game_port):
        parser.error("서로 다른1~65535 포트가 필요합니다.")
    args.exe, args.game, args.output = args.exe.resolve(strict=True), args.game.resolve(strict=True), args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    peers = []  # 이 호출이 실행한 두 자식만 종료 대상으로 보관한다.
    result = dict(passed=False, exe=str(args.exe), exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(), peers=[])
    try:
        for role in ("host", "bot"):  # 봇도 실제 두 번째 클라이언트/등록표/프로토콜을 사용한다.
            peer = Peer(args.exe, args.game, args.output, role, args.lobby_port, args.game_port)
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
            steady = []  # 자연 대기 뒤 양쪽 명단·월드와 로그인창 소멸을 독립 비교한다.
            for peer in peers:  # 간단한 자연 대기 뒤 각자 연결/월드 상태를 다시 읽는다.
                peer.call("netstorm_wait_frames", frames=120)
                steady.append(peer.observe("steady"))
            identities = [player_identity(text) for text in steady]  # 원격 수신 뒤의 실제 Player9 기록이다.
            expected = [dict(owner=1, subscriber=0x610001, name="LocalHost", flags=0x43),
                        dict(owner=2, subscriber=0x610002, name="OpponentBot", flags=0x43)]  # 새 격리 프로필의 명시 등록/이름 입력과 원문 Ready 비트다.
            if identities != [expected, expected] or any(fields(text, "raw_mission_loading") for text in steady):
                raise AssertionError("두 참가자/Ready 또는 일반 시작 안내 소멸 불일치")
            if fields(steady[0], "world") != fields(steady[1], "world"):
                raise AssertionError("두 초기 월드의 객체/타일 수 불일치")
            result["identity"] = identities[0]
            result["world"] = fields(steady[0], "world")
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
        if any(peer.exit_code != 0 for peer in peers) or result.get("closeErrors"):
            result["passed"] = False
        (args.output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
        print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
