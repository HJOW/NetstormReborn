#!/usr/bin/env python3
"""cpppj 로컬 MCP 서버(Netstorm.exe --mcp) 스모크 검사.

게임을 `--run <게임 디렉터리> --mcp --window --no-audio`로 띄워 stdio JSON-RPC로 도구를 호출한다.
사용자 데이터는 임시 폴더(NETSTORM_USER_DATA)로 격리해 실제 Saved Games를 건드리지 않는다.
게임 창이 실제로 뜨므로 AGENTS.md의 실행 확인 규칙(예외 호스트 또는 사용자 확인)을 따른다.

사용법: python tools/mcp_cpp_smoke.py <Netstorm.exe> <게임 디렉터리> [--keep]
"""
import base64
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import zlib


class McpClient:
    """한 줄 한 메시지 stdio MCP 클라이언트다."""

    def __init__(self, command, env):
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        self.next_id = 1
        self.stderr_lines = []
        threading.Thread(target=self._drain_stderr, daemon=True).start()

    def _drain_stderr(self):
        for line in self.process.stderr:
            self.stderr_lines.append(line.decode("utf-8", "replace").rstrip())

    def request(self, method, params=None):
        message = {"jsonrpc": "2.0", "id": self.next_id, "method": method}
        if params is not None:
            message["params"] = params
        self.next_id += 1
        self.process.stdin.write((json.dumps(message) + "\n").encode("utf-8"))
        self.process.stdin.flush()
        line = self.process.stdout.readline()
        if not line:
            raise RuntimeError("서버가 응답 없이 종료됨: " + "\n".join(self.stderr_lines[-10:]))
        reply = json.loads(line)
        assert reply.get("id") == message["id"], reply
        return reply

    def notify(self, method, params=None):
        message = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            message["params"] = params
        self.process.stdin.write((json.dumps(message) + "\n").encode("utf-8"))
        self.process.stdin.flush()

    def call(self, tool, **arguments):
        reply = self.request("tools/call", {"name": tool, "arguments": arguments})
        assert "result" in reply, reply
        return reply["result"]


def decode_png_size(data):
    """PNG 머리와 모든 청크 CRC를 검사하고 (폭, 높이, 색 종류)를 돌려준다."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n", "PNG 서명 오류"
    at, size, idat = 8, None, b""
    while at < len(data):
        length, kind = struct.unpack(">I4s", data[at:at + 8])
        body = data[at + 8:at + 8 + length]
        crc = struct.unpack(">I", data[at + 8 + length:at + 12 + length])[0]
        assert crc == zlib.crc32(kind + body) & 0xFFFFFFFF, "CRC 불일치 " + kind.decode()
        if kind == b"IHDR":
            size = struct.unpack(">IIBBBBB", body)
        if kind == b"IDAT":
            idat += body
        at += 12 + length
    raw = zlib.decompress(idat)  # 표준 zlib로 해제되는지 확인한다.
    width, height, _, color = size[0], size[1], size[2], size[3]
    bytes_per_pixel = {3: 1, 6: 4}[color]
    assert len(raw) == (width * bytes_per_pixel + 1) * height, "해제된 크기 불일치"
    return width, height, color


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    exe, game = sys.argv[1], sys.argv[2]
    keep = "--keep" in sys.argv
    user_data = tempfile.mkdtemp(prefix="netstorm-mcp-")
    env = dict(os.environ, NETSTORM_USER_DATA=user_data)
    client = McpClient([exe, "--run", game, "--mcp", "--window", "--no-audio"], env)
    try:
        reply = client.request("initialize", {"protocolVersion": "2024-11-05", "capabilities": {}, "clientInfo": {"name": "smoke", "version": "1"}})
        assert reply["result"]["serverInfo"]["name"] == "netstorm-cpp", reply
        client.notify("notifications/initialized")
        tools = {tool["name"] for tool in client.request("tools/list")["result"]["tools"]}
        assert {"netstorm_status", "netstorm_report", "netstorm_screenshot", "netstorm_input", "netstorm_wait_frames", "netstorm_quit"} <= tools, tools
        assert client.request("ping")["result"] == {}
        assert "error" in client.request("tools/call", {"name": "no_such_tool", "arguments": {}})

        status = json.loads(client.call("netstorm_status")["content"][0]["text"])
        print("status:", status)
        client.call("netstorm_wait_frames", frames=30)
        result = client.call("netstorm_screenshot")
        image = [item for item in result["content"] if item["type"] == "image"][0]
        width, height, color = decode_png_size(base64.b64decode(image["data"]))
        print("screenshot: %dx%d color=%d bytes=%d" % (width, height, color, len(image["data"]) * 3 // 4))
        assert (width, height) == (status["screen"]["width"], status["screen"]["height"])

        report = client.call("netstorm_report")["content"][0]["text"]
        assert "main_menu_parent" in report or "phase" in report, report[:200]

        # 원문 명령으로 Edit 목록 대화창을 연다(AskEditFort,0).
        client.call("netstorm_input", actions=[{"op": "script-command", "arg": "Tell,Blank"}, {"op": "script-command", "arg": "AskEditFort,0"}], wait_frames=20, screenshot=False)
        status = json.loads(client.call("netstorm_status")["content"][0]["text"])
        print("after AskEditFort:", status)
        assert status.get("page") == "LoadEditable", status

        # 존재하지 않는 컨트롤 클릭은 게임을 죽이지 않고 오류 응답이어야 한다.
        bad = client.call("netstorm_input", actions=[{"op": "click", "arg": "no-such-control"}], wait_frames=1, screenshot=False)
        assert bad["isError"], bad
        status = json.loads(client.call("netstorm_status")["content"][0]["text"])
        assert status["frame"] > 0
        bad = client.call("netstorm_input", actions=[{"op": "test-state-world", "arg": "0"}])
        assert bad["isError"], bad  # 검사 전용 동작은 노출하지 않는다.

        client.call("netstorm_quit")
        code = client.process.wait(timeout=60)
        print("exit code:", code)
        assert code == 0, code
        print("OK")
        return 0
    except Exception:
        print("\n".join(client.stderr_lines[-20:]))
        client.process.kill()
        raise
    finally:
        if not keep:
            shutil.rmtree(user_data, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
