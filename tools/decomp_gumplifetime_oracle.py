#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump 지연 삭제/기본 소멸자 전체 몸체를 격리 실행한다. 게임/GUI/OS는 실행하지 않는다."""
import argparse
import collections
import copy
import csv
import itertools
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,STUBS,POOL,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 전체 노드/가상 표/보호 바이트의 격리 주소, 재현 시드와 저장 경로다.
DATA,SIZE,STRIDE,COUNT,SEED=0x18000000,160,192,6,0x46648078
TOKENS=tuple(DATA+16+i*STRIDE for i in range(COUNT))
VTABLE=DATA+0x9000
VIRTUAL={STUBS+0xa00:'Push',STUBS+0xa20:'Destructor',STUBS+0xa40:'Form'}
FIXTURE=ROOT/'cpppj/tests/fixtures/gumplifetime-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumplifetime-evidence.json'
# 실제 함수/표/스칼라 전역과 CRT free 경계다. CRT 복사는 원본 몸체로 실행한다.
SPECS={
    'originals':dict(Kill=0x466480,Cleanup=0x4662b0,Destroy=0x464c80,Base=0x464420,Scalar=0x466460,
        Idle=0x465330,Overlay=0x4653c0,Dialog=0x465550,registry=0x563e38,idle=0x5634d0,idle_size=500,
        overlay=0x563ca8,kill=0x564de0,dialogs=0x565d88,count=0x565dc8,kill_count=0x565dcc,
        dialog_count=0x565ddc,focus=0x565dd0,override=0x565dd8,current=0x565dd4,diagnostics=0x5e4794,
        pool=0x5c8464,stride=50,free=0x4e4354),
    'originalCD':dict(Kill=0x493a40,Cleanup=0x4929b0,Destroy=0x493b20,Base=0x492ef0,Scalar=0x4950d0,
        Idle=0x494380,Overlay=0x494410,Dialog=0x4945a0,registry=0x56b748,idle=0x56b5b8,idle_size=80,
        overlay=0x56a480,kill=0x56a618,dialogs=0x56a430,count=0x535e80,kill_count=0x535e84,
        dialog_count=0x535e94,focus=0x535e88,override=0x535e90,current=0x535e8c,diagnostics=DATA+0x8000,
        pool=0x5395dc,stride=36,free=0x4f1a50),
}
SPECS['original1037']=dict(SPECS['originalCD'])

def rle(values):
    """연속 같은 DWORD를 길이:값으로 줄인다. 기대 삭제 결과를 계산하지 않는다."""
    return ';'.join(f'{len(list(group))}:{value}' for value,group in itertools.groupby(values))

def node(index,**fields):
    """완성 노드 입력을 준비하고 미사용 DWORD는 독립 바이트로 남긴다."""
    raw=bytearray((37+index*31+i*17)&255 for i in range(SIZE))
    defaults={0:VTABLE,4:9,12:0,16:0,20:0,24:0,28:index,40:index,120:0,124:0}
    # float 표시 필드는 임의 입력 비트의 NaN 변환 없이 그대로 보존할 수 있게 유한 값으로 넣는다.
    for offset in (56,60,64,68,72,76,80): defaults[offset]=fbits(index+.25)
    defaults.update({int(offset):value for offset,value in fields.items()})
    # 입력 변경 위치/값만 쓰며 기대 flags/표 쓰기는 넣지 않는다.
    for offset,value in defaults.items(): struct.pack_into('<I',raw,offset,value&0xffffffff)
    return bytes(raw)

def changed(raw,**fields):
    """호출자가 지정한 DWORD만 변경한 독립 입력을 반환한다."""
    raw=bytearray(raw)
    # 응답 및 분기 입력은 모두 명시한 raw 값이다.
    for offset,value in fields.items(): struct.pack_into('<I',raw,int(offset),value&0xffffffff)
    return bytes(raw)

def plan(edition):
    """표 후보/중복/자식 우선/현재 콜백 변경/실제 기본 가상 소멸자의 독립 입력을 만든다."""
    result=[];capacity=SPECS[edition]['idle_size'];a,b,c,d,e,f=TOKENS
    # 시나리오마다 전체 노드/표를 독립 복사한다. 기대 표/flags는 실행 관찰로만 얻는다.
    def add(actions,nodes=None,globals=None,tables=None,responses=(),binding=0):
        """원본에 전달할 연산과 완성 입력만 저장한다."""
        base_tables=[[0]*1000,[0]*capacity,[0]*100,[0]*1000,[0]*16]
        base_tables[0][:COUNT]=TOKENS
        result.append(dict(actions=list(actions),nodes=nodes or [node(i) for i in range(COUNT)],
            globals=globals or [12,0,0,b,b,c,0],tables=copy.deepcopy(tables or base_tables),responses=list(responses),binding=binding))
        return result[-1]
    # Idle의 첫 후보와 판본별 마지막 칸, 중복 해제 및 같은 플래그 호출의 무변경을 교차한다.
    for hole in (0,1,39,capacity-1):
        case=add([('Idle',b,1),('Idle',b,1),('Idle',b,0),('Idle',b,0)])
        case['tables'][1]=[c]*capacity;case['tables'][1][hole]=0
    for flags in (0x89,0x100089,0x80000089):
        case=add([('Idle',b,0)],nodes=[node(i,**({'4':flags} if i==1 else {})) for i in range(COUNT)])
        case['tables'][1][1]=case['tables'][1][-1]=b
    # Overlay는 마지막 빈 칸을 사용하고 해제는 마지막 중복만 지운다.
    for hole in (0,1,50,99):
        case=add([('Overlay',b,1),('Overlay',b,0)]);case['tables'][2]=[c]*100;case['tables'][2][hole]=0
    # 여러 빈 칸의 선택 위치는 해제 전에 관찰한다. 등록/해제 왕복만으로는 후보 차이가 사라진다.
    add([('Overlay',b,1)])
    case=add([('Overlay',b,1)]);case['tables'][2]=[c]*100
    # 떨어진 세 빈 칸을 명시해 첫 후보와 마지막 후보를 구분한다.
    for hole in (1,50,99): case['tables'][2][hole]=0
    for flags in (9,0x80009,0xffffffff):
        case=add([('Overlay',b,0)],nodes=[node(i,**({'4':flags} if i==1 else {})) for i in range(COUNT)])
        case['tables'][2][0]=case['tables'][2][99]=b
    add([('Overlay',b,0)],nodes=[node(i,**({'4':0x80009} if i==1 else {})) for i in range(COUNT)])
    # Dialog의 모든 삭제 위치/부재/중복과 CRT 겹침 복사를 실제 몸체로 관찰한다.
    for length in (0,1,2,8,16):
        # first/중간/last/없는 token에 대해 입력만 만든다.
        for index in range(-1,length):
            case=add([('Dialog',b)]);case['globals'][2]=length;case['tables'][4]=[c]*16
            if index>=0: case['tables'][4][index]=b
    case=add([('Dialog',b),('Dialog',b)]);case['globals'][2]=3;case['tables'][4][:3]=[b,c,b]
    # Kill은 dead/void/미등록을 중복 제거 없이 처리하며 idle/overlay/Form/Push 순서를 유지한다.
    for flags in (1,9,0x49,0x41,0x89,0x80009,0x800c1,0x10009,0x100001,0x900c9):
        case=add([('Kill',b),('Kill',b)],nodes=[node(i,**({'4':flags} if i==1 else {})) for i in range(COUNT)])
        if flags&0x80: case['tables'][1][0]=b
        if flags&0x80000: case['tables'][2][-1]=b
        case['globals'][2]=3;case['tables'][4][:3]=[b,c,b]
    # 요청의 끝 칸과 큐 외부의 기존 데이터도 입력 그대로 남겨 원본 쓰기 폭을 확인한다.
    case=add([('Kill',b)]);case['globals'][1]=998;case['tables'][3]=[0x12345678]*1000
    # 자식/손자 순서, next의 호출 전 보관과 현재 부모 flags/포인터 변화다.
    nodes=[node(i) for i in range(COUNT)]
    nodes[1]=changed(nodes[1],**{'24':c});nodes[2]=changed(nodes[2],**{'4':1,'12':d,'24':e});nodes[4]=changed(nodes[4],**{'4':1})
    add([('Kill',b)],nodes,responses=[('Push',c,((c,12,0),(b,4,1),(0,3,b)))])
    # Form 콜백이 void를 켜거나 끄면 뒤의 Push 판정은 현재 flags를 읽는다.
    for flags,reply in ((0x41,9),(0x49,1)):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'4':flags})
        add([('Kill',b)],nodes,responses=[('Form',1,((b,4,reply),(0,1,2),(4,0,c)))])
    # 가상 소멸자 호출 이전 등록 해제/dead 제거/destroying과 current id/카운터 변경을 관찰한다.
    for flags in (9,0x10009,0x100009,0x80000009,0xffffffff):
        add([('Destroy',b)],nodes=[node(i,**({'4':flags} if i==1 else {})) for i in range(COUNT)])
    nodes=[node(i) for i in range(COUNT)]
    nodes[1]=changed(nodes[1],**{'24':c});nodes[2]=changed(nodes[2],**{'12':d,'24':e})
    add([('Destroy',b)],nodes,responses=[('Destructor',e,((c,12,0),(b,28,999),(0,0,123),(1,999,b)))])
    # Cleanup의 null/중복 및 콜백이 늘리거나 줄인 현재 큐 범위다.
    for queue in ([],[0],[b],[0,b,0,c],[b,b,c]):
        nodes=[node(i,**{'4':0x10009}) for i in range(COUNT)];case=add([('Cleanup',0)],nodes,binding=1 if queue==[b,b,c] else 0)
        case['globals'][1]=len(queue);case['tables'][3][:len(queue)]=queue
    for binding in (0,1):
        nodes=[node(i,**{'4':0x10009}) for i in range(COUNT)];case=add([('Cleanup',0)],nodes,binding=binding)
        case['globals'][1]=2;case['tables'][3][:4]=[b,c,0,d]
        if not binding: case['responses']=[('Destructor',b,((0,1,4),)),('Destructor',c,((0,1,1),))]
    # 기본 소멸자는 3개 표의 모든 중복만 지우며 등록/dialog/flags/개수는 남긴다.
    for text,alternate in ((0,0),(0x11111111,0),(0,0x22222222),(0x11111111,0x22222222),(0x11111111,0x11111111)):
        # scalar의 하위 바이트 bit 1과 상위 비트의 무관함을 교차한다.
        for op,flag in (('Base',0),('Scalar',0),('Scalar',1),('Scalar',2),('Scalar',255),('Scalar',256)):
            nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'4':0x10008d,'120':text,'124':alternate})
            case=add([(op,b,flag)] if op=='Scalar' else [(op,b)],nodes)
            case['globals'][1:3]=[2,3];case['globals'][6]=1
            # 마지막 칸 및 사용 개수 밖의 일치도 제거 대상이다.
            for table in case['tables'][1:4]: table[0]=table[1]=table[-1]=b;table[2]=c
            case['tables'][4][:3]=[b,c,b]
    nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'120':0x11111111,'124':0x22222222})
    add([('Base',b)],nodes,responses=[('Free',0x11111111,((b,120,0x33333333),(b,124,0x44444444),(3,50,b),(4,900,b)))])
    # 실제 기본 가상 소멸자를 호출하면 자식 우선 요청 큐의 중복/이미 해제한 자식이 null이 된다.
    nodes=[node(i,**{'4':1}) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'24':c});nodes[2]=changed(nodes[2],**{'12':d,'24':e})
    add([('Kill',b),('Cleanup',0)],nodes,binding=1,responses=[('Push',e,((c,24,0),)),('Push',c,((b,24,d),)),('Push',d,((b,24,0),))])
    rng=random.Random(SEED)
    # 미사용 바이트/플래그/등록 id/후보 위치의 독립 유한 입력을 더한다.
    for index in range(64):
        flags=rng.getrandbits(32)&~(0x80|0x80000);target=1+index%5
        nodes=[node(i) for i in range(COUNT)];nodes[target]=changed(nodes[target],**{'4':flags,'28':rng.randrange(1000)})
        case=add([('Kill',TOKENS[target])],nodes);case['globals'][1]=rng.randrange(900)
    return result

def state(globals,tables):
    """스칼라와 전체 다섯 표를 raw DWORD 순서/RLE로 직렬화한다."""
    return '#'.join([','.join(map(str,globals)),*(rle(values) for values in tables)])

def encoded(case):
    """기대 결과를 계산하지 않고 완성 입력만 fixture 열로 직렬화한다."""
    return [state(case['globals'],case['tables']),';'.join(f'{token}={raw.hex()}' for token,raw in zip(TOKENS,case['nodes'],strict=True)),
        ';'.join(','.join(map(str,action)) for action in case['actions']),
        ';'.join(f'{kind},{token}:'+ '/'.join(','.join(map(str,patch)) for patch in patches) for kind,token,patches in case['responses']) or '-',str(case['binding'])]

class GumpLifetimeOracle(ProcessOracle):
    """전체 삭제/기본 소멸자/CRT 복사를 실행하며 명시 외부 경계의 현재 상태를 기록한다."""
    def __init__(self,edition):
        """각 PE의 불연속 함수 범위와 명시 가상 표/전역을 준비한다."""
        super().__init__(edition);self.d=SPECS[edition];self.allowed=[];self.entry_names={}
        self.extra=[ROOT/f'extracted/gumplifetime/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 내보낸 몸체 이외 코드는 실행을 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 함수 사이 빈 영역은 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x10000);d=self.d
        self.global_addresses=[d[name] for name in ('count','kill_count','dialog_count','focus','override','current','diagnostics')]
        self.table_addresses=[d[name] for name in ('registry','idle','overlay','kill','dialogs')]
        self.table_sizes=[1000,d['idle_size'],100,1000,16]
        self.returns,self.boundaries=collections.Counter(),collections.Counter()

    def nodes(self):
        """모든 현재 노드의 전체 0xa0바이트를 기록한다."""
        return ';'.join(f'{token}={bytes(self.mu.mem_read(token,SIZE)).hex()}' for token in TOKENS)

    def state(self):
        """콜백 직전의 스칼라와 전체 표를 기록한다."""
        return state([self.u32(a) for a in self.global_addresses],
            [struct.unpack('<'+'I'*size,self.mu.mem_read(a,size*4)) for a,size in zip(self.table_addresses,self.table_sizes,strict=True)])

    def respond(self,kind,token,args):
        """실제 경계 상태/인자를 관찰한 뒤 완성 입력 DWORD 응답만 적용한다."""
        self.calls.append(','.join(map(str,(kind,token,*args)))+'@'+self.state()+'@'+self.nodes());self.boundaries[kind]+=1
        # 기대 삭제 결과를 계산하지 않고 입력이 지정한 위치/값만 쓴다.
        for target,offset,value in self.responses.get((kind,token),()):
            address=self.global_addresses[offset] if target==0 else self.table_addresses[target-1]+offset*4 if target<=5 else target+offset
            self.put(address,value)

    def on_instruction(self,mu,address,size,data):
        """Push/파생 소멸자/Form/free만 경계이며 그 외 명령은 실제 PE에서 실행한다."""
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX);kind=VIRTUAL.get(address)
        if kind or address==self.d['free']:
            kind=kind or 'Free';count=0 if kind in ('Push','Free') else 1
            token=self.u32(esp+4) if kind=='Free' else (ecx-POOL)//self.d['stride'] if kind=='Form' else ecx
            args=tuple(self.u32(esp+4+i*4) for i in range(count));self.respond(kind,token,args)
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4+count*4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(low<=address and address+size<=high for low,high in self.allowed): raise RuntimeError(f'Gump 수명 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """연속 연산의 정상 반환/ABI/x87/SEH/읽기 전용/보호/전체 현재 상태를 관찰한다."""
        mu,d=self.mu,self.d;self.entries,self.row_returns=collections.Counter(),collections.Counter();self.calls,self.results=[],[]
        self.responses={(kind,token):patches for kind,token,patches in case['responses']}
        mu.mem_write(DATA,b'\xa5'*0x1000)
        # 전체 객체와 실제 전역 표는 완성 입력 바이트다.
        for token,raw in zip(TOKENS,case['nodes'],strict=True): mu.mem_write(token,raw)
        # 스칼라 전역은 raw DWORD로 준비한다.
        for address,value in zip(self.global_addresses,case['globals'],strict=True): self.put(address,value)
        # 인접한 원본 표 사이를 덮지 않고 각 표의 정확한 길이만 준비한다.
        for address,values in zip(self.table_addresses,case['tables'],strict=True): mu.mem_write(address,struct.pack('<'+'I'*len(values),*values))
        self.put(d['pool'],POOL);mu.mem_write(POOL,b'\0'*0x1000)
        # 가상 소멸자는 명시 경계 또는 실제 기본 scalar 몸체를 사용한다.
        for offset,address in ((0,d['Scalar'] if case['binding'] else STUBS+0xa20),(16,STUBS+0xa40),(24,STUBS+0xa00)): self.put(VTABLE+offset,address)
        # Form SID별 가상 표만 제공하며 다른 필드는 읽기 전용이다.
        for sid in range(COUNT): self.put(POOL+sid*d['stride'],VTABLE)
        pool_before=bytes(mu.mem_read(POOL,0x1000));vtable_before=bytes(mu.mem_read(VTABLE,64))
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000)]
        self.writes.extend((token,token+SIZE) for token in TOKENS)
        self.writes.extend((address,address+4) for address in self.global_addresses)
        self.writes.extend((address,address+size*4) for address,size in zip(self.table_addresses,self.table_sizes,strict=True))
        # 표 바깥 8바이트 중 다른 표/전역과 겹치지 않는 바이트만 별도 보호한다.
        occupied=set()
        for low,high in self.writes: occupied.update(range(low,high))
        guards=sorted({p for address,size in zip(self.table_addresses,self.table_sizes,strict=True) for p in (*range(address-8,address),*range(address+size*4,address+size*4+8)) if p not in occupied})
        for address in guards: mu.mem_write(address,b'\xa5')
        # 호출 프레임만 초기화하고 노드/전역은 연속 연산 사이에 유지한다.
        for action in case['actions']:
            op,token=action[:2];args=(token,) if op=='Dialog' else action[2:];purge=0 if op=='Dialog' else len(args)*4
            # 비영 보존 레지스터와 임시 상태를 독립 초기화한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX): mu.reg_write(reg,0)
            mu.reg_write(UC_X86_REG_ECX,token);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
            mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*(v&0xffffffff for v in args)))
            mu.emu_start(d[op],STOP,timeout=30000000,count=2000000)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge or any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()): raise RuntimeError('Gump 수명 정상 반환/ABI 오류')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Gump 수명 x87/SEH 오류')
            self.row_returns[f'{d[op]:08x}']+=1;self.returns[f'{d[op]:08x}']+=1
            self.results.append(str(mu.reg_read(UC_X86_REG_EAX)) if op in ('Dialog','Scalar') else '-')
        protected=bytearray(mu.mem_read(DATA,0x1000))
        # 현재 노드의 출력만 제외하고 사이/전후 미사용 영역을 대조한다.
        for token in TOKENS: protected[token-DATA:token-DATA+SIZE]=b'\xa5'*SIZE
        if protected!=b'\xa5'*0x1000 or any(bytes(mu.mem_read(p,1))!=b'\xa5' for p in guards): raise RuntimeError('Gump 수명 보호 바이트 변경')
        if pool_before!=bytes(mu.mem_read(POOL,0x1000)) or vtable_before!=bytes(mu.mem_read(VTABLE,64)): raise RuntimeError('Gump 수명 읽기 전용 입력 변경')
        return [*encoded(case),self.nodes(),self.state(),'/'.join(self.results),'|'.join(self.calls) or '-',
            json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]

def generate(smoke):
    """두 x87 관찰이 같을 때만 fixture를 저장한다. C++/기대 삭제 계산은 호출하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumplifetime-functions.json',ROOT/'tools/decomp_gumplifetime_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py'}
    # 세 PE를 독립 메모리에서 정상 반환까지 실행한다.
    for edition in BINARIES:
        oracle=GumpLifetimeOracle(edition);indexed=list(enumerate(plan(edition)))
        if smoke: indexed=indexed[::7]+indexed[-5:]
        # 전역/표/노드/경계 당시 전체 관찰을 두 정밀도에서 대조한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump 수명 x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump 수명/CRT 겹침 복사 전체 몸체 관찰. 가상 Push/파생 소멸자/Form/free만 명시 경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['삭제 요청/정리/등록 해제/기본·scalar 소멸자/idle·overlay·dialog 해제와 CRT 복사 전체 정상 몸체',
            '전체 여섯 0xa0바이트/다섯 표/스칼라/콜백 당시 전체 상태·인자를 실제 관찰; 기본 scalar 가상 바인딩 포함',
            '가상 Push/파생 소멸자/Form 삭제/free만 명시 응답; 실제 파생 소멸자·메모리 allocator·assert/log/예외 전파 제외',
            '최상위 정상 반환/재귀 진입/보존 레지스터/스택/x87/SEH/허용 실행·쓰기/읽기 전용/보호 바이트/SHA 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

def verify():
    """SHA/전체 입력/바이트 폭/실제 진입·반환·외부 경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 도구/원본/내보내기/fixture의 정확한 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump 수명 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Gump 수명 총계 오류')
    # 저장된 실제 관찰로만 집계하고 기대 flags/삭제 표를 다시 계산하지 않는다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan(edition)) or item['cases']!=len(chosen): raise RuntimeError('Gump 수명 판본 행 수 오류')
        # 완성 입력 순서/전체 표 길이/전체 raw 노드 폭을 확인한다.
        for index,(row,case) in enumerate(zip(chosen,plan(edition),strict=True)):
            if len(row)!=13 or row[1]!=str(index) or row[2:7]!=encoded(case): raise RuntimeError('Gump 수명 입력/폭 오류')
            if len(row[7].split(';'))!=COUNT or any(len(bytes.fromhex(part.split('=')[1]))!=SIZE for part in row[7].split(';')): raise RuntimeError('Gump 수명 노드 폭 오류')
            parts=row[8].split('#')
            if len(parts)!=6 or len(parts[0].split(','))!=7 or [sum(int(p.split(':')[0]) for p in values.split(';')) for values in parts[1:]]!=[1000,SPECS[edition]['idle_size'],100,1000,16]: raise RuntimeError('Gump 수명 표 폭 오류')
            # 두 제어 모드의 실제 관찰 횟수는 저장 한 행 집계의 두 배다.
            for address,count in json.loads(row[11]).items(): entries[address]+=count*2
            for address,count in json.loads(row[12]).items(): returns[address]+=count*2
            if dict(collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions']))!=json.loads(row[12]): raise RuntimeError('Gump 수명 정상 반환 누락')
            if row[10]!='-':
                # 실제 호출 당시 기록된 종류만 집계한다.
                for call in row[10].split('|'): boundaries[call.split(',')[0]]+=2
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump 수명 진입/반환/경계 집계 오류')
    print(f'gumplifetime 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {sum(sum(v["returns"].values()) for v in report["editions"].values())}회, OS/assert 0')

def main():
    """전체 기록/무저장 표본/저장 감사를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)

if __name__=='__main__': main()
