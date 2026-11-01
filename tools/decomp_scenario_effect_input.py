#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""StartSmoke/Lightning/CreateRandomGuns의 일반 입력을 제품 변경 전에 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
from decomp_game_menu_actions import Oracle as MenuOracle, cases
from unicorn.x86_const import UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 기존 보호 PE와 명시 원문 경계만 사용한다.
WIDTH = 0x531860  # Lightning이 현재 기술 부모를 조회하기 전에 읽는 화면 폭이다.
COMMANDS = (('StartSmoke',0x02000035),('Lightning',0x020000b6),('CreateRandomGuns',0x020000f6))  # 보호 이름표의 실제 코드다.


class Oracle(MenuOracle):
    """원문 Guide/Gump와 일반 분기는 실행하고 표시 생성 부모만 명시 경계로 둔다."""
    def instruction(self,mu,address,size,data):
        """원본 push 인자·기술 조회 순서와 폭의 선행 보관을 관측한다."""
        if address in (0x4a8660,0x48fc60,0x472770):
            self.body_event=True
            if address==0x48fc60:
                self.event('technology')
                self.number(WIDTH,1920)  # 조회 효과 뒤 현재 폭을 바꿔 원문의 앞선 보관과 구별한다.
                self.returned(self.technology)
            else:
                count=3 if address==0x4a8660 else 6
                sp=mu.reg_read(UC_X86_REG_ESP)
                self.event('smoke' if count==3 else 'lightning',tuple(self.number(sp+4*(index+1)) for index in range(count)))
                self.returned()
            return
        super().instruction(mu,address,size,data)


def main():
    """현재 사건576개를 세 정밀도/반복으로 읽고 새 디렉터리에만 관측을 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle()
    seed=cases()[0]
    rows=[]
    observations=[]
    for index,((name,code),flags,second,width,technology,mode) in enumerate(itertools.product(COMMANDS,(0,0x80000000,0x38000000,0xb8000000),(0,6008,0xffffffff),(1024,1280),(0,350),range(4))):
        values=list(map(int,seed['globals'].split(':')))  # Edit/미션을 독립 비트로 넣고 Console/정지는 비활성이다.
        values[0],values[3]=mode&1,(mode>>1)&1
        values[8]=values[9]=0
        event=code|flags
        case=seed.copy()
        case.update(name=f'{name}-{index}',event=f'{event}:305419896:{second}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        observed=[]
        for control in (0x27f,0x37f,0x27f):  # 입력만 다시 설치하며 제품 출력을 기대값에 사용하지 않는다.
            oracle.technology=technology  # 명시 실제 기술 조회의 반환 DWORD다.
            oracle.number(WIDTH,width)
            observed.append(oracle.run(case,control))
        assert observed[0]==observed[1]==observed[2]
        result=observed[0]
        rows.append((name,event,second,width,technology,mode,result['bodytrace'],result['completed'],result['stackdelta']))
        observations.append(dict(input=case,width=width,technology=technology,observations=observed))
    schema=('name','event','second','width','technology','mode','trace','completed','stackdelta')
    fixture=out/'scenario-effect-input-x86.tsv'
    fixture.write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    (out/'observations.json').write_text(json.dumps(observations,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=688,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,precisionOrRepeatDifferences=0,fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),boundaries='실제004d6e61..004da477의Guide/Gump/일반 입력. 4a8660 일반 연기 생성/48fc60 기술 조회/472770 번개 생성만 명시 경계다. 기술 조회 때 화면 폭을 바꾸어 선행 폭 보관도 대조한다. CreateRandomGuns는 이 일반 입력에서 효과0이다. 실제 생성 부모/게임/OS/통신/공격은 실행하지 않는다.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','tools/decomp_scenario_effect_input.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=len(rows),observations=len(rows)*3,differences=0)))


if __name__=='__main__':
    main()
