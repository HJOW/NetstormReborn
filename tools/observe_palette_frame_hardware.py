#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 팔레트 Frame의 실제 CPU 결과와 도달 불가 진단을 관측한다."""
import argparse
import csv
import hashlib
import json
import math
import struct
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 원본과 독립 관측기를 찾는 저장소 기준이다.


def bits(value):
    """현재/기존 시각의 binary64 입력을 정규화하지 않고 전달한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def inputs():
    """시간/색/step/checking 입력만 만들며 결과는 계산하지 않는다."""
    samples=[(1/128,0),(1/128,1/128),(1/128,math.nextafter(1/128,math.inf)),(1/128,0.125),(1/128,0.5),(1/128,0.51),(1/128,-1),(1/128,float('nan')),(1/128,float('inf')),(0,0.25),(-0.5,0.25),(float('nan'),0.25),(float('inf'),0.25)]
    rows=[]
    for interval,now in samples: # 양수/0/음수/비순서 간격과 정확한 경계를 구별한다.
        for step in (-2147483648,-1,0,1,63,64,65): # signed 극단/정상/범위 밖 진단을 입력한다.
            for start,count,pattern in ((228,18,0),(255,1,1),(0,0,0)): # 일반/끝색/빈 팔레트의 실제 범위다.
                for checking in (0,0x80000000,0xffffffff): # 전체 현재 DWORD를 보존하고 boolean으로 바꾸지 않는다.
                    for mode in (0,1): # 화면 bit0의 표시/복구 관문을 구별한다.
                        rows.append([bits(interval),bits(0),bits(now),bits(0),step,checking,mode,start,count,pattern,1])
    for step in (0,1,64,65): # 전체256색의 RGB/예약 바이트와 종료를 포함한다.
        for checking in (0,0xffffffff): # 동일 입력의 checking만 바꾼다.
            for mode in (0,1): # 실제 화면 없이 요청 경계만 기록한다.
                rows.append([bits(1/128),bits(0),bits(0.125),bits(0),step,checking,mode,0,256,1,1])
    for now in (0,1.5): # INT_MAX는 최대 한 번만 진행하는 유한 입력으로 제한한다.
        for checking in (0,0xffffffff): # 도달 불가 진단의 양쪽 입력을 포함한다.
            for mode in (0,1): # 범위 밖 step의 정상 종료 요청을 관측한다.
                rows.append([bits(1),bits(0),bits(now),bits(0),2147483647,checking,mode,228,18,0,1])
    for steps in (2,7): # 같은 last/색/step을 다음 실제 프레임으로 이어간다.
        for checking in (0,0xffffffff): # 재시드/진단 정규화를 하지 않는다.
            for mode in (0,1): # 정지와 별개인 프레임 표시/복구 입력이다.
                rows.append([bits(1/128),bits(3.25),bits(3.251),bits(0.127),64,checking,mode,228,18,1,steps])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """최초 관측은 새 파일만 만들고 재관측은 기존 정확한 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-824/palette-frame-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/palette-frame-hardware-x86.tsv';report=ROOT/'cpppj/palette-frame-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();text=''.join(' '.join(row)+'\n' for row in matrix);binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=text,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'팔레트 실제CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for source,row in zip(matrix,rows): # 입력/폭만 확인하며 색/시간/step 출력을 모델로 만들지 않는다.
        assert len(row)==22 and row[:12]==source
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'팔레트 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / 표시·Kill·진단 요청 관측','binarySha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_palette_frame_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'유효 Init 뒤 Frame/순수 장치·0x027f/53비트·INT_MAX는최대1진행·CD 미초기화 stack Frame은 별도 미복원/문서화'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum(row[20]!='-' for row in rows)}))


if __name__=='__main__':main()
