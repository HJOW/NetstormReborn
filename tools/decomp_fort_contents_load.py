#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004bd130의실제원시Contents재귀/현재변환표·TYPE/직접읽기/Create순서를최초고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TYPES, STACK, STOP, digest
from unicorn import UcError, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ_UNMAPPED
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

STREAMS, BUFFERS = (POOL+0x1000,POOL+0x1020),(POOL+0x2000,POOL+0x3000)  # 직접읽기에쓰는두실제16BYTE와2048BYTE물리입력이다.
TABLES={0:0,11:POOL+0x4000,22:POOL+0x4200}  # 현재255BYTE변환표의호스트관측ID다.
CONVERSION,CHECKING,DEPTH,POOL_BASE=0x5c89bc,0x5e4794,0x5c89b8,0x5c8464  # 원본현재전역이며본문은depth를쓰지않는다.
PARENT_TYPE=TYPES+256*500  # 순수부모가상TYPEgetter가명시반환할같은기록이다.
RANGE=(0x4bd130,0x4bd2d6)  # 정상RET까지의422BYTE이며재귀도실제같은본문이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fort-contents-load-x86.tsv'  # 제품전에최초고정한다.
REPORT=ROOT/'cpppj/fort-contents-load-native.json'  # 직접실행과명시TYPE/Create경계를구별한다.


def program(profile):
    """count/type/QA/QB/listFlags/중첩의원시BYTE입력만구성한다."""
    return (bytes((1,1,0x11)),bytes((0,)),bytes((3,2,254,3,128,255,1,4,85,52,128,16)),bytes((3,1,16,1,1,1,0)),bytes((2,5,16,2,1,0,2,7,1,4)),bytes((2,1,1,0)),bytes((2,5,1,2,1,1,0,1,4)),bytes((3,245,0,246,0,255,0)),bytes((255,))+bytes((1,0))*255,bytes((1,3,128,255,0)))[profile]


def cases():
    """빈입력/영확장count/현재conversion·debug/QAQB/end/부분callback입력만만든다."""
    rows=[]
    for profile,conversion,checking,relaxed in itertools.product(range(10),(0,11,22),(0,1),(0,1)):rows.append((profile,conversion,checking,relaxed,1,1,0,9,0,0,'none'))  # 정상/조기0/중첩/checked변환을교차한다.
    for data,end,selected,parent_flags in itertools.product((0,1),(0,1,0xffffffff,0x80000000),(0,1),(0,4,0x110)):rows.append((0,11,0,1,data,end,0,0xffffffff,parent_flags,selected,'none'))  # end는게이트와QB진단외직접읽기를제한하지않는다.
    for end,position,flags in itertools.product((1,3,4,0xffffffff,0x80000000),(0,512),(0,9,0xffffffff)):rows.append((9,11,1,0,1,end,position,flags,0,0,'none'))  # signedQB와pos/end진단이다.
    changes=('type1-flags0','type1-flagsQB','type1-conversion22','type1-dataB','type1-pos512','type1-end0','type1-stop',
             'type2-list4','type2-list0','type2-conversion22','type2-parent4','type2-stop','parent1-flags4','parent1-dataB','parent1-pos512','parent1-stop',
             'create1-conversion22','create1-conversion0','create1-dataB','create1-pos512','create1-end0','create1-parent4','create1-depth7','create1-return0','create1-stop','create2-stop',
             'assert1-conversion11','assert1-conversion0','assert1-dataB','assert1-pos512','assert1-endffffffff','assert1-checking0','assert1-stop','assert2-stop')
    for change,profile in itertools.product(changes,(2,3,4,9)):rows.append((profile,22 if change.startswith('assert1-conversion') else 11,1,0,1,1,0,9,0,0,change))  # 각경계뒤원래본문이현재값을재조회한다.
    rows.append((7,11,0x80000000,0,1,1,0,9,0,0,'none'))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """본문422BYTE와재귀는실제명령이고TYPE/부모TYPE/Create/진단만명시장치다."""
    def __init__(self):
        """보호PE복사에물리쓰기·null변환표실제실패관측을추가한다."""
        super().__init__();self.total=self.normal=0;self.statuses=Counter();self.boundaries=Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard);self.mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED,self.unmapped)

    def guard(self,mu,access,address,size,value,data):
        """원문CPU쓰기는재귀스택과두stream뿐이다.하위callback변경은별도입력이다."""
        if STACK-8192<=address and address+size<=STACK+20:return
        if any(node<=address and address+size<=node+16 for node in STREAMS):return
        raise RuntimeError(f'Contents원문쓰기범위밖{address:08x}/{size}')

    def unmapped(self,mu,access,address,size,value,data):
        """변환진단복귀뒤current0의실제BYTE접근실패를보고한다."""
        if not 0<=address<246 or size!=1:raise RuntimeError(f'Contents관측밖물리접근{address:08x}/{size}')
        self.status='fault';self.trace.append(f'F:{address}:{size}');return False

    def event(self,kind,text):
        """실제선택하위의인자/순서/1기반번호를기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """외부응답이현재메모리를바꾸는명시입력만적용한다."""
        change=self.case[-1]
        if '-' not in change:return
        trigger,action=change.split('-',1)
        if trigger!=boundary:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action.startswith('conversion'):self.put(CONVERSION,TABLES[int(action[10:])])
        elif action=='dataB':self.put(self.node,BUFFERS[1])
        elif action.startswith('pos'):self.put(self.node+4,int(action[3:]))
        elif action.startswith('end'):self.put(self.node+8,int(action[3:],16))
        elif action=='flags0':self.put(self.current_type+0xe8,0)
        elif action=='flagsQB':self.put(self.current_type+0xe8,0x2000)
        elif action.startswith('list'):self.put(self.current_type+0x100,int(action[4:]))
        elif action in ('parent4','flags4'):self.put(PARENT_TYPE+0xfc,4)
        elif action=='checking0':self.put(CHECKING,0)
        elif action=='depth7':self.put(DEPTH,7)
        elif action=='return0':self.return_zero=True
        else:raise RuntimeError('Contents변경미해석:'+action)

    def instruction(self,mu,address,size,data):
        """재귀실행범위를제한하고주요하위만명시반환한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>100000:raise RuntimeError('Contents명령상한')
        if RANGE[0]<=address<RANGE[1]:return
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        if address==0x49a840:
            typ=self.words(esp+4,1)[0]
            if typ>=256:raise RuntimeError('ContentsTYPE물리범위')
            self.current_type=TYPES+typ*500;boundary=self.event('type','T:'+str(typ));self.mutate(boundary)
            if self.status=='normal':self.ret(self.current_type)
        elif address==0x4ac550:
            parent=(ecx-POOL)//50;boundary=self.event('parent','P:'+str(parent));self.mutate(boundary)
            if self.status=='normal':self.ret(PARENT_TYPE)
        elif address==0x4afa70:
            args=self.words(esp+4,5);parent=(ecx-POOL)//50;boundary=self.event('create','C:'+str(parent)+':'+':'.join(map(str,args)));self.return_zero=False;self.mutate(boundary)
            if self.status=='normal':self.ret(0 if self.return_zero else 20+self.calls['create'],20)
        elif address==0x4e0620:
            line=self.words(esp+4,3)[2];boundary=self.event('assert','A:'+str(line));self.mutate(boundary)
            if self.status=='normal':self.ret()
        else:raise RuntimeError(f'Contents검토밖명령{address:08x}')

    def sample(self,case,control):
        """실제반환/조기중단/물리실패의현재cursor와전역·Create인자를기록한다."""
        self.case,self.trace,self.calls=case,[],Counter();self.node=STREAMS[case[9]];self.status='normal';self.instructions=0;self.current_type=TYPES;self.return_zero=False
        self.put(CONVERSION,TABLES[case[1]]);self.put(CHECKING,case[2]);self.put(DEPTH,3);self.put(POOL_BASE,POOL)
        self.mu.mem_write(TYPES,bytes(257*500));self.put(PARENT_TYPE+0xfc,case[8])
        for typ,flags,listing in ((2,0x1000,0x104),(3,0x2000,0),(4,0x3000,0),(5,0x20000,0)):self.put(TYPES+typ*500+0xe8,flags);self.put(TYPES+typ*500+0x100,listing)  # 현재원시TYPE입력이다.
        for token,pointer in TABLES.items():  # identity및0/타입폭변경의독립255BYTE변환입력이다.
            if not token:continue
            table=bytearray(range(255));table[245]=2 if token==11 else 0
            if token==22:table[0]=1;table[1]=2;table[2]=3;table[3]=0;table[4]=1
            self.mu.mem_write(pointer,bytes(table))
        for index,node in enumerate(STREAMS):  # 두stream의data/pos/end/용량입력을분리한다.
            self.mu.mem_write(node,struct.pack('<IIII',BUFFERS[index] if case[4] else 0,case[6],case[5],2048));raw=bytearray([0xcc if not index else 0xdd])*2048
            for position in (0,512):raw[position:position+len(program(case[0]))]=program(case[0])  # callbackpos에도명시원문을공급한다.
            self.mu.mem_write(BUFFERS[index],bytes(raw))
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)  # CPU상태를독립입력으로되돌린다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<5I',STOP,10,self.node,case[7],case[3]))
        try:self.mu.emu_start(RANGE[0],STOP)
        except UcError:
            if self.status!='fault':raise
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Contentscdecl스택복구차이')
        self.statuses[self.status]+=1;self.normal+=int(self.status=='normal')
        conversion=next(token for token,pointer in TABLES.items() if pointer==self.words(CONVERSION,1)[0]);result=self.mu.reg_read(UC_X86_REG_EAX) if self.status=='normal' else 0
        state=[result,self.words(DEPTH,1)[0],conversion,self.words(CHECKING,1)[0],self.calls['create'],self.words(PARENT_TYPE+0xfc,1)[0]];streams=[]
        for node in STREAMS:fields=self.words(node,4);streams.extend((0 if not fields[0] else BUFFERS.index(fields[0])+1,*fields[1:]))  # 현재포인터는물리버퍼ID로만정규화한다.
        return '\t'.join((str(control),','.join(map(str,case)),self.status,','.join(map(str,state)),','.join(map(str,streams)),';'.join(self.trace) or '-'))


def main():
    """제품을읽지않고최초독립원문을고정하거나기존근거SHA만검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf8'))
        for name,expected in record['sha256'].items():  # 독립원문을제품기대로덮어쓰지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('Contents최초SHA불일치:'+name)
        print('Contents최초SHA확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('Contents최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / profile,conversion,checking,relaxed,data,end,pos,flags,parentFlags,selected,mutation / status / result,depth,conversion,checking,creates,parentFlags / 두stream8DWORD / 실제trace']
    for case in inputs:  # 두x87설정을각각실제실행하며정수관측차이를검사한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('Contents정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf8',newline='\n');paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_contents_load.py','cpppj/tests/fixtures/fort-contents-load-x86.tsv')
    record={'status':'최초원문고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'004bd130','bodyBytes':RANGE[1]-RANGE[0],'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'scope':'원시Contents422BYTE와같은실제재귀.현재conversion/checking/두TYPE조회·부모가상TYPE/QAQB·QB현재pos와signedend진단/Local형제지속·중첩값전달·직접Create인자·조기0·본문depth무변경을관측한다. TYPE/부모TYPE/Create/진단만명시외부경계이며실제ContainedCreate/Pool생성/OS/GUI/게임은별도다. nullconversion의진단뒤실제BYTE접근실패도정상반환과구별한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalReturns','statuses','instructions')}))


if __name__=='__main__':main()
