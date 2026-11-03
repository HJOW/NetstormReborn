#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 도착의 실제CPU 진단 복귀/항상참 범위 진단/삭제 뒤 정리를 관측한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 PE와 독립 관측기 기준이다.


def inputs():
    """기대 결과 없이 가상 반환/현재 헤더·범위/후보·변경 입력만 구성한다."""
    rows=[]
    for result,gate,halt,block,deleted in itertools.product((0,1000,1001,1002,1003),(0,0x80000000,0xffffffff),(0,1),range(5),(0,1)): # 정상/비정상 반환·현재 진단·Halt·실제 후보 입력이다.
        rows.append([gate,50,5,128,0,halt,result,deleted,block,0,0,0,0x41240000,0x41a40000])
    for gate,block,flags,form,parent in itertools.product((0,0x80000000),(0,4),(0,1,2,3),(0,5,128),(0,50,128)): # 예약0/논리범위끝·dead/free 뒤 진단 정상 복귀다.
        rows.append([gate,parent,form,128,flags,0,1002,0,block,0,0,0,0x41240000,0x41a40000])
    for mutation,gate,result,halt,deleted in itertools.product(range(1,13),(0,0x80000000),(1000,1002,1003,0xffffffff),(0,1),(0,1)): # Step/Pop/Halt/도착/진단이 현재값을 바꾼다.
        rows.append([gate,50,128 if mutation in (10,11,12) else 5,128,0,halt,result,deleted,4,mutation,0,0x55667788,0x41240000,0x41a40000])
    for gate,result,halt,block,watch in itertools.product((0,0xffffffff),(1000,1002),(0,1),(0,4),(0,0x12345678)): # 정수 위치/이전 watch/finished 전체DWORD 보존을 구별한다.
        rows.append([gate,50,5,128,0,halt,result,0,block,0,watch,0x55667788,0x41300000,0x41a80000])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """새 관측만 저장하고 재관측은 기존 정확한 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-833/path-arrival-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/path-arrival-parent-hardware-x86.tsv';report=ROOT/'cpppj/path-arrival-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=''.join(' '.join(row)+'\n' for row in matrix),text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'도착 CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for original,row in zip(matrix,rows): # 입력과 물리 필드 폭만 확인한다.
        assert len(row)==33 and row[:15]==original
    counts=collections.Counter()
    for row in rows: # 실제 진단 호출만 줄 번호별로 집계한다.
        for event in row[-1].split(';'):
            if event.startswith('A,'):counts[event.split(',',1)[1].split(':',1)[0]]+=1
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'공통 도착 CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'Windows x86 CPU /0048b340/실제0041be80·CRT절삭/TYPE','rows':len(rows),'diagnosticRows':sum('A,' in row[-1] for row in rows),'diagnosticCallsByLine':dict(counts),'fixtureSha256':digest,'binarySha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_path_arrival_probe.cpp').read_bytes()).hexdigest(),'boundary':'Step·부모 공간/Halt·가상 도착/제거·후보 iterator·정리는 명시 외부 경계;실제 Baseprocess 진단/항상1·CRT·부류/분기는 원문실행;게임/GUI/통신 없음;watch주소는 비영 의미만 비교'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum('A,' in row[-1] for row in rows),'diagnosticCallsByLine':dict(counts)}))


if __name__=='__main__':main()
