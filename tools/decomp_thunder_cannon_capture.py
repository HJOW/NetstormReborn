#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 썬더 캐논131의 방향별 기본/추가 가상 그림과 보호RLE 전체 캡처를 동결한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path

from decomp_temple_capture import Oracle as CaptureOracle, CODES
from decomp_factory_capture import ROOT, TYPES, POOL, BUFFER, OUT, digest
from shp import load_type_texts, read_clusters
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_FPCW, UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/thunder-cannon-capture-x86.tsv'  # 제품 구현을 읽지 않는 새 원본 기대값이다.
REPORT=ROOT/'cpppj/thunder-cannon-capture-native.json'  # 전체 캡처와 실제 가상 함수 도달을 구분한다.
TABLE=0x50bad0  # 보호 생성자46ae50이 쓰는131의 실제 가상 표다.


class Oracle(CaptureOracle):
    """기존 캡처 실행기에46ae10/46ae30의 실제 몸체와131 보호 자산만 더한다."""
    def __init__(self):
        """같은 호스트 정밀 범위와 PE 바이트를 확인하며 기존 fixture는 변경하지 않는다."""
        super().__init__()
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 방향 getter와 타입 조회는 원래 불연속 함수 범위만 허용한다.
        for entry in (0x46ae10,0x4ad710,0x4ac550):
            # Ghidra의 포함 끝 주소를 열린 범위로 바꾸며 함수 사이를 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        expected=bytes.fromhex('8b4424088b5424046aff6a005052e83dc40200c20800')
        assert bytes(self.mu.mem_read(0x46ae30,len(expected)))==expected
        self.allowed.append((0x46ae30,0x46ae30+len(expected)))
        assert self.read(TABLE+0x78)==0x46ae10 and self.read(TABLE+0xa4)==0x46ae30
        self.clusters['thunderCannon']=read_clusters(load_type_texts(str(ROOT/'originals'))['thundercannon'])
        data=self.shapeFile.read_bytes()
        offset=0
        self.blocks=[]
        # 부모가128까지만 쓰던 보호SHP 블록 목록을 실제131까지 원래 순서로 읽는다.
        for index in range(131-70+1):
            self.blocks.append(offset)
            offset+=8+struct.unpack_from('<I',data,offset+4)[0]*8
        assert len(self.clusters['thunderCannon'])==37 and len(self.blocks)>131-70
        self.thunderCalls={}

    def on_instruction(self,mu,address,size,data):
        """원본 기본/추가 Draw 진입만 기록하고 계산·그림 제출·RLE는 대체하지 않는다."""
        if self.captureActive and address in (0x46ae10,0x46ae30):
            self.thunderCalls[hex(address)]=self.thunderCalls.get(hex(address),0)+1
        return super().on_instruction(mu,address,size,data)

    def capture(self,frame,extra,owner,precision):
        """실제37프레임·일반색표0·명시 기준점(-2,3)에서 메타와 전체 픽셀을 읽는다."""
        self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0))
        self.flat_segments()
        self.input_shape('thunderCannon',131)
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,TABLE)
        raw[10]=131
        struct.pack_into('<H',raw,12,0xa5ef)
        raw[34]=owner
        raw[40]=extra
        struct.pack_into('<I',raw,36,frame)
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
        pixels=bytes(self.mu.mem_read(BUFFER,metadata[0]*metadata[1]))
        assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[len(pixels)]
        assert bytes(self.mu.mem_read(self.c['context'],20))==context and bytes(self.mu.mem_read(POOL+50*50,50))==raw
        return ','.join(map(str,metadata)),pixels.hex()


def main():
    """고정 입력을 원본으로 먼저 동결하거나 기존 바이트만 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 기대값을 다시 생성하지 않고 원본·도구·범위·픽셀 바이트를 확인한다.
        for name,value in report['sha256'].items():
            assert digest(ROOT/name)==value,name
        assert report['inputs']==333 and report['normalReturns']==666
        print('썬더 캐논333입력/666정상반환 캡처 출처 통과')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle=Oracle()
    rows=[]
    pixels=0
    # 모든37물리프레임과추가그림관문/owner의 고정 교차 입력이다.
    for frame,extra,owner in itertools.product(range(37),(0,2,3),(0,1,8)):
        a=oracle.capture(frame,extra,owner,0x27f)
        b=oracle.capture(frame,extra,owner,0x37f)
        assert a==b
        rows.append([frame,extra,owner,*a,*b])
        pixels+=len(bytes.fromhex(a[1]))+len(bytes.fromhex(b[1]))
    content='# 실제131/VT50bad0의37프레임 전체 캡처. TYPE기준점(-2,3)/색표0/단위배율/픽셀heap경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    FIXTURE.write_text(content,'utf-8',newline='\n')
    paths={Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',ROOT/'extracted/refined/originals/functions.tsv'}
    paths.update(path for path in oracle.exports if path.name=='functions.tsv')
    paths.update(ROOT/'tools'/name for name in ('decomp_temple_capture.py','decomp_factory_capture.py','decomp_shape_to_buffer_oracle.py','decomp_dudespawn_oracle.py','shp.py'))
    record=dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(rows),normalReturns=len(rows)*2,pixels=pixels,
                virtualCalls=oracle.thunderCalls,scope='실제4a7db0/+78=46ae10/+a4=46ae30/497280/보호RLE 전체. heap만선언경계. CD·다른배율·자연전투완주증거는아님.',
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps({key:record[key] for key in ('inputs','normalReturns','pixels','virtualCalls')},ensure_ascii=False))


if __name__=='__main__':
    main()
