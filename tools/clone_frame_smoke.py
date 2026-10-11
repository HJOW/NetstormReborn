# -*- coding: utf-8 -*-
"""격리된 실제 클론 창에서 정지 중 명령·시각 보존과 재개 뒤 이동을 검사한다."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from clone_world_order_smoke import make_map, PRIEST_CELL
from fort import TypeCatalog


def run_case(root, output, data, language, width, height):
    """한 화면비·언어에서 시간·스캔·표시 좌표를 저장하고 정지 조작 전후에 정확히 비교한다."""
    folder = output / f"{language}-{width}x{height}"
    settings = folder / "settings"
    settings.mkdir(parents=True, exist_ok=True)
    (settings / "settings.json").write_text(json.dumps({"WindowWidth": width, "WindowHeight": height,
        "ViewHeight": 768, "PlayMusic": False, "WindNoise": False}), encoding="utf-8")
    commands = f"""
wait 30; assert battle; key LeftShift+F9; wait 10; assert-detail simulation=paused;
remember-detail tick; remember-detail frame-seconds; remember-detail scan-cursor;
click-center 8,-4; assert-detail selected=priest;
assert-unchanged-detail tick; assert-unchanged-detail frame-seconds; assert-unchanged-detail scan-cursor;
click-center 16,78; assert-detail selected=none; assert-detail priest:-:moving;
remember-detail world-order; capture {(folder / 'paused-before.png').as_posix()};
wait 90; assert-unchanged-detail tick; assert-unchanged-detail frame-seconds;
assert-unchanged-detail scan-cursor; assert-unchanged-detail world-order;
click-center 8,-4; assert-detail selected=priest;
assert-unchanged-detail tick; assert-unchanged-detail frame-seconds; assert-unchanged-detail scan-cursor;
capture {(folder / 'paused-after.png').as_posix()};
key LeftShift+F9; assert-detail simulation=running;
wait-detail priest:E:idle; assert-detail priest@104/121;
capture {(folder / 'arrived.png').as_posix()}; quit;
"""
    script = folder / "commands.txt"
    script.write_text(commands, encoding="utf-8")
    env = dict(os.environ, NETSTORM_DATA=str(data), NETSTORM_SETTINGS_DIR=str(settings))
    exe = root / "dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe"
    args = [str(exe), "--test-battle", "framepause", "--language", language, "--no-sound", "--no-music",
        "--script", "rules 0; combat 0", "--ui-script-file", str(script)]
    with (folder / "stdout.log").open("wb") as stdout, (folder / "stderr.log").open("wb") as stderr:
        subprocess.run(args, cwd=root, env=env, stdout=stdout, stderr=stderr, timeout=60, check=True)
    checks = (folder / "stdout.log").read_bytes().count(b"UI PASS:")
    if checks != 19:
        raise AssertionError(f"{folder.name}: expected 19 checks, got {checks}")
    print(f"FRAME PASS: {folder.name} ({checks} checks)")
    return {"case": folder.name, "checks": checks}


def main():
    """원본·사용자 설정을 보존하는 복사본 맵과 네 창을 만들고 결과를 UTF-8 JSON에 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="extracted/screens/dotnet-frame-20261011/pause")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = (root / args.output).resolve()
    if root / "extracted" not in output.parents:
        raise ValueError("검사 출력은 extracted 하위 폴더여야 합니다.")
    source = root / "assets/game-data"
    data = output / "data"
    shutil.copytree(source, data, dirs_exist_ok=True)
    catalog = TypeCatalog(str(source))
    make_map(data, "framepause", [("priest", *PRIEST_CELL, 1), ("windBattery", 111, 107, 1),
        ("windVortex", 122, 107, 1)], catalog)
    results = []
    # 클론 창은 순서대로 실행하여 장치·창 입력·전체화면 상태의 간섭을 피한다.
    for language, width, height in [("english", 1024, 768), ("korean", 1024, 768),
        ("english", 1280, 720), ("korean", 1280, 800)]:
        results.append(run_case(root, output, data, language, width, height))
    (output / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
