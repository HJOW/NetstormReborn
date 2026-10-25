#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 Pool 정리 부모·실제 하위 Reset·삭제 기록80DWORD·실패 좌표 순서를 먼저 고정한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_sid_oracle import SidOracle, ROOT, SPECS
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 각 PE의 실제 부모·기록 정리·실패 X/Y 주소이며 원문 정밀 C와 교차 대조한다.
PARENTS = {"originals":(0x4af170,0x4952d0,0x545370,0x545374),"originalCD":(0x4aaa70,0x4cff20,0x565498,0x56549c)}
FIXTURE = ROOT/"cpppj/tests/fixtures/squid-pool-reset-x86.tsv"
REPORT = ROOT/"cpppj/squid-pool-reset-native-20261026.json"


class Oracle(SidOracle):
    """기존 검증된 SID CPU 실행기를 확장해 부모와 두40DWORD 초기화도 실제 명령으로 실행한다."""
    def __init__(self,edition):
        """같은 PE·전체 raw Pool·실제 삭제 기록을 사용하고 새 함수 두 개의 범위만 추가한다."""
        self.tracing = False
        super().__init__(edition)
        parent,clear,self.x,self.y = PARENTS[edition]
        facts = {int(row["entry"],16):row for row in csv.DictReader((ROOT/"extracted/refined"/edition/"functions.tsv").open(encoding="utf-8"), delimiter="\t")}
        for entry in (parent,clear):  # 현재 PC의 전체 정밀 결과에서 허용할 실제 몸체 범위를 읽는다.
            for part in facts[entry]["ranges"].split(";"):
                low,high = (int(v,16) for v in part.split("-"))
                self.allowed.append((low,high+1))
        self.entries = dict(self.entries)
        self.entries["WorldReset"] = parent
        self.selected = {parent:"wrapper",self.entries["Reset"]:"pool",clear:"history"}

    def on_instruction(self,mu,address,size,data):
        """부모·실제 Pool Reset·기록 정리의 진입을 기록하며 기존 범위/OS 금지를 유지한다."""
        if self.tracing and address in self.selected:
            self.calls.append(self.selected[address])
        super().on_instruction(mu,address,size,data)

    def on_write(self,mu,access,address,size,value,data):
        """실제80개 기록 DWORD와 Y→X store를 순서·폭 그대로 보존한다."""
        if self.tracing:
            for label,base in zip(("client","other"),self.spec["logs"]):  # 두 주소의 물리 대소와 의미를 혼동하지 않는다.
                if base <= address < base+160:
                    self.stores.append(f"{label}:{address-base}:{size}:{value}")
            if address in (self.x,self.y):
                self.stores.append(f"{'x' if address==self.x else 'y'}:0:{size}:{value}")
        super().on_write(mu,access,address,size,value,data)

    def observe(self,argument,control):
        """실제 할당·반납으로 양쪽20기록을 채운 뒤 원본 정리 부모 전체를 실행한다."""
        self.tracing = False
        self.begin(32768,True)
        self.step("Reset")
        for flags in (2,0):  # client5..24와 판본별 serverFirst..+19를 실제 PE에서 할당·반납한다.
            allocated = []
            for index in range(20):
                sid = self.step("Allocate",flags)[0]
                self.fill(sid,83+index,4,(83+index)&255)
                allocated.append(sid)
            for sid in allocated:
                self.step("Release",sid)
        self.write_ranges.extend((address,address+4) for address in (self.x,self.y))
        self.mu.mem_write(self.x,struct.pack("<I",0x3f123456))
        self.mu.mem_write(self.y,struct.pack("<I",0xc0123456))
        before = [bytes(self.mu.mem_read(address,160)).hex() for address in self.spec["logs"]]
        self.calls,self.stores = [],[]
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.tracing = True
        try:
            snapshot = self.step("WorldReset",argument)
        finally:
            self.tracing = False
        after = [bytes(self.mu.mem_read(address,160)).hex() for address in self.spec["logs"]]
        positions = [struct.unpack("<I",self.mu.mem_read(address,4))[0] for address in (self.x,self.y)]
        expected = [f"other:{offset}:4:0" for offset in range(0,160,4)]+[f"client:{offset}:4:0" for offset in range(0,160,4)]+[f"y:0:4:{0xc61c4000}",f"x:0:4:{0xc61c4000}"]
        if self.calls != ["wrapper","pool","history"] or self.stores != expected:
            raise RuntimeError("Pool 부모의 실제 호출/store 순서 또는 폭 차이")
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)!=0 or self.mu.reg_read(UC_X86_REG_FPTAG)!=0xffff:
            raise RuntimeError("Pool 부모의 FPU 보존 차이")
        return [*before,*snapshot,*after,*positions,",".join(self.calls),";".join(self.stores),self.instructions]


def main():
    """새 제품 부모 전에 실제 PE를 고정하며 이후 기대를 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify",action="store_true")
    args = parser.parse_args()
    digest = lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding="utf-8"))
        for relative,wanted in report["sha256"].items():  # 최초 원시 관측의 동일성만 검사한다.
            if digest(ROOT/relative)!=wanted:
                raise RuntimeError("Pool 정리 최초 SHA 차이: "+relative)
        print(json.dumps({"verified":True,"observations":report["observations"]}))
        return
    if FIXTURE.exists() or REPORT.exists() or (ROOT/"cpppj/src/o/RawSquidPoolReset.cpp").exists():
        raise RuntimeError("Pool 정리 제품 전에 최초 관측 필요 또는 기존 결과 덮어쓰기 금지")
    rows=[]
    for edition in PARENTS:  # PATCH/CD의 서로 다른 Pool 경계와 기록 주소를 각각 실제 실행한다.
        oracle = Oracle(edition)
        for argument,control in itertools.product((0,0xffffffff,0x80000000),(0x27f,0x37f)):
            rows.append("\t".join(map(str,[edition,argument,control,*oracle.observe(argument,control)])))
    with FIXTURE.open("x",encoding="utf-8",newline="\n") as stream:
        stream.write("# edition\targument\tcontrol\tbeforeClient\tbeforeOther\tresult\tfree\tcursor\tclientHead\tclientTail\totherHead\totherTail\tpoolAdler\tlogAdler\tafterClient\tafterOther\txBits\tyBits\tcalls\tstores\tinstructions\n")
        stream.write("\n".join(rows)+"\n")
    paths = ["originals/Netstorm.exe","originalCD/NETSTORM.EXE","tools/decomp_squid_pool_reset.py","tools/decomp_sid_oracle.py","tools/decomp_oracle.py",FIXTURE.relative_to(ROOT).as_posix()]
    paths += [f"extracted/refined/{e}/"+name for e,name in (("originals","Netstorm.c"),("originalCD","NETSTORM.c"))]
    report = {"host":socket.gethostname(),"lastFullDecompileHost":"HJOW-Athlon","lastFullDecompileDate":"2026-10-23","observations":len(rows),"perEdition":6,"nativeWrapperCalls":12,"nativeSelectedLowerPoolResets":12,"nativeHistoryClears":12,"nativeStoresPerCall":82,"originalGameExecuted":False,"osCalls":0,"lowerPoolBodySubstituted":False,"scope":"두 PE의 부모·기존 메모리 Pool Reset·두40DWORD·Y/X 전부 실제 CPU다. 양수 malloc 분기/할당 실패/전체 State1·Edit 진입은 별도다.","sha256":{p:digest(ROOT/p) for p in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(json.dumps({"observations":len(rows),"realLowerPoolResets":12,"storesPerCall":82,"osCalls":0}),flush=True)


if __name__ == "__main__":
    main()
