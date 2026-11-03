#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제단 해방 부모의 실제CPU 진단 복귀/저장 SID/현재 프레임·공간 순서를 관측한다."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 읽기 전용 원본과 독립 관측기 기준이다.


def inputs():
    """전체 출력 모델 없이 두 방향 getter/owner/경계/현재 TYPE 입력만 만든다."""
    rows=[]
    for first in (-1,0,10,11,12,13): # 첫12는 한 번,그 외는 다음 현재 방향 판정을 선택한다.
        for second in (-1,0,11,12): # 두 번째 값11만 추가 진단을 선택한다.
            for gate in (0,0x80000000,0xffffffff): # 전체 DWORD checking을 보존한다.
                for found in (0,1,127,24000,65535): # 원문 pointer 범위의 없음/예약1/유효/끝/범위밖이다.
                    for capacity in (128,24000): # 진단 뒤 읽는 현재 범위의 두 입력이다.
                        for frame in (7,0x80000000): # 표시 없는 프레임 DWORD 저장을 구별한다.
                            rows.append([first,second,gate,found,capacity,frame,0])
    for action in (1,2,3,4,5,6,7,8,16,24,32,36,63): # finder/방향/진단/해방/Unpop이 현재 입력을 바꾼다.
        for found in (0,127): # 저장 결과가 현재 finder 입력과 다르게 바뀔 수 있다.
            for first,second in ((12,0),(11,11),(10,11)): # 단일/두 번째 진단의 현재값 변화를 분리한다.
                for gate in (0,0xffffffff): # 미선택 장치의 변경을 가정하지 않는다.
                    rows.append([first,second,gate,found,128,7,action])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """최초 새 fixture만 만들고 재관측은 기존의 정확한 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-828/altar-release-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/altar-release-parent-hardware-x86.tsv';report=ROOT/'cpppj/altar-release-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();source=''.join(' '.join(row)+'\n' for row in matrix);binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=source,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'해방 부모 CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for original,row in zip(matrix,rows): # 입력과 물리 폭만 확인하며 결과를 복원 코드로 생성하지 않는다.
        assert len(row)==14 and row[:8]==original
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'제단 부모 실제CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / typed finder·방향·진단·해방·공간·TYPE 장치 관측','binarySha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_altar_release_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'00448bd0 부모만 실행;00448500 사제 해방 본체는 명시 관측 경계 / 실제 포획·희생·게임·GUI·통신 없음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum('A,' in row[-1] for row in rows)}))


if __name__=='__main__':main()
