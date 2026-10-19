#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""간헐천122의 실제 기본 외형/현재 애니메이션과 보호 RLE 전체를 독립 동결한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_temple_capture import Oracle as CaptureOracle
from decomp_factory_capture import ROOT, POOL, BUFFER, OUT, digest
from shp import load_type_texts, read_clusters
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_FPCW, UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/geyser-capture-x86.tsv'  # 기존 원문을 고치지 않는 새 전체 픽셀 기대다.
REPORT=ROOT/'cpppj/geyser-capture-native.json'  # 현재 PC/원본 함수/픽셀의 출처다.
TABLE=0x50da88  # 실제4857a0 생성자가 설치하는 간헐천 가상 표다.


class Oracle(CaptureOracle):
    """원본4a7db0/4acfe0/485480/497280과 보호 간헐천 SHP를 실행한다."""
    def __init__(self):
        """현재 정밀 함수 범위와 실제 가상 함수 주소를 검사한다."""
        super().__init__()
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 실제 추가 Draw와 TYPE 조회의 불연속 몸체만 추가한다.
        for entry in (0x485480,0x4ac550):
            # 인접 함수를 허용 범위에 섞지 않는다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        assert self.read(TABLE+0x78)==0x4acfe0 and self.read(TABLE+0xa4)==0x485480
        self.clusters['geyser']=read_clusters(load_type_texts(str(ROOT/'originals'))['geyser'])
        assert len(self.clusters['geyser'])==51
        self.geyser_calls={}  # 실제 두 가상 몸체의 진입 횟수다.

    def on_instruction(self,mu,address,size,data):
        """가상 그림을 대체하지 않고 실제 진입만 센다."""
        if self.captureActive and address in (0x4acfe0,0x485480):
            self.geyser_calls[hex(address)]=self.geyser_calls.get(hex(address),0)+1
        return super().on_instruction(mu,address,size,data)

    def capture(self,frame,extra,owner,precision):
        """51프레임의 명시 일반색/단위배율/TYPE(-2,3)에서 메타와 모든 픽셀을 읽는다."""
        self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0))
        self.flat_segments()
        self.input_shape('geyser',122)
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,TABLE)
        raw[10]=122
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
    """C++ 결과를 읽지 않고 새 원문을 동결하거나 기존 출처만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 기존 관측을 새 구현 결과로 덮어쓰지 않는다.
        for name,value in record['sha256'].items():
            assert digest(ROOT/name)==value,name
        assert record['inputs']==459 and record['normalReturns']==918
        print('간헐천459입력/918정상 반환 전체 캡처 출처 통과')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle=Oracle()
    rows=[]
    pixels=0
    # 전체51프레임과 추가 그림/일반 상태·owner 입력을 교차한다.
    for frame,extra,owner in itertools.product(range(51),(0,2,3),(0,1,8)):
        first=oracle.capture(frame,extra,owner,0x27f)
        second=oracle.capture(frame,extra,owner,0x37f)
        assert first==second
        rows.append([frame,extra,owner,*first,*second])
        pixels+=len(bytes.fromhex(first[1]))+len(bytes.fromhex(second[1]))
    FIXTURE.write_text('# 실제122/VT50da88의51프레임 전체 캡처. TYPE(-2,3)/일반색/단위배율/픽셀heap경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8',newline='\n')
    paths={Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',ROOT/'extracted/refined/originals/functions.tsv'}
    paths.update(path for path in oracle.exports if path.name=='functions.tsv')
    paths.update(ROOT/'tools'/name for name in ('decomp_temple_capture.py','decomp_factory_capture.py','decomp_shape_to_buffer_oracle.py','decomp_dudespawn_oracle.py','shp.py'))
    record=dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(rows),normalReturns=len(rows)*2,pixels=pixels,
        virtualCalls=oracle.geyser_calls,scope='실제 캡처/+78=4acfe0/+a4=485480/497280/보호RLE 전체. heap만 경계다. CD/다른배율/추상색/자연완주 증거는 아니다.',
        sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps({key:record[key] for key in ('inputs','normalReturns','pixels','virtualCalls')},ensure_ascii=False))


if __name__=='__main__':
    main()
