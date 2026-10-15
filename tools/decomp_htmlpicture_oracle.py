#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기본 HTML의 실제 그림 helper를 두 PE에서 격리 실행한다. 게임/GUI/OS 실행은 없다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_htmltext_oracle import Oracle,SPEC,ROOT

# 기존816개의 서식 관찰과 겹치지 않는 그림 전용 입력/보고 경로다.
FIXTURE=ROOT/'cpppj/tests/fixtures/htmlpicture-x86.tsv'
REPORT=ROOT/'cpppj/htmlpicture.json'


def digest(path):
    """보호 원본과 새 근거의 현재 바이트를 SHA-256으로 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cases():
    """원본 미션의 실제 그림 형태와 인용/종료/숨김 상태를 독립 입력으로 정한다."""
    tags=[b'<!"geyser.*">',b'<!"nugget.*">',b'<!"priest.*">',b'<!"windvortex.*">',b'<!"sunFactory.*">',b'<!"mana.17">',b'<!"mana.11">',b'<!"mana.3">',b'<!"mana.8>',b'<!"mana.9>',b'<!"mana.10>',
          b'<! "mana.17">',b'<!invalid>',b'<!"">',b'<!"a>b">',b'<!before"x.y"after>',b'<!"a]b.c">',b'<!"\x80.0">',b'<!"a\0tail">',b'</!"a.b">',b'<!',b'<!"unterminated',b'<!"'+b'x'*78+b'">']
    # 미완성 태그도 뒤 문자열의 '>'/따옴표를 실제 helper가 어떻게 소비하는지 그대로 관찰한다.
    return [( (b'pre'+tag+b'post').hex(),8192,shown,depth,0) for tag,shown,depth in itertools.product(tags,(0,1,0xffffffff),(0,2))]


def generate():
    """C++ 없이 실제 helper/전체 순회가 만든 출력·커서·상태를 저장한다."""
    rows=[];returns={};instructions={}
    # 기존 기본 파서 VM의 정확 범위에 실제 그림 helper 몸체만 추가한다.
    for edition,extra in (('originals',(0x46bf40,0x46bfb5)),('originalCD',(0x495310,0x49536d))):
        oracle=Oracle(edition);oracle.spec={**SPEC[edition],'ranges':[*SPEC[edition]['ranges'],extra]}
        # 같은 입력은 두 판본에서 각각 한 번만 실행한다.
        for case in cases():rows.append([edition,*case,*oracle.run(case)])
        returns[edition]=oracle.returns;instructions[edition]=oracle.instructions
    FIXTURE.write_text('# 두 PE 실제 HTML 그림 helper/전체 순회. tolower ASCII 경계; 게임/GUI/OS 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_htmltext_oracle.py',FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originalCD/NETSTORM.EXE']
    report={'host':socket.gethostname(),'date':'2026-10-15','unique_inputs':len(rows),'returns':returns,'instructions':instructions,'os_calls':0,
        'boundaries':['CRT tolower signed input/ASCII mapping only'],'files':{p.relative_to(ROOT).as_posix():digest(p) for p in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('HTML 그림 관찰',len(rows))


def verify():
    """저장 근거의 SHA/입력계획/집계를 새 native 실행 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본/생성기/fixture 중 하나라도 바뀌면 이전 근거를 현재 근거로 쓰지 않는다.
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('HTML 그림 SHA 불일치 '+path)
    plan=[[edition,*case] for edition in SPEC for case in cases()]
    if len(rows)!=len(plan) or report['unique_inputs']!=len(rows) or report['os_calls']:raise RuntimeError('HTML 그림 집계 불일치')
    # 출력에서 입력 계획을 역산하지 않고 사전에 정한 모든 입력을 확인한다.
    for row,case in zip(rows,plan):
        if row[:6]!=list(map(str,case)):raise RuntimeError('HTML 그림 입력 계획 불일치')
    if any(count!=len(cases()) for count in report['returns'].values()):raise RuntimeError('HTML 그림 반환 집계 불일치')
    print('HTML 그림 근거 감사 통과',len(rows))


def main():
    """새 native 관찰과 저장 근거 감사 중 요청한 작업만 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()


if __name__=='__main__':main()
