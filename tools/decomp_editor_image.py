#!/usr/bin/env python3
"""Image 변환/복사/이중 소멸과 실제 VFX 인코더를 원본 CPU에서 실행한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
import sys
from pathlib import Path

# 읽기 전용 원본과 격리 분석 의존성의 위치다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "extracted/oracle-python"))
from decomp_oracle import Oracle, STOP, STACK
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_GDTR, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS

# 포인터 값은 명시 heap 장치의 격리 주소이며 원본 CODE와 겹치지 않는다.
ARENA, IMAGE, COPY, INPUT, GDT = 0x11000000, 0x11000000, 0x11000100, 0x11010000, 0x11000200
HEAP = 0x11200000
ENTRIES = (0x46d790, 0x46d8a0, 0x46d900, 0x4dad20, 0x4042cd, 0x40463b, 0x4047ab, 0x4437f0)
FIXTURE = ROOT / "cpppj/tests/fixtures/editor-image-x86.tsv"
REPORT = ROOT / "cpppj/editor-image-native-20261027.json"
FIELDS = ("name", "width", "height", "crop", "input", "control", "outWidth", "outHeight", "size", "shape", "copyShape", "destroyed", "trace", "instructions")


class ImageOracle(Oracle):
    """heap 주소만 공급하고 자르기/헤더/토큰/복사/해제를 실제 몸체로 실행한다."""
    def __init__(self):
        """정밀 함수 범위와 VFX의 평면32비트 ES 문맥을 준비한다."""
        super().__init__("originals", [{"name": "Convert", "originals": "0046d790"}], list(ENTRIES[1:]))
        self.stubs = {}
        self.mu.mem_map(ARENA, 0x400000)
        self.mu.mem_write(GDT, struct.pack("<3Q", 0, 0x00cf93000000ffff, 0x00cf9b000000ffff))
        self.mu.reg_write(UC_X86_REG_GDTR, (0, GDT, 23, 0))
        self.mu.reg_write(UC_X86_REG_CS, 16)
        # 실제 push ds/pop es가 사용하는 데이터 세그먼트다.
        for register in (UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS):
            self.mu.reg_write(register, 8)
        self.original = [(low, bytes(self.mu.mem_read(low, high-low))) for low, high in self.allowed]
        self.entered = {}

    def read(self, address):
        """원문 비정렬 DWORD를 읽는다."""
        return struct.unpack("<I", self.mu.mem_read(address, 4))[0]

    def on_instruction(self, mu, address, size, data):
        """원문 malloc/free 경계만 기록하며 계산 결과를 주입하지 않는다."""
        self.instructions += 1
        if address in ENTRIES:
            key = f"{address:08x}"
            self.entered[key] = self.entered.get(key, 0)+1
        if address in (0x4e4391, 0x4e4354, 0x43fa80, 0x43fa90):
            stack = mu.reg_read(UC_X86_REG_ESP)
            argument = self.read(stack+4)
            value = 0
            if address in (0x4e4391, 0x43fa80):
                if argument > 0x100000:
                    raise RuntimeError("Image 실제 할당 상한 초과")
                value = self.next_heap
                self.next_heap += (argument+15) & ~15
                self.trace.append("alloc:"+str(argument))
                self.allocations[value] = argument
                self.mu.mem_write(value, b"\xa5"*argument)
            else:
                if argument not in self.allocations or argument in self.freed:
                    raise RuntimeError("Image 잘못된/중복 free")
                self.freed.add(argument)
                self.trace.append("free:"+str(self.allocations[argument]))
            mu.reg_write(UC_X86_REG_EAX, value)
            mu.reg_write(UC_X86_REG_EIP, self.read(stack))
            mu.reg_write(UC_X86_REG_ESP, stack+4)
            return
        super().on_instruction(mu, address, size, data)

    def invoke(self, entry, target, arguments, control):
        """실제 thiscall의 ret N과 x87/CODE 불변을 확인한다."""
        self.mu.reg_write(UC_X86_REG_ECX, target)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack("<"+"I"*(len(arguments)+1), STOP, *arguments))
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.emu_start(entry, STOP, timeout=10000000, count=12000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4*(len(arguments)+1):
            raise RuntimeError("Image 실제 정상 반환 실패")
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError("Image x87 복귀 실패")

    def sample(self, case, control):
        """입력을 소비하는 변환과 독립 deep copy/소멸의 전체 바이트를 기록한다."""
        pixels = bytes.fromhex(case["input"])
        self.instructions, self.trace, self.freed = 0, [], set()
        self.next_heap, self.allocations = HEAP, {INPUT: len(pixels)}
        self.mu.mem_write(IMAGE, bytes(16)+b"\xa5"*4)
        self.mu.mem_write(COPY, bytes(16)+b"\xa5"*4)
        self.mu.mem_write(INPUT, pixels)
        self.invoke(0x46d790, IMAGE, [INPUT, case["width"], case["height"], case["crop"]], control)
        pointer, shape, width, height, count = struct.unpack("<5I", self.mu.mem_read(IMAGE, 20))
        if pointer != 0 or INPUT not in self.freed or self.allocations.get(shape) != count:
            raise RuntimeError("Image 실제 입력 소비/shape 할당 불일치")
        payload = bytes(self.mu.mem_read(shape, count))
        self.invoke(0x46d900, COPY, [IMAGE], control)
        copied = self.read(COPY+4)
        copy_payload = bytes(self.mu.mem_read(copied, count))
        if copied == shape or bytes(self.mu.mem_read(COPY+8, 12)) != bytes(self.mu.mem_read(IMAGE+8, 12)):
            raise RuntimeError("Image 독립 복사 불일치")
        # 원문처럼 두 번 소멸해도 free는 각 버퍼에 한 번만 일어난다.
        for target in (IMAGE, IMAGE, COPY, COPY):
            self.invoke(0x46d8a0, target, [], control)
        destroyed = bytes(self.mu.mem_read(IMAGE, 20))
        if destroyed[:16] != bytes(16) or self.freed != set(self.allocations):
            raise RuntimeError("Image 소멸 후 필드/heap 불일치")
        # 입력과 원문 전역 출력은 별개이며 실제 코드에는 쓰지 않았다.
        for low, original in self.original:
            if bytes(self.mu.mem_read(low, len(original))) != original:
                raise RuntimeError("Image 원문 CODE 변경")
        return dict(case, control=control, outWidth=width, outHeight=height, size=count, shape=payload.hex(), copyShape=copy_payload.hex(), destroyed=destroyed.hex(), trace=";".join(self.trace), instructions=self.instructions)


def cases():
    """기대 출력 없이 경계 토큰/후보 bitmap/작은 모든 삼색 조합 입력을 준비한다."""
    result = []
    # 전부 빈 입력도 원문 Image는 최소1×1 크기와 특수 empty shape를 만든다.
    for name, width, height, pixels in (("empty", 7, 5, bytes(35)), ("one", 1, 1, b"\x07"),
        ("offset", 8, 7, bytes(3*8+5)+b"\xff"+bytes(56-30)),
        ("border", 9, 6, bytes([0,1,1,0,2,3,4,0,0])*6)):
        for crop in (0,1):
            result.append(dict(name=f"{name}-{crop}", width=width, height=height, crop=crop, input=pixels.hex()))
    # 127 literal/repeat와255 skip 경계의 분할을 앞뒤 문맥 모두에서 관찰한다.
    for count in (2,3,126,127,128,254,255,256,511):
        for profile in range(4):
            middle = bytes([7])*count if profile==0 else bytes(1+i%254 for i in range(count)) if profile==1 else bytes(count) if profile==2 else bytes([255,255,1])*count
            pixels = b"\x03\x04"+middle+b"\x08"
            result.append(dict(name=f"run-{count}-{profile}", width=len(pixels), height=1, crop=1, input=pixels.hex()))
    # 행 시작 pair와 literal 내부 triple의 차이를 모두 포함한다.
    for value in range(3**6):
        number, pixels = value, bytearray()
        for index in range(6):
            pixels.append((0,7,255)[number%3])
            number //= 3
        result.append(dict(name=f"ternary-{value}", width=6, height=1, crop=value%2, input=pixels.hex()))
    # 앞 단위가 실제 원본 Context/Growth/RLE로 만든 전체 bitmap도 직접 입력한다.
    source = ROOT / "cpppj/tests/fixtures/island-bitmap-parent-x86.tsv"
    for row in csv.DictReader(source.open(encoding="utf-8"), delimiter="\t"):
        if int(row["control"]) != 0x27f:
            continue
        for crop in (0,1):
            result.append(dict(name=f"bitmap-{row['name']}-{crop}", width=int(row["width"]), height=int(row["height"]), crop=crop, input=row["pixels"]))
    return result


def main():
    """새 fixture만 생성하며 verify는 실제 기계어를 다시 실행한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    arguments = parser.parse_args()
    oracle, selected = ImageOracle(), cases()
    if arguments.smoke:
        selected = selected[:2]
    rows = []
    for case in selected: # 두 x87 모드는 byte 인코딩과 수명 결과를 별도 관측한다.
        for control in (0x27f, 0x37f):
            rows.append(oracle.sample(case, control))
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter="\t", lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    payload = stream.getvalue().encode("utf-8")
    if not arguments.smoke:
        if arguments.verify:
            if FIXTURE.read_bytes() != payload:
                raise RuntimeError("Image 독립 원문/fixture 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists():
                raise RuntimeError("Image 기존 증거 덮어쓰기 거부")
            FIXTURE.write_bytes(payload)
            REPORT.write_text(json.dumps(dict(host="HJOW-Athlon", lastFullDecompile="HJOW-Athlon / 2026-10-23", runs=len(rows), cases=len(selected), peSha=oracle.sha256,
                sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), fixtureSha=hashlib.sha256(payload).hexdigest(), entered=oracle.entered,
                code=[dict(address=f"{low:08x}", bytes=len(data), sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original],
                limits=["0046d790→004dad20→004042cd→0040463b/004047ab의 실제 crop/헤더/토큰을 실행한다.", "실제0046d900 deep copy와0046d8a0 이중 소멸을 실행한다. malloc/free 주소만 명시 장치다.", "10.78 변환된 shape를 관측하며 raw imageBits 복사와 MetaImage 가상 함수/GUI는 이 자료 범위 밖이다."]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(runs=len(rows), cases=len(selected), fixtureSha=hashlib.sha256(payload).hexdigest(), smoke=arguments.smoke, verify=arguments.verify)))


if __name__ == "__main__":
    main()
