#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비멀티 숫자 확인의 원문 현재 checking/대상 저장/진단 복귀를 실제CPU로 관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
import struct
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 PE와 독립 관측기의 공통 기준이다.


def bits(value):
    """SP 입력을 반올림한32비트 float 비트로만 바꾼다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """AI/미선택 입력만 만들고 기대 반환/분기는 계산하지 않는다."""
    rows=[]
    for text,known,gate,mission,flags in itertools.product(('', '0','-1','700','  +700junk','700\n999','999999999'),(0,1),(0,0x80000000,0xffffffff),(0,1),(0,0x20,0x22)): # 숫자 해석/현재DWORD/AI 우선 가지다.
        rows.append([text,known,bits(1000),bits(1000),gate,mission,flags,0,2,1,0])
    for first,second,editor,action in itertools.product((699.5,700,1000),(699.5,700,1000),(0,1),(0,1,2,3,4,6,7,8,12,15)): # 두 현재 SP 조회/진단·차감 뒤 대상 변경 입력이다.
        rows.append(['700',1,bits(first),bits(second),0x80000000,0,0x20,editor,2,1,action])
    return [[str(index),row[0].encode().hex() or '-',*(str(value) for value in row[1:])] for index,row in enumerate(rows)]


def main():
    """새 관측만 저장하고 재관측은 기존 정확한 바이트와 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-829/money-confirm-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/money-confirm-hardware-x86.tsv';report=ROOT/'cpppj/money-confirm-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=''.join(' '.join(row)+'\n' for row in matrix),capture_output=True,text=True,encoding='utf-8',timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'숫자 확인 CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for original,row in zip(matrix,rows): # 입력과 물리 필드 폭만 검사한다.
        assert len(row)==18 and row[:12]==original
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'숫자 확인 CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','rows':len(rows),'diagnosticRows':sum('A,' in row[-1] for row in rows),'fixtureSha256':digest,'binarySha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_money_confirm_probe.cpp').read_bytes()).hexdigest(),'boundary':'004c9350 숫자 확인의 비멀티 AI/미선택 가지;CRT·CopyText·SP·진단·차감/AI지갑은 명시 외부 경계;연결 Player/패킷 가지는 도달 시 실패;게임/GUI/통신 없음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify,'diagnosticRows':sum('A,' in row[-1] for row in rows)}))


if __name__=='__main__':main()
