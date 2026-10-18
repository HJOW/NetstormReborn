#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 템플/71/128의 ShapeToBuffer→가상그림→보호SHP/RLE 전체 픽셀을 제품 연결 전에 관측한다."""
import argparse,csv,json,socket,struct
from pathlib import Path
from decomp_factory_capture import Oracle as FactoryOracle,BUFFER,OUT,ROOT,TYPES,POOL,FULL_SHAPE,digest
from decomp_sid_oracle import SidOracle
from decomp_temple_draw import SPECS as TEMPLES
from shp import load_type_texts,read_clusters
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
# 원본 전체SHP의 실제 블록 주소와 TYPE 코드 저장소를 구별한다.
CODES=0x17000000
SPECS={**TEMPLES,'sunArcher':(71,0x50b888,0x46a6e0),'sunCannon':(128,0x50b9b8,0x41c100)}
FIXTURE=ROOT/'cpppj/tests/fixtures/temple-capture-x86.tsv'
REPORT=ROOT/'cpppj/temple-capture.json'

class Oracle(FactoryOracle):
    """실제 캡처/가상함수/TYPE·헤더/RLE를 실행하며 픽셀힙과 소리 장치만 선언 경계다."""
    def __init__(self):
        """현재 정밀 표와 보호SHP의 원래 블록 순서/TYPE frame 코드를 공급한다."""
        super().__init__();self.mu.mem_map(CODES,0x10000);facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in (0x4dcf10,0x4dcd00,0x4dd0f0,0x46a6e0,0x41c100,0x497280,0x49a840,0x4ad710,0x49ace0):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        expected=bytes.fromhex('0fb6410a50e856d8feff8b802801000083c404c3');assert bytes(self.mu.mem_read(0x4acfe0,len(expected)))==expected;self.allowed.append((0x4acfe0,0x4acfe0+len(expected)))
        data=self.shapeFile.read_bytes();offset=0;self.blocks=[]
        for index in range(max(spec[0] for spec in SPECS.values())-70+1):
            self.blocks.append(offset);offset+=8+struct.unpack_from('<I',data,offset+4)[0]*8
        texts=load_type_texts(str(ROOT/'originals'));self.clusters={name:read_clusters(texts[name.lower()]) for name in (*SPECS,'sunDisc')};self.soundCalls=0;self.actual={};self.totalPixels=0
        for name,(number,table,draw) in SPECS.items():assert self.read(table+0x78)==0x4acfe0 and self.read(table+0xa4)==draw
    def on_instruction(self,mu,address,size,data):
        """실제 body 도달 수를 기록하고 소리 장치와 원래 크기 힙만 격리 반환한다."""
        if self.captureActive:
            if address==0x4a9cb0:
                esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<7I',mu.mem_read(esp,28));assert args[2:]==(0,0,0,0,0);self.soundCalls+=1;mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,args[0]);return
            if address in (0x4a7db0,0x4acfe0,0x4dcf10,0x4dcd00,0x4dd0f0,0x46a6e0,0x41c100,0x497280,0x4daa70):self.actual[hex(address)]=self.actual.get(hex(address),0)+1
        super().on_instruction(mu,address,size,data)
    def input_shape(self,name,number):
        """TYPE+128/방향표와 실제 원래SHP 블록만 현재 입력으로 공급한다."""
        typ=TYPES+number*self.creation['type_stride'];clusters=self.clusters[name];codes=''.join(item[0][0] for item in clusters);base=next((i for i,item in enumerate(clusters) if 'baseframe' in item[1].lower()),-1)
        self.input_type(number,0x10,0,0);self.mu.mem_write(typ+0xdc,struct.pack('<I',FULL_SHAPE+self.blocks[number-70]));self.mu.mem_write(typ+0x114,struct.pack('<I',len(clusters)));self.mu.mem_write(typ+0x124,struct.pack('<Ii',CODES+number*256,base));self.mu.mem_write(CODES+number*256,b''.join(bytes((ord(letter),0,0,0)) for letter in codes));self.mu.mem_write(typ+self.c['hot'],struct.pack('<ii',-2,3));self.mu.mem_write(0x557940+number*4,bytes(4))
        for i,letter in enumerate('ABCDEFGHIJKLMNOP'):
            self.mu.mem_write(typ+0x130+i*4,struct.pack('<i',codes.find(letter)));self.mu.mem_write(typ+0x170+i*4,struct.pack('<i',codes.count(letter)))
    def run_case(self,name,frame,word,extra,owner,precision=0x27f):
        """현재 raw와 명시 TYPE 기준점(-2,3)에서 메타데이터·모든 픽셀·문맥을 읽는다."""
        self.captureActive=False;self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments();number,table,_=SPECS[name]
        for n,typ in ((name,number),('sunDisc',83)):self.input_shape(n,typ)
        self.mu.mem_write(0x5411a4,struct.pack('<I',83));raw=bytearray(50);struct.pack_into('<I',raw,0,table);raw[10]=number;struct.pack_into('<H',raw,12,word);raw[34]=owner;raw[40]=extra;struct.pack_into('<i',raw,36,frame);self.mu.mem_write(POOL+50*50,bytes(raw));self.mu.mem_write(self.c['scale'],struct.pack('<I',65536));self.mu.mem_write(self.c['extra'],struct.pack('<I',1));self.mu.mem_write(OUT,bytes(16));self.mu.mem_write(BUFFER,b'\x5a'*0x100000);self.heap=[];self.soundCalls=0
        context=bytes(self.mu.mem_read(self.c['context'],20));self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x100000),(OUT,OUT+16)]);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0);self.captureActive=True;self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12]);self.captureActive=False
        metadata=struct.unpack('<4i',self.mu.mem_read(OUT,16));w,h=metadata[:2];assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[w*h] and bytes(self.mu.mem_read(self.c['context'],20))==context;pixels=bytes(self.mu.mem_read(BUFFER,w*h));self.totalPixels+=len(pixels);assert bytes(self.mu.mem_read(POOL+50*50,50))==raw
        return [','.join(map(str,metadata)),pixels.hex(),str(w*h),'1',str(self.soundCalls)]
def main():
    """현재 원문을 동결하거나 전체 해시/픽셀 길이만 읽기 전용 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['rows']
        for row in rows:assert len(bytes.fromhex(row[6]))==int(row[7]) and row[8]=='1'
        print('템플/발사기 전체 캡처',len(rows),'원문/해시 일치');return
    oracle=Oracle();inputs=[]
    for name in TEMPLES:inputs.extend((name,frame,word,2,1) for frame in range(len(oracle.clusters[name])) for word in (0,1))
    inputs.extend(('sunArcher',frame,word,extra,1) for frame in (0,1,17,33,34) for word in (0,1) for extra in (2,3));inputs.extend(('sunCannon',frame,word,extra,1) for frame in (0,8) for word in (0,1) for extra in (2,3))
    for name in SPECS:
        base=next((i for i,item in enumerate(oracle.clusters[name]) if 'baseframe' in item[1].lower()),0);inputs.extend((name,base,0,0,owner) for owner in (0,1,8))
        if name in TEMPLES:inputs.extend((name,base,word,3,1) for word in (0,1))
    rows=[]
    for values in inputs:
        try:result=oracle.run_case(*values)
        except Exception as error:raise RuntimeError(str(values)) from error
        rows.append([*values,*result])
    FIXTURE.write_text('# 실제10.78 ShapeToBuffer/가상Draw/보호SHP/전체RLE. TYPE 기준점(-2,3), 단위배율·일반색. heap/소리 장치만 선언 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8',newline='\n');paths={'tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/decomp_temple_draw.py','tools/shp.py','originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix()};paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
    REPORT.write_text(json.dumps(dict(unit=191,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),pixels=oracle.totalPixels,assertions=oracle.assertions,captureReplaced=0,virtualDrawReplaced=0,rleReplaced=0,osCalls=0,actualCalls=oracle.actual,boundaries='실제 캡처/가상Draw/보호RLE. heap/소리 장치만 선언 경계. TYPE 기준점(-2,3)/단위배율/일반색의 명시입력. 실제 게임 파괴/파편Frame/완주와 별도.',sha256={path:digest(ROOT/path) for path in sorted(paths)}),ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(len(rows),oracle.totalPixels,oracle.assertions)
if __name__=='__main__':main()
