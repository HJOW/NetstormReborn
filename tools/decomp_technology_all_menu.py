#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 HTML 기술 목록004cb690과 기존 세 하위/실제CRT를 격리 실행한다(706)."""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_technology_list_menu import Oracle as ThemeOracle, POOL, TYPES, VTABLE, LEVEL, DESCRIPTION, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_EFLAGS
from decomp_oracle import STACK, STOP

GRID,HEAP,PLAYERS,NAME=0x13000000,0x14000000,SCRATCH+0x18000,SCRATCH+0x17000  #격리 표면 WORD/여덟 목록/현재 Player/원소 이름의 별도 물리다.

def bits(value):
    """독립 float32 좌표의 원시 입력을 반환한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def cases():
    """랭크/멀티플레이와 실제 희생 없이 메뉴 레벨·본섬·표면 범위 입력을 교차한다."""
    rows=[]
    for level,count,owned,variant in itertools.product((-1,0,1,2,3,4),(16,80),(0,1),range(4)):  #항상 미보유 기술이 남아 전체보유 MP랭크 분기를 실행하지 않는다.
        x,y,surface,home,island=((10.25,10.25,11,7,7),(10.25,10.25,11,8,7),(255.5,255.5,64,7,7),(-2.0,10.25,11,7,7))[variant]
        rows.append((3,0,level,count,owned,40,0,bits(x),bits(y),surface,home,island,0))
    rows.extend((3,0,0,16,0,40,0,bits(x),bits(y),surface,7,7,mutation) for x,y,surface,mutation in ((-0.5,255.0,1,0),(256.0,10.25,11,0),(10.25,10.25,0,0),(10.25,10.25,11,1)))  #0쪽 절삭/255 경계/NULL 표면/섬 getter 뒤 Player 변경이다.
    return rows

class Oracle(ThemeOracle):
    """공유 세 몸체와 전체 부모/CRT를 실행하고 UI·게임 조회만 명시 경계로 둔다."""
    def __init__(self):
        """현재 PC 정밀 범위를 추가하고 보호 PE를 읽기 전용으로 적재한다."""
        super().__init__()
        with Path('extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream: facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.entries['all']=0x4cb690
        for address in (0x4cb690,0x4e49c0):  #좌표 CRT 절삭도 실제 x87 명령이며 Python 기대 계산으로 대체하지 않는다.
            for part in facts[address]['ranges'].split(';'): lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_map(GRID,0x20000);self.mu.mem_map(HEAP,0x10000);self.put(0x542514,GRID);self.put(0x5c847c,64);self.put(0x59534c,PLAYERS);self.put(0x540c70,1)
        assert self.get(0x542918)==0x02000089 and self.get(0x542a44)==0x020000d4

    def on_instruction(self,mu,address,size,data):
        """원문 제어는 그대로 실행하고 실제 장치 호출의 순서·인자·라벨을 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c,d,e,f=struct.unpack('<7I',mu.mem_read(sp,28));this=mu.reg_read(UC_X86_REG_ECX);value=0;purge=0
        if address==0x475c40: self.event('C');value=201
        elif address==0x4644e0: assert this==201 and a==10;self.event('B',a);purge=4
        elif address==0x4a8940: self.event('E',a);self.mu.mem_write(NAME,f'E{a}'.encode()+b'\0');value=NAME
        elif address==0x478260: self.event('S')
        elif address==0x4acda0:
            self.event('J',(this-POOL)//50);value=self.row[11]
            if self.row[12]: self.put(PLAYERS+0xb4+0x78,99)
        elif address==0x4e4391: assert a==0xa0;value=HEAP+self.allocations*0x1000;self.allocations+=1
        elif address==0x4de9a0: self.event('X',self.read_string(a).decode());value=a;purge=4
        elif address==0x4a89c0:
            pattern=self.read_string(b).decode()
            if pattern=='%s (Level %d)': name=self.read_string(c).decode();label=pattern%(name,d);self.event('F',int(name[1:]),d)
            elif '%s' in pattern: name=self.read_string(c).decode();label=pattern%name;self.event('FS',pattern,name)
            else: number=c if c<0x80000000 else c-0x100000000;label=pattern%number;self.event('FN',pattern,c)
            self.mu.mem_write(a,label.encode()+b'\0');value=a
        elif address==0x477cb0: assert d==1 and f==0;self.event('I',self.read_string(a).decode(),b,c,e)
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """독립 TYPE·실제 float좌표·표면/Player를 쓰고 원문 전체 경계 trace를 반환한다."""
        self.row,self.trace,self.allocations=row,[],0;self.put(0x541348,row[3]);self.put(0x5412c0,154);self.put(PLAYERS+0xb4+0x78,row[10]);self.mu.mem_write(GRID,struct.pack('<H',row[9])*65536);self.put(POOL+10*50+14,row[7]);self.put(POOL+10*50+18,row[8])
        for kind in range(80):  #공중3종/실제월드/희생을 만들지 않는 독립 TYPE 표다.
            base=TYPES+kind*500;self.put(base+0x94,kind%4-1);self.put(base+0x98,kind%6);self.put(base+0x9c,10 if kind%11==0 else 0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP): self.mu.reg_write(reg,0)  #공유 helper 단독보다 긴 부모에만 별도 유한 실행 한도를 쓴다.
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<2I',STOP,0x1234000a));self.mu.emu_start(self.entries['all'],STOP,timeout=3000000,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.allocations==8,'전체 목록 정상 반환/ABI/할당 경계'
        return ','.join(map(str,row)),';'.join(self.trace)

def main():
    """기존 fixture를 덮지 않고 새 실제 관측과 근거 해시를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# input13 trace']
    for row in cases(): rows.append('\t'.join(oracle.run(row)))  #제품 구현을 원문 기대값 생성에 사용하지 않는다.
    args.fixture.write_text('\n'.join(rows)+'\n','utf-8',newline='\n');record=dict(unit=706,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004cb690','004cb320','004cb3f0','004cb4b0','CRT004e49c0'],explicitBoundaries=['virtual level','TYPE address/current technology/owned kind','current menu and reference','theme name/translate/description/printf/item/separator','surface island getter; Player field remains native memory','heap/assert'],excluded=['full ownership rank branch: static preimplementation only','capture/sacrifice/enemy/victory/airborne-three actions','MP/network/game execution'],peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),parentToolSha256=hashlib.sha256(Path('tools/decomp_technology_list_menu.py').read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__': main()
