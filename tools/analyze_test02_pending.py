# -*- coding: utf-8 -*-
"""
원본 게임 자동 분석 후속: 건설 지시가 대기 중일 때(공사장은 놓았고 사제가 아직 걷는 중) 메뉴·새 지시가 어떻게 되는지 (Windows 전용).

사전 조건: AGENTS.md 의 실행 허용 시스템이거나 사용자가 실행을 확인한 상태여야 한다. 실행 중에는 마우스·키보드를 건드리지 않는다.
시험 (TEST02 시험 전투에서 사제 하나로):
  A. Rain 템플을 설치하고 사제가 걷기 시작하기 전에 사제를 우클릭 → Construct → Temple 줄·Workshop 줄·Altar 줄 상태를 본다
     (건설 중인 템플도 "이미 있음"으로 쳐서 Temple 줄이 어두워지는가?)
  B. 메뉴에서 Sun 워크샵을 골라 다른 자리에 설치 → 앞선 템플 공사장이 취소·환불되는가, 둘 다 남아 순서대로 지어지는가?
  C. 완공 뒤 들고 있는 건물을 허공 우클릭으로 취소할 수 있는가?
결과는 <work>/shots/*.png, actions.jsonl, cursor_timeline.jsonl 로 판독한다. 해석은 docs/videos/auto-test02-construct-20261004.md.

사용법: python tools/analyze_test02_pending.py --work extracted/test02-pending-run1
"""
import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ns_driver as nd  # noqa: E402
import analyze_test02_construct as base  # noqa: E402

# 템플을 놓는 자리와 두 번째로 놓는 워크샵 자리 (기준 화면 좌표, analyze_test02_construct.py 와 같은 좌표계)
TEMPLE_SITE = (780, 220)
WORKSHOP_SITE = (540, 400)


def main():
    """명령줄 진입점"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    run = base.Run(args.work)
    try:
        run.boot_to_battle()
        priest = base.PRIEST_START
        # ---- A. 템플 설치 직후 메뉴 상태 ----
        run.step("click", x=150, y=650, dur=100, settle=300)                    # 선택 없음 보장
        run.open_construct_menu(priest, "temple", "rain")
        run.step("move", x=TEMPLE_SITE[0], y=TEMPLE_SITE[1], settle=300, name="a01-preview")
        run.step("click", x=TEMPLE_SITE[0], y=TEMPLE_SITE[1], dur=100, settle=0, name="a02-temple-placed")
        run.note("A: 템플 설치 클릭")
        # 사제는 설치 클릭 뒤 0.4~0.8초 동안 방향을 바꾸고 걷기 시작하므로, 그림을 찾아 지금 서 있는 자리를 우클릭한다
        time.sleep(0.15)
        found = base.find_priest_by_sprite(run.screen.grab(), (priest[0] - 70, priest[1] - 70, priest[0] + 70, priest[1] + 50))
        spot = (found[0], found[1] - 4) if found else priest
        run.note("A: 우클릭할 사제 자리", spot=spot, found=bool(found))
        run.step("click", x=spot[0], y=spot[1], button="right", dur=100, settle=350, name="a03-menu-while-pending")
        mx, my = spot
        run.step("click", x=min(mx + base.MENU_CONSTRUCT[0], base.MENU_CONSTRUCT_X_MAX), y=my + base.MENU_CONSTRUCT[1],
                 dur=100, settle=400, name="a04-construct-while-pending")
        run.step("move", x=min(mx + base.MENU_TEMPLE_ROW[0], base.MENU_ROW_X_MAX), y=my + base.MENU_TEMPLE_ROW[1],
                 settle=300, name="a05-hover-temple-row")
        run.step("click", x=min(mx + base.MENU_TEMPLE_ROW[0], base.MENU_ROW_X_MAX), y=my + base.MENU_TEMPLE_ROW[1],
                 dur=100, settle=500, name="a06-click-temple-row")
        # ---- B. 대기 중에 워크샵 설치 ----
        run.step("click", x=min(mx + base.MENU_WORKSHOP_ROW[0], base.MENU_ROW_X_MAX),
                 y=my + base.MENU_WORKSHOP_ROW[1], dur=100, settle=400, name="b01-workshop-rows")
        run.step("click", x=min(mx + base.MENU_ITEM_DX, base.MENU_ITEM_X_MAX), y=my + base.MENU_WORKSHOP_ITEM_Y["sun"],
                 dur=100, settle=300, name="b02-picked-workshop")
        run.step("move", x=WORKSHOP_SITE[0], y=WORKSHOP_SITE[1], settle=300, name="b03-preview")
        run.step("click", x=WORKSHOP_SITE[0], y=WORKSHOP_SITE[1], dur=100, settle=0, name="b04-workshop-placed")
        run.note("B: 템플 대기 중 워크샵 설치 클릭")
        # 이후 60초 동안 3초마다 화면과 SP 를 남긴다 (어느 공사장이 남는지·순서)
        for k in range(1, 21):
            time.sleep(3.0)
            run.step("shot", name=f"b10-{k:02d}")
        # ---- C. 들고 있는 건물 취소 ----
        run.step("click", x=150, y=650, dur=100, settle=300)
        bar = run.press_p_select()
        home = run.cam.scr(*base.HOME)
        run.step("click", x=home[0], y=home[1], dur=100, settle=0, name="c00-home-order")
        time.sleep(12.0)
        pos = base.find_priest_by_sprite(run.screen.grab(), (home[0] - 40, home[1] - 50, home[0] + 40, home[1] + 30))
        run.note("C: 사제 위치", pos=pos, bar=bar)
        if pos:
            run.open_construct_menu((pos[0], pos[1] - 4), "workshop", "sun")
            run.step("move", x=700, y=300, settle=400, name="c01-carrying")
            run.step("click", x=150, y=650, button="right", dur=100, settle=500, name="c02-right-click-void-cancel")
            run.step("move", x=720, y=320, settle=400, name="c03-after-cancel")
            run.step("click", x=720, y=320, dur=100, settle=600, name="c04-click-after-cancel")
        run.leave_and_end()
        run.summary["ok"] = True
    except Exception as error:  # 실패해도 게임은 닫지 않고 그대로 둔다
        run.summary["ok"] = False
        run.summary["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        run.summary["finished"] = nd.now_iso()
        (args.work / "summary.json").write_text(json.dumps(run.summary, ensure_ascii=False, indent=1, default=base.json_default), encoding="utf-8")
        run.d.close()


if __name__ == "__main__":
    main()
