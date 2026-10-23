#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 단일 객체/청크 저장과 실제 CRT 절삭을 새 독립 TSV로 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 허용한 새 산출물의 저장소 루트다.
PE_PATH=ROOT/'originals/Netstorm.exe'  # 변경하지 않는 보호 입력이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 확정한10.78 원문 식별자다.
OBJECT_PATH=ROOT/'cpppj/tests/fixtures/fort-object-save-x86.tsv'  # 단일객체 실제관측이다.
CHUNK_PATH=ROOT/'cpppj/tests/fixtures/fort-chunk-save-x86.tsv'  # 청크 실제관측이다.
REPORT_PATH=ROOT/'extracted/cpp-fort-object-save-20261023-native.json'  # 경계/명령/해시 보고서다.
BASE=0x400000  # 원래 PE 이미지 기준 주소다.
HEAP=0x1000000  # 명시 장치와 실제 출력의 격리 주소다.
NODE=HEAP+0x100  # 고정16BYTE MemStream 헤더다.
PAYLOADS=(HEAP+0x1000,HEAP+0x6000)  # 현재 data 변경을 구별하는 두 물리 출력이다.
BUFFER_SIZE=16384  # 부분 쓰기에도 지키는 실제 물리 상한이다.
FORMS=HEAP+0x10000  # 원본50BYTE stride의 고정 Pool이다.
VTABLE=HEAP+0x18000  # 실제 가상 CALL이 읽는 명시 표다.
ROBUST=HEAP+0x19000  # 명시 가상60 경계 주소다.
LEVEL=HEAP+0x19010  # 명시 가상80 경계 주소다.
RETURN=HEAP+0x1F000  # 최상위 RET의 격리 종료점이다.
STACK=0x2100000  # 독립 cdecl 진입 ESP다.
POOL_GLOBAL=0x5C8464  # 본체가 다시 읽는 원본 Pool 기준 포인터다.
LIMIT_GLOBAL=0x5C847C  # 청크가 후보마다 읽는 현재 Pool 개수다.
GLOBAL_FIELDS=('battle','mission','editor','scope','version','localOwner','bridgeType','daisType','battleMask','ownerMask')  # 객체 입력/출력의 전역 순서다.
GLOBALS=dict(zip(GLOBAL_FIELDS,(0x594FBC,0x594FA0,0x5C85A4,0x5C89B0,0x5C89C0,0x540C70,0x5411A0,0x5412EC,0x54263C,0x542644)))  # 실제 DWORD 주소들이다.
DEVICE_FIELDS=('flags1','flags2','robust','island','hasLevel','level')  # 외부 장치의 현재 반환 상태다.
OBJECT_RANGE=(0x4BD940,0x4BDC54)  # 단일객체 본체와 정상 RET다.
CHUNK_RANGE=(0x4BE020,0x4BE151)  # 두 청크 RET를 포함한다.
FTOL_RANGE=(0x4E49C0,0x4E4A35)  # 실제 CRT signed64 절삭 본체다.
WRITE_RANGE=(0x44CCE0,0x44CD40)  # 청크 marker를 쓰는 실제 MemStream 본체다.
BOUNDARIES={0x4AC1E0:'flags1',0x4AC200:'flags2',ROBUST:'robust',0x46F6E0:'island',0x4AD0A0:'hasLevel',LEVEL:'level',0x4BCEA0:'contents',0x44CCE0:'write',0x4E0620:'assert',0x41DEF0:'begin',0x4B1810:'next',0x4BD940:'object'}  # 선택 본체 밖에서만 명시 대체한다.
CONTROLS=(0x27F,0x37F)  # 두 x87 제어어에서 각각 온전한 함수를 실행한다.
OBJECT_SCHEMA=('name','sid','originX','originY','object','flags1','flags2',*GLOBAL_FIELDS,'robust','island','hasLevel','level','position','end','capacity','mutation','completed','result','streamout','payload0','payload1','objectout','globalsout','devicesout','trace','stackdelta','nativeinstructions')  # 앞25열입력/뒤11열관측이다.
CHUNK_SCHEMA=('name','chunkX','chunkY','poollimit','sequence','returns','position','end','capacity','mutation','completed','streamout','payload0','payload1','poollimitout','trace','result','stackdelta','nativeinstructions')  # 앞10열입력/뒤9열관측이다.


def digest(path):
    """보호 입력/새 산출물의 SHA256을 변경 없이 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """명시 float32 입력의 실제 DWORD 비트를 얻는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def form(x=2.9,y=3.9,type=71,owner=1,flags=0,qa=0xABCD,frame=0x12345678):
    """제품 결과와 독립적인 실제50BYTE 슬롯 입력을 만든다."""
    raw=bytearray([0xA5]*50)  # 저장하지 않는 주변 필드도 구별할 원문이다.
    struct.pack_into('<I',raw,0,0);raw[10]=type;raw[34]=owner;raw[40]=flags
    struct.pack_into('<H',raw,12,qa);struct.pack_into('<I',raw,14,bits(x));struct.pack_into('<I',raw,18,bits(y));struct.pack_into('<I',raw,36,frame)
    return raw.hex()


class Oracle:
    """선택한 원문/CRT/marker명령과 명시 외부 장치를 분리해 실행한다."""
    def __init__(self):
        """읽기 전용 보호 PE와 격리 스택/버퍼를 준비한다."""
        self.pe=pefile.PE(str(PE_PATH))  # 보호 입력 파서다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 실제32비트 명령 실행기다.
        self.size=(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095  # 페이지 정렬한 원문 크기다.
        self.mu.mem_map(BASE,self.size);self.mu.mem_write(BASE,self.pe.get_memory_mapped_image());self.mu.mem_map(HEAP,0x30000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.memory_write)
        self.original={first:bytes(self.mu.mem_read(first,last-first)) for first,last in (OBJECT_RANGE,CHUNK_RANGE,FTOL_RANGE,WRITE_RANGE)}  # 실제 본체 불변 확인용이다.
        self.totals={};self.boundaries=Counter();self.writes=Counter()  # 정밀도별명령/외부CALL/원문쓰기를집계한다.
        self.buffer_hashes=[]  # 실행별 전체 출력의 식별자다.

    def number(self,address,value=None,width=4):
        """비정렬 raw 값을 읽거나 명시 입력/외부효과만 쓴다."""
        if value is not None:self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def returned(self,value=0,cleanup=0):
        """실제 CALL의 복귀주소와 thiscall 인자만 소비한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 실제 외부 진입 ESP다.
        target=self.number(esp)  # CPU가 보관한 복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX,value&0xFFFFFFFF);self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EIP,target)

    def event(self,name,text):
        """외부 CALL의 실제 순서를 적고 종류별1기반 효과번호를 반환한다."""
        self.calls[name]+=1;self.trace.append(text);self.boundaries[name]+=1
        return name+str(self.calls[name])

    def mutate(self,event):
        """현재 경계에 명시한 변경만 작성 순서대로 공급한다."""
        for row in self.effects:  # 같은 경계의 여러 효과를 순서대로 적용한다.
            trigger,target,field,text=row  # 시점/대상/필드/십진수 값이다.
            if trigger!=event:continue
            value=int(text)  # 명시 원시 변경 값이다.
            if target=='global':self.number(LIMIT_GLOBAL if field=='poollimit' else GLOBALS[field],value)
            elif target=='device':self.devices[field]=value&0xFFFFFFFF
            elif target=='form':
                offset,width={'type':(10,1),'owner':(34,1),'flags':(40,1),'qa':(12,2),'frame':(36,4),'x':(14,4),'y':(18,4)}[field]  # 실제 후보 raw 필드다.
                self.number(self.form_address+offset,value,width)
            elif target=='stream':self.number(NODE+{'data':0,'position':4,'end':8,'capacity':12}[field],PAYLOADS[value] if field=='data' else value)
            elif target=='finder' and field=='current':self.number(self.finder+0x34,value)
            elif target=='control' and field=='stop' and value==1:self.stopped=True;self.mu.emu_stop()
            else:raise RuntimeError('미명시 callback 효과')

    def memory_write(self,mu,access,address,size,value,data):
        """실제 CPU 쓰기를 스택/고정stream/두물리출력으로 제한한다."""
        if STACK-0x2000<=address and address+size<=STACK+20:category='stack'
        elif NODE<=address and address+size<=NODE+16:category='stream'
        elif any(first<=address and address+size<=first+BUFFER_SIZE for first in PAYLOADS):category='payload'
        else:raise RuntimeError(f'미명시 원문 쓰기 {address:08x}/{size}')
        self.writes[category]+=1

    def append(self,raw):
        """명시 하위 장치만 현재data/pos에 입력bytes를 쓰고 논리끝을 진행한다."""
        position=self.number(NODE+4);pointer=self.number(NODE)  # 외부장치가쓰는현재실제주소다.
        if pointer not in PAYLOADS or position+len(raw)>BUFFER_SIZE:raise RuntimeError('장치의물리출력범위')
        self.mu.mem_write(pointer+position,raw);self.number(NODE+4,position+len(raw));self.number(NODE+8,max(self.number(NODE+8),position+len(raw)))

    def assertion(self,line):
        """원문 진단의 도달과 복귀/명시 중단만 공급한다."""
        self.mutate(self.event('assert','assert:'+str(line)))

    def instruction(self,mu,address,size,data):
        """본체/CRT/청크marker명령을 실행하고 지정 하위만 명시 대체한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        ranges=(OBJECT_RANGE,FTOL_RANGE) if self.kind=='object' else (CHUNK_RANGE,FTOL_RANGE,WRITE_RANGE)  # 이번실행의실제명령범위다.
        if any(first<=address<last for first,last in ranges):
            self.instructions+=1
            if FTOL_RANGE[0]<=address<FTOL_RANGE[1]:self.crt_instructions+=1
            if self.kind=='chunk' and address==WRITE_RANGE[0]:self.trace.append('write:1:63')  # 실제marker Write의순수진입관측이며대체/효과를주지않는다.
            return
        if address not in BOUNDARIES:raise RuntimeError(f'미명시 코드 {address:08x}')
        name=BOUNDARIES[address]  # 실제 진입한 하위 장치다.
        esp=mu.reg_read(UC_X86_REG_ESP)  # 원문이 전달한 인자 스택이다.
        if name in ('flags1','flags2','robust','hasLevel','level'):
            sid=(mu.reg_read(UC_X86_REG_ECX)-FORMS)//50  # 실제 호출의 고정 객체SID다.
            label=f'{name}:{sid}'+(':'+str(self.number(self.form_address+10,width=1)) if name.startswith('flags') else '')  # flags getter가그시점에본실제type다.
            answer=self.devices[name]  # 가상반환값은효과전에캡처한다.
            self.mutate(self.event(name,label))
            if name.startswith('flags'):answer=self.devices[name]  # 명시타입getter만변경뒤현재DWORD를반환한다.
            if not self.stopped:self.returned(answer)
        elif name=='island':
            x=self.number(esp+4);y=self.number(esp+8);answer=self.devices['island']  # 실제float32인자비트와캡처반환값이다.
            self.mutate(self.event(name,f'island:{x}:{y}'))
            if not self.stopped:self.returned(answer,8)
        elif name=='write':
            if mu.reg_read(UC_X86_REG_ECX)!=NODE:raise RuntimeError('객체고정stream불일치')
            raw=bytes(mu.mem_read(self.number(esp+4),self.number(esp+8)))  # caller가진단전에준비한실제값이다.
            event=self.event(name,'write:'+str(len(raw))+':'+raw.hex())  # 장치진입은용량진단보다먼저기록한다.
            if struct.unpack('<i',struct.pack('<I',self.number(NODE+12)))[0]<struct.unpack('<i',struct.pack('<I',(self.number(NODE+4)+len(raw))&0xFFFFFFFF))[0]:self.assertion(36)
            if self.stopped:return
            self.append(raw);self.mutate(event)
            if not self.stopped:self.returned(cleanup=8)
        elif name=='contents':
            sid=self.number(esp+4)  # 원문이현재Pool기준으로계산한내용물부모다.
            if self.number(esp+8)!=NODE:raise RuntimeError('내용물고정stream불일치')
            self.event(name,'contents:'+str(sid));self.append(b'\xee');self.mutate('contents'+str(self.calls[name]))
            if not self.stopped:self.returned()
        elif name=='assert':
            self.assertion(self.number(esp+12))
            if not self.stopped:self.returned()
        elif name=='begin':
            self.finder=mu.reg_read(UC_X86_REG_ECX);self.index=0  # 실제stackFinder와독립후보위치다.
            arguments=[self.number(esp+offset) for offset in (4,8,12,16,20)]  # lowX/Y,highX/Y의floatbits와flags0이다.
            self.number(self.finder+0x34,self.sequence[0] if self.sequence else 0)
            self.mutate(self.event(name,'begin:'+':'.join(map(str,arguments))))
            if not self.stopped:self.returned(self.finder,20)
        elif name=='next':
            before=self.number(self.finder+0x34);self.index+=1  # 외부Next는고정후보열의다음위치로이동한다.
            self.number(self.finder+0x34,self.sequence[self.index] if self.index<len(self.sequence) else 0)
            self.mutate(self.event(name,'next:'+str(before)))
            if not self.stopped:self.returned()
        else:
            sid=(self.number(esp+8)-FORMS)//50;answer=self.returns.get(sid,1)  # SaveObject장치의명시DWORD반환입력이다.
            x=self.number(esp+12);y=self.number(esp+16)  # 실제CRT에서구한고정origin비트다.
            self.event(name,f'object:{sid}:{x}:{y}')
            if answer:self.append(self.child_payloads.get(sid,bytes([sid&255])))  # 명시payload가없으면성공객체를한BYTE로대체한다.
            self.mutate('object'+str(self.calls[name]))
            if not self.stopped:self.returned(answer)

    def run(self,kind,case,control):
        """독립 입력 하나를 온전한 진입부터 RET 또는 명시 진단 중단까지 실행한다."""
        self.kind=kind;self.case=case  # 기대없는현재입력과실행종류다.
        self.calls=Counter();self.trace=[];self.completed=False;self.stopped=False;self.instructions=0;self.crt_instructions=0  # 실제실행관측상태다.
        self.effects=[] if case['mutation']=='-' else [row.split('/') for row in case['mutation'].split(';')]  # 명시외부효과다.
        self.form_address=FORMS+case.get('sid',2)*50;self.finder=0;self.sequence=[];self.returns={};self.child_payloads={};self.index=0  # 고정객체와청크외부장치입력이다.
        self.devices={field:case.get(field,0) for field in DEVICE_FIELDS}  # 같은현재반환장치의rawDWORD다.
        self.mu.mem_write(STACK-0x2000,bytes([0xCC])*0x2020)
        for pointer in PAYLOADS:self.mu.mem_write(pointer,bytes([0xCC])*BUFFER_SIZE)  # 양물리출력의부분쓰기를구별한다.
        self.mu.mem_write(NODE,struct.pack('<IIII',PAYLOADS[0],case['position'],case['end'],case['capacity']))
        self.number(POOL_GLOBAL,FORMS);self.number(LIMIT_GLOBAL,case.get('poollimit',8));self.number(VTABLE+0x60,ROBUST);self.number(VTABLE+0x80,LEVEL)
        for field,address in GLOBALS.items():self.number(address,case.get(field,0))  # 원문전역의지정DWORD만초기화한다.
        if kind=='object':
            self.mu.mem_write(self.form_address,bytes.fromhex(case['object']));self.number(self.form_address,VTABLE)
            self.mu.mem_write(STACK,struct.pack('<IIIII',RETURN,NODE,self.form_address,case['originX']&0xFFFFFFFF,case['originY']&0xFFFFFFFF))
        else:
            self.sequence=[] if case['sequence']=='-' else [int(value) for value in case['sequence'].split(',')]
            if case['returns']!='-':
                for row in case['returns'].split(';'):  # 후보별명시반환만읽고나머지는1이다.
                    fields=row.split(':');sid=int(fields[0]);self.returns[sid]=int(fields[1])&0xFFFFFFFF
                    if len(fields)==3:self.child_payloads[sid]=b'' if fields[2]=='-' else bytes.fromhex(fields[2])
            self.mu.mem_write(STACK,struct.pack('<IIII',RETURN,NODE,case['chunkX']&0xFFFFFFFF,case['chunkY']&0xFFFFFFFF))
        registers={UC_X86_REG_EAX:0x10111213,UC_X86_REG_EBX:0x20212223,UC_X86_REG_ECX:0x30313233,UC_X86_REG_EDX:0x40414243,UC_X86_REG_ESI:0x50515253,UC_X86_REG_EDI:0x60616263,UC_X86_REG_EBP:0x70717273,UC_X86_REG_ESP:STACK,UC_X86_REG_FPCW:control,UC_X86_REG_FPSW:0}  # 명시cdecl진입레지스터다.
        for register,value in registers.items():self.mu.reg_write(register,value)  # 이전관측의잔여를제거한다.
        self.mu.emu_start((OBJECT_RANGE if kind=='object' else CHUNK_RANGE)[0],RETURN+1,count=3000000)
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK-4  # cdecl반환주소만정규화한차이다.
        if not self.completed and not self.stopped:raise RuntimeError('정상RET/명시중단없이명령한도도달')
        if self.completed:
            if delta:raise RuntimeError('정상RET의스택차이')
            for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # callee-save불변을확인한다.
                if self.mu.reg_read(register)!=registers[register]:raise RuntimeError('callee-save변경')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87제어어/스택변경')
        for first,raw in self.original.items():  # 실행가능원문전체의바이트불변을확인한다.
            if bytes(self.mu.mem_read(first,len(raw)))!=raw:raise RuntimeError('원문본체변경')
        end=self.number(NODE+8)  # 실제진행한논리끝이다.
        if end>BUFFER_SIZE:raise RuntimeError('명시물리출력범위초과')
        output={'completed':int(self.completed),'result':self.mu.reg_read(UC_X86_REG_EAX),'streamout':':'.join(map(str,(PAYLOADS.index(self.number(NODE)),self.number(NODE+4),end,self.number(NODE+12)))),'payload0':bytes(self.mu.mem_read(PAYLOADS[0],end)).hex(),'payload1':bytes(self.mu.mem_read(PAYLOADS[1],end)).hex(),'trace':';'.join(self.trace),'stackdelta':delta,'nativeinstructions':self.instructions}  # 실제CPU/장치최종출력이다.
        if kind=='object':
            raw=bytearray(self.mu.mem_read(self.form_address,50));raw[:4]=bytes(4)  # 격리가상표주소만0으로정규화한다.
            output.update(objectout=raw.hex(),globalsout=':'.join(str(self.number(GLOBALS[field])) for field in GLOBAL_FIELDS),devicesout=':'.join(str(self.devices[field]) for field in DEVICE_FIELDS))
        else:output['poollimitout']=self.number(LIMIT_GLOBAL)
        totals=self.totals.setdefault(kind+f'-{control:03x}',{'runs':0,'native_instructions':0,'crt_instructions':0})  # 종류/정밀도별합계다.
        totals['runs']+=1;totals['native_instructions']+=self.instructions;totals['crt_instructions']+=self.crt_instructions
        self.buffer_hashes.append({'name':case['name']+f'-cw{control:03x}',**{key:hashlib.sha256(bytes.fromhex(output[key])).hexdigest() for key in ('payload0','payload1')}})  # 전체버퍼별해시다.
        return output


def object_cases():
    """모드/저장폭/소유자/좌표/현재재조회/부분쓰기를기대없이교차한다."""
    base=dict(sid=2,originX=0,originY=0,object=form(),flags1=0,flags2=0,battle=0,mission=0,editor=0,scope=1,version=2,localOwner=1,bridgeType=82,daisType=165,battleMask=0x40480002,ownerMask=0x5D77CF00,robust=1,island=1,hasLevel=1,level=0x123,position=0,end=0,capacity=4096,mutation='-')  # 정상소규모명시입력이다.
    result=[]  # 독립객체입력목록이다.
    for index,values in enumerate(itertools.product((0,1),(0,1),(0,1),(0,1,2,3),(0,1),(0,1),(0,2))):  # 모든초기관문과Terr/Chaff를교차한다.
        battle,mission,editor,scope,robust,island,flags2=values  # 결과에서유도하지않은원시조건이다.
        result.append(dict(base,name=f'mode-{index:04d}',battle=battle,mission=mission,editor=editor,scope=scope,robust=robust,island=island,flags2=flags2))
    flag_values=(0,0x20,0x1000,0x2000,0x20000,0x3000,0x21020,0x23020)  # 프레임/QA/QB/내용물의독립저장조합이다.
    for index,values in enumerate(itertools.product(flag_values,(0,4,0x4000,0x4004,2,0x10000),(-1,0,1,2,3),(0,1,8,9,255))):  # 현재version과모든좁은저장필드의상호작용을확인한다.
        flags1,flags2,version,owner=values  # signedversion/소유자진단도포함한다.
        result.append(dict(base,name=f'fields-{index:04d}',flags1=flags1,flags2=flags2,version=version,object=form(owner=owner)))
    for index,values in enumerate(itertools.product((-16,-1.9,-1,-0.9,0,0.9,15,15.9,16,16.9,255.9),(-1.9,0,15.9,16),(0,16))):  # 실제float32절삭과nibble진단/상한거부를교차한다.
        x,y,origin=values  # 범위밖하위좌표도원문진단복귀로계속한다.
        result.append(dict(base,name=f'coordinates-{index:03d}',originX=origin,object=form(x=x,y=y)))
    for index,values in enumerate(itertools.product((82,165),(0,1,8,9,255),(1,257,0xFFFFFFFF))):  # typeBYTE와globalDWORD전체비교및다리약식을확인한다.
        type,owner,local=values  # 실제ownerBYTE와축약하지않은로컬DWORD다.
        result.append(dict(base,name=f'special-type-{index:03d}',object=form(type=type,owner=owner),localOwner=local,flags2=0x4004))
    for index,flags in enumerate((1,8,9,2,4,16,255)):  # 저장금지raw+28의bit0/3만선택된다.
        result.append(dict(base,name=f'object-flags-{index}',object=form(flags=flags)))
    changes=('robust1/global/scope/2','robust1/global/scope/0','robust1/form/x/'+str(bits(15.5)),'island1/global/scope/2',
             'flags21/global/battleMask/0','flags11/device/flags1/143392','flags12/device/flags1/12288','flags13/device/flags1/8192',
             'write1/form/type/82','write1/form/owner/8','write1/global/bridgeType/257','write2/form/frame/255','write3/form/qa/4660',
             'write2/global/version/0','hasLevel1/global/version/2','level1/global/version/2','level1/device/level/7',
             'write2/global/ownerMask/0','flags23/form/owner/9','flags21/device/flags2/16388','write1/stream/data/1',
             'write1/stream/position/7','contents1/form/qa/255','assert1/form/owner/8','assert1/stream/capacity/64',
             'assert1/stream/data/1','assert1/control/stop/1','write2/control/stop/1')  # callback뒤현재type/전역/좁은필드를다시읽는위치를관측한다.
    for index,values in enumerate(itertools.product(changes,(0,2))):  # legacy/count전환을양version에서공급한다.
        mutation,version=values  # 명시효과와초기version의독립조합이다.
        result.append(dict(base,name=f'callback-{index:03d}',flags1=0x23020,flags2=0x4004,version=version,battle=1,mission=1,capacity=0 if mutation.startswith('assert') else 4096,mutation=mutation))
    result.append(dict(base,name='nibble-x-stop',object=form(x=-1),mutation='assert1/control/stop/1'))
    result.append(dict(base,name='nibble-y-stop',object=form(y=-1),mutation='assert1/control/stop/1'))
    result.append(dict(base,name='owner-diagnostic-requery',object=form(owner=9),flags2=0x4000,mutation='assert1/form/owner/8'))
    result.append(dict(base,name='owner-global-width',object=form(type=165,owner=2),daisType=421))
    result.append(dict(base,name='bridge-global-width',object=form(type=82),bridgeType=338))
    result.append(dict(base,name='scope-requery-accepts-chaff',island=0,mutation='robust1/global/scope/2'))
    result.append(dict(base,name='battle-mask-requery-after-flags',battle=1,flags2=4,mutation='flags21/global/battleMask/4'))
    result.append(dict(base,name='legacy-has-level-zero',version=0,hasLevel=0,flags2=0x4000))
    result.append(dict(base,name='nonboolean-device-dwords',version=0,robust=2,island=2,hasLevel=2,level=0xFFFFFFFF))
    result.append(dict(base,name='negative-version-minimum',version=-2147483648,flags2=0x4000))
    return result


def chunk_cases():
    """청크marker/고정countWORD/후보경계와현재stream변경을교차한다."""
    base=dict(chunkX=0,chunkY=0,poollimit=8,sequence='2,3',returns='2:1;3:0',position=0,end=0,capacity=4096,mutation='-')  # child저장은명시한BYTE장치다.
    result=[]  # 독립청크입력목록이다.
    for index,values in enumerate(itertools.product((-1,0,1,15,16),(0,1,15),('-', '0','8','65535','2','2,3','2,3,2'),((0,0),(3,19),(7,2)))):  # origin변환/빈목록/경계/중복/cursor를교차한다.
        x,y,sequence,cursor=values  # 실제본체에공급할기대없는입력이다.
        result.append(dict(base,name=f'chunk-{index:04d}',chunkX=x,chunkY=y,sequence=sequence,position=cursor[0],end=cursor[1]))
    changes=('begin1/finder/current/3','begin1/global/poollimit/2','begin1/stream/data/1','begin1/stream/position/5',
             'object1/global/poollimit/3','object1/stream/data/1','object1/stream/position/7','object1/stream/end/19',
             'object1/finder/current/0','object1/control/stop/1','next1/finder/current/2','next1/global/poollimit/2',
             'assert1/stream/data/1','assert1/stream/position/5','assert1/stream/capacity/64','assert1/control/stop/1')  # marker전/예약뒤callback과중단의다른부분출력을관측한다.
    for index,mutation in enumerate(changes):  # 장치효과를입력순서대로독립실행한다.
        result.append(dict(base,name=f'chunk-callback-{index:02d}',capacity=0 if mutation.startswith('assert') else 4096,mutation=mutation))
    result.append(dict(base,name='chunk-count-no-capacity-check',sequence='-',capacity=1))
    result.append(dict(base,name='chunk-count-word-wrap-device',returns='2:65535;3:2'))
    result.append(dict(base,name='chunk-count-dword-low-zero',returns='2:65536;3:0'))
    result.append(dict(base,name='chunk-count-dword-minus-one',returns='2:4294967295;3:0'))
    result.append(dict(base,name='chunk-count-word-wrap-65536-bools',sequence=','.join(['2']*65536),returns='2:1:-'))
    return result


def observe(oracle,kind,inputs,schema,path):
    """각입력을두실제제어어로실행하고온전한새TSV를기록한다."""
    rows=[]  # 독립입력과실제관측을보존할행이다.
    for case in inputs:  # 모든입력을두제어어에서별도실행한다.
        previous=None  # 정밀도간결과차이검사용이다.
        for control in CONTROLS:  # 실제실행을생략하거나기대값을합성하지않는다.
            try:output=oracle.run(kind,case,control)
            except Exception as error:raise RuntimeError(case['name']) from error
            if previous is not None and previous!=output:raise RuntimeError('정밀도차이 '+case['name'])
            previous=output;values=dict(case,name=case['name']+f'-cw{control:03x}',**output)
            rows.append('\t'.join(str(values[key]) for key in schema))
    path.write_text('# 실제10.78 PE/CRT 관측. 지정하위장치만명시경계다.\n#'+'\t'.join(schema)+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    return {'inputs':len(inputs),'precision_runs':len(rows),'precision_differences':0,'schema':list(schema),'sha256':digest(path)}


def main():
    """허용한두새TSV와보고서만작성하고보호입력불변을확인한다."""
    if socket.gethostname().lower()!='vm-w11-codex' or digest(PE_PATH)!=PE_SHA:raise RuntimeError('현재PC/보호PE식별자불일치')
    oracle=Oracle()  # 두실제본체와CRT의격리실행기다.
    outputs={'object':observe(oracle,'object',object_cases(),OBJECT_SCHEMA,OBJECT_PATH)}  # 단일객체관측을먼저완료한다.
    print('object',outputs['object']['inputs'],outputs['object']['precision_runs'],flush=True)
    outputs['chunk']=observe(oracle,'chunk',chunk_cases(),CHUNK_SCHEMA,CHUNK_PATH)
    if digest(PE_PATH)!=PE_SHA:raise RuntimeError('보호입력디스크변경')
    report={'host':socket.gethostname(),'date':'2026-10-23','outputs':outputs,'precision_observations':oracle.totals,
            'native_ranges':[{'first':f'{first:08x}','end_exclusive':f'{last:08x}','sha256':hashlib.sha256(oracle.original[first]).hexdigest()} for first,last in (OBJECT_RANGE,CHUNK_RANGE,FTOL_RANGE,WRITE_RANGE)],
            'boundaries':{f'{address:08x}':name for address,name in BOUNDARIES.items()},'boundary_calls':dict(oracle.boundaries),'native_write_counts':dict(oracle.writes),'buffer_sha256':oracle.buffer_hashes,
            'object_input':'raw50BYTE의vptr만격리VT로설치하고출력에서0으로정규화한다. offset+a=typeBYTE,+c=QAWORD,+e/+12=좌표float32,+22=ownerBYTE,+24=frameDWORD,+28=flagsBYTE다. global10종과device6종은스키마순서다.',
            'object_devices':'flags1/2는trace후mutation후현재반환DWORD를읽는다. robust/island/hasLevel/level은반환DWORD를mutation전에캡처한다. write는caller의실제bytes를읽어trace→명시capacity진단36→현재data/pos로쓰기→mutation순서다. contents는순서관측용ee한BYTE를같은stream에쓴후mutation한다.',
            'chunk_devices':'Begin은lowX/lowY/highX/highY의실제float32DWORD비트와flags를십진수trace로기록한다. sequence를Finder+34의현재DWORD로공급하고Next는고정목록index를진행한다. returns는sid:DWORD[:payloadHex]이며없는sid는1,세번째필드생략은sid하위BYTE,세번째필드-는빈payload다. 비영반환에서만그payload를쓴다. returns65535/65536/ffffffff는실제객체함수0/1반환범위를넘긴명시경계검사다. 별도65536후보는실제반환1/빈장치payload에서원문countWORD감김을직접관측한다. chunk의result는원본void함수의미정EAX관측이며제품반환계약이아니다.',
            'mutation':'trigger/target/field/decimalValue를세미콜론으로나눈다. trigger는flags11/flags21/robust1/island1/hasLevel1/level1/write1/contents1/assert1/begin1/next1/object1등종류별1기반호출번호다. target은global(10전역또는poollimit),device(6반환장치),form(type/owner/flags/qa/frame/x/y),stream(data/position/end/capacity),finder/current,control/stop이다. form좌표값은float32DWORD비트다.',
            'observed_rules':'실제CRT가float32→signed64절삭의EAX하위DWORD를반환한다. 본체가모드/scope/version signed조건/typeBYTE 대globalDWORD비교/nibble/다리/현재flags재조회/순서/폭을수행한다. 청크는실제WriteMemStream으로99를쓴뒤countWORD를초기화없이예약하고최종탈출때만고정주소에누적하위WORD를쓴다.',
            'limits':['전체Objects/Terr/Chaff/Temp부모/실제저장파일/GUI/Editor/게임프로세스실행은범위밖이다.','객체의하위Write/내용물은명시장치다. 청크는단일객체하위와Finder만명시장치이며markerWrite와CRT는실제명령이다.','물리출력각16KB와최대3000000명령으로제한한다. 중단stackdelta는실제CALL도달관측이며원본SEHunwind검증이아니다.'],
            'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE_PATH,Path(__file__),OBJECT_PATH,CHUNK_PATH)},'new_game_runs':0,'new_decompilations':0}
    REPORT_PATH.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print({kind:{'inputs':value['inputs'],'runs':value['precision_runs']} for kind,value in outputs.items()})


if __name__=='__main__':  # 직접실행할때만허용한새산출물을생성한다.
    main()
