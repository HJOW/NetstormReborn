#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CoreWrite364BYTE의최초PE관측을제품작성전에고정한다.게임/OS는실행하지않는다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

# 부모가직접읽는실제주소이며장부/난수/Player를별도게임상태로대체하지않는다.
HEADERS, METAMAP, POST, EDITOR, OWNER, PLAYER_BASE, SEED = 0x5c89f0, 0x594fc8, 0x594fa4, 0x5c85a4, 0x540c70, 0x59534c, 0x5c89b4
PLAYER_BLOCKS = PLAYERS, PLAYERS + 0x1000  # callback뒤현재base와이미보관한Player주소를구별한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-core-write-x86.tsv'  # 최초출력은이후제품기대로덮어쓰지않는다.
REPORT = ROOT / 'cpppj/fort-core-write-native.json'  # 외부하위경계/최초근거SHA를구별한다.


def cases():
    """입력만준비하며원문분기·읽기순서·반환기대를생성하지않는다."""
    rows = [(slot,manager,meta,post,editor,1,0,0,'none') for slot,manager,meta,post,editor in itertools.product((-2,0,1,8,9),(0,11),(0,1),(0,1),(0,1))]
    for slot,seed,sequence in itertools.product((0,1,8,9),(0,1,0x12340000,0x8000ffff),(0,1,2)):
        rows.append((slot,11,0,0,0,slot if 1<=slot<=8 else 0,seed,sequence,'none'))  # 현재seed의상위와low16관문을명시입력에포함한다.
    mutations = ('assert1-manager22','assert1-post1','assert1-meta0','assert1-stop','assert2-post1','assert2-editor1','assert2-stop','assert3-playerB','assert3-core','assert3-stop',
                 'create-playerB','create-owner8','create-manager22','create-post1','create-editor1','create-stop',
                 'refresh-playerB','refresh-core','refresh-seed1','refresh-stop','write1-playerB','write1-core','write1-seed0','write1-seed12340000','write1-stop',
                 'prepare-seed1','prepare-seed80000000','prepare-playerB','prepare-stop','random1-seed80000000','random1-core','random1-stop',
                 'write2-seed1','write2-core','write2-playerB','write2-stop','sp1-core','sp1-playerB','sp1-stop','sp2-core','sp2-stop','sp3-stop')
    for mutation in mutations:
        rows.append((1,0 if mutation.startswith(('assert1-','assert2-')) else 11,1 if mutation.startswith('assert2-') else 0,0,0,7,0,0,mutation))  # 진단의순서와부분중단은원문기계어가결정한다.
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """364BYTE본문만실행하고Create/refresh/일반write/RNG/SP는명시장치로둔다."""
    def __init__(self):
        """보호PE복사와실제읽기쓰기trace를준비한다."""
        super().__init__();self.total=self.normal=0;self.boundaries=Counter();self.statuses=Counter()
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def read(self,mu,access,address,size,value,data):
        """CPU가직접재조회한현재global/manager만기록한다."""
        names={METAMAP:'metamap',POST:'post',EDITOR:'editor',OWNER:'owner',PLAYER_BASE:'players',SEED:'seed'}
        if address in names:
            raw=int.from_bytes(mu.mem_read(address,size),'little');value=(PLAYER_BLOCKS.index(raw)+1) if address==PLAYER_BASE else raw
            self.trace.append(names[address]+':'+str(size)+':'+str(value))
        elif HEADERS-40<=address<=HEADERS+180 and (address-HEADERS)%20==0:
            self.trace.append('manager:'+str(int.from_bytes(mu.mem_read(address,size),'little')))

    def write(self,mu,access,address,size,value,data):
        """부모의직접write는stack과같은seed의lowWORD밖으로나갈수없다."""
        if not (STACK-1024<=address<address+size<=STACK+4 or (address==SEED and size==2)):
            raise RuntimeError('CoreWrite부모쓰기범위밖:'+hex(address))

    def event(self,kind,text):
        """실제호출순서와같은종류의1기준번호를기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """명시callback이바꾸는현재입력만공급한다."""
        mutation=self.case[-1]
        if '-' not in mutation:return
        trigger,action=mutation.split('-',1)
        if trigger!=boundary:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action.startswith('manager'):self.put(HEADERS+self.case[0]*20,int(action[7:]))
        elif action.startswith('post'):self.put(POST,int(action[4:]))
        elif action.startswith('meta'):self.put(METAMAP,int(action[4:]))
        elif action.startswith('editor'):self.put(EDITOR,int(action[6:]))
        elif action.startswith('owner'):self.put(OWNER,int(action[5:]))
        elif action=='playerB':self.put(PLAYER_BASE,PLAYER_BLOCKS[1])
        elif action=='core':
            for block in PLAYER_BLOCKS:  # 앞서보관한주소와현재base양쪽에명시변경입력을둔다.
                for index in range(9):self.mu.mem_write(block+index*180+108,bytes((self.calls[boundary.rstrip('1234567890')]*23+index*17+byte*11)&255 for byte in range(10)))
        elif action.startswith('seed'):self.put(SEED,int(action[4:],16) if action[4:] in ('12340000','80000000') else int(action[4:]))
        else:raise RuntimeError('CoreWrite입력변경미해석:'+action)

    def instruction(self,mu,address,size,data):
        """부모명령은실행하고하위함수는선택인자와물리입력을관측한다."""
        self.total+=1;self.instructions+=1
        if self.instructions>10000:raise RuntimeError('CoreWrite명령상한')
        if 0x4beb30<=address<0x4bec9c:return
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,3)
        if address==0x4e0620:
            boundary=self.event('assert','A:'+str(args[2]));self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x44d1b0:
            boundary=self.event('create','C:'+str(ecx)+':'+self.string(args[0]).decode('ascii'));self.mutate(boundary)
            if self.status=='normal':self.ret(101,4)
        elif address==0x44fc00:
            boundary=self.event('refresh','F');self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x44cce0:
            source,width=args[:2];raw=bytes(mu.mem_read(source,width));self.output.extend(raw)
            boundary=self.event('write','W:'+str(ecx)+':'+str(width)+':'+raw.hex());self.mutate(boundary)
            if self.status=='normal':self.ret(0,8)
        elif address==0x4559d0:
            boundary=self.event('prepare','P');self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x4558c0:
            sequences=((0,65536,0x12348001),(0xffffffff,),(1,));sequence=sequences[self.case[7]];result=sequence[min(self.calls['random'],len(sequence)-1)]
            boundary=self.event('random','R:'+str(args[0])+':'+str(result));self.mutate(boundary)
            if self.status=='normal':self.ret(result)
        elif address==0x4be960:
            boundary=self.event('sp','S:'+str(args[0])+':'+str(args[1]));self.mutate(boundary)
            if self.status=='normal':self.ret()
        else:raise RuntimeError(f'CoreWrite관측밖명령{address:08x}')

    def sample(self,case,control):
        """현재global/고정Player입력/trace/부분출력을제품없이기록한다."""
        self.case=case;self.trace=[];self.calls=Counter();self.output=bytearray();self.status='normal';self.instructions=0
        slot,manager,meta,post,editor,owner,seed,sequence,mutation=case
        for address,value in ((HEADERS+slot*20,manager),(METAMAP,meta),(POST,post),(EDITOR,editor),(OWNER,owner),(PLAYER_BASE,PLAYER_BLOCKS[0]),(SEED,seed)):self.put(address,value)  # 다음관측에직전callback상태를남기지않는다.
        for block_index,block in enumerate(PLAYER_BLOCKS):  # 나머지Player필드와별개인명시10BYTE입력을준다.
            for index in range(9):self.mu.mem_write(block+index*180+108,bytes((block_index*79+index*17+byte*13)&255 for byte in range(10)))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # CPU의관측초기상태를고정한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,HEADERS+slot*20);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(0x4beb30,STOP)
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('CoreWrite스택복귀차이')
        self.normal+=self.status=='normal';self.statuses[self.status]+=1
        globals_state=[self.words(address,1)[0] for address in (HEADERS+slot*20,METAMAP,POST,EDITOR,OWNER,SEED)];globals_state.append(PLAYER_BLOCKS.index(self.words(PLAYER_BASE,1)[0])+1)
        cores=b''.join(bytes(self.mu.mem_read(block+index*180+108,10)) for block in PLAYER_BLOCKS for index in range(9))
        return '\t'.join((str(control),','.join(map(str,case)),self.status,','.join(map(str,globals_state)),cores.hex(),self.output.hex() or '-', ';'.join(self.trace) or '-'))


def main():
    """최초PE결과는한번만저장하고이후에는SHA확인만허용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():  # 제품기대로재생성하지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('CoreWrite최초SHA불일치:'+name)
        print('CoreWrite최초PE확인',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('CoreWrite최초fixture/보고서덮어쓰기금지')
    oracle=Oracle();inputs=cases();lines=['# control / slot,manager,metamap,post,editor,owner,seed,randomSequence,mutation / status / manager,metamap,post,editor,owner,seed,playerBase / 두9x10Core / 부분일반write / trace']
    for case in inputs:  # x87의두정밀도에서같은실제정수관측을대조한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('CoreWrite정밀도차이')
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_core_write.py','cpppj/tests/fixtures/fort-core-write-x86.tsv')
    report={'status':'최초PE고정·제품미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entries':{'004beb30':364},'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'statuses':dict(oracle.statuses),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),'precisionDifferences':0,'gameExecuted':False,'osExecuted':False,'scope':'실제CoreWrite364BYTE. Create/44fc00/일반writer/난수준비·Next/SP는명시외부장치이며부모의manager/metamap/post/editor/currentPlayer/seedlow16/직접Player재읽기·부분중단을관측한다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','normalReturns','statuses','instructions')}))


if __name__=='__main__':main()
