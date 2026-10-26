#!/usr/bin/env python3
"""원본 내장 TELL/HtmlBody 기본 HTML 부모를 실제 CPU로 관측한다."""
import argparse
import csv
import hashlib
import io
import json
import struct
from decomp_meta_image import MetaOracle, ROOT, ARENA, STOP, STACK, bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

# 부모/표시 자식/소스/미션/임시 Config는 CODE 밖의 독립 저장소다.
PARENT, CHILD, SOURCE, MISSION, TEMP, KEY, PATH, VTABLE = (ARENA+value for value in (0x20000,0x30000,0x40000,0x50000,0x60000,0x70000,0x71000,0x72000))
ENTRIES=(0x4c8f60,0x46c9c0,0x46cc60,0x46cac0)
FIXTURE=ROOT / "cpppj/tests/fixtures/embedded-tell-x86.tsv"
REPORT=ROOT / "cpppj/embedded-tell-native-20261027.json"
FIELDS=("op","mask","key","source","control","returned","flags","vtable","serial","text","bodyHash","linkHash","trace","instructions")


def fingerprint(data):
    """8192BYTE 전체의 실제 기록을 기존 FNV32 순서로 투영한다."""
    result=2166136261
    for byte in data: result=((result^byte)*16777619)&0xffffffff # NUL 뒤의 원문 기존 영역도 포함한다.
    return result


class EmbeddedOracle(MetaOracle):
    """조회/할당/다중 줄/기본 HTML 파서 경계만 공급하며 부모 몸체는 실행한다."""
    def __init__(self):
        """현재 정밀 디컴파일의 네 원문 몸체를 추가한다."""
        super().__init__();facts={int(row["entry"],16):row for row in csv.DictReader((ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8"),delimiter="\t")}
        for entry in ENTRIES: # 실제 함수의 분리된 범위도 빠짐없이 허용한다.
            for part in facts[entry]["ranges"].split(";"):
                low,high=(int(value,16) for value in part.split("-"));self.allowed.append((low,high+1))
        self.original=[(low,bytes(self.mu.mem_read(low,high-low))) for low,high in self.allowed]

    def text(self,address):
        """선택된 명시 C문자열의 실제 NUL 앞 바이트만 읽는다."""
        result=bytearray()
        for offset in range(8192): # 원본 물리 저장소를 벗어나는 관측은 거부한다.
            value=self.mu.mem_read(address+offset,1)[0]
            if not value:return bytes(result)
            result.append(value)
        raise RuntimeError("내장 TELL 문자열 NUL 누락")

    def args(self,count):
        """실제 thiscall/cdecl 스택 인자를 읽는다."""
        return struct.unpack("<"+"I"*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def give(self,value=0,purge=0):
        """명시 장치 경계만 실제 인자 폭으로 복귀한다."""
        self.ret(purge);self.mu.reg_write(UC_X86_REG_EAX,value)

    def on_instruction(self,mu,address,size,data):
        """원문 쿼리/변환/표시 자식 설치의 호출 순서를 기록한다."""
        target=mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:self.entered[f"{address:08x}"]=self.entered.get(f"{address:08x}",0)+1
        if address==0x440700:
            owner="mission" if target==MISSION else "fallback";self.trace.append(owner+":"+self.text(self.args(1)[0]).hex())
            present=bool(self.case["mask"]&(2 if target==MISSION else 4));self.give(SOURCE if present else 0,4);return
        if address==0x4e4391:
            count=self.args(1)[0]
            if count==24:self.give(TEMP)
            elif count==0x40a4:self.trace.append("new");self.give(PARENT)
            elif count==0xd8:self.trace.append("allocateText");self.give(CHILD)
            else:raise RuntimeError("내장 TELL 실제 할당 폭")
            return
        if address==0x43fc20:self.give(TEMP,4);return
        if address==0x4410f0:
            if self.text(self.args(4)[0])!=b"missionSpec" or self.text(self.args(4)[1])!=b"tell":raise RuntimeError("내장 TELL 실제 파일 키")
            self.give(PATH);return
        if address==0x440380:self.give(0,4);return
        if address in (0x43fcd0,0x4e4354):self.give();return
        if address==0x4c2630:self.trace.append("missing:"+self.text(self.args(3)[2]).hex());self.give();return
        if address==0x46c810:
            source,out,capacity,links=self.read(target+4),self.read(target+8),self.read(target+12),self.read(target+16)
            if out!=PARENT+0xa4 or links!=PARENT+0x20a4 or capacity!=8192:raise RuntimeError("내장 HTML 실제 두 출력 영역")
            text=self.text(source);self.trace.append("parse:"+text.hex());mu.mem_write(out,b"["+text+b"]\0");mu.mem_write(links,b"link\0")
            if self.case["mask"]&32:self.put(PARENT+0x2c,9001)
            self.give();return
        if address==0x466480:self.trace.append("kill");self.give();return
        if address==0x4bc060:
            self.trace.append("constructText:"+":".join(map(str,self.args(4))));self.put(CHILD,VTABLE);self.give(CHILD,16);return
        if address in (0x46cc30,STOP+0x104):
            x,y,parent,mode=self.args(4);self.trace.append("attach:"+("parent" if target==PARENT else "text")+":"+str(x)+":"+str(y)+":"+str(mode));self.put(target+0x4c,x);self.put(target+0x50,y);self.give(0,16);return
        if address==0x4ba8f0:self.trace.append("styled:"+self.text(self.args(1)[0]).hex());self.give(0,4);return
        if address in (0x4ba360,0x4ba1b0,0x4ba230):
            self.trace.append({0x4ba360:"scroll",0x4ba1b0:"slider",0x4ba230:"scrollable"}[address]+":"+str(self.args(1)[0]));self.give(0,4);return
        if address==0x464560:self.trace.append("push");self.put(target+4,self.read(target+4)|8);self.give();return
        if address==0x4e0620:raise RuntimeError("내장 TELL 원문 진단")
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """두 부모 영역과 실제 반환 serial/자식/flags를 읽는다."""
        self.case,self.trace,self.instructions=case,[],0;self.mu.mem_write(PARENT,bytes(0x40a4));self.mu.mem_write(PARENT+0xa4,b"Z"*0x4000)
        self.mu.mem_write(SOURCE,bytes.fromhex(case["source"])+b"\0");self.mu.mem_write(KEY,bytes.fromhex(case["key"])+b"\0");self.mu.mem_write(PATH,b"d/tell.english\0")
        self.mu.mem_write(0x563e38,bytes(4000));self.put(0x565dc8,0);self.put(0x540418,0);self.put(0x531860,640);self.put(0x531864,480);self.put(0x5b5dc8,7)
        self.put(0x5634a0,MISSION if case["mask"]&1 else 0);self.put(MISSION+0x5c,MISSION);self.put(VTABLE+0x20,STOP+0x104)
        prior=bytes(self.mu.mem_read(0x5cab40,256))
        if case["op"]=="SetHtml":
            self.invoke(0x46c9c0,PARENT,[],control);self.put(PARENT+0x4c,bits(2.25));self.put(PARENT+0x50,bits(-3.5))
            if case["mask"]&16:self.put(PARENT+0xa0,CHILD+0x1000)
            if case["mask"]&8:self.put(PARENT+4,self.read(PARENT+4)&~8)
            self.trace=[];self.invoke(0x46cc60,PARENT,[SOURCE],control);returned=0
        else:
            self.put(STACK+4,KEY);self.invoke(0x4c8f60,0,[],control);returned=self.mu.reg_read(UC_X86_REG_EAX) # cdecl 인자는 호출자가 나중에 반납한다.
        if prior!=bytes(self.mu.mem_read(0x5cab40,256)):raise RuntimeError("내장 TELL이 현재 팝업 이름을 변경함")
        for low,code in self.original: # 관측 후 실제 CODE 불변도 확인한다.
            if bytes(self.mu.mem_read(low,len(code)))!=code:raise RuntimeError("내장 TELL CODE 변경")
        return dict(case,control=control,returned=returned,flags=self.read(PARENT+4),vtable=self.read(PARENT),serial=self.read(PARENT+0x2c),text=int(bool(self.read(PARENT+0xa0))),bodyHash=fingerprint(self.mu.mem_read(PARENT+0xa4,8192)),linkHash=fingerprint(self.mu.mem_read(PARENT+0x20a4,8192)),trace=";".join(self.trace) or "-",instructions=self.instructions)


def cases():
    """미션/파일/빈 섹션과 기존 자식/부착 상태를 독립 입력으로 만든다."""
    result=[]
    for key in (b"PlacingIslands",b"file.part",b"BLANK"):
        for mask in (0,1,3,4,5,7,39):result.append(dict(op="Tell",mask=mask,key=key.hex(),source=(b"" if mask==7 else b"selected").hex())) # 점/BLANK를 일반 이름 그대로 조회해야 한다.
    for source in (b"",b"plain",b"a\0ignored",b"<b>A</b>"):
        for mask in (0,8,16,24,56):result.append(dict(op="SetHtml",mask=mask,key="",source=source.hex())) # 원문 빈/기존 자식/비void와 파서 중 serial 변경이다.
    return result


def main():
    """신규 자료만 생성하고 verify는 원문을 다시 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument("--verify",action="store_true");parser.add_argument("--smoke",action="store_true");args=parser.parse_args();oracle=EmbeddedOracle();selected=cases();selected=selected[:1] if args.smoke else selected
    rows=[oracle.sample(case,control) for case in selected for control in (0x27f,0x37f)];stream=io.StringIO(newline="");writer=csv.DictWriter(stream,fieldnames=FIELDS,delimiter="\t",lineterminator="\n");writer.writeheader();writer.writerows(rows);payload=stream.getvalue().encode("utf-8")
    if not args.smoke:
        if args.verify:
            if FIXTURE.read_bytes()!=payload:raise RuntimeError("내장 TELL 원문/fixture 불일치")
        else:
            if FIXTURE.exists() or REPORT.exists():raise RuntimeError("기존 내장 TELL 증거 덮어쓰기 거부")
            FIXTURE.write_bytes(payload);REPORT.write_text(json.dumps(dict(host="HJOW-Athlon",lastFullDecompile="HJOW-Athlon / 2026-10-23",cases=len(selected),rows=len(rows),peSha=oracle.sha256,sourceSha=hashlib.sha256(__import__("pathlib").Path(__file__).read_bytes()).hexdigest(),fixtureSha=hashlib.sha256(payload).hexdigest(),entered=oracle.entered,limits=["실제004c8f60/0046c9c0/0046cc60/0046cac0와 기본 Gump 생성 몸체를 실행한다.","Config/heap/기본 HTML 파서/다중 줄과 가상 Attach·Push·Kill은 명시 장치 경계다.","Native 기본 파서/픽셀·전체 섬 추가 창·GUI는 이 신규 관측 범위 밖이다."],code=[dict(address=f"{low:08x}",bytes=len(data),sha=hashlib.sha256(data).hexdigest()) for low,data in oracle.original]),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(rows=len(rows),cases=len(selected),sha=hashlib.sha256(payload).hexdigest(),verify=args.verify,smoke=args.smoke)))


if __name__=="__main__":main()
