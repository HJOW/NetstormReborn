#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00417bd0 네 전략의 실제 분기·목록·난수·좌표와 명시 하위 호출 순서를 동결한다."""
import csv
import itertools
import json
import socket
import struct

from decomp_ai_bridge_candidates import CandidateOracle, ROOT, BODY, STACK, STOP, SIZE, POOL, TYPES, digest
from decomp_ai_bridge_start import bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ENTRY=0x417BD0  # 네 전략의 실제 공통 관문이다.
LISTS=(0x2E8,0x330,0x348,0x354)  # 같은 실제16DWORD 목록 네 개의 본체 위치다.
ACTUAL=(ENTRY,0x4AC200,0x49A840,0x40EA00,0x40E9B0,0x40F330,0x4ADEA0,0x49A900,0x41D7A0,0x41D610,0x40E800,0x40E850,0x411310,0x4558C0,0x4558F0,0x4E49C0)  # 계산은원본기계어를그대로실행한다.
BOUNDARIES={0x48FC90:'technology',0x40FCB0:'refresh',0x462BC0:'graph',0x410D40:'connected',0x4174C0:'advance',0x4112B0:'home',0x411350:'reclassify',0x410A30:'select',0x411470:'random',0x4173B0:'extend',0x4119B0:'rectangle',0x4105B0:'log',0x4E0620:'assert'}  # 이미독립검증된하위의명시입력/출력경계이며실제게임성공을뜻하지않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ai-bridge-strategies-x86.tsv'  # 새 원본 관측이며 기존 기대를 변경하지 않는다.
REPORT=ROOT/'cpppj/ai-bridge-strategies-20261022.json'  # 실제범위와대체경계를명시한다.


def word(raw,at,value):
    """caller가명시한DWORD만주변바이트를보존해설치한다."""
    struct.pack_into('<I',raw,at,value&0xFFFFFFFF)


def qbits(value):
    """게임시각의원문QWORD를보존한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


class StrategyOracle(CandidateOracle):
    """게임/OS없이원본분기·순수계산과명시된하위효과경계만실행한다."""

    def __init__(self):
        """현재호스트정밀범위와계산용실제TYPE를준비한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.ranges=[]
        for entry in ACTUAL:
            for part in facts[entry]['ranges'].split(';'):
                lo,hi=(int(value,16) for value in part.split('-'));self.ranges.append((lo,hi+1))
        for kind,genus,foot in ((97,0x10000000,3),(98,0x200,3),(99,0,1)):
            record=bytearray(500);word(record,0xEC,genus);word(record,0x1D4,foot);word(record,0x1D8,foot);self.mu.mem_write(TYPES+kind*500,bytes(record))
        self.counts={};self.trace=[]

    def instruction(self,mu,address,size,data):
        """하위의caller인자/출력만대체하고전략계산과목록조작은원본이수행한다."""
        if address in ACTUAL:
            self.counts[hex(address)]=self.counts.get(hex(address),0)+1
        if address==0x4ADEA0:
            self.trace.append(f'center,{(mu.reg_read(UC_X86_REG_ECX)-POOL)//50}')
        if address in BOUNDARIES:
            esp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<8I',mu.mem_read(esp,32));ret=a[0];name=BOUNDARIES[address];purge=0;result=0
            if name=='technology':self.trace.append(f'technology,{a[1]}');result=50
            elif name=='refresh':
                self.trace.append(f'refresh,{a[1]}');purge=4
                if self.cfg[13]:mu.mem_write(BODY+0x2D4,struct.pack('<I',self.cfg[13]))
            elif name=='graph':self.trace.append(f'graph,{a[1]}');result=self.cfg[4]
            elif name=='connected':self.trace.append(f'connected,{a[1]}');result=self.cfg[5];purge=4
            elif name=='advance':self.trace.append(f'advance,{a[1]},{a[2]}');result=self.cfg[6];purge=8
            elif name=='home':
                self.trace.append('home');mu.mem_write(a[1],struct.pack('<2I',bits(101.25),bits(102.75)));result=a[1];purge=4
            elif name=='reclassify':self.trace.append('reclassify')
            elif name=='select':self.trace.append(f'select,{a[2]},{a[3]},{a[4]},{a[5]}');result=self.cfg[12];purge=20
            elif name=='random':
                self.trace.append('random,'+','.join(map(str,a[2:6])));mu.mem_write(a[1],struct.pack('<2I',*self.cfg[8:10]));result=a[1];purge=20
            elif name=='extend':
                self.trace.append('extend,'+','.join(map(str,a[2:6])));mu.mem_write(a[1],struct.pack('<2I',*self.cfg[10:12]));result=a[1];purge=20
            elif name=='rectangle':self.trace.append('rectangle,'+','.join(map(str,a[1:5])));result=self.cfg[7];purge=16
            elif name=='log':
                event={0x5018CC:0,0x5018B4:1,0x5018A0:2}[a[3]];self.trace.append(f'log,{a[2]},{event},{a[4]}')
            elif name=='assert':self.trace.append(f'assert,{a[3]}')
            mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==0x411310:self.trace.append('delay')
        if not any(lo<=address<hi for lo,hi in self.ranges):raise RuntimeError(f'검토범위밖명령 {address:08x}')

    def sample(self,body,lists,slots,cfg,precision):
        """같은입력의전체본문/목록/난수/순서와정상ABI보존을관측한다."""
        self.cfg=cfg;self.trace=[];self.mu.mem_write(BODY,bytes(body));self.mu.mem_write(BODY+SIZE,bytes(lists));self.mu.mem_write(POOL+50*50,bytes(slots))
        for address,value in ((0x540410,254),(0x531928,256),(0x5E4794,cfg[2]),(0x54DB88,cfg[3]),(0x532710,cfg[1]),(0x52F3E4,29)):
            self.mu.mem_write(address,struct.pack('<I',value))
        self.mu.mem_write(0x55B4D0,struct.pack('<Q',cfg[0]));self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,BODY)
        self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xFFFF)
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):self.mu.reg_write(register,value)
        self.mu.emu_start(ENTRY,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.mu.reg_read(UC_X86_REG_FPCW)==precision
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):assert self.mu.reg_read(register)==value
        assert bytes(self.mu.mem_read(POOL+50*50,len(slots)))==slots
        return bytes(self.mu.mem_read(BODY,SIZE)).hex(),bytes(self.mu.mem_read(BODY+SIZE,len(lists))).hex(),struct.unpack('<I',self.mu.mem_read(0x532710,4))[0],';'.join(self.trace)


def scene(goal):
    """결과를계산하지않고명시본문/네목록/여섯슬롯/하위입력만준비한다."""
    body=bytearray([0xA5])*SIZE;lists=bytearray([0xCD])*256;slots=bytearray(300)
    values={0x294:50,0x298:7,0x248:0x1000,0x260:2,0x2D4:goal,0x2A8:51,0x3CC:51 if goal==1 else 52 if goal==2 else 0,0x3D0:bits(20),0x3D4:bits(30),0x3D8:3,0x3C8:4,0x270:bits(60),0x274:bits(60),0x278:bits(110),0x27C:bits(110),0xAF0:bits(150.75),0xAF4:bits(145.25),0xB00:1}
    for at,value in values.items():word(body,at,value)
    for at in (0x5A8,0x5B0,0x5D0):struct.pack_into('<Q',body,at,qbits(10.0))
    for index,(at,active) in enumerate(zip(LISTS,((53,),(52,54),(52,54,52),(54,54)))):
        word(body,at,BODY+SIZE+index*64);word(body,at+4,8);word(body,at+8,len(active))
        for slot,value in enumerate(active):word(lists,index*64+slot*4,value)
    for index,(kind,x,y) in enumerate(((98,100,100),(98,150.75,150.25),(97,125.75,125.25),(99,75.75,80.25),(97,135.75,130.25),(99,2,2))):
        at=index*50;slots[at+10]=kind;slots[at+34]=2;word(slots,at+14,bits(x));word(slots,at+18,bits(y))
    cfg=[qbits(10),0,0,0,8,0,1,1,bits(65.25),bits(70.75),bits(66.5),bits(71.5),54,0]
    return body,lists,slots,cfg


def main():
    """네case·단락관문·기한/NaN·중복목록/용량·좌표/진단을독립원문으로동결한다."""
    assert not FIXTURE.exists() and not REPORT.exists();cases=[]
    for goal,now in itertools.product((0,1,2,3,4,5),(9.0,10.0,11.0,float('nan'))):
        row=scene(goal);row[3][0]=qbits(now);cases.append(row)
    for field,value in ((0x294,0),(0x298,254),(0x248,0),(0x248,0x2000)):
        for goal in (1,2,3,4):row=scene(goal);word(row[0],field,value);cases.append(row)
    for goal,checking,disabled in itertools.product((1,2,3,4),(0,1),(0,1)):
        row=scene(goal);row[3][2:4]=[checking,disabled];cases.append(row)
    for goal,target,dead,graph in itertools.product((1,2),(0,51,52),(0,2),(7,8)):
        row=scene(goal);word(row[0],0x3CC,target);row[2][max(0,target-50)*50+11]=dead;row[3][4]=graph;cases.append(row)
    for connected,advance,given,due,full in itertools.product((0,1),(0,1),(0,2),(9.0,10.0,11.0,float('nan')),(0,1)):
        row=scene(2);row[3][5:7]=[connected,advance];word(row[0],0x35C,given);struct.pack_into('<Q',row[0],0x5D0,qbits(due))
        if full:word(row[0],0x334,2);word(row[0],0x34C,3);word(row[0],0x358,2)
        cases.append(row)
    for seed,count,position in itertools.product((0,1,0xFFFFFFFF),(0,1),((75.75,80.25),(2,2),(253,253),(-4,-4),(float('nan'),80))):
        row=scene(3);row[3][1]=seed;word(row[0],0x2F0,count);word(row[2],3*50+14,bits(position[0]));word(row[2],3*50+18,bits(position[1]));cases.append(row)
    for point,advance,rectangle in itertools.product(((150.75,145.25),(-10000,-10000),(float('nan'),145.25),(0,0),(255.9999,1)),(0,1),(0,1)):
        row=scene(4);word(row[0],0xAF0,bits(point[0]));word(row[0],0xAF4,bits(point[1]));row[3][6:8]=[advance,rectangle];cases.append(row)
    for goal in (1,2,3,4):
        row=scene(goal);row[3][13]=5-goal;cases.append(row)
    oracle=StrategyOracle();rows=[]
    for index,(body,lists,slots,cfg) in enumerate(cases):
        observed=[oracle.sample(body,lists,slots,cfg,precision) for precision in (0x27F,0x37F)];assert observed[0]==observed[1]
        rows.append((index,','.join(map(str,cfg)),body.hex(),lists.hex(),slots.hex(),*observed[0]))
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 원본00417bd0:번호/하위입력/본문입력/목록입력/슬롯입력/본문출력/목록출력/난수/호출순서\n');stream.writelines('\t'.join(map(str,row))+'\n' for row in rows)
    report={'host':socket.gethostname(),'decompile_host':'VM-W11-Codex','cases':len(rows),'normal_returns':len(rows)*2,'x87':[0x27F,0x37F],'actual_entries':oracle.counts,'boundary_stubs':{hex(key):value for key,value in BOUNDARIES.items()},'game_started':False,'sources':{str(path.relative_to(ROOT)):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'originalCD/NETSTORM.EXE',ROOT/'tools/decomp_ai_bridge_strategies.py',FIXTURE)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'cases':len(rows),'returns':len(rows)*2}))


if __name__=='__main__':main()
