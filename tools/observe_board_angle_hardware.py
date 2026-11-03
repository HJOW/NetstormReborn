#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""결과 모델 없이 실제32비트 콘솔의 원본 PE/CRT 관측을 보존하거나 재검증한다."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path

# 저장소 안의 보호 PE만 명시 경로로 읽으며 원본 프로세스/GUI를 시작하지 않는다.
ROOT=Path(__file__).resolve().parents[1]


def main():
    """기존 관측을 덮지 않고 같은 입력 행렬의 실제 CPU 출력을 기록한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-820/board-angle-probe.exe');parser.add_argument('--inputs',type=Path)
    parser.add_argument('--fixture',type=Path,default=ROOT/'cpppj/tests/fixtures/board-angle-hardware-x86.tsv');parser.add_argument('--report',type=Path,default=ROOT/'cpppj/board-angle-hardware-native-20261104.json');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (args.fixture.exists() or args.report.exists()):raise FileExistsError('기존 증거에는 --verify를 사용하십시오')
    source=args.inputs or (args.fixture if args.fixture.exists() else ROOT/'extracted/board-angle-unicorn-x86.tsv')
    with source.open(encoding='utf-8') as stream:inputs=list(csv.reader(stream,delimiter='\t'))
    rows=[];binaries={}
    # 기존 행의 앞5입력 열만 재사용한다. 결과 열은 관측기에 전달하지 않는다.
    for edition,path in (('originals','originals/Netstorm.exe'),('originalCD','originalCD/NETSTORM.EXE')):
        selected=[row for row in inputs if row[0]==edition];binary=ROOT/path;binaries[edition]=hashlib.sha256(binary.read_bytes()).hexdigest()
        text=''.join(' '.join(row[1:5])+'\n' for row in selected)
        result=subprocess.run([str(args.probe),edition,str(binary)],input=text,text=True,encoding='utf-8',capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
        if result.returncode:raise RuntimeError(f'{edition} CPU 관측 실패{result.returncode}: {result.stderr}')
        observed=list(csv.reader(result.stdout.splitlines(),delimiter='\t'));assert len(observed)==len(selected)
        # 반환값은 CPU 출력 그대로 사용하며 입력 순서와 열 폭만 검사한다.
        for before,after in zip(selected,observed):assert len(after)==8 and after[:5]==before[:5]
        rows.extend(observed)
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert args.fixture.read_bytes()==output,'실제32비트 CPU 재관측 불일치'
    else:
        args.fixture.write_bytes(output);args.report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','backend':'실제 Windows x86 CPU / 별도 콘솔 / PE 메모리 복사','floatingControl':'0x027f / x87 53비트',
         'binaries':binaries,'rows':len(rows),'fixtureSha256':digest,'probeSha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),'probeSourceSha256':hashlib.sha256((ROOT/'tools/native_board_angle_probe.cpp').read_bytes()).hexdigest(),
         'boundary':'원본 초기화/GUI/실제 게임/소켓0·실제 PE 부모/CRT만 호출·진단 UI만 관측 callback·파일은 읽기 전용'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':main()
