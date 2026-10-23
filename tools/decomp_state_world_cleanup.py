#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 State정리004b2df0 전체와 선택/정리48f3b0 자식을 제품 전에 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, TYPES, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW

# 모든 기록 범위는 관측 출력의 물리 폭이며 제품 기대를 계산하는 표가 아니다.
WORDS=(0x540bc0,0x540bc4,0x540bdc,0x540c70,0x540cac,0x541240,0x5424bc,0x542534,0x542538,0x54253c,0x542540,0x542544,0x5453e4,0x5467b8,0x5467bc,0x54c8a0,0x54db14,0x55a420,0x5949d4,0x594fa0,0x594fa4,0x594fb8,0x594fbc,0x594fc8,0x595088,0x59508c,0x595090,0x5950ac,0x5950b0,0x595344,0x59534c,0x595348,0x59a8ac,0x59a8b0,0x5c7a74,0x5c848c,0x5c8490,0x5c85a0,0x5c85a4,0x5c85b4,0x5c85c0,0x5c85c8,0x5c85cc,0x5c85d4,0x5c85e0,0x5c8758,0x5c89b4,0x5c8bb0,0x5ca8f0,0x5ca938)
DOUBLES=(0x565e68,0x5950b8,0x5950c0,0x5950c8,0x595358,0x5c85f0,0x5c85f8,0x5c8610,0x5c8618)
FIELDS=tuple(sorted([(address,4) for address in WORDS]+[(address,8) for address in DOUBLES]+[(0x5c84e8,88),(0x5953f0,10)]))
RANGES=((0x4b2df0,0x4b32d8),(0x48f3b0,0x48f42e))
PLAYER_BASE,CURRENT,CACHED=0x59534c,0x595350,0x5953f0
PLAYER_BUFFERS=(PLAYERS,PLAYERS+0x2000,PLAYERS+0x4000)
EXTERNAL=POOL+0x1000
ZERO_CALLS=(0x4e29c0,0x4e2a10,0x460e30,0x4aa970,0x446e70,0x4c1f20,0x460e70,0x42b1b0,0x43dad0,0x4bfb00,0x4795a0,0x445140,0x4d5c60,0x48fcd0,0x471af0,0x4666d0,0x4662b0,0x4c77c0,0x43b540,0x422490,0x4c2450,0x4437f0,0x469fc0,0x472830,0x4b2b50,0x4328a0,0x4ab940,0x462b20,0x4a9e50,0x4910a0,0x435220)
FIXTURE=ROOT/'cpppj/tests/fixtures/state-world-cleanup-x86.tsv'
REPORT=ROOT/'extracted/cpp-state-world-cleanup-20261024-native.json'


def cases():
    """기본관문·모드중복·선택된callback 현재값변경/중단만 명시하며 결과를 생성하지 않는다."""
    base=(165,1,0,0,0,0,14,1,1,1,6,2,10000,1,'none')
    rows=[]
    for reason,client,modes in itertools.product((0,1),(0,1),range(8)):
        value=list(base);value[1:4]=reason,client,modes;rows.append(tuple(value))
    for editor,post,state,header in itertools.product((0,1),(0,1),(14,29,33,4),(0,6)):
        value=list(base);value[3:7]=3,editor,post,state;value[10]=header;rows.append(tuple(value))
    for seed,owner,known,default in itertools.product((0,165),(1,8),(0,1),(-2147483648,2147483647)):
        value=list(base);value[0]=seed;value[7:9]=known,owner;value[12]=default;rows.append(tuple(value))
    changes=('4e2940.1-client1','42b1b0.1-template2','4bf9a0.1-template2','4bf9a0.1-battle1','4bf9a0.1-editor1','4bf880.1-owner8','413ae0.1-playerB','492240.1-owner8','447810.1-known0','43dad0.1-playerB','49a840.1-defaultNegative','49a840.1-state29','4c77c0.1-coreExternal','48f3b0.1-editor1','4910a0.1-known1','4bfd60.1-knownEditor','435220.1-counterMax','460d70.1-editor1')
    for mutation in changes:
        value=list(base);value[-1]=mutation
        if mutation.startswith('4bf9a0'):value[3]=1
        if mutation.startswith('4bf880'):value[3]=2
        rows.append(tuple(value))
    stops=(*ZERO_CALLS,0x4e2940,0x4e0620,0x460de0,0x4bf9a0,0x4bf880,0x413ae0,0x492240,0x447810,0x40edb0,0x49a840,0x48f3b0,0x4af170,0x46edf0,0x432360,0x4bfd60,0x460d70)
    for address in stops:
        value=list(base);value[-1]=f'{address:x}.1-stop'
        if address in (0x4e29c0,0x4e2a10):value[1]=0
        if address==0x4e0620:value[3]=3
        if address==0x4bf9a0:value[3]=1
        if address==0x4bf880:value[3]=2
        if address==0x4795a0:value[3]=4
        rows.append(tuple(value))
    for mutation in ('48f3b0.2-stop','4437f0.2-stop','460de0.2-stop'):
        value=list(base);value[-1]=mutation;rows.append(tuple(value))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """정리부모/기존Core자식만 CPU실행하고 UI·Kernel·Pool·SP·전송은 명시 하위 장치로 경계화한다."""
    def __init__(self):
        """새 PE복사와 독립 Player세벌·스택 외 실제 OS/게임에 진입하지 않는다."""
        super().__init__();self.running=False;self.status='normal';self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """부모/선택자식이쓸수있는같은전역·Player·TYPE·스택폭만 허용한다."""
        if not self.running:return
        allowed=(*((a,a+n) for a,n in FIELDS),*((a,a+1620) for a in PLAYER_BUFFERS),(CURRENT,CURRENT+4),(TYPES+0xc4,TYPES+0xc8),(STACK-8192,STACK+64))
        if not any(low<=address and address+size<=high for low,high in allowed):raise RuntimeError('State정리 예상밖쓰기:'+hex(address))

    def pointer(self,address):
        """실제 current가 가리키는 물리주소를 원문과 C++의 동일 alias 식별자로 대응한다."""
        if address==0:return '0'
        if address==CACHED:return 'CACHED'
        if address==EXTERNAL:return 'EXTERNAL'
        for index,base in enumerate(PLAYER_BUFFERS):
            for owner in range(9):
                if address==base+owner*180+108:return f'P{index}:{owner}'
        raise RuntimeError('State정리 current미해석:'+hex(address))

    def event(self,address,arguments=()):
        """현재진입인자/호출순서만기록하며제품기대나분기결과를계산하지않는다."""
        key=f'{address:x}';self.calls[key]+=1;self.trace.append('C:'+key+':'+','.join(map(str,arguments)));return key+'.'+str(self.calls[key])

    def mutate(self,boundary):
        """지정한하위효과뒤같은현재값을바꾸거나명시중단한다."""
        mutation=self.case[-1]
        if '-' not in mutation:return
        trigger,action=mutation.split('-',1)
        if trigger!=boundary:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='client1':self.put(0x5c85c0,1)
        elif action=='template2':self.put(0x540cac,2)
        elif action=='battle1':self.put(0x594fbc,1)
        elif action=='editor1':self.put(0x5c85a4,1)
        elif action=='owner8':self.put(0x540c70,8)
        elif action=='playerB':self.put(PLAYER_BASE,PLAYER_BUFFERS[1])
        elif action=='known0':self.put(0x595344,0)
        elif action=='known1':self.put(0x595344,1)
        elif action=='knownEditor':self.put(0x595344,1);self.put(0x5c85a4,1)
        elif action=='defaultNegative':self.put(0x540bdc,-2147483648)
        elif action=='state29':self.put(0x5c8758,29)
        elif action=='coreExternal':self.put(CURRENT,EXTERNAL)
        elif action=='counterMax':self.put(0x5949d4,0xffffffff)
        else:raise RuntimeError('State정리 미해석변경:'+action)

    def instruction(self,mu,address,size,data):
        """부모와Core선택은실제명령으로실행하며그밖실제하위는명시효과만기록한다."""
        self.instructions+=1
        if self.instructions>50000:raise RuntimeError('State정리 명령상한')
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x48f3b0:
            arguments=self.words(esp+4,2);boundary=self.event(address,arguments);self.mutate(boundary);return
        if any(low<=address<high for low,high in RANGES):return
        arguments=()
        if address in ZERO_CALLS:
            if address==0x460e30:self.pause-=1
            if address==0x4910a0:
                self.trace.append('BEFOREPLAYER:'+self.pointer(self.words(CURRENT,1)[0]));self.put(PLAYER_BASE,PLAYER_BUFFERS[2])
        elif address in (0x4e2940,0x413ae0,0x492240,0x4af170):arguments=self.words(esp+4,1)
        elif address in (0x447810,0x40edb0):arguments=self.words(esp+4,2)
        elif address in (0x4bf9a0,0x4bf880):arguments=((ecx-0x5c89f0)//20,)
        elif address in (0x46edf0,0x432360,0x4bfd60):arguments=(ecx,)
        elif address==0x4e0620:
            assertion,file,line=self.words(esp+4,3)
            if line!=0x77a or self.string(file)!=b'.\\State.cpp':raise RuntimeError('State정리 진단차이')
            arguments=(line,)
        elif address==0x460de0:arguments=(self.pause,)
        elif address==0x49a840:arguments=self.words(esp+4,1)
        elif address==0x460d70:
            target=self.words(esp+4,1)[0];mu.mem_write(target,struct.pack('<d',123.5))
        else:raise RuntimeError('State정리 관측밖명령:'+hex(address))
        boundary=self.event(address,arguments);self.mutate(boundary)
        if self.status!='normal':return
        if address==0x460de0:self.ret(arguments[0])
        elif address==0x49a840:self.ret(TYPES)
        elif address==0x460d70:self.ret(target)
        else:self.ret()

    def sample(self,case,control):
        """원시초기입력과실제부모의정상/부분중단메모리·전체호출trace를저장한다."""
        self.case=case;self.status='normal';self.instructions=0;self.calls=Counter();self.trace=[];self.running=False
        seed,reason,client,modes,editor,post,state,known,owner,template,header,pause,default,mission,mutation=case;self.pause=pause
        for address,count in FIELDS:self.mu.mem_write(address,bytes((seed+address+i*13)&255 for i in range(count)))
        for index,base in enumerate(PLAYER_BUFFERS):self.mu.mem_write(base,bytes((seed+index*47+i*17)&255 for i in range(1620)))
        self.mu.mem_write(EXTERNAL,bytes((seed+i*7+31)&255 for i in range(10)));self.put(CURRENT,PLAYER_BUFFERS[0]+180+108);self.put(PLAYER_BASE,PLAYER_BUFFERS[0]);self.put(TYPES+0xc4,0x3f123456)
        overrides={0x5c85c0:client,0x594fb8:modes&1,0x594fbc:(modes>>1)&1,0x594fc8:(modes>>2)&1,0x5c85a4:editor,0x594fa4:post,0x5c8758:state,0x595344:known,0x540c70:owner,0x540cac:template,0x540bdc:default,0x541240:152,0x594fa0:mission,0x5949d4:0xffffffff if seed else 7}
        for address,value in overrides.items():self.put(address,value)
        self.mu.mem_write(0x5c89f0,bytes((seed+i*3)&255 for i in range(180)))
        for selected in range(9):self.put(0x5c89f0+selected*20,0x11110000+selected if header&(1<<selected) else 0)
        self.mu.mem_write(STACK-8192,bytes([seed])*(8192+64))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.mem_write(STACK,struct.pack('<II',STOP,reason));self.running=True
        self.mu.emu_start(0x4b2df0,STOP);self.running=False
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('State정리 원문스택차이')
        raw=''.join(bytes(self.mu.mem_read(address,count)).hex() for address,count in FIELDS)
        players=[bytes(self.mu.mem_read(base,1620)).hex() for base in PLAYER_BUFFERS]
        return '\t'.join((str(control),','.join(map(str,case)),self.status,str(self.pause),self.pointer(self.words(CURRENT,1)[0]),raw,*players,bytes(self.mu.mem_read(TYPES+0xc4,4)).hex(),bytes(self.mu.mem_read(0x5c89f0,180)).hex(),';'.join(self.trace),str(self.instructions)))


def main():
    """최초관측이제품보다앞섰음을확인하고이후에는SHA검증만한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative,expected in report['sha256'].items():assert digest(ROOT/relative)==expected,relative
        print('State정리 최초SHA확인',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('State정리 최초자료 덮어쓰기금지')
    for name in ('cpppj/src/o/RawStateWorldCleanup.h','cpppj/src/o/RawStateWorldCleanup.cpp','cpppj/tests/StateWorldCleanupTests.cpp'):
        if (ROOT/name).exists():raise RuntimeError('State정리 제품전관측요건위반')
    inputs=cases();oracle=Oracle();lines=['# fields='+','.join(f'{address:x}.{count}' for address,count in FIELDS),'# control / input15 / status / pause / current / globals / playerA / playerB / playerC / type+c4 / headers180 / trace / instructions'];statuses=Counter();total=0
    for case in inputs:
        for control in (0x27f,0x37f):
            if oracle.status!='normal':oracle=Oracle()
            lines.append(oracle.sample(case,control));statuses[oracle.status]+=1;total+=oracle.instructions
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','tools/decomp_state_world_cleanup.py','cpppj/tests/fixtures/state-world-cleanup-x86.tsv')
    report={'scope':'PATCH1256BYTE정리부모와기존Core선택자식실제PE/그밖하위는명시필수경계','lastDecompileHost':'HJOW-Athlon / 2026-10-23','host':socket.gethostname(),'patchAddress':'004b2df0','patchBytes':1256,'cdSourceMapAddress':'0042f5c0','cdExecuted':False,'ranges':RANGES,'fields':FIELDS,'uniqueInputs':len(inputs),'observations':len(lines)-2,'statuses':dict(statuses),'instructions':total,'productAbsentBeforeObservation':True,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','statuses','instructions')},ensure_ascii=False))


if __name__=='__main__':main()
