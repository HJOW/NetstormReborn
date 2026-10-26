#!/usr/bin/env python3
"""원본21칸 Image 목록과 실제 MetaImage 자식을 같은 CPU에서 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path
from decomp_meta_image import MetaOracle, ROOT, ARENA, STOP, STACK, bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 목록/자식/빌린 Image는 격리 주소이며 원본 파일에는 쓰지 않는다.
LIST, CHILDREN, IMAGES = ARENA+0x2000,ARENA+0x3000,ARENA+0x6000
OPS={"Construct":0x47ccb0,"Attach":0x47cf00,"Hit":0x47cd90,"Handle":0x47ce60,"Draw":0x47d070,"Id":0x47d170,"Image":0x47d1c0,"Remove":0x47d1f0,"Add":0x47f360,"Destruct":0x47cd80}
FIXTURE=ROOT / "cpppj/tests/fixtures/meta-image-list-x86.tsv"
REPORT=ROOT / "cpppj/meta-image-list-native-20261027.json"
FIELDS=("op","count","columns","baseWidth","baseHeight","scale","cellWidth","cellHeight","selection","x","y","event","index","effect","control","result","baseWidthOut","baseHeightOut","flags","vtable","registered","serial","tail","geometry","trace","instructions")


class ListOracle(MetaOracle):
    """목록과 자식 산술은 원문이며 기본 부착/Push·경계·부모 통지만 명시한다."""
    def __init__(self):
        """실제 목록 전체 범위와 기본 소멸자를 추가한다."""
        super().__init__();facts={int(r["entry"],16):r for r in csv.DictReader((ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8"),delimiter="\t")}
        for entry in (*OPS.values(),0x464420): # 실제 기본 생성/MetaImage/ftol은 상위에서 이미 허용했다.
            for part in facts[entry]["ranges"].split(";"): # 분리된 원문 범위도 그대로 추가한다.
                low,high=(int(value,16) for value in part.split("-"));self.allowed.append((low,high+1))
        self.original=[(low,bytes(self.mu.mem_read(low,high-low))) for low,high in self.allowed]
        self.list_entries=set(OPS.values())|{0x464420}

    def on_instruction(self,mu,address,size,data):
        """기본 장치 효과와 호출 순서만 기록하고 목록/자식 몸체는 대체하지 않는다."""
        stack=mu.reg_read(UC_X86_REG_ESP);target=mu.reg_read(UC_X86_REG_ECX)
        if address in self.list_entries:
            key=f"{address:08x}";self.entered[key]=self.entered.get(key,0)+1
        if address==0x466750:
            args=struct.unpack("<4I",mu.mem_read(stack+4,16));label="list" if target==LIST else str((target-CHILDREN)//0x100)
            self.trace.append("attach:"+label+":"+":".join(map(str,(args[0],args[1]))))
            self.put(target+0x4c,args[0]);self.put(target+0x50,args[1]);self.put(target+0x14,args[2])
            if target==LIST and self.case["effect"]==1: self.put(LIST+0x3c,bits(131.75));self.put(LIST+0x40,bits(99.5)) # 부착 뒤 현재 크기를 읽는 입력이다.
            self.ret(16);return
        if address==0x464560:
            label="list" if target==LIST else str((target-CHILDREN)//0x100);self.trace.append("push:"+label);self.put(target+4,self.read(target+4)|8)
            if target==CHILDREN and self.case["effect"]==2: # 첫 자식 Push가 현재 count/셀 크기를 바꾸는 입력이다.
                self.put(LIST+0xa0,max(1,self.read(LIST+0xa0)-1));self.put(LIST+0x158,bits(17.25));self.put(LIST+0x154,bits(13.5))
            self.ret();return
        if address==0x4648e0:
            mu.mem_write(self.read(stack+4),struct.pack("<4i",10,20,210,140));self.trace.append("bounds");self.ret(4);return
        if address==0x464a50:
            self.trace.append("redraw:"+str(self.read(stack+4)));self.ret(4);return
        if address==0x464e90:
            self.trace.append("notify:"+":".join(map(str,struct.unpack("<3I",mu.mem_read(stack+4,12)))));self.ret(12);return
        if address==0x4a2180:
            self.trace.append("fill:"+":".join(map(str,struct.unpack("<5I",mu.mem_read(stack+4,20)))));self.ret();return
        if address==0x466480:
            self.trace.append("kill:"+str((target-CHILDREN)//0x100));self.ret();return
        if address==0x4e4391:
            if self.read(stack+4)!=0xa8: raise RuntimeError("목록 자식 실제 할당 크기 불일치")
            self.trace.append("allocate");mu.reg_write(UC_X86_REG_EAX,CHILDREN+self.read(LIST+0xa0)*0x100);mu.reg_write(UC_X86_REG_EIP,self.read(stack));mu.reg_write(UC_X86_REG_ESP,stack+4);return
        if address==0x464420: self.trace.append("destruct") # 실제 기본 소멸은 원문을 계속 실행한다.
        if address==0x4e0620: raise RuntimeError("목록 원문 진단 입력 범위 밖")
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """원본 초기화/호출 후21칸 tail과22개 자식의 실제 기하를 기록한다."""
        self.case,self.trace,self.instructions=case,[],0
        self.mu.mem_write(LIST,bytes(0xa0)+b"\x5a"*0xbc);self.mu.mem_write(CHILDREN,bytes(0x1600));self.mu.mem_write(IMAGES,bytes(0x400))
        self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,0);self.put(0x5acd20,43)
        self.invoke(OPS["Construct"],LIST,[case["columns"]&0xffffffff],control)
        if case["op"]!="Construct":
            self.put(LIST+0xa0,case["count"]);self.put(LIST+0x14c,case["selection"])
            self.put(LIST+0x3c,case["baseWidth"]);self.put(LIST+0x40,case["baseHeight"]);self.put(LIST+0x38,case["scale"])
            self.put(LIST+0x158,case["cellWidth"]);self.put(LIST+0x154,case["cellHeight"]);self.put(LIST+0x4c,bits(2.25));self.put(LIST+0x50,bits(3.5))
            for index in range(22): # 마지막1개는 새 추가 입력이며 기존21칸과 별개의 Image다.
                self.mu.mem_write(IMAGES+index*32,struct.pack("<5I",0,0,(224,6,112)[index%3],(112,4,224)[index%3],0))
                if index<case["count"]:
                    self.invoke(0x47f2a0,CHILDREN+index*0x100,[IMAGES+index*32],control);self.put(LIST+0xa4+index*4,CHILDREN+index*0x100);self.put(LIST+0xf8+index*4,4096+index*7)
            if case["op"]=="Destruct": self.put(LIST+4,self.read(LIST+4)|0x40000)
            self.trace=[]
            args=([bits(2.25),bits(3.5),0,0] if case["op"]=="Attach" else [case["x"]&0xffffffff,case["y"]&0xffffffff] if case["op"]=="Hit" else [case["event"],case["x"]&0xffffffff,case["y"]&0xffffffff] if case["op"]=="Handle" else [case["index"]&0xffffffff] if case["op"] in ("Id","Image","Remove") else [IMAGES+case["count"]*32,0x81234567] if case["op"]=="Add" else [])
            self.invoke(OPS[case["op"]],LIST,args,control)
        returned=self.mu.reg_read(UC_X86_REG_EAX)
        if case["op"]=="Construct": returned=1
        elif case["op"] in ("Attach","Draw","Destruct"): returned=0
        elif case["op"]=="Image": returned=(returned-IMAGES)//32+1 if returned else 0
        tail=bytearray(self.mu.mem_read(LIST+0xa0,0xbc))
        for index in range(21): # 호스트 포인터 대신 같은 자식의 논리 순번만 정규화한다.
            pointer=self.read(LIST+0xa4+index*4);struct.pack_into("<I",tail,4+index*4,(pointer-CHILDREN)//0x100+1 if pointer else 0)
        geometry=b"".join(struct.pack("<7I",*(self.read(CHILDREN+index*0x100+offset) for offset in (0x38,0x3c,0x40,0x44,0x48,0x4c,0x50))) for index in range(22))
        for low,original in self.original: # 관측 중 실제 원문 CODE가 변경되지 않았는지 검사한다.
            if bytes(self.mu.mem_read(low,len(original)))!=original: raise RuntimeError("목록 CODE 변경")
        return dict(case,control=control,result=returned,baseWidthOut=self.read(LIST+0x3c),baseHeightOut=self.read(LIST+0x40),flags=self.read(LIST+4),vtable=self.read(LIST),registered=self.read(0x565dc8),serial=self.read(0x540418),tail=tail.hex(),geometry=geometry.hex(),trace=";".join(self.trace) or "-",instructions=self.instructions)


def cases():
    """기대 계산 없이 목록/배치/선택의 경계와 실제 자식 수명 입력을 만든다."""
    base=dict(op="Construct",count=0,columns=4,baseWidth=bits(200),baseHeight=bits(120),scale=bits(1),cellWidth=bits(50),cellHeight=bits(20),selection=-1,x=10,y=20,event=0x10000,index=0,effect=0)
    result=[dict(base,columns=value) for value in (-1,0,1,4,22)]
    for count in (0,1,5,21): # 빈/최초/불완전 행/전체21칸을 구분한다.
        for columns in (0,1,4,22): # 가로 배치와 grid의 원문 두 분기다.
            for effect in (0,1,2): # 기본 부착과 자식 Push 중의 현재 필드 변경을 관측한다.
                result.append(dict(base,op="Attach",count=count,columns=columns,effect=effect,baseWidth=bits(199.75),baseHeight=bits(119.5)))
            for x,y in ((9,20),(10,20),(59,39),(60,40),(109,120),(209,139),(210,140)): # half-open 경계와 빈 마지막 셀을 포함한다.
                result.append(dict(base,op="Hit",count=count,columns=columns,x=x,y=y,scale=bits(.5)))
        for index in (0,4,20): # 유효 저장소 범위 안의 현재 count 바깥도 구분한다.
            for operation in ("Id","Image","Remove"): result.append(dict(base,op=operation,count=count,index=index)) # ID·Image·Kill/압축은 별개의 실제 몸체다.
        result.append(dict(base,op="Add",count=count));result.append(dict(base,op="Destruct",count=count))
    for columns in (0,4): # 선택 토글·release 관문과 화면 밖 알림을 관측한다.
        for selection in (-1,0,5,21): # count와 같은 선택도 원문이 보존한다.
            for event in (0x10000,0x80010000,0x40010000,0x10001):
                result.append(dict(base,op="Handle",count=21,columns=columns,selection=selection,event=event,x=60,y=40))
            result.append(dict(base,op="Handle",count=21,columns=columns,selection=selection,x=9,y=20))
            result.append(dict(base,op="Draw",count=21,columns=columns,selection=selection,cellWidth=bits(33.25),cellHeight=bits(19.75),baseWidth=bits(199.75)))
    return result


def main():
    """새 fixture만 생성하며 verify에서는 원문 CPU를 다시 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument("--verify",action="store_true");parser.add_argument("--smoke",action="store_true");args=parser.parse_args()
    oracle,selected=ListOracle(),cases();selected=selected[:1] if args.smoke else selected;rows=[]
    for case in selected: # 두 x87 정밀도의 실제 결과를 각각 기록한다.
        for control in (0x27f,0x37f): rows.append(oracle.sample(case,control)) # 제어 워드는 독립 입력이다.
    output=io.StringIO(newline="");writer=csv.DictWriter(output,fieldnames=FIELDS,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows);payload=output.getvalue().encode("utf-8")
    if not args.smoke:
        if args.verify:
            if FIXTURE.read_bytes()!=payload: raise RuntimeError("목록 독립 원문/fixture 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists(): raise RuntimeError("기존 목록 증거 덮어쓰기 거부")
            FIXTURE.write_bytes(payload);REPORT.write_text(json.dumps(dict(host="HJOW-Athlon",lastFullDecompile="HJOW-Athlon / 2026-10-23",runs=len(rows),cases=len(selected),peSha=oracle.sha256,sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha=hashlib.sha256(payload).hexdigest(),entered=oracle.entered,limits=["실제 목록 전체 몸체와 기본/MetaImage 생성·자식 Attach·CRT ftol·기본 소멸자를 실행한다.","기본 Attach/Push·Bounds·Redraw·부모 Bubble·Kill·픽셀 소비자와 자식 heap 주소는 명시 경계다.","저장소 밖 진단 입력과 실제 창/GUI는 이 관측 범위 밖이다."],code=[dict(address=f"{low:08x}",bytes=len(data),sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(runs=len(rows),cases=len(selected),fixtureSha=hashlib.sha256(payload).hexdigest(),verify=args.verify,smoke=args.smoke)))


if __name__=="__main__": main()
