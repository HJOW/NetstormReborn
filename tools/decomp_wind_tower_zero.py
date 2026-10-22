#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""윈드타워48ef70의 공격자0 풀 조회를 기존 원문과 별도로 관측한다."""
import argparse
import itertools
import json
import socket
import decomp_special_damage as source
from decomp_special_damage import ROOT,fbits,digest

FIXTURE=ROOT/'cpppj/tests/fixtures/wind-tower-zero-x86.tsv'  # 기존432행은 보존한다.
REPORT=ROOT/'cpppj/wind-tower-zero-20261023.json'  # 실제 두 정밀도의 반환/명령을 기록한다.


def main():
    """독립 추가 원문을 생성하거나 동결된 바이트를 읽기 전용 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**report['sha256'],**report.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 기대 결과를 다시 계산하지 않는다.
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['rows'];print('윈드타워 공격자0 SHA:',len(rows));return
    source.ATTACKER=0;oracle=source.Oracle();rows=[]  # 실제 SID 곱셈에 0을 전달하며 포인터 경계를 대체하지 않는다.
    for genus,kind,side,(x,y,width,height) in itertools.product((0,0x100,0x100000),(0,128,149),map(ord,'LMNOAX'),((50,40,1,1),(60,50,1,1),(50,60,3,3),(40,50,3,3))):  # 현재0슬롯의 명시 타입/좌표 입력이다.
        case=(1,genus,kind,side,fbits(x),fbits(y),width,height);first=oracle.sample(case,0x27f);second=oracle.sample(case,0x37f);assert first==second,case;rows.append(','.join(map(str,case))+'\t'+first)
    FIXTURE.write_text('# 공격자 SID는0 / 입력 / 반환 / 효과\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_special_damage.py','tools/decomp_wind_tower_zero.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,precisionDifferences=0,nativeCalls=dict(oracle.calls),sha256={p:digest(ROOT/p) for p in paths},limitations=['48ef70/TYPE/중심/방향의 실제 몸체를 실행한다.','0슬롯의 명시 입력이며 실제 세션 free-list 모양의 모든 경우를 주장하지 않는다. 기존432행은 바꾸지 않는다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('윈드타워 공격자0:',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
