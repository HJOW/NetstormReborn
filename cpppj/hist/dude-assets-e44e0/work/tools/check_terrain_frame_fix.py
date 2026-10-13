#!/usr/bin/env python3
"""최초 소비자 실패의 quartile 후보 입력만 이전 전체 출처에 정확 역변환으로 연결한다."""
import copy
import hashlib
import json
from pathlib import Path

# 전체 교정 전 파일과 이후 보고서는 모두 새 소비자 단위에 있으며 기존 native/production은 불변이다.
ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "cpppj/normalterrain-binding-frame-correction.json"
# 후보code/설명2개와 실제 단일 배치의 난수/프레임 기대만 교정한 정확 문자열이다.
CHANGES = [
    ("// 모든16방향×16변형은 검색 실패나 임의 프레임 fallback 없이 원본 Choose를 실행하게 한다.",
     "// 각16방향의16후보는 실제 fallback와 패치quarter의 최소8후보 조건을 함께 만족한다."),
    ("// 프레임 수256 안에서 AA의 J후속36프레임도 물리 범위를 유지한다.",
     "// 같은A변형의16물리프레임을 넣고 총256 안에서 AA의 J후속36프레임도 보존한다."),
    ("terrainCodes.push_back({side,variant,1,0});", "terrainCodes.push_back({side,'A',1,0});"),
    ("GameRandom local(77),expected(scene.random.State());expected.Next(1);const auto previousCrt=scene.crt;",
     "GameRandom local(77),expected(scene.random.State());const auto expectedFrame=240+expected.Next(16);const auto previousCrt=scene.crt;"),
    ("Get(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,edition==OriginalEdition::Patch1078 ? 4 : 1)==255",
     "Get(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,edition==OriginalEdition::Patch1078 ? 4 : 1)==expectedFrame")]


def pin(path):
    """전체 바이트 SHA/크기로 당시 출력과 현재 파일을 확인한다."""
    data = (ROOT / path).read_bytes()
    return {"path": path, "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)}


def main():
    """전체 snapshot과 최소 입력 교정의 역변환·이전 교정 사슬·현재 sourceSHA를 감사한다."""
    proof = json.loads(EVIDENCE.read_text(encoding="utf-8"))
    assert proof["new_native_unique"] == proof["production_changes"] == 0
    # 전단계의 전체 소스/보고서/도구/실패 로그를 원문으로 모두 보존했는지 확인한다.
    for item in proof["snapshots"]:
        current = pin(item["snapshot"])
        assert current["sha256"] == item["sha256"] and current["bytes"] == item["bytes"]
    for key in ["before_test", "before_report", "after_test", "tool", "profile_tool"]:
        assert pin(proof[key]["path"]) == proof[key]
    text = (ROOT / proof["after_test"]["path"]).read_bytes().decode("utf-8")
    # 출력 계산을 바꾸지 않고 후보 입력/기대 세 행과 설명 두 행만 정확히 되돌린다.
    for old, new in CHANGES:
        assert text.count(new) == 1 and old not in text
        text = text.replace(new, old)
    assert text.encode("utf-8") == (ROOT / proof["before_test"]["path"]).read_bytes()
    before = json.loads((ROOT / proof["before_report"]["path"]).read_text(encoding="utf-8"))
    report = json.loads((ROOT / "cpppj/normalterrain-world-binding.json").read_text(encoding="utf-8"))
    restored = copy.deepcopy(report)
    # 새 검사SHA와 후속 감사 연결만 되돌려 이전 전체 report 객체와 대조한다.
    for item in restored["cpp_source_files"]:
        if item["path"] == proof["after_test"]["path"]:
            item.update(next(old for old in before["cpp_source_files"] if old["path"] == item["path"]))
    restored["exports"] = before["exports"]
    del restored["cpp_frame_correction"]
    assert restored == before
    old_profile = json.loads((ROOT / proof["before_profile_evidence"]).read_text(encoding="utf-8"))
    profile = json.loads((ROOT / "cpppj/normalterrain-binding-profile-correction.json").read_text(encoding="utf-8"))
    del profile["frame_successor"]
    profile["tool"] = old_profile["tool"]
    assert profile == old_profile
    print("일반 지형 프레임후보 교정 감사 성공: 입력/기대3행·설명2행 역변환, 전체기존 출처/production/native0")


if __name__ == "__main__":
    main()
