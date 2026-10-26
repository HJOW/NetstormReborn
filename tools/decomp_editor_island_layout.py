#!/usr/bin/env python3
"""섬 추가 창의 실제 배치/마우스 미리보기 몸체를 격리 CPU에서 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path
from decomp_meta_image import MetaOracle, ROOT, ARENA, STOP, bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

# 원문 창/일곱 자식/드래그와 가상 장치 주소는 CODE 밖에서만 준비한다.
WINDOW, CHILDREN, DRAG, IMAGE, VTABLE = (ARENA+x for x in (0x2000,0x4000,0x5000,0x6000,0x7000))
ENTRIES = (0x47d280,0x47d5f0,0x46d9f0,0x46da20,0x431fc0,0x46ee50,0x4bd300,0x40d900,0x41cc20)
FIXTURE = ROOT / "cpppj/tests/fixtures/editor-island-layout-x86.tsv"
REPORT = ROOT / "cpppj/editor-island-layout-native-20261027.json"
FIELDS = ("op","width","height","imageWidth","imageHeight","scale","x","y","canPlace","effect","control","geometry","record","color","depth","trace","instructions")


class WindowOracle(MetaOracle):
    """배치/절삭/원문 Builder는 실행하고 자식 가상 효과와 충돌 반환만 명시한다."""
    def __init__(self):
        """마지막 정밀 디컴파일의 전체 함수 범위를 추가한다."""
        super().__init__()
        facts={int(row["entry"],16):row for row in csv.DictReader((ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8"),delimiter="\t")}
        for entry in ENTRIES: # CODE를 바꾸지 않고 실제 분리 범위를 모두 허용한다.
            for part in facts[entry]["ranges"].split(";"): # 원문 범위의 끝은 포함 주소다.
                low,high=(int(value,16) for value in part.split("-"));self.allowed.append((low,high+1))
        self.original=[(low,bytes(self.mu.mem_read(low,high-low))) for low,high in self.allowed]
        self.window_entries=set(ENTRIES)

    def args(self,count):
        """현재 thiscall의 실제 스택 DWORD 인자를 읽는다."""
        return struct.unpack("<"+"I"*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def label(self,address):
        """주소만 논리 창/자식 번호로 정규화한다."""
        return "root" if address==WINDOW else "drag" if address==DRAG else str((address-CHILDREN)//0x100)

    def on_instruction(self,mu,address,size,data):
        """원문 부모 산술과 현재 필드 재조회를 가상 호출 경계에서 관측한다."""
        target=mu.reg_read(UC_X86_REG_ECX)
        if address in self.window_entries:
            key=f"{address:08x}";self.entered[key]=self.entered.get(key,0)+1
        if address in (0x466750,STOP+0x104):
            x,y,parent,mode=self.args(4);label=self.label(target)
            self.trace.append(f"attach:{label}:{x}:{y}:{self.label(parent) if parent else '-'}:{mode}")
            self.put(target+0x4c,x);self.put(target+0x50,y)
            if target==WINDOW and self.case["effect"]==1:
                self.put(WINDOW+0x3c,bits(500.25));self.put(WINDOW+0x40,bits(480.75))
            if target==CHILDREN and self.case["effect"]==3:
                self.put(CHILDREN+0x3c,bits(199.25));self.put(CHILDREN+0x40,bits(201.75));self.put(CHILDREN+4*0x100+0x40,bits(19.25))
            self.ret(16);return
        if address==STOP+0x100:
            self.trace.append("push:"+self.label(target))
            if target==CHILDREN+4*0x100 and self.case["effect"]==2:
                self.put(CHILDREN+0x3c,bits(241.25));self.put(CHILDREN+0x40,bits(253.5));self.put(CHILDREN+2*0x100+0x40,bits(29.75))
            if target==DRAG and self.case["effect"]==4:
                self.put(DRAG+0x3c,bits(33.75));self.put(DRAG+0x40,bits(51.25))
            self.ret();return
        if address==0x4648e0:
            self.mu.mem_write(self.args(1)[0],struct.pack("<4i",100,-37,324,187));self.trace.append("bounds");self.ret(4);return
        if address==0x464740:
            self.mu.mem_write(self.args(1)[0],struct.pack("<2i",103,-41));self.trace.append("position");self.ret(4);return
        if address==0x46de50:
            state=struct.unpack("<6I",mu.mem_read(target,24));args=self.args(2)
            self.trace.append("can:"+":".join(map(str,(*state[1:],args[0],args[1]&0xffff))))
            self.ret(8);mu.reg_write(UC_X86_REG_EAX,self.case["canPlace"]);return
        if address==0x4e0620: raise RuntimeError("창 배치의 원문 진단 입력 밖")
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """실제 float 저장 비트와 드래그 record/색상/깊이·순서를 읽는다."""
        self.case,self.trace,self.instructions=case,[],0
        self.mu.mem_write(WINDOW,bytes(0x6000));self.put(VTABLE+0x18,STOP+0x100);self.put(VTABLE+0x20,STOP+0x104)
        self.put(WINDOW+0x3c,case["width"]);self.put(WINDOW+0x40,case["height"])
        self.put(WINDOW+0xd84,DRAG);self.put(WINDOW+0xf8,14);self.put(WINDOW+0xfc,14)
        self.put(IMAGE+8,case["imageWidth"]);self.put(IMAGE+12,case["imageHeight"])
        self.put(WINDOW+0xcc,case["imageWidth"]);self.put(WINDOW+0xd0,case["imageHeight"])
        self.mu.mem_write(WINDOW+0xd88,bytes((9,3,0x76,37,41,43)))
        for index in range(8): # 자식0..6과 드래그의 명시 현재 크기/배율을 공급한다.
            node=CHILDREN+index*0x100 if index<7 else DRAG
            self.put(node,VTABLE);self.put(node+0x38,case["scale"])
            self.put(node+0x3c,bits((257,225,64.5,11,128,77,96,32.5)[index]));self.put(node+0x40,bits((257,225,24.5,11,18.5,77,96,48.25)[index]))
            if index<7:self.put(WINDOW+0xd8+index*4,node)
        self.put(0x540cac,6);self.put(0x568b60,4);self.put(0x563e38,WINDOW)
        self.invoke(0x47d280 if case["op"]=="Attach" else 0x47d5f0,WINDOW,[bits(2.25),bits(-3.5),0,0] if case["op"]=="Attach" else [0x10000,case["x"]&0xffffffff,case["y"]&0xffffffff],control)
        geometry=b"".join(struct.pack("<7I",*(self.read(node+offset) for offset in (0x38,0x3c,0x40,0x44,0x48,0x4c,0x50))) for node in (WINDOW,*(CHILDREN+index*0x100 for index in range(7)),DRAG))
        for low,original in self.original: # 실제 CODE의 어느 바이트도 수정되지 않았음을 확인한다.
            if bytes(self.mu.mem_read(low,len(original)))!=original: raise RuntimeError("창 배치 CODE 변경")
        return dict(case,control=control,geometry=geometry.hex(),record=bytes(self.mu.mem_read(WINDOW+0xd88,6)).hex(),color=self.read(DRAG+0xa4),depth=self.read(DRAG+0x30),trace=";".join(self.trace),instructions=self.instructions)


def cases():
    """격자/원점/반크기·끝 경계와 호출 도중 필드 변경 입력을 만든다."""
    base=dict(op="Attach",width=bits(432),height=bits(412),imageWidth=224,imageHeight=224,scale=bits(1),x=150,y=80,canPlace=1,effect=0)
    result=[]
    for width,height in ((432,412),(500.25,480.75),(200.5,270.25),(-1,-5.75)):
        for scale in (1,.5,1.333333):
            for effect in (0,1,2,3): result.append(dict(base,width=bits(width),height=bits(height),scale=bits(scale),effect=effect)) # 현재 가상 효과 후의 필드 읽기다.
    for width,height in ((0,0),(1,1),(112,224),(224,112)):
        result.append(dict(base,imageWidth=width,imageHeight=height))
    for x,y in ((99,-38),(100,-37),(115,-14),(116,-13),(117,-12),(339,210),(340,211),(355,226),(-2147483648,2147483647),(2147483647,-2147483648)):
        for accepted in (0,1):
            for effect in (0,4): result.append(dict(base,op="Motion",x=x,y=y,canPlace=accepted,effect=effect)) # 격자 밖에서는 충돌 장치를 호출하지 않는다.
    return result


def main():
    """새 증거만 생성하고 verify에서는 원본 CPU를 다시 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument("--verify",action="store_true");parser.add_argument("--smoke",action="store_true");args=parser.parse_args()
    oracle=WindowOracle();selected=cases();selected=selected[:1] if args.smoke else selected
    rows=[oracle.sample(case,control) for case in selected for control in (0x27f,0x37f)]
    stream=io.StringIO(newline="");writer=csv.DictWriter(stream,fieldnames=FIELDS,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows);payload=stream.getvalue().encode("utf-8")
    if not args.smoke:
        if args.verify:
            if FIXTURE.read_bytes()!=payload: raise RuntimeError("창 배치 독립 원문/fixture 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists(): raise RuntimeError("기존 창 배치 증거 덮어쓰기 거부")
            FIXTURE.write_bytes(payload);REPORT.write_text(json.dumps(dict(host="HJOW-Athlon",lastFullDecompile="HJOW-Athlon / 2026-10-23",runs=len(rows),cases=len(selected),peSha=oracle.sha256,sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha=hashlib.sha256(payload).hexdigest(),entered=oracle.entered,limits=["0047d280/0047d5f0 전체 몸체와 실제 Builder/owner/CRT ftol을 실행한다.","기본/자식 가상 Attach·Push·지도 Bounds/Position·CanPlace 반환은 명시 장치 경계다.","창 생성/Refresh/Idle/수명·실제 GUI는 이 관측 범위 밖이다."],code=[dict(address=f"{low:08x}",bytes=len(data),sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(runs=len(rows),cases=len(selected),fixtureSha=hashlib.sha256(payload).hexdigest(),verify=args.verify,smoke=args.smoke)))


if __name__=="__main__": main()
