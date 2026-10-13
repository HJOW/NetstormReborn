#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 PE의 MenuGump 크기/확장/controlled 스택을 격리 x86에서 관찰한다. 게임/OS는 실행하지 않는다."""
import argparse
import collections
import csv
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_process_oracle import (ROOT, pefile, Uc, UC_ARCH_X86, UC_MODE_32,
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)
from decomp_drawsort_oracle import REGISTERS

# 실제 두 판본의 함수/표/기본 Redraw 경계 주소다. 생성자와 레이아웃/입력은 실행 범위에 포함하지 않는다.
SPECS = {
    'originals': dict(Width=0x475f20, Height=0x475fb0, Redraw=0x476050, Item=0x476630,
        Idle=0x476700, Expand=0x4767d0, Take=0x475c00, Current=0x475c40, Count=0x475c50,
        Attach=0x475ca0, Object=0x475cf0, stack=0x592fd0, index=0x593020,
        idle=0x5634d0, idle_size=500, root=0x563e38, base_redraw=0x464a50),
    'originalCD': dict(Width=0x452260, Height=0x4522f0, Redraw=0x452380, Item=0x4528d0,
        Idle=0x452a70, Expand=0x452b00, Take=0x451070, Current=0x4510b0, Count=0x4510c0,
        Attach=0x451310, Object=0x451360, stack=0x51fc28, index=0x51fc78,
        idle=0x56b5b8, idle_size=80, root=0x56b748, base_redraw=0x4937e0),
}
# 메뉴/fancy/title의 안정된 가상 주소와 원본 객체 크기 및 의미별 snapshot offset이다.
DATA, SIZE, VTABLE, RESULT = 0x18000000, 0xb90, 0x18009000, 0x18008500
TOKENS = (DATA+16, DATA+0x1010, DATA+0x2010)
FIELDS = (0,4,56,60,64,68,72,184,368,372,376,*range(380,540,4),544,548,2952,2956)
# 가상 호출 경계는 명시 입력 응답만 수행한다. 수학/CRT/SetIdle/스택 몸체는 실제 PE를 실행한다.
STUBS, STOP, STACK = 0x13000000, 0x20000000, 0x3000f000
VIRTUAL = {STUBS+0x100:('push',0), STUBS+0x120:('pop',0), STUBS+0x140:('redraw',1),
    STUBS+0x160:('attach',4), STUBS+0x180:('object',5)}
FIXTURE = ROOT/'cpppj/tests/fixtures/menugump-x86.tsv'
REPORT = ROOT/'cpppj/menugump.json'


def bits(value):
    """입력 float32를 원본 DWORD 비트로 바꾼다. 기대 산술 결과는 만들지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def raw_nodes(fancy=False,scale=1.0):
    """미사용 바이트를 임의 패턴으로 채우고 실제 접근 입력 필드만 작성한다."""
    result=[]
    # 세 객체는 메뉴/장식/제목으로 쓰지만 같은 전체 원본 크기를 가진 격리 저장소다.
    for i in range(3):
        raw=bytearray((n*31+i*19+11)&255 for n in range(SIZE))
        values={0:VTABLE,4:1,56:bits(scale if i==0 else 1.5),60:bits(75),64:bits(19.75),
            68:bits(75),72:bits(19.75),184:1,368:bits(3.5),372:bits(100),376:bits(80),
            544:0x800000 if fancy else 0,548:17,2952:TOKENS[1] if fancy else 0,2956:TOKENS[2]}
        # 항목 offset은 기대 배치가 아닌 원본 함수가 읽는 독립 입력 배열이다.
        for j in range(40): values[380+j*4]=bits(j*4.25-2.5)
        for offset,value in values.items(): struct.pack_into('<I',raw,offset,value)
        result.append(bytes(raw))
    return result


def change(raw,offset,value):
    """완성 입력의 지정 DWORD만 수정한다."""
    out=bytearray(raw);struct.pack_into('<I',out,offset,value&0xffffffff);return bytes(out)


def nodes_text(nodes):
    """현재 선택 필드를 비트 그대로 기록한다. 미선택 바이트는 별도 쓰기 보호로 확인한다."""
    return ';'.join(b''.join(raw[offset:offset+4] for offset in FIELDS).hex() for raw in nodes)


def state_text(index,entries,idle,root):
    """현재 스택/앞 네 idle 칸/루트를 동일한 입력 및 출력 형식으로 기록한다."""
    return '#'.join((str(index),','.join(map(str,entries)),','.join(map(str,idle)),str(root)))


def plan():
    """산술 경계/두 메뉴 방향/콜백의 현재 필드 재조회/스택 변경 입력을 만든다."""
    cases=[]
    def add(actions,fancy=False,scale=1.0,patches=(),index=2):
        """예상 호출이나 예상 상태 없이 입력과 콜백 응답만 넣는다."""
        case=dict(actions=actions,nodes=raw_nodes(fancy,scale),patches=patches,index=index,
            entries=[0]+[TOKENS[0] for _ in range(19)],idle=[0]*4,root=TOKENS[2]);cases.append(case);return case
    # float32 저장과 배율 곱에 큰 폭/음수/0/소수/2의 24승 경계를 독립 조합한다.
    for op in ('Width','Height'):
        for size in (-100.25,-0.0,0.0,0.5,19.75,16777215.0,3e20):
            for scale in (0.5,1.0,1.5):
                for fancy in (False,True): add([(op,bits(size))],fancy,scale)
    # redraw는 menuFlags와 무관하게 실제 fancy 포인터를 검사한다.
    for fancy in (False,True):
        for erase in (0,1): add([('Redraw',erase)],fancy)
        c=add([('Redraw',1)],fancy);c['nodes'][0]=change(c['nodes'][0],544,0 if fancy else 0x800000)
    # itemCount와 별개인 실제 index 0/1/39와 가로 메뉴의 큰/음수 정수 곱이다.
    for horizontal in (False,True):
        for index in ((0,1,39,-3,16777217) if horizontal else (0,1,39)):
            c=add([('Item',index&0xffffffff)]);c['nodes'][0]=change(c['nodes'][0],544,0x40000 if horizontal else 0)
    # 서로 다른 완료/확장 상태와 NaN 비교의 첫 unordered 분기를 관찰한다.
    for fancy in (False,True):
        c=add([('Idle',)],fancy);c['nodes'][0]=change(c['nodes'][0],184,0)
        for width,height in ((20,10),(100,80),(110,90),(100,1),(1,80),(float('nan'),80)):
            c=add([('Idle',),('Idle',)],fancy);c['nodes'][0]=change(change(c['nodes'][0],68,bits(width)),72,bits(height))
        for flag in (0,0x80000): add([('Expand',flag),('Idle',),('Idle',)],fancy)
    # 현재 메뉴 관리의 첫/중간/마지막 칸을 각각 독립 실행한다.
    for index in (1,2,19):
        for op in ('Take','Current','Count'): add([(op,)],index=index)
        for x,y in ((-21,16),(2147483647,-2147483648),(16777217,-16777217)):
            for op in ('Attach','Object'):
                args=(x&0xffffffff,y&0xffffffff,TOKENS[2]) if op=='Attach' else (123,x&0xffffffff,y&0xffffffff)
                add([(op,*args)],index=index)
    # Push 이후 현재 폭/제목/장식 token 재조회와 확장 방향/크기 변경을 관찰한다.
    for op in ('Width','Height'):
        for offset,value in ((60,bits(16777215)),(64,bits(33.25)),(2952,TOKENS[2]),(2956,TOKENS[1])):
            add([(op,bits(7.5))],True,patches=(('push',TOKENS[1],TOKENS[0],offset,value),))
    add([('Idle',)],patches=(('push',TOKENS[0],TOKENS[0],372,bits(9)),('push',TOKENS[0],TOKENS[0],376,bits(7))))
    for op in ('Attach','Object'):
        args=(5,7,TOKENS[2]) if op=='Attach' else (321,5,7)
        add([(op,*args)],patches=((op.lower(),TOKENS[0],0,0,3),))
    return cases


class Oracle:
    """실제 PE와 읽기 전용 내보내기 범위만 실행하며 OS/assert 진입은 거부한다."""
    def __init__(self,edition):
        """선택 판본의 PE/격리 메모리/내보낸 함수 범위와 관찰 훅을 초기화한다."""
        self.edition=edition;self.d=SPECS[edition];self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        path=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        pe=pefile.PE(str(path));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
        self.mu.mem_map(base,(len(image)+4095)&~4095);self.mu.mem_write(base,image)
        # 격리 데이터/경계/정상 반환/스택만 실제 PE 바깥에 매핑한다.
        for low,size in ((DATA,0x10000),(STUBS,0x1000),(STOP,0x1000),(STACK-0xf000,0x10000)):
            self.mu.mem_map(low,size)
        self.allowed=[];self.function_entries=set();self.entries=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        table=ROOT/f'extracted/menugump/{edition}/functions.tsv'
        # 불연속 몸체까지 내보내기 범위에 적힌 명령만 허용한다.
        for row in csv.DictReader(table.open(encoding='utf-8'),delimiter='\t'):
            self.function_entries.add(int(row['entry'],16))
            for part in row['ranges'].split(';'):
                low,high=part.split('-');self.allowed.append((int(low,16),int(high,16)+1))
        self.mu.hook_add(UC_HOOK_CODE,self.code);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
    def u32(self,address):
        """현재 x86 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]
    def put(self,address,value):
        """외부 입력/응답 DWORD를 쓴다. 기대 결과 계산은 수행하지 않는다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def nodes(self):
        """실제 현재 세 저장소를 읽는다."""
        return nodes_text([bytes(self.mu.mem_read(token,SIZE)) for token in TOKENS])
    def state(self):
        """콜백 직전과 정상 반환 뒤 현재 표를 읽는다."""
        return state_text(self.u32(self.d['index']),struct.unpack('<20I',self.mu.mem_read(self.d['stack'],80)),
            struct.unpack('<4I',self.mu.mem_read(self.d['idle'],16)),self.u32(self.d['root']))
    def write(self,mu,access,address,size,value,data):
        """선택 필드/idle 표/스택 표/실행 스택 이외의 명령 쓰기는 즉시 거부한다."""
        if not any(lo<=address and address+size<=hi for lo,hi in self.writes):
            raise RuntimeError(f'허용 밖 MenuGump 쓰기 {address:x}/{size}')
    def code(self,mu,address,size,data):
        """명시 경계에서 실제 인자/전체 상태를 기록하고 준비된 응답 쓰기만 수행한다."""
        boundary=VIRTUAL.get(address)
        if address==self.d['base_redraw']: boundary=('base_redraw',1)
        if boundary:
            kind,count=boundary;esp=mu.reg_read(UC_X86_REG_ESP);token=mu.reg_read(UC_X86_REG_ECX)
            args=[self.u32(esp+4+i*4) for i in range(count)]
            self.calls.append(','.join(map(str,(kind,token,*args)))+'@'+self.nodes()+'@'+self.state());self.boundaries[kind]+=1
            # 입력은 콜백 종류/대상에 명시한 DWORD만 변경한다. 어떤 콜백이 호출될지는 계산하지 않는다.
            for k,t,target,offset,value in self.patches:
                if (k,t)==(kind,token): self.put(self.d['index'] if target==0 else target+offset,value)
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4+count*4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):
            raise RuntimeError(f'허용 밖 MenuGump 실행 {self.edition} {address:x}')
        if address in self.function_entries: self.entries[f'{address:08x}']+=1
    def run(self,case,control):
        """각 action의 정상 반환/ABI/x87/필드 보호와 독립 관찰을 수집한다."""
        mu=self.mu;self.calls=[];self.patches=case['patches'];results=[]
        mu.mem_write(DATA,b'\xa5'*0x10000)
        for token,raw in zip(TOKENS,case['nodes']): mu.mem_write(token,raw)
        for slot,address in ((8,STUBS+0x140),(0x18,STUBS+0x100),(0x1c,STUBS+0x120),(0x20,STUBS+0x160),(0x3c,STUBS+0x180)):
            self.put(VTABLE+slot,address)
        readonly=bytes(mu.mem_read(VTABLE,64))
        self.put(self.d['index'],case['index']);self.put(self.d['root'],case['root'])
        mu.mem_write(self.d['stack'],struct.pack('<20I',*case['entries']))
        mu.mem_write(self.d['idle'],struct.pack('<'+'I'*self.d['idle_size'],*(case['idle']+[0]*(self.d['idle_size']-4))))
        self.writes=[(STACK-0xf000,STACK+0x1000),(self.d['stack'],self.d['stack']+80),
            (self.d['index'],self.d['index']+4),(self.d['idle'],self.d['idle']+self.d['idle_size']*4),(RESULT,RESULT+8)]
        self.writes += [(token+offset,token+offset+4) for token in TOKENS for offset in FIELDS]
        # 연속 action은 앞 action의 실제 상태를 보존한다.
        for action in case['actions']:
            op,*args=action
            if op=='Item': args=[RESULT,*args]
            static=op in ('Take','Current','Count','Attach','Object')
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            mu.reg_write(UC_X86_REG_ECX,0 if static else TOKENS[0]);mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0)
            mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(self.d[op],STOP,count=500000)
            expected=STACK+4+(0 if static else len(args)*4)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=expected or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()):
                raise RuntimeError(f'MenuGump ABI/정상 반환 오류 {op}')
            if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800: raise RuntimeError('MenuGump x87 오류')
            results.append(str(mu.reg_read(UC_X86_REG_EAX)) if op in ('Take','Current','Count') else
                ','.join(map(str,struct.unpack('<2I',mu.mem_read(RESULT,8)))) if op=='Item' else '-')
            self.returns+=1
        # 선택 필드 이외의 모든 원본 객체 바이트와 읽기 전용 가상 표를 보호한다.
        for token,original in zip(TOKENS,case['nodes']):
            current=bytearray(mu.mem_read(token,SIZE))
            for offset in FIELDS: current[offset:offset+4]=original[offset:offset+4]
            if bytes(current)!=original: raise RuntimeError('MenuGump 미선택 필드 변경')
        if bytes(mu.mem_read(VTABLE,64))!=readonly or any(mu.mem_read(self.d['idle']+16,(self.d['idle_size']-4)*4)):
            raise RuntimeError('MenuGump 가상 표/idle 뒷칸 변경')
        return [self.nodes(),self.state(),'|'.join(self.calls) or '-', ';'.join(results)]


def digest(path):
    """독립 관찰의 원본/도구/출력 바이트 해시를 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """fixture 생성 또는 저장 해시/행 수 감사를 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,sha in report['files'].items():
            if digest(ROOT/path)!=sha: raise RuntimeError('MenuGump 해시 변경: '+path)
        rows=list(csv.DictReader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))
        if len(rows)!=report['rows']: raise RuntimeError('MenuGump fixture 행 수 변경')
        print(f'MenuGump 감사 통과: {len(rows)}행');return
    rows=[];stats={};files=[Path(__file__),ROOT/'tools/ghidra/menugump-functions.json',
        ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py']
    for edition in SPECS:
        oracle=Oracle(edition);cases=plan()
        for index,case in enumerate(cases):
            before=[nodes_text(case['nodes']),state_text(case['index'],case['entries'],case['idle'],case['root'])]
            out=oracle.run(case,0x027f);other=oracle.run(case,0x037f)
            if out!=other: raise RuntimeError(f'MenuGump x87 모드 불일치 {edition}/{index}')
            rows.append([edition,str(index),';'.join(','.join(map(str,a)) for a in case['actions']),*before,
                ';'.join(','.join(map(str,p)) for p in case['patches']) or '-',*out])
        stats[edition]=dict(cases=len(cases),normal_returns=oracle.returns,entries=dict(oracle.entries),boundaries=dict(oracle.boundaries))
        files += [ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'),
            ROOT/f'extracted/menugump/{edition}/creation.c',ROOT/f'extracted/menugump/{edition}/functions.tsv']
    with FIXTURE.open('w',encoding='utf-8',newline='') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n');writer.writerow(['edition','case','actions','nodes','state','patches','out_nodes','out_state','calls','results']);writer.writerows(rows)
    files.append(FIXTURE)
    report=dict(last_decompile_host=socket.gethostname(),rows=len(rows),editions=stats,
        method='실제 PE 격리 x86; 가상 Push/Pop/Attach/객체 Attach/Redraw 및 기본 Redraw만 명시 경계',
        files={str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in files})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'MenuGump 관찰 완료: {len(rows)}행, 정상 반환 {sum(s["normal_returns"] for s in stats.values())}회')


if __name__=='__main__': main()
