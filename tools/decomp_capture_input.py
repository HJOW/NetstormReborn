#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Capture 일반 입력의 현재 active/두번째 인자·뗌·모드 관문을 제품 변경 전에 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
from decomp_oracle import ROOT
from decomp_game_menu_actions import Oracle as MenuOracle,cases

COMMAND=0x0200009d  # 543070의 Capture 이름이 가리키는542968 초기 명령이다.
ACTIVE=0x5df828  # 실제 Frame과 일반 입력이 공유하는 현재 전역이다.


class Oracle(MenuOracle):
    """일반 입력은 실제 실행하고 이전에 동결한 시작/종료 본체만 명시 경계로 둔다."""
    def instruction(self,mu,address,size,data):
        """호출된 실제 부모와 앞서 정해진 상태 효과를 기록한다."""
        if address in (0x4db210,0x4db2a0):
            self.body_event=True
            self.event('start' if address==0x4db210 else 'stop')
            self.number(ACTIVE,1 if address==0x4db210 else 0)
            self.returned()
            return
        super().instruction(mu,address,size,data)


def main():
    """두 정밀도와 반복으로192입력을 설치하고 독립 결과만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle()
    seed=cases()[0]
    rows=[]
    observations=[]
    for index,(active,first,second,flags,mode) in enumerate(itertools.product((0,9),(0,305419896),(0,1,0xffffffff),(0,0x80000000,0x38000000,0xb8000000),range(4))):  # 현재active와 독립 Editor/missionPost·수정키·인자를 구별한다.
        values=list(map(int,seed['globals'].split(':')))
        values[0],values[3]=mode&1,(mode>>1)&1
        values[8]=values[9]=0
        event=COMMAND|flags
        case=seed.copy()
        case.update(name=f'Capture-{index}',event=f'{event}:{first}:{second}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        results=[]
        for control in (0x27f,0x37f,0x27f):  # 매번 현재 전역과 전체 입력을 재설치한다.
            oracle.number(ACTIVE,active)
            result=oracle.run(case,control)
            result['active']=oracle.number(ACTIVE)
            results.append(result)
        assert results[0]==results[1]==results[2]
        result=results[0]
        rows.append((event,first,second,mode,active,result['bodytrace'],result['active'],result['completed'],result['stackdelta']))
        observations.append(dict(input=case,active=active,observations=results))
    fixture=out/'capture-input-x86.tsv'
    schema=('event','first','second','mode','active','trace','activeout','completed','stackdelta')
    fixture.write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    (out/'observations.json').write_text(json.dumps(observations,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=695,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,differences=0,fixtures={fixture.name:hashlib.sha256(fixture.read_bytes()).hexdigest()},boundaries='실제Guide/Gump/일반분기와현재사건종료. 4db210/4db2a0은693의별도원문/제품대조뒤명시경계로두고active효과만제공한다. Tell은기존명시경계다. Client/OS/파일/게임/통신/MP실행0.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','tools/decomp_capture_input.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=len(rows),observations=len(rows)*3,differences=0)))


if __name__=='__main__':
    main()
