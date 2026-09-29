"""실제 stdio MCP의 스키마/오류/이미지와 선택적인 원본 실행을 확인한다."""

import argparse
import base64
import json
from pathlib import Path
import queue
import subprocess
import threading
import time


class Client:
    """서버 로그를 stdout 프로토콜과 분리해 검사하는 최소 MCP 클라이언트."""

    def __init__(self, command):
        """보조 서버는 콘솔 창 없이 실행하고 응답을 별도 스레드로 받는다."""
        self.process = subprocess.Popen(
            command + ["mcp"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, encoding="utf-8",
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        self.messages = queue.Queue()
        self.errors = []
        self.number = 0
        threading.Thread(target=self.read_stdout, daemon=True).start()
        threading.Thread(target=self.read_stderr, daemon=True).start()

    def read_stdout(self):
        """JSON 이외 출력도 테스트 실패로 전달한다."""
        # 서버 알림과 응답을 도착 순서대로 보관한다.
        for line in self.process.stdout:
            try:
                self.messages.put(json.loads(line))
            except json.JSONDecodeError as error:
                self.messages.put(error)
        self.messages.put(RuntimeError("MCP stdout이 종료되었습니다."))

    def read_stderr(self):
        """진단 출력을 비워 서버 파이프가 막히지 않게 한다."""
        # 경고가 생기면 종료 시 확인할 수 있도록 모은다.
        for line in self.process.stderr:
            self.errors.append(line.rstrip())

    def send(self, message):
        """JSON-RPC 메시지 하나를 한 줄로 보낸다."""
        self.process.stdin.write(json.dumps({"jsonrpc": "2.0", **message}) + "\n")
        self.process.stdin.flush()

    def request(self, method, params):
        """요청 식별자를 만들고 45초 안에 대응 응답을 기다린다."""
        self.number += 1
        self.send({"id": self.number, "method": method, "params": params})
        deadline = time.monotonic() + 45
        # 중간 서버 알림은 건너뛰며 stdout의 비 JSON 출력은 허용하지 않는다.
        while True:
            response = self.messages.get(timeout=max(0.01, deadline - time.monotonic()))
            if isinstance(response, Exception):
                raise response
            if response.get("id") == self.number:
                assert "error" not in response, response
                return response["result"]

    def call(self, name, arguments, expect_error=False):
        """도구 오류도 MCP 전송 오류와 구분하고 구조화 응답을 확인한다."""
        result = self.request("tools/call", {"name": name, "arguments": arguments})
        assert bool(result.get("isError")) == expect_error, result
        assert isinstance(result.get("structuredContent"), dict), result
        return result

    def close(self):
        """입력 EOF에 서버가 정상 종료하는지 확인하고 실패 시 보조 서버만 정리한다."""
        self.process.stdin.close()
        try:
            assert self.process.wait(timeout=10) == 0, self.errors
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()


def cli(command, tool, arguments):
    """쉘 인용 문제 없이 독립 CLI 프로세스에서 같은 세션에 접근한다."""
    completed = subprocess.run(
        command + ["call", tool, "--json", json.dumps(arguments, ensure_ascii=False)],
        capture_output=True, text=True, encoding="utf-8", timeout=45,
        creationflags=subprocess.CREATE_NO_WINDOW,
    )
    result = json.loads(completed.stdout)
    assert completed.returncode == 0 and not result["isError"], result
    return result


def main():
    """기본은 프로토콜만, --live는 원본 복사본과 실제 데스크톱을 함께 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--live", action="store_true", help="실제 게임 실행·입력·캡처·종료")
    parser.add_argument("--hold-seconds", type=int, default=0, help="추가 관찰용 실행 유지 시간(최대 300초)")
    args = parser.parse_args()
    command = [str(args.exe.resolve()), "--repo", str(args.repo.resolve())]
    client = Client(command)
    session_id = None
    try:
        initialized = client.request("initialize", {
            "protocolVersion": "2025-03-26", "capabilities": {},
            "clientInfo": {"name": "analyze-manager-smoke", "version": "1"},
        })
        client.send({"method": "notifications/initialized"})
        listing = client.request("tools/list", {})["tools"]
        names = {tool["name"] for tool in listing}
        assert names == {"list_sessions", "start_session", "game_status", "capture_state",
                         "game_input", "wait_for_change", "record_observation", "end_session"}
        # 각 도구에 입력 객체 스키마가 있는지 실제 협상 결과로 확인한다.
        for tool in listing:
            assert tool["inputSchema"]["type"] == "object"
        # AI가 보는 실행 도구 설명에도 개발자 확인 조건이 들어 있는지 확인한다.
        start_tool = next(tool for tool in listing if tool["name"] == "start_session")
        assert "개발자" in start_tool["description"] and "확인" in start_tool["description"]
        assert "10.0.0.15" in start_tool["description"] and "vm-debian-codex" in start_tool["description"]
        client.call("list_sessions", {})
        client.call("game_status", {"sessionId": "../invalid"}, expect_error=True)
        print(json.dumps({"protocol": initialized["protocolVersion"], "tools": len(names)}, ensure_ascii=False), flush=True)
        if args.live:
            started = cli(command, "start_session", {"label": "CLI/MCP 실제 창 모드 연동 검증"})
            session_id = started["data"]["sessionId"]
            print(json.dumps({"sessionId": session_id, "started": started["imagePath"]}), flush=True)
            # CLI 종료 뒤에도 같은 상위 작업이 살아 있으면 게임 세션을 이어 쓸 수 있어야 한다.
            time.sleep(3)
            status = client.call("game_status", {"sessionId": session_id})["structuredContent"]
            assert status["running"] and status["window"]["width"] == 1024
            capture = client.call("capture_state", {"sessionId": session_id})
            images = [item for item in capture["content"] if item["type"] == "image"]
            assert len(images) == 1 and images[0]["mimeType"] == "image/png"
            assert base64.b64decode(images[0]["data"], validate=True).startswith(b"\x89PNG\r\n\x1a\n")
            clicked = client.call("game_input", {"sessionId": session_id, "kind": "click",
                                                 "x": 512, "y": 384, "settleMs": 1500, "includeImage": False})
            keyed = client.call("game_input", {"sessionId": session_id, "kind": "key",
                                               "key": "ESCAPE", "settleMs": 1000, "includeImage": False})
            assert all(item["type"] != "image" for item in keyed["content"])
            waited = client.call("wait_for_change", {"sessionId": session_id, "region": "0,0,100,50",
                                                       "timeoutMs": 300, "threshold": 1, "includeImage": False})
            assert "timedOut" in waited["structuredContent"]
            client.call("record_observation", {"sessionId": session_id,
                "note": "CLI 실행 후 MCP 재접속으로 1024×768 창과 PNG 응답을 확인했다. 클릭과 ESCAPE의 실제 의미는 화면 증거와 대조한다.",
                "evidenceHash": capture["structuredContent"]["evidence"]["sha256"]})
            print(json.dumps({"live": "passed", "sessionId": session_id,
                "capture": capture["structuredContent"]["evidence"]["path"],
                "click": clicked["structuredContent"]["after"]["path"],
                "key": keyed["structuredContent"]["after"]["path"],
                "report": status["report"]}), flush=True)
            time.sleep(max(0, min(args.hold_seconds, 300)))
    finally:
        try:
            if session_id:
                client.call("end_session", {"sessionId": session_id, "force": True})
        finally:
            client.close()
    print("MCP smoke: passed (EOF 종료 포함)", flush=True)


if __name__ == "__main__":
    main()
