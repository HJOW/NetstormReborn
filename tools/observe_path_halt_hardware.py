#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 Halt의 실제CPU 진단 복귀/현재 권한·부모/기존 재지정 입력을 관측한다."""
import argparse
import csv
import itertools
import hashlib
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 PE와 독립 관측기의 기준이다.


def inputs():
    """출력 모델 없이 실제 가상 반환/현재 전역/재지정 장치 입력만 만든다."""
    rows=[]
    for result,gate,boss,form,forced,deleted in itertools.product((0,10,11,12,1000,0xffffffff),(0,0x80000000,0xffffffff),(0,1),(0,4,5,14999,15000,0xffffffff),(0,32),(0,1)): # 첫 가상 결과/진단·signed 로컬 범위/강제 정지다.
        rows.append([result,gate,boss,form,forced,0,0,0,0,0,deleted])
    for result,gate,boss,genus,targets,finds,choice in itertools.product((0,10,11,12),(0,0x80000000),(0,1),(0,0x10000,0x30000),(1,2,3),((0,0),(0,1),(1,0),(1,1)),(0,70)): # 원문 내용물 두 조회/실제 후보·새 이동의 외부 반환이다.
        rows.append([result,gate,boss,5,0,genus,targets,*finds,choice,0])
    for result,targets,gate,action in itertools.product((0,10,11,12),(0,3),(0,0x80000000),(2,4,8,16,32,64,128,129,130,160,192,256,257,512,513,546,768)): # 첫 가상/진단/새 이동 뒤 현재 상태와 이전 watch 덮어쓰기 입력이다.
        rows.append([result,gate,1,15000,0,0x10000,targets,1,0,70,action])
    return [[str(index),*(str(v) for v in row)] for index,row in enumerate(rows)]


def main():
    """최초 새 fixture만 저장하며 재관측은 기존 정확한 바이트와 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-832/path-halt-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/path-halt-parent-hardware-x86.tsv';report=ROOT/'cpppj/path-halt-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=''.join(' '.join(row)+'\n' for row in matrix),text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'Halt CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for original,row in zip(matrix,rows): # 입력 순서/물리 필드 폭만 검증하고 기대 분기를 만들지 않는다.
        assert len(row)==24 and row[:12]==original
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest();counts={'diagnosticRows':sum('A,725:' in row[-1] for row in rows),'deadFlagDiagnosticRows':sum('A,754:' in row[-1] for row in rows)}
    if args.verify:assert fixture.read_bytes()==data,'공통 Halt 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'Windows x86 CPU / 부모00489ff0·순수 GetGenus/TYPE·실제 재지정 분기','rows':len(rows),**counts,'fixtureSha256':digest,'binarySha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_path_halt_probe.cpp').read_bytes()).hexdigest(),'boundary':'부모 가상Halt/진단/삭제 요청·내용물/후보/새 이동은 명시 외부 경계;실제 game/GUI/통신/포획·희생·공격 프레임 없음;stack watch 주소 자체는 비교하지 않음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,**counts}))


if __name__=='__main__':main()
