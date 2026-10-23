#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004b5890의772BYTE와SEH파일cleanup을제품전에독립관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TEXT, STACK, STOP, digest
from unicorn import UcError, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ_UNMAPPED
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

# 원문steadySP와그안의호스트명시392BYTE입력이다.미정stack를제로기대로가장하지않는다.
FRAME, SCRATCH, SCRATCH_BYTES = STACK-0x1a4, STACK-0x194, 0x188
BASES = PLAYERS, PLAYERS+0x2000  # 현재Player포인터가바뀌어도살아있는두9×180BYTE할당이다.
PLAYER_BASE, OWNER, IDENTITY, HANDLE = 0x59534c, 0x542528, 0x54252c, 0x5c84c0
TAILS = 0x595090, 0x55a420, 0x55b4ac  # 마지막세직접read의실제DWORD주소다.
CACHE = 0x595360  # 명시48f350장치가쓰는같은9×16BYTE이며현재Core10BYTE와구별한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/saved-state-read-x86.tsv'
REPORT = ROOT/'cpppj/saved-state-read-native.json'


def scratch(profile):
    """짧은읽기가남겨둘명시초기물리를만들며부모기대는만들지않는다."""
    raw=bytearray([0 if profile==0 else 0xa5]*SCRATCH_BYTES)
    for offset,value in ((4,1),(8,0x22334455),(0,0x201),(0x3c,0x80000000),(0x40,0xa004e if profile else 0)):struct.pack_into('<I',raw,offset,value)  # 미완전파일에서도명시Player1입력으로물리범위를지킨다.
    for offset,name,width in ((0x48,b'StackA',32),(0x68,b'StackB',32),(0x88,b'StackFort',256)):
        raw[offset:offset+width]=bytes(width);raw[offset:offset+len(name)]=name  # 모든NUL과이웃padding은입력이다.
    return bytes(raw)


def image(profile,participants,tail,saved):
    """고정파일레이아웃의명시BYTE만작성하며파서나제품출력을참조하지않는다."""
    name=(b'SavedBattle',b'.Hidden',b'Has.Dot',b'',b'A'*255,b'Short')[profile]
    raw=bytearray(struct.pack('<I',0xa004e if profile!=5 else 0x12345678));raw.extend(name+bytes(256-len(name)));raw.extend(struct.pack('<III',1,saved,0x8899aabb))
    for index in range(8):  # 각레코드는정확히86BYTE이며원문은8회고정읽는다.
        owner=index+1;first=f'New{owner}'.encode();second=f'Address{owner}'.encode();flags=0x41 if index<participants else 0x201
        raw.extend(struct.pack('<II',owner,100+index));raw.extend(first+bytes(30-len(first)));raw.extend(second+bytes(30-len(second)));raw.extend(struct.pack('<II',flags,200+index));raw.extend(bytes((index*17+byte*13)&255 for byte in range(10)))
    raw.extend(struct.pack('<IIII',0x11223344,0x55667788,0x99aabbcc,tail));return bytes(raw)


def cases():
    """서명/점이름/참가자/잘림/외부변경입력만교차한다."""
    rows=[(present,profile,participants,tail,saved,976,0,'none') for present,profile,participants,tail,saved in itertools.product((0,1),range(6),(0,1,2,8),(0,1),(0,0x80000000))]
    for length,stack_profile in itertools.product((0,1,3,4,5,32,259,260,263,264,268,272,276,310,357,358,500,900,960,964,968,972,973,975), (0,1)):
        rows.append((1,0,2,0,123456789,length,stack_profile,'none'))  # read반환길이를무시하는원문과초기물리를함께관측한다.
    changes=('path1-stop','exists1-stop','open1-stop','read1-stop','read2-stop','pending1-stop','read3-stop','read6-stop','read12-stop','read61-stop','read65-stop','close1-stop',
             'init1-stop','init8-stop','metadata1-stop','metadata8-stop','read2-cursor976','read5-owner8','read6-baseB','metadata1-baseB','metadata4-baseA',
             'pending1-owner8','metadata1-owner8','metadata1-flags0','metadata8-flags0','metadata8-time0','close1-time0','read65-tail1','read65-tail0','read3-short0','read7-short2')
    for change in changes:rows.append((1,0,2,0,123456789,976,1,change))  # 각실제하위의1기준이벤트이름을쓴다.
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """본문/별도cleanup명령을실행하고파일·이름·Core초기화·캐시만명시경계로둔다."""
    def __init__(self):
        """FS페이지와직접쓰기/Player현재읽기관측을추가한다."""
        super().__init__();self.mu.mem_map(0,4096);self.total=self.normal=self.cleanups=0;self.boundaries=Counter();self.statuses=Counter();self.current=0
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED,self.unmapped)

    def fail(self):
        """호스트명시물리밖접근을원문정상0반환과구별한다."""
        self.status='fault';self.mu.emu_stop()

    def read(self,mu,access,address,size,value,data):
        """각레코드읽기뒤현재Playerbase를원문이읽는시점을기록한다."""
        if address==PLAYER_BASE:self.trace.append('B:'+str(BASES.index(self.words(PLAYER_BASE,1)[0])+1))

    def write(self,mu,access,address,size,value,data):
        """실제부모쓰기범위를FS/stack/실제출력전역/Player로한정한다."""
        allowed=((0,4),(STACK-0x2000,STACK+0x800),(OWNER,IDENTITY+4),(HANDLE,HANDLE+256),*((base,base+1620) for base in BASES),*((address,address+4) for address in TAILS))
        if not any(low<=address and address+size<=high for low,high in allowed):self.fail()

    def unmapped(self,mu,access,address,size,value,data):
        """잘못된원시포인터의실제실패만별도기록한다."""
        self.fail();return False

    def event(self,kind,text):
        """실제경계의1기준번호와선택인자순서를기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """외부callback이바꾼현재값만원문에공급한다."""
        change=self.case[-1]
        if '-' not in change:return
        trigger,action=change.split('-',1)
        if boundary!=trigger:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='cursor976':self.position=976
        elif action=='owner8':self.put(OWNER,8)
        elif action=='baseB':self.put(PLAYER_BASE,BASES[1])
        elif action=='baseA':self.put(PLAYER_BASE,BASES[0])
        elif action=='flags0':self.put(SCRATCH,0)
        elif action=='time0':self.put(SCRATCH+0x3c,0)
        elif action=='tail1':self.put(SCRATCH+0x34,1)
        elif action=='tail0':self.put(SCRATCH+0x34,0)
        elif action.startswith('short'):pass  # 해당read의복사폭은호출처에서먼저명시변경한다.
        else:raise RuntimeError('SavedState변경미해석:'+action)

    def instruction(self,mu,address,size,data):
        """772BYTE와cleanup6BYTE외의실제하위는명시입력으로응답한다."""
        self.total+=1;self.instructions+=1;self.current=address
        if self.instructions>30000:raise RuntimeError('SavedState명령상한')
        if 0x4b5890<=address<0x4b5b94 or 0x4fb5d0<=address<0x4fb5db:return
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,5)
        if address==0x4b5470:
            boundary=self.event('path','P');self.mutate(boundary)
            if self.status=='normal':self.ret(TEXT)
        elif address==0x4dd560:
            if args[0]!=TEXT:raise RuntimeError('SavedState존재경로차이')
            boundary=self.event('exists','E:!state.dat');self.mutate(boundary)
            if self.status=='normal':self.ret(self.case[0])
        elif address==0x41b4c0:
            if ecx!=FRAME+0x1c or args[:3]!=(TEXT,0,0):raise RuntimeError('SavedState파일open인자차이')
            boundary=self.event('open','O:!state.dat:0:0');self.mutate(boundary)
            if self.status=='normal':self.ret(ecx,12)
        elif address==0x41a7a0:
            target,width,limit=args[:3]
            if ecx!=FRAME+0x1c or limit!=0xffffffff:raise RuntimeError('SavedState실제read인자차이')
            count=min(width,max(0,len(self.data)-self.position));boundary='read'+str(self.calls['read']+1)
            if self.case[-1]==boundary+'-short0':count=0
            elif self.case[-1]==boundary+'-short2':count=min(count,2)
            if count:mu.mem_write(target,self.data[self.position:self.position+count])
            self.position+=count;label='S'+str(target-SCRATCH) if SCRATCH<=target<FRAME+0x198 else 'G'+str(((OWNER,)+TAILS).index(target)) if target in (OWNER,)+TAILS else 'bad'
            boundary=self.event('read','R:'+label+':'+str(width)+':'+str(count));self.mutate(boundary)
            if self.status=='normal':self.ret(count,12)
        elif address==0x4e63e0:
            source,character=args[:2]
            if character!=46:raise RuntimeError('SavedState실제점검사인자')
            result=0
            for offset in range(FRAME+0x198-source):  # 호스트물리끝을넘어선NUL없는stack는성공으로가장하지않는다.
                byte=mu.mem_read(source+offset,1)[0]
                if byte==46:result=source+offset;break
                if not byte:break
            else:self.fail();return
            self.ret(result)
        elif address==0x460bb0:
            name=self.string(args[0]);mu.mem_write(POOL+0x5000,name+b'\0');boundary=self.event('pending','N:'+name.hex());self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x42ce70:
            if ecx!=SCRATCH+0x24:raise RuntimeError('SavedStateCore초기화주소')
            mu.mem_write(ecx,bytes(10));boundary=self.event('init','I');self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x48f350:
            owner,identity=args[:2];core=bytes(mu.mem_read(esp+12,10))
            if owner>8:self.fail();return
            mu.mem_write(CACHE+owner*16,struct.pack('<I',identity)+core);boundary=self.event('metadata','M:'+str(owner)+':'+str(identity)+':'+core.hex());self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x41b410:
            if ecx!=FRAME+0x1c:raise RuntimeError('SavedState실제File해제주소')
            boundary=self.event('close','D');self.mutate(boundary)
            if self.status=='normal':self.ret()
        else:raise RuntimeError(f'SavedState관측밖명령{address:08x}')

    def sample(self,case,control):
        """현재전역/두Player/캐시/초기stack잔여/정상·SEH해제를제품없이기록한다."""
        self.case=case;self.trace=[];self.calls=Counter();self.position=0;self.status='normal';self.instructions=0
        present,profile,participants,tail,saved,length,initial,change=case;self.data=image(profile,participants,tail,saved)[:length]
        self.mu.mem_write(STACK-0x2000,bytes([0xa5])*0x2800);self.mu.mem_write(SCRATCH,scratch(initial));self.put(0,0);self.mu.mem_write(TEXT,b'!state.dat\0');self.mu.mem_write(POOL+0x5000,b'BeforeSaved\0'+bytes(244));self.mu.mem_write(HANDLE,b'BeforeHandle\0'+bytes(243));self.mu.mem_write(CACHE,bytes([0x91])*144)
        for address,value in ((PLAYER_BASE,BASES[0]),(OWNER,7),(IDENTITY,0x11223344),*((address,0xabababab) for address in TAILS)):self.put(address,value)  # 이전관측의현재상태는남기지않는다.
        for block_index,block in enumerate(BASES):  # 초기Player이름은새이름복사전기존handle보존순서를구별한다.
            self.mu.mem_write(block,bytes([0x31+block_index])*1620)
            for owner in range(9):self.mu.mem_write(block+owner*180+12,f'Old{block_index}{owner}'.encode()+b'\0')
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # CPU초기상태도명시한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<I',STOP))
        try:self.mu.emu_start(0x4b5890,STOP)
        except UcError:
            if self.status!='fault':raise
        body_status=self.status;result=self.mu.reg_read(UC_X86_REG_EAX) if body_status=='normal' else 0;cleanup=0
        if body_status=='normal':
            if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('SavedState스택복귀차이')
        elif body_status=='stop' and self.words(FRAME+0x1a0,1)[0]==0:
            self.status='normal';self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x400);self.mu.mem_write(STACK+0x400,struct.pack('<I',STOP));self.mu.emu_start(0x4fb5d0,STOP);cleanup=1;self.cleanups+=1
            if self.status!='normal':raise RuntimeError('SavedState별도cleanup명시중단')
        self.statuses[body_status]+=1;self.normal+=body_status=='normal';state=[self.words(address,1)[0] for address in (OWNER,IDENTITY,*TAILS)];state.extend((BASES.index(self.words(PLAYER_BASE,1)[0])+1,self.position,cleanup))
        raw=bytes(self.mu.mem_read(HANDLE,256))+bytes(self.mu.mem_read(CACHE,144))+b''.join(bytes(self.mu.mem_read(base,1620)) for base in BASES)
        return '\t'.join((str(control),','.join(map(str,case)),body_status,str(result),','.join(map(str,state)),self.string(POOL+0x5000).hex() or '-',raw.hex(),bytes(self.mu.mem_read(SCRATCH,SCRATCH_BYTES)).hex(),';'.join(self.trace) or '-'))


def main():
    """제품작성전최초PE자료를고정하고이후에는SHA확인만한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():  # 제품기대로최초자료를재생성하지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('SavedState최초SHA불일치:'+name)
        print('SavedState최초PE확인',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('SavedState최초관측덮어쓰기금지')
    oracle=Oracle();inputs=cases();lines=['# control / exists,profile,participants,tail,time,length,initialStack,mutation / status / EAX(normal only) / owner,id,threeTailGlobals,Playerbase,cursor,extraCleanup / pending / handle256+cache144+twoPlayer1620 / scratch392 / trace']
    for case in inputs:  # 두x87정밀도에서동일원시결과를독립대조한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('SavedState정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_saved_state_read.py','cpppj/tests/fixtures/saved-state-read-x86.tsv')
    report={'status':'최초PE고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entries':{'004b5890':772,'004fb5d0':6},'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'separateSehCleanups':oracle.cleanups,'statuses':dict(oracle.statuses),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),'precisionDifferences':0,'gameExecuted':False,'osExecuted':False,'scope':'실제772BYTE부모와필요시실제6BYTESEHcleanup. 파일경로/존재/open/read/close·CRT점검색·pending·Core초기화·메타데이터저장은명시하위이며짧은read와392BYTE초기stack를별도명시입력으로보존한다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','normalReturns','separateSehCleanups','statuses','instructions')}))


if __name__=='__main__':main()
