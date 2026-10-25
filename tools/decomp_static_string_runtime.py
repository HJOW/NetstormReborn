#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 임시 문자열 초기화/예약/서식 외곽만 Unicorn에서 실행한다. 게임/OS는 시작하지 않는다."""
import itertools,json,struct,zlib
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,TEXT,STACK,STOP,digest
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP

ARENAS=(POOL+0x10000,POOL+0x20000)  # 두 명시 할당의 실제8192BYTE 물리를 따로 보존한다.
GLOBAL=0x5c8940  # size/base/current/last의 연속 네 실제 DWORD다.
FIXTURE=ROOT/'cpppj/tests/fixtures/static-string-runtime-x86.tsv'  # 제품 작성 전에 고정하는 원문 관측이다.
REPORT=ROOT/'extracted/cpp-editor-player-ui-421-static-string-native.json'  # 실제 함수와 명시 CRT 경계를 구별한다.


def cases():
    """기대 결과 없이 현재 물리/크기/위치/반복과 외부 중단만 입력한다."""
    values=[]
    for size,base,seed in itertools.product((1,2,3,4,5,31,32,8192),(0,1),(0,165)):  # 모든 초기화 나머지BYTE/재시작 진단을 교차한다.
        values.append((0,size,base,1,7,seed,1,0,'none'))
    for capacity,cursor,length,repeat,seed in itertools.product((8,32),(1,2,6),(0,1,3,6),(1,3),(0,165)):  # end와 같은 unsigned 경계 및 반복 wrap을 실제 CPU가 판정한다.
        values.append((1,capacity,1,cursor,length,seed,repeat,0,'none'))
    for length in (0,30,8190,0xffffffff):  # 최초 lazy8192와32비트 음수 예약을 명시한다.
        values.append((1,8192,0,1,length,165,1,0,'none'))
    for capacity,cursor,pattern,length in itertools.product((8,32),(1,2,6),(0,1,2),(0,1,3)):  # pattern0은 외부주소,1/2는 같은 고리의 실제 alias다.
        values.append((2,capacity,1,cursor,length,165,1,pattern,'none'))
    for mode,change in itertools.product((0,1,2),('assert-stop','new-stop','new-null','new-size4','format-stop','format-cursor3')):  # 선택하지 않은 경계의 변경은 실행하지 않는다.
        values.append((mode,32,0 if mode==1 else 1,6,3,165,1,0,change))
    return values


class Oracle(MemoryOracle):
    """게임 계산은 세 원본 함수만 실행하고 할당/진단/CRT 서식 장치는 관측한다."""
    def __init__(self):
        """기존 읽기 전용 PE/스택 장치를 빌리고 검사한 함수 범위만 허용한다."""
        super().__init__();self.ranges=((0x4b92b0,0x4b9318),(0x4b9320,0x4b9384),(0x4b9390,0x4b93bb));self.total=0

    def normalized(self,address):
        """host 주소가 아닌 두 원문 입력 arena의 offset을 기록한다."""
        if not address:return '0'
        for index,base in enumerate(ARENAS):  # 실제 allocation과 고리 offset을 구별한다.
            if base<=address<base+8192:return f'{index+1}+{address-base}'
        return hex(address)

    def instruction(self,mu,address,size,data):
        """외곽의 직접쓰기/현재값 재조회는 실제 기계어가 실행한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>100000:raise RuntimeError('임시 문자열 원문 명령 상한')
        if any(low<=address<high for low,high in self.ranges):return
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,3);change=self.case[-1]
        if address==0x4e0620:
            self.trace.append('A:'+str(args[2]))
            if change=='assert-stop':self.status='stop';mu.emu_stop()
            else:self.ret()
        elif address==0x4e4391:
            self.trace.append('N:'+str(args[0]));self.allocations+=1
            if change=='new-stop':self.status='stop';mu.emu_stop();return
            if change=='new-size4':self.put(GLOBAL,4)
            self.ret(0 if change=='new-null' else ARENAS[1])
        elif address==0x4de490:
            value=self.words(args[2],1)[0];self.trace.append('F:'+self.normalized(args[0])+':'+self.normalized(args[1])+':'+str(value))
            if change=='format-stop':self.status='stop';mu.emu_stop();return
            mu.mem_write(args[0],b'R\0')  # 이 formatter는 명시 CRT 장치 입력이며 원문 printf 본체 검증으로 세지 않는다.
            if change=='format-cursor3':self.put(GLOBAL+8,ARENAS[0]+3)
            self.ret(1)
        else:raise RuntimeError(f'임시 문자열 관측 밖 명령 {address:08x}')

    def sample(self,case):
        """같은 실제 전역과 전체 두 arena의 원문 결과/순서를 수집한다."""
        self.case=case;mode,capacity,base,cursor,length,seed,repeat,pattern,change=case;self.status='normal';self.trace=[];self.instructions=0;self.allocations=0;result=0
        for index,arena in enumerate(ARENAS):  # 미작성 bytes도 서로 다른 명시 입력이다.
            self.mu.mem_write(arena,bytes((seed+index*71+offset*13)&255 for offset in range(8192)))
        self.put(GLOBAL,capacity);self.put(GLOBAL+4,ARENAS[0] if base else 0);self.put(GLOBAL+8,ARENAS[0]+cursor if base else 0);self.put(GLOBAL+12,ARENAS[0]+7 if base else 0)
        source=TEXT if not pattern else ARENAS[0]+pattern;self.mu.mem_write(TEXT,b'%d\0')
        for invocation in range(repeat):  # 초기화/예약의 현재 전역은 호출 사이 그대로 유지한다.
            for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 레지스터 입력만 다시 고정한다.
            self.mu.reg_write(UC_X86_REG_ESP,STACK);args=(capacity,) if mode==0 else (length,) if mode==1 else (length,source,7)
            self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args))
            try:self.mu.emu_start((0x4b92b0,0x4b9320,0x4b9390)[mode],STOP)
            except UcError:
                if change=='new-null':self.status='fault'
                else:raise
            if self.status!='normal':break
            assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
            result=self.mu.reg_read(UC_X86_REG_EAX)
        words=self.words(GLOBAL,4);buffers=[bytes(self.mu.mem_read(arena,8192)).hex() for arena in ARENAS]
        return '\t'.join((','.join(map(str,case)),self.status,','.join(map(str,words)),str(result) if mode else '-',*buffers,';'.join(self.trace) or '-',str(self.instructions)))


def main():
    """원본 관측을 먼저 고정하고 실행 범위/보호 입력/관측 수를 함께 기록한다."""
    assert not FIXTURE.exists() and not REPORT.exists();oracle=Oracle();rows=[oracle.sample(case) for case in cases()]
    FIXTURE.write_text('# input/status/size,base,current,last/result/arena1/arena2/trace/instructions\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    value=dict(host='HJOW-Athlon',lastFullDecompileDate='2026-10-23',gameExecuted=False,rows=len(rows),instructions=oracle.total,functions=[hex(v) for v in (0x4b92b0,0x4b9320,0x4b9390)],externalBoundaries=['operator new','assertion','explicit CRT format device'],status=dict(Counter(row.split('\t')[1] for row in rows)),hashes={p.relative_to(ROOT).as_posix():digest(p) for p in [ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',Path(__file__),FIXTURE]})
    REPORT.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'rows':len(rows),'instructions':oracle.total,'status':value['status']}))


if __name__=='__main__':
    from pathlib import Path
    main()
