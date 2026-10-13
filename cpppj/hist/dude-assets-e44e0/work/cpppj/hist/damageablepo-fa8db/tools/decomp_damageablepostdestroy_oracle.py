#!/usr/bin/env python3
"""실제 DamageablePostDestroy·첫 드롭 helper와 현재 finder/HP/다리 쓰기를 두 PE에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS as PE_SPECS,TYPES,STOP,STACK_BASE,STACK,digest,pefile
from decomp_priestgeometry_oracle import GeometryOracle
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 분석용 raw 풀/4단계해시/생성관찰메모리이며 보호 원본파일을 수정하지 않는다.
POOL,HASH,HEAP,VTABLE=0x11000000,0x11200000,0x11300000,0x11307000
UPDATE,DESTROY=0x11307ff0,0x11307ff4
SOURCE=50
IDS=(50,60,61,62,63,64,65)
SCALES=(1,2,4,16)
FIXTURE=ROOT/"cpppj/tests/fixtures/damageablepostdestroy-x86.tsv"
REPORT=ROOT/"cpppj/damageablepostdestroy.json"
# 현재 전체 부모/선행helper와 명시 생성·공통post·표시 경계의 실제 주소다.
SPECS={
 "originals":dict(post=0x44b9e0,drops=0x44b830,stride=50,extra=40,owner=34,frame=36,frame_width=4,
   drop_random=0x450b30,drop_residence=0x450c20,island=0x46e6c0,base=0x4b0840,
   alloc=0x4e4391,regular=0x496e80,fall=0x421fe0,notify=0x4214a0,
   battle=0x594fbc,boss=0x540bc4,dudes=0x55a4f8,crack=0x52f960,event=0x5326c0,
   finder_table=0x502318,grid=0x542514,capacity=0x5c847c),
 "originalCD":dict(post=0x461b40,drops=0x461970,stride=36,extra=35,owner=32,frame=34,frame_width=1,
   drop_random=0x4e8d20,drop_residence=0x4e8e00,island=0x48f960,base=0x4adbe0,
   alloc=0x4f1650,regular=0x48efd0,fall=0x449170,notify=0x448c10,
   battle=0x540a20,boss=0x540a2c,dudes=0x5b72c0,crack=0x51f05c,event=0x520fdc,
   finder_table=0x5003d0,grid=0x5670cc,capacity=0x5395f4)}
# 입력은 기대결과와분리한 원본 DWORD/현재metadata/장면/현재전역이다.
KEYS=("flags","battle","boss","extra","type","flags1","genus","group","hp","footx","footy",
      "scene","dudes","event","crack","mutation","x","y")


def bits(value):
    """명시 float입력을 원본 DWORD 비트로 만든다."""
    return struct.unpack("<I",struct.pack("<f",value))[0]


def inputs(edition):
    """모든원본분기와packed count·방향frame·현재효과뒤재조회를 독립 입력한다."""
    cases=[]
    # HP/드롭/군도정리 억제와group/abstract·buried조건을 교차한다.
    for flags,genus,group,battle,extra in itertools.product((0,0x100000,0x200000,0x400000,0x310000,0x4500000),
            (0,0x200,0x10000,0x20000,0x40000,0x400000,0x4000,0x10000000),(0,1,2,10),(0,1),(0,1,8,9)):
        cases.append(("Post",(flags,battle,1,extra,89,0x10,genus,group,100,3,3,0,0,6699,5,0,bits(50.5),bits(50.5))))
    # 실제하위효과뒤raw재조회·carry가있는packed count·반경 almost-ceil·프레임모드를 독립 관찰한다.
    for scene,mutation,hp in itertools.product(range(5),range(7),(1,131073)):
        cases.append(("Post",(0x110000,1,scene&1,0,89,0x410,0x4000,0,hp,1+scene%3,3,scene,21,6699,3+scene,mutation,
                    bits((50.0,50.00001,50.99998,50.99999,50.5)[scene]),bits(50.5))))
    # 기존Contained Release와별개인드롭helper의타입·수량·현재20명경계·셋째분기를관찰한다.
    types=(89,133,156,144,138,135,170)+( (187,) if edition=="originals" else () )
    for typ,genus,dudes,random,extra in itertools.product(types,(0,0x200,0x4200,0x20000,0x174200),(0,20,21,0xffffffff),(0,1),(0,1)):
        cases.append(("Drops",(random,0,1,extra,typ,0x10,genus,0,100,3,3,0,dudes,6699,5,0,bits(50.5),bits(50.5))))
    return list(dict.fromkeys(cases))


class PostDestroyOracle:
    """실제HP/finder/다리쓰기는원본명령이고하위생성/예약/공통post만명시경계로관찰한다."""
    prepare_registers=GeometryOracle.prepare_registers
    check=GeometryOracle.check

    def __init__(self,edition):
        """현재readOnly내보내기의불연속body만허용하며직접PE메모리/원본전역을준비한다."""
        self.edition,self.s,self.spec=edition,SPECS[edition],PE_SPECS[edition]
        pe=pefile.PE(str(ROOT/self.spec["binary"]));data=pe.get_memory_mapped_image();self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(0x400000,(len(data)+4095)&~4095);self.mu.mem_write(0x400000,data)
        # 분석메모리페이지는해시/풀/타입/생성관찰과스택을각각분리한다.
        for address,size in ((0,4096),(POOL,0x10000),(TYPES,0x20000),(HASH,0x30000),(HEAP,0x10000),(STOP,4096),(STACK_BASE,0x40000)):
            self.mu.mem_map(address,size)
        self.allowed=bytearray(0x200000);self.entries=set();self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        self.exports=[ROOT/f"extracted/damageablepostdestroy/{edition}/{name}" for name in ("creation.c","functions.tsv")]
        with self.exports[1].open(encoding="utf-8") as stream:
            # 원본전체body의정확한불연속범위만등록한다.
            for row in csv.DictReader(stream,delimiter="\t"):
                self.entries.add(int(row["entry"],16))
                for segment in row["ranges"].split(";"):
                    low,high=(int(value,16) for value in segment.split("-"));self.allowed[low-0x400000:high+1-0x400000]=b"\1"*(high+1-low)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def read(self,address,width=4):
        """현재raw/경계관찰메모리의지정폭만읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),"little")

    def put(self,address,value,width=4):
        """명시입력/하위경계변경만지정한little endian폭에쓴다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,"little"))

    def slot(self,sid):
        """원본판본의SID×실제stride주소다."""
        return POOL+sid*self.s["stride"]

    def write(self,mu,access,address,size,value,data):
        """원본명령의쓰기는stack/FS/raw후보/할당Regularcount에만허용한다."""
        if address==0 and size==4:return
        if any(low<=address and address+size<=low+length for low,length in ((STACK_BASE,0x40000),(POOL,128*self.s["stride"]),(HEAP,0x6000))):return
        raise RuntimeError(f"PostDestroy 허용밖쓰기 {self.edition} {address:08x}+{size}")

    def instruction(self,mu,address,size,data):
        """실제첫드롭/helper/finder를실행하고이름이정해진하위효과만ABI대로반환한다."""
        s=self.s;targets={s[key] for key in ("drop_random","drop_residence","island","base","alloc","regular","fall","notify")}|{UPDATE,DESTROY}
        if address not in targets:
            if not 0x400000<=address<0x600000 or not self.allowed[address-0x400000]:
                raise RuntimeError(f"PostDestroy 허용밖native {self.edition} {address:08x}")
            if address in self.entries:self.native[f"{address:08x}"]+=1
            return
        self.boundaries[f"{address:08x}"]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(sp+4+4*index) for index in range(4)];purge=value=0;c=self.case
        if address in (s["drop_random"],s["drop_residence"]):
            self.events.append(f"D:{args[0]}:{args[1]}:{int(address==s['drop_random'])}")
            if c["mutation"]==1:self.put(self.slot(SOURCE)+10,91,1)
            if c["mutation"]==6:self.put(self.slot(SOURCE)+s["owner"],3,1)
        elif address==s["island"]:
            self.events.append("I:"+":".join(map(str,args)))
            if c["mutation"]==2:self.put(self.slot(SOURCE)+s["extra"],1,1)
        elif address==s["alloc"]:
            assert args[0]==40;value=HEAP+len(self.regulars)*40;self.mu.mem_write(value,bytes(40))
        elif address==s["regular"]:
            # constructor의새body를대조한것이아니라명시생성경계다. 후속+20쓰기는부모원본명령이다.
            self.events.append(("R",this,args[1],args[0],args[2]));self.regulars.append(this);value=this;purge=12
            if c["mutation"]==4:self.put(self.slot(61)+11,2,1)
        elif address==s["fall"]:
            self.events.append(f"F:{args[0]}");value=int(c["scene"]==3)
        elif address==s["notify"]:
            self.events.append(f"N:{args[0]}")
            if c["mutation"]==3:self.put(self.slot(args[0])+s["frame"],2,s["frame_width"])
        elif address==UPDATE:
            sid=(this-POOL)//s["stride"];self.events.append(f"U:{sid}:{self.read(this+s['frame'],s['frame_width'])}")
            if c["mutation"]==5:self.put(this+12,0xfedc,2)
        elif address==DESTROY:
            sid=(this-POOL)//s["stride"];self.events.append(f"X:{sid}:{args[0]}");purge=4
        elif address==s["base"]:
            self.events.append(f"B:{(this-POOL)//s['stride']}:{args[0]}");purge=4
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def setup(self,case):
        """명시현재타입/풀/해시/전역만준비하며finder결과와효과순서는계산하지않는다."""
        s=self.s;self.case=case;self.events=[];self.regulars=[]
        self.mu.mem_write(POOL,bytes(128*s["stride"]));self.mu.mem_write(TYPES,bytes(0x20000));self.mu.mem_write(HASH,bytes(0x30000))
        self.put(self.spec["globals"]["pool"],POOL);self.put(self.spec["globals"]["types"],TYPES);self.put(s["capacity"],128)
        for key in ("battle","boss","dudes","event","crack"):self.put(s[key],case[key])
        self.put(VTABLE+0x88,UPDATE);self.put(VTABLE+0x10,DESTROY)
        # 네단계해시는유일한입력SID머리이며양판본현재원본inline배열주소를사용한다.
        bases=[];cursor=HASH
        for scale in SCALES:bases.append(cursor);cursor+=(256//scale)**2*2
        self.mu.mem_write(s["grid"]-12,struct.pack("<7I",0,1,256,*bases))
        source=(SOURCE,case["type"],2,case["extra"],case["x"],case["y"],case["footx"],case["footy"],1)
        objects=[source,(60,90,0,0,bits(49.25),bits(49.25),1,1,1),(61,90,0,1,bits(49.5),bits(49.5),1,1,1),
                 (62,92,0,0,bits(50),bits(50),1,1,1),(63,82,0,0,bits(49),bits(49),1,1,0),
                 (64,82,0,0,bits(50),bits(49),1,1,0),(65,94,0,0,bits(50.5),bits(50.5),3,3,1)]
        # 원본metadata와raw후보는입력이다. 일반Pop의새독립완료로세지않는다.
        for sid,typ,state,extra,xb,yb,width,height,level in objects:
            raw=bytearray(s["stride"]);raw[10]=typ;raw[11]=state;raw[s["extra"]]=extra;raw[s["owner"]]=2
            struct.pack_into("<I",raw,0,VTABLE);struct.pack_into("<II",raw,14,xb,yb);struct.pack_into("<H",raw,12,0x7355)
            frame=case["scene"]%4 if sid==63 else (1 if sid==64 else 0);raw[s["frame"]:s["frame"]+s["frame_width"]]=frame.to_bytes(s["frame_width"],"little")
            self.mu.mem_write(self.slot(sid),bytes(raw));type_addr=TYPES+typ*self.spec["type_stride"]
            foot=0x1d4 if self.edition=="originals" else 0x1b4
            self.mu.mem_write(type_addr+foot,struct.pack("<ii",width,height));self.put(type_addr+0xe8,0x10 if typ==90 else 0)
            self.put(type_addr+0xec,4 if typ==82 else (0x1000000 if typ==94 else 0));self.put(type_addr,100);self.put(type_addr+0x9c,0)
            self.put(type_addr+0x114,4);self.put(type_addr+0x124,HEAP+0x6000)
            if sid!=SOURCE:
                x=int(struct.unpack("<f",struct.pack("<I",xb))[0]);y=int(struct.unpack("<f",struct.pack("<I",yb))[0]);scale=SCALES[level]
                address=bases[level]+((y//scale)*(256//scale)+x//scale)*2;self.put(self.slot(sid)+4,self.read(address,2),2);self.put(address,sid,2)
        typ=TYPES+case["type"]*self.spec["type_stride"];self.put(typ+0xe8,case["flags1"]);self.put(typ+0xec,case["genus"])
        self.put(typ,case["hp"]);self.put(typ+0x9c,case["group"])
        self.mu.mem_write(HEAP+0x6000,bytes((65,80,1,0,65,80,11,32,65,80,21,64,65,80,11,96)))
        if self.edition=="originals":self.put(0x5e4794,0)

    def run(self,kind,values,control):
        """실제ABI/x87반환뒤전체raw후보와진단없는경계효과·원본count쓰기를관찰한다."""
        case=dict(zip(KEYS,values));self.setup(case);self.prepare_registers(control);self.mu.reg_write(UC_X86_REG_ECX,self.slot(SOURCE))
        args=[case["flags"]] if kind=="Post" else [SOURCE,case["genus"],case["flags"]]
        self.mu.mem_write(STACK,struct.pack("<"+"I"*(len(args)+1),STOP,*[value&0xffffffff for value in args]))
        self.mu.emu_start(self.s["post"] if kind=="Post" else self.s["drops"],STOP,count=1000000)
        self.check(control,STACK+8 if kind=="Post" else STACK+4);self.returns+=1
        events=[]
        # 부모원본명령이constructor직후쓴+20 count를최종관찰하며Python에서packed값을재계산하지않는다.
        for item in self.events:
            if isinstance(item,tuple):
                _,address,sid,event,payload=item;events.append(f"R:{sid}:{event}:{payload}:{self.read(address+32)}")
            else:events.append(item)
        raw=";".join(f"{sid}:{bytes(self.mu.mem_read(self.slot(sid),self.s['stride'])).hex()}" for sid in IDS)
        return [";".join(events) or "-",raw,0]


def generate(smoke=False):
    """두x87정밀도전체관찰을대조하고원본고유입력만한번계수한다."""
    rows=[];editions={};sources={Path(__file__),ROOT/"tools/ghidra/damageablepostdestroy-functions.json",
        ROOT/"tools/decomp_owner_oracle.py",ROOT/"tools/decomp_priestgeometry_oracle.py",ROOT/"tools/decomp_damageablepredestroy_oracle.py",
        ROOT/"cpppj/src/o/RawDamageablePostDestroy.h",ROOT/"cpppj/src/o/RawDamageablePostDestroy.cpp"}
    # 새부모/드롭의독립입력만세고기존HP/finder/다리helper진입은새고유입력이아니다.
    for edition in SPECS:
        oracle=PostDestroyOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:12]+cases[-12:]+[("Post",(0,1,1,0,89,0x410,0x4000,0,100,3,3,0,0,6699,5,0,bits(50.5),bits(50.5)))]
        for kind,values in cases:
            result=[oracle.run(kind,values,control) for control in CONTROLS]
            assert result[0]==result[1],(edition,kind,values)
            rows.append([edition,kind,",".join(map(str,values)),*result[0]])
        editions[edition]={"unique":len(cases),"normal_unique":len(cases),"assertion_unique":0,"returns":oracle.returns,
                           "native_calls":dict(oracle.native),"external_calls":dict(oracle.boundaries)}
        sources.update(oracle.exports);sources.add(ROOT/oracle.spec["binary"])
        print(f"{edition}: PostDestroy/드롭 {len(cases)}정상,두정밀도반환{oracle.returns}",flush=True)
    if smoke:return
    FIXTURE.write_bytes(("# Damageable PostDestroy/첫드롭 원본전체후보/count와 명시생성/공통post경계. helper중복합산0.\n"+
                        "\n".join("\t".join(map(str,row)) for row in rows)+"\n").encode("utf-8"));sources.add(FIXTURE)
    report={"host":"HJOW-Athlon","date":"2026-10-14","total":len(rows),"normal_total":len(rows),"assertion_total":0,
        "controls":list(CONTROLS),"editions":editions,"game_runs":0,"os_calls":0,"files":{p.relative_to(ROOT).as_posix():digest(p) for p in sorted(sources)},
        "limits":["실제PostDestroy/첫드롭/HP/finder/다리WORD·frame은원본명령이다. 모든명시하위경계와전체raw후보를관찰한다.",
                  "Dude생성/task5103/5104·섬정리·Regularconstructor/Host/공통post/공간표시·가상삭제는명시경계이며새완료body가아니다.",
                  "ActualHost의초기count setter는후속12공유API가필요하다. 생성/HP/Pop/owner를noop으로닫은실제월드완료를주장하지않는다."]}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+"\n").encode("utf-8"))


def main():
    """--smoke는무저장표본이며기본은현재독립원본전체관찰이다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--smoke",action="store_true")
    generate(parser.parse_args().smoke)


if __name__=="__main__":main()
