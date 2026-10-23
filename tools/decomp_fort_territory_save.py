#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Territory 부모와 지역 조회·seed 하위의 실제 PE 명령을 격리 관측한다."""
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
PE_PATH=ROOT/'originals/Netstorm.exe'  # 수정하지 않는 보호 PE다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 확정 원문 입력 식별자다.
REPORT_PATH=ROOT/'extracted/cpp-fort-territory-save-20261023-native.json'  # 실행 범위와 해시 보고서다.
SAVE_PATH=ROOT/'cpppj/tests/fixtures/fort-territory-save-x86.tsv'  # 부모 독립 관측이다.
QUERY_PATH=ROOT/'cpppj/tests/fixtures/fort-territory-query-x86.tsv'  # 두 실제 지역 getter 관측이다.
SEED_PATH=ROOT/'cpppj/tests/fixtures/fort-territory-seed-x86.tsv'  # 실제 seed 부분 쓰기 관측이다.
BASE=0x400000  # 보호 이미지의 원래 기준 주소다.
HEAP=0x1000000  # 명시 외부 장치의 격리 주소다.
MANAGERS={11:HEAP+0x1000,22:HEAP+0x1200}  # 서로 다른 manager와 current를 구별한다.
NODES=(HEAP+0x2000,HEAP+0x2040)  # 두 실제16BYTE stream 헤더다.
BUFFERS=(HEAP+0x3000,HEAP+0x8000)  # 두16KB 출력 버퍼다.
BUFFER_SIZE=16384  # 논리 alloc과 다른 물리 출력 상한이다.
SCRATCH=HEAP+0xD000  # 명시 ChunkMap 반환6BYTE다.
ISLANDS=HEAP+0x10000  # getter가 직접 읽는 count+32BYTE 배열이다.
SEED_RECORD=HEAP+0x18000  # 실제 seed가 부분 갱신할6BYTE다.
HEADERS=0x5C89F0  # 원본20BYTE Template 슬롯들의 기준 주소다.
SPECIAL=0x5C85A4  # 부모가 Territory 읽기 뒤 한 번 읽는 editor 전역이다.
FORT=0x594FB8  # 실제 일반 getter가 진입 때 읽는 fort 전역이다.
MINE=0x540CAC  # 부모의 고정 this 슬롯과 구별할 현재 owner 전역이다.
NO_REGION=0x50C038  # 부모가 매 지역 getter 뒤 다시 비교하는 원본 sentinel이다.
STACK=0x2100000  # 실제 함수 진입 ESP다.
RETURN=HEAP+0x1F000  # 최상위 RET의 격리 종료점이다.
CONTROLS=(0x27F,0x37F)  # 두 x87 제어어를 각각 실행한다.
RANGES={'save':(0x4BF6D0,0x4BF871),'normal':(0x46F380,0x46F3E2),'any':(0x46F3F0,0x46F429),'seed':(0x4B37C0,0x4B37F5)}  # 실제 RET까지 포함한 본체 범위다.
BOUNDARIES={0x4BD660:'get',0x46F380:'region',0x46F3F0:'any',0x432430:'record',0x4B37C0:'seed',0x44D1B0:'create',0x4E0620:'assert',0x4558C0:'random'}  # 현재 실행 본체 밖에서만 대체한다.
SAVE_SCHEMA=('name','slot','manager','current11','current22','special','fort','mine','noRegion','position','end','capacity','regions','records','mutation','completed','managerout','currentsout','globalsout','streamsout','payload0','payload1','recordsout','trace','stackdelta','nativeinstructions')  # 앞15열입력/뒤11열관측이다.
QUERY_SCHEMA=('name','kind','fort','owner','region','count','islands','result','trace','stackdelta','nativeinstructions')  # 앞7열입력/뒤4열관측이다.
SEED_SCHEMA=('name','index','record','random','mutation','completed','recordout','trace','stackdelta','nativeinstructions')  # 앞5열입력/뒤5열관측이다.


def digest(path):
    """보호 입력과 새 산출물의 SHA256을 읽기 전용으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """허용한 실제 본체만 실행하고 명시 하위 장치를 공급한다."""
    def __init__(self):
        """보호 PE와 격리된 장치/스택을 준비한다."""
        self.pe=pefile.PE(str(PE_PATH))  # 읽기 전용 원문 파서다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 실제32비트 명령 실행기다.
        self.size=(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095  # 정렬된 원문 이미지 크기다.
        self.mu.mem_map(BASE,self.size);self.mu.mem_write(BASE,self.pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP,0x30000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.original={kind:bytes(self.mu.mem_read(first,last-first)) for kind,(first,last) in RANGES.items()}  # 각 본체 불변 확인용이다.
        self.totals={}  # 종류/정밀도별 실제 명령과 호출 합계다.
        self.boundaries=Counter()  # 실제 도달한 명시 경계 합계다.
        self.writes=Counter()  # 실제 CPU 쓰기의 범위별 합계다.
        self.buffer_hashes=[]  # 부모의 전체 두 출력/120BYTE 원문을 실행별로 식별한다.

    def number(self,address,value=None,width=4):
        """명시 raw DWORD/BYTE를 읽거나 외부 효과로만 쓴다."""
        if value is not None:self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def returned(self,value=0,cleanup=0):
        """원문 CALL이 저장한 주소와 thiscall 인자만 소비한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 실제 경계 진입 ESP다.
        target=self.number(esp)  # CPU가 쓴 실제 복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX,value&0xFFFFFFFF);self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EIP,target)

    def mutate(self,event):
        """입력에 지정한 현재 경계의 효과만 작성 순서대로 적용한다."""
        for effect in self.effects:  # 여러 변경을 같은 경계에서 순서대로 수행한다.
            trigger,target,field,text=effect  # 시점/대상/필드/십진값이다.
            if trigger!=event:continue
            value=int(text)  # 지정한 raw 변경 값이다.
            if target=='global':self.number({'special':SPECIAL,'fort':FORT,'mine':MINE,'noRegion':NO_REGION}[field],value)
            elif target=='header' and field=='manager':self.number(self.this,MANAGERS[value] if value else 0)
            elif target.startswith('manager') and field=='current':self.number(MANAGERS[int(target[7:])]+8,NODES[value])
            elif target.startswith('stream'):
                node=NODES[int(target[6:])]  # 바꿀 명시 stream이다.
                self.number(node+{'data':0,'position':4,'end':8,'capacity':12}[field],BUFFERS[value] if field=='data' else value)
            elif target in ('source','record') and field.startswith('byte'):
                address=self.source if target=='source' else SEED_RECORD  # 부모 지역배열 또는 seed 원문이다.
                self.number(address+int(field[4:]),value,1)
            elif target=='control' and field=='stop' and value==1:self.stopped=True;self.mu.emu_stop()
            else:raise RuntimeError('미명시 callback 효과')

    def write(self,mu,access,address,size,value,data):
        """원문 CPU 쓰기가 스택/stream/출력/seed 안에만 있는지 확인한다."""
        if STACK-0x1000<=address and address+size<=STACK+12:kind='stack'
        elif any(node<=address and address+size<=node+16 for node in NODES):kind='stream'
        elif any(first<=address and address+size<=first+BUFFER_SIZE for first in BUFFERS):kind='payload'
        elif SEED_RECORD<=address and address+size<=SEED_RECORD+6:kind='seed'
        else:raise RuntimeError(f'미명시 원문 쓰기 {address:08x}/{size}')
        self.writes[kind]+=1

    def instruction(self,mu,address,size,data):
        """선택한 본체의 모든 명령과 명시 외부 CALL 순서를 관측한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        first,last=RANGES[self.kind]  # 이번 실행에서 허용한 단일 원문 본체다.
        if first<=address<last:self.instructions+=1;return
        if address not in BOUNDARIES:raise RuntimeError(f'미명시 코드 {address:08x}')
        name=BOUNDARIES[address]  # 실제 진입한 하위 장치다.
        self.boundaries[name]+=1;self.calls[name]+=1
        event=name+str(self.calls[name])  # 입력에서 지정하는 종류별1기반 경계번호다.
        esp=mu.reg_read(UC_X86_REG_ESP)  # 실제 경계의 인자 스택이다.
        if name=='get':
            self.source=self.number(esp+4);self.trace.append('get:'+str(self.case['slot']))
            mu.mem_write(self.source,bytes.fromhex(self.case['records']));self.mutate(event);self.returned(cleanup=4)
        elif name in ('region','any'):
            index=self.number(esp+(8 if name=='region' else 4))  # 부모가 전달한 실제 지역번호다.
            owner=self.number(esp+4) if name=='region' else None  # 일반 분기에서만 전달하는 고정 clamp owner다.
            answer=self.regions.get(index,(self.number(NO_REGION),self.number(NO_REGION)))[0 if name=='region' else 1]  # 명시 getter의 반환입력이다.
            self.trace.append(f'region:{self.number(FORT)}:{owner}:{index}' if name=='region' else f'any:{index}')
            self.mutate(event);self.returned(answer,8 if name=='region' else 4)
        elif name=='record':
            destination=self.number(esp+4);region=self.number(esp+8)  # 원문이 지정한 명시 반환버퍼와 지역번호다.
            self.trace.append('record:'+str(region));mu.mem_write(destination,bytes((region*17+byte*29+7)&255 for byte in range(6)))  # 명시 지도 장치의6BYTE 식이다.
            self.mutate(event);self.returned(destination,8)
        elif name=='seed':
            record=self.number(esp+4);index=self.number(esp+8)&255  # 원문 부모가 전달한 빈 지역순번 BYTE다.
            self.trace.append('seed:'+str(index));raw=bytearray(mu.mem_read(record,6))  # 이 외부 장치만 부모 실행에서는 대체한다.
            raw[0]=(raw[0]&0xC0)|(index&0x3F);raw[1]=(raw[1]&0xF7)|3;raw[3]=(index*37+11)%254+1
            mu.mem_write(record,bytes(raw));self.mutate(event);self.returned()
        elif name=='create':
            manager=mu.reg_read(UC_X86_REG_ECX);token=next((key for key,value in MANAGERS.items() if value==manager),0)  # 호출시 캡처된 실제 manager다.
            self.trace.append('create:'+str(token)+':Territory');self.mutate(event);self.returned(NODES[0],4)  # 반환0번stream과 현재stream을 의도적으로 구별한다.
        elif name=='random':
            limit=self.number(esp+4);self.trace.append('random:'+str(limit))  # seed 하위는 실제 Next 인자254를 관측한다.
            if limit!=254:raise RuntimeError('seed의 난수 인자 불일치')
            self.mutate(event)
            if not self.stopped:self.returned(self.case['random'])
        else:
            line=self.number(esp+12);self.trace.append('assert:'+str(line));self.mutate(event)  # 원문 진단36/521의 도달 순서다.
            if not self.stopped:self.returned()

    def run(self,kind,case,control):
        """한 독립 입력을 실제 함수 진입부터 RET 또는 명시 중단까지 실행한다."""
        self.kind=kind;self.case=case  # 이번 실행 종류와 기대 없는 입력이다.
        self.calls=Counter();self.trace=[]  # 이번 입력의 경계번호와 순서다.
        self.effects=[] if case.get('mutation','-')=='-' else [part.split('/') for part in case['mutation'].split(';')]  # 명시 외부 변경들이다.
        self.completed=False;self.stopped=False;self.instructions=0  # 실제 RET/중단/본체명령 관측이다.
        self.source=STACK-120;self.this=HEADERS+case.get('slot',1)*20  # 부모의 고정 지역배열과 Template this다.
        self.regions={}  # 부모가 사용하는 명시 지역 반환표다.
        self.mu.mem_write(STACK-0x1000,bytes([0xCC])*0x1010)
        self.number(FORT,case.get('fort',0));self.number(SPECIAL,case.get('special',0));self.number(MINE,case.get('mine',1));self.number(NO_REGION,case.get('noRegion',127))
        cleanup=0  # 실제 정상 RET가 스택에서 제거할 인자 크기다.
        if kind=='save':
            self.number(self.this,MANAGERS[case['manager']] if case['manager'] else 0)
            for token,field in ((11,'current11'),(22,'current22')):self.number(MANAGERS[token]+8,NODES[case[field]])  # 외부Create의 명시 현재stream 후조건이다.
            for index,node in enumerate(NODES):  # 두stream과 두물리버퍼의 초기상태를 분리한다.
                self.mu.mem_write(node,struct.pack('<IIII',BUFFERS[index],case['position'],case['end'],case['capacity']));self.mu.mem_write(BUFFERS[index],bytes([0xCC])*BUFFER_SIZE)
            if case['regions']!='-':
                for row in case['regions'].split(';'):  # 지정한 지역 반환만 공급하고 나머지는 현재sentinel을 반환한다.
                    index,normal,any_region=map(int,row.split(':'));self.regions[index]=(normal,any_region)
            self.number(STACK,RETURN)
        elif kind in ('normal','any'):
            self.number(ISLANDS,case['count']);self.mu.mem_write(ISLANDS+4,bytes(32*8))
            if case['islands']!='-':
                for index,row in enumerate(case['islands'].split(';')):  # 존재/current/original/region을 실제32BYTE 레코드에 놓는다.
                    exists,current,original,region=map(int,row.split(':'));record=ISLANDS+4+index*32
                    self.number(record+4,exists);self.number(record+8,current);self.number(record+13,original,1);self.number(record+12,region,1)
            self.mu.mem_write(STACK,struct.pack('<III',RETURN,case['owner']&0xFFFFFFFF,case['region']&0xFFFFFFFF) if kind=='normal' else struct.pack('<II',RETURN,case['region']&0xFFFFFFFF));cleanup=8 if kind=='normal' else 4
        else:
            self.mu.mem_write(SEED_RECORD,bytes.fromhex(case['record']));self.mu.mem_write(STACK,struct.pack('<III',RETURN,SEED_RECORD,case['index']&0xFFFFFFFF))
        registers={UC_X86_REG_EAX:0x10111213,UC_X86_REG_EBX:0x20212223,UC_X86_REG_ECX:ISLANDS if kind in ('normal','any') else self.this,UC_X86_REG_EDX:0x30313233,UC_X86_REG_ESI:0x40414243,UC_X86_REG_EDI:0x50515253,UC_X86_REG_EBP:0x60616263,UC_X86_REG_ESP:STACK,UC_X86_REG_FPCW:control,UC_X86_REG_FPSW:0}  # 명시 함수 진입 레지스터다.
        for register,value in registers.items():self.mu.reg_write(register,value)  # 이전 실행의 잔여를 제거한다.
        self.mu.emu_start(RANGES[kind][0],RETURN+1,count=100000)
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK-4-cleanup  # thiscall 정리까지 반영한 정상 스택 차이다.
        if not self.completed and not self.stopped:raise RuntimeError('RET/명시중단 없이 한도도달')
        if self.completed and delta:raise RuntimeError('실제 RET의 스택 정리 불일치')
        if self.completed:
            for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 원문 callee-save 레지스터를 확인한다.
                if self.mu.reg_read(register)!=registers[register]:raise RuntimeError('callee-save 레지스터 변경')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 상태 변경')
        first,last=RANGES[kind]  # 실행한 본체를 다시 읽을 실제 범위다.
        if bytes(self.mu.mem_read(first,last-first))!=self.original[kind]:raise RuntimeError('원문 본체 바이트 변경')
        totals=self.totals.setdefault(kind+f'-{control:03x}',{'runs':0,'native_instructions':0})  # 종류/정밀도별 합계다.
        totals['runs']+=1;totals['native_instructions']+=self.instructions
        common={'trace':';'.join(self.trace),'stackdelta':delta,'nativeinstructions':self.instructions}  # 실제 외부효과/스택/명령 관측이다.
        if kind in ('normal','any'):return dict(common,result=self.mu.reg_read(UC_X86_REG_EAX))
        if kind=='seed':return dict(common,completed=int(self.completed),recordout=bytes(self.mu.mem_read(SEED_RECORD,6)).hex())
        ends=[self.number(node+8) for node in NODES]  # 양stream의 마지막 논리끝이다.
        if max(ends)>BUFFER_SIZE:raise RuntimeError('명시 물리 출력 범위 초과')
        output=dict(common,completed=int(self.completed),managerout=next((key for key,value in MANAGERS.items() if self.number(self.this)==value),0),
                    currentsout=':'.join(str(NODES.index(self.number(MANAGERS[token]+8))) for token in (11,22)),
                    globalsout=':'.join(str(self.number(address)) for address in (SPECIAL,FORT,MINE,NO_REGION)),
                    streamsout=';'.join(':'.join(map(str,(BUFFERS.index(self.number(node)),self.number(node+4),self.number(node+8),self.number(node+12)))) for node in NODES),
                    payload0=bytes(self.mu.mem_read(BUFFERS[0],max(ends))).hex(),payload1=bytes(self.mu.mem_read(BUFFERS[1],max(ends))).hex(),
                    recordsout=bytes(self.mu.mem_read(self.source,120)).hex())
        self.buffer_hashes.append({'name':case['name']+f'-cw{control:03x}',**{key:hashlib.sha256(bytes.fromhex(output[key])).hexdigest() for key in ('payload0','payload1','recordsout')}})  # 실제전체버퍼별해시다.
        return output


def save_cases():
    """부모 분기/고정 슬롯/현재manager/current/진단 부분 쓰기를 교차한다."""
    raw=bytearray((index*23+9)&255 for index in range(120))  # 이전120BYTE의 서로 다른 모든 필드다.
    for region in range(20):raw[region*6+1]=(raw[region*6+1]&0xFE)|int(region%4 in (1,2))  # 일반분기의지역1/2는켜고4는끄며갱신/보존을교차한다.
    original=raw.hex()  # 보존필드와명시bit0의독립입력이다.
    regions=';'.join(f'{index}:{index+30 if index%3 else 127}:{index+60 if index%3 else 127}' for index in range(20))  # 두 getter의 명시 반환값이다.
    base=dict(slot=1,manager=11,current11=0,current22=1,special=0,fort=0,mine=1,noRegion=127,position=0,end=0,capacity=4096,regions=regions,records=original,mutation='-')  # 정상 입력 계약이다.
    result=[]  # 독립 부모 입력 목록이다.
    for index,values in enumerate(itertools.product((0,1,2,3,4,5,6,7,8,9,15),(0,1),(0,1),(0,1),((0,0),(3,139),(9,2)))):  # ownerclamp/editor/fort/current/cursor를교차한다.
        slot,special,fort,current,cursor=values  # 제품결과와 독립된 입력 조합이다.
        result.append(dict(base,name=f'parent-{index:04d}',slot=slot,mine=slot,special=special,fort=fort,current11=current,position=cursor[0],end=cursor[1]))
    changes=('get1/global/special/1','get1/global/special/0','get1/global/mine/8','get1/header/manager/22',
             'region1/global/special/1','any1/global/special/0','region1/global/mine/8','region1/global/noRegion/30',
             'region1/source/byte7/0','region1/source/byte7/1','record1/source/byte13/0','seed1/source/byte0/255',
             'create1/header/manager/22','create1/manager11/current/1','create1/manager22/current/0',
             'create1/stream0/data/1','create1/stream0/position/7','create1/stream0/capacity/0',
             'assert1/stream0/capacity/4096','assert1/stream0/data/1','assert1/stream0/position/11',
             'assert1/header/manager/22','assert1/manager11/current/1','assert1/control/stop/1')  # 현재값 재조회와 이미고정한stream을구별한다.
    for index,values in enumerate(itertools.product(changes,(0,1))):  # 같은효과를양부모분기에서별도공급한다.
        mutation,special=values  # 경계와분기의독립조합이다.
        result.append(dict(base,name=f'callback-{index:03d}',special=special,capacity=100 if mutation.startswith('assert') else 4096,mutation=mutation))
    result.append(dict(base,name='missing-manager-repair',manager=0,mutation='assert1/header/manager/11'))
    result.append(dict(base,name='missing-manager-stop',manager=0,mutation='assert1/control/stop/1'))
    result.append(dict(base,name='all-missing-normal',regions='-'))
    result.append(dict(base,name='all-missing-editor',regions='-',special=1))
    result.append(dict(base,name='all-present-editor',special=1,regions=';'.join(f'{index}:{index}:{index}' for index in range(20))))
    for enabled in (0,1):  # 전부꺼짐/켜짐에서도일반분기의지역존재조건과갱신횟수를구별한다.
        flags=bytearray(raw)  # 다른원문필드는동일하게보존한다.
        for region in range(20):flags[region*6+1]=(flags[region*6+1]&0xFE)|enabled  # bit0만명시입력으로바꾼다.
        result.append(dict(base,name=f'normal-all-flags-{enabled}',records=flags.hex()))
    return result


def query_cases():
    """실제 두 getter의 signed owner/region과 첫일치/존재/count를 교차한다."""
    result=[]  # 독립 조회 입력 목록이다.
    for index,values in enumerate(itertools.product(('normal','any'),(0,1),(0,1,2,127,-1),(0,1,127,128,255,-128,-1),(0,1),(0,1,2),(0,1,2,255),(0,1,2))):  # 모든원시조건과0/1/2개검색범위를교차한다.
        kind,fort,owner,region,exists,current,original,count=values  # 기대가없는독립입력이다.
        result.append(dict(name=f'query-{index:05d}',kind=kind,fort=fort,owner=owner,region=region,count=count,islands=f'{exists}:{current}:{original}:{region};1:{owner&0xFFFFFFFF}:{owner&255}:{region}'))
    result.append(dict(name='negative-count',kind='normal',fort=0,owner=1,region=2,count=0xFFFFFFFF,islands='1:1:1:2'))
    return result


def seed_cases():
    """실제 seed의 index BYTE와 보존필드 및 난수경계 전후를 교차한다."""
    result=[]  # 독립 seed 입력 목록이다.
    for index,values in enumerate(itertools.product((0,1,19,63,64,127,128,255,256,511),(0,1,253),(bytes(6),bytes([255])*6,bytes.fromhex('c8a56917b3de')))):  # 인덱스폭과현재원문보존비트를교차한다.
        selected,random,record=values  # 명시 함수인자/난수반환/raw6BYTE다.
        result.append(dict(name=f'seed-{index:03d}',index=selected,record=record.hex(),random=random,mutation='-'))
    for index,effect in enumerate(('random1/control/stop/1','random1/record/byte0/85','random1/record/byte1/170','random1/record/byte3/99')):  # 난수CALL전에완료된필드와반환뒤byte3을관측한다.
        result.append(dict(name=f'seed-callback-{index}',index=7,record='c8a56917b3de',random=42,mutation=effect))
    return result


def observe(oracle,cases,schema,path,kind):
    """입력별 두 실제 정밀도 실행 결과를 허용한 새 TSV에 기록한다."""
    rows=[]  # 기대값합성없이실행한행들이다.
    for case in cases:  # 각독립입력을두정밀도로온전히실행한다.
        previous=None  # 첫실제관측과둘째관측의비교용이다.
        for control in CONTROLS:  # 정밀도차이를숨기지않고각행으로보존한다.
            try:output=oracle.run(case['kind'] if kind=='query' else kind,case,control)
            except Exception as error:raise RuntimeError(case['name']) from error
            if previous is not None and previous!=output:raise RuntimeError('정밀도별차이 '+case['name'])
            previous=output
            values=dict(case,name=f'{case["name"]}-cw{control:03x}',**output)  # 명시입력과실제출력을합친행이다.
            rows.append('\t'.join(str(values[column]) for column in schema))
    path.write_text('# 실제10.78 PE 관측. 명시경계/범위는같은도구보고서참고.\n#'+'\t'.join(schema)+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    return {'inputs':len(cases),'precision_runs':len(rows),'precision_differences':0,'schema':list(schema),'sha256':digest(path)}


def main():
    """새 세 TSV와 보고서만 생성하고 보호 PE 및 본체 해시를 재확인한다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('확정한 디컴파일PC와 다름')
    before=digest(PE_PATH)  # 보호 원문 식별자다.
    if before!=PE_SHA:raise RuntimeError('보호 PE SHA256 불일치')
    oracle=Oracle()  # 실제 명령 실행을 담당할격리장치다.
    outputs={'save':observe(oracle,save_cases(),SAVE_SCHEMA,SAVE_PATH,'save')}  # 부모를먼저완료한다.
    print('parent',outputs['save']['inputs'],outputs['save']['precision_runs'],flush=True)
    outputs['query']=observe(oracle,query_cases(),QUERY_SCHEMA,QUERY_PATH,'query')
    outputs['seed']=observe(oracle,seed_cases(),SEED_SCHEMA,SEED_PATH,'seed')
    if digest(PE_PATH)!=before:raise RuntimeError('보호 PE 디스크변경')
    report={'host':socket.gethostname(),'date':'2026-10-23','outputs':outputs,'precision_observations':oracle.totals,
            'ranges':{kind:{'first':f'{first:08x}','end_exclusive':f'{last:08x}','sha256':hashlib.sha256(oracle.original[kind]).hexdigest()} for kind,(first,last) in RANGES.items()},
            'boundaries':{f'{address:08x}':name for address,name in BOUNDARIES.items()},'boundary_calls':dict(oracle.boundaries),'native_write_counts':dict(oracle.writes),'parent_buffer_sha256':oracle.buffer_hashes,
            'parent_devices':'Get는records120을실제stack출력에복사한다. region/any는regions표를반환하고없는index는그시점의noRegion을반환한다. record(island)는byte[j]=(island*17+j*29+7)&255의6BYTE를지정출력에복사해그주소를반환한다. seed하위는index별Next254반환=(index*37+11)%254로명시대체한다. Create는입력current/stream후조건을보존하고항상stream0을반환한다. 원문부모가반환값을무시하고현재manager/current를다시읽는지관측한다.',
            'mutation':'trigger/target/field/decimalValue를세미콜론으로나눈다. trigger는get1/region1/any1/record1/seed1/create1/assert1/random1이다. trace기록→명시장치행동→효과→반환순서다. region반환만효과전에캡처한다. source/byteN은부모120BYTE배열을수정하고record/byteN은실제seed6BYTE를수정한다. global={special,fort,mine,noRegion},header/manager,manager11|22/current,stream0|1/{data,position,end,capacity},control/stop을지원한다.',
            'parent_output':'currentsout=current11:current22,globalsout=special:fort:mine:noRegion,streamsout=data:pos:end:capacity;data:pos:end:capacity다. payload0/1은각물리버퍼를두stream의최종end중큰값까지전체hex로보존한다. recordsout은부모stack의마지막120BYTE다.',
            'query_devices':'islands는existsDWORD:currentOwnerDWORD:originalOwnerBYTE:regionBYTE의32BYTE레코드다. 실제getter가count/fort/모든필드를직접읽는다. 하위CALL경계없다. index순첫일치반환과signedBYTE비교는대체계산하지않는다.',
            'seed_devices':'실제004b37c0이index하위BYTE와record보존/부분쓰기를수행한다. Next004558c0만명시random입력으로대체하고인자254를확인한다. random경계중단은byte0/1변경뒤byte3변경전상태다.',
            'limits':['게임프로세스/GUI/Editor전체/전체snapshot/새디컴파일은실행하지않았다.','부모의Get/region/record/seed/Create/assert하위는명시장치다. 하위getter/seed본체는별도실행관측과구별한다.','각출력16KB/물리섬8개/최대100000명령범위다. 중단은실제CALL도달상태이며SEHunwind검증이아니다.'],
            'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE_PATH,Path(__file__),SAVE_PATH,QUERY_PATH,SEED_PATH)},'new_game_runs':0,'new_decompilations':0}
    REPORT_PATH.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print({kind:{'inputs':value['inputs'],'runs':value['precision_runs']} for kind,value in outputs.items()})


if __name__=='__main__':  # 직접실행시에만허용한새산출물을만든다.
    main()
