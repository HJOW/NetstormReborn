#!/usr/bin/env python3
"""실제 일반 해시 체인의 검사 교정만 전체 이전 소스/출처에 역변환으로 연결한다."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

# 모두 새 소비자 단위의 파일이며 기존 완료 소스/fixture/원본은 변경하지 않는다.
ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "cpppj/tests/NormalTerrainWorldBindingTests.cpp"
REPORT = ROOT / "cpppj/normalterrain-world-binding.json"
HISTORY = ROOT / "cpppj/hist/normalterrai-4f241"
EVIDENCE = ROOT / "cpppj/normalterrain-binding-cpp-correction.json"
TOOL = "tools/analyze_normalterrain_binding_cpp_correction.py"
# 처음 검사와 교정 검사의 정확한 문자열이다. production 몸체와 예상 난수 계산은 그대로 둔다.
BEFORE = "            CHECK(core.hash.Bucket(level,object.object.x,object.object.y)==object.sid);\n"
AFTER = """            bool found=false;std::uint32_t inspected=0; // 실제 같은 버킷의 SID 체인 포함 여부와 순회 한도다.
            // 일반 해시는 여러 SID를 한 머리에 연결하므로 대상이 머리인지를 요구하지 않는다.
            for (Sid current{core.hash.Bucket(level,object.object.x,object.object.y)};
                 current.value && inspected++<core.pool.Capacity();current=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(current),4,2))}) {
                if (current==object.sid) { found=true;break; }
            }
            CHECK(found);
"""


def digest(data):
    """전체 파일의 SHA-256을 사용하여 이전 원문과 현재 파일을 확인한다."""
    return hashlib.sha256(data).hexdigest()


def pin(path):
    """저장소 상대 경로와 전체 바이트 SHA/크기를 반환한다."""
    data = (ROOT / path).read_bytes()
    return {"path": path, "sha256": digest(data), "bytes": len(data)}


def apply():
    """처음 검사/보고서를 전체 보존한 뒤 한 해시 검사만 교정하고 새 출처를 연결한다."""
    if EVIDENCE.exists() or HISTORY.exists():
        raise RuntimeError("해시 검사 교정은 최초 한 번만 적용할 수 있습니다")
    before_test = TEST.read_bytes()
    before_report = REPORT.read_bytes()
    text = before_test.decode("utf-8")
    assert text.count(BEFORE) == 1 and AFTER not in text
    HISTORY.mkdir(parents=True)
    (HISTORY / "NormalTerrainWorldBindingTests.cpp").write_bytes(before_test)
    (HISTORY / "evidence.json").write_bytes(before_report)
    TEST.write_bytes(text.replace(BEFORE, AFTER).encode("utf-8"))
    proof = {"new_native_unique": 0, "production_changes": 0,
             "reason": "단계3 일반 해시는 같은16칸 버킷의 여러SID를 실제+4 WORD 체인으로 연결한다.",
             "before_test": pin("cpppj/hist/normalterrai-4f241/NormalTerrainWorldBindingTests.cpp"),
             "before_report": pin("cpppj/hist/normalterrai-4f241/evidence.json"),
             "after_test": pin("cpppj/tests/NormalTerrainWorldBindingTests.cpp"), "tool": pin(TOOL)}
    EVIDENCE.write_text(json.dumps(proof, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    report = json.loads(before_report)
    # 등록된 새 C++ 검사 한 파일의 SHA만 successor로 갱신한다.
    for item in report["cpp_source_files"]:
        if item["path"] == proof["after_test"]["path"]:
            item.update(proof["after_test"])
    report["exports"] += [pin(TOOL), pin("cpppj/normalterrain-binding-cpp-correction.json")]
    report["cpp_correction"] = {"evidence": "cpppj/normalterrain-binding-cpp-correction.json",
                                "new_native_unique": 0, "production_changes": 0}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def verify():
    """한 검사 역변환과 전체 이전 보고서 연결을 확인하며 원본 출처 변경을 허용하지 않는다."""
    proof = json.loads(EVIDENCE.read_text(encoding="utf-8"))
    assert proof["new_native_unique"] == proof["production_changes"] == 0
    # 전체 이전/현재 파일과 도구 SHA를 각각 확인한다.
    for key in ["before_test", "before_report", "after_test", "tool"]:
        assert pin(proof[key]["path"]) == proof[key]
    # 최초 도구의 Windows 기본 CRLF 쓰기 실패는 전체 도구/출력을 보존하고 write_bytes 한줄만 교정했다.
    if "initial_tool" in proof:
        assert pin(proof["initial_tool"]["path"]) == proof["initial_tool"]
        initial = (ROOT / proof["initial_tool"]["path"]).read_text(encoding="utf-8")
        current_tool = (ROOT / TOOL).read_text(encoding="utf-8")
        initial_apply = initial.split("def verify():")[0]
        current_apply = current_tool.split("def verify():")[0]
        assert current_apply.replace('TEST.write_bytes(text.replace(BEFORE, AFTER).encode("utf-8"))',
                                     'TEST.write_text(text.replace(BEFORE, AFTER), encoding="utf-8")') == initial_apply
        for key in ["first_evidence", "first_test"]:
            assert pin(proof[key]["path"]) == proof[key]
    current = TEST.read_bytes().decode("utf-8")
    assert current.count(AFTER) == 1 and BEFORE not in current
    assert current.replace(AFTER, BEFORE).encode("utf-8") == (ROOT / proof["before_test"]["path"]).read_bytes()
    before = json.loads((ROOT / proof["before_report"]["path"]).read_text(encoding="utf-8"))
    current_report = json.loads(REPORT.read_text(encoding="utf-8"))
    restored = copy.deepcopy(current_report)
    # 검사 SHA와 새 교정 메타데이터만 되돌려 전체 기존 출처 객체가 같음을 확인한다.
    for item in restored["cpp_source_files"]:
        if item["path"] == proof["after_test"]["path"]:
            item.update(next(old for old in before["cpp_source_files"] if old["path"] == item["path"]))
    del restored["exports"][-2:]
    del restored["cpp_correction"]
    assert restored == before
    print("일반 지형 해시 체인 검사 교정 감사 성공: 한 검사 역변환, production/native 변경0")


def main():
    """--apply만 최초 적용하며 기본 실행은 전체 보존 출처 감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true", help="처음 검사 원문을 보존하고 체인 포함 검사로 교정한다")
    args = parser.parse_args()
    if args.apply:
        apply()
    verify()


if __name__ == "__main__":
    main()
