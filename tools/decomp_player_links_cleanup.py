#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""0048fcd0의79BYTE와선택free경계를제품전독립PE로고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_INVALID, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 현재Player물리두벌과free의실제callee진입만별도장치로구별한다.
BASE, OWNER, BLOCKS, FREE, REENTER = 0x59534c,0x540c70,(PLAYERS,PLAYERS+0x1000),0x4e4354,STOP+0x100
FIXTURE=ROOT/'cpppj/tests/fixtures/player-links-cleanup-x86.tsv'
REPORT=ROOT/'extracted/cpp-player-links-cleanup-20261024-native.json'


def cases():
    """초기Player전체/동적포인터mask/명시free변경만준비한다."""
    rows=[(seed,a,b,current,0,'none') for seed,a,b,current in itertools.product((0,165),(0,1,0x155,0x100,0x1ff),(0,0x100,0x1ff),(0,1))]
    for mutation in ('f1-baseB','f2-baseB','f9-baseB','f1-baseA','f1-null','f1-owner8','f1-pointerA','f1-nextA','f1-reenter'):
        rows.append((165,0x1ff,0x1ff,1 if mutation=='f1-baseA' else 0,0,mutation))
    for stop in (1,2,5,9,10):
        rows.append((165,0x1ff,0x1ff,0,stop,'none'));rows.append((165,0x1ff,0,0,stop,'f1-reenter'))
    rows.extend(((0,0,0,2,0,'none'),(165,0x1ff,0x1ff,2,0,'none')))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """원문부모전체를CPU로수행하고CRTfree만명시토큰장치로둔다."""
    def __init__(self):
        """두Player할당은freecallback뒤에도물리를유지하며포인터해제대상과분리한다."""
        super().__init__();self.running=False;self.total=0;self.status='normal';self.free_total=0
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid)

    def current(self):
        """현재Player주소를0/1/NULL2로만정규화한다."""
        value=self.words(BASE,1)[0];return 2 if value==0 else BLOCKS.index(value)

    def read(self,mu,access,address,size,value,data):
        """각store/load직전실제현재base를재조회한횟수/순서를그대로기록한다."""
        if self.running and address==BASE:self.trace.append('R:'+str(self.current()))

    def invalid(self,mu,access,address,size,value,data):
        """NULL현재base의실제접근만명시물리오류로기록한다."""
        if self.running and address<1620:self.status='fault-null';mu.emu_stop();return False
        raise RuntimeError('Player링크예상밖미매핑:'+hex(address))

    def instruction(self,mu,address,size,data):
        """free의입력인자/선택중단과실제부모재진입만외부효과로제공한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>5000:raise RuntimeError('Player링크명령상한')
        if 0x48fcd0<=address<0x48fcd8 or 0x48fce0<=address<0x48fd27:return
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==REENTER:
            saved=self.reentry.pop();mu.reg_write(UC_X86_REG_ESP,saved);self.trace.append('RE:end');self.ret();return
        if address!=FREE:raise RuntimeError('Player링크관측밖명령:'+hex(address))
        token=self.words(esp+4,1)[0];self.calls+=1;self.free_total+=1;self.trace.append('F:'+str(token));self.released.append(token)
        if self.case[4]==self.calls:self.status='stop';mu.emu_stop();return
        boundary='f'+str(self.calls);trigger,_,action=self.case[5].partition('-')
        if trigger==boundary:
            if action=='baseB':self.put(BASE,BLOCKS[1])
            elif action=='baseA':self.put(BASE,BLOCKS[0])
            elif action=='null':self.put(BASE,0)
            elif action=='owner8':self.put(OWNER,8)
            elif action=='pointerA':self.put(BLOCKS[0]+0x68,0x21234567)
            elif action=='nextA':self.put(BLOCKS[0]+180+0x68,0)
            elif action=='reenter':
                self.trace.append('RE:begin');self.reentry.append(esp);mu.reg_write(UC_X86_REG_ESP,esp-4);self.put(esp-4,REENTER);mu.reg_write(UC_X86_REG_EIP,0x48fcd0);return
            else:raise RuntimeError('Player링크미해석변경:'+action)
        self.ret()

    def sample(self,case,control):
        """명시초기전체물리와부모의최종부분상태/trace만기대로기록한다."""
        self.case=case;seed,mask_a,mask_b,current,stop,mutation=case;self.running=False;self.trace=[];self.released=[];self.reentry=[];self.calls=0;self.status='normal';self.instructions=0
        for block,base in enumerate(BLOCKS):  # 각동적토큰은현재base포인터와별개opaque식별자다.
            self.mu.mem_write(base,bytes((seed+block*53+index*17)&255 for index in range(1620)))
            for owner in range(9):self.put(base+owner*180+0x68,0x21000000+block*0x1000+owner*0x10 if (mask_a if block==0 else mask_b)&(1<<owner) else 0)
        self.put(BASE,0 if current==2 else BLOCKS[current]);self.put(OWNER,7);self.mu.mem_write(STACK-8192,bytes(8224))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.put(STACK,STOP);self.running=True
        try:self.mu.emu_start(0x48fcd0,STOP)
        except UcError:
            if self.status!='fault-null':raise
        self.running=False
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Player링크스택복귀')
        memory=b''.join(bytes(self.mu.mem_read(base,1620)) for base in BLOCKS)
        return '\t'.join((str(control),','.join(map(str,case)),self.status,str(self.current())+','+str(self.words(OWNER,1)[0]),memory.hex(),','.join(map(str,self.released)) or '-', ';'.join(self.trace),str(self.instructions)))


def main():
    """최초제품전결과를한번만쓰고후속호출은SHA불변만확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():assert digest(ROOT/name)==expected,name
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    assert not FIXTURE.exists() and not REPORT.exists(),'Player링크최초자료덮어쓰기금지'
    for name in ('cpppj/src/o/RawPlayerLinksCleanup.h','cpppj/src/o/RawPlayerLinksCleanup.cpp','cpppj/tests/PlayerLinksCleanupTests.cpp'):assert not (ROOT/name).exists(),'Player링크제품전관측위반'
    inputs=cases();oracle=Oracle();lines=['# control / seed,maskA,maskB,currentBase,freeStopIndex,mutation / status / currentBase,unchangedOwner / twoPlayer1620 / releasedOpaqueTokens / currentBaseReadAndFreeTrace / instructions'];statuses=Counter()
    for case in inputs:  # 정상/외부중단/NULL/재진입의양모드전체를같이보존한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f);assert first.split('\t')[1:]==second.split('\t')[1:]
        for line in (first,second):lines.append(line);statuses[line.split('\t')[2]]+=1
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_player_links_cleanup.py','cpppj/tests/fixtures/player-links-cleanup-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','sourceAbsentBeforeObservation':True,'parentAddress':'0048fcd0','parentRanges':'48fcd0-48fcd7;48fce0-48fd26','parentBytes':79,'cdSourceMap':'0046dfb0','uniqueInputs':len(inputs),'observations':len(lines)-1,'statuses':dict(statuses),'instructions':oracle.total,'selectedFreeCalls':oracle.free_total,'precisionDifferences':0,'gameOrOsExecuted':False,'scope':'실제9Player0..8의현재base매접근재조회와+9c/+8/비0+68 free→현재base+68zero. _free는실제토큰반납을선택하는명시외부경계다.재진입중복free를안전하다고판정하지않는다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'observations':report['observations'],'statuses':dict(statuses),'instructions':oracle.total,'freeCalls':oracle.free_total}))


if __name__=='__main__':main()
