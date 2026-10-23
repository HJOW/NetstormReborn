#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 Game/Edit/View/Options 네 부모를 실제 PE에서 실행해 독립 관측한다."""
import itertools
import json
import socket
from pathlib import Path

from decomp_game_menu_bar import Oracle as BarOracle, BASE, HEAP, STACK, STOP, CONTROLS, ROOT, PE, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,
    UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

FIXTURE = ROOT/'cpppj/tests/fixtures/game-menus-x86.tsv'  # 새 부모의 실제 관측 자료다.
REPORT = ROOT/'extracted/cpp-game-menus-20261023-native.json'  # 범위와 해시다.
ENTRIES = ((0x4CBB80,0x4CBF04),(0x4CB9C0,0x4CBAAC),(0x4CA4D0,0x4CA760),(0x4CA850,0x4CAFD0),(0x4CBF10,0x4CC20C))  # 네부모와별도Players관측도구가빌리는원문범위다.
FIELDS = ('kind','editor','battle','missionpost','fort','meta','mission','editing','demo','skip','known','width','height','dd','audio','sound','music','pause','config','mutation')  # 기대를 포함하지 않는 입력이다.
ADDRESSES = {'editor':0x5C85A4,'battle':0x594FBC,'missionpost':0x594FA4,'fort':0x594FB8,'meta':0x594FC8,'mission':0x594FA0,'skip':0x59507C,'known':0x595344,'width':0x531860,'height':0x531864,'dd':0x5C78D4,'audio':0x5C7B14,'sound':0x531900,'music':0x531904,'pause':0x55B4B0}  # 실제 독립 전역이다.
CONFIG_KEYS = ('theammode','useBanners','sound','music','playWind','swapLeftRightSpeakers','edgeScroll','cursorSnapToGrid','autoDemo','tellTips','rotmode','passedDiags')  # 해당 부모의 실제 설정철자다.
PLAYER_BASE = HEAP+0x6000  # 같은9×180BYTE의 명시 입력 저장소다.
TYPE = HEAP+0x7000  # 현재 신전 TYPE+c4 비용을 공급한다.


class Oracle(BarOracle):
    """원문 분기/문자열/정수 계산을 실행하고 큰 하위와 GUI만 명시한다."""
    def begin(self,kind):
        """명시 GUI 생성의 현재 controlled 칸만 공급한다."""
        self.event(kind);self.created+=1;token=HEAP+0x1000+self.created*0x100
        self.stack.append(token);self.returned(token)

    def parameter(self,value):
        """원본 문자열 포인터 의미와 숫자 인자를 구별한다."""
        return 's'+self.text(value).hex() if BASE <= value < BASE+len(self.image) else 'n'+str(value)

    def config(self,key):
        """명시 설정 마스크와 재진입 입력만 읽는다."""
        index=CONFIG_KEYS.index(key);self.reads[key]=self.reads.get(key,0)+1
        value=(self.case['config']>>index)&1
        if self.case['mutation']&4:
            value^=(self.reads[key]-1)&1
        return value

    def instruction(self,mu,address,size,data):
        """실제 부모와 작고 순수한 현재 getter만 원문 실행한다."""
        if address == STOP:
            self.completed=True;mu.emu_stop();return
        entry,end=ENTRIES[self.case['kind']]
        if entry <= address < end:
            self.instructions+=1;return
        esp=mu.reg_read(UC_X86_REG_ESP);arg=lambda index:self.word(esp+4+index*4)  # 실제CALL의인자다.
        if address == 0x477BB0:
            self.begin('V')
        elif address == 0x477AE0:
            self.begin('F')
        elif address == 0x4DE9A0:
            text=self.text(arg(0));self.event('T',text.hex())
            if self.case['mutation']&1 and text == b'Hide/Show chat - ~iF3~.':
                self.word(0x594FA4,self.word(0x594FA4)^1)
            if self.case['mutation']&2 and text == b'Direct Draw / Full Screen':
                self.word(0x5C7910,self.word(0x5C7910)^1)
            if self.case['mutation']&8 and text == b'View Your Priest - ~iF5~.':
                self.word(0x540C70,2)
            self.returned(arg(0),4)
        elif address == 0x477CB0:
            self.event('I',self.text(arg(0)).hex(),arg(1),self.parameter(arg(2)),arg(3),arg(4),arg(5));self.returned()
        elif address == 0x477DD0:
            self.event('K',arg(0),self.text(arg(1)).hex(),arg(2),self.parameter(arg(3)),arg(4),arg(5));self.returned()
        elif address == 0x478160:
            self.event('S',self.text(arg(0)).hex(),arg(1),arg(2),arg(3));self.returned()
        elif address == 0x478260:
            self.event('X');self.returned()
        elif address == 0x475C00:
            self.event('Q');self.returned(self.stack.pop())
        elif address == 0x441090:
            key=self.text(arg(0)).decode('ascii');self.event('G',key);self.returned(self.config(key))
        elif address == 0x441270:
            key=self.text(arg(0)).decode('ascii');self.event('R',key);self.word(arg(1),self.config(key));self.returned(1)
        elif address == 0x4639E0:
            self.event('E');self.returned(self.case['editing'])
        elif address == 0x463A20:
            self.event('D');self.returned(self.case['demo'])
        elif address == 0x48F7E0:
            self.returned(self.word(mu.reg_read(UC_X86_REG_ECX)+0x54)&0x400)
        elif address == 0x490970:
            self.event('L');self.returned(2)
        elif address == 0x4921F0:
            self.event('P',arg(0));self.returned(99)
        elif address == 0x40EC50:
            self.event('M',arg(0));self.word(arg(1),0x461C4000);self.returned()
        elif address == 0x49A840:
            self.event('COST');self.returned(TYPE)
        elif address == 0x492090:
            self.event('B',arg(0));self.returned(0)
        elif address == 0x448FD0:
            self.event('RI');self.returned(0)
        elif address == 0x4490D0:
            self.event('RO',arg(0));self.returned(0)
        elif address == 0x4922E0:
            self.event('CA',arg(0));self.returned(0)
        elif address == 0x460DE0:
            self.event('PA');self.returned(int(0 < (self.word(0x55B4B0)^0x80000000)-0x80000000))
        else:
            raise RuntimeError(f'미명시 원문 실행 {address:08x}')

    def run(self,case,control):
        """새 현재전역을 준비하고 실제반환토큰/controlled수명/경계순서를읽는다."""
        self.case,self.trace,self.instructions,self.created,self.stack,self.reads=case,[],0,0,[],{}
        self.completed=False
        for name,address in ADDRESSES.items():  # 기대없이원문독립전역을초기화한다.
            self.word(address,case[name])
        self.word(0x5C7910,1);self.word(0x59A8AC,1);self.word(0x540C70,1);self.word(0x59534C,PLAYER_BASE)
        self.word(TYPE+0xC4,0x44BB8000)
        self.mu.mem_write(PLAYER_BASE,bytes(9*180))
        self.mu.mem_write(STACK-0x8000,bytes(0x10000));self.word(STACK,STOP)
        self.prepare()
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 원본문체가빌리는일반레지스터입력이다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        entry,end=ENTRIES[case['kind']];self.mu.emu_start(entry,STOP+1,count=20000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4 or self.stack:
            raise RuntimeError('원문반환/스택/controlled수명불일치')
        if bytes(self.mu.mem_read(entry,end-entry)) != self.image[entry-BASE:end-BASE]:
            raise RuntimeError('보호원문변경')
        total=self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0});total['runs']+=1;total['instructions']+=self.instructions
        return str(self.mu.reg_read(UC_X86_REG_EAX))+'|'+';'.join(self.trace)

    def prepare(self):
        """별도Players도구의명시Player입력을위한추가초기화경계다."""
        return None


def cases():
    """네부모의현재모드/설정과재진입입력을기대없이구성한다."""
    rows,seen=[],set()  # 중복입력은한번만실행한다.
    def add(**changes):
        """명시기본환경에독립입력만추가한다."""
        case=dict(zip(FIELDS,(0,0,1,1,0,0,1,0,0,0,1,1024,768,0,1,3,4,0,0,0)));case.update(changes)
        key=tuple(case[name] for name in FIELDS)
        if key not in seen:
            seen.add(key);rows.append(case)
    for kind,editor,battle,missionpost,fort,meta,mission in itertools.product(range(4),(0,1),(0,1),(0,1),(0,1),(0,1),(0,1)):  # 각분기조건을전체교차한다.
        add(kind=kind,editor=editor,battle=battle,missionpost=missionpost,fort=fort,meta=meta,mission=mission)
    for kind,config,mutation in itertools.product(range(4),(0,0xFFF,0x555,0xAAA),(1,2,4,8,15)):  # 설정반전과번역후현재전역재조회를관측한다.
        add(kind=kind,config=config,mutation=mutation)
    for dd,audio,pause,width,height in itertools.product((0,1),(0,1),(0,1,0xFFFFFFFF),(640,800,1024),(480,600,768)):  # Options의음향/모드/정지/해상도체크다.
        add(kind=3,dd=dd,audio=audio,pause=pause,width=width,height=height,config=0xFFF)
    for editing,demo,skip,known in itertools.product((0,1),(0,1),(0,1),(0,1)):  # Game의실제미션별개필드를교차한다.
        add(kind=0,editing=editing,demo=demo,skip=skip,known=known)
    return rows


def main():
    """기대목록을작성하지않고네실제부모의두정밀도관측만저장한다."""
    if digest(PE) != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('보호PE식별자불일치')
    oracle,rows=Oracle(),[];inputs=cases()  # 실제실행기와독립입력이다.
    for index,case in enumerate(inputs):  # 두정밀도반환/전체호출순서를비교한다.
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
    report={'host':socket.gethostname(),'lastDecompileHost':'VM-W11-Codex / 2026-10-23','inputs':len(inputs),'observations':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'scope':'Game004cbb80/Edit004cb9c0/View004ca4d0/Options004ca850의전체부모. GUI/번역/Config와큰SP·사제·로그인·의식·현재미션getter는명시경계다.','limits':['Players004cbf10/ESC/실제메뉴그림/명령소비/실제게임GUI는이관측범위가아니다.','멀티플레이leave의helper결과는고정명시입력이고전체네트워크/의식본체의검증이아니다.'],'gameExecuted':False,'guiExecuted':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,Path(__file__),ROOT/'tools/decomp_game_menu_bar.py',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE)}}  # 원문범위/한계/식별자다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('inputs','observations','precisionDifferences')}))


if __name__ == '__main__':
    main()
