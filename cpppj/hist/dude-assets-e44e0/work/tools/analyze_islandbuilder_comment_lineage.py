#!/usr/bin/env python3
"""IslandBuilder 검사 주석 한 줄을 한국어로 교정하고 기존 전체 native 출처를 보존한다."""
import argparse
import hashlib
import json
import runpy
from pathlib import Path

# 기존 native/source/report를 덮어쓰지 않고 현재 검사 SHA를 successor에서 연결한다.
ROOT=Path(__file__).resolve().parents[1]
TEST="cpppj/tests/IslandBuilderTests.cpp"
HISTORY="cpppj/hist/islandbuilde-a876e"
REPORT=ROOT/"cpppj/islandbuilder-comment-lineage.json"
FILES=[TEST,"cpppj/islandbuilder.json","cpppj/islandbuilder-partial-profile-lineage.json",
       "tools/decomp_islandbuilder_oracle.py","tools/analyze_islandbuilder_partial_profile_lineage.py"]
# 설명 용도를 유지하면서 혼합 문자만 한국어로 바꾼다. C++ 토큰 변경은0이다.
BEFORE="pointer同一조건의world와별도임시지도를각각소유한다."
AFTER="주소가같은world와별도임시지도를각각소유한다."


def sha(data):
    """전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def pin(path):
    """한 전체 파일의 경로/바이트수/SHA를 기록한다."""
    data=(ROOT/path).read_bytes();return {"path":path,"bytes":len(data),"sha256":sha(data)}


def record():
    """교정전 전체 파일을 보존한 뒤 검사 설명 한 줄만 최초 교정한다."""
    if REPORT.exists():raise RuntimeError("주석 successor 덮어쓰기 금지")
    saved=[]
    # 기존 native/tool/report 전체 바이트는 별도 이력으로 그대로 남긴다.
    for path in FILES:
        target=ROOT/f"{HISTORY}/{path}";target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/path).read_bytes())
        saved.append({"original_path":path,"saved":pin(target.relative_to(ROOT).as_posix())})
    text=(ROOT/TEST).read_text(encoding="utf-8");assert text.count(BEFORE)==1
    (ROOT/TEST).write_bytes(text.replace(BEFORE,AFTER,1).encode("utf-8"))
    result={"host":"HJOW-Athlon","date":"2026-10-14","new_native_unique":0,"new_game_runs":0,
            "native_unique_preserved":1156,"normal_preserved":814,"assertion_preserved":342,
            "before_files":saved,"current_test":pin(TEST),"tool":pin("tools/analyze_islandbuilder_comment_lineage.py"),
            "limits":["한국어 용도 주석 한 줄만 변경하며 생산/검사 C++ 토큰·원본 입력·전체 fixture 변경0.",
                      "기존 native report/tool/부분프레임 lineage는 당시 SHA 그대로 유지한다. 현재 CPP SHA는 이 successor에 명시한다.",
                      "이전 두 감사기의 TEST 한 파일만 교정전 전체 snapshot으로 연결하여 이전 관찰을 감사한다. 다른 현재 파일은 SHA 검사를 그대로 받는다."]}
    REPORT.write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+"\n").encode("utf-8"))


def audit_previous():
    """이전 두 감사기를 그때의 전체 검사 소스에 연결하며 모든 다른 출처는 현재 SHA로 감사한다."""
    before=(ROOT/f"{HISTORY}/{TEST}").read_bytes()
    native=runpy.run_path(str(ROOT/"tools/decomp_islandbuilder_oracle.py"))
    original_digest=native["verify"].__globals__["digest"]
    # 한 교정전 검사 파일만 당시 바이트 SHA로 해석하고 나머지는 원래 감사기를 사용한다.
    native["verify"].__globals__["digest"]=lambda path:sha(before) if Path(path).resolve()==(ROOT/TEST).resolve() else original_digest(path)
    native["verify"]()
    partial=runpy.run_path(str(ROOT/"tools/analyze_islandbuilder_partial_profile_lineage.py"))
    scope=partial["verify"].__globals__;original_pin=scope["pin"];original_inverse=scope["inverse"]
    # 이전 partial report가 고정한 TEST만 전체 snapshot으로 읽는다. Native report와 fixture는 변경0이다.
    scope["pin"]=lambda path:{"path":TEST,"sha256":sha(before),"bytes":len(before)} if path==TEST else original_pin(path)
    def prior_inverse(path):
        """부분프레임 입력 역변환도 교정전 전체 검사 원문에서 수행한다."""
        if path!=TEST:return original_inverse(path)
        text=before.decode("utf-8")
        # 기존 partial 교정의 명시 두 문자열만 이전 순서로 역변환한다.
        for old,new in reversed(scope["CHANGES"][TEST]):
            assert text.count(new)==1;text=text.replace(new,old,1)
        return text.encode("utf-8")
    scope["inverse"]=prior_inverse;partial["verify"]()


def verify():
    """정확 주석 역변환과 전체 이전 source/native/fixture의 연결을 감사한다."""
    proof=json.loads(REPORT.read_text(encoding="utf-8"));assert proof["new_native_unique"]==proof["new_game_runs"]==0
    assert pin(TEST)==proof["current_test"] and pin(proof["tool"]["path"])==proof["tool"]
    # 이전 전체 파일은 보존하며 교정대상 검사 외에는 현재 파일도 바이트가 동일하다.
    for item in proof["before_files"]:
        assert pin(item["saved"]["path"])==item["saved"]
        if item["original_path"]!=TEST:
            assert (ROOT/item["original_path"]).read_bytes()==(ROOT/item["saved"]["path"]).read_bytes()
    text=(ROOT/TEST).read_text(encoding="utf-8");assert text.count(AFTER)==1
    assert text.replace(AFTER,BEFORE,1).encode("utf-8")==(ROOT/f"{HISTORY}/{TEST}").read_bytes()
    audit_previous()
    print("IslandBuilder 한국어 주석 lineage 감사 성공: 토큰/원본1156행 불변, 추가native0")


def main():
    """--record는 최초 보존/교정이고 기본 실행은 현재 전체 감사다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--record",action="store_true")
    if parser.parse_args().record:record()
    verify()


if __name__=="__main__":main()
