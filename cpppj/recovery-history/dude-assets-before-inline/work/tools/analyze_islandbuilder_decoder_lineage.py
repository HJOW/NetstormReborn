#!/usr/bin/env python3
"""공유 디코더의 선택적 누락 프레임 진단을 이전 전체 파일/관찰과 역변환으로 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

# 완료 단위10의 기준 커밋과 이번 공유 확장 출처다.
ROOT = Path(__file__).resolve().parents[1]
BASELINE = "a349e63"
REPORT = ROOT / "cpppj/recovery-islandbuilder-decoder-lineage-evidence.json"
HISTORY = ROOT / "cpppj/recovery-history/islandbuilder-decoder-baseline"
# 기존 lookup 몸체를 재작성하지 않고 해당 실패 직후의 원본 보고만 선택적으로 요청한다.
CHANGES = {
    "cpppj/src/o/CanonDecoder.h": [
        ("#include <cstdint>", "#include <cstdint>\n#include <functional>"),
        ("CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y);",
         "CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y,\n        std::function<void()> missingFrameDiagnostic={});"),
        ("    // rotation_: 패턴을 회전할 방향 값.",
         "    // missingFrameDiagnostic_: 실제 FindNumber 실패의 보고 후 계속 진행을 요청한다. 빈 값은 기존 호출 동작이다.\n    std::function<void()> missingFrameDiagnostic_;\n    // rotation_: 패턴을 회전할 방향 값.")],
    "cpppj/src/o/CanonDecoder.cpp": [
        ("#include <limits>", "#include <limits>\n#include <utility>"),
        ("CanonDecoder::CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y)\n    : frames_(&frames), pattern_(&pattern), rotation_(direction / 2), originX_(x), originY_(y) {",
         "CanonDecoder::CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y,\n    std::function<void()> missingFrameDiagnostic)\n    : frames_(&frames), pattern_(&pattern), missingFrameDiagnostic_(std::move(missingFrameDiagnostic)),\n      rotation_(direction / 2), originX_(x), originY_(y) {"),
        ("                frame_ = frames_->FindNumber(side, variant, static_cast<std::uint8_t>(variation));",
         "                frame_ = frames_->FindNumber(side, variant, static_cast<std::uint8_t>(variation));\n                // 원본 assert lookup은 보고한 뒤 -1을 반환하며 동일 Advance가 그 칸을 계속 건너뛴다.\n                if (frame_ == -1 && missingFrameDiagnostic_) missingFrameDiagnostic_();")]
}
# 수정하지 않은 검색/패턴/타입 디코더와 전체 기존 검사의 출처도 보존한다.
UNCHANGED = ["cpppj/src/o/RiftType.h", "cpppj/src/o/RiftType.cpp", "cpppj/src/o/CanonDecoderTables.inc",
             "cpppj/src/o/CanonTypeDecoder.h", "cpppj/src/o/CanonTypeDecoder.cpp", "cpppj/tests/CanonTypeDecoderTests.cpp"]


def sha(data):
    """파일 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def git_bytes(path):
    """완료 기준 커밋의 전체 파일을 읽으며 저장소를 변경하지 않는다."""
    return subprocess.check_output(["git", "show", f"{BASELINE}:{path}"], cwd=ROOT)


def pin(path):
    """현재 파일의 전체 크기와 SHA를 기록한다."""
    data = (ROOT / path).read_bytes()
    return {"path": path, "bytes": len(data), "sha256": sha(data)}


def transform(text, reverse=False):
    """이번의 명시된 세 부분씩만 교정하거나 정확히 역변환한다."""
    path, source = text
    # 역변환에서는 마지막 삽입부터 제거하여 이전 전체 본문을 복원한다.
    for before, after in (reversed(CHANGES[path]) if reverse else CHANGES[path]):
        old, new = (after, before) if reverse else (before, after)
        if source.count(old) != 1:
            raise AssertionError(f"디코더 변경 위치가 유일하지 않습니다: {path}/{old[:60]}")
        source = source.replace(old, new, 1)
    return source


def referenced_paths(value):
    """이전 보고서에 명시된 fixture와 native 도구 경로를 전체 원문 보존 대상으로 찾는다."""
    paths = set()
    if isinstance(value, dict):
        # reports의 키와 값에 있는 경로 표현을 모두 읽는다.
        for key, item in value.items():
            paths.update(referenced_paths(key)); paths.update(referenced_paths(item))
    elif isinstance(value, list):
        # 소스 목록의 모든 명시 경로를 누락 없이 읽는다.
        for item in value:
            paths.update(referenced_paths(item))
    elif isinstance(value, str) and (value.startswith("cpppj/tests/fixtures/") or value.startswith("tools/")):
        if "\n" not in value and (ROOT / value).is_file():
            paths.add(value)
    return paths


def record():
    """기존 전체 파일/보고서/fixture를 보존한 뒤 선택적 진단 API만 최초 작성한다."""
    if REPORT.exists() or HISTORY.exists():
        raise RuntimeError("기존 디코더 lineage를 덮어쓸 수 없습니다")
    tracked = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", BASELINE, "cpppj"], cwd=ROOT, text=True).splitlines()
    reports = []
    # 이번 수정의 이전 출처를 가진 완료 보고서만 선택하며 다른 진행 단위는 읽지 않는다.
    for path in tracked:
        if path.startswith("cpppj/recovery-") and path.endswith("evidence.json") and "/recovery-history/" not in path:
            data = git_bytes(path)
            if any(name.encode() in data for name in ["CanonDecoder.h", "CanonDecoder.cpp", "RiftType.h", "RiftType.cpp"]) or path == "cpppj/recovery-canontype-evidence.json":
                reports.append(path)
    paths = set(CHANGES) | set(UNCHANGED) | set(reports)
    # 기존 관찰 보고서의 도구/fixture 전체와 각 보고서 자체를 기준 커밋에서 보존한다.
    for path in reports:
        paths.update(referenced_paths(json.loads(git_bytes(path))))
    files = []
    # Git 바이트와 작업 파일의 CRLF/LF를 따로 남기며 몸체 차이는 허용하지 않는다.
    for path in sorted(paths):
        before = (ROOT / path).read_bytes(); baseline = git_bytes(path)
        if before.replace(b"\r\n", b"\n") != baseline.replace(b"\r\n", b"\n"):
            raise AssertionError(f"단위10 기준 파일이 이미 변경됐습니다: {path}")
        saved = HISTORY / path; saved.parent.mkdir(parents=True, exist_ok=True); saved.write_bytes(before)
        git_saved = HISTORY / "git" / path; git_saved.parent.mkdir(parents=True, exist_ok=True); git_saved.write_bytes(baseline)
        files.append({"path": path, "snapshot": pin(saved.relative_to(ROOT).as_posix()),
                      "git_snapshot": pin(git_saved.relative_to(ROOT).as_posix()), "changed": path in CHANGES})
    # 검색/셀/순회 몸체는 유지하며 세 지정 부분씩만 확장한다.
    for path in CHANGES:
        current = transform((path, (ROOT / path).read_text(encoding="utf-8")))
        (ROOT / path).write_bytes(current.encode("utf-8"))
    result = {"host": "HJOW-Athlon", "date": "2026-10-14", "baseline_commit": BASELINE,
              "baseline_cpp_tests": 1023, "new_native_unique": 0, "new_game_runs": 0,
              "files": files, "current_changed": [pin(path) for path in CHANGES],
              "tool": pin("tools/analyze_islandbuilder_decoder_lineage.py"),
              "limits": ["이번 공유 API 자체의 새 native 합산0. 새 IslandBuilder 전체 관찰은 별도 보고서다.",
                         "기존 FindNumber 검색/Advance의 셀·프레임 건너뛰기·x/y/유효성 몸체를 재작성하지 않는다.",
                         "새 callback을 전달하지 않은 모든 기존 호출은 이전 보고 후 계속 진단 없는 C++ 동작을 유지한다.",
                         "기존 NormalTerrainWorldBinding 등의 보고서는 당시 SHA 그대로 보존하며 현재 변경 두 파일은 이 successor로 연결한다."]}
    REPORT.write_bytes((json.dumps(result, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


def verify():
    """정확한 역변환/이전 전체 바이트/현재 변경 SHA로 기본 동작의 출처를 감사한다."""
    report = json.loads(REPORT.read_text(encoding="utf-8"))
    assert report["new_native_unique"] == report["new_game_runs"] == 0
    assert report["baseline_cpp_tests"] == 1023 and pin(report["tool"]["path"]) == report["tool"]
    # 모든 이전 전체 파일은 당시 기준 Git 바이트와 별도 현재 파일 바이트를 보존한다.
    for item in report["files"]:
        for key in ["snapshot", "git_snapshot"]:
            assert pin(item[key]["path"]) == item[key]
        saved = (ROOT / item["snapshot"]["path"]).read_bytes()
        baseline = (ROOT / item["git_snapshot"]["path"]).read_bytes()
        assert baseline == git_bytes(item["path"])
        assert saved.replace(b"\r\n", b"\n") == baseline.replace(b"\r\n", b"\n")
        current = (ROOT / item["path"]).read_bytes()
        if item["changed"]:
            restored = transform((item["path"], current.decode("utf-8")), True).encode("utf-8")
            assert restored == saved.replace(b"\r\n", b"\n"), item["path"]
        else:
            assert current == saved, item["path"]
    # 현재 확장의 전체 파일 SHA도 독립적으로 확인한다.
    for item in report["current_changed"]:
        assert pin(item["path"]) == item
    print(f"IslandBuilder 디코더 lineage 감사 성공: 전체 {len(report['files'])}파일, 기준1023, 추가native0")


def main():
    """--record는 최초 보존/교정, 기본 실행은 현재 전체 감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true")
    args = parser.parse_args()
    if args.record:
        record()
    verify()


if __name__ == "__main__":
    main()
