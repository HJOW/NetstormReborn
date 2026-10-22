#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다섯 워크샾 자산의 전체 클러스터를 보호 캡처/가상 함수/RLE로 관측한다."""
import argparse
import itertools
import json
import socket
import struct
import decomp_generator_capture as source
from decomp_generator_capture import Oracle as BaseOracle
from decomp_factory_capture import ROOT,digest
from shp import load_type_texts,read_clusters

SPECS={'rainCannon':(129,0x50b9b8),'sunFence':(136,0x509470),'windArcher':(140,0x50b6e8),'rainAviary':(143,0x5099e8),'thunderArcher':(148,0x50b7c0)}  # 정확한 타입/VT 쌍이며128과129의 공유VT도 구별한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/plain-workshop-capture-x86.tsv'  # 기존 모든 캡처 fixture와 분리한다.
REPORT=ROOT/'cpppj/plain-workshop-capture-20261023.json'  # 현재 호스트/전체 픽셀/함수/메타데이터 근거다.


class Oracle(BaseOracle):
    """공통 캡처 전체 실행기에 다섯 실제 타입의 원래 SHP 블록을 공급한다."""
    def __init__(self):
        """원본 로드 순서와 기본/도움말 클러스터·빈 추가 그림을 모두 확인한다."""
        super().__init__();texts=load_type_texts(str(ROOT/'originals'));self.methods={}
        for name,(number,table) in SPECS.items():  # 각 원래 타입의 실제 클러스터와 가상 진입을 읽는다.
            self.clusters[name]=read_clusters(texts[name.lower()]);entries=[self.read(table+off) for off in (0x78,0xa0,0xa4)];assert entries==[0x4acfe0,0x41c0f0,0x41c100];self.methods[str(number)]={'name':name,'vtable':f'{table:08x}','frames':len(self.clusters[name]),'captureMethods':[f'{entry:08x}' for entry in entries]}
        data=self.shapeFile.read_bytes()
        while len(self.blocks)<=148-70:  # 보호 파일의 블록 헤더를 차례로 읽어 최대 타입까지 확장한다.
            previous=self.blocks[-1];self.blocks.append(previous+8+struct.unpack_from('<I',data,previous+4)[0]*8)


def main():
    """새 관측을 한 번 동결하고 이후에는 전체 SHA와 픽셀 길이만 읽기 전용 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 현재 제품으로 원문 기대값을 재생성하지 않는다.
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['rows']==2115 and len({row[0]+','+','.join(row[1:4]) for row in rows})==len(rows)
        for row in rows:assert len(bytes.fromhex(row[5]))==int(row[6]) and row[7]=='1'  # 메타데이터/버퍼의 전체 길이다.
        print('다섯 워크샾 캡처 SHA:',len(rows));return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 동결 원문을 덮어쓰지 않는다';oracle=Oracle();source.SPECS={name:values[0] for name,values in SPECS.items()};rows=[]
    for name,(number,table) in SPECS.items():  # 숫자와 설명이 아닌 정확한 가상 표를 현재 캡처에 입력한다.
        source.TABLE=table
        for frame,extra,owner in itertools.product(range(len(oracle.clusters[name])),(0,2,3,16,18),(0,1,8)):  # 모든 클러스터/추가·추상 상태/현재 owner다.
            result=oracle.capture(name,frame,0,extra,owner);rows.append([number,frame,extra,owner,*result])
    assert len(rows)==2115
    FIXTURE.write_text('# TYPE129/136/140/143/148/정확한VT/전체141클러스터/TYPE발기준(-2,3)/단위배율. 실제 캡처/+78/빈+a4/보호RLE, 픽셀힙만 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={'originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv','tools/decomp_plain_workshop_capture.py','tools/decomp_generator_capture.py','tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/shp.py',FIXTURE.relative_to(ROOT).as_posix()};paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',rows=len(rows),pixels=oracle.totalPixels,types=oracle.methods,actualCalls=oracle.actual,assertions=oracle.assertions,captureReplaced=0,rleReplaced=0,osCalls=0,boundaries='실제 캡처/가상 기본·빈 추가/보호RLE. 명시 TYPE 발 기준점(-2,3)/단위배율/일반색이며 전체 자연 플레이 증거와 구별한다.',sha256={p:digest(ROOT/p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다섯 워크샾 캡처:',len(rows),oracle.totalPixels)


if __name__=='__main__':main()
