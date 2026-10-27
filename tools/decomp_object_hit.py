#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""498000의 실제 좌표/헤더/고정 캡처/복원을 관측한다. 가상 getter와 픽셀 출력만 명시 경계다."""
import argparse
import hashlib
import itertools
import json
import random
import re
import socket
import struct
import zlib
from decomp_oracle import Oracle,ROOT,SCRATCH,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW
import pefile

RAW=SCRATCH+0x18000  # 합성TYPE80의 단일50BYTE 객체다. 실제 유닛 생성자는 실행하지 않는다.
VT=SCRATCH+0x19000  # 원래+78/+a4 호출 경계만 설치한다.
SHAPE=SCRATCH+0x1a000  # 네 물리 헤더의 원문 주소표다.
PIXELS=0x5caff0  # 원본200×200 캡처 전역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/object-hit-x86.tsv'  # 원문 결과와 정확한 호출 순서다.
TABLE=ROOT/'cpppj/src/client/ObjectHitVtables.inc'  # 보호PE에서 읽은83개 실제 슬롯이다.
REPORT=ROOT/'cpppj/object-hit-native-20261028.json'  # 관측 범위와 입력 해시다.


def digest(path):
    """증거의 전체 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def vtables():
    """기존83개 선택 가상 표의+78/+a4/+b0를 보호PE에서 읽는다."""
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));image=pe.get_memory_mapped_image();rows=[]
    for value in re.findall(r'\{0x([0-9a-f]+)U,', (ROOT/'cpppj/src/o/SelectionVtables.inc').read_text('utf-8')):  # 실제 생성자 표의 순서를 보존한다.
        address=int(value,16);slots=[struct.unpack_from('<I',image,address-0x400000+offset)[0] for offset in (0x78,0xa4,0xb0)]
        assert slots[2]==0x498000;rows.append('    {'+','.join(f'0x{item:08x}U' for item in (address,*slots))+'},')
    assert len(rows)==83
    return '// 보호10.78의83VT: 주소,+78,+a4,+b0다. 호스트 포인터/동적 생성 검증이 아니다.\nconstexpr std::array<ObjectHitVtable,83> kObjectHitVtables{{\n'+'\n'.join(rows)+'\n}};\n'


def cases():
    """signed 헤더·투명255·opaque·음수·200 경계와 getter 재호출을 교차한다."""
    result=[];rng=random.Random(0x5241078)
    for flags,extra,rx,ry in itertools.product((0,0x800000),(0,2),(-1,0,1,2,3,31,199,200),(-1,0,2,3,31,199,200)):  # 경계 바깥에서도 그림 호출 여부를 기록한다.
        result.append((0x42480000,0x42480000,700,500,97+rx,52+ry,flags,extra,1,0,0,0,32,32,0,0,1))
    for index in range(384):  # 분수/음수 좌표·기준점·배율 비적용·실제값 재조회의 명시 입력이다.
        x=rng.choice((-3.5,-0.03125,0,0.03125,50.03125,50.5,51));y=rng.choice((-2.5,-0.04545455,0,0.04545455,50.5,51))
        xb=struct.unpack('<I',struct.pack('<f',x))[0];yb=struct.unpack('<I',struct.pack('<f',y))[0]
        cx,cy=rng.choice((-800,0,700)),rng.choice((-600,0,500));hx,hy=rng.choice((-12,0,4,32)),rng.choice((-9,0,3,40))
        result.append((xb,yb,cx,cy,rng.randrange(-20,850),rng.randrange(-20,600),rng.choice((0,0x800000)),rng.choice((0,1,2,3,0x12)),rng.randrange(4),*rng.choice(((-1,-1,-1),(0,0,0),(1,2,3),(2,1,-1))),rng.choice((-1,0,1,32,201)),rng.choice((-1,0,1,32,201)),hx,hy,rng.randrange(3)))
    return result


class HitOracle(Oracle):
    """원문498000/419850/캡처 문맥/CRT를 실행하며 OS나 게임 진입점을 호출하지 않는다."""
    def __init__(self):
        """검토한 몸체와 가상 출력 경계만 가상 메모리에 준비한다."""
        super().__init__('originals',[dict(name='Hit',originals='00498000')],[0x4ac550,0x49a840,0x419850,0x4a2b10,0x4a2c70,0x4e49c0])
        self.mu.mem_map(0,4096);self.mu.mem_write(VT+0x78,struct.pack('<I',STOP+0x100));self.mu.mem_write(VT+0xa4,struct.pack('<I',STOP+0x200));self.instructions=0
        self.allowed.append((0x4e49c0,0x4e4a35))  # Ghidra CRT 끝 경계의 실제 절삭 명령도 허용한다.

    def word(self,address):
        """현재DWORD를 주소 정렬과 무관하게 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def finish(self,value=0,purge=0):
        """명시 경계만 원래 스택 정리로 반환한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_EIP,self.word(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def pixel(self,x,y,value):
        """명시한 출력 경계의 한 픽셀만 원본 캡처 문맥에 쓴다."""
        if 0<=x<200 and 0<=y<200:self.mu.mem_write(PIXELS+y*200+x,bytes((value,)))

    def on_instruction(self,mu,address,size,data):
        """조회 인자/순서를 기록하고 출력 외 선택 계산을 대체하지 않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP)
        if address==STOP+0x100:
            value=self.case[9+min(self.getter,2)];self.getter+=1;self.trace.append('frame:'+str(value));self.finish(value);return
        if address==0x4daa70:
            shape,frame,x,y=[self.word(sp+4+i*4) for i in range(4)];assert shape==SHAPE
            x,y=[value if value<0x80000000 else value-0x100000000 for value in (x,y)];self.trace.append(f'plain:{frame},{x},{y}')
            self.pixel(x,y,(255,9,0,255)[frame]);self.pixel(x+1,y+1,(255,9,0,255)[frame]);self.finish();return
        if address==STOP+0x200:
            x,y=struct.unpack('<ii',mu.mem_read(sp+4,8));self.trace.append(f'extra:{x},{y}')
            if self.case[16]:self.pixel(x+2,y+2,7 if self.case[16]==1 else 255)
            self.finish(purge=8);return
        return super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """현재 입력과 문맥을 설치하고 반환/출력/전체raw 불변성을 확인한다."""
        self.case=case;self.trace=[];self.getter=0;xb,yb,cx,cy,mx,my,flags,extra,current,_,_,_,width,height,hx,hy,_=case
        mu=self.mu;mu.mem_write(SCRATCH,bytes(500));mu.mem_write(SCRATCH,struct.pack('<I',1));mu.mem_write(SCRATCH+0xdc,struct.pack('<I',SHAPE));mu.mem_write(SCRATCH+0xe8,struct.pack('<I',flags));mu.mem_write(SCRATCH+0x114,struct.pack('<I',4));mu.mem_write(SCRATCH+0x1dc,struct.pack('<ii',3,-2))
        raw=bytearray(50);struct.pack_into('<I',raw,0,VT);raw[10]=80;raw[40]=extra;struct.pack_into('<II',raw,14,xb,yb);struct.pack_into('<I',raw,36,current);mu.mem_write(RAW,bytes(raw))
        for frame in range(4):  # 같은 signed 메타 헤더를 네 물리 번호에 설치한다.
            offset=0x100+frame*0x100;mu.mem_write(SHAPE+8+frame*8,struct.pack('<I',offset+36));mu.mem_write(SHAPE+offset+0x18,struct.pack('<hhhh',width,height,hx,hy))
        mu.mem_write(0x59ab20,struct.pack('<I',SCRATCH-80*500));mu.mem_write(0x59a91c,struct.pack('<ii',cx,cy));mu.mem_write(PIXELS,bytes([173])*40000)
        before=bytes(mu.mem_read(0x59af88,72));mu.reg_write(UC_X86_REG_FPCW,0x27f);value=self.call('Hit',[mx,my],RAW,8)&255
        assert bytes(mu.mem_read(RAW,50))==bytes(raw);assert bytes(mu.mem_read(0x59af88,72))==before
        return value,';'.join(self.trace),zlib.adler32(bytes(mu.mem_read(PIXELS,40000)))


def main():
    """새 증거만 작성하거나 원본과 기존fixture를 재실행해 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=HitOracle();rows=[]
    for case in cases():  # 기대값은 원문 실행에서만 얻는다.
        value,trace,checksum=oracle.observe(case);rows.append(','.join(map(str,case))+'\t'+str(value)+'\t'+trace+'\t'+str(checksum))
    content='# input=xBits,yBits,cameraX,cameraY,mouseX,mouseY,flags,extra,current,get1,get2,get3,width,height,hotX,hotY,extraPaint result trace captureAdler\n'+'\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8')==content;assert TABLE.read_text('utf-8')==vtables()
        for name,value in json.loads(REPORT.read_text('utf-8'))['sha256'].items():assert digest(ROOT/name)==value,name  # 이전 원문/도구/증거를 교체하지 않는다.
        print('픽셀 선택 원문 재실행',len(rows),'차이0');return
    assert not FIXTURE.exists() and not TABLE.exists() and not REPORT.exists(),'기존 증거를 덮어쓰지 않는다'
    FIXTURE.write_text(content,'utf-8',newline='\n');TABLE.write_text(vtables(),'utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_oracle.py','tools/decomp_object_hit.py',FIXTURE.relative_to(ROOT).as_posix(),TABLE.relative_to(ROOT).as_posix())
    REPORT.write_text(json.dumps(dict(unit=524,host=socket.gethostname(),lastDecompile='HJOW-Athlon / 2026-10-23',outputsModified=False,inputs=len(rows),instructions=oracle.instructions,assertions=oracle.assertions,boundaries='498000/4ac550/419850/4a2b10/4a2c70/실제CRT. +78은3회명시입력, plain과+a4는단일픽셀출력경계. 실제RLE/파생Draw/게임/OS/네트워크 실행 아님. 합성TYPE80만 사용.',sha256={name:digest(ROOT/name) for name in names}),ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print('원문 픽셀 선택',len(rows),'입력',oracle.instructions,'명령',oracle.assertions,'진단')


if __name__=='__main__':main()
