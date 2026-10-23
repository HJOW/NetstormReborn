#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""동일 실제 ColorSort를 유지한 채 화면 RGB만 두번 변경한 43c010의 전체9표를 제품 전에 고정한다."""
import argparse
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_gumpbackground_oracle import BackgroundOracle, ROOT, CONTROLS, palette_inputs, LOGICAL, SORT, SPEC

FIXTURE = ROOT/'cpppj/tests/fixtures/colorsort-fixed-input-x86.tsv'  # 기존3표 자료를 덮지 않는 최초 전체9표 수명 관측이다.
REPORT = ROOT/'cpppj/colorsort-fixed-input-native.json'  # 기존 원문 본체와 명시 입력의 경계를 기록한다.


def digest(path):
    """원시 바이트 SHA256만 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def generate():
    """같은 원본 객체의 재생성 없이 화면 입력·현재 빨강만 변경하며 출력은 PE가 결정한다."""
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('고정 ColorSort 최초 자료 덮어쓰기 금지')
    palettes = palette_inputs()  # 기존 실제 GIFCLOUD/혼합/중복회색 입력을 그대로 재사용한다.
    lines, instructions = [], 0  # 두 정밀도와 세 호출 단계의 실제 관측이다.
    files = set()  # 사용한 이전 정밀 본체와 입력도 고정한다.
    for control in CONTROLS:
        oracle = BackgroundOracle('originals')  # 매 정밀도는 독립 실제 입력에서 시작한다.
        oracle.maps(palettes[0],control)  # 실제439de0을 딱 한번 실행해 초기 RGB/HLS를 저장한다.
        source = bytes(oracle.mu.mem_read(SORT,0x3800))  # 실제 객체 전체의 같은 주소·내용을 이후에도 유지한다.
        initial = struct.pack('<256I',*palettes[0])  # 화면 입력과 객체 내부 복사를 구별한다.
        for stage,(logical,red) in enumerate(zip(palettes,(189,13,255))):
            current = struct.pack('<256I',*logical)  # 객체를 다시 생성하지 않고 화면 논리RGB만 교체한다.
            oracle.mu.mem_write(LOGICAL,current)
            oracle.mu.mem_write(0x5b5e58,struct.pack('<I',red))
            oracle.call(0x43c010,b'',control)  # 같은00555d88로 전체 maps 본체를 직접 재호출한다.
            instructions += oracle.instructions
            if bytes(oracle.mu.mem_read(SORT,len(source))) != source:
                raise RuntimeError('ColorSort 고정 객체 변경')
            if bytes(oracle.mu.mem_read(LOGICAL,len(current))) != current:
                raise RuntimeError('현재 화면 입력 변경')
            outputs = ':'.join(bytes(oracle.mu.mem_read(address,256)).hex() for address in SPEC['originals']['outputs'])  # 실제9개의256BYTE를 모두 보존한다.
            lines.append('\t'.join((str(control),str(stage),str(red),initial.hex(),current.hex(),hashlib.sha256(source).hexdigest(),outputs)))
            print('원문 고정 ColorSort',control,stage,flush=True)
        files.update(oracle.exports)
    map_differences = sum(left.split('\t')[6] != right.split('\t')[6] for left,right in zip(lines[:3],lines[3:]))  # HLS64비트별도출력을53비트기대에섞지않는다.
    sort_differences = sum(left.split('\t')[5] != right.split('\t')[5] for left,right in zip(lines[:3],lines[3:]))  # RGB복사는고정이어도생성HLS정밀도는달라질수있다.
    FIXTURE.write_text('# control / stage / currentRed / initialLogical1024 / currentScreenLogical1024 / unchangedNativeSortHash / maps9x256\n'+'\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    files.update((Path(__file__).resolve(),ROOT/'tools/decomp_gumpbackground_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_gumpvisual_oracle.py',ROOT/'originals/Netstorm.exe',ROOT/'originals/d/GIFCLOUD.COL',FIXTURE))
    report = {'status':'단위385후속·제품전최초고정', 'host':socket.gethostname(), 'lastDecompileHost':'HJOW-Athlon / 2026-10-23',
              'observations':len(lines), 'stagesPerControl':3, 'controls':list(CONTROLS), 'mapsInstructions':instructions,
              'sortAddress':SORT,'screenInputAddress':LOGICAL,'mapPrecisionDifferentStages':map_differences,'sortPrecisionDifferentStages':sort_differences,'unchangedSortBytes':0x3800,
              'scope':'기존실제439de0 한번→같은물리ColorSort에서현재화면RGB만2회변경→전체43c010의9표.43b430/파일캐시/OS/GUI 전체생성부모는범위밖.',
              'gameExecuted':False,'guiExecuted':False,'sha256':{path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def main():
    """최초 원문 관측 또는 변경 없는 SHA 감사만 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,value in report['sha256'].items():
            if digest(ROOT/name)!=value:
                raise RuntimeError('고정 ColorSort 최초 SHA 불일치 '+name)
        print('고정 ColorSort 최초보존',report['observations'])
    else:
        generate()


if __name__=='__main__':
    main()
