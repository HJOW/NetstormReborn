#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 Unpop의 현재 진단/지도 저장값과 실제 해시 변경을 독립 CPU로 관측한다."""
import argparse,collections,csv,hashlib,itertools,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1] # 보호 PE와 새 관측 파일의 저장소 기준이다.
def inputs():
    """원문 결과를 계산하지 않고 명시 상태/부류/해시/외부 변경 입력만 만든다."""
    rows=[]
    for genus,gate,layout,flags,foot in itertools.product((0,1,4,6,8,0x200,0x400002,0x50444202),(0,0x80000000,0xffffffff),range(5),(0,0x2000),(1,3)): # 일반 경로와 겹친 단계의 첫 제거를 구별한다.
        rows.append([gate,50,70,0,0,0x41820000,0x41a40000,foot,foot,genus,0x800 if genus==1 else 0,layout,flags,0,0x35,1])
    for state,typ,sid,gate,mutation in itertools.product((0,1,2,4,8,12),(6,70),(0,50),(0,0x80000000,0xffffffff),(0,3,11)): # 무조건 접두 진단의 정상 복귀와 reserved0을 관측한다.
        rows.append([gate,sid,typ,state,0,0x41820000,0x41a40000,1,1,4,0,1,0,mutation,0x35,0])
    for mutation,genus,gate,layout,integer in itertools.product(range(1,13),(1,4,6,0x202),(0,0x80000000),(1,2,3),(0,1)): # 미니맵/옆면/진단 뒤 현재 TYPE/위치/매달림과 보존 byte를 구별한다.
        rows.append([gate,50,70,0,0,0x41800000 if integer else 0x41820000,0x41a00000 if integer else 0x41a40000,3,3,genus,0x800 if genus==1 else 0,layout,0x2000,mutation,0x35,1])
    for width,height,gate,layout in itertools.product((0,1),(0,3),(0,0x80000000),(1,2)): # 빈 반복 범위도 void/체인/표시를 보존한다.
        rows.append([gate,50,70,0,0,0x41800000,0x41a00000,width,height,4,0,layout,0,0,0xff,0])
    for extra,gate,genus in itertools.product((8,0x88),(0,0xffffffff),(4,6,0x202)): # 현재 buried는 지도/미니맵만 생략한다.
        rows.append([gate,50,70,0,extra,0x41820000,0x41a40000,3,3,genus,0,2,0,0,0x35,1])
    return [[str(index),*(str(value) for value in row)] for index,row in enumerate(rows)]
def main():
    """새 관측만 저장하고 재관측에서는 기존 fixture의 정확한 바이트를 검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--probe',type=Path,default=ROOT/'extracted/cpp-resume-834/squid-unpop-probe.exe');parser.add_argument('--verify',action='store_true');args=parser.parse_args();fixture=ROOT/'cpppj/tests/fixtures/squid-unpop-parent-hardware-x86.tsv';report=ROOT/'cpppj/squid-unpop-hardware-native-20261104.json'
    if not args.verify and (fixture.exists() or report.exists()):raise FileExistsError('기존 관측에는 --verify를 사용하십시오')
    matrix=inputs();binary=ROOT/'originals/Netstorm.exe';run=subprocess.run([str(args.probe),str(binary)],input=''.join(' '.join(row)+'\n' for row in matrix),text=True,encoding='utf-8',capture_output=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
    if run.returncode:raise RuntimeError(f'Unpop CPU 실패{run.returncode}: {run.stderr}')
    rows=list(csv.reader(run.stdout.splitlines(),delimiter='\t'));assert len(rows)==len(matrix)
    for source,row in zip(matrix,rows):assert len(row)==29 and row[:17]==source # 실제 필드 폭/입력만 검증한다.
    counts=collections.Counter()
    for row in rows: # 실제로 선택된 진단 호출만 집계한다.
        for event in row[-1].split(';'):
            if event.startswith('A,'):counts[event.split(',',1)[1].split(':',1)[0]]+=1
    data=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(data).hexdigest()
    if args.verify:assert fixture.read_bytes()==data,'Unpop CPU 재관측 불일치'
    else:
        fixture.write_bytes(data);report.write_text(json.dumps(dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-11-03',backend='Windows x86 CPU /004afe50/실제004afd30·TYPE·CRT·GetGridSid·HashBucket',rows=len(rows),diagnosticRows=sum('A,' in row[-1] for row in rows),diagnosticCallsByLine=dict(counts),containedRows=sum('D,' in row[-1] for row in rows),fixtureSha256=digest,binarySha256=hashlib.sha256(binary.read_bytes()).hexdigest(),probeSha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),probeSourceSha256=hashlib.sha256((ROOT/'tools/native_squid_unpop_probe.cpp').read_bytes()).hexdigest(),boundary='미니맵/옆면 통지/가상 표시·진단/contained 분리와 UI 종단만 명시 외부 경계;원문 spot/side byte/해시 변경은 실제 실행. contained 요청 관측을 실제 하위 연결/실행으로 확대하지 않음. 게임/GUI/통신 없음.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(rows=len(rows),sha256=digest,verify=args.verify,diagnosticRows=sum('A,' in row[-1] for row in rows),diagnosticCallsByLine=dict(counts),containedRows=sum('D,' in row[-1] for row in rows))))
if __name__=='__main__':main()
