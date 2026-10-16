#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의9개 소유자 색표와 보호 SHP의 실제 mapped 캡처 전체 픽셀을 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle,ROOT,STACK,STOP,digest
from shp import read_blocks,load_type_texts,read_clusters,TYPE_LOAD_ORDER
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_GS

POOL,TYPES,MAP,BUFFER,OUT,GDT,SHAPE=(0x12000000,0x12100000,0x12200000,0x12300000,0x12400000,0x12500000,0x14000000)  # 서로 겹치지 않는 현재 입력/결과/실제 SHP 주소다.
ENTRIES={0x43b950,0x43ba00,0x4a7db0,0x4ac550,0x49a840,0x419850,0x4daa70,0x4daab0,0x4dabe0,0x401d92,0x4021b5,0x40227e,0x40229f,0x402796}  # 실제 생성/선택/캡처/RLE 몸체만 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/owner-capture-x86.tsv'  # 전체9개 색표 또는 캡처 픽셀의 원문이다.
REPORT=ROOT/'cpppj/owner-capture.json'  # 실제 입력/정밀도/근거 SHA와 외부 경계다.


class Oracle(CallOracle):
    """색표와 캡처를 대체하지 않고 new 요청만 격리 메모리에 연결한다."""
    def __init__(self):
        """현재 정밀 함수표와 보호 전체 SHP/평면 세그먼트를 준비한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # 원본 포인터/스택과 겹치지 않는 격리 영역이다.
        for address,size in ((POOL,0x10000),(TYPES,0x20000),(MAP,0x10000),(BUFFER,0x10000),(OUT,0x1000),(GDT,0x1000),(STACK-0x7f000,0x80000),(STOP,0x1000)):
            self.mu.mem_map(address,size)
        data=(ROOT/'originals/d/_shapes.shp').read_bytes();self.blocks=read_blocks(data);self.shapeSize=len(data);self.mu.mem_map(SHAPE,(len(data)+4095)&~4095);self.mu.mem_write(SHAPE,data)
        self.texts=load_type_texts(str(ROOT/'originals'));self.candidates=bytes(image[0x5319e8-0x400000:0x531b08-0x400000]);self.calls=Counter();self.returns=0;self.allocations=[];self.mode=0
        self.mu.mem_write(GDT,struct.pack('<3Q',0,0x00cf92000000ffff,0x00cf9a000000ffff));self.mu.reg_write(UC_X86_REG_GDTR,(0,GDT,23,0));self.mu.reg_write(UC_X86_REG_CS,16)
        # 실제 VFX의 push ds/pop es 뒤에도32비트 스택/주소를 유지한다.
        for register in (UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_GS):self.mu.reg_write(register,8)
        self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 전체 불연속 함수 구간을 보존하며 게임 밖 코드의 실행은 허용하지 않는다.
        for entry in ENTRIES:
            # tail helper의 실제 구간도 별도 유지한다.
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """요청 크기와 반환을 기록하고 실제 색표/캡처/RLE 계산을 계속 실행한다."""
        if address==0x4e4391:
            esp=mu.reg_read(UC_X86_REG_ESP);ret,amount=struct.unpack('<2I',mu.mem_read(esp,8));assert 0<amount<=0x10000
            if self.mode==0:assert amount==2304
            self.allocations.append(amount);mu.reg_write(UC_X86_REG_EAX,MAP if self.mode==0 else BUFFER);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'소유자 캡처 범위 밖 실행 {address:08x}')
        if address in ENTRIES:self.calls[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """실제 색표/출력/문맥/스택과 mapped VFX의256바이트 현재 색표만 쓸 수 있다."""
        regions=((MAP,MAP+2304),(BUFFER,BUFFER+0x10000),(OUT,OUT+16),(0x557940,0x557940+188*4),(0x5b5ddc,0x5b5df0),(0x52eac0,0x52ebc0),(STACK-0x7f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'소유자 캡처 범위 밖 쓰기 {address:08x}+{size}')

    def maps(self,typ,bridge,candidates,precision):
        """실제 원문 생성 후9×256 바이트를 읽는다. 후보는 명시 DWORD 입력이다."""
        self.mode=0;self.allocations=[];mu=self.mu;mu.mem_write(MAP,b'\xa5'*2304);mu.mem_write(0x5319e8,candidates);mu.mem_write(0x5411a0,struct.pack('<I',bridge))
        self.call(0x43ba00,0,struct.pack('<I',typ),0,precision);self.returns+=1;assert self.allocations==[2304]
        return bytes(mu.mem_read(MAP,2304))

    def capture(self,typ,frame,owner,color,zoom,extra,precision):
        """실제 TYPE/보호 헤더/소유자 번호를 입력해 캡처 전체와 문맥 복원을 관찰한다."""
        mu=self.mu;self.maps(typ,82,self.candidates,precision);self.mode=1;self.allocations=[];block=self.blocks[typ-70]
        count=len(read_clusters(self.texts[TYPE_LOAD_ORDER[typ-70].lower()]));record=bytearray(500);struct.pack_into('<I',record,0,20);struct.pack_into('<I',record,0xdc,SHAPE+block.offset);struct.pack_into('<I',record,0x114,count);struct.pack_into('<2i',record,0x1dc,-2,3);mu.mem_write(TYPES+typ*500,bytes(record))
        raw=bytearray(50);struct.pack_into('<I',raw,0,0x50a388 if typ==144 else 0x501dd0);raw[10]=typ;raw[34]=owner;struct.pack_into('<i',raw,36,frame);mu.mem_write(POOL+2500,bytes(raw))
        mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x59ab20,struct.pack('<I',TYPES));mu.mem_write(0x59a918,struct.pack('<i',zoom));mu.mem_write(0x542414,struct.pack('<I',extra));mu.mem_write(0x531b08+owner*4,struct.pack('<I',color))
        context=struct.pack('<5I',0x12345678,11,22,33,44);mu.mem_write(0x5b5ddc,context);mu.mem_write(BUFFER,b'\xa5'*0x10000);mu.mem_write(OUT,bytes(16));before=bytes(mu.mem_read(POOL+2500,50))
        self.call(0x4a7db0,0,struct.pack('<5I',50,OUT,OUT+4,OUT+8,OUT+12),0,precision,result=BUFFER);self.returns+=1
        metadata=bytes(mu.mem_read(OUT,16));width,height=struct.unpack('<2i',metadata[:8]);assert self.allocations==[width*height]
        assert bytes(mu.mem_read(0x5b5ddc,20))==context and bytes(mu.mem_read(POOL+2500,50))==before
        return metadata.hex(),bytes(mu.mem_read(BUFFER,width*height)).hex()


def generate(oracle,precision,smoke=False):
    """타입/다리 특례와 후보 DWORD/실제 여러 소유자 캡처를 교차한다."""
    rows=[];candidates=(oracle.candidates,struct.pack('<72I',*range(72)),struct.pack('<72I',*(0xff00+(index*37)%256 for index in range(72))))
    # 같은 타입과 다른 다리 타입의 특례를 각각 방문한다.
    for typ,bridge,colors in itertools.product((70,82,144),(82,144),candidates):rows.append(('M',str(typ),str(bridge),colors.hex(),oracle.maps(typ,bridge,colors,precision).hex()))
    # 원래 보호 그림의 일반/빈/방향 프레임과 owner0 우회·owner8·색0 항등을 포함한다.
    for typ in ((144,) if smoke else (70,73,128,144)):
        count=len(read_clusters(oracle.texts[TYPE_LOAD_ORDER[typ-70].lower()]));frames=sorted({0,1,min(7,count-1),count-1})
        # 캡처 zoom은 메타데이터만 바꾸며 RLE 자체의 확대/축소가 아니다.
        for frame,(owner,color),zoom,extra in itertools.product(frames,((0,8),(1,0),(1,1),(8,2),(8,8)),(32768,65536,98304),(0,1)):
            output=oracle.capture(typ,frame,owner,color,zoom,extra,precision);rows.append(('C',str(typ),str(frame),str(owner),str(color),str(zoom),str(extra),*output))
            if smoke:return rows
    return rows


def main():
    """원문 기대값을 새 파일로 보존하거나 SHA/고유 입력만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 과거 기대값을 다시 만들지 않고 전체 파일 바이트를 확인한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:-1] if row[0]=='M' else row[:-2]) for row in rows})==len(rows)
        print('소유자 색표/캡처 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];outputs=[generate(oracle,precision,args.smoke) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1]
    if args.smoke:print('소유자 색표/실제 mapped 캡처 smoke 정상 반환');return
    rows=outputs[0];FIXTURE.write_text('# M type bridge candidates72DWORD maps2304 / C type frame owner color zoom extra metadata16 pixels\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originals/d/_shapes.shp','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/shp.py','tools/decomp_owner_capture.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(rows),inputsByKind=dict(Counter(row[0] for row in rows)),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=[],nativeCalls=dict(oracles[0].calls),sha256={path:digest(ROOT/path) for path in paths},limitations=['실제9개 색표/색 선택/보호SHP/capture/plain·mapped RLE를 실행한다. new만 격리 메모리 경계다.','현재 source/TYPE/색 번호/zoom/추가 기준점은 입력이다. 추가 가상 그림(extra bit2)/전체 초기화/캠페인 효과 수명은 후속이다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('소유자 색표/캡처:',len(rows),'고유 입력, 두 정밀도 차이0')


if __name__=='__main__':main()
