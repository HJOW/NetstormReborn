"""미션 진입 덤프를 수집하고 구조 차이와 전체 L2 일치를 구별해 보고한다.

저장소 루트에서 python dotnetpj/tools/l2_compare.py --capture --mission bc5walkers76
처럼 사용한다. --capture가 없으면 기존 산출물만 읽는다. 실제 창 실행은
AGENTS.md 예외 호스트에서만 허용하며 원본 대신 extracted의 사본을 사용한다.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys


# 도구 위치로 찾은 저장소 루트이며 현재 셸 디렉터리에 의존하지 않는다.
ROOT = Path(__file__).resolve().parents[2]
# 원본 출하 캠페인 목록(offical1~5.english) 순서의 서로 다른 미션31개다.
MISSIONS = (
    *(f"tutorial{i}" for i in range(1, 7)),
    "bc1menu76", "bc2bridging76", "bc3acid_bar76", "bc4bow_rush76",
    "bc5walkers76", "bc6mows76", "bc7sun_bars76", "bc8fury76", "bc9ssun_bars76",
    "thewarbegins", "masterofwhirligigs", "savetheisland", "fragilefortune",
    "thunderingpower", "dissolvedalliance", "thenoose", "rainvsrain", "runforit",
    "guardmyback", "surrounded", "breakingthrough", "totherescue", "vicious",
    "enemyterritory", "finalconfrontation",
)
# tell.english의 DEMOVILLE 목록이다. 같은 맵의 일반 미션과 데모 시작을 별도 경로로 센다.
DEMOS = ("demostormrages", "guardmyback", "dissolvedalliance")
# AGENTS.md에 실행 확인 예외로 명시한 PC 이름이다. 다른 PC에서는 수집하지 않는다.
EXECUTION_HOSTS = {"hjow-athlon", "vm-w11-codex", "vm-debian-codex"}
# 양쪽 모두 적용하는 기본 영어·창 모드·4:3 옵션이다.
OPTIONS = "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0;currentLanguage=english"


def sha256(path):
    """파일 전체 SHA-256을 반환한다. 오라클 식별과 보호 파일 감사에 쓴다."""
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def original_hashes():
    """originals의 파일명과 해시를 함께 기록해 추가·삭제·수정을 검출한다."""
    return {str(path.relative_to(ROOT)): sha256(path)
            for path in sorted((ROOT / "originals").rglob("*")) if path.is_file()}


def read_dump(path):
    """판번호1의 실제 world1 덤프만 읽는다. 메뉴 덤프를 미션 성공으로 세지 않는다."""
    rows = [line.split("\t") for line in path.read_text(encoding="ascii").splitlines()]
    if not rows or rows[0] != ["netstorm_session_dump", "1"] or ["world", "1"] not in rows:
        raise ValueError(f"실제 미션 world1 덤프가 아님: {path}")
    return rows


def slots(rows):
    """SID를 키로 슬롯 전체 바이트를 반환하며 중복 SID를 거부한다."""
    result = {}
    # 덤프의 슬롯 행만 원문 순서로 읽는다.
    for row in rows:
        if row[0] != "slot":
            continue
        sid = int(row[1])
        raw = bytes.fromhex(row[2])
        if sid in result or len(raw) != 50:
            raise ValueError(f"중복 SID 또는 잘못된 슬롯 폭: {sid}")
        result[sid] = raw
    return result


def terrain_bounds(values):
    """97번 지형 셀의4방향 연결 군집과 bounds를 구한다. 프레임·소유자는 사용하지 않는다."""
    cells = {(round(struct.unpack_from("<f", raw, 14)[0]),
              round(struct.unpack_from("<f", raw, 18)[0]))
             for raw in values.values() if raw[10] == 97}
    result = []
    # 각 셀을 한 번만 소비하여 모든 분리 섬을 찾는다.
    while cells:
        seed = min(cells)
        cells.remove(seed)
        pending, component = [seed], [seed]
        # 현재 섬과 상하좌우로 연결된 셀을 넓혀 간다.
        while pending:
            x, y = pending.pop()
            # 모서리 접촉을 연결로 취급하지 않고4방향 이웃만 소비한다.
            for other in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if other in cells:
                    cells.remove(other)
                    pending.append(other)
                    component.append(other)
        result.append([min(x for x, _ in component), min(y for _, y in component),
                       max(x for x, _ in component), max(y for _, y in component), len(component)])
    return sorted(result)


def compare(cpp_path, dn_path):
    """전체 행 일치와 SID/타입/소유자/프레임별 일치 수를 함께 반환한다."""
    cpp_rows, dn_rows = read_dump(cpp_path), read_dump(dn_path)
    cpp, dn = slots(cpp_rows), slots(dn_rows)
    common = cpp.keys() & dn.keys()
    graph_cpp = {row[1]: row[2:] for row in cpp_rows if row[0] == "graph"}
    graph_dn = {row[1]: row[2:] for row in dn_rows if row[0] == "graph"}
    return {
        "l2_equal": cpp_rows == dn_rows,
        "cpp_slots": len(cpp), "dn_slots": len(dn), "common_slots": len(common),
        "cpp_only": len(cpp.keys() - dn.keys()), "dn_only": len(dn.keys() - cpp.keys()),
        "type_equal": sum(cpp[sid][10] == dn[sid][10] for sid in common),
        "owner_equal": sum(cpp[sid][34] == dn[sid][34] for sid in common),
        "frame_equal": sum(cpp[sid][36:40] == dn[sid][36:40] for sid in common),
        "slot_equal": sum(cpp[sid] == dn[sid] for sid in common),
        "graph_equal": sum(graph_cpp.get(key) == value for key, value in graph_dn.items()),
        "graph_rows": len(graph_dn), "terrain_bounds": terrain_bounds(dn),
        "cpp_random": next(row[1:] for row in cpp_rows if row[0] == "random"),
        "dn_random": next(row[1:] for row in dn_rows if row[0] == "random"),
    }


def capture(engine, mission, demo, directory, seed):
    """한 엔진을 새 사용자 루트로 실행하고90프레임의 관측을 남긴다. 공격 명령은 보내지 않는다."""
    output = directory / engine
    output.mkdir(parents=True, exist_ok=True)
    dump = output / "f090.tsv"
    if dump.exists():
        raise FileExistsError(f"이전 관측 덮어쓰기 거부: {dump}")
    user = output / "user"
    user.mkdir()
    steps = output / "steps.tsv"
    prefix = "5\tclick-if\tPlay Mission\n30\tclick-if\tPlay Mission\n60\tclick-if\tPlay Mission\n"
    if demo:
        prefix = f"5\tscript-command\tStartDemo,{mission}\n"
    steps.write_text((prefix if engine == "cpp" else "") + f"90\tsession-dump\t{dump.as_posix()}\n", encoding="utf-8")
    executable = ROOT / ("cpppj/build/bin/Release/Netstorm.exe" if engine == "cpp" else
                         "dotnetpj/src/Netstorm.Client.Host/bin/Release/net10.0/Netstorm.Client.Host.exe")
    data = ROOT / "extracted/dotnet-oracle-data"
    args = [str(executable)]
    if engine == "cpp":
        args += ["--run", str(data)]
    if not demo:
        args += ["--mission", mission]
    if engine == "dn":
        args += ["--handle", "Youpson"]
    args += ["--window", "--mute-audio", "--frames", "120", "--fixed-frame-ms", "25",
             "--fixed-unix-seconds", str(seed), "--set", OPTIONS, "--ui-script", str(steps),
             "--ui-report", str(output / "report.tsv")]
    env = dict(os.environ, NETSTORM_USER_DATA=str(user), NETSTORM_DATA=str(data))
    metadata = {"argv": args, "exe_sha256": sha256(executable), "host": socket.gethostname()}
    # stdout/stderr는 실행별 파일에 보존하여 자동 수집 실패의 원인을 확인할 수 있게 한다.
    with (output / "console.txt").open("wb") as log:
        try:
            process = subprocess.run(args, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=60)
            metadata["exit_code"] = process.returncode
            if process.returncode:
                raise RuntimeError(f"{engine} 종료 코드 {process.returncode}: {mission}")
            read_dump(dump)
        finally:
            (output / "run.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8")
    return dump


def main():
    """지정한 경로를 순서대로 수집·대조하고 미완료를 숨기지 않는JSON 요약을 쓴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", action="store_true", help="예외PC에서 실제 게임 창을 실행한다")
    parser.add_argument("--mission", action="append", choices=MISSIONS, help="생략하면34경로 모두")
    parser.add_argument("--out", type=Path, default=ROOT / "extracted/dotnet-l2-0499")
    parser.add_argument("--seed", type=int, default=1760000000)
    args = parser.parse_args()
    if args.capture and socket.gethostname().lower() not in EXECUTION_HOSTS:
        parser.error("AGENTS.md 실행 확인 예외PC에서만 자동 수집할 수 있습니다")
    before = original_hashes() if args.capture else None
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    routes = [(key, key, False) for key in (args.mission or MISSIONS)]
    if not args.mission:
        routes += [("demo-" + key, key, True) for key in DEMOS]
    results = {}
    try:
        # 두 실행이 서로의 창·옵션에 간섭하지 않도록 경로와 엔진을 순차 실행한다.
        for route, mission, demo in routes:
            directory = args.out / route
            try:
                # 데모는 새 호스트의 진입점이 아직 없으므로 cpp 기준선만 수집한다.
                for engine in (("cpp",) if demo else ("cpp", "dn")):
                    if args.capture and not (directory / engine / "f090.tsv").exists():
                        capture(engine, mission, demo, directory, args.seed)
                if demo:
                    read_dump(directory / "cpp/f090.tsv")
                    result = {"l2_equal": False, "pending": "새 호스트 StartDemo 진입 미연결"}
                else:
                    result = compare(directory / "cpp/f090.tsv", directory / "dn/f090.tsv")
                results[route] = result
                print(route, json.dumps(result, ensure_ascii=False), flush=True)
            except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
                results[route] = {"l2_equal": False, "error": str(error)}
                print(route, str(error), flush=True)
            (args.out / "summary.json").write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    finally:
        if before is not None:
            after = original_hashes()
            audit = {"files_before": len(before), "files_after": len(after),
                     "unchanged": before == after, "hashes_before": before, "hashes_after": after}
            (args.out / "originals-audit.json").write_text(json.dumps(audit, indent=2), encoding="utf-8")
            if before != after:
                raise RuntimeError("originals 파일명 또는 해시가 실행 전후 달라졌습니다")
    return 0 if all(row["l2_equal"] for row in results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
