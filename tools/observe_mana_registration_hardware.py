#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본 CPU의 공급원 중복 진단/등록/생산 카운터를 새 fixture로 보존한다."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 읽기 전용 원본과 관측기 기준이다.


def inputs():
    """목록/개수/용량/진단 입력만 만들며 출력 판단은 실제 CPU에 맡긴다."""
    rows=[]
    for count,capacity in ((0,0),(0,4),(1,4),(3,4),(4,4),(4,2),(0,0x80000000),(3,0xffffffff)): # 빈/여유/가득참/축소/음수 용량의 signed 비교다.
        for pattern in (0,1,2,3): # 중복 없음/앞/뒤/양쪽 중복의 명시 배치다.
            for value in (0,50,65536,0xffffffff): # 원문 인자는16비트 SID가 아닌 전체 DWORD다.
                for dirty in (0,0xffffffff): # 생산 카운터의 감김을 확인한다.
                    for checking in (0,0x80000000): # 현재 checking 상위 비트를 보존한다.
                        rows.append([value,count,capacity,pattern,dirty,checking,0])
    for action in range(1,16): # 진단에서 현재 checking/물리 목록/개수/카운터/중복 원소를 바꾼다.
        for checking in (0,0xffffffff): # 미선택 장치는 모든 변경을 수행하지 않아야 한다.
            rows.append([50,3,4,3,0x7fffffff,checking,action])
    return [[str(index),*(str(word) for word in row)] for index,row in enumerate(rows)]


def main():
    """최초 관측은 새 파일만 만들고 재관측은 정확한 기존 바이트와 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-826/mana-registration-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/mana-registration-hardware-x86.tsv';report=ROOT/'cpppj/mana-registration-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();source=''.join(' '.join(row)+'\n' for row in matrix);rows=[];binaries={}
    for edition,relative in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')): # 게임 시작 없이 두 독립 원문 부모를 호출한다.
        binary=ROOT/relative;run=subprocess.run([str(args.probe),edition,str(binary)],input=source,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
        if run.returncode:raise RuntimeError(f'{edition} CPU 실패{run.returncode}: {run.stderr}')
        observed=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(matrix)
        for original,row in zip(matrix,observed): # 입력과 폭만 확인하며 결과를 제품 모델로 만들지 않는다.
            assert len(row)==17 and row[:9]==[edition,*original]
        rows.extend(observed);binaries[edition]={'path':relative,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'마나 등록 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / 진단 장치 관측','binaries':binaries,'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_mana_registration_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'유효 물리 목록16DWORD/활성0~4·signed 용량·독립 부모만 실행 / 실제 게임·GUI·통신 없음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum(row[-1]!='-' for row in rows)}))


if __name__=='__main__':main()
