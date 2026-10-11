# -*- coding: utf-8 -*-
"""실제 클론 창의 긴 프레임·갱신/그리기 짝·정지 시각을 격리된 맵에서 검사한다."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from clone_world_order_smoke import make_map, PRIEST_CELL
from fort import TypeCatalog


def run_case(root, output, data, language, width, height, max_fps):
    """정상·700ms 지연·정지 지연을 실행하고 실제 루프 시간과 세션 상태를 로그에서 대조한다."""
    folder = output / f"{language}-{width}x{height}-fps{max_fps}"
    settings = folder / "settings"
    settings.mkdir(parents=True, exist_ok=True)
    (settings / "settings.json").write_text(json.dumps({"WindowWidth": width, "WindowHeight": height,
        "ViewHeight": 768, "PlayMusic": False, "WindNoise": False}), encoding="utf-8")
    commands = f"""
wait 30; assert battle; assert-detail loop-fixed=false; assert-detail loop-updates-per-draw=1;
trace-detail; stall 700; trace-detail; assert-detail loop-updates-per-draw=1;
assert-detail loop-long-frame=true; trace-detail;
key LeftShift+F9; wait 10; assert-detail simulation=paused; assert-detail loop-paused=true;
remember-detail tick; remember-detail frame-seconds; remember-detail scan-cursor; remember-detail world-order;
trace-detail; stall 700; trace-detail; assert-detail loop-updates-per-draw=1;
assert-unchanged-detail tick; assert-unchanged-detail frame-seconds;
assert-unchanged-detail scan-cursor; assert-unchanged-detail world-order;
capture {(folder / 'paused.png').as_posix()};
key LeftShift+F9; assert-detail simulation=running; assert-detail loop-paused=false;
click-center 8,-4; assert-detail selected=priest; click-center 16,78;
wait-detail priest:E:idle; assert-detail priest@104/121;
capture {(folder / 'arrived.png').as_posix()}; quit;
"""
    script = folder / "commands.txt"
    script.write_text(commands, encoding="utf-8")
    env = dict(os.environ, NETSTORM_DATA=str(data), NETSTORM_SETTINGS_DIR=str(settings))
    exe = root / "dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe"
    args = [str(exe), "--test-battle", "frameloop", "--language", language, "--no-sound", "--no-music",
        "--max-fps", str(max_fps), "--script", "rules 0; combat 0", "--ui-script-file", str(script)]
    with (folder / "stdout.log").open("wb") as stdout, (folder / "stderr.log").open("wb") as stderr:
        subprocess.run(args, cwd=root, env=env, stdout=stdout, stderr=stderr, timeout=60, check=True)
    log = (folder / "stdout.log").read_text(encoding="utf-8")
    # 명시적으로 찍은 다섯 화면의 숫자·세션 상태를 키별로 읽는다.
    traces = [dict(part.split("=", 1) for part in line[len("UI DETAIL: "):].split(";") if "=" in part)
        for line in log.splitlines() if line.startswith("UI DETAIL: ")]
    if len(traces) != 5:
        raise AssertionError(f"{folder.name}: expected 5 traces, got {len(traces)}")
    # 지연 바로 다음 실제 프레임을 읽는다. 최대값이나 평균을 쓰면 다른 초기 지연으로 잘못 통과할 수 있다.
    for index in (1, 4):
        elapsed = float(traces[index]["loop-delta-ms"])
        if elapsed < 700 or traces[index]["loop-updates-per-draw"] != "1":
            raise AssertionError(f"{folder.name}: lost elapsed time or repeated update: {traces[index]}")
    frame_delta = float(traces[2]["frame-seconds"]) - float(traces[0]["frame-seconds"])
    if frame_delta < 0.7:
        raise AssertionError(f"{folder.name}: session lost real frame time: {frame_delta}")
    # 정지 중에도 실제 화면 시계는 흐르지만 세션·스캔·표시 위치는 바뀌지 않아야 한다.
    for key in ("tick", "frame-seconds", "scan-cursor", "world-order"):
        if traces[3][key] != traces[4][key]:
            raise AssertionError(f"{folder.name}: paused {key} changed")
    checks = log.count("UI PASS:")
    if checks != 17:
        raise AssertionError(f"{folder.name}: expected 17 checks, got {checks}")
    result = {"case": folder.name, "ui_checks": checks, "external_checks": 8,
        "running_stall_ms": float(traces[1]["loop-delta-ms"]),
        "paused_stall_ms": float(traces[4]["loop-delta-ms"]), "session_delta_seconds": frame_delta}
    print(f"LOOP PASS: {folder.name} ({checks} UI + 8 external checks)")
    return result


def main():
    """원본·사용자 설정을 건드리지 않는 복사본 데이터와 네 언어/화면비/상한 조합을 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="extracted/screens/dotnet-loop-20261012")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = (root / args.output).resolve()
    if root / "extracted" not in output.parents:
        raise ValueError("검사 출력은 extracted 하위 폴더여야 합니다.")
    source = root / "assets/game-data"
    data = output / "data"
    shutil.copytree(source, data, dirs_exist_ok=True)
    make_map(data, "frameloop", [("priest", *PRIEST_CELL, 1), ("windBattery", 111, 107, 1),
        ("windVortex", 122, 107, 1)], TypeCatalog(str(source)))
    results = []
    # 장치·입력 간섭을 피하도록 실제 클론 창은 순서대로 실행한다.
    for language, width, height, max_fps in [("english", 1024, 768, 75), ("korean", 1024, 768, 75),
        ("english", 1280, 720, 30), ("korean", 1280, 800, 0)]:
        results.append(run_case(root, output, data, language, width, height, max_fps))
    (output / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
