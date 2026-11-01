#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""URL 일반 입력의 실제 인자·뗌/수정키/모드 분기를 제품 변경 전에 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
from decomp_oracle import ROOT
from decomp_game_menu_actions import Oracle as MenuOracle,cases
from unicorn.x86_const import UC_X86_REG_ESP

COMMAND=0x0200005b  # 보호542e60의 URL 이름→542864 전역이다.


class Oracle(MenuOracle):
    """원문 Guide/Gump/일반 입력은 실행하고 링크 소비 부모만 명시 경계로 둔다."""
    def instruction(self,mu,address,size,data):
        """4a00c0에 실제 push한 DWORD를 기록하며 링크/OS/통신을 실행하지 않는다."""
        if address==0x4a00c0:
            self.body_event=True
            self.event('url',(self.number(mu.reg_read(UC_X86_REG_ESP)+4),))
            self.returned()
            return
        super().instruction(mu,address,size,data)


def main():
    """독립96입력을 두 정밀도와 반복으로 관측해 새 디렉터리에만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle()
    seed=cases()[0]
    rows=[]
    observations=[]
    for index,(first,second,flags,mode) in enumerate(itertools.product((0,305419896),(0,6008,0xffffffff),(0,0x80000000,0x38000000,0xb8000000),range(4))):  # 두 인자/수정키와 독립 Editor·missionPost 입력이다.
        values=list(map(int,seed['globals'].split(':')))
        values[0],values[3]=mode&1,(mode>>1)&1
        values[8]=values[9]=0
        event=COMMAND|flags
        case=seed.copy()
        case.update(name=f'URL-{index}',event=f'{event}:{first}:{second}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        results=[oracle.run(case,control) for control in (0x27f,0x37f,0x27f)]  # 같은 실제 입력을 재설치하며 제품 정답을 사용하지 않는다.
        assert results[0]==results[1]==results[2]
        result=results[0]
        rows.append((event,first,second,mode,result['bodytrace'],result['completed'],result['stackdelta']))
        observations.append(dict(input=case,observations=results))
    fixture=out/'url-input-x86.tsv'
    schema=('event','first','second','mode','trace','completed','stackdelta')
    fixture.write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    (out/'observations.json').write_text(json.dumps(observations,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=692,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,differences=0,fixtures={fixture.name:hashlib.sha256(fixture.read_bytes()).hexdigest()},boundaries='실제4d6e61..4da477의Guide/Gump/일반분기와현재사건종료. 4a00c0 링크소비만명시경계이며실제전체DWORD인자를기록한다. 링크/OS/외부프로그램/통신/게임/MP실행0. 상위Guide/Gump의기존소비증거와구별한다.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','tools/decomp_url_input.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=len(rows),observations=len(rows)*3,differences=0)))


if __name__=='__main__':
    main()
