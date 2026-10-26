#!/usr/bin/env python3
"""원본004c1860과 보호 SHP의 실제 RLE를 격리 CPU에서 실행해 섬 미리보기 픽셀을 기록한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
import sys
from pathlib import Path

# 원본 파일과 선택 분석 의존성은 읽기 전용이다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "extracted/oracle-python"))
from decomp_oracle import Oracle, STOP, STACK
from shp import read_blocks
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_GDTR, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_FS, UC_X86_REG_GS

# 실제 context와 명시 할당/타입 조회 경계의 격리 주소다.
ARENA = 0x11000000
CONTEXT, TYPES, OUTPUT, PIXELS, SEED, GDT = [ARENA + value for value in (0, 0x30000, 0x32000, 0x40000, 0x60000, 0x61000)]
SHAPES = 0x14000000
HELPERS = [0x4a2b10, 0x4a2c70, 0x4daa70, 0x401d92, 0x4021b5, 0x4bca20] # VFX의 전체 내부 fast 경로도 원문대로 실행한다.
FIXTURE = ROOT / "cpppj/tests/fixtures/island-bitmap-x86.tsv"
REPORT = ROOT / "cpppj/island-bitmap-native-20261027.json"
FIELDS = ("name", "width", "height", "seed", "green", "initialized", "oldGreen", "profile", "control", "seedOut", "classX", "tail", "trace", "pixels", "instructions")


def digest(path):
    """출처와 기대 자료의 실제 바이트 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def codes(profile):
    """출력 기대값 없이 X열 우선 문자의 명시 입력만 만든다."""
    result = bytearray(b"." * 65536)
    # 같은 전체256×256 배열에 서로 다른 경계·수직 fringe·끝줄 입력을 놓는다.
    for x in range(64):
        for y in range(64):
            if profile == 1:
                value = "x" if 2 <= x < 13 and 2 <= y < 11 else "=" if 2 <= x < 13 and y == 11 else "."
            elif profile == 2:
                value = "x=..Xo"[(x * 7 + y) % 6]
            elif profile == 3:
                value = "x" if y % 8 == 0 else "=" if y % 8 in (1, 2) else "."
            elif profile == 4:
                value = "x" if y == 0 else "=" if y == 1 else "."
            else:
                value = "."
            result[x * 256 + y] = ord(value)
    return bytes(result)


class BitmapOracle(Oracle):
    """타입 포인터 조회와 heap만 공급하며 문맥 저장/실제 texture RLE/래스터/난수는 원문을 실행한다."""
    def __init__(self):
        """원본 bitmap과 helper의 허용 범위·보호 그림·32비트 평면 GDT를 준비한다."""
        super().__init__("originals", [{"name": "Raster", "originals": "004c1860"}], HELPERS)
        self.stubs = {}
        self.mu.mem_map(0, 4096)
        self.put(0, 0xffffffff) # SEH의 FS:[0]만 격리 입력으로 공급한다. OS 예외 처리는 실행하지 않는다.
        self.mu.mem_map(ARENA, 0x100000)
        self.shape_path = ROOT / "originals/d/_shapes.shp"
        self.shape_bytes = self.shape_path.read_bytes()
        self.blocks = read_blocks(self.shape_bytes)
        self.mu.mem_map(SHAPES, (len(self.shape_bytes) + 4095) & ~4095)
        self.mu.mem_write(SHAPES, self.shape_bytes)
        self.original = [(lo, bytes(self.mu.mem_read(lo, hi-lo))) for lo, hi in self.allowed]
        self.entered = {}
        self.mu.mem_write(GDT, struct.pack("<3Q", 0, 0x00cf93000000ffff, 0x00cf9b000000ffff))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 23, 0))
        self.mu.reg_write(UC_X86_REG_CS, 16)
        # VFX의 push ds/pop es가 실제32비트 평면 주소를 유지한다.
        for register in (UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_FS, UC_X86_REG_GS):
            self.mu.reg_write(register, 8)

    def put(self, address, value):
        """명시 장치의 DWORD 입력만 쓰며 원문 CODE에는 쓰지 않는다."""
        self.mu.mem_write(address, struct.pack("<I", value & 0xffffffff))

    def read(self, address):
        """정렬되지 않은 실제 DWORD를 읽는다."""
        return struct.unpack("<I", self.mu.mem_read(address, 4))[0]

    def ret(self, value=0):
        """cdecl 명시 장치만 원문 호출자로 복귀한다."""
        stack = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.read(stack))
        self.mu.reg_write(UC_X86_REG_ESP, stack + 4)

    def on_instruction(self, mu, address, size, data):
        """할당/타입 조회 이외의 실제 texture와 bitmap 명령은 대체하지 않는다."""
        self.instructions += 1
        if address in (0x4c1860, *HELPERS):
            key = f"{address:08x}"
            self.entered[key] = self.entered.get(key, 0) + 1
        if address == 0x49a840:
            number = self.read(mu.reg_read(UC_X86_REG_ESP) + 4)
            if number not in (97, 102):
                raise RuntimeError("예상 밖 bitmap texture TYPE")
            self.trace.append("type:" + str(number))
            self.ret(TYPES + (number-97) * 512)
            return
        if address == 0x4e4391:
            count = self.read(mu.reg_read(UC_X86_REG_ESP) + 4)
            if count != self.width * self.height:
                raise RuntimeError("실제 bitmap 할당 크기 불일치")
            self.trace.append("new:" + str(count))
            self.ret(PIXELS)
            return
        if address == 0x4e0620:
            raise RuntimeError("원문 bitmap 진단")
        super().on_instruction(mu, address, size, data)

    def sample(self, case, control):
        """전체 픽셀/로컬 seed/분류 캐시/반환 폭과 CODE·문맥 보존을 대조한다."""
        self.instructions = 0
        self.trace = []
        self.width, self.height = case["width"], case["height"]
        self.mu.mem_write(CONTEXT, bytes(0x20840))
        self.mu.mem_write(CONTEXT + 8, codes(case["profile"]))
        self.mu.mem_write(TYPES, bytes(4096))
        # 현재 TYPE의 실제 SHP 블록 포인터와 유효 frameCheck 자료만 공급한다.
        for number in (97, 102):
            base = TYPES + (number-97) * 512
            self.put(base + 0xdc, SHAPES + self.blocks[number-70].offset)
            self.put(base + 0x114, 1)
        self.mu.mem_write(PIXELS, b"\xa5" * 65536)
        self.mu.mem_write(OUTPUT, b"\xa5" * 12)
        self.put(SEED, case["seed"])
        self.mu.mem_write(0x5c8ab0, bytes(256))
        self.mu.mem_write(0x5c8b28, bytes([case["oldGreen"]]))
        self.mu.mem_write(0x5c8baf, b"\xa5")
        self.put(0x5c8bb8, case["initialized"])
        self.put(0x5acd20, case["green"])
        # 본문이 저장/복구하는 실제18DWORD와 화면 전역의 안정된 초기 문맥이다.
        self.mu.mem_write(0x59af88, bytes(72))
        self.mu.mem_write(0x5b5ddc, bytes(20))
        self.mu.mem_write(0x5c7a44, bytes(16))
        self.put(0x5c78d4, 0)
        self.put(0x5c78f8, 0)
        for address in (0x5c78f0, 0x5c78ec, 0x5c78f4):
            self.put(address, 0)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ECX, CONTEXT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack("<5I", STOP, OUTPUT, self.width, self.height, SEED))
        try:
            self.mu.emu_start(0x4c1860, STOP, timeout=10000000, count=5000000)
        except Exception as error:
            raise RuntimeError(f"bitmap CPU 실패 eip={self.mu.reg_read(UC_X86_REG_EIP):08x} esp={self.mu.reg_read(UC_X86_REG_ESP):08x} trace={self.trace}") from error
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 20:
            raise RuntimeError("bitmap 정상 ret16 실패")
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError("bitmap x87 복귀 실패")
        if struct.unpack("<3I", self.mu.mem_read(OUTPUT, 12)) != (PIXELS, self.width, self.height):
            raise RuntimeError("bitmap 실제 반환 triple 불일치")
        if bytes(self.mu.mem_read(0x59af88, 72)) != bytes(72) or bytes(self.mu.mem_read(0x5b5ddc, 20)) != b"\0"*12 + b"\xff"*8:
            raise RuntimeError("texture 문맥 복원 불일치")
        for lo, original in self.original:
            if bytes(self.mu.mem_read(lo, len(original))) != original:
                raise RuntimeError("원문 bitmap CODE 변경")
        return dict(case, control=control, seedOut=self.read(SEED), classX=self.mu.mem_read(0x5c8b28, 1)[0], tail=self.mu.mem_read(0x5c8baf, 1)[0],
            trace=";".join(self.trace), pixels=bytes(self.mu.mem_read(PIXELS, self.width*self.height)).hex() or "-", instructions=self.instructions)


def cases():
    """정상 크기·빈 출력·분류 캐시·아래 끝 fringe와 서로 다른 난수 입력을 분리한다."""
    base = dict(name="normal", width=16, height=16, seed=4323, green=43, initialized=0, oldGreen=0, profile=1)
    result = [base]
    # 정상 픽셀 외에도 현재 분류 최초값과 유지된 캐시를 구분한다.
    for name, changes in (("empty", dict(profile=0)), ("mixed", dict(width=33, height=35, profile=2)),
        ("vertical", dict(width=19, height=20, profile=3)), ("bottom", dict(width=8, height=2, profile=4)),
        ("single", dict(width=1, height=1)), ("zeroWidth", dict(width=0)), ("zeroHeight", dict(height=0)),
        ("greenZero", dict(green=0)), ("greenLowByte", dict(green=299)),
        ("cachedZero", dict(initialized=1, oldGreen=0)), ("cached", dict(initialized=1, oldGreen=77)),
        ("seedZero", dict(seed=0)), ("seedFull", dict(seed=0xffffffff))):
        result.append(dict(base, name=name, **changes))
    return result


def main():
    """새 자료만 생성하고 verify에서는 원본을 재실행해 바이트를 비교한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    oracle = BitmapOracle()
    selected = cases()[:1] if args.smoke else cases()
    rows = []
    # 두 x87 제어 모드에서도 같은 실제 bitmap 결과를 관찰한다.
    for case in selected:
        for control in (0x27f, 0x37f):
            rows.append(oracle.sample(case, control))
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter="\t", lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    payload = stream.getvalue().encode("utf-8")
    if not args.smoke:
        if args.verify:
            if FIXTURE.read_bytes() != payload:
                raise RuntimeError("기존 bitmap fixture와 독립 원문 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists():
                raise RuntimeError("기존 bitmap 자료 덮어쓰기 거부")
            FIXTURE.write_bytes(payload)
            REPORT.write_text(json.dumps(dict(host="HJOW-Athlon", lastFullDecompile="HJOW-Athlon / 2026-10-23", runs=len(rows),
                cases=len(selected), peSha=oracle.sha256, shapeSha=digest(oracle.shape_path), sourceSha=digest(Path(__file__)), fixtureSha=hashlib.sha256(payload).hexdigest(),
                entered=oracle.entered, code=[dict(address=f"{lo:08x}", size=len(data), sha=hashlib.sha256(data).hexdigest()) for lo,data in oracle.original],
                limits=["실제 bitmap·로컬 난수·texture200×200 저장/복구·보호97/102 SHP의 RLE를 실행한다.", "TYPE 조회 포인터와 bitmap heap만 명시 장치다.", "지형 Context/Growth 전체 부모와 전용 Gump/GUI는 이 관측 범위 밖이다."]), ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(dict(runs=len(rows), fixtureSha=hashlib.sha256(payload).hexdigest(), verify=args.verify, smoke=args.smoke)))


if __name__ == "__main__":
    main()
