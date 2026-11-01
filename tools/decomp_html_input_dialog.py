#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""HTML Input 생성/공장/확인 몸체를 게임 없이 격리 실행한다(708)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

PARENT,VTABLE,KEY,TITLE=SCRATCH+0x1000,SCRATCH+0x2000,SCRATCH+0x3000,SCRATCH+0x4000  #원문 파생/가상표/후속 섹션/제목의 격리 메모리다.

def bits(value):
    """float32의 실제 비트를 기록한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def cases():
    """공통 생성의 제목 크기/모드/화면과 확인 문자열의 독립 입력을 만든다."""
    rows=[('C',mode,bits(width),641,481,b'next',b'Prompt',0) for mode,width in itertools.product((0,1,2,0xffffffff),(0.,10.,180.,240.,300.))]  #비멀티 일반 HTML 입력이며 실제 게임 상태를 만들지 않는다.
    text=(b'',b'Alpha',b'  Alpha  ',b'\nIgnored',b'42\nIgnored',b'0012 ',b'-7x',b'()[]',b'A\r',b'A\0Ignored',b'x'*240)
    rows.extend(('A',mode,0,0,0,key,value,0) for mode,key,value in itertools.product((0,1,2,0xffffffff),(b'',b'next'),text))  #기술·포획·희생·MP 요청과 구별한다.
    rows.append(('A',2,0,0,0,b'next',b'changed \nignored',1))  #CopyText 효과 뒤 같은 mode/후속 섹션을 다시 읽는 입력이다.
    return rows

class Oracle(BaseOracle):
    """세 부모와 strchr/ftol을 실행하며 큰 공통 UI/문자/CRT 효과만 명시 경계로 둔다."""
    def __init__(self):
        """현재 보호 PE와 같은 PC의 검토 범위를 읽기 전용으로 적재한다."""
        super().__init__('originals',[dict(name='accept',originals='004cfae0'),dict(name='construct',originals='004d3c20'),dict(name='show',originals='004d3ed0')],[0x4e49c0,0x4e63e0]);self.mu.mem_map(0,0x1000);self.trace=[];self.allocated=0;self.put(VTABLE+0x20,0x466750);self.put(VTABLE+0x18,0x464560);self.mu.mem_write(TITLE,b'Prompt\0')

    def put(self,address,value):
        """비정렬 DWORD의 실제 하위 비트를 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """실제 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def event(self,*values):
        """장치에 전달된 실제 인자/문자열을 같은 순서로 기록한다."""
        self.trace.append(':'.join(map(str,values)))

    def allocate(self):
        """부모와 여섯 자식은 독립 실제 raw 물리를 사용한다."""
        value=PARENT if not self.allocated else SCRATCH+0x5000+self.allocated*0x1000;self.allocated+=1;self.mu.mem_write(value,bytes(0x200));self.put(value,VTABLE);return value

    def on_instruction(self,mu,address,size,data):
        """허용한 부모 제어는 실행하고 명시 UI/번역/복사/서식 경계만 제공한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c,d,e,f=struct.unpack('<7I',mu.mem_read(sp,28));this=mu.reg_read(UC_X86_REG_ECX);value=0;purge=0
        if address==0x4e4391: value=self.allocate()
        elif address==0x487500:
            for offset,number in ((0x30,12),(0x38,bits(1)),(0x3c,bits(200)),(0x40,bits(100)),(0x44,bits(200)),(0x48,bits(100))): self.put(this+offset,number)  #공통 두 버튼 부모의 실제 초기 크기만 공급한다.
        elif address==0x452430:
            self.entry=this;self.put(this+0x3c,bits(a));self.put(this+0x40,bits(b));self.event('entry',self.read_string(c).hex());value=this;purge=24
        elif address==0x4655d0: self.border=this;self.put(this+0x3c,a);self.put(this+0x40,b);value=this;purge=24
        elif address==0x466750: self.put(this+0x4c,a);self.put(this+0x50,b);purge=16
        elif address==0x4de9a0: self.event('translate',self.read_string(a).hex());value=a;purge=4
        elif address==0x487260:
            self.title=self.allocate();self.put(this+0xc4,self.title);self.put(self.title+0x3c,bits(20));self.put(self.title+0x40,bits(12));self.put(self.title+0x4c,a);self.put(self.title+0x50,bits(10));self.put(self.title+8,4);self.titleWidth=int(struct.unpack('<f',struct.pack('<I',a))[0]);self.event('title',self.titleWidth,self.read_string(b).hex());purge=8
        elif address==0x4bb310: self.put(this+0x3c,self.row[2]);purge=8
        elif address==0x464560: pass
        elif address==0x487530: self.allocate();self.allocate();purge=4  #기존 공통 버튼 생성/배치는 명시 경계이며 새로 검증했다고 세지 않는다.
        elif address==0x4652b0: self.depth=a;purge=4
        elif address==0x466600: self.modal=a;purge=4
        elif address==0x451e80:
            self.event('copy');mu.mem_write(a,(self.row[6]+b'\0')[:256]);purge=8
            if self.row[7]: self.put(PARENT+0xd4,1);mu.mem_write(PARENT+0xd8,b'changed\0')
        elif address==0x4e701a: self.event('space',a);value=0
        elif address==0x4e66de: self.event('atol',self.read_string(a).hex())
        elif address==0x4a89c0:
            assert self.read_string(b)==b'%s(%s)';key,text=self.read_string(c),self.read_string(d);self.event('format',key.hex(),text.hex());mu.mem_write(a,key+b'('+text+b')\0');value=a
        elif address==0x4cf730: assert b==0;self.event('tell',self.read_string(a).hex())
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """독립 입력을 준비하고 실제 raw 필드/확인 반환과 경계 trace를 읽는다."""
        self.row,self.trace,self.allocated=row,[],0;self.depth=self.modal=0;self.put(0x5c78f0,row[3]);self.put(0x5c78f4,row[4]);self.put(0x563e38,99);self.mu.mem_write(KEY,row[5]+b'\0');self.mu.mem_write(TITLE,row[6]+b'\0')
        if row[0]=='A':
            self.mu.mem_write(PARENT,bytes(0x128));self.put(PARENT+0xd0,SCRATCH+0x6000);self.put(PARENT+0xd4,row[1]);self.mu.mem_write(PARENT+0xd8,row[5]+b'\0');result=self.call('accept',[],PARENT);snapshot='-'
        else:
            self.call('show',[row[1],TITLE,KEY]);result=0;values=[self.get(PARENT),self.get(PARENT+0x20),self.get(PARENT+0xd4)]
            for address,offsets in ((PARENT,(0x3c,0x40,0x44,0x48,0x4c,0x50)),(self.entry,(0x3c,0x40,0x100,0xf4,0x4c,0x50)),(self.border,(0x3c,0x40,0x4c,0x50)),(self.title,(0x3c,0x40,0x4c,0x50,8))): values.extend(self.get(address+offset) for offset in offsets)  #기존helper전체ABI대신이번부모가읽고쓴실제필드만비교한다.
            values.extend((self.titleWidth,self.depth,self.modal));snapshot=','.join(map(str,values));assert self.read_string(PARENT+0xd8)==row[5]
        encoded=[row[0],*map(str,row[1:5]),row[5].hex() or '-',row[6].hex() or '-',str(row[7]),str(result),snapshot,';'.join(self.trace) or '-'];return '\t'.join(encoded)

def main():
    """기존 fixture를 덮지 않고 새 실제 관측만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# action mode titleWidthBits screenW screenH continuationHex textHex mutation result snapshot trace']
    for row in cases(): rows.append(oracle.run(row))  #제품 모델로 기대값을 만들지 않는다.
    args.fixture.write_text('\n'.join(rows)+'\n','utf-8',newline='\n');report=dict(unit=708,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004d3c20','004d3ed0','004cfae0','CRT004e49c0','CRT004e63e0'],explicitBoundaries=['common two-button base/entry/background/title/helper buttons/attach/modal/depth','translation/copy/isspace/atol/printf/Tell','heap; no allocation failure/SEH ABI claim'],gameExecutions=0,mpTests=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest());args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__': main()
