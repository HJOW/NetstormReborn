#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""섬 조각95/157의 실제 가상Draw·ShapeToBuffer·보호SHP 전체 픽셀을 관측한다."""
import argparse
import itertools
import json
import socket
import struct

from decomp_temple_capture import Oracle as CaptureOracle
from decomp_factory_capture import BUFFER, OUT, ROOT, POOL, digest
from shp import load_type_texts, read_clusters
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SPECS={'islandStalag':(95,0x506e50),'noIsland':(157,0x506f08)}  # 보호 로딩 순서와 실제 생성자 가상 표다.
FIXTURE=ROOT/'cpppj/tests/fixtures/support-capture-x86.tsv'  # 제품 결과를 사용하지 않는 원본 메타/전체 픽셀이다.
REPORT=ROOT/'cpppj/support-capture-native.json'  # 전체 정상 반환과 선언한 TYPE 입력의 범위를 기록한다.


class Oracle(CaptureOracle):
    """픽셀 heap 외에는 캡처·가상 getter/추가 Draw·RLE를 실제 PE로 실행한다."""
    def __init__(self):
        """같은 보호 SHP에서 두 실제 타입의9프레임을 추가한다."""
        super().__init__()
        texts=load_type_texts(str(ROOT/'originals'))
        # 가상 표의 동일 공통 기본 프레임/빈 추가 Draw를 실제 포인터로 확인한다.
        for name,(number,table) in SPECS.items():
            assert self.read(table+0x78)==0x4acfe0 and self.read(table+0xa4)==0x41c100
            self.clusters[name]=read_clusters(texts[name.lower()])
            assert len(self.clusters[name])==9
        data=self.shapeFile.read_bytes()
        offset=0
        self.blocks=[]
        #157번까지의 원래 블록 순서를 유지한다.
        for index in range(157-70+1):
            self.blocks.append(offset)
            offset+=8+struct.unpack_from('<I',data,offset+4)[0]*8

    def capture(self,name,frame,extra,owner,precision):
        """명시 TYPE 기준점(-2,3)·일반색표0·단위배율에서 전체 버퍼를 읽는다."""
        self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0))
        self.flat_segments()
        number,table=SPECS[name]
        self.input_shape(name,number)
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,table)
        raw[10]=number
        raw[34]=owner
        raw[40]=extra
        struct.pack_into('<i',raw,36,frame)
        self.mu.mem_write(POOL+50*50,bytes(raw))
        self.mu.mem_write(self.c['scale'],struct.pack('<I',65536))
        self.mu.mem_write(self.c['extra'],struct.pack('<I',1))
        self.mu.mem_write(OUT,bytes(16))
        self.mu.mem_write(BUFFER,b'\x5a'*0x100000)
        self.heap=[]
        self.soundCalls=0
        context=bytes(self.mu.mem_read(self.c['context'],20))
        self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x100000),(OUT,OUT+16)])
        self.mu.reg_write(UC_X86_REG_FPCW,precision)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.captureActive=True
        self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12])
        self.captureActive=False
        metadata=struct.unpack('<4i',self.mu.mem_read(OUT,16))
        width,height=metadata[:2]
        pixels=bytes(self.mu.mem_read(BUFFER,width*height))
        self.totalPixels+=len(pixels)
        assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[width*height]
        assert bytes(self.mu.mem_read(self.c['context'],20))==context and bytes(self.mu.mem_read(POOL+50*50,50))==raw
        assert self.soundCalls==0
        return ','.join(map(str,metadata)),pixels.hex()


def main():
    """새 독립 원문을 동결하거나 과거 바이트를 보존하며 재생한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--replay',action='store_true')
    args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 원문을 다시 쓰지 않고 전체 바이트만 비교한다.
        for name,expected in report['sha256'].items():
            assert digest(ROOT/name)==expected,name
        assert report['inputs']==162 and report['normalReturns']==324
        print('섬 조각 캡처 원문 출처 통과')
        return
    if not args.replay:
        assert not FIXTURE.exists() and not REPORT.exists()
    oracles=[Oracle(),Oracle()]
    rows=[]
    # 두 정밀도는 같은 입력에서 별개 실행하며 결과는 보호 PE에서만 읽는다.
    for name,frame,extra,owner in itertools.product(SPECS,range(9),(0,2,3),(0,1,8)):
        result=[o.capture(name,frame,extra,owner,precision) for o,precision in zip(oracles,(0x27f,0x37f))]
        assert result[0]==result[1]
        rows.append('\t'.join([name,str(frame),str(extra),str(owner),*result[0],*result[1]]))
    content='# name frame extra owner metadata53 pixels53 metadata64 pixels64; 명시TYPEhotspot(-2,3)\n'+'\n'.join(rows)+'\n'
    if args.replay:
        assert FIXTURE.read_bytes()==content.encode('utf-8')
        print('섬 조각 캡처 기존 원문 재생 일치',len(rows),len(rows)*2)
        return
    FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    names=['originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','tools/decomp_support_capture.py',
        'tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/shp.py',
        'extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',
        inputs=len(rows),normalReturns=len(rows)*2,pixels=sum(o.totalPixels for o in oracles),
        scope='10.78 실제4a7db0/+78=4acfe0/+a4=41c100/보호RLE 전체. 픽셀 heap만 선언 경계다. TYPEhotspot(-2,3)·색표0·단위배율 입력이며 CD/자연 낙하 전체의 증거로 확대하지 않는다.',
        sha256={name:digest(ROOT/name) for name in names}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('섬 조각 캡처 독립 원문',len(rows),len(rows)*2)


if __name__=='__main__':
    main()
