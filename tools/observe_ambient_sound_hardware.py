#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 실제 주변음/재예약/바람 시퀀스를 CPU에서 관측한다. 기대 결과 모델은 없다."""
import argparse
import csv
import hashlib
import json
import struct
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 실제 원본/새 독립 관측기만 읽는 저장소 기준이다.


def bits(value):
    """외부 시각 입력을 binary64 그대로 전달한다. 원문 결과는 계산하지 않는다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def inputs():
    """기한 경계·signed 강제값·현재 모달·연속 호출을 입력으로만 열거한다."""
    rows=[]
    # 기본 단일 부모는 미래/동일/과거/NaN 기한과 세 정상 종류/범위 밖 종류를 모두 포함한다.
    for deadline in (0.0,100.0,100.00000000000001,float('nan')): # 현재 시각100과의 경계를 독립 입력으로 둔다.
        for kind in (-1,0,1,2,3): # 정상 세 종류와 범위 밖 현재값을 구별한다.
            for forced in (-2147483648,0,1,2147483647): # signed 양수 관문의 전체 폭을 유지한다.
                for dialog in (0,0x80000000): # 모달은 개수 대신 현재 DWORD 비영을 쓴다.
                    for seed in (0,55,0xffffffff): # 0대체/최초55/상위 비트 난수 입력이다.
                        rows.append([seed,bits(deadline),kind,forced,dialog,bits(100),bits(0.25),1,0,0])
    # 첫 시계/소리 경계 뒤 바뀐 현재 전역을 실제 부모가 다시 읽는 순서를 보존한다.
    for action in (1,2,4,8,9,10): # 경계 복귀 뒤 바뀌는 현재 전역의 조합이다.
        for kind in (0,1,2): # 음향/추가 난수/flash의 각 선택 경로를 만든다.
            for dialog in (0,0x80000000): # callback 앞의 최초 모달 입력이다.
                rows.append([55,bits(0),kind,1,dialog,bits(100),bits(0.25),1,0,action])
    # 실제 Client 순서의 같은 난수·원문 재예약·여러 만료를 결합한다.
    for mode in (1,2,3): # 재예약/주변음-바람/두 결합을 구별한다.
        for steps in (0,1,2,7,32): # 0은 재예약만, 나머지는 같은 상태의 연속 호출이다.
            for seed in (0,1,55,0xffffffff): # 각 행 안의 난수는 재시드하지 않는다.
                for increment in (0.25,121): # 짧은 간격/여러 만료를 외부 시계로 공급한다.
                    rows.append([seed,bits(-3.25),0,0,0,bits(100),bits(increment),steps,mode,0])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """최초 파일은 새로만 생성하고 --verify는 기존 관측 바이트를 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-822/ambient-sound-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    fixture=ROOT/'cpppj/tests/fixtures/ambient-sound-hardware-x86.tsv';report=ROOT/'cpppj/ambient-sound-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 CPU 관측에는 --verify를 사용하십시오')
    matrix=inputs();text=''.join(' '.join(row)+'\n' for row in matrix);rows=[];binaries={}
    # 양판본은 각각 새 콘솔 프로세스에서 같은 입력으로 관측한다. 게임 시작/GUI/장치는 실행하지 않는다.
    for edition,path in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')):
        binary=ROOT/path;binaries[edition]=hashlib.sha256(binary.read_bytes()).hexdigest()
        result=subprocess.run([str(args.probe),edition,str(binary)],input=text,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
        if result.returncode:raise RuntimeError(f'{edition} 실제 CPU 관측 실패{result.returncode}: {result.stderr}')
        observed=list(csv.reader(result.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(matrix)
        # 입력12열과 출력 폭을 검사하며 의미 결과는 계산하지 않는다.
        for source,row in zip(matrix,observed): # 입력이 바뀌거나 행 폭이 무너지면 관측을 거부한다.
            assert len(row)==22 and row[:12]==[edition]+source
        rows.extend(observed)
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert fixture.read_bytes()==output,'주변음 실제 CPU 재관측 불일치'
    else:
        fixture.write_bytes(output);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / 시계·음향·화면 요청 관측','binaries':binaries,'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_ambient_sound_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'주변음/난수/재예약/선택 바람 원문 실행·0x027f/53비트·실제 OS시계/PCM/화면 프로세스는 경계 관측'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':main()
