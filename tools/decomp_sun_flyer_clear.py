#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리기그 첫Frame 전 삭제의 실제DWORD SID 범위 검사와 정리를 대조한다."""
import argparse
import csv
import json
import socket
from collections import Counter
from decomp_sun_flyer_frame import Oracle as BaseOracle,ROOT,digest

FIXTURE=ROOT/'cpppj/tests/fixtures/sun-flyer-clear-x86.tsv'  # 기존410행을바꾸지않는추가경계원문이다.
REPORT=ROOT/'cpppj/sun-flyer-clear-20261022.json'  # 실제helper/일반정리/부모삭제의정상복귀근거다.


class Oracle(BaseOracle):
    """기존정리실행기의외부SID경계를실제4ac140기계어로바꾼다."""
    def __init__(self,control):
        """개발자진단0의정상범위검사몸체만추가한다."""
        super().__init__(control);facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for part in facts[0x4ac140]['ranges'].split(';'):  # 실제불연속범위를그대로사용한다.
            lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))

    def instruction(self,mu,address,size,data):
        """범위helper를훅의성공값으로대신하지않고순수원문으로실행한다."""
        if address==0x4ac140:self.calls['004ac140']+=1;self.instructions+=1;return
        super().instruction(mu,address,size,data)


def main():
    """새경계30입력을별도동결하거나원본/제품SHA를확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 원문/제품의동일바이트만감사한다.
        print('훨리기그 전체DWORD 정리 SHA:',facts['uniqueInputs']);return
    values=(0,80,65535,65536,0x7fffffff,0x80000000,0xffffffff,0x55555555,0x12345678,0x12000000)
    cases=[('C',(value,0)) for value in values]+[('P',(1,home,0,value)) for home in (0,61) for value in values]
    oracles=[Oracle(control) for control in (0x27f,0x37f)];rows=[]
    for case in cases:  # 고유입력과정밀도반복을혼동하지않는다.
        results=[oracle.sample(case) for oracle in oracles];assert results[0]==results[1],case;rows.append(results[0])
    FIXTURE.write_text('# kind input fields136 objects450 globals20 result effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_sun_flyer_frame.py','tools/decomp_flyer_frame.py','tools/decomp_sun_flyer_clear.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),nativeInstructions=sum(oracle.instructions for oracle in oracles),nativeCalls=dict(sum((oracle.calls for oracle in oracles),Counter())),precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['45be00/45beb0/45b7a0과실제4ac140을실행한다. 개발자상세진단은0이며별도world삭제/홈가상효과는기존명시경계다.','0/80/ffff및16비트밖값을관측한다. 1..fffe중격리슬롯밖을역참조하는비정상입력은정상관측으로세지않는다. 기존410행은변경하지않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('훨리기그 DWORD 정리:',len(rows),'입력/두 정밀도 차이0')


if __name__=='__main__':main()
