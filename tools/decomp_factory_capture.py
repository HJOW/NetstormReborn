#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Factory의 기본 프레임/추가 그림과 ShapeToBuffer의 전체 보호 픽셀을 격리 관측한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_shape_to_buffer_oracle import ShapeToBufferOracle, OUT
from decomp_dudespawn_oracle import FULL_SHAPE
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL, SidOracle
from decomp_oracle import ROOT
from decomp_owner_oracle import digest
from shp import load_type_texts, read_clusters
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

BUFFER=0x16000000 #큰 실제 워크샾 픽셀은 기존 작은 Dude 버퍼/스택과 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/factory-capture-x86.tsv' #C++ 구현 전에 동결하는 전체 원본 픽셀이다.
REPORT=ROOT/'cpppj/factory-capture.json' #검토한 원본 범위/보호 자산/도구 해시다.


class Oracle(ShapeToBufferOracle):
    """캡처와 Factory 가상 함수·실제 RLE를 실행하며 픽셀 heap만 명시 공급한다."""
    def __init__(self):
        """현재 PC 원문에서 실제 Factory/기본 Level/추가 그림 범위를 더한다."""
        super().__init__('originals');self.mu.mem_map(BUFFER,0x100000)
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        #가상+78/+a4와 실제 공통+80/hasLevel/TYPE getter/추가 그림의 정상 몸체다.
        for entry in (0x454cd0,0x454cf0,0x4ad0f0,0x4ad0a0,0x4ac550,0x44c180,0x4021b5):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.captureCalls={};self.block=0
        data=self.shapeFile.read_bytes()
        #원본 로딩 순서의15번째 sunFactory 블록 헤더를 실제 SHP에서 찾는다.
        for _ in range(15):self.block+=8+struct.unpack_from('<I',data,self.block+4)[0]*8
        clusters=read_clusters(load_type_texts(str(ROOT/'originals'))['sunfactory']);self.mainCount=len(clusters)
        self.first=[next((i for i,item in enumerate(clusters) if item[0][0]==letter),-1) for letter in 'ABCDEFGH']
        self.currentFrames=[0,self.mainCount-1]

    def on_instruction(self,mu,address,size,data):
        """원본 계산은 대체하지 않으며 실제 가상 호출 수와 heap 요청을 기록한다."""
        if self.captureActive:
            if address in (0x454cd0,0x454cf0,0x44c180):self.captureCalls[address]=self.captureCalls.get(address,0)+1
            if address==self.c['new']:
                esp=mu.reg_read(UC_X86_REG_ESP);count=self.read(esp+4);assert 0<=count<=0x100000
                self.heap.append(count);mu.reg_write(UC_X86_REG_EAX,BUFFER);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,self.read(esp));return
        SidOracle.on_instruction(self,mu,address,size,data)

    def run(self,values):
        """실제 보호 TYPE85/SHP와 현재 슬롯 입력만 준비해 전체 버퍼/문맥 복원을 읽는다."""
        table,level,extra,frame,hotspot,zoom=values;self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments()
        raw=bytearray(50);struct.pack_into('<I',raw,0,table);raw[10]=85;raw[11]=level<<5;raw[34]=1;raw[40]=extra;struct.pack_into('<i',raw,36,frame);self.mu.mem_write(POOL+50*50,bytes(raw))
        self.input_type(85,0x28008010,0x00800000,0);typ=TYPES+85*self.creation['type_stride']
        self.mu.mem_write(typ+0xdc,struct.pack('<I',FULL_SHAPE+self.block));self.mu.mem_write(typ+0x114,struct.pack('<I',self.mainCount))
        self.mu.mem_write(typ+0x130,struct.pack('<8i',*self.first));self.mu.mem_write(typ+self.c['hot'],struct.pack('<ii',-2,3));self.mu.mem_write(0x557940+85*4,bytes(4))
        self.mu.mem_write(self.c['scale'],struct.pack('<i',zoom));self.mu.mem_write(self.c['extra'],struct.pack('<I',hotspot));self.mu.mem_write(OUT,bytes(16));self.mu.mem_write(BUFFER,b'\x5a'*0x100000);self.heap=[]
        context=bytes(self.mu.mem_read(self.c['context'],20));self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x100000),(OUT,OUT+16)])
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPSW,0);self.captureActive=True
        self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12]);self.captureActive=False
        metadata=struct.unpack('<4i',self.mu.mem_read(OUT,16));w,h=metadata[:2]
        assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[w*h] and bytes(self.mu.mem_read(self.c['context'],20))==context
        return [','.join(map(str,metadata)),bytes(self.mu.mem_read(BUFFER,w*h)).hex(),str(w*h),'1']


def verify():
    """동결 관측의 전체 원문 해시와 고유 입력 수를 읽기 전용으로 확인한다."""
    report=json.loads(REPORT.read_text('utf-8'))
    #현재 핀과 이전 원문은 재관측으로 덮어쓰지 않는다.
    for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
    rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:]
    assert len(rows)==report['normalReturns']==200 and len({tuple(row[:6]) for row in rows})==200
    print('Factory capture:200 normal returns, full native RLE pixels/context restored')


def observe():
    """원본 가상 표/상태/abstract·추가 그림/현재 프레임·배율을 실제 캡처로 동결한다."""
    oracle=Oracle();rows=[]
    cases=list(itertools.product((0x509148,0x509210),range(8),(2,3,10),oracle.currentFrames,(0,1),(65536,)))
    cases.extend((0x509148,0,3,frame,hotspot,zoom) for frame,hotspot,zoom in itertools.product(oracle.currentFrames,(0,1),(32768,98304)))
    #C++ 결과 없이 실제 원본 반환과 전체 픽셀만 저장한다.
    for values in cases:rows.append([*values,*oracle.run(values)])
    with FIXTURE.open('w',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t',lineterminator='\n');writer.writerow(['vtable','level','extra','frame','hotspot','zoom','metadata','pixels','heap','contextRestored']);writer.writerows(rows)
    paths={'tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/decomp_dudespawn_oracle.py','originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix(),oracle.shapeFile.relative_to(ROOT).as_posix()}
    paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
    report=dict(edition='10.78',normalReturns=len(rows),scope='실제Factory+78/+a4/Level/ShapeToBuffer/보호sunFactory85/실제plain RLE. 픽셀heap만 명시 경계. mapped와 nonunit배율의 비abstract추가그림은 이 관측에 없다. 창/OS호출0.',mainCount=oracle.mainCount,first=oracle.first,actualCalls={hex(k):v for k,v in oracle.captureCalls.items()},sha256={path:digest(ROOT/path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


if __name__=='__main__':
    if '--verify' in sys.argv:verify()
    else:observe()
