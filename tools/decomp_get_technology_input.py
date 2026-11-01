#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GetTechnology의 실제 일반 입력/원문 thunk와 두 명령 이름표를 변경 전에 관측한다(683)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_game_menu_actions import Oracle as MenuOracle, cases, gate
from unicorn.x86_const import UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 이 저장소의 보호 PE만 출처로 사용한다.
SCHEMA = ('name','event','first','second','editor','mission','trace','completed','stackdelta')  # 앞6열 입력/뒤3열 원문 관측이다.


class Oracle(MenuOracle):
    """기존 Guide/Gump/일반 입력은 그대로 실행하고 기술 획득 부모만 명시 외부 경계로 둔다."""
    def __init__(self):
        """실제494ea0의 JMP5바이트도 보호/실행 범위에 포함한다."""
        super().__init__()
        self.original[0x494ea0] = bytes(self.mu.mem_read(0x494ea0,5))

    def instruction(self,mu,address,size,data):
        """실제 thunk를 실행하고492a30의 현재 DWORD 인자를 관측한다."""
        if address==0x494ea0:
            self.instructions+=1
            self.visited.add(address)
            return
        if address==0x492a30:
            self.body_event=True
            self.event('technology',(self.number(mu.reg_read(UC_X86_REG_ESP)+4),))
            self.returned()
            return
        super().instruction(mu,address,size,data)


def main():
    """이름표와48현재 사건의 정밀도/재관측을 동결하며 C++을 호출하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle()
    lookup=[]
    # 정적 이름 문자열/표/명령 전역을 읽으며 숫자 명령과 임의 별칭을 혼동하지 않는다.
    for name,address in (('GetTechnology',0x542970),('ChangeMoney',0x54287c)):
        raw=oracle.pe.__data__
        offset=raw.find(name.encode('ascii')+b'\0')
        assert offset>=0
        string=0x400000+oracle.pe.get_rva_from_offset(offset)
        entry=raw.find(struct.pack('<II',string,address))
        assert entry>=0
        command=struct.unpack('<I',oracle.pe.get_data(address-0x400000,4))[0]
        lookup.append(dict(name=name,string=f'{string:08x}',entry=f'{0x400000+oracle.pe.get_rva_from_offset(entry):08x}',globalAddress=f'{address:08x}',command=f'{command:08x}'))
    seed=cases()[0]
    rows=[]
    originals=[]
    # 원문은 타입 DWORD를 그대로 전달하며 고위 WORD를 자르거나 모드로 새 거부를 하지 않는다.
    for index,(kind,flags,mode) in enumerate(itertools.product((0,80,97,187,0xffff0001,0xffffffff),(0,0x80000000,0x38000000,0xb8000000),(0,1))):
        case=seed.copy()
        values=list(map(int,seed['globals'].split(':')))
        values[0]=mode
        values[3]=mode
        values[8]=values[9]=0
        event,first=0x0200009f|flags,0x12345678+index
        case.update(name=f'get-{index}',event=f'{event}:{first}:{kind}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        observed=[oracle.run(case,control) for control in (0x27f,0x37f,0x27f)]
        assert observed[0]==observed[1]==observed[2]
        result=observed[0]
        originals.append(dict(input=case,observations=observed))
        rows.append((case['name'],event,first,kind,mode,mode,result['bodytrace'],result['completed'],result['stackdelta']))
    fixture=out/'get-technology-input-x86.tsv'
    fixture.write_text('\t'.join(SCHEMA)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    (out/'observations.json').write_text(json.dumps(originals,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_get_technology_input.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')
    report=dict(unit=683,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,precisionOrRepeatDifferences=0,lookup=lookup,fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},boundaries='실제004d6e61..004da477/494ea0 thunk를 실행하고492a30 부모는 명시 장치 경계로 뒀다. 기존 부모45사례 fixture와 별도이며 새 타입 지급/포획/희생/승리·실제GUI/MP/CD본문 검사는 아니다. ChangeMoney는 이름표 정적 읽기만 했다.')
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'GetTechnology {len(rows)}입력/{len(rows)*3}관측 차이0, 원문 이름2개')


if __name__=='__main__':
    main()
