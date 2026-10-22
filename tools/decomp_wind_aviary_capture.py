#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""데빌메이커의 공통 캡처/+78/빈+a4/보호RLE를 실제 전체 픽셀로 관측한다."""
import argparse
import itertools
import json
import socket
import struct
import decomp_generator_capture as source
from decomp_generator_capture import Oracle as BaseOracle
from decomp_factory_capture import ROOT,digest
from shp import load_type_texts,read_clusters

FIXTURE=ROOT/'cpppj/tests/fixtures/wind-aviary-capture-x86.tsv'  # 기존 캡처 fixture는 변경하지 않는다.
REPORT=ROOT/'cpppj/wind-aviary-capture-20261023.json'  # 실제 실행/픽셀/호스트/바이트 근거다.


class Oracle(BaseOracle):
    """기존 공통 캡처 전체 실행기에 실제139의 TYPE/SHP/VT만 입력한다."""
    def __init__(self):
        """원래 타입 로드 순서의69번째 SHP 블록까지 원본 헤더를 읽는다."""
        super().__init__();self.clusters['windAviary']=read_clusters(load_type_texts(str(ROOT/'originals'))['windaviary']);assert len(self.clusters['windAviary'])>=6
        assert [self.read(0x509aa0+off) for off in (0x78,0xa0,0xa4)]==[0x4acfe0,0x41c0f0,0x41c100]
        data=self.shapeFile.read_bytes()
        while len(self.blocks)<=139-70:  # 블록 수와 오프셋을 기존 파일 헤더에서 차례로 얻는다.
            previous=self.blocks[-1];self.blocks.append(previous+8+struct.unpack_from('<I',data,previous+4)[0]*8)


def main():
    """원본54입력을 동결하고 이후에는 SHA와 전체 픽셀 길이만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 제품 결과로 기대값을 생성하지 않는다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['rows']==54
        for row in rows:assert len(bytes.fromhex(row[4]))==int(row[5]) and row[6]=='1'  # 원문 전체 버퍼 길이를 확인한다.
        print('Wind 캡처 SHA:',len(rows));return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 동결 원문을 덮어쓰지 않는다';oracle=Oracle();source.SPECS={'windAviary':139};source.TABLE=0x509aa0;rows=[]
    for frame,extra,owner in itertools.product(range(6),(0,2,3),(0,1,8)):  # 여섯 생산 프레임과 추상/추가 플래그·현재 owner다.
        result=oracle.capture('windAviary',frame,0,extra,owner);rows.append([frame,extra,owner,*result])
    FIXTURE.write_text('# TYPE139/VT509aa0/TYPE발기준(-2,3)/base=-1/단위배율/일반색. 실제 캡처/+78/빈+a4/보호RLE, 픽셀힙만 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={'originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv','tools/decomp_wind_aviary_capture.py','tools/decomp_generator_capture.py','tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/shp.py',FIXTURE.relative_to(ROOT).as_posix()};paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',rows=len(rows),pixels=oracle.totalPixels,actualCalls=oracle.actual,assertions=oracle.assertions,captureReplaced=0,rleReplaced=0,osCalls=0,boundaries='실제 캡처/기본프레임/빈추가/보호RLE. 명시 TYPE 기준점(-2,3)/base=-1/일반색/단위배율이며 전체 자연 플레이 증거와 구별한다.',sha256={p:digest(ROOT/p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Wind 캡처:',len(rows),oracle.totalPixels)


if __name__=='__main__':main()
