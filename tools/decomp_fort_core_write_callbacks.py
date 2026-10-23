#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초360관측을보존하고실제1기준callback명으로주소보관/부분중단을추가관측한다."""
import argparse
import json
import socket
from decomp_fort_core_write import Oracle, ROOT, digest

# 초기관측과별개인추가명시입력만기록한다.기존도구/보고서/fixture는수정하지않는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-core-write-callbacks-x86.tsv'
REPORT = ROOT / 'cpppj/fort-core-write-callbacks-native.json'


def main():
    """부모의현재global재조회와고정Player주소를선택된callback변경으로검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():  # 최초보고서/도구는그대로SHA대조한다.
            if digest(ROOT/name)!=expected:raise RuntimeError('CoreWritecallback최초SHA불일치:'+name)
        print('CoreWritecallback최초PE확인',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('CoreWritecallback최초관측덮어쓰기금지')
    mutations=('create1-playerB','create1-owner8','create1-manager22','create1-post1','create1-editor1','create1-stop',
               'refresh1-playerB','refresh1-core','refresh1-seed1','refresh1-stop','prepare1-seed1','prepare1-seed80000000','prepare1-playerB','prepare1-stop',
               'create1-seed12340000','create1-seed1','create1-core','refresh1-seed12340000')
    inputs=[(1,11,0,0,0,1,0,0,mutation) for mutation in mutations]
    oracle=Oracle();lines=['# 초기자료의create/refresh/prepare는명시1기준이벤트와불일치하여변경되지않았다.이별도자료는실제create1/refresh1/prepare1을선택한다.열은초기fixture와같다.']
    for case in inputs:  # 추가입력의결과는제품없이실제364BYTE가결정한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('CoreWritecallback정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','tools/decomp_fort_core_write.py','tools/decomp_fort_core_write_callbacks.py','cpppj/tests/fixtures/fort-core-write-x86.tsv','cpppj/tests/fixtures/fort-core-write-extra-x86.tsv','cpppj/tests/fixtures/fort-core-write-callbacks-x86.tsv')
    report={'status':'독립추가PE고정·기대역산없음','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','normalReturns','statuses','instructions')}))


if __name__=='__main__':main()
