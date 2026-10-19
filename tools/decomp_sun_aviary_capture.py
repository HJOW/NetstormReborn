#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 훨리베이스72의 생산 기본/추가 가상 그림과 보호RLE 전체 캡처를 동결한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path

from decomp_temple_capture import Oracle as CaptureOracle
from decomp_factory_capture import ROOT, POOL, BUFFER, OUT, digest
from shp import load_type_texts, read_clusters
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_FPCW, UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/sun-aviary-capture-x86.tsv'  # 기존 홈 통지/캡처 fixture와 구별하는 새 기대값이다.
REPORT=ROOT/'cpppj/sun-aviary-capture-native.json'  # 전체 정상 반환과 실제 가상 함수 도달을 기록한다.
TABLE=0x509930  # 실제72 생성자와 기존 홈 통지가 공유하는10.78 가상 표다.


class Oracle(CaptureOracle):
    """원본 두 짧은 몸체를 정확한 보호 바이트 범위로 실행하며 RLE는 대체하지 않는다."""
    def __init__(self):
        """기존 같은PC 실행기와 보호SHP의 원래72블록을 연결한다."""
        super().__init__()
        bodies=((0x45b530,'f641280975258b492483f90bb8ffffffff7c0883f9137f038d41f783f9147c0e83f91a7f09b80a000000c38b4124c3'),
                (0x45b560,'f641280975138b4424088b5424046aff6a005052e807bd0300c20800'))  # Capstone으로 모든 분기/ret을 확인한 실제 몸체다.
        # 정밀표에서 독립함수로 빠진 짧은 몸체만 허용하며 주변 코드는 포함하지 않는다.
        for entry,hexbytes in bodies:
            expected=bytes.fromhex(hexbytes)
            assert bytes(self.mu.mem_read(entry,len(expected)))==expected
            self.allowed.append((entry,entry+len(expected)))
        assert self.read(TABLE+0x78)==0x45b530 and self.read(TABLE+0xa4)==0x45b560
        self.clusters['sunAviary']=read_clusters(load_type_texts(str(ROOT/'originals'))['sunaviary'])
        assert len(self.clusters['sunAviary'])==32 and len(self.blocks)>72-70
        self.aviaryCalls={}

    def on_instruction(self,mu,address,size,data):
        """원본 getter/추가 Draw의 진입만 기록하고 계산·그림 제출은 유지한다."""
        if self.captureActive and address in (0x45b530,0x45b560):
            self.aviaryCalls[hex(address)]=self.aviaryCalls.get(hex(address),0)+1
        return super().on_instruction(mu,address,size,data)

    def capture(self,frame,extra,owner,precision):
        """실제32프레임과일반색표0·명시TYPE기준점(-2,3)에서전체버퍼를읽는다."""
        self.captureActive=False
        self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0))
        self.flat_segments()
        self.input_shape('sunAviary',72)
        raw=bytearray(50)
        struct.pack_into('<I',raw,0,TABLE)
        raw[10]=72
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
    """새 원본 기대값을 한 번 동결하거나 이미 기록한 파일만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 과거 관측값은 다시 생성하지 않고 현재 근거 바이트와 비교한다.
        for name,value in report['sha256'].items():
            assert digest(ROOT/name)==value,name
        assert report['inputs']==384 and report['normalReturns']==768
        print('훨리베이스384입력/768정상반환 캡처 출처 통과')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    oracle=Oracle()
    rows=[]
    pixels=0
    # 기본/생산/도움말32프레임과 추가/매몰/그림자 관문을 모든 입력으로 교차한다.
    for frame,extra,owner in itertools.product(range(32),(0,2,3,10),(0,1,8)):
        a=oracle.capture(frame,extra,owner,0x27f)
        b=oracle.capture(frame,extra,owner,0x37f)
        assert a==b
        rows.append([frame,extra,owner,*a,*b])
        pixels+=len(bytes.fromhex(a[1]))+len(bytes.fromhex(b[1]))
    content='# 실제72/VT509930의32프레임 전체 캡처. TYPE기준점(-2,3)/색표0/단위배율/픽셀heap경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    FIXTURE.write_text(content,'utf-8',newline='\n')
    paths={Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',ROOT/'extracted/refined/originals/functions.tsv'}
    paths.update(path for path in oracle.exports if path.name=='functions.tsv')
    paths.update(ROOT/'tools'/name for name in ('decomp_temple_capture.py','decomp_factory_capture.py','decomp_shape_to_buffer_oracle.py','decomp_dudespawn_oracle.py','shp.py'))
    record=dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(rows),normalReturns=len(rows)*2,pixels=pixels,
                virtualCalls=oracle.aviaryCalls,scope='실제4a7db0/+78=45b530/+a4=45b560/497280/보호RLE 전체. heap만선언경계. CD·다른배율·추상색·자연전투완주증거는아님.',
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps({key:record[key] for key in ('inputs','normalReturns','pixels','virtualCalls')},ensure_ascii=False))


if __name__=='__main__':
    main()
