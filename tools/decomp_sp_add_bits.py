#!/usr/bin/env python3
"""실제 SP 가산 두 부모의 모든 float 입력 비트와 x87 정수 불확정 반환을 독립 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 기존 보상/입력 fixture를 덮지 않는 새 관측 자료다.
FIXTURE = ROOT / "cpppj/tests/fixtures/sp-add-bits-x86.tsv"
REPORT = ROOT / "cpppj/sp-add-bits-native-20261027.json"
FIELDS = ("entry", "owner", "fixed", "sp", "delta", "control", "stored", "mirror", "trace")


class AddOracle(Oracle):
    """Get/Set/지갑/표시 장치만 관측하고 실제 float 연산·ftol은 변경하지 않는다."""
    def __init__(self):
        """실제 두 가산 부모와 원본 CRT ftol만 허용한다."""
        super().__init__("originals", [{"name":"Local", "originals":"0040eec0"}, {"name":"Other", "originals":"0041df80"}], [0x4e49c0])
        self.stubs = {}
        self.original = [(lo, bytes(self.mu.mem_read(lo, hi-lo))) for lo,hi in self.allowed]

    def read(self, address):
        """원문 cdecl 인자의 DWORD를 읽는다."""
        return struct.unpack("<I", self.mu.mem_read(address,4))[0]

    def ret(self, value=0):
        """명시 장치만 cdecl로 복귀한다."""
        stack = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.read(stack))
        self.mu.reg_write(UC_X86_REG_ESP, stack+4)

    def on_instruction(self, mu, address, size, data):
        """관측 경계의 인자를 기록하며 부모의 비유한 값도 유효 입력으로 실행한다."""
        stack = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x40ec50:
            if self.read(stack+4)!=self.owner:
                raise RuntimeError("SP read owner 불일치")
            mu.mem_write(self.read(stack+8), struct.pack("<I", self.sp))
            self.trace.append("get")
            self.ret()
            return
        if address == 0x40edb0:
            if self.read(stack+4)!=self.owner:
                raise RuntimeError("SP write owner 불일치")
            self.stored = self.read(stack+8)
            self.trace.append("set")
            self.ret()
            return
        if address == 0x413bb0:
            if self.read(stack+4)!=self.owner:
                raise RuntimeError("SP wallet owner 불일치")
            self.mirror = self.read(stack+8)
            self.trace.append("wallet")
            self.ret()
            return
        if address == 0x43dad0:
            self.trace.append("refresh")
            self.ret()
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, entry, owner, fixed, sp, delta, control):
        """정상 스택/x87/CODE 보존과 실제 저장·정수 지급 비트를 반환한다."""
        self.owner,self.sp = owner,sp
        self.stored,self.mirror = None,None
        self.trace = []
        self.mu.mem_write(0x540c70, struct.pack("<I",owner))
        self.mu.mem_write(0x594fb8, struct.pack("<I",fixed&1))
        self.mu.mem_write(0x5c85a4, struct.pack("<I",fixed>>1))
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.call(entry,[delta] if entry=="Local" else [owner,delta])
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:
            raise RuntimeError("SP 가산의 x87 복원 실패")
        if self.trace!=["get","set","wallet","refresh"] or self.stored is None or self.mirror is None:
            raise RuntimeError("SP 가산의 실제 장치 순서 불일치")
        for low, original in self.original:
            if bytes(self.mu.mem_read(low,len(original)))!=original:
                raise RuntimeError("SP 가산 원문 CODE 변경")
        return dict(entry=entry,owner=owner,fixed=fixed,sp=sp,delta=delta,control=control,stored=self.stored,mirror=self.mirror,trace=";".join(self.trace))


def main():
    """정상/큰 유한/무한대/NaN/서브노멀을 같은 실제 부모에 실행한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify",action="store_true")
    args = parser.parse_args()
    oracle = AddOracle()
    rows = []
    inputs = (0,0x80000000,0x00000001,0x3f800000,0x4f000000,0x5f000000,0x6f800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc00000,0x7fa12345,0xffc12345)
    # 소유자0과 두 실제 부모/고정 모드 모두 같은 입력 비트로 교차한다.
    for entry,owner in (("Local",1),("Other",0),("Other",3)):
        for fixed in (0,1,2):
            for sp in inputs:
                for delta in (0,0x3fa00000):
                    for control in (0x27f,0x37f):
                        rows.append(oracle.sample(entry,owner,fixed,sp,delta,control))
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream,fieldnames=FIELDS,delimiter="\t",lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    payload = stream.getvalue().encode("utf-8")
    if args.verify:
        if FIXTURE.read_bytes()!=payload:
            raise RuntimeError("SP 가산 비트의 독립 원문 불일치")
    else:
        if FIXTURE.exists() or REPORT.exists():
            raise RuntimeError("기존 SP 가산 관측 덮어쓰기 거부")
        FIXTURE.write_bytes(payload)
        REPORT.write_text(json.dumps(dict(host="HJOW-Athlon",lastFullDecompile="HJOW-Athlon / 2026-10-23",runs=len(rows),
            fixtureSha=hashlib.sha256(payload).hexdigest(),peSha=oracle.sha256,sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            code=[dict(address=f"{low:08x}",size=len(data),sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original],
            limits=["실제0040eec0/0041df80 및 원본 ftol을 실행한다.","SP store Get/Set과 지갑·표시는 명시 장치이며 실제 원문 호출 순서/인자를 대조한다.","원본 파일/기존 보상 fixture/게임/GUI를 변경하거나 실행하지 않는다."]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(runs=len(rows),verify=args.verify,fixtureSha=hashlib.sha256(payload).hexdigest())))


if __name__=="__main__":
    main()
