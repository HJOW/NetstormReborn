#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""MetaZone 마커00479520·Zone6BYTE 정리004bfd60의 실제 PE 몸체를 먼저 관측한다."""
import argparse
import csv
import itertools
import json
import socket
from collections import Counter
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 보호 이미지의 고정 기록·Pool 포인터와 격리된 가상 함수 장치 주소다.
TABLE, RECORD_BYTES, TABLE_BYTES, POOL_POINTER = 0x546908, 144, 7344, 0x5c8464
MARKER, ZONE, VIRTUAL, ZONE_BYTES = 0x479520, 0x4bfd60, STOP+0x100, TEXT+0x800
FIXTURES = {"marker": ROOT/"cpppj/tests/fixtures/meta-zone-marker-x86.tsv", "zone": ROOT/"cpppj/tests/fixtures/state-zone-reset-x86.tsv"}
REPORT = ROOT/"cpppj/state-storage-leaves-native-20261026.json"


class StopVirtual(Exception):
    """가상 삭제 장치가 명시 중단한 경우이며 원문 오류로 간주하지 않는다."""


class Oracle(MemoryOracle):
    """두 작은 원문은 그대로 실행하고 실제 가상+10 호출만 관측 장치로 둔다."""
    def __init__(self):
        """같은 보호 PE·고정 표·별도 Pool에 CPU 쓰기 감사를 붙인다."""
        super().__init__()
        facts = {int(r["entry"],16): r for r in csv.DictReader((ROOT/"extracted/refined/originals/functions.tsv").open(encoding="utf-8"), delimiter="\t")}
        self.ranges = {entry: (entry, entry+int(facts[entry]["size"])) for entry in (MARKER, ZONE)}
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """스택 외 CPU store의 실제 폭·순서·값을 관측하며 가상 장치의 쓰기와 분리한다."""
        if not self.running or STACK-4096 <= address < STACK+64:
            return
        self.stores.append((address-self.base,size,value))
        wanted = self.base+(76 if self.entry == MARKER else (0 if not self.stores[:-1] else 4))
        width = 4 if self.entry == MARKER or len(self.stores)==1 else 2
        if address != wanted or size != width or value != 0:
            raise RuntimeError("State 작은 정리의 예상 밖 CPU store: "+str(self.stores))

    def instruction(self, mu, address, size, data):
        """가상 this/flags와 복귀 뒤 SID0 쓰기만 관측하고 다른 원문 함수는 허용하지 않는다."""
        self.instructions += 1
        if self.instructions > 100:
            raise RuntimeError("State 작은 정리의 명령 상한")
        if address == VIRTUAL:
            this = self.mu.reg_read(UC_X86_REG_ECX)
            flags = self.words(self.mu.reg_read(UC_X86_REG_ESP)+4,1)[0]
            self.calls.append((this,flags))
            if self.mutation == "stop":
                raise StopVirtual()
            if self.mutation == "sid":
                self.put(self.base+76,0x76543210)
            elif self.mutation == "status":
                self.put(self.base,0x89abcdef)
            elif self.mutation != "none":
                raise RuntimeError("State 작은 정리의 미등록 관측 입력")
            self.ret(0,4)
            return
        low, high = self.ranges[self.entry]
        if not low <= address < high:
            raise RuntimeError("State 작은 정리 외 실행: "+hex(address))

    def mapped(self, address):
        """DWORD SID의 원문32비트 주소 산술 결과에만 격리한 슬롯 페이지를 준비한다."""
        for page in (address&~4095, (address+3)&~4095):  # 페이지 경계의 DWORD도 원시 입력으로 제공한다.
            try:
                self.mu.mem_read(page,1)
            except UcError:
                self.mu.mem_map(page,4096)

    def observe(self, kind, seed, index, status, marker, mutation, control):
        """독립 명시 입력에서 실제 CPU 반환·전역 표·this·store를 기록한다."""
        self.running = False
        self.entry = MARKER if kind == "marker" else ZONE
        self.base = TABLE+index*RECORD_BYTES if kind == "marker" else ZONE_BYTES
        self.instructions = 0
        self.calls, self.stores, self.mutation = [], [], mutation
        size = TABLE_BYTES+16 if kind == "marker" else 24
        begin = TABLE if kind == "marker" else ZONE_BYTES
        raw = bytes((seed+byte*17)&255 for byte in range(size))
        self.mu.mem_write(begin,raw)
        if kind == "marker":
            self.put(self.base,status)
            self.put(self.base+76,marker)
            self.put(POOL_POINTER,POOL)
            slot = (POOL+marker*50)&0xffffffff
            self.mapped(slot)
            self.put(slot,TEXT)
            self.put(TEXT+16,VIRTUAL)
        self.mu.mem_write(STACK-4096,b"\xcd"*(4096+64))
        self.put(STACK,STOP)
        self.put(STACK+4,index)
        registers = ((UC_X86_REG_EAX,0x89abcdef),(UC_X86_REG_ECX,self.base),(UC_X86_REG_EDX,0x3456789a),(UC_X86_REG_EBX,0x456789ab),(UC_X86_REG_ESI,0x56789abc),(UC_X86_REG_EDI,0x6789abcd),(UC_X86_REG_EBP,0x789abcde))
        for register,value in registers:  # 비휘발 보존을 따로 대조하는 원시 CPU 입력이다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        outcome = "normal"
        self.running = True
        try:
            self.mu.emu_start(self.entry,STOP)
        except StopVirtual:
            outcome = "stop"
        finally:
            self.running = False
        if outcome == "normal":
            if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:
                raise RuntimeError("State 작은 정리의 정상 복귀 차이")
            for register,value in registers[3:]:  # 호출자가 보존해야 할 EBX/ESI/EDI/EBP다.
                if self.mu.reg_read(register)!=value:
                    raise RuntimeError("State 작은 정리의 비휘발 레지스터 차이")
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)!=0 or self.mu.reg_read(UC_X86_REG_FPTAG)!=0xffff:
            raise RuntimeError("State 작은 정리의 FPU 보존 차이")
        actual = bytes(self.mu.mem_read(begin,size))
        if actual[-16:] != raw[-16:]:
            raise RuntimeError("State 작은 정리의 인접 canary 변경")
        return outcome, actual.hex(), ";".join(f"{a}:{f}" for a,f in self.calls) or "-", ";".join(f"{a}:{w}:{v}" for a,w,v in self.stores) or "-", str(self.instructions)


def main():
    """새 제품 몸체 전에 최초 입력·기대값을 고정하고 이후 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify",action="store_true")
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding="utf-8"))
        for relative,wanted in report["sha256"].items():  # 기존 최초 관측의 바이트만 대조한다.
            if digest(ROOT/relative)!=wanted:
                raise RuntimeError("State 작은 정리 최초 SHA 차이: "+relative)
        print(json.dumps({"verified":True,"observations":report["observations"]}))
        return
    if REPORT.exists() or any(p.exists() for p in FIXTURES.values()):
        raise RuntimeError("State 작은 정리 최초 관측 덮어쓰기 금지")
    if "ClearMetaZoneMarker" in (ROOT/"cpppj/src/o/RawMetaZoneCleanup.h").read_text(encoding="utf-8"):
        raise RuntimeError("새 제품 마커 몸체 전에 최초 관측 필요")
    oracle = Oracle()
    inputs = [("marker",165,index,status,marker,"none") for index,status,marker in itertools.product((0,50),(0,1,0x7fffffff,0x80000000,0xffffffff),(0,5,0x10000,0xffffffff))]
    inputs += [("marker",83,25,1,0x123,mutation) for mutation in ("sid","status","stop")]
    inputs += [("zone",seed,0,0,0,"none") for seed in (0,1,83,165,255)]
    rows = {kind: [] for kind in FIXTURES}
    outcomes = Counter()
    for item,control in itertools.product(inputs,(0x27f,0x37f)):  # x87 정밀도에 관계없는 정수 몸체도 두 환경에서 실제 실행한다.
        result = oracle.observe(*item,control)
        kind,seed,index,status,marker,mutation = item
        row = [str(v) for v in (seed,index,status,marker,mutation,control,*result)]
        rows[kind].append("\t".join(row))
        outcomes[result[0]]+=1
    for kind,path in FIXTURES.items():  # 실제 CPU 관측이 전부 끝난 뒤 최초 기대를 한 번만 기록한다.
        with path.open("x",encoding="utf-8",newline="\n") as stream:
            stream.write("# seed\tindex\tstatus\tmarker\tmutation\tcontrol\toutcome\traw\tvirtualThisFlags\tstores\tinstructions\n")
            stream.write("\n".join(rows[kind])+"\n")
    sources = ["originals/Netstorm.exe","extracted/refined/originals/Netstorm.c","extracted/refined/originals/functions.tsv", "tools/decomp_state_storage_leaves.py",*(p.relative_to(ROOT).as_posix() for p in FIXTURES.values())]
    report = {"host":socket.gethostname(),"lastFullDecompileHost":"HJOW-Athlon","lastFullDecompileDate":"2026-10-23","observations":{k:len(v) for k,v in rows.items()},"statuses":dict(outcomes),"entrySizes":{f"{e:08x}":high-low for e,(low,high) in oracle.ranges.items()},"originalGameExecuted":False,"osCalls":0,"virtualDestroyBoundary":True,"scope":"PATCH 실제63BYTE marker·9BYTE Zone와 가상 this/flags·실제 store 폭/순서다. 가상 삭제 자식은 관측 장치이며 전체 MetaMap/Native State1/제품 Edit 진입 완료가 아니다.","sha256":{p:digest(ROOT/p) for p in sources}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(json.dumps({"observations":report["observations"],"statuses":dict(outcomes),"osCalls":0}))


if __name__ == "__main__":
    main()
