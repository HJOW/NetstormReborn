#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 CoreData 선택0048f3b0/조회0048f0d0의 실제 PE를 신규 제품 작성 전에 고정한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

# 조회와 선택의 실제 몸체만 허용하며 불법 owner 주소를 host span으로 가장하지 않는다.
RANGES = ((0x48f0d0,0x48f0ff),(0x48f3b0,0x48f42e))
CURRENT, CACHED, ACTIVE, KNOWN = 0x595350,0x5953f0,0x595348,0x595344
BATTLE, TEMPLATE, METAMAP, PLAYER_BASE = 0x540c70,0x540cac,0x594fc8,0x59534c
EXTERNAL = POOL+0x1000  # 부분 읽기/외부 실제 CoreData를 구별하는 명시 물리다.
FIXTURE = ROOT/'cpppj/tests/fixtures/player-core-data-x86.tsv'
REPORT = ROOT/'extracted/cpp-player-core-data-20261024-native.json'


def cases():
    """기대를 계산하지 않고 유효 Player alias·정수 현재값·진단/물리 중단 입력만 선택한다."""
    rows=[]
    for seed,kind,metamap,known,active in itertools.product((0,165),range(5),(0,1,0xffffffff),(0,7),(0,9)):
        rows.append((0,seed,kind,10,metamap,known,active,0,1,'none'))
    for seed,kind,owner,template,metamap in itertools.product((0,165),range(5),(0,1,8),(0,1,7,0xffffffff),(0,1)):
        rows.append((1,seed,kind,10,metamap,7,9,owner,template,'none'))
    for seed,length,metamap in itertools.product((0,165),(0,1,2,3,4,5,7,8,9,10),(0,1)):
        rows.append((1,seed,4,length,metamap,7,9,0,1,'none'))
    for mutation in ('current0','currentPlayer8','currentCache','metamap0','active0','stop'):
        rows.append((0,165,1,10,1,7,9,0,1,mutation))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """두 몸체의 실제 BYTE 효과를 기록하고 원문 assertion만 명시 장치로 처리한다."""
    def __init__(self):
        """독립 PE/스택/Player를 만들며 원본 파일이나 실제 OS를 실행하지 않는다."""
        super().__init__();self.running=False;self.status='normal';self.total=0
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def normalized(self,address):
        """같은 실제 가상 포인터를 NULL/캐시/Player/외부 주소로 구별한다."""
        if address==0:return '0'
        if address==CACHED:return 'CACHED'
        if address==EXTERNAL:return 'EXTERNAL'
        for owner in range(9):  # Player+6c의 실제 주소를 번호와 일대일 대응한다.
            if address==PLAYERS+owner*0xb4+0x6c:return 'PLAYER'+str(owner)
        raise RuntimeError('CoreData 미해석 포인터:'+hex(address))

    def read(self,mu,access,address,size,value,data):
        """짧은 현재 CoreData의 DWORD/DWORD/WORD 부분 읽기 실패만 관측한다."""
        if self.running and EXTERNAL<=address<EXTERNAL+32 and address+size>EXTERNAL+self.case[3]:
            self.status='fault';mu.emu_stop()

    def write(self,mu,access,address,size,value,data):
        """원문 현재 전역/캐시10BYTE/스택 외 예기치 않은 CPU 쓰기를 거부한다."""
        if not self.running:return
        ranges=((STACK-8192,STACK+64),(CURRENT,CURRENT+4),(ACTIVE,ACTIVE+4),(KNOWN,KNOWN+4),(BATTLE,BATTLE+4),(TEMPLATE,TEMPLATE+4),(CACHED,CACHED+10))
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError('CoreData 예상밖 쓰기:'+hex(address))

    def instruction(self,mu,address,size,data):
        """원문 내부는 CPU로 수행하고 유일한 assertion의 인자/현재값 변화만 기록한다."""
        self.instructions+=1;self.total+=1
        if any(low<=address<high for low,high in RANGES):return
        if address!=0x4e0620:raise RuntimeError('CoreData 관측밖 명령:'+hex(address))
        assertion,file,line=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)
        self.trace.append('A:'+str(line)+':'+self.string(assertion).decode('ascii'))
        mutation=self.case[-1]
        if mutation=='current0':self.put(CURRENT,0)
        elif mutation=='currentPlayer8':self.put(CURRENT,PLAYERS+8*0xb4+0x6c)
        elif mutation=='currentCache':self.put(CURRENT,CACHED)
        elif mutation=='metamap0':self.put(METAMAP,0)
        elif mutation=='active0':self.put(ACTIVE,0)
        elif mutation=='stop':self.status='stop';mu.emu_stop();return
        elif mutation!='none':raise RuntimeError('CoreData 미해석 변경')
        self.ret()

    def sample(self,case,control):
        """모든 입력/물리와 반환주소·부분 효과를 CPU 명령 실행 결과로 고정한다."""
        self.case=case;self.status='normal';self.instructions=0;self.trace=[];self.running=False
        mode,seed,kind,length,metamap,known,active,owner,template,mutation=case
        pointers=(0,PLAYERS+0xb4+0x6c,PLAYERS+8*0xb4+0x6c,CACHED,EXTERNAL)
        self.put(CURRENT,pointers[kind]);self.put(METAMAP,metamap);self.put(KNOWN,known);self.put(ACTIVE,active);self.put(BATTLE,55);self.put(TEMPLATE,66);self.put(PLAYER_BASE,PLAYERS)
        self.mu.mem_write(PLAYERS,bytes((seed+i*17)&255 for i in range(9*0xb4)))
        self.mu.mem_write(CACHED,bytes((seed+i*11+23)&255 for i in range(10)))
        self.mu.mem_write(EXTERNAL,bytes((seed+i*7+49)&255 for i in range(16)))
        self.mu.mem_write(STACK-8192,bytes([seed])*(8192+64))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        arguments=() if mode==0 else (owner,template)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(arguments)),STOP,*arguments));self.running=True
        self.mu.emu_start(0x48f0d0 if mode==0 else 0x48f3b0,STOP);self.running=False
        if self.status=='normal' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('CoreData 원문 스택 복구 차이')
        result=self.normalized(self.mu.reg_read(UC_X86_REG_EAX)) if mode==0 and self.status=='normal' else '-'
        state=(self.words(TEMPLATE,1)[0],self.words(BATTLE,1)[0],self.words(KNOWN,1)[0],self.words(ACTIVE,1)[0],self.words(METAMAP,1)[0])
        return '\t'.join((str(control),','.join(map(str,case)),self.status,result,','.join(map(str,state)),self.normalized(self.words(CURRENT,1)[0]),bytes(self.mu.mem_read(CACHED,10)).hex(),bytes(self.mu.mem_read(PLAYERS,9*0xb4)).hex(),bytes(self.mu.mem_read(EXTERNAL,16)).hex(),';'.join(self.trace) or '-',str(self.instructions)))


def main():
    """제품보다 먼저 최초 원문을 쓰고 재호출은 고정 SHA 검증만 허용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative,expected in record['sha256'].items():assert digest(ROOT/relative)==expected,relative
        print('Player CoreData 최초 SHA 확인',record['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('CoreData 최초 자료 덮어쓰기 금지')
    for relative in ('cpppj/src/o/RawPlayerCoreData.h','cpppj/src/o/RawPlayerCoreData.cpp','cpppj/tests/PlayerCoreDataTests.cpp'):
        if (ROOT/relative).exists():raise RuntimeError('CoreData 제품 작성 전 관측 요건 위반')
    oracle=Oracle();inputs=cases();lines=['# control / mode,seed,currentKind,sourceLength,metamap,known,active,ownerArg,templateArg,mutation / status / returnPointer / template,battle,known,active,metamap / currentPointer / cached10 / players1620 / external16 / trace / instructions'];statuses=Counter();total=0
    for case in inputs:  # 반환/메모리를 제품이나 정적 기대에서 생성하지 않는다.
        for control in (0x27f,0x37f):
            if oracle.status!='normal':oracle=Oracle()
            line=oracle.sample(case,control);lines.append(line);statuses[oracle.status]+=1;total+=oracle.instructions
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','tools/decomp_player_core_data.py','cpppj/tests/fixtures/player-core-data-x86.tsv')
    record={'scope':'실제 PE 두 몸체 최초 격리 관측/유효 Player1..8 선택과 현재 물리 부분 읽기', 'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','ranges':RANGES,'uniqueInputs':len(inputs),'observations':len(lines)-1,'statuses':dict(statuses),'instructions':total,'sourcesAbsentBeforeObservation':True,'invalidNonzeroPlayerSelectionOutsidePhysicalContract':True,'sha256':{relative:digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','statuses','instructions')},ensure_ascii=False))


if __name__=='__main__':main()
