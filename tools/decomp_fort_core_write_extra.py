#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초364BYTE자료를보존하고진단3/상위DWORD추가입력만제품전에관측한다."""
import argparse
import json
import socket
from decomp_fort_core_write import Oracle, ROOT, digest

# 최초fixture/보고서/도구는변경하지않으며추가조건만별도파일에고정한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-core-write-extra-x86.tsv'
REPORT = ROOT / 'cpppj/fort-core-write-extra-native.json'


def main():
    """3번째진단까지모두선택한콜백과비영DWORD관문을추가한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():  # 최초자료를제품기대로덮어쓰지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('CoreWrite추가최초SHA불일치:'+name)
        print('CoreWrite추가최초PE확인',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('CoreWrite추가최초관측덮어쓰기금지')
    inputs=[(1,0,1,0,0,7,0,0,mutation) for mutation in ('assert3-playerB','assert3-core','assert3-stop','assert1-meta0','assert1-editor1','assert1-manager22','assert2-post1')]
    inputs.extend((1,11,meta,post,editor,1,seed,1,'none') for meta,post,editor,seed in ((0x80000000,0,0,0),(0,0x80000000,0,0),(0,0,0x80000000,0),(0xffffffff,0,0,0xffffffff)))
    oracle=Oracle();lines=['# 최초364BYTE추가진단3/현재상위DWORD입력.열은fort-core-write-x86.tsv와같다.']
    for case in inputs:  # 동일입력을두x87정밀도에서독립실행한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('CoreWrite추가정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','tools/decomp_fort_core_write.py','tools/decomp_fort_core_write_extra.py','cpppj/tests/fixtures/fort-core-write-x86.tsv','cpppj/fort-core-write-native.json','cpppj/tests/fixtures/fort-core-write-extra-x86.tsv')
    report={'status':'추가PE고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','normalReturns','statuses','instructions')}))


if __name__=='__main__':main()
