#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""네 다리 전략의 연결/재시도/무작위 지면/빈 사각형을 실제 x86 하위까지 실행한다."""
import collections
import csv
import itertools
import json
import socket
import struct

from decomp_ai_bridge_candidates import CandidateOracle, ROOT, BODY, STACK, STOP, SIZE, POOL, SURFACES, SPOTS, TYPES, FRAMES, digest
from decomp_ai_bridge_start import bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ENTRIES = {"Connected": 0x410D40, "Delay": 0x411310, "Random": 0x411470, "Rectangle": 0x4119B0}  # 복원 대상의 실제 진입점이다.
HELPERS = (0x462BC0,0x462B80,0x4AC1E0,0x40EAF0,0x40E9D0,0x40FAA0,0x4AD390,0x4AD680,0x49A840,0x4E49C0,0x4558F0)  # graph/지도/코드/CRT/난수도 실제 명령이다.
OUTPUT = BODY + 0x1000  # 원본 caller가 소유한8BYTE 결과다.
REPORT = ROOT / "cpppj/ai-bridge-planning-20261022.json"  # 보호 원본·입력·실행 범위를 보존한다.
PREFIX = ROOT / "cpppj/tests/fixtures"  # 기존 기대와 구별한 새 관측 위치다.


class PlanningOracle(CandidateOracle):
    """원본 게임/OS/계산 스텁 없이 검토된 실제 몸체만 실행한다."""

    def __init__(self):
        """현재 호스트 정밀 결과로 모든 하위 범위를 허용한다."""
        super().__init__()
        with (ROOT / "extracted/refined/originals/functions.tsv").open(encoding="utf-8") as stream:
            facts = {int(row["entry"],16): row for row in csv.DictReader(stream,delimiter="\t")}
        self.ranges = []
        for entry in (*ENTRIES.values(),*HELPERS):  # 함수 사이 padding/다른 경로는 허용하지 않는다.
            for part in facts[entry]["ranges"].split(";"):
                lo,hi = (int(word,16) for word in part.split("-"))
                self.ranges.append((lo,hi+1))
        self.counts = collections.Counter()
        self.mu.mem_write(0x540414,struct.pack("<I",1))
        self.mu.mem_write(0x5E4794,bytes(4))
        self.mu.mem_write(0x542514,struct.pack("<I",SURFACES))

    def instruction(self, mu, address, size, data):
        """실제 하위 진입 횟수만 기록하며 대체 반환을 주입하지 않는다."""
        if address in (*ENTRIES.values(),*HELPERS):
            self.counts[hex(address)] += 1
        if not any(lo <= address < hi for lo,hi in self.ranges):
            raise RuntimeError(f"검토 범위 밖 명령 {address:08x}")

    def call(self, name, args, precision, purge):
        """각 caller의 stack/FPU를 준비하고 정상 ABI 보존과 반환만 읽는다."""
        self.mu.mem_write(STACK,struct.pack("<"+"I"*(len(args)+1),STOP,*[value & 0xFFFFFFFF for value in args]))
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,BODY)
        self.mu.reg_write(UC_X86_REG_FPCW,precision)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_FPTAG,0xFFFF)
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):
            self.mu.reg_write(register,value)
        self.mu.emu_start(ENTRIES[name],STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):
            assert self.mu.reg_read(register)==value
        return self.mu.reg_read(UC_X86_REG_EAX)

    def body(self, graph=7, owner=2):
        """명시 caller 입력만 적용한 전체0xb08 본체를 반환한다."""
        raw=bytearray([0xA5])*SIZE
        struct.pack_into("<I",raw,0x298,graph)
        struct.pack_into("<I",raw,0x260,owner)
        self.mu.mem_write(BODY,bytes(raw))
        return bytes(raw)

    def world(self, mode, kind, side):
        """명시 지도 네 종류와 실제 getter가 읽을 TYPE/슬롯 입력만 구성한다."""
        surface,spots=bytearray(0x20000),bytearray(65536)
        for y in range(256):  # 현재 flat 지도 입력이며 성공 지점을 계산하지 않는다.
            for x in range(256):
                at=y*256+x
                if mode and (mode!=3 or (x+3*y)%5):
                    struct.pack_into("<H",surface,at*2,50)
                spots[at]=0x80 if mode==2 else 0x40 if mode==3 and (x+3*y)%5==0 else 0
        self.mu.mem_write(SURFACES,bytes(surface));self.mu.mem_write(SPOTS,bytes(spots))
        source=bytearray(50);source[10],source[30],source[34]=kind,7,2
        self.mu.mem_write(POOL+50*50,bytes(source))
        record=bytearray(500);struct.pack_into("<I",record,0xE8,0x800);struct.pack_into("<I",record,0x124,FRAMES)
        self.mu.mem_write(TYPES+kind*500,bytes(record));self.mu.mem_write(FRAMES,bytes((side,65,0,0)))

    def random(self, values, precision):
        """전체본문불변/실제20회재시도/공용난수상태와출력XY를관측한다."""
        mode,kind,side,left,top,right,bottom,graph,owner,state=values
        self.world(mode,kind,side);body=self.body(graph,owner)
        self.mu.mem_write(0x532710,struct.pack("<I",state));self.mu.mem_write(OUTPUT,bytes([0x5A])*8)
        assert self.call("Random",[OUTPUT,left,top,right,bottom],precision,20)==OUTPUT
        assert bytes(self.mu.mem_read(BODY,SIZE))==body
        return bytes(self.mu.mem_read(OUTPUT,8)).hex(),struct.unpack("<I",self.mu.mem_read(0x532710,4))[0]

    def connected(self, values, precision):
        """실제 위치graph→가장자리12칸의 현재J/K 판정을 함께 실행한다."""
        edge,kind,side,graph,owner,bridge,xbits,ybits=values
        self.world(0,kind,side);body=self.body(graph,owner)
        self.mu.mem_write(0x5411A0,struct.pack("<I",bridge))
        base=bytearray(50);base[10],base[30],base[34]=97,7,2
        record=bytearray(500);struct.pack_into("<I",record,0xE8,0x800)
        self.mu.mem_write(TYPES+97*500,bytes(record));self.mu.mem_write(POOL+51*50,bytes(base))
        geyser=bytearray(50);geyser[10]=98;struct.pack_into("<II",geyser,14,xbits,ybits)
        self.mu.mem_write(TYPES+98*500,bytes(500));self.mu.mem_write(POOL+60*50,bytes(geyser))
        x,y=(int(struct.unpack("<f",struct.pack("<I",value))[0]) for value in (xbits,ybits))
        gx,gy=(int(struct.unpack("<f",struct.pack("<I",value))[0]+0.9998999834060669) for value in (xbits,ybits))  #실제40eaf0의별도거의올림graph조회입력이다.
        if 0<=gx<256 and 0<=gy<256:
            self.mu.mem_write(SURFACES+(gy*256+gx)*2,struct.pack("<H",51))
        edges=[(x+dx,y+dy) for dx,dy in ((-2,-3),(-2,1),(-1,-3),(-1,1),(0,-3),(0,1),(-3,-2),(1,-2),(-3,-1),(1,-1),(-3,0),(1,0))]  # 실제caller입력지형의배치좌표다.
        if edge>=0:
            cx,cy=edges[edge]
            if 0<=cx<256 and 0<=cy<256:
                self.mu.mem_write(SURFACES+(cy*256+cx)*2,struct.pack("<H",50))
        result=self.call("Connected",[60],precision,4)
        assert bytes(self.mu.mem_read(BODY,SIZE))==body
        return result

    def rectangle(self, values, precision):
        """반전/단일축/행경계도실제inclusive주소의첫점유반환으로확인한다."""
        left,top,right,bottom,mode=values
        self.world(mode,82,ord('J'));body=self.body()
        result=self.call("Rectangle",[left,top,right,bottom],precision,16)
        assert bytes(self.mu.mem_read(BODY,SIZE))==body
        return result

    def delay(self, values, precision):
        """count와현재QWORD입력으로지정8BYTE이외의전체본문보존을읽는다."""
        count,now=values;body=bytearray(self.body());struct.pack_into("<I",body,0x350,count)
        self.mu.mem_write(BODY,bytes(body));self.mu.mem_write(0x55B4D0,struct.pack("<Q",now))
        self.call("Delay",[],precision,0)
        return bytes(self.mu.mem_read(BODY,SIZE)).hex()


def main():
    """원본 반환을 두x87정밀도로 대조한 뒤 기존 관측과 별도로 동결한다."""
    names=("random","connected","rectangle","delay")
    paths=[PREFIX/f"ai-bridge-planning-{name}-x86.tsv" for name in names]
    assert not REPORT.exists()
    oracle=PlanningOracle();cases={name:[] for name in names}
    for mode,kind,side,state in itertools.product(range(4),(82,97),range(65,81),(0,0xFFFFFFFF)):  #256개현재지도/연결/난수입력이다.
        cases["random"].append((mode,kind,side,*map(bits,(49.75,61.5,55.25,67.5)),7,2,state))
    for graph,owner in ((0x107,2),(7,0x102)):
        cases["random"].append((1,97,ord('P'),*map(bits,(49.75,61.5,55.25,67.5)),graph,owner,1))
    for boundary in ((55.25,61.5,49.75,67.5),(-2.75,61.5,2.25,67.5),(254.75,61.5,258.25,67.5)):
        cases["random"].append((1,97,ord('P'),*map(bits,boundary),7,2,1))
    for edge,kind,side in itertools.product(range(-1,12),(82,97),map(ord,'JKA')):  #각12칸과없음·타입/프레임차이를교차한다.
        cases["connected"].append((edge,kind,side,7,2,82,bits(50.75),bits(63.25)))
    for graph,owner,bridge in ((0x107,2,82),(7,0x102,82),(7,2,0x152),(254,2,82)):
        cases["connected"].append((0,82,ord('J'),graph,owner,bridge,bits(50.75),bits(63.25)))
    for x,y in ((0.75,0.25),(255.75,255.25),(-1.75,63.25)):
        cases["connected"].append((0,82,ord('J'),7,2,82,bits(x),bits(y)))
    for bounds,mode in itertools.product(((49,61,55,67),(55,61,49,67),(49,67,55,61),(50,63,50,63),(-1,63,1,63),(255,63,256,63)),range(4)):
        cases["rectangle"].append((*bounds,mode))
    for count,now in itertools.product((0,1,0xFFFFFFFF,0x80000000),(0.0,-0.0,1.25,-120.0,9007199254740992.0,float('inf'),float('-inf'))):
        cases["delay"].append((count,struct.unpack("<Q",struct.pack("<d",now))[0]))
    counts={}
    for name,path in zip(names,paths):  #정상반환과원문기대를분리해재작성하지않는다.
        output=[]
        for values in cases[name]:
            samples=[getattr(oracle,name)(values,precision) for precision in (0x27F,0x37F)]
            assert samples[0]==samples[1]
            observed=samples[0] if isinstance(samples[0],tuple) else (samples[0],)
            output.append((*values,*observed))
        text=f"# 실제x86 {name}:caller입력과원문출력\n"+"".join("\t".join(map(str,row))+"\n" for row in output)
        if path.exists():
            assert path.read_text(encoding="utf-8")==text  #부분실행에서동결한행도전부일치해야하며다시쓰지않는다.
        else:
            with path.open("x",encoding="utf-8",newline="\n") as stream:
                stream.write(text)
        counts[name]=len(output)
    report={"host":socket.gethostname(),"decompile_host":"VM-W11-Codex","rows":counts,"normal_returns":sum(counts.values())*2,"x87":[0x27F,0x37F],"actual_entries":oracle.counts,"stubs":[],"game_started":False,"sources":{str(path.relative_to(ROOT)):digest(path) for path in (ROOT/"originals/Netstorm.exe",ROOT/"originalCD/NETSTORM.EXE",ROOT/"tools/decomp_ai_bridge_planning.py",*paths)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"rows":counts,"returns":report["normal_returns"]}))


if __name__=="__main__":
    main()
