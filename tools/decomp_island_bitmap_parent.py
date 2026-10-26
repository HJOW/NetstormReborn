#!/usr/bin/env python3
"""실제004c1cc0 전체 부모와 Context/Growth/경계/보호 RLE/Bitmap을 같은 CPU에서 연결 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
import collections
from pathlib import Path
from decomp_island_bitmap import BitmapOracle, ROOT, CONTEXT, OUTPUT, PIXELS, ARENA, STOP, STACK, HELPERS, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 원본 전체 부모와 ctor/성장 helper를 현재 호스트의 읽기 전용 정밀 범위에서 선택한다.
PARENT = 0x4c1cc0
CHUNKS = ARENA + 0x70000
FIXTURE = ROOT / "cpppj/tests/fixtures/island-bitmap-parent-x86.tsv"
REPORT = ROOT / "cpppj/island-bitmap-parent-native-20261027.json"
FIELDS = ("name", "owner", "left", "top", "right", "bottom", "profile", "control", "width", "height", "trace", "pixels", "instructions")


class ParentOracle(BitmapOracle):
    """실제 초기 지형부터 Bitmap까지 실행하며 TYPE 조회와 두 heap의 주소만 공급한다."""
    def __init__(self):
        """현재 호스트의 명시 함수 범위만 추가하고 빠른 읽기 전용 허용 표를 만든다."""
        super().__init__()
        entries = {PARENT, 0x4c0390, 0x4c02d0, 0x4c09a0, 0x4e49c0}
        table = ROOT / "extracted/terraingrowth/originals/functions.tsv"
        entries.update(int(row["entry"], 16) for row in csv.DictReader(table.open(encoding="utf-8"), delimiter="\t"))
        facts = {int(row["entry"], 16): row for row in csv.DictReader((ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8"), delimiter="\t")}
        # 분석 산출물은 변경하지 않고 현재 전체 범위를 그대로 허용한다.
        for entry in entries:
            for span in facts[entry]["ranges"].split(";"):
                low, high = (int(value, 16) for value in span.split("-"))
                self.allowed.append((low, high + 1))
        self.parent_entries = entries | {0x4c1860,*HELPERS}
        self.permitted = bytearray(0x200000)
        for low, high in self.allowed:
            self.permitted[low-0x400000:high-0x400000] = b"\1" * (high-low)
        self.original = [(low, bytes(self.mu.mem_read(low, high-low))) for low,high in self.allowed]
        self.parent_active = False

    def on_instruction(self, mu, address, size, data):
        """실제 지형/bitmap 명령을 대체하지 않고 두 명시 할당과 free만 구분한다."""
        if not getattr(self, "parent_active", False):
            return super().on_instruction(mu, address, size, data)
        if address == 0x4e4391 and self.read(mu.reg_read(UC_X86_REG_ESP)+4) == 0x20840:
            self.trace.append("new:context")
            self.ret(CONTEXT)
            return
        if address == 0x4e4354:
            if self.read(mu.reg_read(UC_X86_REG_ESP)+4) != CONTEXT:
                raise RuntimeError("예상 밖 원문 context free")
            self.trace.append("free:context")
            self.ret()
            return
        if address in (0x49a840, 0x4e4391, 0x4e0620):
            return super().on_instruction(mu, address, size, data)
        self.instructions += 1
        if address in self.parent_entries:
            key = f"{address:08x}"
            self.entered[key] = self.entered.get(key, 0)+1
        offset = address-0x400000
        if offset<0 or offset+size>len(self.permitted) or not all(self.permitted[offset:offset+size]):
            raise RuntimeError(f"허용하지 않은 실제 bitmap 부모 주소:{address:08x}")

    def parent(self, case, control):
        """Native 사전 장치 준비 뒤 전체 부모를 실행하고 실제 할당·정상복귀·픽셀을 읽는다."""
        # 크기0 bitmap으로 실제 type/texture 문맥과 최초 분류를 준비한다. 전체 부모의 시간/난수/지형 기대값을 계산하지 않는다.
        self.sample(dict(name="prepare", width=0, height=0, seed=0, green=43, initialized=0, oldGreen=0, profile=0), control)
        self.mu.mem_write(CHUNKS, struct.pack("<3I", 1, 14, 14)+bytes(3072))
        for index in range(256):
            self.mu.mem_write(CHUNKS+12+index*12, bytes([127]))
        points = [] if case["profile"] == 0 else [(2,2)] if case["profile"] == 1 else [(2,2),(3,2),(2,3),(3,3)]
        # C++와 같은 물리 X*16+Y의 명시 청크 입력만 쓴다.
        for index, (x,y) in enumerate(points):
            self.mu.mem_write(CHUNKS+12+(x*16+y)*12, bytes([0,ord('A')+index,0,1,0x22,37+index,0,0])+bytes(4))
        self.put(0x594fbc, 0)
        self.put(0x50c038, 127)
        self.put(0x5411dc, 97)
        self.put(0x5411e0, 98)
        self.trace = []
        self.instructions = 0
        self.entered = {}
        self.width = (case["right"]-case["left"])*16
        self.height = (case["bottom"]-case["top"])*16
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        arguments = [STOP, OUTPUT, case["owner"], case["left"], case["top"], case["right"], case["bottom"], CHUNKS, 127]
        self.mu.mem_write(STACK, struct.pack("<9I", *arguments))
        self.parent_active = True
        try:
            self.mu.emu_start(PARENT, STOP, timeout=30000000, count=15000000)
        finally:
            self.parent_active = False
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4:
            raise RuntimeError("전체 bitmap 부모의 정상 cdecl 반환 실패")
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:
            raise RuntimeError("전체 bitmap 부모의 x87 복원 실패")
        if struct.unpack("<3I", self.mu.mem_read(OUTPUT, 12)) != (PIXELS, self.width, self.height):
            raise RuntimeError("전체 bitmap 부모의 반환 triple 불일치")
        for low, original in self.original:
            if bytes(self.mu.mem_read(low, len(original))) != original:
                raise RuntimeError("전체 bitmap 부모 원문 CODE 변경")
        return dict(case, control=control, width=self.width, height=self.height, trace=";".join(self.trace),
            pixels=bytes(self.mu.mem_read(PIXELS, self.width*self.height)).hex(), instructions=self.instructions)


def main():
    """새 전체 부모 자료만 생성하고 verify에서는 바이트를 재관측한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    cases = [dict(name="empty", owner=1, left=0, top=0, right=4, bottom=4, profile=0),
        dict(name="single", owner=1, left=0, top=0, right=4, bottom=4, profile=1),
        dict(name="neutral", owner=0, left=0, top=0, right=4, bottom=4, profile=1),
        dict(name="multiple", owner=8, left=0, top=0, right=4, bottom=4, profile=2),
        dict(name="offset", owner=1, left=1, top=1, right=5, bottom=5, profile=1),
        dict(name="full14", owner=1, left=0, top=0, right=14, bottom=14, profile=1)]
    if args.smoke:
        cases = cases[:1]
    oracle = ParentOracle()
    rows = []
    entered = collections.Counter() # 모든 실제 전체 부모 호출의 함수 진입을 누적한다.
    for case in cases:
        for control in (0x27f, 0x37f):
            rows.append(oracle.parent(case, control))
            entered.update(oracle.entered)
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter="\t", lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    payload = stream.getvalue().encode("utf-8")
    if not args.smoke:
        if args.verify:
            if FIXTURE.read_bytes()!=payload:
                raise RuntimeError("기존 전체 bitmap 부모 자료 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists():
                raise RuntimeError("기존 전체 bitmap 부모 자료 덮어쓰기 거부")
            FIXTURE.write_bytes(payload)
            REPORT.write_text(json.dumps(dict(host="HJOW-Athlon", lastFullDecompile="HJOW-Athlon / 2026-10-23", runs=len(rows), cases=len(cases),
                fixtureSha=hashlib.sha256(payload).hexdigest(), peSha=oracle.sha256,
                shapeSha=digest(oracle.shape_path), sourceSha=digest(Path(__file__)), rasterToolSha=digest(ROOT / "tools/decomp_island_bitmap.py"), entered=dict(entered),
                code=[dict(address=f"{low:08x}", size=len(data), sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original],
                limits=["004c1cc0→실제Context/Growth/MarkEdges→실제보호RLE/Bitmap/로컬난수를 실행한다.", "타입 포인터 조회와 heap/free만 명시 장치다. 캐시는 녹색43으로 사전 준비한다.", "원본 게임/GUI/월드 객체는 실행하지 않는다."]), ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(dict(runs=len(rows), fixtureSha=hashlib.sha256(payload).hexdigest(), smoke=args.smoke, verify=args.verify)))


if __name__ == "__main__":
    main()
