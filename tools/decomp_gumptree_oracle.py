#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Gump 부착/밀어내기/정렬/깊이/이동의 세 PE 전체 몸체를 격리 실행한다. 게임/GUI/OS는 실행하지 않는다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,STUBS,POOL,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 완성 노드/결과/가상 표의 격리 주소와 재현 가능한 시드/출력 경로다.
DATA,SIZE,STRIDE,COUNT,SEED=0x17000000,160,192,6,0x46675020
TOKENS=tuple(DATA+16+i*STRIDE for i in range(COUNT))
RESULT,VTABLE=DATA+0x7000,DATA+0x9000
VIRTUAL={STUBS+0x900:'Redraw',STUBS+0x920:'Push',STUBS+0x940:'Attach'}
FIXTURE=ROOT/'cpppj/tests/fixtures/gumptree-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumptree-evidence.json'
# 실제 실행 함수/전역/응답 경계다. CD의 진단 모드는 미사용 격리 입력이다.
SPECS={
    'originals':dict(Attach=0x466750,Push=0x464560,Align=0x464600,AddDepth=0x4652b0,SetDepth=0x4652f0,
        Move=0x464d10,Relative=0x464d60,root=0x563e38,screen=0x5c78f0,map=0x5ca9cc,debug=0x5318ec,
        diagnostics=0x5e4794,pool=0x5c8464,stride=50,project=0x497220,kill=0x466480),
    'originalCD':dict(Attach=0x493070,Push=0x493000,Align=0x493370,AddDepth=0x494300,SetDepth=0x494340,
        Move=0x493bb0,Relative=0x493c00,root=0x56b748,screen=0x516b10,map=0x583e20,debug=DATA+0x8000,
        diagnostics=0x531454,pool=0x5395dc,stride=36,project=0x455c90,kill=0x493a40),
}
SPECS['original1037']=dict(SPECS['originalCD'])

def node(index,**fields):
    """전체 미사용 필드를 임의 입력으로 남기고 분기에 필요한 raw 필드만 명시한다."""
    value=bytearray((37+index*31+i*17)&255 for i in range(SIZE))
    defaults={0:VTABLE,4:9,8:0,12:0,16:0,20:TOKENS[0] if index else 0,24:0,28:index,32:27,36:index,
        48:0xffffffd8,52:0,56:fbits(1),60:fbits(12.75),64:fbits(9.5),68:fbits(12.75),72:fbits(9.5),
        76:fbits(2.75),80:fbits(-1.5),84:fbits(.75),88:fbits(-.25)}
    defaults.update({int(offset):value for offset,value in fields.items()})
    # 모든 변경은 완성 입력 DWORD이며 기대 정렬/등록/깊이는 넣지 않는다.
    for offset,item in defaults.items(): struct.pack_into('<I',value,offset,item&0xffffffff)
    return bytes(value)

def changed(raw,**fields):
    """완성 입력의 지정 DWORD만 바꾼 새 바이트를 만든다."""
    value=bytearray(raw)
    # 입력 변경 위치는 호출자가 명시한다.
    for offset,item in fields.items(): struct.pack_into('<I',value,int(offset),item&0xffffffff)
    return bytes(value)

def plan():
    """판본/정렬 비트/형제/현재 콜백 변경/월드/깊이/이동 순서의 독립 입력을 만든다."""
    result=[]
    # 노드 여섯 개/현재 전역/명시 응답은 모든 시나리오에서 독립 복사한다.
    def add(actions,nodes=None,screen=(101,79),map_view=(-9,7,82,64),debug=0,responses=(),anchors=((1,30,-20),)):
        """원본에 전달할 입력만 저장한다. 기대 링크/위치/호출은 원본 실행으로 얻는다."""
        result.append(dict(actions=actions,nodes=nodes or [node(i) for i in range(COUNT)],
            globals=(TOKENS[0],*screen,*map_view,debug),responses=list(responses),anchors=list(anchors)))
    # 256개 정렬 비트 조합은 루트에서 모두 실행하며 가운데→끝→지도 중심의 우선순위를 관찰한다.
    for align in range(256):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'8':align,'56':fbits(1.25)})
        add([('Align',TOKENS[1])],nodes)
    # 일반 부모에서는 지도 중심 비트를 제외하고 기본/현재 크기와 scale을 구분한다.
    for align in range(64):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'20':TOKENS[2],'8':align,'56':fbits(-.5)})
        nodes[2]=changed(nodes[2],**{'60':fbits(52.5),'64':fbits(31.25),'68':fbits(400),'72':fbits(500)})
        add([('Align',TOKENS[1])],nodes)
    # signed 중심 덧셈 감김/정수→float/화면 크기 한계와 음수 0을 드러낸다.
    for screen in ((16777217,-16777217),(2147483647,-2147483648),(0,0)):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'8':0xc0})
        add([('Align',TOKENS[1])],nodes,screen=screen,map_view=(2147483647,-2147483648,1,-1))
    # 부착의 첫 형제 삽입·깊이 상속/보존·화면 제한/정렬 생략 조합이다.
    for flags in (9,9|0x200000,9|0x20000,9|0x2000,9|0x4000):
        # 루트/일반 부모와 기존 부모 재사용을 나눈다.
        for parent in (0,TOKENS[0],TOKENS[2]):
            # 좌표/크기/부착 모드마다 순서와 전체 바이트를 기록한다.
            for mode in (0,1,2,3):
                nodes=[node(i) for i in range(COUNT)]
                nodes[1]=changed(nodes[1],**{'4':flags,'8':0x14,'52':0xfffffff9,'16':TOKENS[5]})
                nodes[2]=changed(nodes[2],**{'48':0x80000001,'24':TOKENS[3]})
                nodes[0]=changed(nodes[0],**{'24':TOKENS[3]})
                nodes[3]=changed(nodes[3],**{'4':0x14001,'12':TOKENS[4]});nodes[4]=changed(nodes[4],**{'4':0x4001})
                add([('Attach',TOKENS[1],fbits(120.75),fbits(-9.5),parent,mode)],nodes)
    # 기본 깊이 -3/명시 secondary/넘침과 CD/Patch의 큰 화면 및 0 부호 차이다.
    for secondary in (0,3,0xffffffff,0x7fffffff):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'52':secondary})
        nodes[2]=changed(nodes[2],**{'48':0x7fffffff})
        add([('Attach',TOKENS[1],fbits(.25),fbits(.5),TOKENS[2],0)],nodes)
    for x in (0x80000000,fbits(16777216),fbits(-100),fbits(300)):
        # 분수 폭과 정수 폭은 큰 화면 크기의 float 선반올림 차이를 각각 드러낸다.
        for width in (.25,2):
            nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'68':fbits(width),'72':fbits(200)})
            add([('Attach',TOKENS[1],x,x,TOKENS[0],0)],nodes,screen=(16777217,79))
    # 밀어내기: 머리/중간/마지막/루트, 자기 링크 보존, 패치 진단 3/4를 관찰한다.
    for target in (0,1,2,3):
        # 원본 진단 분기는 CD에서는 전역을 읽지 않는다.
        for debug in (0,3,4):
            nodes=[node(i) for i in range(COUNT)];nodes[0]=changed(nodes[0],**{'4':1,'24':TOKENS[1]})
            # 서로 연결된 세 형제를 완성 입력으로 공급한다.
            for i in (1,2,3): nodes[i]=changed(nodes[i],**{'4':1,'12':TOKENS[i+1] if i<3 else 0,'16':TOKENS[i-1] if i>1 else 0})
            add([('Push',TOKENS[target])],nodes,debug=debug)
    # 자식/손자 재배치: 콜백 이전 next 보관과 이후 현재 secondary/align/parent를 구분한다.
    for response_mode in range(4):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'24':TOKENS[3]})
        nodes[3]=changed(nodes[3],**{'20':TOKENS[1],'8':0x14,'52':0xfffffffc,'12':TOKENS[4]})
        nodes[4]=changed(nodes[4],**{'20':TOKENS[1],'8':0,'52':7,'12':TOKENS[5]})
        nodes[5]=changed(nodes[5],**{'20':TOKENS[1],'8':0,'52':0})
        responses=[]
        if response_mode==1: responses=[('Redraw',TOKENS[1],((TOKENS[1],48,0x7fffffff),(TOKENS[3],52,11),(TOKENS[3],8,0x20)))]
        if response_mode==2: responses=[('Push',TOKENS[3],((TOKENS[3],12,0),(TOKENS[3],52,0),(TOKENS[3],8,3),(TOKENS[3],56,fbits(2))))]
        if response_mode==3: responses=[('Redraw',TOKENS[1],((TOKENS[1],24,TOKENS[4]),(TOKENS[4],52,0),(TOKENS[4],8,0)))]
        add([('Attach',TOKENS[1],fbits(7.25),fbits(8.5),TOKENS[0],0)],nodes,responses=responses)
    # 삭제/투영의 명시 상태 변경 뒤에 읽는 현재 부모/머리/화면 크기도 관찰한다.
    nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'4':0x4009});nodes[0]=changed(nodes[0],**{'24':TOKENS[3]});nodes[3]=changed(nodes[3],**{'4':0x4001,'12':TOKENS[4]});nodes[4]=changed(nodes[4],**{'4':0x4001})
    add([('Attach',TOKENS[1],fbits(8),fbits(9),TOKENS[0],0)],nodes,responses=[('Kill',TOKENS[3],((TOKENS[3],12,0),(TOKENS[0],24,TOKENS[4])))])
    nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'4':0x2009})
    add([('Attach',TOKENS[1],fbits(120),fbits(130),TOKENS[0],0)],nodes,responses=[('Project',1,((0,1,200),(0,2,300),(TOKENS[1],4,1)))])
    # 깊이 재귀는 visible/dead/void 필터 없이 모든 자식을 먼저 갱신한다.
    for op in ('AddDepth','SetDepth'):
        # 한 칸 트리/형제와 손자/감김 DWORD를 교차한다.
        for value in (0,3,0xffffffff,0x80000000):
            nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'24':TOKENS[2]});nodes[2]=changed(nodes[2],**{'24':TOKENS[4],'12':TOKENS[3]})
            nodes[3]=changed(nodes[3],**{'4':0x10008});nodes[4]=changed(nodes[4],**{'48':0x7fffffff})
            add([(op,TOKENS[1],value)],nodes)
    # 단순 이동은 기준점 보정 뒤 Push, 상대 이동은 Push 뒤 현재 기준 노드의 위치/크기를 읽는다.
    for flags in (1,9,0x11,0x19):
        # 명시 가상 Push 응답이 입력을 바꿔도 각 함수의 읽는 시점을 관찰한다.
        for response_mode in (0,1):
            nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'4':flags})
            responses=[('Push',TOKENS[1],((TOKENS[1],84,fbits(100)),(TOKENS[1],20,TOKENS[2]),(TOKENS[2],76,fbits(20)),(TOKENS[2],68,fbits(50))))] if response_mode else []
            add([('Move',TOKENS[1],fbits(8.25),fbits(-3.75),TOKENS[2])],nodes,responses=responses)
            # 같은 부모/다른 기준과 너비/높이 기준점 비트를 나눈다.
            for reference in (TOKENS[0],TOKENS[2]):
                # 인접 비트는 기준 크기를 더하지 않는다.
                for anchor in (0,4,8,12,0xff): add([('Relative',TOKENS[1],reference,fbits(8.25),fbits(-3.75),anchor,TOKENS[2])],nodes,responses=responses)
    # 연속 밀어내기/재부착은 남겨 둔 부모를 사용하며 개별 현재 상태가 이어진다.
    nodes=[node(i) for i in range(COUNT)]
    add([('Attach',TOKENS[1],fbits(5),fbits(6),TOKENS[0],0),('Push',TOKENS[1]),('Attach',TOKENS[1],fbits(7),fbits(8),0,1)],nodes)
    rng=random.Random(SEED)
    # 유한 분수/scale/깊이/크기의 완성 입력만 무작위로 더한다.
    for index in range(64):
        nodes=[node(i) for i in range(COUNT)];nodes[1]=changed(nodes[1],**{'8':rng.randrange(256),'56':fbits(rng.randrange(-8,9)/4),'52':rng.getrandbits(32),'68':fbits(rng.randrange(-30,100)/8),'72':fbits(rng.randrange(-30,100)/8)})
        add([('Attach',TOKENS[1],fbits(rng.randrange(-200,201)/8),fbits(rng.randrange(-200,201)/8),TOKENS[0],index%4)],nodes)
    return result

def encoded(case):
    """완성 전역/바이트/연산/응답을 C++용 열로 직렬화한다. 기대 출력 계산은 하지 않는다."""
    return [','.join(map(str,case['globals'])),';'.join(f'{token}={raw.hex()}' for token,raw in zip(TOKENS,case['nodes'],strict=True)),
        ';'.join(','.join(map(str,action)) for action in case['actions']),
        ';'.join(f'{kind},{token}:'+ '/'.join(','.join(map(str,patch)) for patch in patches) for kind,token,patches in case['responses']) or '-',
        ';'.join(','.join(map(str,anchor)) for anchor in case['anchors']) or '-']

class GumpTreeOracle(ProcessOracle):
    """전체 트리 함수만 실행하고 가상 처리/삭제 요청/투영은 명시 입력 응답으로 처리한다."""
    def __init__(self,edition):
        """기존 PE/쓰기 검사기에 이번 정확한 불연속 범위와 가상 표를 연결한다."""
        super().__init__(edition);self.d=SPECS[edition];self.allowed=[];self.entry_names={}
        self.extra=[ROOT/f'extracted/gumptree/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이 주소를 실행하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 실제 마지막 바이트까지의 몸체만 허용한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x10000)
        # 세 가상 슬롯은 응답 경계이며 실행 전에 현재 this/인자를 기록한다.
        for offset,address in ((8,STUBS+0x900),(24,STUBS+0x920),(32,STUBS+0x940)):
            self.put(VTABLE+offset,address);self.mu.mem_write(address,b'\xc3')
        d=self.d;self.global_addresses=[d['root'],d['screen'],d['screen']+4,*(d['map']+i*4 for i in range(4)),d['debug']]
        self.returns,self.boundaries=collections.Counter(),collections.Counter()

    def globals(self):
        """루트/화면/지도/진단 모드의 현재 DWORD를 signed 필요한 위치만 변환해 기록한다."""
        values=[self.u32(address) for address in self.global_addresses]
        # 화면/지도 좌표는 signed 입력이며 나머지는 raw DWORD다.
        for i in range(1,7): values[i]=struct.unpack('<i',struct.pack('<I',values[i]))[0]
        return ','.join(map(str,values))

    def nodes(self):
        """전체 현재 0xa0바이트 여섯 노드를 기록한다."""
        return ';'.join(f'{token}={bytes(self.mu.mem_read(token,SIZE)).hex()}' for token in TOKENS)

    def respond(self,kind,token,args):
        """가상/명시 처리 경계 당시 상태를 관찰하고 입력에 명시한 DWORD만 변경한다."""
        self.calls.append(','.join(map(str,(kind,token,*args)))+'@'+self.globals()+'@'+self.nodes());self.boundaries[kind]+=1
        # 기대 링크/크기/깊이를 계산하지 않는 완성 응답 바이트다.
        for target,offset,value in self.responses.get((kind,token),()):
            self.put(self.global_addresses[offset] if target==0 else target+offset,value)

    def on_instruction(self,mu,address,size,data):
        """경계 이외의 명령은 실제 PE를 실행하며 진입/콜백 당시 전체 상태를 기록한다."""
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        kind=VIRTUAL.get(address)
        if kind or address==self.d['kill']:
            kind=kind or 'Kill';count={'Redraw':1,'Push':0,'Attach':4,'Kill':0}[kind]
            args=struct.unpack('<'+'I'*count,mu.mem_read(esp+4,count*4)) if count else ()
            self.respond(kind,ecx,args);mu.reg_write(UC_X86_REG_EAX,0x12345678)
            mu.reg_write(UC_X86_REG_ESP,esp+4+count*4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if address==self.d['project']:
            pointer,sid,_=struct.unpack('<3I',mu.mem_read(esp+4,12));point=self.anchors[sid]
            self.respond('Project',sid,());mu.mem_write(ecx,struct.pack('<2i',*point));mu.mem_write(pointer,struct.pack('<2i',*point))
            mu.reg_write(UC_X86_REG_EAX,ecx);mu.reg_write(UC_X86_REG_ESP,esp+16);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(low<=address and address+size<=high for low,high in self.allowed): raise RuntimeError(f'Gump 트리 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """연속 연산 전체의 정상 반환/ABI/x87/SEH/보호/현재 상태를 실제 실행으로 얻는다."""
        mu,d=self.mu,self.d;self.entries,self.row_returns=collections.Counter(),collections.Counter();self.calls,self.results=[],[]
        self.responses={(kind,token):patches for kind,token,patches in case['responses']};self.anchors={sid:(x,y) for sid,x,y in case['anchors']}
        mu.mem_write(DATA,b'\xa5'*0x1000);mu.mem_write(RESULT-16,b'\xa5'*48)
        # 모든 노드 전후/미사용 공간과 SID 입력을 독립 바이트로 준비한다.
        for token,raw in zip(TOKENS,case['nodes'],strict=True): mu.mem_write(token,raw)
        # 전역과 투영용 SID 좌표는 완성 입력이며 PE에서 원본대로 읽는다.
        for address,value in zip(self.global_addresses,case['globals'],strict=True): self.put(address,value)
        self.put(d['diagnostics'],0);self.put(d['pool'],POOL);mu.mem_write(POOL,b'\0'*0x1000)
        # world x가 SID 입력 경계를 식별한다.
        for sid in range(COUNT): self.put(POOL+sid*d['stride']+14,sid)
        pool_before=bytes(mu.mem_read(POOL,0x1000));vtable_before=bytes(mu.mem_read(VTABLE,64))
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(RESULT,RESULT+8)]
        self.writes.extend((token,token+SIZE) for token in TOKENS)
        # 각 최상위 연산은 현재 노드/전역을 이어 쓰고 호출 프레임만 초기화한다.
        for action in case['actions']:
            op,token=action[:2];args=(RESULT,) if op=='Align' else action[2:]
            # 비영 보존 레지스터와 thiscall 인자를 준비한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            # 임시 상태/x87/SEH를 매 호출에 독립 초기화한다.
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX): mu.reg_write(reg,0)
            mu.reg_write(UC_X86_REG_ECX,token);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
            mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*(v&0xffffffff for v in args)))
            mu.emu_start(d[op],STOP,timeout=30000000,count=2000000)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4 or any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()): raise RuntimeError('Gump 트리 정상 반환/ABI 오류')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Gump 트리 x87/SEH 오류')
            self.row_returns[f'{d[op]:08x}']+=1;self.returns[f'{d[op]:08x}']+=1
            self.results.append(bytes(mu.mem_read(RESULT,8)).hex() if op=='Align' else '-')
        protected=bytearray(mu.mem_read(DATA,0x1000))
        # 실제 노드의 출력만 보호 비교에서 제외한다.
        for token in TOKENS: protected[token-DATA:token-DATA+SIZE]=b'\xa5'*SIZE
        if protected!=b'\xa5'*0x1000 or bytes(mu.mem_read(RESULT-16,16))+bytes(mu.mem_read(RESULT+8,24))!=b'\xa5'*40: raise RuntimeError('Gump 트리 보호 바이트 변경')
        if pool_before!=bytes(mu.mem_read(POOL,0x1000)) or vtable_before!=bytes(mu.mem_read(VTABLE,64)): raise RuntimeError('Gump 트리 읽기 전용 입력 변경')
        return [*encoded(case),self.nodes(),self.globals(),'/'.join(self.results),'|'.join(self.calls) or '-',
            json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]

def generate(smoke):
    """두 x87 관찰이 같을 때만 저장한다. C++ 호출/기대 트리 계산은 하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumptree-functions.json',ROOT/'tools/decomp_gumptree_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py'}
    # 세 PE는 별도 메모리에서 정상 반환까지 실행한다.
    for edition in BINARIES:
        oracle=GumpTreeOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::37]+indexed[-5:]
        # 명시 입력/현재 콜백의 전체 관찰을 두 정밀도에서 대조한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump 트리 x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump 트리 전체 몸체 관찰. 가상/삭제 요청/투영만 명시 입력 경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['부착/밀어내기/정렬/깊이/이동/지도 중심 전체 정상 몸체; 가상 +8/+0x18/+0x20·삭제 요청·월드 투영만 명시 응답',
            '전체 여섯 0xa0바이트/콜백 당시 전체 노드·전역·인자/현재 링크/정렬/판본별 float 저장을 실제 관찰',
            '유한 float 좌표/크기/scale·정상 트리·원본 assert/log 입력 및 실제 가상 처리기/삭제/예외 전파 제외',
            '최상위 정상 반환/재귀 진입/보존 레지스터/스택/x87/SEH/허용 실행·쓰기/읽기 전용/보호 바이트/SHA 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

def verify():
    """SHA/완성 입력/전체 바이트 폭/실제 진입·정상 반환·응답 경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 전체 도구/원본/내보내기/fixture 바이트를 감사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump 트리 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Gump 트리 총계 오류')
    # 실제 진입/최상위 정상 반환과 명시 경계는 저장 관찰로만 집계한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Gump 트리 판본 행 수 오류')
        # 기대 링크/정렬 바이트를 계산하지 않고 입력/순서/기록 폭을 확인한다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=13 or row[1]!=str(index) or row[2:7]!=encoded(case): raise RuntimeError('Gump 트리 입력/폭 오류')
            if len(row[7].split(';'))!=COUNT or any(len(bytes.fromhex(part.split('=')[1]))!=SIZE for part in row[7].split(';')): raise RuntimeError('Gump 트리 노드 폭 오류')
            # 두 정밀도 집계는 저장 행의 각 실제 호출 횟수의 두 배다.
            for address,count in json.loads(row[11]).items(): entries[address]+=count*2
            # 내부 재귀와 최상위 반환은 다른 집계다.
            for address,count in json.loads(row[12]).items(): returns[address]+=count*2
            expected=collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions'])
            if dict(expected)!=json.loads(row[12]): raise RuntimeError('Gump 트리 정상 반환 누락')
            if row[10]!='-':
                # 관찰된 각 응답 종류만 집계한다.
                for call in row[10].split('|'): boundaries[call.split(',')[0]]+=2
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump 트리 진입/반환/경계 집계 오류')
    print(f'gumptree 검증 통과: {len(rows)}개, 두 정밀도 최상위 정상 반환 {sum(sum(v["returns"].values()) for v in report["editions"].values())}회, OS/assert 0')

def main():
    """전체 기록/무저장 표본/저장 감사를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)

if __name__=='__main__': main()
