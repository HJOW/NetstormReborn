# -*- coding: utf-8 -*-
"""격리된 클론 창에서 저장·신규 객체의 겹침 순서와 실제 클릭 대상을 PNG까지 검사한다."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess

from PIL import Image
from clone_combat_fixture import object_bytes
from fort import TypeCatalog, SECTION_NAMES, parse_fort
from shp import TYPE_LOAD_ORDER, load_type_texts, read_blocks, read_clusters, frame_to_image, read_palette

# 원본 칸 크기와 시작 사제 기준점. 모든 검사 맵은 같은 사제 위치로 카메라를 맞춘다.
CELL_PIXELS = (16, 11)
PRIEST_CELL = (104, 115)
PRIEST_SCREEN = (528, 396)
# 앞뒤 객체의 발자국은 겹치지 않지만 높은 본체 그림은 겹친다.
FRONT = ("sunBlocker", 112, 116, 1)
BACK = ("windBlocker", 112, 113, 1)


def make_map(data, name, objects, catalog):
    """한 섬의 네 청크에 저장 객체를 넣고 최소한의 미션 머리로 실제 플레이 입력을 켠다."""
    section = bytearray([2])
    # 모양 7의 최종 월드 청크 순서는 y 다음 x이며 저장 니블은 청크 내부 x 다음 y다.
    for cx, cy in [(6, 6), (7, 6), (6, 7), (7, 7)]:
        # 현재 청크에 속한 객체만 원본 직렬화 순서로 묶는다.
        records = [object_bytes(catalog, *obj) for obj in objects if (obj[1] // 16, obj[2] // 16) == (cx, cy)]
        section.extend(b"c" + struct.pack("<H", len(records)) + b"".join(records))
    sections = {"Territory": bytes([7, 1, 0x55, 16, 0, 0]) + bytes(114), "Terr00": bytes(section)}
    fort = bytearray(b"F\0")
    # 원본 고정 섹션 순서와 길이 접두를 유지한다.
    for key in SECTION_NAMES:
        content = sections.get(key, b"")
        fort.extend(struct.pack("<H", len(content) + 2) + content)
    parse_fort(bytes(fort), catalog)
    (data / "d" / f"{name}.fort").write_bytes(fort)
    (data / "d" / f"{name}.english").write_text(
        '[Header]\nmissionType="Mission"\nmyStartMoney=50000\nmyTech="all"\nmoreGeysers=0\n'
        f'title="{name}"\n[Briefing]\n$Title="World order"\n$Text="Rendering and picking check"\n', encoding="utf-8")


def sprite_pixels(source, obj):
    """원본 RLE와 타입의 기본 프레임·hotFootRatio로 불투명 본체의 화면 좌표를 독립 계산한다."""
    name, x, y, _ = obj
    text = load_type_texts(str(source))[name.lower()]
    clusters = read_clusters(text)
    default = 0
    # default 표시는 마지막 일치를 사용한다.
    for index, (_, flags, _) in enumerate(clusters):
        if "default" in flags.lower().split():
            default = index
    import re
    hot_x = re.search(r"hotFootRatioX\s*=\s*([\d.]+)", text, re.IGNORECASE)
    hot_y = re.search(r"hotFootRatioY\s*=\s*([\d.]+)", text, re.IGNORECASE)
    shift_x = int(float(hot_x[1]) * CELL_PIXELS[0]) if hot_x else 0
    shift_y = int(float(hot_y[1]) * CELL_PIXELS[1]) if hot_y else 0
    shape_data = (source / "d" / "_shapes.shp").read_bytes()
    frame = read_blocks(shape_data)[TYPE_LOAD_ORDER.index(name)].frames[default]
    image = frame_to_image(shape_data, frame, read_palette(str(source / "d" / "GIFCLOUD.COL")))
    left = int(PRIEST_SCREEN[0] + (x - PRIEST_CELL[0]) * CELL_PIXELS[0] - shift_x + frame.rect[0])
    top = int(PRIEST_SCREEN[1] + (y - PRIEST_CELL[1]) * CELL_PIXELS[1] - shift_y + frame.rect[1])
    # 본체의 각 행·열에서 불투명 화소만 모아 그림자나 바탕의 우연한 일치를 제외한다.
    return {(left + px, top + py) for py in range(image.height) for px in range(image.width)
            if image.getpixel((px, py))[3] != 0}


def run_case(root, output, data, name, setup, click, expected, width, height, label=None):
    """사용자 설정과 입력을 건드리지 않고 클론 전용 입력으로 정지 화면·선택·숨김을 확인한다."""
    folder = output / (label or name)
    settings = folder / "settings"
    settings.mkdir(parents=True, exist_ok=True)
    (settings / "settings.json").write_text(json.dumps({"WindowWidth": width, "WindowHeight": height,
        "ViewHeight": 768, "PlayMusic": False, "WindNoise": False}), encoding="utf-8")
    capture = (folder / "scene.png").as_posix()
    commands = f'wait 30; assert battle; key LeftShift+F9; wait 10; capture {capture};'
    if name == "ordermoving":
        # 저장 사제가 새 골렘보다 뒤에 있다가 남쪽으로 이동하면 표시 순서가 뒤집혀야 한다.
        commands += 'assert-detail world-order=windbattery@111/107,windvortex@122/107,priest@104/115,sunwalker@104/119;'
        commands += 'click-center 8,-4; assert-detail selected=priest; key LeftShift+F9; click-center 16,78;'
        commands += 'wait-detail sunwalker@104/119,priest@104/121; key LeftShift+F9; wait 10;'
        commands += f'capture {(folder / "moved.png").as_posix()}; click-center 8,62; assert-detail selected=priest;'
    elif click:
        # 논리 화면 중심 기준 입력은 4:3·16:9·16:10에서 같은 월드 화소를 누른다.
        commands += f'click-center {click[0] - 512},{click[1] - 384}; assert-detail selected={expected};'
        commands += 'key F2; wait-detail world-order=priest@104/115; assert-detail-not sunblocker@; assert-detail-not windblocker@; key F2;'
        commands += 'wait-detail world-order=windbattery@111/107,windvortex@122/107,windblocker@112/113,priest@104/115,sunblocker@112/116;'
    commands += 'quit;'
    (folder / "commands.txt").write_text(commands, encoding="utf-8")
    import os
    env = dict(os.environ, NETSTORM_DATA=str(data), NETSTORM_SETTINGS_DIR=str(settings))
    exe = root / "dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe"
    args = [str(exe), "--test-battle", name, "--language", "english", "--no-sound", "--no-music",
            "--script", "rules 0; combat 0; " + setup, "--ui-script-file", str(folder / "commands.txt")]
    with (folder / "stdout.log").open("wb") as stdout, (folder / "stderr.log").open("wb") as stderr:
        subprocess.run(args, cwd=root, env=env, stdout=stdout, stderr=stderr, timeout=60, check=True)
    return Image.open(folder / "scene.png").convert("RGB")


def main():
    """저장 앞/신규 뒤·저장 뒤/신규 앞·신규 역순 생성의 렌더링 결과를 단독 앞 객체와 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="extracted/screens/dotnet-world-20261011/order")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = (root / args.output).resolve()
    # 검사 출력은 무시되는 extracted 아래에만 생성하여 원본·배포 자료를 보호한다.
    if root / "extracted" not in output.parents:
        raise ValueError("검사 출력은 extracted 하위 폴더여야 합니다.")
    source = root / "assets/game-data"
    data = output / "data"
    shutil.copytree(source, data, dirs_exist_ok=True)
    catalog = TypeCatalog(str(source))
    base = [("priest", *PRIEST_CELL, 1), ("windBattery", 111, 107, 1), ("windVortex", 122, 107, 1)]
    cases = [("orderfront", [FRONT], ""), ("orderstored", [FRONT], "place windBlocker 112,113"),
             ("orderplaced", [BACK], "place sunBlocker 112,116"),
             ("orderreverse", [], "place sunBlocker 112,116; place windBlocker 112,113")]
    # 모든 맵은 동일한 지면과 카메라를 사용하고 저장/신규 여부만 달리한다.
    for name, saved, _ in cases:
        make_map(data, name, base + saved, catalog)
    make_map(data, "ordermoving", base, catalog)
    overlap = sorted(sprite_pixels(source, FRONT) & sprite_pixels(source, BACK))
    if len(overlap) < 20:
        raise AssertionError(f"본체 겹침이 너무 작습니다: {len(overlap)}")
    click = overlap[len(overlap) // 2]
    reference = run_case(root, output, data, "orderfront", "", None, "", 1024, 768)
    results = []
    # 저장 여부와 역순 생성이 화면의 앞뒤 관계를 바꾸지 않아야 한다.
    for name, _, setup in cases[1:]:
        scene = run_case(root, output, data, name, setup, click, "sunblocker", 1024, 768)
        # 겹친 모든 본체 화소를 앞 객체만 있는 독립 화면과 비교한다.
        different = sum(scene.getpixel(point) != reference.getpixel(point) for point in overlap)
        if different:
            raise AssertionError(f"{name}: 앞 객체의 겹친 본체 {different}/{len(overlap)} 화소가 바뀌었습니다.")
        results.append({"case": name, "overlapPixels": len(overlap), "click": click, "differentPixels": different})
        print(f"WORLD ORDER PASS: {name} ({len(overlap)} pixels, front picked)")
    # 와이드 두 화면비에서도 같은 논리 화소로 앞 객체를 선택하고 숨김·복귀 순서를 검사한다.
    for label, width, height in [("order-16x9", 1280, 720), ("order-16x10", 1280, 800)]:
        run_case(root, output, data, "orderreverse", cases[-1][2], click, "sunblocker", width, height, label)
        results.append({"case": label, "click": click, "display": [width, height]})
        print(f"WORLD ORDER PASS: {label} (front picked, hidden and restored)")
    run_case(root, output, data, "ordermoving", "place sunWalker 104,119", None, "", 1024, 768)
    results.append({"case": "ordermoving", "before": "priest, sunwalker", "after": "sunwalker, priest"})
    print("WORLD ORDER PASS: ordermoving (current position reorders saved and new mobiles)")
    (output / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
