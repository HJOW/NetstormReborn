#!/usr/bin/env python3
"""기존 전체 복원 출처와 새 실제 월드 소비자 연결을 새 native 0으로 감사한다."""
from __future__ import annotations

import argparse
import hashlib
import json
import socket
import subprocess
from pathlib import Path

# 저장소 경로와 완료된 단위09 커밋이다. 공통 인수인계/CMake는 이 도구가 변경하지 않는다.
ROOT = Path(__file__).resolve().parents[1]
BASELINE = "5b10ce8"
REPORT = ROOT / "cpppj/recovery-normalterrain-world-binding-evidence.json"
# 새 소비자와 검사만 현재 소스로 고정한다. 기존 모듈은 아래 전체 원문으로 별도 보존한다.
CPP_FILES = ["cpppj/src/client/RawNormalTerrainWorldBinding.h", "cpppj/src/client/RawNormalTerrainWorldBinding.cpp",
             "cpppj/tests/NormalTerrainWorldBindingTests.cpp"]
# 호출한 완료 모듈의 전체 헤더/몸체와 실제 Constructor 표를 기준 커밋에 연결한다.
DEPENDENCIES = [f"cpppj/src/{folder}/{name}.{suffix}" for folder, names in [
    ("client", ["RawSurfaceWorld", "RawTerrainWorldBinding"]),
    ("o", ["RawTerrainContext", "RawTerrainGrowth", "RawTerrainStamp", "RawFrameChoose", "RawFarmCluster",
           "RawFortRandomPlacement", "RawIslandRuntime", "ChunkMap", "CanonDecoder", "RawCanonPlacementPipeline",
           "RawCarrierPostPop", "SquidFactory", "SquidReward", "SquidOwner", "SquidPop", "SquidUnpop", "SquidPostPop"])]
    for name in names for suffix in ["h", "cpp"]]
DEPENDENCIES += ["cpppj/src/o/TypeConstructors.inc", "cpppj/src/o/DerivedConstructors.inc",
                 "cpppj/src/o/CanonDecoderTables.inc"]
# 앞 단계의 전체 원본 관찰은 출처로 보존하며 이번 고유 입력에 합산하지 않는다.
NATIVE_REUSE = [("terraincontext", 224), ("terraingrowth", 108), ("terrainstamp", 1560),
                ("farmcluster", 208), ("islandruntime", 2018)]
# 원본 상위/패턴Count와 새 helper 내보내기이며 정밀 C만 읽고 원본 실행은 추가하지 않는다.
EXPORT_FILES = [f"extracted/{group}/{edition}/{name}" for group in ["terrainworld", "normalterrainbinding"]
                for edition in ["originals", "originalCD"] for name in ["creation.c", "functions.tsv"]]
EXPORT_FILES += ["tools/ghidra/normalterrainbinding-functions.json", "tools/ghidra/terrainworld-functions.json"]
# 원본 PE는 읽기만 하고 두 판본의 바이트 출처를 분리한다.
PE_FILES = ["originals/Netstorm.exe", "originalCD/NETSTORM.EXE"]


def sha(data: bytes) -> str:
    """전체 파일의 SHA-256을 계산하여 원문 보존을 확인한다."""
    return hashlib.sha256(data).hexdigest()


def pin(path: str) -> dict:
    """존재하는 한 파일의 전체 바이트 출처와 크기를 기록한다."""
    data = (ROOT / path).read_bytes()
    return {"path": path, "sha256": sha(data), "bytes": len(data)}


def git_bytes(path: str) -> bytes:
    """기준 커밋의 전체 파일을 읽으며 현재 파일이나 git 저장소를 수정하지 않는다."""
    return subprocess.check_output(["git", "show", f"{BASELINE}:{path}"], cwd=ROOT)


def record() -> None:
    """새 소비자 기록을 최초 한 번 작성하며 기존 보고서의 원본 출처를 덮어쓰지 않는다."""
    if REPORT.exists():
        raise RuntimeError("이미 존재하는 소비자 보고서는 새 lineage 없이 재기록할 수 없습니다")
    baseline = subprocess.check_output(["git", "rev-parse", BASELINE], cwd=ROOT, text=True).strip()
    dependencies = []
    # 모든 완료 몸체의 전체 원문을 보존하고 현재 바이트도 같은지 확인한다.
    for path in DEPENDENCIES:
        before = git_bytes(path)
        current = (ROOT / path).read_bytes()
        if current.replace(b"\r\n", b"\n") != before.replace(b"\r\n", b"\n"):
            raise AssertionError(f"완료 의존 소스가 기준 커밋과 다릅니다: {path}")
        dependencies.append({**pin(path), "baseline_text": current.decode("utf-8"),
                             "git_baseline_text": before.decode("utf-8"), "git_baseline_sha256": sha(before),
                             "change": "몸체/토큰 변경 없음; Git과 작업 파일의 CRLF/LF만 분리"})
    reused = []
    # 보고서/fixture/원본 실행 도구를 전체 바이트 SHA로 연결하고 기존 관찰 수는 분리한다.
    for name, total in NATIVE_REUSE:
        report_path = f"cpppj/recovery-{name}-evidence.json"
        previous = (ROOT / report_path).read_bytes()
        native = json.loads(previous)
        if native["total"] != total:
            raise AssertionError(f"기존 전체 관찰 수 오류: {name}/{native['total']}")
        reused.append({"module": name, "previous_unique": total, "new_unique": 0,
                       "report": {**pin(report_path), "complete_previous_text": previous.decode("utf-8")},
                       "fixture": pin(f"cpppj/tests/fixtures/{name}-x86.tsv"),
                       "oracle": pin(f"tools/decomp_{name}_oracle.py")})
    result = {"schema": 1, "host": socket.gethostname(), "last_decompile_host": "HJOW-Athlon",
              "last_decompile_date": "2026-10-14", "baseline_commit": baseline, "baseline_tests": 1010,
              "new_native_unique": 0, "new_native_assertions": 0, "new_cpp_consumer_tests": 3,
              "game_runs": 0, "full_parent_completed": False, "archipelago_completed": False,
              "cpp_source_files": [pin(path) for path in CPP_FILES], "whole_dependency_lineage": dependencies,
              "native_reuse": reused, "exports": [pin(path) for path in EXPORT_FILES],
              "pe_files": [pin(path) for path in PE_FILES], "tool": pin("tools/analyze_normalterrain_world_binding.py"),
              "limitations": ["합성 프레임/SHP와 두 섬 입력의 실제 소비자이며 전체 wrapper 독립 PE 재실행은 0이다.",
                              "실제 상위 분기 판정과 전체 Fort/IslandBuilder 부모·군도는 완료하지 않았다.",
                              "기존 pipeline의 private 입력 identity는 조회할 수 없으므로 같은 Core자료 연결은 호출자 계약이다.",
                              "실제121은 특수 vortex 지역 제한을 조회하지 않으므로 그 검사에서 섬 목록 사본을 만들지 않는다.",
                              "소비자 입력 범위는 내부 유효 청크1..15이며 부모의 비정상 bounds/assert 전체 복원은 아니다."]}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def verify() -> None:
    """새 출처와 보존한 기존 전체 몸체/관찰을 연결하고 native 추가0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding="utf-8"))
    assert report["new_native_unique"] == report["new_native_assertions"] == report["game_runs"] == 0
    assert not report["full_parent_completed"] and not report["archipelago_completed"]
    files = report["cpp_source_files"] + report["exports"] + report["pe_files"] + [report["tool"]]
    # 현재 소스/내보내기/PE의 전체 SHA와 크기를 하나씩 확인한다.
    for item in files:
        assert pin(item["path"]) == {key: item[key] for key in ["path", "sha256", "bytes"]}, item["path"]
    # 각 완료 몸체의 전체 원문은 기준 커밋과 현재 파일 모두에 일치해야 한다.
    for item in report["whole_dependency_lineage"]:
        data = item["baseline_text"].encode("utf-8")
        git_data = item["git_baseline_text"].encode("utf-8")
        assert data == (ROOT / item["path"]).read_bytes(), item["path"]
        assert git_data == git_bytes(item["path"]) and sha(git_data) == item["git_baseline_sha256"]
        assert data.replace(b"\r\n", b"\n") == git_data.replace(b"\r\n", b"\n"), item["path"]
        assert sha(data) == item["sha256"] and len(data) == item["bytes"]
    # 과거 보고서 전체·원본 fixture·도구는 보존되고 이번 관찰 합산은 각각0이다.
    for item in report["native_reuse"]:
        assert item["new_unique"] == 0
        for key in ["report", "fixture", "oracle"]:
            source = item[key]
            assert pin(source["path"]) == {field: source[field] for field in ["path", "sha256", "bytes"]}
        assert item["report"]["complete_previous_text"].encode("utf-8") == (ROOT / item["report"]["path"]).read_bytes()
    source = (ROOT / CPP_FILES[1]).read_text(encoding="utf-8")
    # wrapper 안의 실제 호출 순서만 정적으로 확인하고 이를 새 원본 실행 관찰로 세지 않는다.
    begin = source.index("o::Sid RawNormalTerrainWorldBinding::Generate")
    body = source[begin:source.index("RawNormalTerrainWorldBinding::FirstRecord", begin)]
    names = ["RawTerrainContext>", "growth.RunNormal()", "context->MarkEdges('x','=')", "stamp.Stamp('x')",
             "context->NormalizeAfterStamp()", "PostprocessFarms(request.selectedRegion)"]
    positions = [body.index(name) for name in names]
    assert positions == sorted(positions)
    tests = (ROOT / CPP_FILES[2]).read_text(encoding="utf-8")
    assert tests.count("TEST_CASE(NormalTerrainWorldBinding_") == report["new_cpp_consumer_tests"] == 3
    print("일반 지형 실제 월드 소비자 lineage 감사 성공: 기존 전체 소스/출처 불변, 새 native 0, 소비자 검사 3")


def main() -> None:
    """최초 --record만 기록하며 기본/--verify는 보존된 출처 감사만 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true", help="기준 전체 원문과 새 소비자 SHA를 최초 기록한다")
    parser.add_argument("--verify", action="store_true", help="기록된 전체 출처를 감사한다")
    args = parser.parse_args()
    if args.record:
        record()
    verify()


if __name__ == "__main__":
    main()
