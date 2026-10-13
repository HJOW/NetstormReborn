#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump 커서 선택/이벤트 필터·전달을 격리 실행한다. 실제 위치/경계/CRT도 실행한다.

가상 필터/처리기와 월드 투영만 완성 입력 응답 경계다. 게임/GUI/OS는 실행하지 않는다.
"""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_gumpcollection_oracle import SPECS as COLLECTION
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,STUBS,POOL,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 전체 기본 노드/가상 표와 보호 영역의 격리 주소 및 입력만 만드는 재현 시드다.
DATA,SIZE,STRIDE,COUNT,VTABLE,SEED=0x19000000,160,192,6,0x19009000,0x46602028
TOKENS=tuple(DATA+16+i*STRIDE for i in range(COUNT))
# 가상 필터와 실제 처리기 경계는 스택 인자 세 DWORD의 원본 thiscall이다.
VIRTUAL={STUBS+0xb00:'Filter',STUBS+0xb20:'Handle'}
# 실제 새 함수와 선택/현재 dialog 전역 주소다.
SPECS={
    'originals':dict(Base=0x464dd0,Bubble=0x464e90,Dispatch=0x4651b0,Top=0x465ed0,Children=0x465f60,Pick=0x466020,
        globals=(0x563e38,0x565dd4,0x565de8,0x565dec,0x564dd8,0x563ca0,0x565d84)),
    'originalCD':dict(Base=0x493c70,Bubble=0x493d50,Dispatch=0x494070,Top=0x4924f0,Children=0x492590,Pick=0x492640,
        globals=(0x56b748,0x535e8c,0x56a428,0x56a42c,0x56a478,0x56a474,0x56a47c)),
}
SPECS['original1037']=dict(SPECS['originalCD'])
# 독립 원본 관찰과 SHA/입력/호출 감사 보고서의 저장 경로다.
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpevents-x86.tsv'
REPORT=ROOT/'cpppj/gumpevents.json'


def changed(raw,**fields):
    """완성 입력의 명시 offset/DWORD만 바꾼다. 기대 선택/전달 결과는 계산하지 않는다."""
    value=bytearray(raw)
    # 입력에 지정한 필드만 비트 그대로 쓴다.
    for offset,number in fields.items(): struct.pack_into('<I',value,int(offset),number&0xffffffff)
    return bytes(value)


def nodes():
    """루트·두 직계 자식·손자/형제·남은 노드의 완성 입력을 준비한다."""
    result=[]
    # 미사용 필드는 독립 바이트이며 위치 캐시도 기대 좌표로 초기화하지 않는다.
    for index in range(COUNT):
        raw=bytes((index*37+offset*13+19)&255 for offset in range(SIZE))
        result.append(changed(raw,**{'0':VTABLE,'4':1,'12':0,'20':0,'24':0,'36':index,'48':20-index,
            '68':fbits(12.75),'72':fbits(9.5),'76':fbits(10),'80':fbits(20),'108':99,'112':101}))
    result[0]=changed(result[0],**{'4':0,'24':TOKENS[1],'76':0,'80':0})
    result[1]=changed(result[1],**{'12':TOKENS[2],'20':TOKENS[0],'24':TOKENS[3]})
    result[2]=changed(result[2],**{'20':TOKENS[0]})
    result[3]=changed(result[3],**{'12':TOKENS[4],'20':TOKENS[1],'76':fbits(0),'80':fbits(0)})
    result[4]=changed(result[4],**{'20':TOKENS[1],'76':fbits(1),'80':fbits(1)})
    return result


def plan():
    """플래그/경계/동률·signed 깊이/전달 순서/현재 링크·가상 응답의 독립 입력을 만든다."""
    result=[]
    # 한 행의 여러 호출은 현재 상태를 공유한다. 없는 가상 응답의 명시 계약은 반환 0/쓰기 없음이다.
    def add(actions,raw=None,binding=0,responses=(),dialog=0,anchors=()):
        """노드/전역/연산/응답만 복사하며 출력/선택 토큰은 입력하지 않는다."""
        result.append(dict(actions=list(actions),nodes=list(raw or nodes()),binding=binding,responses=list(responses),
            anchors=list(anchors) if anchors else [(index,0,0) for index in range(COUNT)],
            globals=[TOKENS[0],dialog,17,0xffffffe9,0x1234,0x80000000,TOKENS[4]]))
    flags=(0,0x200,0x400,0x800,0x600,0xa00,0xc00,0xe00,0x1000200,0xffffffff)
    events=(0x10000,0x20000,0x4010000,0x4020000,0x2010000,0x2000000,0x2000001,1,0x80000001)
    points=((10,20),(21,28),(22,20),(10,29),(-1,-1))
    # help 사건 종류 마스크/좌우·상하의 half-open 경계와 독립 이벤트 플래그를 교차한다.
    for flag in flags:
        # 각 사건은 다른 경계 좌표를 연속 조회한다.
        for event in events:
            raw=nodes();raw[1]=changed(raw[1],**{'4':flag})
            add([('Base',TOKENS[1],event,x,y) for x,y in points],raw)
    # 현재 dialog는 flags/좌표와 무관하며 다른 dialog에는 적용하지 않는다.
    for event in events:
        add([('Base',TOKENS[1],event,-200,-300),('Base',TOKENS[2],event,-200,-300)],dialog=TOKENS[1])
    # 후속 전체 선택이 캐시를 다시 계산하기 전에 숨은/죽은 직계 자식의 경계 조회를 관찰한다.
    for flag in (0,0x10001):
        raw=nodes();raw[1]=changed(raw[1],**{'4':flag})
        add([('Top',0,11,21)],raw)
    # 직계/전체 선택의 void 차이, 화면 밖 통과와 visible/dead/후보 마스크를 검사한다.
    for flag in (0,1,9,0x21,0x29,0x10001,0x10021,0x201,0x401,0x80000001):
        # 자식의 안/밖 위치와 깊이가 다른 입력이다.
        for outside in (False,True):
            raw=nodes();raw[1]=changed(raw[1],**{'4':flag,'76':fbits(100 if outside else 10)})
            raw[3]=changed(raw[3],**{'76':fbits(-90 if outside else 0),'48':0xfffffff0})
            add([('Top',0,11,21),('Pick',0,11,21,0),('Pick',0,11,21,0x200),('Pick',0,11,21,0x80000000)],raw)
    # 깊이는 signed DWORD이며 동률에서는 방문한 첫 노드가 남는다.
    for depth in (0,999,1000,9999,10000,0xffffffff,0x80000000,0x7fffffff):
        # 동률 직계/손자와 손자의 더 작은 깊이를 나눈다.
        for tie in (False,True):
            raw=nodes()
            # 입력 깊이는 기대 선택과 무관하게 모든 후보에 지정한다.
            for index in (1,2,3,4): raw[index]=changed(raw[index],**{'48':depth})
            if not tie: raw[3]=changed(raw[3],**{'48':(depth-1)&0xffffffff})
            add([('Top',0,11,21),('Pick',0,11,21,0)],raw)
    # 빈/역전/분수 크기와 큰 float 좌표의 실제 위치/CRT 저장을 실행한다.
    for x,y,width,height in ((-.9,-1.9,12.75,9.5),(10,20,0,0),(10,20,-1,10),(10,20,10,-1),
        (16777216,0,.75,9.5),(-16777216,0,12.75,9.5)):
        raw=nodes();raw[1]=changed(raw[1],**{'76':fbits(x),'80':fbits(y),'68':fbits(width),'72':fbits(height)})
        add([('Top',0,10,20),('Pick',0,10,20,0),('Base',TOKENS[1],0x10000,10,20)],raw)
    # 실제 가상 필터 응답과 각 처리기 반환값을 독립 입력으로 공급한다.
    for include_hidden in (0,7):
        # 선택 첫 자식/형제/손자/부모 성공과 전체 실패를 교차한다.
        for winner in (-1,0,1,2,3,4):
            # 모든 필터/첫 필터 거부 및 dead/void/hidden 노드 전달을 나눈다.
            for flag in (0,1,9,0x10001):
                raw=nodes();raw[3]=changed(raw[3],**{'4':flag})
                responses=[('Filter',token,1,()) for token in TOKENS]
                if winner>=0: responses.append(('Handle',TOKENS[winner],0x80000007,()))
                add([('Dispatch',TOKENS[0],0x81234567,0x76543210,0xfedcba98,include_hidden)],raw,responses=responses)
    # 기본 필터를 실제 가상 +0x34에 바인딩하여 handler와 같은 원본 호출열을 실행한다.
    for flag in (0x200,0x400,0x800,0xe00):
        # 현재 dialog의 우선 소비도 실제 전달 트리에서 확인한다.
        for dialog in (0,TOKENS[3]):
            raw=nodes()
            # 서로 다른 이벤트 소비 플래그를 모든 노드에 지정한다.
            for index in range(COUNT): raw[index]=changed(raw[index],**{'4':flag|1})
            add([('Dispatch',TOKENS[0],0x10000,11,21,0),('Dispatch',TOKENS[0],1,11,21,0),
                ('Dispatch',TOKENS[0],0x2000001,11,21,0)],raw,binding=1,dialog=dialog,
                responses=[('Handle',token,3,()) for token in TOKENS])
    # 부모 전파는 자신을 제외하며 필터를 호출하지 않고 처리기 반환 전체 DWORD를 보존한다.
    for winner in (-1,0,1,3):
        add([('Bubble',TOKENS[3],0xffffffff,0x80000000,0x7fffffff)],responses=[] if winner<0 else [('Handle',TOKENS[winner],0x80000007,())])
    # 실패한 자식의 가상 필터/처리기가 현재 next를 바꾸면 새 형제로 이어간다.
    for kind in ('Filter','Handle'):
        raw=nodes();raw[1]=changed(raw[1],**{'24':0});raw[2]=changed(raw[2],**{'12':TOKENS[5]})
        responses=[('Filter',token,1,()) for token in TOKENS]
        responses.append((kind,TOKENS[1],0,((TOKENS[1],12,TOKENS[5]),)))
        responses.append(('Handle',TOKENS[5],5,()))
        add([('Dispatch',TOKENS[0],1,2,3,0)],raw,responses=responses)
    # 가상 필터가 현재 dialog/부모/자식의 소비 결과를 바꾸는 시점과 bubble의 새 부모 링크를 관찰한다.
    add([('Bubble',TOKENS[3],1,2,3)],responses=[('Handle',TOKENS[1],0,((TOKENS[1],20,TOKENS[2]),)),('Handle',TOKENS[2],9,())])
    add([('Dispatch',TOKENS[1],1,2,3,0)],responses=[('Filter',TOKENS[3],1,()),('Handle',TOKENS[3],0,((TOKENS[3],12,0),)),
        ('Filter',TOKENS[1],1,()),('Handle',TOKENS[1],7,())])
    # 원본 월드 부착 노드와 실제 SID/투영/CRT 위치 경로다. 이 두 입력은 투영 정수점만 공급한다.
    for sid in (1,2):
        raw=nodes();raw[sid]=changed(raw[sid],**{'4':0x2001,'76':fbits(.75),'80':fbits(-.75)})
        add([('Top',0,11,21),('Pick',0,11,21,0)],raw,anchors=[(sid,11,22)])
    rng=random.Random(SEED)
    # 유한 위치/플래그/깊이/마스크의 임의 조합을 추가한다.
    for index in range(48):
        raw=nodes()
        # 정상 링크는 유지하고 조회/선택 필드만 임의 값으로 바꾼다.
        for item in range(1,COUNT):
            raw[item]=changed(raw[item],**{'4':rng.choice((0,1,9,0x21,0x201,0x10001)),
                '48':rng.getrandbits(32),'76':fbits(rng.randrange(-16,17)/4),'80':fbits(rng.randrange(-16,17)/4)})
        add([('Top',0,index%15,index%11),('Pick',0,index%15,index%11,rng.choice((0,0x200,0x20)))],raw)
    return result


def encoded(case):
    """완성 입력을 raw 열로 쓴다. 선택/이벤트 기대값을 생성하지 않는다."""
    return [','.join(map(str,case['globals'])),';'.join(f'{token}={raw.hex()}' for token,raw in zip(TOKENS,case['nodes'],strict=True)),
        ';'.join(','.join(map(str,action)) for action in case['actions']),str(case['binding']),
        ';'.join(f'{kind},{token},{value}:'+('/'.join(','.join(map(str,patch)) for patch in patches) or '-')
            for kind,token,value,patches in case['responses']) or '-',
        ';'.join(','.join(map(str,anchor)) for anchor in case['anchors']) or '-']


class GumpEventsOracle(ProcessOracle):
    """새 이벤트 함수와 기존 실제 위치/경계/CRT의 몸체만 실행하는 독립 관찰기다."""
    def __init__(self,edition):
        """읽기 전용 PE와 검토한 두 묶음의 실제 불연속 몸체 범위를 준비한다."""
        super().__init__(edition);self.d=SPECS[edition];self.c=COLLECTION[edition]
        self.extra=[ROOT/f'extracted/{group}/{edition}/{name}' for group in ('gumpevents','gumpcollection') for name in ('creation.c','functions.tsv')]
        self.allowed,self.entry_names=[],{}
        # 새 몸체와 현재 위치/경계/CRT의 실제 조각만 실행한다.
        for path in self.extra[1::2]:
            with path.open(encoding='utf-8') as fp:
                # 함수 사이의 미검토 주소는 실행하지 않는다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entry_names[int(row['entry'],16)]=row['entry']
                    # 불연속 함수의 마지막 명령까지 포함한다.
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x10000);self.returns,self.boundaries=collections.Counter(),collections.Counter()

    def nodes(self):
        """전체 현재 기본 노드 바이트를 주소 순서로 기록한다."""
        return ';'.join(f'{token}={bytes(self.mu.mem_read(token,SIZE)).hex()}' for token in TOKENS)

    def state(self):
        """루트/dialog/선택 전역의 실제 현재 DWORD를 기록한다."""
        return ','.join(str(self.u32(address)) for address in self.d['globals'])

    def response(self,kind,token,args):
        """경계 직전 인자/전역/전체 노드를 기록하고 입력으로 지정한 DWORD만 바꾼다."""
        self.calls.append(','.join(map(str,(kind,token,*args)))+'@'+self.state()+'@'+self.nodes());self.boundaries[kind]+=1
        value,patches=self.responses.get((kind,token),(0,()))
        # 응답은 결과를 계산하지 않고 명시 offset과 값을 그대로 적용한다.
        for target,offset,number in patches:
            self.put(self.d['globals'][offset] if target==0 else target+offset,number&0xffffffff)
        return value

    def on_instruction(self,mu,address,size,data):
        """가상 필터/처리기와 월드 투영만 응답하며 선택/좌표/필터 계산은 실제 PE 명령이다."""
        esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        if address in VIRTUAL:
            value=self.response(VIRTUAL[address],ecx,tuple(self.u32(esp+4+i*4) for i in range(3)))
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+16);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if address==self.c['project']:
            pointer,sid,unused=struct.unpack('<3I',mu.mem_read(esp+4,12));x,y=self.anchors[sid]
            self.response('Project',sid,());mu.mem_write(ecx,struct.pack('<2i',x,y));mu.mem_write(pointer,struct.pack('<2i',x,y))
            mu.reg_write(UC_X86_REG_EAX,ecx);mu.reg_write(UC_X86_REG_ESP,esp+16);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(low<=address and address+size<=high for low,high in self.allowed): raise RuntimeError(f'Gump 사건 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """연속 호출의 정상 반환/ABI/x87/SEH/쓰기/읽기 보호/전체 현재 상태를 관찰한다."""
        mu,d=self.mu,self.d;self.calls,self.results=[],[];self.entries,self.row_returns=collections.Counter(),collections.Counter()
        self.responses={(kind,token):(value,patches) for kind,token,value,patches in case['responses']};self.anchors={sid:(x,y) for sid,x,y in case['anchors']}
        mu.mem_write(DATA,b'\xa5'*4096)
        # 전체 노드와 SID의 실제 원본 필드는 완성 입력이다.
        for index,(token,raw) in enumerate(zip(TOKENS,case['nodes'],strict=True)):
            mu.mem_write(token,raw);self.put(POOL+index*self.c['stride']+14,index);self.put(POOL+index*self.c['stride']+18,0)
        self.put(self.c['pool'],POOL)
        # 루트/dialog/선택 초기값은 DWORD 비트 그대로 공급한다.
        for address,value in zip(d['globals'],case['globals'],strict=True): self.put(address,value)
        self.put(VTABLE+0x28,STUBS+0xb20);self.put(VTABLE+0x34,d['Base'] if case['binding'] else STUBS+0xb00)
        before_pool=bytes(mu.mem_read(POOL,COUNT*self.c['stride']));before_vtable=bytes(mu.mem_read(VTABLE,64))
        self.writes=[(0,4),(STACK-0xf000,STACK+4096)]+[(token+108,token+116) for token in TOKENS]+[(address,address+4) for address in d['globals']]
        # 호출 프레임은 새로 준비하고 그 전 호출의 노드/선택 상태는 유지한다.
        for action in case['actions']:
            op,token,*args=action
            # 정상 반환에서 실제 보존 레지스터를 비교한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            # 임시 레지스터/this/스택/x87/SEH만 독립 초기화한다.
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX): mu.reg_write(reg,0)
            mu.reg_write(UC_X86_REG_ECX,token);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
            mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*(v&0xffffffff for v in args)))
            mu.emu_start(d[op],STOP,timeout=30000000,count=2000000)
            purge=0 if op in ('Top','Pick') else len(args)*4
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge or any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()):
                raise RuntimeError(f'Gump 사건 정상 반환/ABI 오류: {self.binary} {op}')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Gump 사건 x87/SEH 오류')
            self.results.append(str(mu.reg_read(UC_X86_REG_EAX)));self.row_returns[f'{d[op]:08x}']+=1;self.returns[f'{d[op]:08x}']+=1
        protected=bytearray(mu.mem_read(DATA,4096))
        # 전체 노드 사이/전후 보호 영역은 바뀌지 않아야 한다.
        for token in TOKENS: protected[token-DATA:token-DATA+SIZE]=b'\xa5'*SIZE
        if protected!=b'\xa5'*4096 or before_pool!=bytes(mu.mem_read(POOL,COUNT*self.c['stride'])) or before_vtable!=bytes(mu.mem_read(VTABLE,64)):
            raise RuntimeError('Gump 사건 보호/읽기 전용 바이트 변경')
        return [*encoded(case),self.nodes(),self.state(),','.join(self.results),'|'.join(self.calls) or '-',
            json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]


def generate(smoke):
    """두 x87의 실제 관찰이 같을 때만 저장한다. 기대 UI 선택/전달 결과를 계산하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumpevents-functions.json'}
    files.update(ROOT/'tools'/name for name in ('decomp_gumpevents_oracle.py','decomp_process_oracle.py','decomp_drawsort_oracle.py','decomp_gumpcollection_oracle.py'))
    # 세 판본을 독립 PE/메모리에서 실제 정상 반환까지 실행한다.
    for edition in BINARIES:
        oracle=GumpEventsOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::29]+indexed[-6:]
        # 두 정밀도의 전체 raw 상태/호출/반환을 대조한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump 사건 x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# Gump 커서 선택/기본 필터/자식 우선 전달/부모 전파의 실제 전체 몸체 관찰.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,
        editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},
        limits=['선택/기본 필터/자식 우선 전달/부모 전파의 전체 정상 몸체와 실제 위치/경계/CRT',
            '전체 0xa0바이트 여섯 노드·일곱 전역·반환·콜백 직전 전체 상태/인자를 실제 관찰',
            '가상 필터/처리기·월드 투영만 명시 입력 경계; 실제 기본 필터 가상 바인딩 포함',
            '파생 버튼 입력/최상위 UI 프레임·사건 큐/OS·손상 링크/예외 전파 제외',
            '정상 반환/보존 레지스터/스택/x87/SEH/허용 실행·쓰기/보호·읽기 전용/SHA/호출 집계 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/완성 입력/전체 바이트 폭/실제 호출·정상 반환 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 도구/원본/내보내기/fixture의 정확한 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump 사건 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES):
        raise RuntimeError('Gump 사건 총계 오류')
    # 저장 관찰로만 진입/반환/응답을 집계한다. 기대 선택/필터 결과는 재계산하지 않는다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Gump 사건 판본 행 수 오류')
        # 순서/입력/전체 여섯 노드/일곱 전역/최상위 정상 반환을 대조한다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=14 or row[1]!=str(index) or row[2:8]!=encoded(case): raise RuntimeError('Gump 사건 입력/폭 오류')
            if len(row[8].split(';'))!=COUNT or any(len(bytes.fromhex(part.split('=')[1]))!=SIZE for part in row[8].split(';')) or len(row[9].split(','))!=7:
                raise RuntimeError('Gump 사건 전체 상태 폭 오류')
            if len(row[10].split(','))!=len(case['actions']) or dict(collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions']))!=json.loads(row[13]):
                raise RuntimeError('Gump 사건 정상 반환 누락')
            # 두 모드 집계는 저장 관찰의 정확히 두 배다.
            for address,count in json.loads(row[12]).items(): entries[address]+=count*2
            for address,count in json.loads(row[13]).items(): returns[address]+=count*2
            if row[11]!='-':
                # 경계 종류는 실제 호출 기록의 첫 칸에서만 읽는다.
                for call in row[11].split('|'): boundaries[call.split(',')[0]]+=2
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump 사건 호출 집계 오류')
    print(f'gumpevents 감사 통과: {len(rows)}개, 두 정밀도 정상 반환 {sum(sum(v["returns"].values()) for v in report["editions"].values())}회, OS/assert 0')


def main():
    """전체 기록/무저장 표본/저장 근거 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__': main()
