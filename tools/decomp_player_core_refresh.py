#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""0044fc00과실제Core/기술/Finder/Ch/iterator몸체를제품작성전에독립PE로고정한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 실제주소식별과같은가상물리두벌은callback뒤현재base재조회를구별한다.
CURRENT, CACHED, METAMAP = 0x595350, 0x5953f0, 0x594fc8
POOL_GLOBAL, CAPACITY, OWNER, PLAYER_TYPE = 0x5c8464, 0x5c847c, 0x540c70, 0x5412ec
CORE_A, CORE_B, TYPE_BASE, VTABLE, LEVEL, REENTER = PLAYERS+0x4000, PLAYERS+0x4100, 0x14000000, POOL+0x80000, STOP+0x100, STOP+0x200
POOLS = (POOL, POOL+0x10000)
FIXTURE = ROOT/'cpppj/tests/fixtures/player-core-refresh-x86.tsv'
REPORT = ROOT/'extracted/cpp-player-core-refresh-20261024-native.json'
ENTRIES = (0x44fc00,0x48f0d0,0x48fc60,0x4b2270,0x4b1b20,0x44daa0,0x49a840,0x42b280,0x40eb40)


def cases():
    """본문기대없이기술bit/객체/첫둘째가상반환·물리중단입력만선택한다."""
    base = (0,7,0,31,-1,0,0,1,152,8,0,0,10,'none')
    rows = [base]
    for floor,mask,free,dead in itertools.product((0,3,255),(0,1,7),(0,0xfc),(0,0xc4)):
        rows.append((floor,mask,0,31,-1,free,dead,1,152,8,0,0,10,'none'))
    for bit in (-2147483648,-2,-1,0,1,31,32,33,2147483647):
        rows.append((9,7,bit,31,bit,0,0,1,152,8,0,0,10,'none'))
    for owner,ptype,capacity,levels in itertools.product((0,1,256),(6,152,256),(0,1,2,3,8),range(5)):
        rows.append((4,7,0,31,-1,0,0,owner,ptype,capacity,levels,0,10,'none'))
    mutations = ('g1-currentB','g2-currentB','g3-currentB','g4-currentB','g5-currentB','g1-stop','g2-stop','g3-stop',
                 'p1-known0','p1-owner8','p1-stop','b1-poolB','b1-stop','n1-poolB','n1-contained152','n1-stop',
                 't1-currentB','t1-poolB','t1-stop','v1-currentB','v1-poolB','v1-cap2','v1-dead','v1-stop','v2-stop',
                 'v1-reenter','a1-currentB','a1-cache','a1-stop','a2-stop','v1-owner0','v1-playerType6')
    for mutation in mutations:
        row = list(base);row[-1] = mutation
        if mutation.startswith('a'):
            row[2] = 32
        rows.append(tuple(row))
    for kind,length in itertools.product(range(4),(0,5,6,9,10)):
        row = list(base);row[11:13] = (kind,length);rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """복사PE의허용몸체만실행하며가상level과진단의명시효과를기록한다."""
    def __init__(self):
        """65535TYPE도검증없이실제주소산술로도달할수있는가상물리를준비한다."""
        super().__init__()
        self.mu.mem_map(TYPE_BASE,0x2000000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts = {int(row['entry'],16): row for row in csv.DictReader(source,delimiter='\t')}
        self.ranges=[]
        for entry in ENTRIES:  # 원문정밀범위안에서만CPU명령을실행한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high = (int(value,16) for value in part.split('-'));self.ranges.append((low,high+1))
        self.running=False;self.total=0;self.status='normal';self.boundaries=Counter()
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid)

    def invalid(self,mu,access,address,size,value,data):
        """NULL Core의실제미매핑접근만호스트계약과구별해기록한다."""
        if self.running and address<16:
            self.status='fault-null';mu.emu_stop();return False
        raise RuntimeError('CoreRefresh예상밖미매핑:'+hex(address)+':'+str(self.case))

    def pointer(self,address):
        """현재Core의같은실제물리주소만정규화한다."""
        return {0:'0',CORE_A:'A',CORE_B:'B',CACHED:'C'}[address]

    def read(self,mu,access,address,size,value,data):
        """명시Core물리가짧으면최초실제load에서부분효과를유지하고중단한다."""
        if not self.running:return
        for base in (CORE_A,CORE_B,CACHED):  # 캐시를포함해같은입력길이로원문접근폭을관측한다.
            if base<=address<base+16 and address+size>base+self.case[12]:
                self.status='fault';mu.emu_stop();return

    def write(self,mu,access,address,size,value,data):
        """직접Core쓰기와스택외예상밖store를거부하고짧은store는실제위치에서중단한다."""
        if not self.running:return
        if STACK-8192<=address<address+size<=STACK+4:return
        for base in (CORE_A,CORE_B,CACHED):  # 원문은Core+6DWORD와최종+5BYTE만직접쓴다.
            if base<=address<address+size<=base+16:
                if address+size>base+self.case[12]:self.status='fault';mu.emu_stop()
                return
        raise RuntimeError('CoreRefresh원문쓰기범위밖:'+hex(address))

    def event(self,kind,text):
        """같은종류호출의1기준번호를재진입에서도공유한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """현재global/물리만바꾸고원문분기나출력을계산하지않는다."""
        trigger,_,action=self.case[-1].partition('-')
        if trigger!=boundary:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='currentB':self.put(CURRENT,CORE_B)
        elif action=='cache':self.put(CURRENT,CACHED);self.put(METAMAP,1)
        elif action=='poolB':self.put(POOL_GLOBAL,POOLS[1])
        elif action=='cap2':self.put(CAPACITY,2)
        elif action=='known0':self.put(0x595344,0)
        elif action=='owner8':self.put(OWNER,8)
        elif action=='owner0':self.put(OWNER,0)
        elif action=='contained152':self.put(0x541088,152)
        elif action=='playerType6':self.put(PLAYER_TYPE,6)
        elif action=='dead':
            base=self.words(POOL_GLOBAL,1)[0];index=self.current_level[1];self.put(base+index*50+11,2,1)
        elif action=='reenter':pass  # 실제재진입스택은level장치의호출위치에서구성한다.
        else:raise RuntimeError('CoreRefresh명시변경미해석:'+action)

    def level_value(self,block,index):
        """가상함수의명시반환열만제공하며max선택은CPU가수행한다."""
        key=(block,index);self.level_calls[key]+=1;second=self.level_calls[key]%2==0;mode=self.case[10]
        if mode==1 and second:return -3
        if mode==2 and second:return -2147483648
        if mode==3:return 2147483647
        if mode==4:return -1
        return (4,3,2)[(index-2)%3]+block

    def instruction(self,mu,address,size,data):
        """부모와선택실제자식은CPU로수행하고외부진단/level만명시한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>20000:raise RuntimeError('CoreRefresh명령상한')
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==REENTER:
            previous_esp,value=self.reentry.pop();self.trace.append('R:end');mu.reg_write(UC_X86_REG_ESP,previous_esp);self.ret(value);return
        if address==0x48f0d0:
            boundary=self.event('g','G:'+self.pointer(self.words(CURRENT,1)[0]));self.mutate(boundary)
        elif address==0x48fc60:
            boundary=self.event('p','P');self.mutate(boundary)
        elif address==0x4b2270:
            parent=self.words(esp+4,1)[0];boundary=self.event('b','B:'+str(parent));self.mutate(boundary)
        elif address==0x4b1b20 and self.words(esp,1)[0]==0x44fc83:
            boundary=self.event('n','N');self.mutate(boundary)
        elif address==0x49a840:
            kind=self.words(esp+4,1)[0];boundary=self.event('t','T:'+str(kind));self.mutate(boundary)
        if self.status!='normal':return
        if any(low<=address<high for low,high in self.ranges):return
        if address==0x4e0620:
            assertion,source,line=self.words(esp+4,3);boundary=self.event('a','A:'+str(line));self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==LEVEL:
            block=next((number for number,base in enumerate(POOLS) if base<=ecx<base+400),None)
            if block is None:raise RuntimeError('CoreRefresh가상호출물리:'+hex(ecx))
            index=(ecx-POOLS[block])//50;self.current_level=(block,index);value=self.level_value(block,index)
            boundary=self.event('v',f'V:{block}:{index}:{value}');self.mutate(boundary)
            if self.status!='normal':return
            if self.case[-1]==boundary+'-reenter':
                self.trace.append('R:begin');self.reentry.append((esp,value));mu.reg_write(UC_X86_REG_ESP,esp-4);self.put(esp-4,REENTER);mu.reg_write(UC_X86_REG_EIP,0x44fc00)
            else:self.ret(value)
        else:raise RuntimeError('CoreRefresh관측밖명령:'+hex(address))

    def sample(self,case,control):
        """초기물리와현재전역을완전히재구성해양정밀도결과를독립고정한다."""
        self.case=case;self.trace=[];self.calls=Counter();self.level_calls=Counter();self.reentry=[];self.status='normal';self.instructions=0;self.running=False
        floor,mask,bit_a,bit_b,bit_c,free,dead,owner,player_type,capacity,levels,current_kind,length,mutation=case
        for base in (CORE_A,CORE_B,CACHED):  # 최종+5의getter별현재물리를구별하는명시패턴이다.
            self.mu.mem_write(base,bytes((41+(base-CORE_A)//256*29+i*11)&255 for i in range(16)));self.put(base+5,floor if base==CORE_A else (floor+17)&255,1)
        for block,base in enumerate(POOLS):  # 전체raw400BYTE를비교하되필요필드만명시입력으로쓴다.
            self.mu.mem_write(base,bytes(400))
            for index in range(8):
                raw=base+index*50;self.put(raw,VTABLE);self.put(raw+10,152 if index in (2,6,7) else 6,1);self.put(raw+11,((free>>index)&1)|(((dead>>index)&1)<<1),1);self.put(raw+34,1,1)
            self.put(base+50+6,3,2)
            for index,next_index,kind in ((3,4,1),(4,5,2),(5,0,65535)):
                self.put(base+index*50+4,next_index,2);self.put(base+index*50+18,kind,2);self.put(base+index*50+20,4 if mask&(1<<(index-3)) else 0,1)
            if block:self.put(base+2*50+11,2,1)  # pool변경후dead재조회와두번째vfunc도구별한다.
        for kind,bit in ((1,bit_a),(2,bit_b),(65535,bit_c)):self.put(TYPE_BASE+kind*500+492,bit)
        self.mu.mem_write(PLAYERS,bytes(1620));self.put(PLAYERS+180+156,1);self.put(PLAYERS+8*180+156,1)
        globals_values=((CURRENT,(CORE_A,CORE_B,CACHED,0)[current_kind]),(METAMAP,0),(0x595344,1),(0x59534c,PLAYERS),(0x59ab20,TYPE_BASE),(POOL_GLOBAL,POOLS[0]),(CAPACITY,capacity),(OWNER,owner),(PLAYER_TYPE,player_type),(0x541088,6),(0x5e4794,0),(0x540bc4,1))
        for address,value in globals_values:self.put(address,value)
        self.put(VTABLE+0x80,LEVEL);self.mu.mem_write(STACK-8192,bytes(8196))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.put(STACK,STOP);self.running=True
        try:self.mu.emu_start(0x44fc00,STOP)
        except UcError:
            if self.status!='fault-null':raise
        self.running=False
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('CoreRefresh스택복귀차이')
        state=[POOLS.index(self.words(POOL_GLOBAL,1)[0]),self.words(CAPACITY,1)[0],self.words(OWNER,1)[0],self.words(PLAYER_TYPE,1)[0],self.words(0x541088,1)[0],self.words(0x595344,1)[0],self.words(METAMAP,1)[0]]
        cores=b''.join(bytes(self.mu.mem_read(base,16)) for base in (CORE_A,CORE_B,CACHED));pools=b''.join(bytes(self.mu.mem_read(base,400)) for base in POOLS)
        return '\t'.join((str(control),','.join(map(str,case)),self.status,self.pointer(self.words(CURRENT,1)[0]),','.join(map(str,state)),cores.hex(),pools.hex(),';'.join(self.trace) or '-',str(self.instructions)))


def main():
    """신규제품이없는최초PE자료만생성하고후속호출은SHA검증만허용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():assert digest(ROOT/name)==expected,name
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('CoreRefresh최초fixture/report덮어쓰기금지')
    for relative in ('cpppj/src/o/RawPlayerCoreRefresh.h','cpppj/src/o/RawPlayerCoreRefresh.cpp','cpppj/tests/PlayerCoreRefreshTests.cpp'):
        if (ROOT/relative).exists():raise RuntimeError('CoreRefresh제품작성전관측위반')
    oracle=Oracle();inputs=cases();lines=['# control / floor,techmask,bitA,bitB,bitC,freeMask,deadMask,owner,playerType,capacity,levelSequence,currentKind,coreLength,mutation / status / current / poolIndex,capacity,owner,playerType,containedType,known,metamap / threeCore16 / twoPool400 / trace / instructions'];statuses=Counter();instructions=0
    for case in inputs:  # 실제CPU출력만최초기대자료로저장한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('CoreRefresh양정밀도차이')
        for line in (first,second):lines.append(line);statuses[line.split('\t')[2]]+=1;instructions+=int(line.split('\t')[-1])
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_player_core_refresh.py','cpppj/tests/fixtures/player-core-refresh-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','sourceAbsentBeforeObservation':True,'entries':ENTRIES,'parentRanges':'44fc00-44fc35;44fc40-44fd6e','parentBytes':357,'uniqueInputs':len(inputs),'observations':len(lines)-1,'statuses':dict(statuses),'instructions':instructions,'boundaries':dict(oracle.boundaries),'precisionDifferences':0,'gameExecuted':False,'osExecuted':False,'scope':'실제357BYTE부모/현재Coregetter/현재기술부모/Finder Begin·Next·true가상필터/TYPEgetter/Ch/iterator. level80과진단은명시외부장치. TYPE65535는실제500배주소산술. 짧은Core와NULL은명시물리중단.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','statuses','instructions')},ensure_ascii=False))


if __name__=='__main__':main()
