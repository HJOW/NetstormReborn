#!/usr/bin/env python3
"""실제 MetaImage 생성/측정/부착 뒤 배율/Draw 인자를 격리 CPU에서 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path
from decomp_editor_image import ImageOracle, ROOT, ARENA, IMAGE, STOP, STACK
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FS

# 원문 파생/기본 생성과 실제 CRT ftol만 추가 허용한다.
NODE = ARENA+0x400
ENTRIES = {"Construct":0x47f2a0,"Resize":0x47cbd0,"Attach":0x47cc00,"Draw":0x47cae0}
FIXTURE = ROOT / "cpppj/tests/fixtures/meta-image-x86.tsv"
REPORT = ROOT / "cpppj/meta-image-native-20261027.json"
FIELDS = ("op","width","height","scale","baseWidth","baseHeight","mode","px","py","present","change","control","outScale","outBaseWidth","outBaseHeight","outWidth","outHeight","command","vtable","flags","count","serial","trace","instructions")


def bits(value):
    """명시 float 입력만 IEEE 비트로 바꾸며 기대 배율은 계산하지 않는다."""
    return struct.unpack("<I",struct.pack("<f",value))[0]


class MetaOracle(ImageOracle):
    """원문 파생과 실제 기본 생성을 실행하고 현재 위치/기본 부착/픽셀 소비자만 명시한다."""
    def __init__(self):
        """현재 호스트의 정밀 범위와 SEH 입력을 추가한다."""
        super().__init__()
        facts = {int(row["entry"],16):row for row in csv.DictReader((ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8"),delimiter="\t")}
        for entry in (*ENTRIES.values(),0x4642c0,0x4e49c0): # CODE를 고치지 않고 실제 함수 범위만 추가한다.
            for span in facts[entry]["ranges"].split(";"): # 분리된 원문 몸체도 모두 그대로 허용한다.
                low,high=(int(value,16) for value in span.split("-"));self.allowed.append((low,high+1))
        self.mu.mem_map(0,4096);self.put(0,0xffffffff);self.mu.reg_write(UC_X86_REG_FS,8)
        self.original=[(low,bytes(self.mu.mem_read(low,high-low))) for low,high in self.allowed]
        self.meta_entries=set(ENTRIES.values())|{0x4642c0,0x4e49c0}

    def put(self,address,value):
        """가상 장치/입력 필드의 DWORD를 쓴다."""
        self.mu.mem_write(address,struct.pack("<I",value&0xffffffff))

    def ret(self,purge=0):
        """명시 외부 경계에서 원문 호출자의 스택으로 복귀한다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,0)
        self.mu.reg_write(UC_X86_REG_EIP,self.read(stack));self.mu.reg_write(UC_X86_REG_ESP,stack+4+purge)

    def on_instruction(self,mu,address,size,data):
        """관측한 Draw/부착 순서를 기록하며 x87 산술과 ftol은 원문을 실행한다."""
        if address in self.meta_entries:
            key=f"{address:08x}";self.entered[key]=self.entered.get(key,0)+1
        stack=mu.reg_read(UC_X86_REG_ESP)
        if address==0x464740:
            pointer=self.read(stack+4);mu.mem_write(pointer,struct.pack("<ii",self.case["px"],self.case["py"]))
            self.trace.append("position");self.ret(4);return
        if address==0x466750:
            args=struct.unpack("<4I",mu.mem_read(stack+4,16));self.trace.append("attach:"+":".join(map(str,args)))
            self.put(NODE+0x3c,self.case["baseWidth"]);self.put(NODE+0x40,self.case["baseHeight"])
            if self.case["change"]: # 기본 부착의 통지가 실제 image를 바꾸는 명시 재진입 입력이다.
                self.put(IMAGE+8,37);self.put(IMAGE+12,19)
            self.ret(16);return
        if address==0x4dabe0:
            if self.read(stack+4)!=0x556038: raise RuntimeError("MetaImage 색표 주소 불일치")
            self.trace.append("color:5595192");self.ret();return
        if address in (0x4daa70,0x4daab0,0x4daaf0):
            count=7 if address==0x4daaf0 else 4;args=struct.unpack("<"+"I"*count,mu.mem_read(stack+4,count*4))
            if args[0]!=0x11900000 or args[1]!=0 or (count==7 and (args[4]!=0 or args[5]!=args[6])): raise RuntimeError("MetaImage 실제 Draw 인자 불일치")
            kind="scaled" if count==7 else "mapped" if address==0x4daab0 else "plain"
            self.trace.append("draw:"+kind+":"+":".join(str(value) for value in (args[2],args[3],args[5] if count==7 else 65536)))
            self.ret();return
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """현재 파생 필드와 등록/가상 효과 순서를 원문 반환에서 직접 읽는다."""
        self.case,self.trace,self.instructions=case,[],0
        self.mu.mem_write(NODE,bytes([case["change"] if case["op"]=="Construct" else 0])*0xa8)
        self.mu.mem_write(IMAGE,struct.pack("<5I",0,0x11900000 if case["present"] else 0,case["width"],case["height"],0))
        self.put(NODE+0xa0,IMAGE);self.put(NODE+0xa4,case["mode"])
        for offset,key in ((0x38,"scale"),(0x3c,"baseWidth"),(0x40,"baseHeight")): self.put(NODE+offset,case[key]) # 명시 입력 float만 준비한다.
        self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,0)
        args=[IMAGE] if case["op"]=="Construct" else [bits(1.25),bits(-2.5),0xaabbccdd,0x80000000] if case["op"]=="Attach" else []
        self.invoke(ENTRIES[case["op"]],NODE,args,control)
        if self.read(0)!=0xffffffff: raise RuntimeError("MetaImage SEH 복귀 실패")
        for low,original in self.original: # 허용 범위의 실제 CODE 바이트가 모두 그대로인지 확인한다.
            if bytes(self.mu.mem_read(low,len(original)))!=original: raise RuntimeError("MetaImage CODE 변경")
        return dict(case,control=control,outScale=self.read(NODE+0x38),outBaseWidth=self.read(NODE+0x3c),outBaseHeight=self.read(NODE+0x40),outWidth=self.read(NODE+0x44),outHeight=self.read(NODE+0x48),command=self.read(NODE+0x20),vtable=self.read(NODE),flags=self.read(NODE+4),count=self.read(0x565dc8),serial=self.read(0x540418),trace=";".join(self.trace) or "-",instructions=self.instructions)


def cases():
    """실제 사용 크기/부호/단위배율 경계와 부착 중 image 변경 입력을 나눈다."""
    base=dict(op="Construct",width=6,height=4,scale=bits(1),baseWidth=bits(0),baseHeight=bits(0),mode=0,px=13,py=-7,present=1,change=0)
    result=[dict(base,change=value) for value in (0,0x35,0xa5,0xff)]
    for width,height in ((0,0),(1,1),(6,4),(224,112),(112,224)): # 지도/후보/빈 Image의 치수를 교차한다.
        for scale in (1,.5,0,-1,1.333333): # Resize의 실제 float 저장도 원문에서 관측한다.
            result.append(dict(base,op="Resize",width=width,height=height,scale=bits(scale)))
        for bw,bh in ((0,0),(50,40),(224,112),(37.25,11.75),(500,500)): # 비율의 0/크기 같음/가로·세로 제한을 나눈다.
            for change in (0,1): # 기본 부착 뒤의 현재 image를 읽는지 확인한다.
                result.append(dict(base,op="Attach",width=width,height=height,baseWidth=bits(bw),baseHeight=bits(bh),change=change))
    for width,height in ((1,1),(6,4),(224,112)): # 원본 image 치수를 기준점 산술 입력으로 사용한다.
        for scale in (1,.5,.75,-1,1.25,0,1.000000119): # 정확한 단위 비트 비교와 실제 CRT 절삭이다.
            for mode in (0,1): # mapped는 단위배율 분기에서만 색표를 설치한다.
                for present in (0,1): # shape 부재는 위치 getter조차 호출하지 않는다.
                    result.append(dict(base,op="Draw",width=width,height=height,scale=bits(scale),mode=mode,present=present))
    return result


def main():
    """새 Native 자료를 생성하고 verify는 원문 CPU를 다시 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument("--verify",action="store_true");parser.add_argument("--smoke",action="store_true");arguments=parser.parse_args()
    oracle,selected=MetaOracle(),cases();selected=selected[:1] if arguments.smoke else selected
    rows=[]
    for case in selected: # 두 x87 정밀도의 실제 결과를 별도 행으로 보존한다.
        for control in (0x27f,0x37f): rows.append(oracle.sample(case,control)) # 두 제어 워드 자체가 입력이다.
    stream=io.StringIO(newline="");writer=csv.DictWriter(stream,fieldnames=FIELDS,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows);payload=stream.getvalue().encode("utf-8")
    if not arguments.smoke:
        if arguments.verify:
            if FIXTURE.read_bytes()!=payload: raise RuntimeError("MetaImage 독립 원문/fixture 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists(): raise RuntimeError("기존 MetaImage 증거 덮어쓰기 거부")
            FIXTURE.write_bytes(payload);REPORT.write_text(json.dumps(dict(host="HJOW-Athlon",lastFullDecompile="HJOW-Athlon / 2026-10-23",runs=len(rows),cases=len(selected),peSha=oracle.sha256,sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha=hashlib.sha256(payload).hexdigest(),entered=oracle.entered,
                limits=["실제0047f2a0와 기본Gump004642c0·측정0047cbd0·부착 뒤0047cc00·Draw0047cae0·CRT ftol을 실행한다.","기본 Attach 효과/위치와 색표·픽셀 소비자만 명시 입력/관측 경계다. Native raster 픽셀 관측은 이 fixture의 범위 밖이다.","10.78 전용 관측이며 전체 목록/창/GUI는 후속이다."],code=[dict(address=f"{low:08x}",bytes=len(data),sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(runs=len(rows),cases=len(selected),fixtureSha=hashlib.sha256(payload).hexdigest(),verify=arguments.verify,smoke=arguments.smoke)))


if __name__=="__main__": main()
