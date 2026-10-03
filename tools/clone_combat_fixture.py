# -*- coding: utf-8 -*-
"""클론 전투 화면 검사에 쓸 격리 데이터와 작은 맵을 만든다. 원본·배포 데이터는 읽기만 한다."""
import argparse
from pathlib import Path
import shutil
import struct

from fort import (TypeCatalog, F1_SAVE_FRAME, F1_SAVE_QA, F1_SAVE_QB, F1_CONTAINER,
                  F2_FACTORY, F2_OWNER_MASK, SECTION_NAMES, parse_fort)
from shp import TYPE_LOAD_ORDER, load_type_texts


def object_bytes(catalog, name, x, y, owner):
    """현재 타입 번호 체계와 원본 선택 필드 순서로 오브젝트 하나를 저장한다."""
    first, second = catalog.flags[name]
    data = bytearray([(x % 16) << 4 | y % 16, 70 + TYPE_LOAD_ORDER.index(name)])
    if first & F1_SAVE_FRAME:
        data.append(0)
    if first & F1_SAVE_QA:
        data.append(0)
    if first & F1_SAVE_QB:
        data.extend(b"\0\0")
    if second & F2_FACTORY:
        data.append(0)
    if second & F2_OWNER_MASK:
        data.append(owner)
    if first & F1_CONTAINER:
        data.append(0)
    return bytes(data)


def main():
    """복사한 자원에 검사 맵과 파괴 그림 확인용 낮은 체력의 신전을 추가한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default="assets/game-data")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    source, output = Path(args.source).resolve(), Path(args.output).resolve()
    if source == output or source in output.parents:
        raise ValueError("검사 출력은 배포 데이터 바깥의 별도 폴더여야 합니다.")
    shutil.copytree(source, output, dirs_exist_ok=True)
    catalog = TypeCatalog(str(source))
    objects = [
        ("sunCannon", 80, 80, 1), ("windVortex", 100, 82, 2),
        ("sunArcher", 80, 100, 1), ("sunBlocker", 87, 100, 2),
        ("windArcher", 100, 98, 1), ("sunBlocker", 99, 86, 2),
    ]
    chunks = {}
    # 월드 청크의 y·x 순서로 시설을 묶는다.
    for name, x, y, owner in objects:
        chunks.setdefault(y // 16 * 16 + x // 16, []).append(object_bytes(catalog, name, x, y, owner))
    chaff = bytearray([2])
    # 빈 청크도 남겨 저장 인덱스가 정확한 월드 좌표를 나타내게 한다.
    for index in range(256):
        items = chunks.get(index, [])
        chaff.extend(b"c" + struct.pack("<H", len(items)) + b"".join(items))
    sections = {"Chaff": bytes(chaff), "Territory": bytes(120)}
    data = bytearray(b"F\0")
    # 원본의 고정 섹션 순서와 길이 접두 규칙으로 맵을 기록한다.
    for name in SECTION_NAMES:
        section = sections.get(name, b"")
        data.extend(struct.pack("<H", len(section) + 2) + section)
    parse_fort(bytes(data), catalog)
    (output / "d" / "fancombat.fort").write_bytes(data)
    text = load_type_texts(str(source))["windvortex"]
    # 실제 전투 피해 수치의 검증과 분리해 신전 폭발을 짧은 시간에 화면에서 확인한다.
    end = text.index("}")
    text = text[:end] + "\nmaxHitPoints = 50;\n" + text[end:]
    (output / "d" / "windVortex.type").write_text(text, encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
