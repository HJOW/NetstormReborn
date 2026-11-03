#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 PE의 바람/게임 난수/각도를32비트 CPU에서 결과 모델 없이 관측한다."""
import argparse
import csv
import hashlib
import json
import struct
import subprocess
from pathlib import Path

# 보호 원본은 읽기만 하고 새 콘솔 관측기는 게임/GUI 초기화를 수행하지 않는다.
ROOT=Path(__file__).resolve().parents[1]


def inputs():
    """시작 float 비트/난수/반복 수만 만들며 바람/각도의 기대 결과를 계산하지 않는다."""
    pack=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
    states=[(0,1,0),(1,-1,12.5),(-1,0,359.9999),(0,-1,-0.0),(1,1,180),(-0.0,-0.0,17.25),
        (1e-30,-1e-30,123.25),(3.4e38,3.4e38,99),(-3.4e38,-3.4e38,-99),(0.001,0.99,270)]
    states=[tuple(pack(value) for value in state) for state in states]+[(1,0x80000001,0x7fc00001),(0x80000001,1,0x80000000)]
    # 각 상태와 시드의 조합은 독립 입력이며 원문의10% 분기만 CPU가 결정한다.
    for index,state in enumerate(states):
        # 0 시드는 원문 고정 대체값 분기를 포함한다.
        for seed in (0,1,7,55,1000,0xffffffff,0x0bad0bad,0x12345678):
            # 단일/연속/서브노멀 감쇠와 같은 난수 장기 소비를 관측한다.
            for steps in (1,2,7,64,1024):
                # checking은 상태를 정규화하지 않고 전체 DWORD 입력으로 둔다.
                for gate in (0,0x80000000):yield [str(index),str(seed),str(steps),*(str(value) for value in state),str(gate)]


def main():
    """최초 관측은 새 파일만 만들며 --verify는 기존 정확한 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-821/ambient-wind-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    fixture=ROOT/'cpppj/tests/fixtures/ambient-wind-hardware-x86.tsv';report=ROOT/'cpppj/ambient-wind-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=list(inputs());text=''.join(' '.join(row)+'\n' for row in matrix);rows=[];binaries={}
    # 양판본에서 같은 입력을 독립 실행하고 CPU 출력만 보존한다.
    for edition,path in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')):
        binary=ROOT/path;binaries[edition]=hashlib.sha256(binary.read_bytes()).hexdigest()
        result=subprocess.run([str(args.probe),edition,str(binary)],input=text,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
        if result.returncode:raise RuntimeError(f'{edition} CPU 바람 관측 실패{result.returncode}: {result.stderr}')
        observed=list(csv.reader(result.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(matrix)
        # 처음8열이 실제 입력과 같고 결과 열 폭이 고정인지 검사한다.
        for source,row in zip(matrix,observed):assert len(row)==14 and row[:8]==[edition]+source
        rows.extend(observed)
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert fixture.read_bytes()==output,'바람 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(output);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 별도 콘솔 / 보호 PE 메모리 복사','binaries':binaries,'rows':len(rows),'fixtureSha256':digest,
            'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_ambient_wind_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),
            'boundary':'0x027f/53비트·실제 바람/두 난수/fpatan/각도·원본 초기화/GUI/게임/소켓0'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':main()
