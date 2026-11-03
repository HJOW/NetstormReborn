#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""연소 생성/정리의 실제 두 판본 CPU 결과를 새 관측으로 보존한다."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 원본과 독립 관측기 기준이다.


def inputs():
    """명시 HP/검색/생성/현재 전역만 만들며 결과를 모델로 계산하지 않는다."""
    rows=[]
    for hp,maximum in ((49,100),(50,100),(51,100),(49,101),(0,0),(-1,0),(-51,-100),(-50,-100),(-32769,65536),(2147483647,2147483647),(-2147483648,-2147483648),(0,-2147483647)): # signed 극단·절반 경계·CD WORD 절삭을 구별한다.
        for first in (0,90): # 첫 검색의 없음/기존 결과다.
            for second in (0,91): # 중복 진단의 양쪽 입력이다.
                for created in (0,92): # 실제 생성 장치의 null/성공 반환이다.
                    for gate in (0,0x80000000): # 현재 DWORD 상위비트를 보존한다.
                        for kind in (70,135): # 일반/불프 위치음 요청을 구별한다.
                            rows.append([hp&0xffffffff,maximum,first,second,created,gate,kind,0,1,0])
    for action in (1,2,3,4,5,6,7,8,10,15): # finder/진단의 현재 owner 변경을 분리한다.
        for hp in (49,50): # 진단 전에 저장한 HP의 양쪽 분기다.
            for first in (0,90): # 진단 뒤에도 최초 검색 결과를 계속 사용한다.
                for created in (0,92): # 생성 성공을 가정하지 않는다.
                    for gate in (0,0xffffffff): # 첫 finder에서 켠 checking도 확인한다.
                        rows.append([hp,100,first,91,created,gate,70,0,1,action])
    for free,effect in ((1,1),(0,0),(1,0)): # free와 효과 없음의 조기 반환을 독립 입력으로 준다.
        for gate in (0,0xffffffff): # 조기 반환은 조회/진단 경계를 요구하지 않는다.
            rows.append([49,100,0,91,92,gate,135,free,effect,15])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """기존 fixture를 덮지 않고 재관측은 정확한 바이트로 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-825/damageable-burning-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    fixture=ROOT/'cpppj/tests/fixtures/damageable-burning-checking-hardware-x86.tsv';report=ROOT/'cpppj/damageable-burning-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();source=''.join(' '.join(row)+'\n' for row in matrix);rows=[];binaries={}
    for edition,relative in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')): # 게임 시작 대신 같은 입력을 독립 CPU 부모에 전달한다.
        binary=ROOT/relative
        if not binary.exists() and edition=='originalCD':binary=next((ROOT/'originalCD').rglob('NETSTORM.EXE'))
        run=subprocess.run([str(args.probe),edition,str(binary)],input=source,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
        if run.returncode:raise RuntimeError(f'{edition} CPU 실패{run.returncode}: {run.stderr}')
        observed=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(matrix)
        for original,row in zip(matrix,observed): # 입력/폭만 확인하며 출력을 새 기대 모델로 계산하지 않는다.
            assert len(row)==20 and row[:12]==[edition,*original]
        rows.extend(observed);binaries[edition]={'path':str(binary.relative_to(ROOT)),'sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'연소 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / 검색·생성·삭제·소리·진단 장치 관측','binaries':binaries,'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_damageable_burning_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'공통 부모만 실행 / 합성 HP·검색 결과 / 실제 공격·게임 시작·GUI·통신 없음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum('D,' in row[-1] for row in rows)}))


if __name__=='__main__':main()
