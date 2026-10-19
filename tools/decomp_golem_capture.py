#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 골렘66프레임의 보호RLE와 TYPE 발 기준점을 캡처해 화면 회귀 검사의 기대값으로 보존한다."""
import argparse
import json
import socket
import struct
from decomp_generator_capture import Oracle as GeneratorOracle
from decomp_factory_capture import ROOT,TYPES,digest
from shp import load_type_texts,read_clusters
import decomp_generator_capture as generator

FIXTURE=ROOT/'cpppj/tests/fixtures/golem-capture-x86.tsv' # 제품 변경 전에 동결하는 원본 전체 픽셀이다.
REPORT=ROOT/'cpppj/golem-capture-native.json' # 현재 호스트의 원본 실행과 명시 입력 경계를 기록한다.


class Oracle(GeneratorOracle):
    """실제4a7db0 캡처와 보호SHP/RLE를 실행하며 픽셀힙만 선언된 격리 메모리로 공급한다."""
    def __init__(self):
        """실제 골렘 VT와 원래 블록 순서/TYPE의66프레임을 입력한다."""
        super().__init__()
        self.clusters['sunWalker']=read_clusters(load_type_texts(str(ROOT/'originals'))['sunwalker'])
        assert len(self.clusters['sunWalker'])==66
        assert self.read(0x50e568+0x78)==0x4acfe0 and self.read(0x50e568+0xa4)==0x41c100
        data=self.shapeFile.read_bytes()
        # 기존 캡처의 마지막 블록 뒤부터 실제133번째 타입의 헤더까지 순서대로 읽는다.
        while len(self.blocks)<=133-70:
            offset=self.blocks[-1]
            self.blocks.append(offset+8+struct.unpack_from('<I',data,offset+4)[0]*8)

    def input_shape(self,name,number):
        """원본 TYPE의 hotFootRatioX=.5/Y=.2에서 얻은 정수 발 기준점8,2를 공급한다."""
        super().input_shape(name,number)
        self.mu.mem_write(TYPES+number*self.creation['type_stride']+self.c['hot'],struct.pack('<ii',8,2))


def main():
    """최초 원문을 보존하고 이후 --verify는 해시/입력 수/전체 픽셀 길이만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 제품 회귀 실패를 기대값 재생성으로 덮지 않도록 동결 출처를 읽기 전용 대조한다.
        for path,wanted in report['sha256'].items():
            assert digest(ROOT/path)==wanted,path
        rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==report['rows']==66
        # 불투명 픽셀을 포함하는 정상 프레임과 원본 빈 프레임을 구분한다.
        for row in rows:
            assert len(bytes.fromhex(row[2]))==int(row[3]) and row[4]=='1'
        print('골렘66프레임 원본 캡처/해시 일치')
        return
    assert not FIXTURE.exists() and not REPORT.exists(),'동결 원문을 덮어쓰지 않는다'
    oracle=Oracle()
    generator.SPECS={'sunWalker':133}
    generator.TABLE=0x50e568
    rows=[]
    # 북/북동/동/남동/남/남서/서/북서 각8장과 원래 빈/도움말 프레임을 모두 관측한다.
    for frame in range(66):
        result=oracle.capture('sunWalker',frame,0,128,0)
        if frame<64:
            assert any(value!=255 for value in bytes.fromhex(result[1])),frame
        rows.append([frame,*result])
    FIXTURE.write_text('# 실제10.78 골렘133/VT50e568, 현재프레임/extra128/owner0/TYPE발기준8,2/단위배율. 실제 캡처와 보호RLE, 픽셀힙만 선언 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8',newline='\n')
    paths={'tools/decomp_golem_capture.py','tools/decomp_generator_capture.py','tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/shp.py','originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix()}
    missing=[] # 상속 도구가 열지 않는 옛 C 부록과 실제 사용한 TSV/원본 실행 코드를 구분한다.
    # 상속 도구의 현재 환경에서 존재하는 출처만 해시하며 없는 C 부록은 명시 목록으로 남긴다.
    for path in oracle.exports:
        name=path.relative_to(ROOT).as_posix()
        if path.exists():
            paths.add(name)
        else:
            assert name in {f'extracted/{part}/originals/creation.c' for part in ('burning-frame-helper','burning-frame-paint-helper','burning-frame-raster-helper')},name
            missing.append(name)
    report=dict(unit=227,host=socket.gethostname(),lastDecompileHost='VM-W11-Codex',lastDecompileDate='2026-10-19',rows=len(rows),pixels=oracle.totalPixels,actualCalls=oracle.actual,assertions=oracle.assertions,captureReplaced=0,rleReplaced=0,osCalls=0,
        boundaries='실제 캡처/보호RLE. 픽셀힙만 선언 경계. 원본TYPE 발 기준점8,2/owner0/extra128/단위배율의 명시 입력이며 실제 미션 보행 관측과 구별한다.',unusedMissingCAppendices=missing,sha256={path:digest(ROOT/path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print(len(rows),oracle.totalPixels,oracle.actual)


if __name__=='__main__':
    main()
