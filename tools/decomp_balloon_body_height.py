#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존233원문을 보존하고 벌룬 그림자 고도 BYTE 전체256값을 추가 관측한다."""
import argparse,json,socket
from decomp_balloon_body import Oracle,ROOT,digest

FIXTURE=ROOT/'cpppj/tests/fixtures/balloon-body-height-x86.tsv' #기존233입력과 동결파일을 수정하지 않는 추가 원문이다.
REPORT=ROOT/'cpppj/balloon-body-height.json' #별도 고도 전범위 관측 근거다.

def cases():
    """X=0에서 중간float 저장 유무가 드러나도록 모든 고도를 입력한다."""
    return [(0,0,0,51,0,height,1,0,0,0,1) for height in range(256)]

def main():
    """기대 좌표 계산 없이 같은 원문 몸체의 별도 관측을 생성한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        for p,h in r['sha256'].items():assert digest(ROOT/p)==h,p
        assert len(rows)==256 and [r[0] for r in rows]==[','.join(map(str,v)) for v in cases()] and all(r[1:4]==r[4:7] for r in rows);print('벌룬 그림자 고도 전범위256 감사');return
    oracle=Oracle();rows=[];different=[]
    # 같은 보호PE의 두 정밀도 결과만 읽는다.
    for index,value in enumerate(cases()):
        result=[oracle.sample(value,p) for p in (0x27f,0x37f)]
        if result[0]!=result[1]:different.append(index)
        rows.append((','.join(map(str,value)),*result[0],*result[1]))
    FIXTURE.write_text('# input effects53 raw53 globals53 effects64 raw64 globals64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','tools/decomp_balloon_body_height.py','tools/decomp_balloon_body.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/balloon-body-height-x86.tsv')
    r=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=dict(oracle.counts),instructions=oracle.instructions,precisionDifferentInputs=different,boundaries='42ac90의 같은 원문/경계를 재사용한다. 기존233입력/fixture/관측기는 변경하지 않는다. C++ 결과를 읽지 않는다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','precisionDifferentInputs')},ensure_ascii=False))

if __name__=='__main__':main()
