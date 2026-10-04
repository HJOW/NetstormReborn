# -*- coding: utf-8 -*-
"""
원본 게임 자동 분석용 MCP 드라이버 (Windows 전용)

analyzeManager 의 stdio MCP 서버를 자식 프로세스로 띄워 두고, 명령 파일(JSON)을 받아 도구를 호출한다.
CLI 를 호출할 때마다 드는 2~4초의 시작 비용이 없어서 "건설 명령 직후 사제 이동" 같은 짧은 간격의 조작을 시험할 수 있다.
게임 창 화면 캡처에는 마우스 커서가 찍히지 않으므로, 별도 스레드가 GetCursorInfo 로 커서 모양(핸들)이
바뀔 때마다 시각·좌표·그림을 기록한다 (cursor_timeline.jsonl, cursors/*.png).

사용법 (원본 게임을 실제로 구동하므로 AGENTS.md 의 실행 허용 조건을 만족하는 시스템에서만 쓴다):
    python tools/ns_driver.py serve --work <작업 폴더>
        작업 폴더의 cmd/*.json 을 파일 이름 순서로 실행하고 out/<이름>.json 에 결과를 쓴다.
        명령 파일 형식: {"steps": [{"op": "click", "x": 100, "y": 200}, ...]}
        cmd/QUIT 파일(내용 무관)이 생기면 정리하고 끝낸다.
"""
import argparse
import ctypes
import json
import os
import queue
import shutil
import subprocess
import sys
import threading
import time
from ctypes import wintypes
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageGrab

# Windows 에서 보조 프로세스의 콘솔 창을 숨기는 플래그
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
# 저장소 루트 (이 파일은 tools/ 아래에 있다)
REPO = Path(__file__).resolve().parents[1]
# 분석기 Release 실행 파일
ANALYZER_EXE = REPO / "analyzeManager" / "bin" / "Release" / "net10.0-windows" / "Netstorm.AnalyzeManager.exe"
# 세션 증거가 저장되는 폴더
SESSIONS_DIR = REPO / "extracted" / "analyzeManager"
# 커서 감시 간격(초). 약 100Hz 로 모양 변화를 놓치지 않게 한다
CURSOR_POLL_SECONDS = 0.01
# 디스크 여유가 이 값(바이트)보다 작아지면 명령 실행을 거부한다 (사용자 허용량 19GB 보호용 여유분)
MIN_FREE_BYTES = 2 * 1024 ** 3

# DPI 가상화 없이 실제 물리 픽셀 좌표를 얻는다 (분석기와 같은 좌표계)
try:
    ctypes.windll.shcore.SetProcessDpiAwareness(2)
except Exception:  # 이미 설정되었거나 지원하지 않는 환경
    pass

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32


class CURSORINFO(ctypes.Structure):
    """GetCursorInfo 결과 구조체"""
    _fields_ = [("cbSize", wintypes.DWORD), ("flags", wintypes.DWORD),
                ("hCursor", wintypes.HANDLE), ("ptScreenPos", wintypes.POINT)]


class ICONINFO(ctypes.Structure):
    """GetIconInfo 결과 구조체 (커서 핫스팟 확인용)"""
    _fields_ = [("fIcon", wintypes.BOOL), ("xHotspot", wintypes.DWORD), ("yHotspot", wintypes.DWORD),
                ("hbmMask", wintypes.HBITMAP), ("hbmColor", wintypes.HBITMAP)]


class BITMAPINFOHEADER(ctypes.Structure):
    """GetDIBits 에 넘기는 비트맵 머리"""
    _fields_ = [("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG), ("biHeight", wintypes.LONG),
                ("biPlanes", wintypes.WORD), ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
                ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG),
                ("biYPelsPerMeter", wintypes.LONG), ("biClrUsed", wintypes.DWORD), ("biClrImportant", wintypes.DWORD)]


# 함수 시그니처를 명시해 64비트 핸들이 잘리지 않게 한다
user32.GetCursorInfo.argtypes = [ctypes.POINTER(CURSORINFO)]
user32.GetIconInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(ICONINFO)]
user32.DrawIconEx.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.HANDLE, ctypes.c_int,
                              ctypes.c_int, wintypes.UINT, wintypes.HBRUSH, wintypes.UINT]
user32.GetDC.restype = wintypes.HDC
user32.GetDC.argtypes = [wintypes.HWND]
user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleBitmap.restype = wintypes.HBITMAP
gdi32.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
gdi32.SelectObject.restype = wintypes.HGDIOBJ
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT, ctypes.c_void_p,
                            ctypes.c_void_p, wintypes.UINT]
gdi32.CreateSolidBrush.restype = wintypes.HBRUSH
gdi32.CreateSolidBrush.argtypes = [wintypes.COLORREF]
user32.FillRect.argtypes = [wintypes.HDC, ctypes.POINTER(wintypes.RECT), wintypes.HBRUSH]

# 커서를 그릴 정사각 캔버스 한 변(픽셀). 원본 커서는 32x32 이하이다
CANVAS = 64
# DrawIconEx 의 DI_NORMAL 플래그 (마스크와 이미지를 모두 그린다)
DI_NORMAL = 0x3


def now_iso() -> str:
    """UTC 시각을 밀리초까지 ISO 문자열로 돌려준다 (입력 로그·영상 프레임 시각과 대조용)"""
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"


def render_cursor(hcursor, background: int) -> Image.Image:
    """커서 하나를 단색 배경 위에 그려 PIL 이미지로 돌려준다 (background = 0xBBGGRR)"""
    screen = user32.GetDC(None)
    dc = gdi32.CreateCompatibleDC(screen)
    bitmap = gdi32.CreateCompatibleBitmap(screen, CANVAS, CANVAS)
    old = gdi32.SelectObject(dc, bitmap)
    # 배경을 먼저 칠해 투명 부분이 어떤 색인지 구분되게 한다
    brush = gdi32.CreateSolidBrush(background)
    rect = wintypes.RECT(0, 0, CANVAS, CANVAS)
    user32.FillRect(dc, ctypes.byref(rect), brush)
    gdi32.DeleteObject(brush)
    user32.DrawIconEx(dc, 0, 0, hcursor, 0, 0, 0, None, DI_NORMAL)
    header = BITMAPINFOHEADER(ctypes.sizeof(BITMAPINFOHEADER), CANVAS, -CANVAS, 1, 32, 0, 0, 0, 0, 0, 0)
    buffer = ctypes.create_string_buffer(CANVAS * CANVAS * 4)
    gdi32.GetDIBits(dc, bitmap, 0, CANVAS, buffer, ctypes.byref(header), 0)
    gdi32.SelectObject(dc, old)
    gdi32.DeleteObject(bitmap)
    gdi32.DeleteDC(dc)
    user32.ReleaseDC(None, screen)
    # BGRA 바이트 배열을 RGB 이미지로 바꾼다
    return Image.frombuffer("RGBA", (CANVAS, CANVAS), buffer.raw, "raw", "BGRA", 0, 1).convert("RGB")


def cursor_sheet(hcursor) -> tuple:
    """커서를 검은 배경·흰 배경에서 각각 그려 나란히 붙인 그림과 핫스팟 정보를 돌려준다"""
    black = render_cursor(hcursor, 0x000000)
    white = render_cursor(hcursor, 0xFFFFFF)
    # 두 배경에서 같은 픽셀 = 확실히 그려진 픽셀, 다른 픽셀 = 투명 또는 반전 픽셀
    sheet = Image.new("RGB", (CANVAS * 2, CANVAS))
    sheet.paste(black, (0, 0))
    sheet.paste(white, (CANVAS, 0))
    info = ICONINFO()
    hotspot = None
    if user32.GetIconInfo(hcursor, ctypes.byref(info)):
        hotspot = [info.xHotspot, info.yHotspot]
        # GetIconInfo 가 만든 비트맵은 호출자가 지워야 한다
        if info.hbmMask:
            gdi32.DeleteObject(info.hbmMask)
        if info.hbmColor:
            gdi32.DeleteObject(info.hbmColor)
    return sheet, hotspot


def cursor_rgba(hcursor) -> tuple:
    """커서를 검은·흰 배경에서 각각 그려 알파가 있는 그림과 핫스팟으로 합친다 (두 배경에서 같은 픽셀만 불투명)"""
    black = render_cursor(hcursor, 0x000000)
    white = render_cursor(hcursor, 0xFFFFFF)
    rgba = Image.new("RGBA", black.size)
    pb, pw, po = black.load(), white.load(), rgba.load()
    # 모든 픽셀을 훑으며 배경과 무관하게 같은 색인 곳만 커서 픽셀로 본다
    for y in range(CANVAS):
        for x in range(CANVAS):
            if pb[x, y] == pw[x, y]:
                po[x, y] = pb[x, y] + (255,)
            elif pb[x, y] != (0, 0, 0) and pw[x, y] != (255, 255, 255):
                # 반전 마스크 등으로 배경에 따라 바뀌는 픽셀은 회색 반투명으로 표시한다
                po[x, y] = (255, 0, 255, 160)
    info = ICONINFO()
    hotspot = (0, 0)
    if user32.GetIconInfo(hcursor, ctypes.byref(info)):
        hotspot = (info.xHotspot, info.yHotspot)
        # GetIconInfo 가 만든 비트맵은 호출자가 지워야 한다
        if info.hbmMask:
            gdi32.DeleteObject(info.hbmMask)
        if info.hbmColor:
            gdi32.DeleteObject(info.hbmColor)
    return rgba, hotspot


def composite_cursor(image_path: str, out_path: str, state: dict, origin=(8, 31)) -> bool:
    """캡처 PNG 위에 현재 시스템 커서를 합성해 저장한다 (캡처는 커서를 담지 않으므로 확인용). origin = 게임 클라이언트의 화면 좌표"""
    if not state.get("handle") or not state.get("showing"):
        return False
    base = Image.open(image_path).convert("RGBA")
    rgba, hotspot = cursor_rgba(state["handle"])
    # 화면 좌표를 클라이언트 좌표로 바꾸고 핫스팟만큼 왼쪽 위로 옮겨 붙인다
    px, py = state["x"] - origin[0] - hotspot[0], state["y"] - origin[1] - hotspot[1]
    base.alpha_composite(rgba, dest=(max(px, 0), max(py, 0)), source=(max(-px, 0), max(-py, 0)))
    base.convert("RGB").save(out_path)
    return True



class ClientScreen:
    """게임 클라이언트 영역(1024x768)을 화면 좌표와 이어 주는 보조 객체.
    분석기의 game_input 은 입력 전후 화면을 저장하느라 한 번에 0.5초 안팎 걸리므로, 시간이 많이 드는 훑어보기(미리보기 판정)에는
    SetCursorPos 와 직접 화면 복사를 쓴다. 상태를 바꾸는 클릭·키 입력은 항상 분석기를 거친다."""

    def __init__(self, origin_x: int, origin_y: int, width: int = 1024, height: int = 768):
        self.origin_x, self.origin_y, self.width, self.height = origin_x, origin_y, width, height

    def move(self, x: int, y: int):
        """클라이언트 좌표 (x, y) 로 시스템 커서를 옮긴다 (게임은 WM_MOUSEMOVE 로 받는다)"""
        user32.SetCursorPos(self.origin_x + x, self.origin_y + y)

    def grab(self) -> Image.Image:
        """게임 클라이언트 영역을 RGB 이미지로 복사한다 (시스템 커서는 포함되지 않는다)"""
        box = (self.origin_x, self.origin_y, self.origin_x + self.width, self.origin_y + self.height)
        return ImageGrab.grab(bbox=box, all_screens=True).convert("RGB")


def read_cursor() -> dict:
    """현재 시스템 커서의 핸들·표시 여부·화면 좌표를 읽는다"""
    info = CURSORINFO()
    info.cbSize = ctypes.sizeof(CURSORINFO)
    if not user32.GetCursorInfo(ctypes.byref(info)):
        return {"handle": 0, "showing": False, "x": 0, "y": 0}
    # flags 의 0x1 = CURSOR_SHOWING, 0x2 = CURSOR_SUPPRESSED
    return {"handle": int(info.hCursor or 0), "flags": int(info.flags), "showing": bool(info.flags & 1),
            "x": info.ptScreenPos.x, "y": info.ptScreenPos.y}


class CursorLogger(threading.Thread):
    """커서 모양(핸들)·표시 상태가 바뀔 때마다 한 줄씩 기록하고, 처음 보는 핸들은 그림으로 저장한다"""

    def __init__(self, folder: Path):
        super().__init__(daemon=True)
        self.folder = folder
        self.stop_flag = threading.Event()
        (folder / "cursors").mkdir(parents=True, exist_ok=True)
        self.log = open(folder / "cursor_timeline.jsonl", "a", encoding="utf-8")
        # 이미 그림으로 저장한 핸들 집합
        self.saved = set()
        self.last_key = None

    def run(self):
        """종료 신호가 올 때까지 짧은 간격으로 커서를 읽는다"""
        while not self.stop_flag.is_set():
            state = read_cursor()
            key = (state["handle"], state.get("flags"))
            # 모양이나 표시 상태가 바뀐 순간만 기록한다
            if key != self.last_key:
                self.last_key = key
                handle = state["handle"]
                if handle and handle not in self.saved:
                    self.saved.add(handle)
                    try:
                        sheet, hotspot = cursor_sheet(handle)
                        sheet.save(self.folder / "cursors" / f"{handle}.png")
                        state["hotspot"] = hotspot
                    except Exception as error:  # 그림 저장 실패는 기록만 남긴다
                        state["saveError"] = str(error)
                state["t"] = now_iso()
                self.log.write(json.dumps(state, ensure_ascii=False) + "\n")
                self.log.flush()
            time.sleep(CURSOR_POLL_SECONDS)

    def stop(self):
        """감시를 멈추고 파일을 닫는다"""
        self.stop_flag.set()
        self.join(timeout=2)
        self.log.close()


class McpClient:
    """analyzeManager MCP 서버(stdio)를 자식 프로세스로 띄우는 최소 클라이언트"""

    def __init__(self, stderr_path: Path):
        self.stderr_file = open(stderr_path, "w", encoding="utf-8")
        self.process = subprocess.Popen(
            [str(ANALYZER_EXE), "--repo", str(REPO), "mcp"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=self.stderr_file, text=True, encoding="utf-8", creationflags=NO_WINDOW)
        self.messages = queue.Queue()
        self.number = 0
        threading.Thread(target=self._read_stdout, daemon=True).start()
        self.request("initialize", {"protocolVersion": "2025-03-26", "capabilities": {},
                                    "clientInfo": {"name": "ns-driver", "version": "1"}})
        self._send({"method": "notifications/initialized"})

    def _read_stdout(self):
        """서버가 한 줄씩 내보내는 JSON-RPC 메시지를 큐에 모은다"""
        for line in self.process.stdout:
            try:
                self.messages.put(json.loads(line))
            except json.JSONDecodeError as error:
                self.messages.put(error)
        self.messages.put(RuntimeError("MCP stdout 종료"))

    def _send(self, message: dict):
        """JSON-RPC 메시지 한 줄을 서버에 보낸다"""
        self.process.stdin.write(json.dumps({"jsonrpc": "2.0", **message}, ensure_ascii=False) + "\n")
        self.process.stdin.flush()

    def request(self, method: str, params: dict, timeout: float = 900.0):
        """요청을 보내고 대응하는 응답이 올 때까지 기다린다"""
        self.number += 1
        self._send({"id": self.number, "method": method, "params": params})
        deadline = time.monotonic() + timeout
        # 알림 등 다른 메시지는 건너뛴다
        while True:
            response = self.messages.get(timeout=max(0.01, deadline - time.monotonic()))
            if isinstance(response, Exception):
                raise response
            if response.get("id") == self.number:
                if "error" in response:
                    raise RuntimeError(json.dumps(response["error"], ensure_ascii=False))
                return response["result"]

    def call(self, name: str, arguments: dict, timeout: float = 900.0) -> dict:
        """도구를 호출하고 구조화 결과(data)에 isError 를 덧붙여 돌려준다"""
        result = self.request("tools/call", {"name": name, "arguments": arguments}, timeout)
        data = dict(result.get("structuredContent") or {})
        data["isError"] = bool(result.get("isError"))
        # 구조화 결과가 없으면 텍스트 내용을 그대로 담는다
        if not data.get("sessionId") and result.get("content"):
            data["text"] = [c.get("text") for c in result["content"] if c.get("type") == "text"]
        return data

    def close(self):
        """입력을 닫아 서버를 끝내고 남으면 강제 종료한다"""
        try:
            self.process.stdin.close()
            self.process.wait(timeout=10)
        except Exception:
            self.process.kill()
        self.stderr_file.close()


class Driver:
    """명령 한 건(steps 목록)을 순서대로 실행하고 결과를 모은다"""

    def __init__(self, work: Path, session_id: str = None):
        self.work = work
        work.mkdir(parents=True, exist_ok=True)
        (work / "cmd").mkdir(exist_ok=True)
        (work / "out").mkdir(exist_ok=True)
        self.actions = open(work / "actions.jsonl", "a", encoding="utf-8")
        self.mcp = McpClient(work / "mcp-stderr.log")
        self.cursor = CursorLogger(work)
        self.cursor.start()
        # 현재 분석 세션 ID와 증거 폴더
        self.session_id = session_id
        self.session_dir = SESSIONS_DIR / session_id if session_id else None

    def log(self, record: dict):
        """실행한 단계를 시각과 함께 actions.jsonl 에 한 줄 남긴다"""
        record["t"] = now_iso()
        self.actions.write(json.dumps(record, ensure_ascii=False) + "\n")
        self.actions.flush()

    def evidence_path(self, evidence) -> str:
        """증거 항목(Path 필드)을 세션 폴더 기준 절대 경로로 바꾼다"""
        if not evidence or not self.session_dir:
            return ""
        return str(self.session_dir / evidence["path"])

    def save_shot(self, name: str, image: str, cursor: dict, result: dict):
        """증거 PNG 를 shots/<name>.png 로 복사하고, 시스템 커서를 합성한 <name>.cur.png 도 만든다"""
        target = self.work / "shots" / (name + ".png")
        target.parent.mkdir(exist_ok=True)
        shutil.copyfile(image, target)
        result["copy"] = str(target)
        # 커서가 보이는 상태이면 합성본도 저장한다
        if composite_cursor(image, str(target.with_suffix(".cur.png")), cursor):
            result["copyCursor"] = str(target.with_suffix(".cur.png"))

    def run_step(self, step: dict) -> dict:
        """단계 하나를 실행한다. op 별 인자는 모듈 설명과 아래 분기를 따른다"""
        op = step["op"]
        started = now_iso()
        result = {"op": op, "started": started}
        free = shutil.disk_usage(REPO).free
        # 디스크 여유가 너무 적으면 새 입력을 보내지 않는다
        if op not in ("end", "sleep", "disk", "mark") and free < MIN_FREE_BYTES:
            raise RuntimeError(f"디스크 여유 부족: {free} 바이트")
        if op == "start":
            data = self.mcp.call("start_session", {"label": step.get("label", "TEST02 자동 분석"), "fps": step.get("fps", 30)})
            result.update(data)
            if data.get("sessionId"):
                self.session_id = data["sessionId"]
                self.session_dir = SESSIONS_DIR / self.session_id
                result["sessionDir"] = str(self.session_dir)
        elif op in ("click", "move", "key", "drag"):
            args = {"sessionId": self.session_id, "kind": op, "includeImage": False,
                    "durationMs": step.get("dur", 80), "settleMs": step.get("settle", 0)}
            # 좌표·버튼·키 등 선택 인자를 그대로 넘긴다
            for src, dst in (("x", "x"), ("y", "y"), ("toX", "toX"), ("toY", "toY"), ("button", "button"),
                             ("key", "key"), ("viaX", "viaX"), ("viaY", "viaY"), ("holdViaMs", "holdViaMs"),
                             ("holdMs", "holdMs")):
                if src in step:
                    args[dst] = step[src]
            data = self.mcp.call("game_input", args)
            result["isError"] = data.get("isError")
            result["error"] = data.get("error")
            result["changedRatio"] = data.get("changedRatio")
            result["after"] = self.evidence_path(data.get("after"))
            result["before"] = self.evidence_path(data.get("before"))
            result["args"] = {k: v for k, v in step.items() if k != "op"}
            # 입력 직후 커서 상태도 같이 남긴다
            result["cursor"] = read_cursor()
            # 이름이 있으면 입력 뒤 화면(+커서 합성)을 shots/ 에 저장한다
            if step.get("name") and result["after"]:
                self.save_shot(step["name"], result["after"], result["cursor"], result)
        elif op == "shot":
            data = self.mcp.call("capture_state", {"sessionId": self.session_id, "includeImage": False})
            result["isError"] = data.get("isError")
            result["image"] = self.evidence_path(data.get("evidence"))
            result["cursor"] = read_cursor()
            # 이름이 있으면 작업 폴더 shots/ 에도 복사해 두고 경로를 알려 준다
            if step.get("name") and result["image"]:
                self.save_shot(step["name"], result["image"], result["cursor"], result)
        elif op == "cursor":
            result["cursor"] = read_cursor()
        elif op == "wait":
            data = self.mcp.call("wait_for_change", {"sessionId": self.session_id, "includeImage": False,
                                                      "region": step.get("region", ""), "timeoutMs": step.get("timeoutMs", 5000),
                                                      "threshold": step.get("threshold", 0.01)}, timeout=60)
            result.update({k: data.get(k) for k in ("matched", "changedRatio", "elapsedMs", "isError", "error")})
        elif op == "status":
            result.update(self.mcp.call("game_status", {"sessionId": self.session_id}))
        elif op == "sleep":
            time.sleep(step["ms"] / 1000.0)
        elif op == "mark":
            result["text"] = step.get("text", "")
        elif op == "note":
            result.update(self.mcp.call("record_observation", {"sessionId": self.session_id, "note": step["text"]}))
        elif op == "disk":
            result["freeGB"] = round(free / 1024 ** 3, 2)
        elif op == "end":
            result.update(self.mcp.call("end_session", {"sessionId": self.session_id, "force": step.get("force", False)}, timeout=90))
        else:
            raise ValueError(f"알 수 없는 op: {op}")
        result["finished"] = now_iso()
        self.log(result)
        return result

    def run_command(self, command: dict) -> dict:
        """명령 파일 하나의 모든 단계를 실행하고, 오류가 나면 거기서 멈추고 알린다"""
        results = []
        error = None
        # 단계를 순서대로 실행한다
        for step in command.get("steps", []):
            try:
                results.append(self.run_step(step))
            except Exception as exc:  # 한 단계 실패 시 이후 단계는 실행하지 않는다
                error = f"{type(exc).__name__}: {exc}"
                self.log({"op": "error", "error": error, "step": step})
                break
        return {"sessionId": self.session_id, "results": results, "error": error}

    def serve(self):
        """cmd/ 폴더를 감시하며 명령 파일을 이름 순서로 처리한다. cmd/QUIT 이 생기면 끝낸다"""
        cmd_dir = self.work / "cmd"
        (self.work / "ready.txt").write_text(now_iso(), encoding="utf-8")
        # 종료 신호 파일이 생길 때까지 반복한다
        while not (cmd_dir / "QUIT").exists():
            pending = sorted(p for p in cmd_dir.glob("*.json") if time.time() - p.stat().st_mtime > 0.3)
            if not pending:
                time.sleep(0.1)
                continue
            path = pending[0]
            try:
                command = json.loads(path.read_text(encoding="utf-8-sig"))
            except json.JSONDecodeError:
                # 아직 쓰는 중일 수 있으니 잠시 뒤 다시 읽는다
                time.sleep(0.3)
                try:
                    command = json.loads(path.read_text(encoding="utf-8-sig"))
                except json.JSONDecodeError as exc:
                    (self.work / "out" / (path.stem + ".json")).write_text(
                        json.dumps({"error": f"JSON 오류: {exc}"}, ensure_ascii=False), encoding="utf-8")
                    path.rename(path.with_suffix(".bad"))
                    continue
            outcome = self.run_command(command)
            tmp = self.work / "out" / (path.stem + ".tmp")
            tmp.write_text(json.dumps(outcome, ensure_ascii=False, indent=1), encoding="utf-8")
            os.replace(tmp, self.work / "out" / (path.stem + ".json"))
            path.rename(path.with_suffix(".done"))

    def close(self):
        """커서 감시와 MCP 서버를 정리한다 (게임 종료는 end 단계로 별도 수행)"""
        self.cursor.stop()
        self.mcp.close()
        self.actions.close()


def main():
    """명령줄 진입점: serve 하위 명령"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    serve = sub.add_parser("serve")
    serve.add_argument("--work", type=Path, required=True)
    # 이미 실행 중인 세션에 다시 붙을 때 쓴다
    serve.add_argument("--session", default=None)
    args = parser.parse_args()
    driver = Driver(args.work, args.session)
    try:
        driver.serve()
    finally:
        driver.close()


if __name__ == "__main__":
    sys.exit(main())
