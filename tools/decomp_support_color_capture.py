#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""작은 섬94와 하단95의 아홉 프레임·소유자 색을 보호 RLE 전체로 독립 동결한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_temple_capture import Oracle as CaptureOracle
from decomp_factory_capture import ROOT,POOL,BUFFER,OUT,digest
from decomp_gumpbackground_oracle import palette_inputs
from shp import load_type_texts,read_clusters
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/support-color-capture-x86.tsv'  # 기존 원문은 수정하지 않는 새 픽셀 기대다.
REPORT=ROOT/'cpppj/support-color-capture-native.json'  # 실제 함수/입력 색표/출력의 계보다.
MAPS=0x17800000  # 기존 캡처 코드·픽셀 버퍼와 겹치지 않는 독립 색표 입력이다.
KINDS=((94,'island',0x506d98),(95,'islandStalag',0x506e50))  # 실제 두 생성자의 가상 표다.
OWNERS=((0,0),(1,0),*((1,color) for color in range(1,9)),(8,2))  # 소유자와 색 번호가 다른 경우도 포함한다.


class Oracle(CaptureOracle):
    """원본 캡처/일반·색표 RLE를 실행하며 픽셀 힙만 공급한다."""
    def __init__(self):
        """독립 HSL fixture의 실제 팔레트 행을 입력 색표로 사용한다."""
        super().__init__()
        self.mu.mem_map(MAPS,0x10000)
        self.supportCalls={}  # 실제 기본/추가/색표/일반 그림 진입 횟수다.
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 색표 복사의 실제 CRT 몸체만 허용하며 인접 함수는 포함하지 않는다.
        for part in facts[0x402796]['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'))
            self.allowed.append((low,high+1))
        rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/owner-hsl-colors-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
        row=next(row for row in rows if row[:2]==['0','94'])
        assert row[2]==struct.pack('<256I',*palette_inputs()[0]).hex() and row[3]==row[4]
        self.ownerMaps=bytes.fromhex(row[3])  # C++ 색표를 읽지 않는 기존 독립 원문2304바이트다.
        texts=load_type_texts(str(ROOT/'originals'))
        # 실제 타입별 가상 표와 아홉 본체 프레임을 확인한다.
        for number,name,table in KINDS:
            assert self.read(table+0x78)==0x4acfe0 and self.read(table+0xa4)==0x41c100
            self.clusters[name]=read_clusters(texts[name.lower()])
            assert len(self.clusters[name])==9

    def on_instruction(self,mu,address,size,data):
        """가상 그림 미사용과 실제 RLE 분기를 대체 없이 기록한다."""
        if self.captureActive and address in (0x4acfe0,0x41c100,0x4daab0,0x4daa70):
            self.supportCalls[hex(address)]=self.supportCalls.get(hex(address),0)+1
        return super().on_instruction(mu,address,size,data)

    def capture(self,number,name,table,frame,owner,color,precision):
        """원본43bc40의 등록 결과를 명시 입력하고 전체 픽셀과 문맥 복원을 읽는다."""
        self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0))
        self.flat_segments()
        self.input_shape(name,number)
        self.mu.mem_write(MAPS,self.ownerMaps)
        self.mu.mem_write(0x557940+number*4,struct.pack('<I',MAPS if number==94 else 0))
        self.mu.mem_write(0x531b08+owner*4,struct.pack('<I',color))
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,table)
        raw[10]=number
        raw[34]=owner
        raw[40]=128  # 보존된 실제 받침처럼 extra2 가상 그림 분기를 사용하지 않는다.
        struct.pack_into('<I',raw,36,frame)
        self.mu.mem_write(POOL+50*50,bytes(raw))
        self.mu.mem_write(self.c['scale'],struct.pack('<I',65536))
        self.mu.mem_write(self.c['extra'],struct.pack('<I',1))
        self.mu.mem_write(OUT,bytes(16))
        self.mu.mem_write(BUFFER,b'\x5a'*0x100000)
        self.heap=[]
        context=bytes(self.mu.mem_read(self.c['context'],20))
        self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x100000),(OUT,OUT+16),(0x52eac0,0x52ebc0)])
        self.mu.reg_write(UC_X86_REG_FPCW,precision)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.captureActive=True
        self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12])
        self.captureActive=False
        metadata=struct.unpack('<4i',self.mu.mem_read(OUT,16))
        pixels=bytes(self.mu.mem_read(BUFFER,metadata[0]*metadata[1]))
        assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[len(pixels)] and any(value!=255 for value in pixels)
        assert bytes(self.mu.mem_read(self.c['context'],20))==context and bytes(self.mu.mem_read(POOL+50*50,50))==raw
        return ','.join(map(str,metadata)),pixels.hex()


def main():
    """제품을 읽지 않고 새 원문을 동결하거나 기존 자료의 해시를 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 관측 기대는 재계산 결과로 덮어쓰지 않는다.
        for name,value in record['sha256'].items():
            assert digest(ROOT/name)==value,name
        assert record['inputs']==198 and record['normalReturns']==396
        print('작은 섬198입력/396정상 반환 전체 픽셀 출처 통과')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle=Oracle()
    rows=[]
    pixels=0
    # 두 실제 타입·아홉 프레임·소유자/색 입력의 전체 교차다.
    for (number,name,table),frame,(owner,color) in itertools.product(KINDS,range(9),OWNERS):
        first=oracle.capture(number,name,table,frame,owner,color,0x27f)
        second=oracle.capture(number,name,table,frame,owner,color,0x37f)
        assert first==second
        rows.append([number,frame,owner,color,*first])
        pixels+=2*len(bytes.fromhex(first[1]))
    contents='# 실제94/95의9프레임 전체 캡처. TYPE(-2,3)/일반색/단위배율/독립 HSL 색표 입력.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'  # 최초 유효 원문과 전체 바이트가 같아야 한다.
    assert contents==(ROOT/'extracted/goal-support-color-20261019-native-good-v3.tsv').read_text('utf-8')
    FIXTURE.write_text(contents,'utf-8',newline='\n')
    paths={Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',ROOT/'originals/d/GIFCLOUD.COL',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'cpppj/tests/fixtures/owner-hsl-colors-x86.tsv'}
    paths.update(path for path in oracle.exports if path.name=='functions.tsv')
    paths.update(ROOT/'tools'/name for name in ('decomp_temple_capture.py','decomp_factory_capture.py','decomp_shape_to_buffer_oracle.py','decomp_dudespawn_oracle.py','decomp_gumpbackground_oracle.py','shp.py'))
    record=dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(rows),normalReturns=2*len(rows),pixels=pixels,nativeCalls=oracle.supportCalls,
        scope='실제 캡처/일반그림/보호 RLE 전체. 실제 받침 extra128은 가상+78/+a4를 호출하지 않는다. 43bc40 등록 결과와 기존 독립 HSL94 색표는 명시 입력이며 등록 부모 재실행은 아니다. TYPE(-2,3)/단위배율/일반색 전용이다.',
        sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps({key:record[key] for key in ('inputs','normalReturns','pixels','nativeCalls')},ensure_ascii=False))


if __name__=='__main__':
    main()
