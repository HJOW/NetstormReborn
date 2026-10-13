#!/usr/bin/env python3
"""부분 프레임 입력의 실제 누락 방향 교정을 이전 전체 native 관찰과 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path

# 첫1156관찰과현재1156관찰은최종고유수에한번만계수한다.
ROOT=Path(__file__).resolve().parents[1]
HISTORY="cpppj/recovery-history/islandbuilder-partial-profile-initial"
NATIVE="cpppj/recovery-islandbuilder-evidence.json"
FIXTURE="cpppj/tests/fixtures/islandbuilder-x86.tsv"
REPORT=ROOT/"cpppj/recovery-islandbuilder-partial-profile-lineage-evidence.json"
# profile3의입력은동일하고그프레임표에서원본패턴3이실제로쓰는G방향을제거한다.
CHANGES={
    "tools/decomp_islandbuilder_oracle.py":[("부분누락은명시입력A방향만빼며","부분누락은명시입력G방향만빼며"),
                                          ("codes[offset]!=ord('A')","codes[offset]!=ord('G')")],
    "cpppj/tests/IslandBuilderTests.cpp":[("부분누락profile3은A방향만제거하여","부분누락profile3은G방향만제거하여"),
                                         ("return code.side=='A';","return code.side=='G';")]
}


def sha(data):
    """전체바이트의SHA-256을계산한다."""
    return hashlib.sha256(data).hexdigest()


def pin(path):
    """현재한파일의바이트수와SHA를기록한다."""
    data=(ROOT/path).read_bytes();return {"path":path,"sha256":sha(data),"bytes":len(data)}


def read_rows(path):
    """기대값을계산하지않고저장한독립원본전체행을읽는다."""
    return [line.split("\t") for line in (ROOT/path).read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]


def inverse(path):
    """이번입력방향한문자와용도설명만역변환한다."""
    text=(ROOT/path).read_text(encoding="utf-8")
    # 두명시교정을정확히한번씩역변환한다.
    for before,after in reversed(CHANGES[path]):
        assert text.count(after)==1,(path,after)
        text=text.replace(after,before,1)
    return text.encode("utf-8")


def audit_observations():
    """동일1156고유입력의profile3밖전체행불변과누락진단활성화를감사한다."""
    before=read_rows(f"{HISTORY}/{FIXTURE}");after=read_rows(FIXTURE)
    assert len(before)==len(after)==1156
    changed=unchanged=0
    # 원본입력/행순서는같으며해당합성프레임profile밖출력은모두같아야한다.
    for old,new in zip(before,after):
        assert old[:4]==new[:4]
        if int(new[2].split(",")[9])==3:
            assert "F" not in old[8] and "F" in new[8]
            changed+=1
        else:
            assert old==new
            unchanged+=1
    assert changed==24 and unchanged==1132
    return changed,unchanged


def record():
    """이전도구/fixture/report/전체CPP를보존한후현재재관찰의출처를최초연결한다."""
    if REPORT.exists():raise RuntimeError("부분입력lineage덮어쓰기금지")
    changed,unchanged=audit_observations()
    before=json.loads((ROOT/f"{HISTORY}/{NATIVE}").read_text(encoding="utf-8"))
    files=[]
    # 첫전체report의모든바이트출처는교정전snapshot또는변경없는현재파일로연결한다.
    for path,expected in before["files"].items():
        source=f"{HISTORY}/{path}" if (ROOT/f"{HISTORY}/{path}").is_file() else path
        item=pin(source);assert item["sha256"]==expected,(path,source)
        files.append({"original_path":path,"saved":item})
    current=[pin(path) for path in [NATIVE,FIXTURE,*CHANGES]]
    # 첫감사는방향코드만교정되고주석A가남은것을발견했다. 당시전체소스/보고서도보존한다.
    failure_path=f"{HISTORY}/first-audit-failure-evidence.json"
    failure=json.loads((ROOT/failure_path).read_text(encoding="utf-8"))
    failed_files=[pin(f"{HISTORY}/first-audit-failure/{item['path']}") for item in [*failure["current_source_files"],failure["tool"]]]
    for saved,observed in zip(failed_files,[*failure["current_source_files"],failure["tool"]]):
        assert saved["sha256"]==observed["sha256"]
    proof={"host":"HJOW-Athlon","date":"2026-10-14","new_native_unique":0,
           "previous_unique":1156,"current_unique":1156,"reobserved_total":1156,
           "changed_profile_rows":changed,"unchanged_whole_rows":unchanged,
           "before_report":pin(f"{HISTORY}/{NATIVE}"),"before_source_files":files,
           "current_source_files":current,"first_audit_failure":{"report":pin(failure_path),"whole_files":failed_files},
           "tool":pin("tools/analyze_islandbuilder_partial_profile_lineage.py"),
           "limits":["A제거입력은패턴3의실제O/G/N칸을누락시키지못했으므로G제거입력으로교정했다.",
                     "전체이전1156관찰/PE/nativeexport/toolSHA를보존한다. 재관찰1156을새고유수에중복합산하지않는다.",
                     "profile3만24행변경이며나머지1132행은원본전체행이동일하다. productionBuilder와공유decoder는변경0."]}
    REPORT.write_bytes((json.dumps(proof,ensure_ascii=False,indent=2)+"\n").encode("utf-8"))


def verify():
    """전체이전관찰SHA/정확입력역변환/현재원본행연결을검사한다."""
    proof=json.loads(REPORT.read_text(encoding="utf-8"))
    assert proof["new_native_unique"]==0 and proof["previous_unique"]==proof["current_unique"]==1156
    assert audit_observations()==(24,1132)
    # 이전전체파일과현재연결의바이트출처를확인한다.
    for item in [proof["before_report"],proof["tool"],*proof["current_source_files"],*[entry["saved"] for entry in proof["before_source_files"]],
                 proof["first_audit_failure"]["report"],*proof["first_audit_failure"]["whole_files"]]:
        assert pin(item["path"])==item,item["path"]
    # 입력교정두파일만정확히역변환하며기존CPP몸체는아래원문으로보존한다.
    for path in CHANGES:
        assert inverse(path)==(ROOT/f"{HISTORY}/{path}").read_bytes(),path
    for path in ["cpppj/src/o/RawIslandBuilder.h","cpppj/src/o/RawIslandBuilder.cpp"]:
        assert (ROOT/path).read_bytes()==(ROOT/f"{HISTORY}/{path}").read_bytes(),path
    print("IslandBuilder 부분프레임lineage 감사 성공: 최종1156, 교정24/동일1132, 추가native0")


def main():
    """--record는최초교정연결이고기본은전체출처감사다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--record",action="store_true")
    if parser.parse_args().record:record()
    verify()


if __name__=="__main__":main()
