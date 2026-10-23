#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본004cbf10 Players목록부모전체와실제48f550/CRT좌표절삭을독립관측한다."""
import itertools
import json
import socket
import struct
from pathlib import Path

from decomp_game_menus import Oracle as MenusOracle, ROOT, PE, digest, BASE, HEAP, STACK, STOP, CONTROLS, FIELDS, PLAYER_BASE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/player-list-menu-x86.tsv'  # 새실제Players부모관측이다.
REPORT=ROOT/'extracted/cpp-player-list-menu-20261023-native.json'  # 범위/한계/해시다.


class Oracle(MenusOracle):
    """현재Player원시바이트와큰하위/GUI만명시장치로제공한다."""
    def prepare(self):
        """생성부모가읽을현재Player9와관전BYTE를별도로초기화한다."""
        self.word(STACK+4,self.case['editing']);self.word(0x563E38,0x01001234)
        for owner in range(9):  # 원본Player0도물리적으로존재하나목록은1..8만방문한다.
            address=PLAYER_BASE+owner*180;active=(self.case['config']>>(owner-1))&1 if owner else 0
            self.word(address+0x2C,owner*100 if active else 0)
            self.word(address+0x50,(1 if owner%2 else 0x20)|(8 if owner%3 == 0 else 0))
            self.mu.mem_write(address+0xC,f'Player{owner}'.encode('ascii')+b'\0')
            self.mu.mem_write(0x52F81E+owner,bytes([(self.case['sound']>>owner)&1]))

    def begin(self,kind):
        """실제GUI생성의현재크기입력만노드에기록한다."""
        super().begin(kind)
        node=self.stack[-1];self.mu.mem_write(node+0x3C,struct.pack('<ff',130.5,201.25))

    def instruction(self,mu,address,size,data):
        """문자열연결/목록/숨김/위치계산은원문을실행하고큰하위만대체한다."""
        if 0x48F550 <= address < 0x48F5C0 or 0x4E49C0 <= address < 0x4E4A40:
            self.instructions+=1;return
        esp=mu.reg_read(UC_X86_REG_ESP);arg=lambda index:self.word(esp+4+index*4)  # 실제CALL인자다.
        if address == 0x477EE0:
            self.event('TITLE',self.text(arg(0)).hex());self.returned()
        elif address == 0x495060:
            owner=arg(0);self.event('RF',owner)
            if self.case['mutation']&16:
                location=PLAYER_BASE+owner*180+0x50;self.word(location,self.word(location)^0x21)
            self.returned()
        elif address == 0x41C5B0:
            self.event('RANK',arg(0));self.returned(arg(0)+3,4)
        elif address == 0x42F480:
            rank=arg(1);self.event('PIC',rank);self.mu.mem_write(arg(0),f'~[Iicon.B{rank}]'.encode('ascii')+b'\0');self.returned(arg(0))
        elif address == 0x42B210:
            owner=(mu.reg_read(UC_X86_REG_ECX)-0x6C-PLAYER_BASE)//180;self.event('LEVEL',owner)
            if self.case['mutation']&32:
                self.word(0x594FA4,1)
            self.returned(0x100+owner+4)
        elif address == 0x4A89C0:
            pattern=self.text(arg(1));value=arg(2);self.event('FMT',pattern.hex(),value);self.mu.mem_write(arg(0),pattern.replace(b'%d',str(value).encode('ascii'))+b'\0');self.returned(arg(0))
        elif address == 0x4C9700:
            self.event('PM',arg(0));self.returned(200+arg(0)//100)
        elif address == 0x475C40:
            self.event('N');self.returned(self.stack[-1])
        elif address == 0x475CA0:
            self.event('A',arg(0),arg(1),arg(2));self.stack.pop();self.returned()
        else:
            super().instruction(mu,address,size,data)


def main():
    """현재메타맵/미션/Player/관전/부착/재진입입력을두정밀도로관측한다."""
    if digest(PE) != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('보호PE식별자불일치')
    oracle,rows,cases=Oracle(),[],[]  # 실제관측과명시입력목록이다.
    for missionpost,meta,fancy,active,hidden,mutation in itertools.product((0,1),(0,1),(0,1),(0,1,0x55,0xAA,0xFF),(0,0x1FE),(0,16,32)):  # 빈/교차/전체목록과각종현재모드를교차한다.
        cases.append(dict(zip(FIELDS,(4,0,1,missionpost,0,meta,1,fancy,0,0,1,1024,768,0,1,hidden,4,0,active,mutation))))
    for width,height in itertools.product((0,1,641,1280,0xFFFFFFFF),(0,1,481,720,0xFFFFFFFF)):  # 원본signed정수반과실수폭/높이절삭의경계다.
        cases.append(dict(zip(FIELDS,(4,0,1,0,0,0,1,1,0,0,1,width,height,0,1,0x1FE,4,0,0xFF,0))))
    for index,case in enumerate(cases):  # 모든행은같은보호원문을두정밀도로실행한다.
        previous=None
        for control in CONTROLS:
            try:
                observed=oracle.run(case,control)
            except Exception as error:
                raise RuntimeError(f'{index}/{case}: {error}') from error
            if previous is not None and previous != observed:
                raise RuntimeError('정밀도차이')
            previous=observed;rows.append(f'{index}-cw{control:03x}\t'+','.join(str(case[name]) for name in FIELDS)+'\t'+observed)
    FIXTURE.write_text('# name\tinput\toutput\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastDecompileHost':'VM-W11-Codex / 2026-10-23','inputs':len(cases),'observations':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'scope':'004cbf10목록전체와실제Player번호48f550/CRT_ftol4e49c0을실행한다. 현재Player9/관전BYTE/GUI/번역/사제갱신/랭크아이콘/레벨/서식/Player메뉴는명시입력과하위경계다.','limits':['각Player하위004c9700의몸체검증은기존Player메뉴fixture의별도범위다.','사제/랭크/레벨을실제자연게임에서계산하거나실제GUI그림/메뉴명령을실행한증거가아니다.'],'gameExecuted':False,'guiExecuted':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,Path(__file__),ROOT/'tools/decomp_game_menus.py',ROOT/'tools/decomp_game_menu_bar.py',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE)}}  # 범위와식별자다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('inputs','observations','precisionDifferences')}))


if __name__ == '__main__':
    main()
