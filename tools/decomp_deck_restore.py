#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Deck Open/복원Append/FullDeck과실제하위Append·Entry·공유난수를독립고정한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TYPES, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

DECK, RANDOM, TYPE_COUNT, EXCLUDED_A, EXCLUDED_B = POOL+0x1000,0x532710,0x541348,0x5412c0,0x541240  # 실제Deck352BYTE/같은공유난수/현재타입수/두제외타입이다.
ENTRIES=(0x44f250,0x44f330,0x44f680,0x44f290,0x44f160,0x4558c0)  # 여섯실제몸체만실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/deck-restore-x86.tsv'  # 제품전에최초고정한다.
REPORT=ROOT/'cpppj/deck-restore-native.json'  # 몸체/실제하위범위와명시TYPE/진단경계를구별한다.


def cases():
    """DWORD절단/현재count/현재TYPE와부분중단입력만만든다."""
    rows=[]
    for state,seed in itertools.product((0,1,0xffffffff,0x80000000,0x12345678),(0,1,0xffffffff)):rows.append((0,state,seed,0,0,0,0,4,6,0,2,4,'none'))  # Open의0seed만공유난수를쓴다.
    for typ,chance,power,remaining in itertools.product((0,71,255,256,0xffffffff),(0,255,256,0xffffffff),(0,127,128,255,0xffffff80),(0,255,256)):rows.append((1,1,0,typ,chance,power,remaining,4,6,0,2,4,'none'))  # 네인자를미리BYTE로자르지않는다.
    for count,typ in itertools.product((0,1,4,65,66,69),(0,71,255)):rows.append((1,1,0,typ,17,1,255,count,6,0,2,4,'none'))  # metadata겹침도352BYTE물리범위면보존한다.
    for profile,count,a,b in itertools.product(range(3),(-1,0,1,6,66,67),(2,0xffffffff),(4,0xffffffff)):rows.append((2,0x12345678,0,0,0,0,0,4,count,profile,a,b,'none'))  # 실제Full의두TYPE조회와제외두타입을교차한다.
    rows.extend(((2,1,0,0,0,0,0,4,260,0,255,256,'none'),(2,1,0,0,0,0,0,4,260,0,0xffffffff,0xffffffff,'none')))
    for change in ('assert1-count0','assert1-count1','assert1-sum1234','assert1-power0','assert1-chance0','assert1-stop','assert2-stop','assert3-stop','assert4-stop','assert5-stop'):  # 중복/필드절단진단뒤현재메모리재조회를관측한다.
        rows.append((1,1,0,71,256,128,256,4,6,0,2,4,change))
    for change in ('assert1-count0','assert1-stop'):rows.append((1,1,0,256,17,1,255,66,6,0,2,4,change))  # capacity진단뒤실제현재count를다시읽는다.
    for change in ('type1-count0','type1-count6','type1-group9','type1-flags256','type1-excluded0','type1-random0','type1-stop','type2-count0','type2-group9','type2-stop','type3-stop'):
        rows.append((2,1,0,0,0,0,0,4,6,0,2,4,change))
    return list(dict.fromkeys(rows))


def initial_deck(count):
    """전체352BYTE의원시초기항목과다섯DWORD를고정입력으로준비한다."""
    raw=bytearray([0xa7])*352
    for index in range(66):raw[index*5:index*5+5]=bytes(((0,71,158,255)[index%4],index%7+1,(255,0,1,128,127)[index%5],37,index&255))  # 현재tail도보존대상이다.
    struct.pack_into('<5I',raw,332,count,0x12345678,0x23456789,0x3456789a,0x456789ab)
    return bytes(raw)


class Oracle(MemoryOracle):
    """Open/두Append/Entry/Full/난수는실제명령이고TYPE주소/진단만명시장치다."""
    def __init__(self):
        """현재PC함수표에서정확한여섯범위를읽고실제쓰기영역을제한한다."""
        super().__init__();self.ranges=[];self.bodies={};self.total=self.normal=self.stops=0;self.boundaries=Counter()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf8') as source:
            for row in csv.DictReader(source,delimiter='\t'):  # 최초근거의실제명령범위만허용한다.
                address=int(row['entry'],16)
                if address not in ENTRIES:continue
                self.bodies[f'{address:08x}']=row['ranges']
                for part in row['ranges'].split(';'):low,high=(int(value,16) for value in part.split('-'));self.ranges.append((low,high+1))  # 포함끝을반열린범위로옮긴다.
        if len(self.bodies)!=len(ENTRIES):raise RuntimeError('Deckprimitive몸체누락')
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """원문쓰기만Deck352BYTE/공유난수/격리스택에허용한다."""
        if STACK-256<=address and address+size<=STACK+24:return
        if DECK<=address and address+size<=DECK+352:return
        if address==RANDOM and size==4:return
        raise RuntimeError(f'Deckprimitive물리쓰기밖{address:08x}/{size}')

    def mutate(self,boundary):
        """외부진단/TYPEcallback의명시현재값변경만공급한다."""
        change=self.case[-1]
        if '-' not in change:return
        trigger,action=change.split('-',1)
        if trigger!=boundary:return
        if action=='stop':self.stopped=True;self.mu.emu_stop()
        elif action.startswith('count'):self.put(TYPE_COUNT if boundary.startswith('type') else DECK+332,int(action[5:]))
        elif action.startswith('sum'):self.put(DECK+336,int(action[3:]))
        elif action=='power0':self.put(self.entry+2,0,1)
        elif action=='chance0':self.put(self.entry+1,0,1)
        elif action=='group9':self.put(self.current_type+0x9c,9)
        elif action=='flags256':self.put(self.current_type+0xec,256)
        elif action=='excluded0':self.put(EXCLUDED_A,0)
        elif action=='random0':self.put(RANDOM,0)
        else:raise RuntimeError('Deckprimitive명시변경미해석:'+action)

    def instruction(self,mu,address,size,data):
        """검토된실제몸체와두외부경계밖으로분기하지않는다."""
        self.instructions+=1;self.total+=1
        if self.instructions>100000:raise RuntimeError('Deckprimitive명령상한')
        if address==0x44f160:self.entry=mu.reg_read(UC_X86_REG_ECX)
        if any(low<=address<high for low,high in self.ranges):return
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4e0620:
            line=self.words(esp+4,3)[2];self.calls['assert']+=1;self.boundaries['assert']+=1;self.trace.append('A:'+str(line));self.mutate('assert'+str(self.calls['assert']))
            if not self.stopped:self.ret()
        elif address==0x49a840:
            index=self.words(esp+4,1)[0]
            if index>=260:raise RuntimeError('DeckprimitiveTYPE입력물리범위')
            self.current_type=TYPES+index*500;self.calls['type']+=1;self.boundaries['type']+=1;self.trace.append('T:'+str(index));self.mutate('type'+str(self.calls['type']))
            if not self.stopped:self.ret(self.current_type)
        else:raise RuntimeError(f'Deckprimitive검토밖명령{address:08x}')

    def sample(self,case,control):
        """정상/중단의전체352BYTE와공유난수/현재TYPE전역을독립관측한다."""
        self.case,self.trace,self.calls=case,[],Counter();self.instructions=0;self.stopped=False;self.entry=DECK;self.current_type=TYPES
        self.mu.mem_write(DECK,initial_deck(case[7]));self.put(RANDOM,case[1]);self.put(TYPE_COUNT,case[8]);self.put(EXCLUDED_A,case[10]);self.put(EXCLUDED_B,case[11])
        self.mu.mem_write(TYPES,bytes(260*500))
        for index in range(260):  # TYPE순수외부입력만준비하고선택기대는계산하지않는다.
            self.put(TYPES+index*500+0x9c,10 if case[9]!=1 or index%2 else 9);self.put(TYPES+index*500+0xec,256 if case[9]==2 else 0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)  # 독립CPU상태다.
        self.mu.reg_write(UC_X86_REG_ECX,DECK);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        arguments=(case[2],) if case[0]==0 else case[3:7] if case[0]==1 else ()
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(arguments)+1),STOP,*arguments));self.mu.emu_start(ENTRIES[case[0]],STOP)
        completed=not self.stopped;self.normal+=int(completed);self.stops+=int(not completed)
        if completed and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(arguments)*4:raise RuntimeError('Deckprimitive스택복구차이')
        state=(self.words(RANDOM,1)[0],self.words(TYPE_COUNT,1)[0],self.words(EXCLUDED_A,1)[0],self.words(EXCLUDED_B,1)[0])
        return '\t'.join((str(control),','.join(map(str,case)),str(int(completed)),','.join(map(str,state)),bytes(self.mu.mem_read(DECK,352)).hex(),';'.join(self.trace) or '-'))


def main():
    """제품파일을읽지않고최초fixture를만들거나근거SHA만검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf8'))
        for name,expected in record['sha256'].items():  # 이미고정된원문을제품기대로덮어쓰지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('Deckprimitive최초SHA불일치:'+name)
        print('Deckprimitive최초SHA확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('Deckprimitive최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / kind,random,seed,type,chance,power,remaining,count,typeCount,profile,excludedA,excludedB,mutation / completed / random,typeCount,excludedA,excludedB / 전체Deck352BYTE / trace']
    for case in inputs:  # 두정밀도를각각실제실행해관측차이를검사한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('Deckprimitive정밀도별차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_deck_restore.py','cpppj/tests/fixtures/deck-restore-x86.tsv')
    record={'status':'최초primitive원문고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','ranges':oracle.bodies,'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'explicitStops':oracle.stops,'instructions':oracle.total,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'scope':'실제Open44f250/RestoreAppend44f330/FullDeck44f680과도달하는Append44f290/Entry44f160/공유난수4558c0여섯몸체다. TYPE주소조회/진단만명시경계다.352BYTE물리범위내metadata겹침·count재조회·전인자DWORD/저장BYTE/진단뒤현재필드를비교한다. 부모/실제Client/OS/GUI/게임실행은별도다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalReturns','explicitStops','instructions')}))


if __name__=='__main__':main()
