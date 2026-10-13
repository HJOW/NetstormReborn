#!/usr/bin/env python3
"""실제 MayPlace의 nonsurface 지면 무시 조건을 합성121 입력에 보존한 한행 교정을 감사한다."""
import copy
import hashlib
import json
from pathlib import Path

# 기존 원본/production/fixture는 변경하지 않고 새 소비자의 합성 입력 한 행만 연결한다.
ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "cpppj/normalterrain-binding-profile-correction.json"
BEFORE = "record.flags1=type==kNormalIsle ? 0x28000803U : randomFarm ? 0x00100812U : 0x00200812U;"
AFTER = "record.flags1=type==kNormalIsle ? 0x28000803U : randomFarm ? 0x00100012U : 0x00200012U;"


def pin(path):
    """전체 파일의 SHA/크기로 이전 검사·도구·보고서와 현재 출처를 확인한다."""
    data = (ROOT / path).read_bytes()
    return {"path": path, "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)}


def main():
    """한 입력 행을 역변환하고 해시 교정의 전체 이전 연결/최신 출처를 감사한다."""
    proof = json.loads(EVIDENCE.read_text(encoding="utf-8"))
    assert proof["new_native_unique"] == proof["production_changes"] == 0
    # 이전 원문·현재 소스·교정 도구의 전체 SHA를 각각 확인한다.
    for key in ["before_test", "before_report", "before_hash_tool", "before_hash_evidence", "after_test", "tool", "after_hash_tool"]:
        assert pin(proof[key]["path"]) == proof[key]
    current = (ROOT / proof["after_test"]["path"]).read_bytes().decode("utf-8")
    assert current.count(AFTER) == 1 and BEFORE not in current
    assert current.replace(AFTER, BEFORE).encode("utf-8") == (ROOT / proof["before_test"]["path"]).read_bytes()
    before = json.loads((ROOT / proof["before_report"]["path"]).read_text(encoding="utf-8"))
    report = json.loads((ROOT / "cpppj/normalterrain-world-binding.json").read_text(encoding="utf-8"))
    restored = copy.deepcopy(report)
    # 후속 검사SHA/감사기 연결만 역변환해 이전 전체 PE/native/완료 source 출처가 보존됐는지 확인한다.
    for item in restored["cpp_source_files"]:
        if item["path"] == proof["after_test"]["path"]:
            item.update(next(old for old in before["cpp_source_files"] if old["path"] == item["path"]))
    restored["exports"] = before["exports"]
    del restored["cpp_profile_correction"]
    assert restored == before
    old_hash = json.loads((ROOT / proof["before_hash_evidence"]["path"]).read_text(encoding="utf-8"))
    current_hash = json.loads((ROOT / "cpppj/normalterrain-binding-cpp-correction.json").read_text(encoding="utf-8"))
    restored_hash = copy.deepcopy(current_hash)
    restored_hash["tool"] = old_hash["tool"]
    del restored_hash["profile_successor"]
    assert restored_hash == old_hash
    print("일반 지형121 합성flags 입력 교정 감사 성공: 한행 역변환, 기존전체/hash출처 보존, production/native0")


if __name__ == "__main__":
    main()
