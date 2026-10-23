#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Subscriber122BYTE/문자열144BYTE/등록ID78BYTE의최초독립원문을제품전에고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UcError, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ_UNMAPPED
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADER, STREAMS = POOL+0x100, (POOL+0x200,POOL+0x220)  # 고정this와독립선택반환두stream이다.
BUFFERS, OUTPUT, PHYSICAL = (POOL+0x1000,POOL+0x2000),POOL+0x3000,512  # 논리end와별개인명시물리입력/출력장치다.
SUBSCRIBER = 0x540c74  # 부모의실제출력대상이며등록helper도같은현재DWORD를직접읽고쓴다.
RANGES = ((0x4bd450,0x4bd4ca),(0x4bcb60,0x4bcbf0),(0x4b38d0,0x4b391e))  # 실제부모/길이읽기/등록부모의모든정상명령이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/fort-subscriber-load-x86.tsv'  # 최초관측이후제품기대로덮어쓰지않는다.
REPORT = ROOT/'cpppj/fort-subscriber-load-native.json'  # 근거SHA와선택된외부장치를구별한다.


def cases():
    """파일ID/길이/논리끝과명시외부변경만만들고기대cursor는계산하지않는다."""
    rows=[]
    for identity,length,end in itertools.product((0,1,0xffffffff,0x80000000),(0,1,3,4,7,31,63,64,127,255,256),(0,1,6,512,0xffffffff,0x80000000)):
        rows.append((0,1,11,0,1,end,0,identity,length,0,77,0x80000001,'none'))  # 양의저장길이를사전256제한없이읽는다.
    for length,position,data in itertools.product((0x8000,0xffff),(0,64),(0,1)):
        rows.append((0,8,0,1,data,1,position,0xffffffff,length,1,9,88,'none'))  # 실제접근실패와앞선부분복사를구별한다.
    for identity,result,read_value,generated in itertools.product((0,1,0xffffffff,0x80000000),(0,1),(0,0xffffffff,0x80000000),(0,0xffffffff)):
        rows.append((1,0,11,0,1,512,0,identity,0,result,read_value,generated,'none'))  # 등록부모만독립실행해미선택장치를구별한다.
    changes=('assert1-manager22','assert1-subscriber9','assert1-stop','select-stream1','select-manager0','select-pos64','select-data0','select-stop',
             'assert1-pos16','assert1-dataB','assert1-endffffffff','assert2-pos16','assert2-dataB','assert2-endffffffff','assert2-stop',
             'assert3-pos16','assert3-dataB','assert3-subscriber9','assert3-stop','read-subscriber9','read-stop','generate-subscriber9','generate-stop','config-subscriber9','config-stop')
    for change in changes:
        rows.append((0,1,0 if change.startswith('assert1-manager') else 11,0,1,1,0,0xffffffff,7,0,77,0xffffffff,change))  # 각외부효과복귀후의현재manager/data/pos/end/ID를관측한다.
    rows.extend(((0,0,11,0,0,1,0,1,3,0,77,88,'none'),(0,1,11,1,1,512,64,1,3,0,77,88,'none')))
    return list(dict.fromkeys(rows))


def payload(identity,length):
    """원시ID/길이/내부NUL을가진저장BYTE만준비한다."""
    return struct.pack('<IH',identity,length)+bytes((index*37+65)&255 for index in range(min(length,300)))


class Oracle(MemoryOracle):
    """세실제몸체를실행하고Select/진단/Config/새ID/CRT서식만명시경계로둔다."""
    def __init__(self):
        """보호PE복사에직접물리읽기/쓰기실패관측만추가한다."""
        super().__init__();self.total=self.normal=0;self.statuses=Counter();self.boundaries=Counter();self.current=0
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED,self.unmapped)

    def fail(self):
        """호스트명시물리장치끝을원문정상반환/진단중단과구별한다."""
        self.status='fault';self.mu.emu_stop()

    def read(self,mu,access,address,size,value,data):
        """직접ID/길이/복사의실제메모리범위를명시512BYTE로제한한다."""
        if self.status!='normal':return
        if self.current in (0x4bd484,0x4bcb6c,0x4bcba8,0x4bcbaf) and not any(base<=address and address+size<=base+PHYSICAL for base in BUFFERS):self.fail()

    def write(self,mu,access,address,size,value,data):
        """원문출력/현재ID/cursor/스택외실제쓰기는물리실패로남긴다."""
        if self.status!='normal':return
        allowed=((STACK-1024,STACK+16),(SUBSCRIBER,SUBSCRIBER+4),(OUTPUT,OUTPUT+PHYSICAL),*((base,base+16) for base in STREAMS))
        if not any(low<=address and address+size<=high for low,high in allowed):self.fail()

    def unmapped(self,mu,access,address,size,value,data):
        """data0등실제미매핑접근도같은물리실패로보고한다."""
        self.fail();return False

    def event(self,kind,text):
        """실제호출순서/인자와경계별순번만기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """외부응답이바꾸는명시현재상태만적용한다."""
        mutation=self.case[-1]
        if '-' not in mutation:return
        trigger,action=mutation.split('-',1)
        if trigger not in (boundary,boundary.rstrip('1234567890')):return
        node=STREAMS[self.selected]
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='manager22':self.put(HEADER,22)
        elif action=='manager0':self.put(HEADER,0)
        elif action=='subscriber9':self.put(SUBSCRIBER,9)
        elif action=='stream1':self.selected=1
        elif action.startswith('pos'):self.put(node+4,int(action[3:]))
        elif action=='data0':self.put(node,0)
        elif action=='dataB':self.put(node,BUFFERS[1])
        elif action=='endffffffff':self.put(node+8,0xffffffff)
        else:raise RuntimeError('Subscriber변경미해석:'+action)

    def instruction(self,mu,address,size,data):
        """실제parent→string→등록부모를통과하며하위선택만명시응답한다."""
        self.instructions+=1;self.total+=1;self.current=address
        if self.instructions>100000:raise RuntimeError('Subscriber명령상한')
        if any(low<=address<high for low,high in RANGES):return
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        if address==0x44ceb0:
            name,writable=self.words(esp+4,2);boundary=self.event('select','S:'+str(ecx)+':'+self.string(name).decode('ascii')+':'+str(writable));self.mutate(boundary)
            if self.status=='normal':self.ret(STREAMS[self.selected],8)
        elif address==0x4e0620:
            line=self.words(esp+4,3)[2];boundary=self.event('assert','A:'+str(line));self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x441270:
            key,target=self.words(esp+4,2)
            if target!=SUBSCRIBER:raise RuntimeError('Subscriber현재ID참조불일치')
            boundary=self.event('read','R:'+self.string(key).decode('ascii'));self.put(target,self.case[10]);self.mutate(boundary)
            if self.status=='normal':self.ret(self.case[9])
        elif address==0x4bc950:
            boundary=self.event('generate','G');self.mutate(boundary)
            if self.status=='normal':self.ret(self.case[11])
        elif address==0x4e58e9:
            target,formatting,value=self.words(esp+4,3)
            if self.string(formatting)!=b'registerdSubId = "%u"':raise RuntimeError('Subscriber실제서식불일치')
            raw=b'registerdSubId = "'+str(value).encode('ascii')+b'"';mu.mem_write(target,raw+b'\0');self.boundaries['sprintf']+=1;self.ret(len(raw))
        elif address==0x441580:
            text=self.string(self.words(esp+4,1)[0]);boundary=self.event('config','C:'+text.decode('ascii'));self.mutate(boundary)
            if self.status=='normal':self.ret()
        else:raise RuntimeError(f'Subscriber관측밖명령{address:08x}')

    def sample(self,case,control):
        """제품기대없이실제ID/stream/출력전체/부분중단/경계trace를기록한다."""
        self.case,self.trace,self.calls=case,[],Counter();self.selected=case[3];self.status='normal';self.instructions=0
        self.put(HEADER,case[2]);self.put(SUBSCRIBER,case[7] if case[0] else 0x12345678);self.mu.mem_write(OUTPUT,bytes([0xa5])*PHYSICAL)
        raw=payload(case[7],case[8])
        for index,node in enumerate(STREAMS):  # 서로다른선택반환/현재data를구별하는실제입력이다.
            self.mu.mem_write(node,struct.pack('<IIII',BUFFERS[index] if case[4] else 0,case[6],case[5],PHYSICAL));contents=bytearray((byte*17+index*91)&255 for byte in range(PHYSICAL))
            for position in (0,64):contents[position:position+len(raw)]=raw  # 명시callbackpos에도동일원시레코드만놓는다.
            self.mu.mem_write(BUFFERS[index],bytes(contents))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 매관측을초기CPU상태에서시작한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,HEADER);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<III',STOP,SUBSCRIBER,OUTPUT))
        try:self.mu.emu_start(0x4bd450 if not case[0] else 0x4b38d0,STOP)
        except UcError:
            if self.status!='fault':raise
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(12 if not case[0] else 4):raise RuntimeError('Subscriber스택복구차이')
        self.statuses[self.status]+=1;self.normal+=int(self.status=='normal');stream=[]
        for node in STREAMS:  # 실제data포인터만물리버퍼번호로정규화한다.
            words=self.words(node,4);stream.extend((0 if not words[0] else BUFFERS.index(words[0])+1,*words[1:]))
        state=(self.words(SUBSCRIBER,1)[0],self.words(HEADER,1)[0]);return '\t'.join((str(control),','.join(map(str,case)),self.status,','.join(map(str,state)),','.join(map(str,stream)),bytes(self.mu.mem_read(OUTPUT,PHYSICAL)).hex(),';'.join(self.trace) or '-'))


def main():
    """처음실제관측을고정하거나이후에는최초SHA만검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf8'))
        for name,expected in record['sha256'].items():  # 제품결과로기대를재생성하지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('Subscriber최초SHA불일치:'+name)
        print('Subscriber최초SHA확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('Subscriber최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases();lines=['# control / mode,slot,manager,selected,data,end,pos,fileID,length,readResult,readID,newID,mutation / status / currentID,manager / 두stream8DWORD / 출력512BYTE / trace']
    for case in inputs:  # 두x87정밀도의실제정수상태를각각기록한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('Subscriber정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf8',newline='\n');paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_subscriber_load.py','cpppj/tests/fixtures/fort-subscriber-load-x86.tsv')
    record={'status':'최초원문고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entries':{'004bd450':122,'004bcb60':144,'004b38d0':78},'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'precisionDifferences':0,'boundaries':dict(oracle.boundaries),'gameExecuted':False,'osExecuted':False,'scope':'실제Subscriber부모122/string144/등록helper78BYTE. 현재manager재조회/ID쓰기후pos재조회/signed길이와DWORD단위직접복사/현재data·pos·end진단/부분복사실패/같은실제ID등록관문을관측한다.Select/진단/ConfigRead/새ID/CRT서식/Config식만명시외부경계다.부모는깊이·TYPE를읽거나쓰지않는다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','normalReturns','statuses','instructions')}))


if __name__=='__main__':main()
