#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Subscriber ID161BYTE와실제CRT srand13/rand34BYTE를제품전에독립관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

INITIALIZED, TLS = 0x5c89a4,POOL+0x100  # 실제첫호출전역과같은스레드CRT장치의명시holdrand주소다.
RANGES = ((0x4bc950,0x4bc9f1),(0x4e649e,0x4e64cd))  # 부모161BYTE와실제srand/rand47BYTE를그대로실행한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/subscriber-id-generate-x86.tsv'  # 제품결과를보기전에고정하는원문이다.
REPORT = ROOT/'cpppj/subscriber-id-generate-native.json'  # 실제CRT와명시rawrand장치를별도표시한다.


def cases():
    """현재첫호출플래그/CRT상태/시간/명시rand반환/장치변형만교차한다."""
    rows=[]
    for initialized,crt,seconds,clock,invocations in itertools.product((0,1,0xffffffff,0x80000000),(0,1,7,0xffffffff,0x80000000),(0,0xffffffff,1234567890),(0,1,0xffffffff),(1,2)):
        rows.append((0,initialized,crt,seconds,clock,0,invocations,'none'))  # 실제CRT몸체/현재공유상태와연속호출을관측한다.
    for profile,initialized,crt,invocations in itertools.product(range(13),(0,1),(0,0xffffffff),(1,2)):
        rows.append((1,initialized,crt,0xffffffff,0x80000001,profile,invocations,'none'))  # signed%100/64경계/최대16회재시도의명시rand입력이다.
    changes=('time-init7','time-crt9','time-stop','clock-initffffffff','clock-crt9','clock-stop','seed-init7','seed-initffffffff','seed-crt9','seed-stop',
             'rand1-init0','rand1-crt9','rand1-stop','rand100-init7','rand100-crt9','rand100-stop','rand101-init0','rand101-crt0','rand101-stop','rand132-init7','rand132-stop',
             'expired-init0','expired-crt9','expired-stop')
    for change,mode in itertools.product(changes,(0,1)):
        rows.append((mode,0,7,0x80000000,0xffffffff,1,1,change))  # callback복귀뒤현재플래그/CRT를원문이결정한다.
    return list(dict.fromkeys(rows))


def scripted(profile,ordinal):
    """원시randDWORD응답만준비한다.부모의생성후보/반환기대는계산하지않는다."""
    if ordinal<=100:return 0xffffffff if profile&1 else 0x7fffffff
    index=ordinal-101;attempt,bit=index//32,31-index%32
    if profile==8:return (49,50,99,100,0xffffffff,0xffffff9c,0x80000000,0x7fffffff)[index%8]
    if profile==9:return 0xffffffff
    if profile==10:return 0x80000000
    if profile==11:return 0x7fffffff
    if profile==12:return (49,50,99,100)[index%4]
    candidate=(65,0,64,0 if not attempt else 65,0 if attempt<15 else 65,0xffffffff,0x80000040,0x80000040 if attempt<15 else 0x80000041)[profile]
    return 0 if candidate&(1<<bit) else 50


class Oracle(MemoryOracle):
    """원본부모/CRT를실행하며OS시각·PTD주소·로그와선택한rawrand만명시반환한다."""
    def __init__(self):
        """보호PE복사에현재전역/CRT/스택외CPU쓰기거부를설치한다."""
        super().__init__();self.total=self.normal=0;self.statuses=Counter();self.boundaries=Counter();self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """게임실행없이부모플래그와실제CRT holdrand쓰기만허용한다."""
        allowed=((STACK-1024,STACK+8),(INITIALIZED,INITIALIZED+4),(TLS+20,TLS+24))
        if not any(low<=address and address+size<=high for low,high in allowed):raise RuntimeError(f'SubscriberID원문쓰기범위밖{address:08x}/{size}')

    def event(self,kind,text):
        """외부진입의실제인자/순서와현재상태를기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """실제외부응답이바꿀현재전역/CRT만명시한다."""
        mutation=self.case[-1]
        if '-' not in mutation:return
        trigger,action=mutation.split('-',1)
        if trigger not in (boundary,boundary.rstrip('1234567890')):return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='init7':self.put(INITIALIZED,7)
        elif action=='init0':self.put(INITIALIZED,0)
        elif action=='initffffffff':self.put(INITIALIZED,0xffffffff)
        elif action=='crt9':self.put(TLS+20,9)
        elif action=='crt0':self.put(TLS+20,0)
        else:raise RuntimeError('SubscriberID변형미해석:'+action)

    def instruction(self,mu,address,size,data):
        """원문RAND/seed몸체에진입하기전관측하고명시raw모드에서만RAND를대체한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>100000:raise RuntimeError('SubscriberID명령상한')
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4e649e:
            seed=self.words(esp+4,1)[0];boundary=self.event('seed','S:'+str(seed));self.mutate(boundary)
            return
        if address==0x4e64ab:
            boundary=self.event('rand','R:'+str(self.calls['rand']+1)+':'+str(self.words(INITIALIZED,1)[0])+':'+str(self.words(TLS+20,1)[0]));self.mutate(boundary)
            if self.status=='normal' and self.case[0]:self.ret(scripted(self.case[5],self.calls['rand']))
            return
        if any(low<=address<high for low,high in RANGES):return
        if address==0x4e4f1f:
            if self.words(esp+4,1)[0]:raise RuntimeError('SubscriberID time32(0)인자불일치')
            boundary=self.event('time','T');self.mutate(boundary)
            if self.status=='normal':self.ret(self.case[3])
        elif address==0x4e78d0:
            boundary=self.event('clock','C');self.mutate(boundary)
            if self.status=='normal':self.ret(self.case[4])
        elif address==0x4ea61d:
            self.boundaries['ptd']+=1;self.ret(TLS)
        elif address==0x4c2630:
            level,text=self.words(esp+4,2);message=self.string(text).decode('ascii');boundary=self.event('expired','E:'+str(level)+':'+message.replace('\n','\\n'));self.mutate(boundary)
            if self.status=='normal':self.ret()
        else:raise RuntimeError(f'SubscriberID관측밖명령{address:08x}')

    def sample(self,case,control):
        """같은원문전역/CRT로한번또는연속두번호출하고중단도구별한다."""
        self.case,self.trace=case,[];self.put(INITIALIZED,case[1]);self.put(TLS+20,case[2]);results=[];self.status='normal'
        for invocation in range(case[6]):  # 플래그/CRT는초기화하지않고실제호출프레임만새로만든다.
            self.calls=Counter();self.instructions=0;self.trace.append('I:'+str(invocation+1))
            for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 원본C함수호출의독립입력CPU상태다.
            self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(0x4bc950,STOP)
            if self.status!='normal':break
            if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('SubscriberIDcdecl스택복구차이')
            results.append(self.mu.reg_read(UC_X86_REG_EAX));self.normal+=1
        self.statuses[self.status]+=1;state=(self.words(INITIALIZED,1)[0],self.words(TLS+20,1)[0]);return '\t'.join((str(control),','.join(map(str,case)),self.status,','.join(map(str,results)) or '-',','.join(map(str,state)),';'.join(self.trace)))


def main():
    """처음원문을고정한뒤에는제품기대로덮어쓰지않고SHA만검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf8'))
        for name,expected in record['sha256'].items():  # 최초원문/도구/fixture바이트를확인한다.
            if digest(ROOT/name)!=expected:raise RuntimeError('SubscriberID최초SHA불일치:'+name)
        print('SubscriberID최초SHA확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('SubscriberID최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / rawMode,initialized,crt,time32,clock,profile,invocations,mutation / status / 실제반환목록 / currentInitialized,currentCrt / trace']
    for case in inputs:  # 두정밀도를독립실행하여실제정수상태차이도검사한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('SubscriberID정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf8',newline='\n');paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_subscriber_id_generate.py','cpppj/tests/fixtures/subscriber-id-generate-x86.tsv')
    record={'status':'최초원문고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entries':{'004bc950':161,'004e649e':13,'004e64ab':34},'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalFunctionReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'scope':'실제ID부모161와CRT srand13/rand34BYTE.순수PTD주소/OS time32·clock/실패로그만외부장치이며rawMode1은별도명시signedRAND응답으로최대16회/실패경계를관측한다.동일현재초기화플래그와CRT연속두번호출/100warmup/시각·seed·rand뒤상태변경·중단을고정한다.27비트mask때문에후보ffffffff비교의실패분기는정상입력으로도달하지않는다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalFunctionReturns','statuses','instructions')}))


if __name__=='__main__':main()
