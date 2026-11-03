#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 시간/임시서식 부모를 실제 CPU로 관측한다. 기대 모델/GUI/통신은 없다."""
import argparse
import csv
import hashlib
import json
import struct
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 원본과 새 관측기를 찾는 저장소 기준이다.


def bits(value):
    """현재 시각/기한 입력만 binary64로 직렬화한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def timer_inputs():
    """현재 guard/두 옵션/기한/시계/경계 callback 입력을 만든다."""
    rows=[]
    for guard in (0,1,0x80000000,0x80000001): # 첫 bit0와 상위 비트 보존을 구별한다.
        for deadline in (0.0,-0.0,100.0,100.25,-5.25,float('nan')): # 예약/동일/미래/과거/비순서 경계다.
            for monitor,alert in ((0,0),(1,0),(0,1),(-2147483648,0)): # 두 옵션은 원문 signed 전체 DWORD 입력이다.
                for steps in (1,2,7): # 한 행 안에서는 현재 전역을 재초기화하지 않는다.
                    for increment in (0.25,181): # 짧은 간격과 여러180초 만료를 공급한다.
                        rows.append([guard,bits(deadline),monitor,alert,bits(100),bits(increment),steps,0])
    for action in (1,2,4,8,16,24): # 시계/달력/설정/제출 복귀 뒤 현재 전역 변경을 구별한다.
        for steps in (1,2): # 다음 프레임의 비활성/현재 이름 보존도 포함한다.
            rows.append([1,bits(1),1,1,bits(100),bits(181),steps,action])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]


def main():
    """최초 파일은 새로만 생성하고 --verify는 두 기존 파일을 덮지 않고 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-823/monitor-time-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    paths={'timer':ROOT/'cpppj/tests/fixtures/monitor-time-hardware-x86.tsv','format':ROOT/'cpppj/tests/fixtures/temporary-chat-format-hardware-x86.tsv'};report=ROOT/'cpppj/monitor-time-hardware-native-20261104.json'
    if not args.verify and (report.exists() or any(p.exists() for p in paths.values())):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrices={'timer':timer_inputs(),'format':[[str(index),str(pattern),str(action)] for index,(pattern,action) in enumerate((p,a) for p in (0,1,2) for a in (0,1))]};outputs={};binaries={}
    for mode,matrix in matrices.items(): # 두 원문 부모를 같은 관측기의 별도 프로세스로 실행한다.
        rows=[];text=''.join(' '.join(row)+'\n' for row in matrix)
        for edition,path in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')): # 판본별 상태는 서로 공유하지 않는다.
            binary=ROOT/path;binaries[edition]=hashlib.sha256(binary.read_bytes()).hexdigest();run=subprocess.run([str(args.probe),edition,mode,str(binary)],input=text,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
            if run.returncode:raise RuntimeError(f'{edition}/{mode} CPU 관측 실패{run.returncode}: {run.stderr}')
            observed=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(matrix)
            for source,row in zip(matrix,observed): # 입력 열/폭만 확인하고 의미 결과는 계산하지 않는다.
                assert row[:len(source)+1]==[edition]+source and len(row)==(17 if mode=='timer' else 8)
            rows.extend(observed)
        outputs[mode]=('\n'.join('\t'.join(row) for row in rows)+'\n').encode()
    for mode,output in outputs.items(): # 모든 CPU 실행이 성공한 뒤에만 최초 새 결과를 저장한다.
        if args.verify:assert paths[mode].read_bytes()==output,f'{mode} 실제 CPU 재관측 불일치'
        else:paths[mode].write_bytes(output)
    digests={mode:{'rows':len(data.splitlines()),'sha256':hashlib.sha256(data).hexdigest()} for mode,data in outputs.items()}
    if not args.verify:report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 전용 payload / 시계·달력·Config·제출 경계 관측','binaries':binaries,'fixtures':digests,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_monitor_time_probe.cpp').read_bytes()).hexdigest(),'sharedProbeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),'boundary':'원문 시간/서식/CRT setter 실행·0x027f/53비트·원본 게임/GUI/통신/실제 달력 시계 미실행'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'fixtures':digests,'verify':args.verify}))


if __name__=='__main__':main()
