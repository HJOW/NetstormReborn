# -*- coding: utf-8 -*-
"""
원본 게임 자동 분석: TEST02 맵의 사제 건설 한도 (Windows 전용, 원본 게임을 실제로 구동한다)

사전 조건: AGENTS.md 의 실행 허용 시스템이거나 사용자가 실행을 확인한 상태여야 한다. 실행 중에는 마우스·키보드를 건드리지 않는다.
흐름: 메인 메뉴 → Edit → Create New Map 에 TEST02 입력 → 편집기 → Esc → Game → Test Battle → Go!
      → 사제로 Rain 템플 건설(완공까지) → 템플 두 번째 시도(메뉴 줄 확인)
      → 워크샵 설치 가능 위치 전수 훑기(아직 워크샵이 없는 섬) → 빈틈없이 쌓아 가며 건설(더 못 지을 때까지)
      → Leave Mission → 종료.
1차·2차 정찰에서 확정한 좌표·메뉴 오프셋·판정 방식을 쓴다. 상세와 결과는 docs/videos/auto-test02-construct-20261004.md.

화면 좌표는 모두 "기준 화면"(시험 전투 시작 때 카메라) 좌표로 적고, 카메라가 움직이면 템플·집 그림을 찾아 이동량을 재서
실제 화면 좌표로 바꾼다. (P 키를 이미 선택된 사제에게 한 번 더 누르면 카메라가 사제 위치로 이동하는 등 카메라가 밀릴 수 있다.)

사용법: python tools/analyze_test02_construct.py --work extracted/test02-construct-run3
결과: <work>/summary.json, actions.jsonl(모든 단계의 시각), cursor_timeline.jsonl(커서 모양 변화), shots/*.png
"""
import argparse
import json
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ns_driver as nd  # noqa: E402

# ---- 상수 (모두 기준 화면 좌표, 1024x768 클라이언트) ----
# 선택한 사제 머리 위 체력 막대의 정확한 색 (1차 정찰 화면에서 측정)
HEALTH_BAR_COLOR = (118, 173, 105)
# 체력 막대로 인정하는 가로 연속 픽셀 수 범위 (사제 막대는 약 25픽셀)
HEALTH_BAR_RUN = (18, 40)
# 체력 막대에서 사제 몸통(우클릭 대상)까지의 아래쪽 거리(픽셀)
PRIEST_BODY_DY = 20
# 사제 우클릭 메뉴 항목 오프셋: 우클릭 지점 기준 (x, y). Construct 줄과 Construct Building 하위 창의 줄들
MENU_CONSTRUCT = (51, 21)
MENU_TEMPLE_ROW = (226, 21)
MENU_WORKSHOP_ROW = (226, 38)
# Temple 하위 창의 항목 y 오프셋 (Wind, Rain, Thunder) — 줄 간격 21
MENU_TEMPLE_ITEM_Y = {"wind": 25, "rain": 46, "thunder": 67}
# Workshop 하위 창의 항목 y 오프셋 (Sun, Wind, Rain, Thunder)
MENU_WORKSHOP_ITEM_Y = {"sun": 41, "wind": 62, "rain": 83, "thunder": 104}
# 항목 클릭 x: 우클릭 지점 + 330 (오른쪽 가장자리에서 하위 창이 안쪽으로 밀리므로 940 을 넘기지 않는다)
MENU_ITEM_DX = 330
MENU_ITEM_X_MAX = 940
# 오른쪽 가장자리에서 메뉴가 안쪽으로 밀릴 때를 위한 클릭 x 한계 (Construct 줄 / 하위 창 줄)
MENU_CONSTRUCT_X_MAX = 990
MENU_ROW_X_MAX = 960
# 한 칸의 화면 크기 (가로 16, 세로 11)
CELL_W, CELL_H = 16, 11
# 워크샵(7x8칸) 설치 미리보기 테두리는 커서가 가장 가까운 칸에 붙어 움직인다 (1차 정찰 실측):
# 왼쪽 = 16 x (round(rx/16) - 4), 위 = 11 x round(ry/11) - 34, 가로 112·세로 88픽셀
WS_COLS_LEFT, WS_TOP_OFFSET, WS_WIDTH, WS_HEIGHT = 4, 34, 112, 88
# 사제 이동 속도 (칸/초, 직선 실측 1.74~1.82)
PRIEST_SPEED = 1.8
# 설치 뒤 사제가 서는 위치: 설치 커서 기준 (-3, +48) (1차 정찰 실측)
SITE_FRONT = (-3, 48)
# 시험 전투 시작 때 사제 위치와 템플 설치 위치
PRIEST_START = (520, 388)
TEMPLE_SITE = (780, 220)
# 사제를 매 사이클마다 불러 세우는 자리 (섬 가운데). 우클릭 메뉴를 열기 좋은 빈 곳이다
HOME = (560, 470)
# HOME 둘레 예약 영역(왼쪽, 위, 오른쪽, 아래): 이 영역과 겹치는 워크샵 자리는 짓지 않는다
RESERVED = (524, 430, 596, 500)
# 훑기 범위(커서 칸 번호: 가로 16·세로 11픽셀 단위): S3 영상에서 복원한 가능 구간(열 22~57, 행 13~48)에 여유를 둔 범위
SCAN_COLS = range(18, 61)
SCAN_ROWS = range(10, 53)
# 후보 위치마다 커서를 옮긴 뒤 게임이 미리보기를 다시 그릴 때까지 기다리는 시간(초). 게임이 이 PC에서 약 14FPS 로 돌아 0.11초는 모자라 이전 칸이 남는 일이 있었다
HOVER_SETTLE = 0.17
# 훑기에서 읽지 못한 칸을 다시 확인할 때 기다리는 시간(초)
RETRY_SETTLE = 0.4
# 사제가 HOME 에 도착하기를 기다리는 최대 시간(초)
HOME_TIMEOUT = 75.0
# 건설 시간(사제 도착 뒤 약 10초) 위에 더하는 여유(초)
BUILD_MARGIN = 6.0
# 워크샵 건설을 멈추는 최대 개수 (안전 상한: 디스크·시간)
MAX_WORKSHOPS = 40
# 카메라 이동량 측정용 템플릿 영역(기준 화면): 템플 그림, 집 그림
TEMPLE_BOX = (715, 170, 845, 275)
HOUSE_BOX = (555, 250, 630, 310)
# 템플릿 일치로 인정하는 정규화 상관 하한
MATCH_MIN = 0.8
# 설치 가능 미리보기 테두리의 순백 색 값
WHITE = 255


def find_selected_priest(img: Image.Image):
    """선택된 사제의 체력 막대를 찾아 우클릭할 몸통 좌표를 돌려준다 (없으면 None)"""
    arr = np.asarray(img)
    mask = (arr[..., 0] == HEALTH_BAR_COLOR[0]) & (arr[..., 1] == HEALTH_BAR_COLOR[1]) & (arr[..., 2] == HEALTH_BAR_COLOR[2])
    # 막대 후보 행을 위에서부터 훑으며 가로 연속 길이가 막대 범위인 곳을 찾는다
    for y in range(mask.shape[0]):
        xs = np.nonzero(mask[y])[0]
        if len(xs) < HEALTH_BAR_RUN[0]:
            continue
        # 연속 구간으로 나눈다
        runs = np.split(xs, np.nonzero(np.diff(xs) != 1)[0] + 1)
        for run in runs:
            if HEALTH_BAR_RUN[0] <= len(run) <= HEALTH_BAR_RUN[1]:
                return int(run.mean()), y + PRIEST_BODY_DY
    return None


def find_priest_by_sprite(img: Image.Image, box):
    """영역 box=(왼쪽,위,오른쪽,아래) 안에서 사제 그림(어두운 옷·파란 로브·금발 머리)의 중심을 찾는다 (없으면 None).
    눈 지면은 밝은 라벤더색이라 이 세 색 조건에 들지 않고, 그림자는 반투명이라 어둡지 않아 제외된다."""
    arr = np.asarray(img.crop(box)).astype(int)
    r, g, b = arr[..., 0], arr[..., 1], arr[..., 2]
    dark = (r < 100) & (g < 100) & (b < 100)
    robe = (b > r + 40) & (r < 120)
    hair = (r > 150) & (r - b > 60)
    ys, xs = np.nonzero(dark | robe | hair)
    # 사제 그림은 약 60~150픽셀이다. 너무 적으면 사제가 없는 것으로 본다
    if len(xs) < 25:
        return None
    return int(xs.mean()) + box[0], int(ys.mean()) + box[1]


def locate(template: np.ndarray, image: np.ndarray):
    """회색조 이미지 안에서 템플릿의 위치를 FFT 정규화 상관으로 찾는다. (x, y, 상관값)"""
    th, tw = template.shape
    t = template - template.mean()
    h, w = image.shape
    size = (h + th, w + tw)

    def corr(a, b):
        """두 배열의 상호상관(FFT)"""
        return np.fft.irfft2(np.fft.rfft2(a, size) * np.conj(np.fft.rfft2(b, size)), size)

    ones = np.ones_like(t)
    num = corr(image, t)
    s1, s2 = corr(image, ones), corr(image ** 2, ones)
    n = th * tw
    ncc = num / np.sqrt(np.maximum(s2 - s1 ** 2 / n, 1e-6) * np.sum(t ** 2))
    ncc = ncc[:h - th + 1, :w - tw + 1]
    y, x = np.unravel_index(np.argmax(ncc), ncc.shape)
    return int(x), int(y), float(ncc[y, x])


class Cam:
    """카메라 이동량 (sx, sy): 실제 화면 좌표 = 기준 화면 좌표 + (sx, sy).
    기준 화면(템플 완공 직후, 건물 없음)에서 템플·집 그림을 잘라 두고, 현재 화면에서 그 그림을 찾아 이동량을 잰다."""

    def __init__(self):
        self.sx = 0
        self.sy = 0
        self.templates = []

    def set_reference(self, img: Image.Image):
        """기준 화면에서 템플·집 템플릿을 잘라 둔다 (이 화면의 이동량을 (0,0)으로 정의)"""
        gray = np.asarray(img.convert("L")).astype(float)
        self.templates = [(box, gray[box[1]:box[3], box[0]:box[2]]) for box in (TEMPLE_BOX, HOUSE_BOX)]
        self.sx = self.sy = 0

    def refresh(self, img: Image.Image) -> bool:
        """현재 화면에서 이동량을 다시 잰다. 어느 템플릿도 충분히 맞지 않으면 이전 값을 유지하고 False"""
        gray = np.asarray(img.convert("L")).astype(float)
        best = None
        # 템플·집 템플릿 중 가장 잘 맞는 것을 쓴다
        for box, tpl in self.templates:
            x, y, score = locate(tpl, gray)
            if best is None or score > best[2]:
                best = (x - box[0], y - box[1], score)
        if best and best[2] >= MATCH_MIN:
            self.sx, self.sy = best[0], best[1]
            return True
        return False

    def scr(self, x: int, y: int):
        """기준 화면 좌표를 실제 화면 좌표로"""
        return x + self.sx, y + self.sy

    def ref(self, x: int, y: int):
        """실제 화면 좌표를 기준 화면 좌표로"""
        return x - self.sx, y - self.sy


def frame_ref(rx: int, ry: int):
    """기준 화면 커서 (rx, ry) 에서 워크샵 설치 테두리의 (왼쪽, 위, 오른쪽, 아래)를 계산한다 (칸에 붙어 움직임)"""
    left = CELL_W * (round(rx / CELL_W) - WS_COLS_LEFT)
    top = CELL_H * round(ry / CELL_H) - WS_TOP_OFFSET
    return left, top, left + WS_WIDTH - 1, top + WS_HEIGHT - 1


def read_preview(img: Image.Image, cam: Cam):
    """화면에 보이는 설치 미리보기 테두리의 (col, row, 가능 여부)를 읽는다 (기준 화면 칸 번호). 테두리가 없으면 None.
    커서 위치가 아니라 테두리 자체의 위치를 읽으므로 게임이 미리보기를 아직 못 옮겼어도 어느 칸의 결과인지 틀리지 않는다.
    가능 = 순백(255,255,255) 세로선 두 줄(간격 111), 불가 = 순적색(255,0,0) 세로선 두 줄"""
    arr = np.asarray(img)
    white = (arr[..., 0] == WHITE) & (arr[..., 1] == WHITE) & (arr[..., 2] == WHITE)
    red = (arr[..., 0] == 255) & (arr[..., 1] == 0) & (arr[..., 2] == 0)
    # 흰색을 먼저, 없으면 붉은색 테두리를 찾는다
    for mask, ok in ((white, True), (red, False)):
        counts = mask[:, 90:].sum(axis=0)
        candidates = {x + 90 for x in np.nonzero(counts >= 56)[0]}
        # 왼쪽 선 후보마다 오른쪽 선이 111픽셀 옆에 있는지 본다
        for x in sorted(candidates):
            if x + WS_WIDTH - 1 in candidates:
                rows = np.nonzero(mask[:, x] & mask[:, x + WS_WIDTH - 1])[0]
                if len(rows) < 56:
                    continue
                left, top = x - cam.sx, int(rows.min()) - cam.sy
                return round(left / CELL_W) + WS_COLS_LEFT, round((top + WS_TOP_OFFSET) / CELL_H), ok
    return None


def preview_valid(img: Image.Image, cam: Cam, rx: int, ry: int) -> bool:
    """기준 화면 커서 (rx, ry) 에 들고 있는 워크샵의 미리보기가 '설치 가능'(흰 테두리)인지 판정한다.
    불가능이면 붉은 테두리·분홍 채움이다. 건물 그림(나무)이 위쪽 테두리를 가리므로 왼쪽·오른쪽·아래 세 변이 순백이면 가능으로 본다."""
    arr = np.asarray(img).astype(int)
    white = (arr[..., 0] == WHITE) & (arr[..., 1] == WHITE) & (arr[..., 2] == WHITE)
    left, top, right, bottom = frame_ref(rx, ry)
    left, top, right, bottom = left + cam.sx, top + cam.sy, right + cam.sx, bottom + cam.sy
    if top - 1 < 0 or bottom + 1 >= white.shape[0] or left - 1 < 90 or right + 1 >= white.shape[1]:
        return False
    # ±1 픽셀 어긋남을 허용해 가장 흰 비율을 고른다
    left_ratio = max(white[top:bottom + 1, left + d].mean() for d in (-1, 0, 1))
    right_ratio = max(white[top:bottom + 1, right + d].mean() for d in (-1, 0, 1))
    bottom_ratio = max(white[bottom + d, left:right + 1].mean() for d in (-1, 0, 1))
    return left_ratio > 0.8 and right_ratio > 0.6 and bottom_ratio > 0.6


def rects_overlap(a, b) -> bool:
    """두 사각형(왼쪽, 위, 오른쪽, 아래; 끝점 포함)이 겹치는지"""
    return not (a[2] < b[0] or b[2] < a[0] or a[3] < b[1] or b[3] < a[1])


def cells_between(a, b) -> float:
    """두 화면 좌표 사이를 사제가 걷는 칸 수(체비쇼프 거리: 대각선 먼저 이동)"""
    return max(abs(a[0] - b[0]) / CELL_W, abs(a[1] - b[1]) / CELL_H)


class Run:
    """한 번의 분석 실행 상태: 드라이버·화면 보조·카메라·요약"""

    def __init__(self, work: Path, session_id: str = None):
        self.d = nd.Driver(work, session_id)
        self.screen = None
        self.cam = Cam()
        self.summary = {"started": nd.now_iso(), "workshops": [], "events": []}
        self.work = work

    def attach_screen(self):
        """이미 실행 중인 세션의 게임 창 위치로 화면 보조 객체를 만든다 (--session 이어 붙기용)"""
        window = self.step("status")["window"]
        self.screen = nd.ClientScreen(window["x"], window["y"])

    def step(self, op: str, **kw):
        """드라이버 단계를 실행하고 결과를 돌려준다"""
        return self.d.run_step({"op": op, **kw})

    def note(self, text: str, **extra):
        """요약에 사건 한 줄을 남긴다 (시각 포함)"""
        self.summary["events"].append({"t": nd.now_iso(), "text": text, **extra})
        self.d.log({"op": "mark", "text": text})

    def save_grab(self, name: str) -> Image.Image:
        """직접 화면 복사본을 shots/ 에 저장한다 (커서 합성 없음)"""
        img = self.screen.grab()
        path = self.work / "shots" / (name + ".png")
        path.parent.mkdir(exist_ok=True)
        img.save(path)
        return img

    def cam_refresh(self) -> bool:
        """현재 화면으로 카메라 이동량을 갱신하고, 바뀌었으면 기록한다"""
        before = (self.cam.sx, self.cam.sy)
        ok = self.cam.refresh(self.screen.grab())
        if (self.cam.sx, self.cam.sy) != before:
            self.note("카메라 이동량 변경", before=before, after=(self.cam.sx, self.cam.sy))
        if not ok:
            self.note("카메라 이동량을 재지 못함(템플릿 불일치)")
        return ok

    def click_expect(self, x: int, y: int, min_ratio: float, name: str = None, settle: int = 900, retries: int = 4):
        """클릭 뒤 화면 변화 비율이 min_ratio 이상일 때까지(모달 창이 닫히거나 열릴 때까지) 같은 클릭을 되풀이한다.
        게임이 아직 입력을 받지 못하는 시작 직후의 클릭 유실을 막는다 (유실되면 변화 비율이 거의 0)"""
        # 재시도 횟수만큼 클릭을 시도한다
        for attempt in range(retries):
            res = self.step("click", x=x, y=y, dur=120, settle=settle, **({"name": name} if name else {}))
            ratio = res.get("changedRatio") or 0.0
            if ratio >= min_ratio:
                return res
            self.note(f"클릭 ({x},{y}) 화면 변화 {ratio:.4f} < {min_ratio}: 재시도 {attempt + 1}")
            time.sleep(2.0)
        raise RuntimeError(f"클릭 ({x},{y}) 이 화면을 바꾸지 못했다")

    def boot_to_battle(self):
        """원본 시작 → 팁 창 닫기 → Edit 로 TEST02 불러오기 → 시험 전투 시작"""
        res = self.step("start", label="TEST02 건설 한도 분석: 템플·워크샵 전수 (30FPS 자동 분석)", fps=30)
        window = res["evidence"]["window"]
        self.screen = nd.ClientScreen(window["x"], window["y"])
        # 시작 직후 약 10초 동안은 팁 창이 입력을 받지 못하므로 넉넉히 기다린다
        time.sleep(9.0)
        self.click_expect(661, 436, 0.015)                                  # 팁 창 OK → Not Validated 창
        self.click_expect(511, 464, 0.05, settle=1200)                      # Not Validated OK → 메인 메뉴
        self.click_expect(393, 343, 0.10, name="b01-edit-list")             # Edit → Load Battle Map 목록
        self.click_expect(451, 647, 0.01, name="b02-new-name")              # Create New Map → 이름 입력 창
        # 맵 이름 TEST02 입력 (Shift 로 대문자)
        for key in ("SHIFT+T", "SHIFT+E", "SHIFT+S", "SHIFT+T", "0", "2"):
            self.step("key", key=key, dur=60, settle=120)
        self.step("shot", name="b03-typed")
        self.click_expect(483, 419, 0.30, settle=1800, name="b04-editor")   # OK → 편집기
        self.note("편집기에서 TEST02 로드")
        self.step("key", key="ESCAPE", dur=90, settle=700)                  # 메뉴 막대
        self.step("click", x=104, y=9, dur=100, settle=700, name="b05-game-menu")          # Game
        self.click_expect(130, 25, 0.05, settle=1800, name="b06-briefing")  # Test Battle → 브리핑 창
        self.click_expect(598, 462, 0.02, settle=1500, name="b07-battle")   # Go! → 전투
        self.note("시험 전투 시작")

    def open_construct_menu(self, priest_xy, kind: str, item: str):
        """사제 우클릭 → Construct → Temple/Workshop 줄 → 항목을 눌러 건물을 커서에 붙인다 (좌표는 실제 화면)"""
        px, py = priest_xy
        self.step("click", x=px, y=py, button="right", dur=100, settle=350, name=f"menu-{kind}-a")
        self.step("click", x=min(px + MENU_CONSTRUCT[0], MENU_CONSTRUCT_X_MAX), y=py + MENU_CONSTRUCT[1], dur=100, settle=350)
        row = MENU_TEMPLE_ROW if kind == "temple" else MENU_WORKSHOP_ROW
        self.step("click", x=min(px + row[0], MENU_ROW_X_MAX), y=py + row[1], dur=100, settle=350, name=f"menu-{kind}-b")
        item_y = (MENU_TEMPLE_ITEM_Y if kind == "temple" else MENU_WORKSHOP_ITEM_Y)[item]
        self.step("click", x=min(px + MENU_ITEM_DX, MENU_ITEM_X_MAX), y=py + item_y, dur=100, settle=300,
                  name=f"menu-{kind}-c-picked")

    def wait_theme_snow(self, timeout: float = 80.0) -> float:
        """템플 완공 때 지면이 초록 풀밭에서 눈으로 바뀌는 시각까지 기다린다 (경과 초 반환)"""
        t0 = time.time()
        # 지면 한 점의 빨강 성분이 올라가면 눈으로 본다 (풀밭은 약 63, 눈은 약 149)
        while time.time() - t0 < timeout:
            img = self.screen.grab()
            if img.getpixel((700, 500))[0] > 110:
                return time.time() - t0
            time.sleep(0.25)
        raise RuntimeError("템플 완공(지면 테마 변화)을 시간 안에 확인하지 못했다")

    def build_temple(self):
        """Rain 템플을 (780,220) 에 설치하고 완공까지 기다린다. 사제·커서 확인용 이동 포함"""
        self.step("move", x=PRIEST_START[0], y=PRIEST_START[1], settle=250, name="t01-hover-priest-unselected")
        self.step("click", x=PRIEST_START[0], y=PRIEST_START[1], dur=100, settle=250, name="t02-priest-selected")
        for name, x, y in (("t03-hover-ground", 700, 450), ("t04-hover-void", 150, 400), ("t05-hover-house", 595, 290),
                           ("t06-hover-self", PRIEST_START[0], PRIEST_START[1])):
            self.step("move", x=x, y=y, settle=300, name=name)
        # 선택된 사제의 우클릭은 메뉴를 열지 못하므로 허공을 눌러 선택을 먼저 푼다
        self.step("click", x=150, y=650, dur=100, settle=300, name="t06b-deselected")
        self.open_construct_menu(PRIEST_START, "temple", "rain")
        # 설치 불가 위치와 가능 위치에서 미리보기·커서를 남긴다
        for name, x, y in (("t07-preview-void", 150, 400), ("t08-preview-house", 560, 290), ("t09-preview-edge", 400, 300),
                           ("t10-preview-ok", TEMPLE_SITE[0], TEMPLE_SITE[1])):
            self.step("move", x=x, y=y, settle=350, name=name)
        t_place = nd.now_iso()
        self.step("click", x=TEMPLE_SITE[0], y=TEMPLE_SITE[1], dur=100, settle=0, name="t11-temple-placed")
        self.note("템플 설치 클릭", site=TEMPLE_SITE)
        elapsed = self.wait_theme_snow()
        self.note("템플 완공(지면이 눈으로 바뀜)", seconds_after_place=round(elapsed, 2))
        self.summary["temple"] = {"placedAt": t_place, "completeAfterSec": round(elapsed, 2), "site": TEMPLE_SITE}
        time.sleep(1.5)
        self.step("shot", name="t12-temple-done")
        # 건물이 없는 이 화면을 카메라 기준으로 삼는다
        self.cam.set_reference(self.screen.grab())

    def press_p_select(self):
        """사제를 P 키로 선택한다. P 를 이미 선택된 상태에서 또 누르면 카메라가 사제 위치로 이동하므로(정찰에서 확인)
        먼저 허공을 눌러 선택을 푼 뒤 한 번만 누른다. 체력 막대가 보이면 그 좌표를, 건물에 가려 안 보이면 None 을 돌려준다"""
        self.step("click", x=150, y=650, dur=100, settle=250)
        self.step("key", key="P", dur=60, settle=700)
        self.cam_refresh()
        return find_selected_priest(self.screen.grab())

    def bring_priest_home(self):
        """사제를 HOME 으로 걸어오게 하고 우클릭할 몸통 좌표(실제 화면)를 돌려준다.
        P 로 선택한 뒤 HOME 땅을 눌러 이동시키고(명령 뒤 선택은 자동으로 풀린다), 둘레에서 사제 그림의 중심이 0.8초 동안
        움직이지 않으면 도착으로 본다. 도착하지 않으면 선택부터 두 번까지 다시 한다"""
        # 선택~이동~도착 확인을 최대 3번 시도한다
        for attempt in range(3):
            bar = self.press_p_select()
            home = self.cam.scr(*HOME)
            near = bar and abs(bar[0] - home[0]) <= 14 and abs(bar[1] - (home[1] - 8)) <= 14
            if near:
                # 이미 HOME 에 서 있다: 선택을 풀고 돌려준다
                self.step("click", x=150, y=650, dur=100, settle=250)
                return bar
            self.step("click", x=home[0], y=home[1], dur=100, settle=0, name="home-order")
            box = (home[0] - 36, home[1] - 48, home[0] + 36, home[1] + 24)
            t0 = time.time()
            last, still_since = None, None
            # HOME 둘레에서 사제 그림이 보이고 0.8초 동안 같은 자리에 머물 때까지 기다린다
            while time.time() - t0 < HOME_TIMEOUT:
                pos = find_priest_by_sprite(self.screen.grab(), box)
                if pos and last and abs(pos[0] - last[0]) <= 1 and abs(pos[1] - last[1]) <= 1:
                    still_since = still_since or time.time()
                    if time.time() - still_since >= 0.8:
                        return pos[0], pos[1] - 4
                else:
                    still_since = None
                last = pos
                time.sleep(0.25)
            self.note(f"사제가 HOME 에 도착하지 않음: 재시도 {attempt + 1}")
            self.save_grab(f"error-home-timeout-{attempt + 1}")
        raise RuntimeError("사제를 HOME 으로 불러오지 못했다")

    def temple_second_try(self):
        """템플이 이미 있을 때 Construct 메뉴의 Temple 줄을 확인하고 눌러 본다"""
        priest = self.bring_priest_home()
        px, py = priest
        self.step("click", x=px, y=py, button="right", dur=100, settle=350, name="u01-menu")
        self.step("click", x=min(px + MENU_CONSTRUCT[0], MENU_CONSTRUCT_X_MAX), y=py + MENU_CONSTRUCT[1], dur=100, settle=400, name="u02-construct")
        row_x = min(px + MENU_TEMPLE_ROW[0], MENU_ROW_X_MAX)
        self.step("move", x=row_x, y=py + MENU_TEMPLE_ROW[1], settle=300, name="u03-hover-temple-row")
        self.step("click", x=row_x, y=py + MENU_TEMPLE_ROW[1], dur=100, settle=500, name="u04-click-temple-row")
        # 메뉴를 닫는다 (허공 클릭)
        self.step("click", x=150, y=650, dur=100, settle=400, name="u05-closed")

    def hover_checks_after_build(self):
        """완공된 템플·집·땅 위에서 선택된 사제의 커서를 남긴다"""
        self.press_p_select()
        # 기준 화면 좌표를 실제 화면 좌표로 바꿔 커서를 올린다
        for name, x, y in (("h01-over-temple", 780, 215), ("h02-over-house", 595, 288), ("h03-over-ground", 520, 520)):
            sx, sy = self.cam.scr(x, y)
            self.step("move", x=sx, y=sy, settle=350, name=name)
        self.step("click", x=150, y=650, dur=100, settle=300)   # 허공 클릭으로 선택 해제

    def hover_ref(self, rx: int, ry: int):
        """기준 화면 좌표 (rx, ry) 로 커서를 옮기고 게임이 미리보기를 다시 그릴 때까지 기다린다"""
        x, y = self.cam.scr(rx, ry)
        self.screen.move(x, y)
        time.sleep(HOVER_SETTLE)

    def scan_valid_anchors(self):
        """워크샵을 커서에 붙인 채 칸마다 커서를 옮겨 설치 가능(흰 테두리)·불가(붉은 테두리) 칸을 전수 조사한다.
        테두리 자체의 위치를 읽어 기록하므로 미리보기가 한 박자 늦어도 칸이 어긋나지 않는다. 읽지 못한 칸은 더 오래 기다려 다시 읽는다.
        결과는 즉시 scan.json 에 저장하고 가능한 칸의 기준 화면 커서 좌표 목록을 돌려준다"""
        seen = {}
        t0 = time.time()
        stale = 0
        # 위에서 아래로, 왼쪽에서 오른쪽으로 모든 칸을 훑는다
        for row in SCAN_ROWS:
            for col in SCAN_COLS:
                self.hover_ref(col * CELL_W, row * CELL_H)
                result = read_preview(self.screen.grab(), self.cam)
                if result:
                    seen[(result[0], result[1])] = result[2]
                    stale += (result[0], result[1]) != (col, row)
        # 읽지 못한 칸(테두리가 화면 밖이거나 그리는 중이었던 칸)을 더 오래 기다려 다시 읽는다
        missing = [(c, r) for r in SCAN_ROWS for c in SCAN_COLS if (c, r) not in seen]
        retried = 0
        # 누락 칸마다 커서를 옮기고 충분히 기다린 뒤 읽는다
        for col, row in missing:
            x, y = self.cam.scr(col * CELL_W, row * CELL_H)
            self.screen.move(x, y)
            time.sleep(RETRY_SETTLE)
            result = read_preview(self.screen.grab(), self.cam)
            if result:
                seen[(result[0], result[1])] = result[2]
                retried += 1
        valid = sorted((c, r) for (c, r), ok in seen.items() if ok)
        invalid = sorted((c, r) for (c, r), ok in seen.items() if not ok)
        still_missing = [(c, r) for r in SCAN_ROWS for c in SCAN_COLS if (c, r) not in seen]
        info = {"valid": valid, "invalid": invalid, "missing": still_missing, "stale": stale, "firstMissing": len(missing),
                "retried": retried, "cols": [SCAN_COLS.start, SCAN_COLS.stop], "rows": [SCAN_ROWS.start, SCAN_ROWS.stop],
                "seconds": round(time.time() - t0, 1)}
        (self.work / "scan.json").write_text(json.dumps(info, ensure_ascii=False), encoding="utf-8")
        self.note("설치 가능 위치 훑기 끝", valid=len(valid), invalid=len(invalid), missing=len(still_missing), stale=stale, seconds=info["seconds"])
        self.summary["scan"] = {k: info[k] for k in ("stale", "firstMissing", "retried", "seconds")}
        self.summary["scan"].update(valid=len(valid), invalid=len(invalid), missing=len(still_missing))
        return [(c * CELL_W, r * CELL_H) for c, r in valid]

    def wait_site_complete(self, site, shadow_ref: Image.Image, timeout: float):
        """설치 자리 영역이 (그림자 단계와 충분히 달라진 뒤) 2초 동안 변하지 않으면 완공으로 보고 그 시각까지의 초를 돌려준다"""
        prev, stable_since, t0 = None, None, time.time()
        ref_arr = None
        # 최대 timeout 초까지 0.4초 간격으로 관찰한다
        while time.time() - t0 < timeout:
            left, top, right, bottom = frame_ref(*site)
            box = (left + self.cam.sx, top + self.cam.sy, right + self.cam.sx + 1, bottom + self.cam.sy + 1)
            if ref_arr is None:
                ref_arr = np.asarray(shadow_ref.crop(box)).astype(int)
            cur = np.asarray(self.screen.grab().crop(box)).astype(int)
            far = np.abs(cur - ref_arr).mean()
            if prev is not None and far > 12:
                if np.abs(cur - prev).mean() < 1.0:
                    stable_since = stable_since or time.time()
                    if time.time() - stable_since >= 2.0:
                        return stable_since - t0
                else:
                    stable_since = None
            prev = cur
            time.sleep(0.4)
        return None

    def plan_packing(self, valid):
        """설치 가능 위치 중에서 서로 겹치지 않게 골라 빈틈없이 쌓는 순서를 만든다.
        위쪽 줄부터, 같은 줄에서는 왼쪽부터 놓는다: 다음 건물이 항상 사제 뒤쪽(남쪽)이나 옆에 서므로 사제가 건물에 가려지지 않는다.
        사제를 불러 세우는 HOME 둘레(RESERVED)와 겹치는 자리는 뺀다"""
        plan, rects = [], [RESERVED]
        # 위쪽(y 작은 쪽) 줄 먼저, 같은 줄에서는 왼쪽부터
        for rx, ry in sorted(valid, key=lambda p: (p[1], p[0])):
            rect = frame_ref(rx, ry)
            if any(rects_overlap(rect, other) for other in rects):
                continue
            plan.append((rx, ry))
            rects.append(rect)
        return plan

    def build_workshops(self, plan, carrying: bool = False):
        """계획한 자리에 Sun 워크샵을 하나씩 짓는다: 사제 불러오기 → 메뉴 → (미리보기가 가능한 첫 자리) → 설치 → 완공 대기.
        메뉴로 집은 건물은 허공을 눌러도 내려놓아지지 않으므로(정찰에서 확인) 불가 자리는 건너뛰고 같은 건물로 다음 후보를 시험한다"""
        remaining = list(plan)
        built = 0
        # 남은 자리가 있는 동안 한 채씩 짓는다
        while remaining and built < MAX_WORKSHOPS:
            if carrying:
                # 훑기를 마친 직후라 건물을 이미 들고 있고 사제도 선택되어 있다(HOME 에 서 있음)
                priest = self.cam.scr(*HOME)
                carrying = False
            else:
                priest = self.bring_priest_home()
                self.open_construct_menu(priest, "workshop", "sun")
            site = None
            skipped = []
            # 남은 후보를 차례로 시험해 처음 가능한 자리를 고른다
            while remaining:
                candidate = remaining.pop(0)
                self.hover_ref(*candidate)
                if preview_valid(self.screen.grab(), self.cam, *candidate):
                    site = candidate
                    break
                skipped.append(candidate)
            if skipped:
                self.note("미리보기가 불가라 건너뜀", sites=skipped, built=built)
            if site is None:
                self.note("더 지을 수 있는 자리가 없음", built=built)
                self.save_grab("z00-no-more-sites")
                break
            built += 1
            n = built
            x, y = self.cam.scr(*site)
            self.step("click", x=x, y=y, dur=100, settle=0, name=f"w{n:02d}-placed")
            placed_at = nd.now_iso()
            time.sleep(0.5)
            shadow_ref = self.screen.grab()
            # 설치가 받아들여졌는지(미리보기 테두리가 사라짐) 확인한다
            if preview_valid(shadow_ref, self.cam, *site):
                self.note("설치 클릭이 거부된 것으로 보임(미리보기가 남음)", site=site)
                self.save_grab(f"error-w{n:02d}-rejected")
                break
            front = (site[0] + SITE_FRONT[0], site[1] + SITE_FRONT[1])
            walk = cells_between(self.cam.ref(*priest), front) / PRIEST_SPEED * 1.3 + 1.0
            done = self.wait_site_complete(site, shadow_ref, walk + 10.0 + BUILD_MARGIN)
            self.cam_refresh()
            self.summary["workshops"].append({"n": n, "site": site, "placedAt": placed_at,
                                              "completeAfterSec": None if done is None else round(done + 0.5, 2)})
            self.note(f"워크샵 {n} 설치", site=site, done=done)
            self.save_grab(f"w{n:02d}-done")
            time.sleep(0.6)
        self.step("shot", name="z01-fill-final")

    def leave_and_end(self):
        """Esc → Game → Leave Mission → Main Menu 로 나가고 세션을 종료한다"""
        self.step("click", x=150, y=650, dur=100, settle=300)
        self.step("key", key="ESCAPE", dur=90, settle=700)
        self.step("click", x=108, y=9, dur=100, settle=600, name="e01-game-menu")
        self.step("click", x=160, y=111, dur=100, settle=800, name="e02-leave")
        self.step("click", x=443, y=442, dur=100, settle=1800, name="e03-main-menu")
        time.sleep(1.5)
        status = self.step("status")
        self.summary["finalWindowTitle"] = (status.get("window") or {}).get("title")
        result = self.step("end", force=False)
        self.summary["ended"] = {"closed": result.get("closed"), "recording": result.get("automaticRecording")}


def json_default(value):
    """numpy 정수·튜플 등 JSON 으로 못 쓰는 값을 일반 값으로 바꾼다"""
    if isinstance(value, (np.integer,)):
        return int(value)
    if isinstance(value, (np.bool_,)):
        return bool(value)
    return str(value)


def main():
    """명령줄 진입점: 전체 흐름을 한 번에 실행한다 (실행 중 승인 프롬프트·사람 입력 금지)"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--work", type=Path, required=True)
    # 이전 실행의 훑기 결과(tools/preview_scan_from_video.py 의 출력)를 쓰면 9분짜리 훑기를 건너뛴다
    parser.add_argument("--valid-json", type=Path, default=None)
    args = parser.parse_args()
    run = Run(args.work)
    try:
        run.boot_to_battle()
        run.build_temple()
        run.temple_second_try()
        run.hover_checks_after_build()
        if args.valid_json:
            data = json.loads(args.valid_json.read_text(encoding="utf-8"))
            valid = [(c * CELL_W, r * CELL_H) for c, r in data["valid"]]
            run.note("훑기 결과를 파일에서 읽음", valid=len(valid))
        else:
            # 워크샵을 커서에 붙여 훑기: 사제를 불러 메뉴로 워크샵을 집은 뒤 훑는다 (끝나면 첫 후보에 바로 놓아야 한다)
            priest = run.bring_priest_home()
            run.open_construct_menu(priest, "workshop", "sun")
            valid = run.scan_valid_anchors()
        plan = run.plan_packing(valid)
        run.summary["plan"] = plan
        run.note("쌓기 계획", count=len(plan))
        run.build_workshops(plan, carrying=not args.valid_json)
        run.leave_and_end()
        run.summary["ok"] = True
    except Exception as error:  # 실패해도 게임은 닫지 않고 그대로 두어 이어서 조사할 수 있게 한다
        run.summary["ok"] = False
        run.summary["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        run.summary["finished"] = nd.now_iso()
        (args.work / "summary.json").write_text(json.dumps(run.summary, ensure_ascii=False, indent=1, default=json_default), encoding="utf-8")
        run.d.close()


if __name__ == "__main__":
    main()
