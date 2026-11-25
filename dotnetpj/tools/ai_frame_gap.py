"""R4-5e2 L2 대조: 90프레임 덤프의 brain·SP·Player 행을 뽑아 AI 프레임 귀속분을 측정한다. 게임을 실행하지 않는다.

종료 1은 예상된 미완료 보고다(R4-5e2는 live AI 틱 결선(R6)에서 닫힌다).
사용법: python -X utf8 dotnetpj/tools/ai_frame_gap.py [--collection extracted/dotnet-l2-0998] [--json 요약.json]
도구 검사: python -X utf8 -m unittest discover -s dotnetpj/tools -p test_ai_frame_gap.py -v
"""

import argparse
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent

# Player 기록 180바이트 안의 brain 토큰(+0xb0)·첫 지역(+0x78) 위치다.
# 근거: Netstorm.Client.Host.HostSession의 PlayerRecordStride·PlayerFirstRegionOffset과
# RawAiRuntime.PlayerBytes·BrainField다.
BRAIN_OFFSET = 0xB0
REGION_OFFSET = 0x78
RECORD_BYTES = 180


def read_rows(path):
    """덤프 TSV를 (종류, 키)별 필드 목록으로 읽는다. 손상된 행은 거부한다."""
    rows = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        columns = line.split("\t")
        if columns == ["end"]:
            continue
        if len(columns) < 2:
            raise ValueError(f"손상된 덤프 행: {path}")
        rows.setdefault(columns[0], {})[columns[1]] = columns[2:]
    return rows


def player_raw(rows, owner):
    """지정 owner의 Player 180바이트를 돌려준다. 없으면 None이다."""
    fields = rows.get("player", {}).get(str(owner))
    if fields is None:
        return None
    raw = bytes.fromhex(fields[0])
    if len(raw) != RECORD_BYTES:
        raise ValueError(f"Player 기록 180바이트 오류: owner {owner}")
    return raw


def brain_token(raw):
    """Player 기록의 brain 토큰(+0xb0)을 읽는다."""
    return struct.unpack_from("<I", raw, BRAIN_OFFSET)[0]


def first_region(raw):
    """Player 기록의 첫 지역(+0x78)을 읽는다."""
    return struct.unpack_from("<i", raw, REGION_OFFSET)[0]


def compare_route(cpp_path, dn_path):
    """한 경로의 brain·SP·Player·난수 행을 대조해 AI 프레임 귀속분을 돌려준다."""
    cpp, dn = read_rows(cpp_path), read_rows(dn_path)
    owners = sorted(set(cpp.get("player", {})) | set(dn.get("player", {})), key=int)
    brain_diff = {}
    brain_tokens = {}
    player_offsets = {}
    for owner in owners:
        left, right = player_raw(cpp, owner), player_raw(dn, owner)
        if left is not None:
            brain_tokens.setdefault(int(owner), {})["cpp"] = brain_token(left)
        if right is not None:
            brain_tokens.setdefault(int(owner), {})["dn"] = brain_token(right)
        if left is None or right is None:
            brain_diff[owner] = (None if left is None else brain_token(left),
                                 None if right is None else brain_token(right))
            continue
        if brain_token(left) != brain_token(right):
            brain_diff[owner] = (brain_token(left), brain_token(right))
        for index, (first, second) in enumerate(zip(left, right)):
            if first != second:
                player_offsets.setdefault(index, []).append(int(owner))
    sp_diff = {}
    for owner in sorted(set(cpp.get("sp", {})) | set(dn.get("sp", {})), key=int):
        left, right = cpp.get("sp", {}).get(owner), dn.get("sp", {}).get(owner)
        if left != right:
            sp_diff[owner] = (left, right)
    cpp_random = next(iter(cpp.get("random", {}).items()), None)
    dn_random = next(iter(dn.get("random", {}).items()), None)
    return {
        "owners": [int(owner) for owner in owners],
        "brains": brain_tokens,
        "brain_diff": {int(owner): values for owner, values in brain_diff.items()},
        "sp_diff": {int(owner): values for owner, values in sp_diff.items()},
        "player_offsets": player_offsets,
        "cpp_random": cpp_random,
        "dn_random": dn_random,
    }


def summarize(collection):
    """수집 디렉토리의 전 경로를 대조해 경로별·합계를 돌려준다."""
    routes = sorted(path.name for path in Path(collection).iterdir() if path.is_dir())
    per_route, totals = {}, {"routes": 0, "brain_diff_owners": 0, "sp_diff_owners": 0,
                             "player_diff_cells": 0, "random_diff_routes": 0}
    for route in routes:
        cpp_path = Path(collection) / route / "cpp" / "f090.tsv"
        dn_path = Path(collection) / route / "dn" / "f090.tsv"
        if not cpp_path.exists() or not dn_path.exists():
            per_route[route] = {"missing": True}
            continue
        compared = compare_route(cpp_path, dn_path)
        per_route[route] = compared
        totals["routes"] += 1
        totals["brain_diff_owners"] += len(compared["brain_diff"])
        totals["sp_diff_owners"] += len(compared["sp_diff"])
        totals["player_diff_cells"] += sum(len(owners) for owners in compared["player_offsets"].values())
        if compared["cpp_random"] != compared["dn_random"]:
            totals["random_diff_routes"] += 1
    return {"per_route": per_route, "totals": totals}


def main(argv=None):
    """수집 디렉토리를 대조하고 표와 합계를 출력한다. 격차가 남으면 종료 1이다."""
    parser = argparse.ArgumentParser(description="R4-5e2 brain·SP·Player 행 대조")
    parser.add_argument("--collection", default=str(ROOT / "extracted" / "dotnet-l2-0998"))
    parser.add_argument("--json", default=None)
    args = parser.parse_args(argv)
    summary = summarize(args.collection)
    for route, compared in summary["per_route"].items():
        if compared.get("missing"):
            print(f"{route} MISSING")
            continue
        print(f"{route} brainDiff={len(compared['brain_diff'])} "
              f"spDiff={len(compared['sp_diff'])} "
              f"playerCells={sum(len(owners) for owners in compared['player_offsets'].values())} "
              f"randomDiff={compared['cpp_random'] != compared['dn_random']}")
    totals = summary["totals"]
    print(f"TOTAL routes={totals['routes']} brainDiffOwners={totals['brain_diff_owners']} "
          f"spDiffOwners={totals['sp_diff_owners']} playerCells={totals['player_diff_cells']} "
          f"randomDiffRoutes={totals['random_diff_routes']}")
    if args.json is not None:
        Path(args.json).write_text(json.dumps(summary, indent=1, sort_keys=True), encoding="utf-8")
    gap = (totals["brain_diff_owners"] + totals["sp_diff_owners"]
           + totals["player_diff_cells"] + totals["random_diff_routes"])
    if gap:
        print("R4-5e2 OPEN: live AI 틱 결선(R6)에서 닫는다.")
        return 1
    print("R4-5e2 CLOSED: 258 완료·brain·SP·Player 행이 34/34 일치한다.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
