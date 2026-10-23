#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Chatmanager 생성/초기화/랭크formatter/prefix setter/갱신 원문을 제품 작성 전에 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UcError, UC_HOOK_INTR, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ_UNMAPPED
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

# 실제 함수 범위와 독립 가상 할당/현재 전역 주소다. 게임/OS로 진입하지 않는다.
RANGES = ((0x42e000,0x42e04e),(0x42e050,0x42e1d9),(0x42d620,0x42d63d),(0x42f480,0x42f533),(0x42f540,0x42f582))
MANAGERS, BUFFERS = (POOL+0x1000,POOL+0x4000),(POOL+0x8000,POOL+0x9000)
OUTPUT, SOURCE, CORE = POOL+0x10000,POOL+0x11000,POOL+0x13000
CURRENT, ACTIVE, SUBSCRIBER = 0x548714,0x595348,0x540c74
MANAGER_BYTES = 0xa98  # manager 전체를 비교해 생성자가 쓰지 않는 BYTE도 검증한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-manager-prefix-x86.tsv'
REPORT = ROOT/'extracted/cpp-chat-manager-prefix-20261024-native.json'


def signed(value):
    """unsigned 입력의 원문 signed DWORD 해석이다."""
    return struct.unpack('<i',struct.pack('<I',value&0xffffffff))[0]


def cases():
    """기대 결과 없이 현재 전역/할당 실패/서식 경계/부수효과 입력만 만든다."""
    rows=[]
    for seed,allocation in itertools.product((0,165),(0,2)):  # constructor는 내부0x80 실패 진단 뒤 부분 쓰기도 관측한다.
        rows.append((0,seed,2,1,3,1,allocation,3,31,1,0,0,'none'))
    for seed,subscriber,front,back in itertools.product((0,165),(0,0xffffffff,0x80000000),(0,59,60,61),(0,499,500)):  # 전체 signed 키와 각 strncpy 끝을 독립 입력한다.
        rows.append((1,seed,2,1,3,subscriber,0,3,31,1,front,back,'none'))
    for allocation,repeat,front,back in itertools.product((0,1,2,3),(1,2),(-1,61),(-1,500)):  # 누락 ReadString과 재초기화/실패를 구별한다.
        rows.append((1,165,1,1,3,7,allocation,3,31,repeat,front,back,'none'))
    for rank,minor,maximum in itertools.product((-2147483648,-1,0,1,2,3,4,5,15,16,255,2147483647),(-1,0,1,3,16),(-1,31)):  # 실제 signed clamp/진단/idiv와 호출 횟수를 관측한다.
        rows.append((3,165,1,1,rank,1,0,minor,maximum,1,0,0,'none'))
    for current,active,rank,minor,maximum in itertools.product((0,1),(0,1),(0,1,255),(0,3),(5,31)):  # 미선택 getter/서식/쓰기의 자연 생략을 명확히 한다.
        rows.append((4,165,current,active,rank,1,0,minor,maximum,1,0,0,'none'))
    for seed,length in itertools.product((0,165),(0,1,255,256,511,2131,2132)):  # setter에는 의미상256절단이 없으며 물리 마지막 쓰기 실패도 남긴다.
        rows.append((2,seed,1,1,length,1,0,3,31,1,0,0,'none'))
    for change in ('alias-forward','alias-backward'):  # 실제 한 BYTE 순방향 alias 복사와 물리 중단을 구별한다.
        rows.append((2,165,1,1,4,1,0,3,31,1,0,0,change))
    for change in ('new1-manager2','new1-stop','new2-manager2','new2-stop','assert1-manager2','assert1-stop','command1-subscriber9','command2-manager2','command3-stop','read1-subscriber9','read1-manager2','read1-stop','read2-manager2','read2-manager0','read2-stop','strncpy1-manager2','strncpy1-stop','strncpy2-manager1','strncpy2-stop'):
        rows.append((1,165,1,1,3,7,2 if change.startswith('assert') else 0,3,31,1,61,500,change))
    for change in ('max1-maximum0','max1-maximumNegative','max1-manager2','max2-stop','minor1-stop','assert1-minor3','assert1-stop','append1-manager2','append1-stop','core1-manager2','core1-stop'):
        rows.append((4,165,1,1,255,1,0,0 if change.startswith('assert') else 3,31,1,0,0,change))
    return list(dict.fromkeys(rows))


def pattern(seed,size,offset=0):
    """비0 미초기화 영역의 명시 원시 입력이며 제품 출력과 무관하다."""
    return bytes((seed+index*17+offset)&255 for index in range(size))


class Oracle(MemoryOracle):
    """다섯 부모만 실행하고 할당/Config/명령등록/CRT 서식·복사/타입조회는 명시 장치로 둔다."""
    def __init__(self):
        """PE 복사와 독립 스택/힙 외 CPU 쓰기를 제한한다."""
        super().__init__();self.mu.mem_map(0,0x1000);self.total=0;self.statuses=Counter();self.boundaries=Counter();self.running=False;self.current_instruction=0
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED,self.unmapped);self.mu.hook_add(UC_HOOK_INTR,self.interrupt)

    def interrupt(self,mu,number,data):
        """실제 CPU idiv의 interrupt0을 관측하고 OS 처리기로 진입하지 않는다."""
        if number!=0 or self.current_instruction!=0x42f4e7:raise RuntimeError('Chatmanager 예상밖 CPU interrupt:'+str(number))
        self.fault()

    def fault(self):
        """원문 물리 접근/정수 나눗셈 실패를 정상 반환과 구별한다."""
        self.status='fault';self.mu.emu_stop()

    def write(self,mu,access,address,size,value,data):
        """스택/SEH/current/명시 manager/출력 외 실제 CPU 쓰기는 거부한다."""
        if not self.running or self.status!='normal':return
        allowed=((0,4),(STACK-8192,STACK+64),(CURRENT,CURRENT+4),(OUTPUT,OUTPUT+256),*((base,base+MANAGER_BYTES) for base in MANAGERS))
        if not any(low<=address and address+size<=high for low,high in allowed):self.fault()

    def unmapped(self,mu,access,address,size,value,data):
        """current0 뒤 진단 복귀의 실제 null 접근도 부분 효과와 함께 보존한다."""
        self.fault();return False

    def event(self,kind,text):
        """원문 장치 진입 인자/현재 포인터/순서만 기록한다."""
        self.calls[kind]+=1;self.boundaries[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """외부 장치의 명시 현재값 변경만 적용하며 부모 기대를 계산하지 않는다."""
        change=self.case[-1]
        if '-' not in change:return
        trigger,action=change.split('-',1)
        if trigger!=boundary:return
        if action=='stop':self.status='stop';self.mu.emu_stop()
        elif action=='manager0':self.put(CURRENT,0)
        elif action=='manager1':self.put(CURRENT,MANAGERS[0])
        elif action=='manager2':self.put(CURRENT,MANAGERS[1])
        elif action=='subscriber9':self.put(SUBSCRIBER,9)
        elif action=='maximum0':self.maximum=0
        elif action=='maximumNegative':self.maximum=-1
        elif action=='minor3':self.minor=3
        else:raise RuntimeError('Chatmanager 변경 미해석:'+action)

    def normalized(self,address):
        """진단 trace에서 실제 가상 주소를 고정 manager/버퍼/출력 식별자로 바꾼다."""
        if address==0:return '0'
        for index,base in enumerate(MANAGERS):  # 실제 manager 안의 offset까지 보존한다.
            if base<=address<base+MANAGER_BYTES:return f'M{index+1}+{address-base}'
        for index,base in enumerate(BUFFERS):  # 내부 할당과 manager 할당을 구별한다.
            if address==base:return f'B{index+1}'
        if address==OUTPUT:return 'OUT'
        return hex(address)

    def instruction(self,mu,address,size,data):
        """부모 계산/직접 복사는 원문 명령으로 실행하고 외부 장치만 명시 응답한다."""
        self.instructions+=1;self.total+=1;self.current_instruction=address
        if self.instructions>100000:raise RuntimeError('Chatmanager 원문 명령 상한')
        if any(low<=address<high for low,high in RANGES):return
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e4391:
            amount=self.words(esp+4,1)[0]
            if amount==MANAGER_BYTES:
                self.allocations+=1;result=0 if self.case[6]&1 else MANAGERS[(self.allocations-1)%2]
            elif amount==128:result=0 if self.case[6]&2 else BUFFERS[max(0,self.allocations-1)%2]
            else:raise RuntimeError('Chatmanager 예상밖 할당:'+str(amount))
            boundary=self.event('new','N:'+str(amount)+':'+self.normalized(result));self.mutate(boundary)
            if self.status=='normal':self.ret(result)
        elif address==0x4e0620:
            assertion,file,line=self.words(esp+4,3);boundary=self.event('assert','A:'+str(line)+':'+self.string(assertion).decode('ascii'));self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x43f660:
            name,callback,flags=self.words(esp+4,3);boundary=self.event('command','C:'+self.string(name).decode('ascii')+':'+hex(callback)+':'+str(flags));self.mutate(boundary)
            if self.status=='normal':self.ret()
        elif address==0x4e58e9:
            target,formatting,value,suffix=self.words(esp+4,4)
            if self.string(formatting)!=b'%d%s':raise RuntimeError('Chatmanager key 서식 차이')
            text=str(signed(value)).encode('ascii')+self.string(suffix);mu.mem_write(target,text+b'\0');self.ret(len(text))
        elif address==0x441300:
            key,target=self.words(esp+4,2);ordinal=self.calls['read']%2;length=self.case[10+ordinal];text=bytes([70+ordinal])*max(0,length)
            boundary=self.event('read','R:'+self.string(key).decode('ascii')+':'+str(length))
            if length>=0:mu.mem_write(target,text+b'\0')
            self.mutate(boundary)
            if self.status=='normal':self.ret(int(length>=0))
        elif address==0x4e51f0:
            target,source,count=self.words(esp+4,3);text=self.string(source);copied=text[:count].ljust(count,b'\0')
            boundary=self.event('strncpy','P:'+self.normalized(target)+':'+str(count))
            if not any(base<=target and target+count<=base+MANAGER_BYTES for base in MANAGERS):self.fault();return
            mu.mem_write(target,copied);self.mutate(boundary)
            if self.status=='normal':self.ret(target)
        elif address==0x4c6c20:
            result=self.maximum;boundary=self.event('max','X:'+str(result));self.mutate(boundary)
            if self.status=='normal':self.ret(result)
        elif address==0x4c6c00:
            result=self.minor;boundary=self.event('minor','I:'+str(result));self.mutate(boundary)
            if self.status=='normal':self.ret(result)
        elif address==0x4a8a00:
            target,formatting,value=self.words(esp+4,3);format_bytes=self.string(formatting);text=self.string(target)+format_bytes.replace(b'%d',str(signed(value)).encode('ascii'))
            if format_bytes not in (b'~[Iicon.C%d]',b'~[Iicon.B%d]'):raise RuntimeError('Chat 랭크 append 서식 차이')
            mu.mem_write(target,text+b'\0');boundary=self.event('append','F:'+format_bytes.decode('ascii')+':'+str(signed(value)));self.mutate(boundary)
            if self.status=='normal':self.ret(target)
        elif address==0x48f0d0:
            boundary=self.event('core','K');self.mutate(boundary)
            if self.status=='normal':self.ret(CORE)
        else:raise RuntimeError(f'Chatmanager 관측밖 명령 {address:08x}')

    def sample(self,case,control):
        """각 입력의 전체 객체/내부할당/출력과 부분 중단을 실제 CPU 관측으로 고정한다."""
        self.case=case;self.calls=Counter();self.boundaries=Counter();self.trace=[];self.status='normal';self.instructions=0;self.allocations=0;self.minor=case[7];self.maximum=case[8]
        self.running=False;self.put(CURRENT,0 if not case[2] else MANAGERS[case[2]-1]);self.put(ACTIVE,case[3]);self.put(SUBSCRIBER,case[5]);self.put(0,0x11223344);self.put(CORE+4,case[4],1)
        for index,base in enumerate(MANAGERS):self.mu.mem_write(base,pattern(case[1],MANAGER_BYTES,index*59))  # 전체 미작성 BYTE도 입력에서 구별한다.
        for index,base in enumerate(BUFFERS):self.mu.mem_write(base,pattern(case[1],128,index*71+13))  # 내부 new0x80은 constructor가 초기화하지 않는다.
        self.mu.mem_write(OUTPUT,bytes([case[1]])*256);source=SOURCE
        if case[0]==2:
            length=case[4];text=bytes([65])*length+b'\0';self.mu.mem_write(SOURCE,text)
            if case[-1].startswith('alias'):
                source=MANAGERS[0]+(0x243 if case[-1]=='alias-forward' else 0x245);self.mu.mem_write(source,b'ABCD\0')
        entry=(0x42e000,0x42e050,0x42d620,0x42f480,0x42f540)[case[0]]
        for invocation in range(case[9]):  # 두 초기화는 현재 manager/이전 할당을 남기고 같은 실제 전역을 이어 쓴다.
            self.mu.mem_write(STACK-8192,bytes([case[1]])*(8192+64))
            for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 매 호출 레지스터 입력을 고정한다.
            self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,MANAGERS[0]);self.mu.reg_write(UC_X86_REG_FPCW,control)
            arguments=[source] if case[0]==2 else [OUTPUT,case[4]&0xffffffff] if case[0]==3 else []
            self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(arguments)+1),STOP,*arguments));self.running=True
            try:self.mu.emu_start(entry,STOP)
            except UcError:
                if self.current_instruction==0x42f4e7 or self.status=='fault':self.status='fault'
                else:raise
            self.running=False
            if self.status!='normal':break
            expected=8 if case[0]==2 else 4
            if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+expected:raise RuntimeError('Chatmanager 원문 스택 복구 차이')
        self.statuses[self.status]+=1
        current=self.words(CURRENT,1)[0];state=(0 if not current else MANAGERS.index(current)+1,self.words(SUBSCRIBER,1)[0],self.allocations)
        objects=[bytes(self.mu.mem_read(base,MANAGER_BYTES)).hex() for base in MANAGERS]
        buffers=[bytes(self.mu.mem_read(base,128)).hex() for base in BUFFERS]
        return '\t'.join((str(control),','.join(map(str,case)),self.status,','.join(map(str,state)),bytes(self.mu.mem_read(OUTPUT,256)).hex(),*objects,*buffers,';'.join(self.trace) or '-',str(self.instructions)))


def main():
    """최초 원문만 생성하고 이후에는 SHA 검증으로 동결 자료를 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative,expected in record['sha256'].items():  # 제품 결과로 최초 원문을 다시 쓰지 않는다.
            assert digest(ROOT/relative)==expected,relative
        print('Chatmanager 최초 SHA 확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('Chatmanager 최초 fixture/보고서 덮어쓰기 금지')
    for relative in ('cpppj/src/o/RawChatManagerPrefix.h','cpppj/src/o/RawChatManagerPrefix.cpp','cpppj/tests/ChatManagerPrefixTests.cpp'):  # 제품 작성보다 최초 관측이 먼저인지 명시 확인한다.
        if (ROOT/relative).exists():raise RuntimeError('Chatmanager 최초 PE 관측 전에 제품이 존재함:'+relative)
    oracle=Oracle();inputs=cases();total=0;statuses=Counter();boundaries=Counter();lines=['# control / mode,seed,current,active,rankOrLength,subscriber,allocation,minor,maximum,repeat,frontLength,backLength,mutation / status / current,subscriber,allocations / output256 / managerA2712 / managerB2712 / bufferA128 / bufferB128 / trace / instructions']
    for case in inputs:  # 실제 parent를 양 x87 정밀도에서 실행하며 기대를 계산하지 않는다.
        for control in (0x27f,0x37f):
            if getattr(oracle,'status','normal')!='normal':oracle=Oracle()  # CPU 예외/중단의 내부 exception 상태를 다음 독립 입력으로 전파하지 않는다.
            lines.append(oracle.sample(case,control));total+=oracle.instructions;statuses[oracle.status]+=1;boundaries.update(oracle.boundaries)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_chat_manager_prefix.py','cpppj/tests/fixtures/chat-manager-prefix-x86.tsv')
    record={'scope':'독립 PE 최초 관측/게임·OS·GUI 아님','lastDecompileHost':'HJOW-Athlon / 2026-10-23','host':socket.gethostname(),'ranges':RANGES,'uniqueInputs':len(inputs),'observations':len(lines)-1,'statuses':dict(statuses),'instructions':total,'boundaries':dict(boundaries),'productSourcesAbsentBeforeObservation':True,'freshEngineAfterFaultOrStop':True,'wholeManagerBytes':MANAGER_BYTES,'wholeInternalBufferBytes':128,'rankOutputBytes':256,'currentCoreGetterAndRankCountsAreExplicitBoundaries':True,'crtFormattingAndStrncpyAreExplicitBoundaries':True,'sha256':{relative:digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','statuses','instructions')},ensure_ascii=False))


if __name__=='__main__':main()
