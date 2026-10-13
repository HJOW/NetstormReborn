#!/usr/bin/env python3
"""DamageablePostDestroy의현재전체관찰·필수경계·owner재조회입력교정과CPP출처를감사한다."""
import argparse
import hashlib
import json
from pathlib import Path
from decomp_damageablepostdestroy_oracle import ROOT,CONTROLS,inputs

# 첫원본전체관찰은현재완료수에중복합산하지않는다.
NATIVE="cpppj/recovery-damageablepostdestroy-evidence.json"
FIXTURE="cpppj/tests/fixtures/damageablepostdestroy-x86.tsv"
HISTORY="cpppj/recovery-history/damageablepost-before-owner-mutation"
REPORT=ROOT/"cpppj/recovery-damageablepostdestroy-lineage-evidence.json"
CPP=["cpppj/src/o/RawDamageablePostDestroy.h","cpppj/src/o/RawDamageablePostDestroy.cpp","cpppj/tests/DamageablePostDestroyTests.cpp"]
# 부모몸체는보존하고effectprofile6만실제ownerBYTE변경으로연결하는입력추가다.
INSERT='            if c["mutation"]==6:self.put(self.slot(SOURCE)+s["owner"],3,1)\n'


def sha(data):
    """전체바이트의SHA-256을계산한다."""
    return hashlib.sha256(data).hexdigest()


def pin(path):
    """현재한파일의전체크기와SHA를읽는다."""
    data=(ROOT/path).read_bytes();return {"path":path,"bytes":len(data),"sha256":sha(data)}


def rows(path):
    """관찰행을그대로읽으며새기대HP/frame/owner를계산하지않는다."""
    return [line.split("\t") for line in (ROOT/path).read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]


def audit_native():
    """현재원본파일SHA와실제입력/정상assert계수·두정밀도복귀를감사한다."""
    report=json.loads((ROOT/NATIVE).read_text(encoding="utf-8"));observed=rows(FIXTURE)
    assert report["total"]==report["normal_total"]==len(observed)==4412 and report["assertion_total"]==0
    assert report["game_runs"]==report["os_calls"]==0 and report["controls"]==list(CONTROLS)
    # 모든실제source/PE/export/fixture의현재전체SHA를확인한다.
    for path,expected in report["files"].items():assert pin(path)["sha256"]==expected,path
    # 양판본의고유입력은독립generator의실제준비입력과같으며행마지막assert수를분류한다.
    for edition,counts in report["editions"].items():
        selected=[row for row in observed if row[0]==edition]
        actual={(row[1],tuple(map(int,row[2].split(",")))) for row in selected}
        expected=set(inputs(edition));assert actual==expected and len(actual)==len(selected)==counts["unique"]
        assert counts["normal_unique"]==len(selected) and counts["assertion_unique"]==0 and counts["returns"]==len(selected)*2
        assert all(row[5]=="0" for row in selected)
    return report


def audit_input():
    """owner변경profile6밖의전체4392행은이전관찰과동일해야한다."""
    before=rows(f"{HISTORY}/{FIXTURE}");after=rows(FIXTURE);assert len(before)==len(after)==4412
    changed=0
    # 입력/순서는같으며profile6의20행에서만원본이읽은owner와raw가바뀐다.
    for old,new in zip(before,after):
        assert old[:3]==new[:3]
        if int(new[2].split(",")[15])==6:
            assert old!=new;changed+=1
        else:assert old==new
    assert changed==20
    return changed


def record():
    """전체첫관찰/도구/CPP를보존한owner재조회교정과현재검사소스를최초연결한다."""
    if REPORT.exists():raise RuntimeError("Damageable post lineage 덮어쓰기 금지")
    current=audit_native();audit_input();old=json.loads((ROOT/f"{HISTORY}/{NATIVE}").read_text(encoding="utf-8"));sources=[]
    # 이전전체native출처는해당snapshot또는불변현재파일로연결한다.
    for path,expected in old["files"].items():
        source=f"{HISTORY}/{path}" if (ROOT/f"{HISTORY}/{path}").exists() else path
        saved=pin(source);assert saved["sha256"]==expected,(path,source);sources.append({"original_path":path,"saved":saved})
    proof={"host":"HJOW-Athlon","date":"2026-10-14","new_native_unique":0,"native_unique":4412,"normal_unique":4412,"assertion_unique":0,
        "before_report":pin(f"{HISTORY}/{NATIVE}"),"before_source_files":sources,"current_native_report":pin(NATIVE),"current_cpp":list(map(pin,CPP)),
        "tool":pin("tools/analyze_damageablepostdestroy.py"),"changed_profile_rows":20,"unchanged_whole_rows":4392,
        "actual_host_completed":False,"dude_generation_completed":False,"full_delete_chain_completed":False,
        "limits":["profile6의누락된owner변경입력을한줄로공급하고다음섬정리요청에서현재owner BYTE를읽는지관찰한다.",
                  "이전4412/재관찰4412를현재4412고유원본수에중복합산하지않는다. 생산CPP변경0.",
                  "새CPP2검사는원본부모body/명시외부경계의대조초안이다. 실제Host SetInitialCount와Dude생성·전체삭제는미완료다."]}
    REPORT.write_bytes((json.dumps(proof,ensure_ascii=False,indent=2)+"\n").encode("utf-8"))


def verify():
    """전체현재출처와교정전원문/정확한한줄역변환을감사한다."""
    proof=json.loads(REPORT.read_text(encoding="utf-8"));audit_native();audit_input();assert proof["new_native_unique"]==0
    assert not proof["actual_host_completed"] and not proof["dude_generation_completed"] and not proof["full_delete_chain_completed"]
    # 모든보존파일과현재CPP/native/tool을전체바이트SHA로연결한다.
    for item in [proof["before_report"],proof["current_native_report"],proof["tool"],*proof["current_cpp"],*[entry["saved"] for entry in proof["before_source_files"]]]:
        assert pin(item["path"])==item,item["path"]
    path="tools/decomp_damageablepostdestroy_oracle.py";text=(ROOT/path).read_text(encoding="utf-8");assert text.count(INSERT)==1
    assert text.replace(INSERT,"",1).encode("utf-8")==(ROOT/f"{HISTORY}/{path}").read_bytes()
    # 실제부모본문은교정전과전체바이트가같으며변경은명시입력경계한줄뿐이다.
    for path in CPP[:2]:assert (ROOT/path).read_bytes()==(ROOT/f"{HISTORY}/{path}").read_bytes(),path
    print("DamageablePostDestroy 감사 성공: 원본4412정상/assert0, 교정20/동일4392, 실제Host·Dude생성미완료")


def main():
    """--record는최초출처연결이고기본은현재전체감사다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--record",action="store_true")
    if parser.parse_args().record:record()
    verify()


if __name__=="__main__":main()
