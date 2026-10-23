#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Deck로드004bf190의부모431BYTE와별도cleanup15BYTE를제품전에독립고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, DEPTH, CONVERSION, FULL = 0x5c89f0, 0x5c89b8, 0x5c89bc, 0x54db14  # 고정this/실제공유깊이/현재변환표/fullDeckDWORD다.
STREAMS, BUFFERS = (POOL+0x1000, POOL+0x1020), (POOL+0x2000, POOL+0x3000)  # 두명시선택반환과서로다른실제2048BYTE입력이다.
MANAGERS, DECKS, TABLES = {0:0,11:POOL+0x100,22:POOL+0x200}, {0:0,101:POOL+0x5000,202:POOL+0x5200}, {0:0,11:POOL+0x6000,22:POOL+0x6200}  # 포인터절단없는관측용ID다.
RANGES = ((0x4bf190,0x4bf33f),(0x4fb860,0x4fb868),(0x4bcc50,0x4bcc57))  # 본문과별도cleanup명령이며상위SEH분배기는실행하지않는다.
FIXTURE = ROOT/'cpppj/tests/fixtures/fort-deck-load-x86.tsv'  # 제품구현전에최초고정하고덮어쓰지않는다.
REPORT = ROOT/'cpppj/fort-deck-load-native.json'  # 부모와아직별도관측할primitive를구별한보고서다.


def cases():
    """고정slot/빈stream/fullDeck/현재conversion/callback변경의원시입력만만든다."""
    rows=[]
    for slot,manager,deck,full,data,end in itertools.product((-2,0,1,8,9),(0,11),(0,101),(0,1,0x80000000),(0,1),(0,1,0xffffffff)):  # ownerclamp/진단과빈stream을교차한다.
        rows.append((slot,manager,deck,0,full,data,end,0,0,11,3,'none'))
    for profile,conversion,selected,position,depth in itertools.product(range(4),(0,11,22),(0,1),(0,512),(0,0xffffffff)):  # BYTE255와부호없는저장필드를포함한직접읽기입력이다.
        rows.append((1,11,101,selected,0,1,1,position,profile,conversion,depth,'none'))
    changes=('assert1-manager22','assert1-full1','assert1-depth7','assert1-stop','assert2-manager22','assert2-stop',
             'get-manager22','get-full1','get-conversion22','get-stop','select-stream1','select-manager22','select-full1','select-stop',
             'clear-full1','clear-dataB','clear-pos512','clear-depth7','clear-stop','open-full1','open-full0','open-conversion22','open-dataB','open-pos512','open-end0','open-depth7','open-stop',
             'prepare-manager22','prepare-full1','prepare-conversion0','prepare-conversion22','prepare-dataB','prepare-pos512','prepare-end0','prepare-depth7','prepare-stop',
             'append1-conversion0','append1-conversion22','append1-dataB','append1-pos512','append1-end0','append1-depth7','append1-manager22','append1-stop','append2-stop',
             'free-conversion22','free-depth7','free-manager22','free-stop','refresh-depth7','refresh-stop','full-conversion22','full-depth7','full-stop')
    for change in changes:  # 모든외부경계에서바뀐현재값은부모의실제후속명령이읽는다.
        rows.append((1,0 if change.startswith('assert1-') else 11,0 if change.startswith('assert2-') else 101,0,1 if change.startswith('full-') or change=='open-full0' else 0,1,1,0,2,11,3,change))
    return list(dict.fromkeys(rows))


def buffer(profile,alternate=False):
    """저장count와네BYTE항목만입력으로구성하며변환/추가/출력기대를계산하지않는다."""
    raw=bytearray([0xcc if not alternate else 0xdd])*2048
    types=() if profile==1 else (0,1,245,246,254,255) if profile==2 else tuple(range(255)) if profile==3 else (75,)
    for position in (0,512):  # callback의별도pos도동일폭내독립입력이다.
        raw[position]=len(types)
        for index,typ in enumerate(types):  # type/chance/power/remaining은각각전체BYTE를구별한다.
            raw[position+1+index*4:position+5+index*4]=bytes((typ,(index*19+7)&255,(index*37+128)&255,(index*53+255)&255))
    return bytes(raw)


class Oracle(MemoryOracle):
    """실제부모의depth/직접BYTE읽기/조건과정상복귀를실행하고하위몸체만명시장치로둔다."""
    def __init__(self):
        """FS0한DWORD와보호PE복사·명시입력만준비한다."""
        super().__init__();self.mu.mem_map(0,4096)
        self.total=self.normal=self.stops=self.cleanups=0
        self.boundaries=Counter();self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """원본CPU쓰기만FS/스택/깊이/변환표포인터/선택stream으로제한한다."""
        allowed=((0,4),(STACK-256,STACK+8),(DEPTH,CONVERSION+4),*((node,node+16) for node in STREAMS))
        if not any(low<=address and address+size<=high for low,high in allowed):
            raise RuntimeError(f'Deck부모CPU쓰기범위밖{address:08x}/{size}')
        if address==DEPTH:self.events.append('L:'+str(value&0xffffffff))

    def event(self,kind,text):
        """실제하위CALL의인자와순서만기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.events.append(text)
        return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """명시callback의현재값변경만공급하며후속부모분기는계산하지않는다."""
        mutation=self.case[-1]
        if '-' not in mutation:return
        trigger,action=mutation.split('-',1)
        if trigger not in (boundary,boundary.rstrip('1234567890')):return
        if action=='stop':self.stopped=True;self.mu.emu_stop()
        elif action.startswith('manager'):self.put(self.this,MANAGERS[int(action[7:])])
        elif action.startswith('conversion'):self.put(CONVERSION,TABLES[int(action[10:])])
        elif action.startswith('depth'):self.put(DEPTH,int(action[5:]))
        elif action.startswith('full'):self.put(FULL,int(action[4:]))
        elif action=='stream1':self.selected=1
        elif action=='dataB':self.put(STREAMS[self.selected],BUFFERS[1])
        elif action.startswith('pos'):self.put(STREAMS[self.selected]+4,int(action[3:]))
        elif action.startswith('end'):self.put(STREAMS[self.selected]+8,int(action[3:]))
        else:raise RuntimeError('Deck변경입력미해석:'+action)

    def instruction(self,mu,address,size,data):
        """부모/별도cleanup범위와지정하위외에게임/OS코드는실행하지않는다."""
        self.instructions+=1;self.total+=1
        if self.instructions>30000:raise RuntimeError('Deck부모명령상한')
        if any(low<=address<high for low,high in RANGES):return
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        args=self.words(esp+4,4)
        if address==0x4e0620:
            line=args[2];boundary=self.event('assert',f'A:{line}');self.mutate(boundary)
            if not self.stopped:self.ret()
        elif address==0x44f700:
            boundary=self.event('get',f'G:{args[0]}:{self.case[2]}');self.mutate(boundary)
            if not self.stopped:self.ret(DECKS[self.case[2]])
        elif address==0x44ceb0:
            manager=next(key for key,value in MANAGERS.items() if value==ecx)
            if self.string(args[0])!=b'Deck' or args[1]!=1:raise RuntimeError('Deck선택인자')
            boundary=self.event('select',f'S:{manager}:Deck:1');self.mutate(boundary)
            if not self.stopped:self.ret(STREAMS[self.selected],8)
        elif address in (0x44f230,0x44f250,0x44fbf0):
            kind={0x44f230:'clear',0x44f250:'open',0x44fbf0:'full'}[address]
            token=next(key for key,value in DECKS.items() if value==ecx)
            boundary=self.event(kind,f'{kind}:{token}'+(f':{args[0]}' if kind=='open' else ''));self.mutate(boundary)
            if not self.stopped:self.ret(purge=4 if kind=='open' else 0)
        elif address==0x4bd800:
            boundary=self.event('prepare',f'P:{(ecx-HEADERS)//20}');self.mutate(boundary)
            if not self.stopped:self.ret()
        elif address==0x44f330:
            token=next(key for key,value in DECKS.items() if value==ecx)
            boundary=self.event('append',f'E:{token}:'+':'.join(map(str,args)));self.mutate(boundary)
            if not self.stopped:self.ret(purge=16)
        elif address==0x4e4354:
            token=next(key for key,value in TABLES.items() if value==args[0])
            boundary=self.event('free',f'X:{token}');self.mutate(boundary)
            if not self.stopped:self.ret()
        elif address==0x44f7c0:
            boundary=self.event('refresh',f'R:{args[0]}');self.mutate(boundary)
            if not self.stopped:self.ret()
        else:raise RuntimeError(f'Deck부모검토밖명령{address:08x}')

    def run(self,entry,control):
        """명시스택/CPU로본문또는독립cleanup을실행한다."""
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)  # 관측간정수상태를격리한다.
        self.mu.reg_write(UC_X86_REG_ECX,self.this);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(entry,STOP)

    def sample(self,case,control):
        """전체입력과실제정상/중단부분상태·별도cleanup결과를읽는다."""
        self.case,self.events,self.calls=case,[],Counter();self.selected=case[3];self.stopped=False;self.instructions=0;self.this=HEADERS+case[0]*20
        self.put(0,0);self.put(self.this,MANAGERS[case[1]]);self.put(DEPTH,case[10]);self.put(FULL,case[4]);self.put(CONVERSION,TABLES[case[9]])
        for index,node in enumerate(STREAMS):  # 두실제반환stream은다른버퍼와같은초기pos/end를갖는다.
            self.mu.mem_write(node,struct.pack('<IIII',BUFFERS[index] if case[5] else 0,case[7],case[6],2048));self.mu.mem_write(BUFFERS[index],buffer(case[8],index!=0))
        for token,pointer in TABLES.items():  # identity와0/상한근처변환의서로다른255BYTE입력이다.
            if token:self.mu.mem_write(pointer,bytes((index if token==11 else (index*7+9)&255) if index not in (0,1,245) else (9,0,5)[(0,1,245).index(index)] for index in range(255)))
        self.run(0x4bf190,control)
        completed=not self.stopped;self.normal+=int(completed);self.stops+=int(not completed)
        if completed and (self.words(0,1)[0] or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Deck정상FS/스택복구불일치')
        manager=next(key for key,value in MANAGERS.items() if value==self.words(self.this,1)[0]);conversion=next(key for key,value in TABLES.items() if value==self.words(CONVERSION,1)[0])
        state=[manager,self.selected,self.words(DEPTH,1)[0],self.words(FULL,1)[0],conversion]
        streams=[]
        for node in STREAMS:  # 물리포인터는입력버퍼ID로만정규화하고세필드전부보존한다.
            fields=self.words(node,4);streams.extend((0 if not fields[0] else 1+BUFFERS.index(fields[0]),*fields[1:]))
        trace=';'.join(self.events);cleanup='-'
        if not completed:
            self.events=[];self.stopped=False;self.run(0x4fb860,control);self.cleanups+=1;cleanup=str(self.words(DEPTH,1)[0])+':'+ ';'.join(self.events)
        return '\t'.join((str(control),','.join(map(str,case)),str(int(completed)),','.join(map(str,state)),','.join(map(str,streams)),trace,cleanup))


def main():
    """최초부모관측을제품작성전에고정하거나그근거SHA만검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in record['sha256'].items():  # 기존관측은제품결과로덮어쓰지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('Deck최초근거SHA불일치:'+name)
        print('Deck부모최초관측SHA확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('Deck부모최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / slot,manager,deck,selected,full,data,end,pos,profile,conversion,depth,mutation / completed / 현재5DWORD / 두stream8DWORD / trace / 별도cleanup깊이:trace']
    for case in inputs:  # 두x87설정을별도실행하고정수결과차이를검사한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('Deck정밀도별결과차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_deck_load.py','cpppj/tests/fixtures/fort-deck-load-x86.tsv')
    record={'status':'최초부모원문고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'004bf190','bodyBytes':431,'cleanupBytes':15,'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'explicitStops':oracle.stops,'separateCleanupReturns':oracle.cleanups,'instructions':oracle.total,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'scope':'부모431BYTE의slot1..8/owner0clamp·manager재조회·Select뒤Clear/Open·현재fullDeck·count와4BYTE직접읽기·type<246현재변환·free뒤무조건0·Refresh·정상깊이와별도cleanup15BYTE다. Get/Select/Clear/Open/FullDeck/Append/TypeNames/free/진단/Refresh하위는명시경계이며원본SEH분배기·게임·GUI·OS를실행하지않는다. primitive본체는별도후속관측범위다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalReturns','explicitStops','instructions')}))


if __name__=='__main__':main()
