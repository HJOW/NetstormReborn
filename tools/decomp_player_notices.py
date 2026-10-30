#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""프레임말490da0/실제48fc60 및 별도42b210의 원래 분기·기록을 동결한다."""
import argparse
import csv
import hashlib
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TEXT,STACK,STOP,PLAYERS,POOL,TYPES
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS


class ObservationStop(Exception):
    """실제 하위 진입에서 효과 적용 전 부분 관측을 끝낸다."""


class Oracle(MemoryOracle):
    """알림/실제 SID getter와 별도 Core 합계는 CPU, 큰 하위는 명시 응답이다."""
    def __init__(self):
        """현재 PC의 전체 정밀 범위만 허용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream: facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[]
        for entry in (0x490da0,0x48fc60,0x42b210):  #부모와 선택된 실제 자식의 떨어진 범위를 유지한다.
            for part in facts[entry]['ranges'].split(';'):  #검토하지 않은 다음 함수로 새지 않게 한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def prepare(self):
        """각 관측의 caller/스택/효과 순서를 독립적으로 초기화한다."""
        self.trace=[];self.events=0;self.stop=0;self.getters=0;self.put(STACK,STOP)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP): self.mu.reg_write(register,0)  #이전 호출의 보존 레지스터를 섞지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)

    def event(self,value):
        """실제 하위 진입 전 trace를 남긴다."""
        self.trace.append(value);self.events+=1
        if self.events==self.stop: raise ObservationStop()

    def mutate_parent(self,point):
        """명시 하위 응답이 현재 owner/대기값을 바꾼 경우의 재읽기만 확인한다."""
        if self.mutation==point: self.put(0x540c70,2);self.put(PLAYERS+180+0xa0,125)

    def instruction(self,mu,address,size,data):
        """큰 획득/저장/UI는 실행하지 않고 원래 부모/합계 계산을 유지한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,4);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x48fc60: self.event('T')  #getter 본문도 계속 실제 실행해 EDX 보존을 확인한다.
        elif address==0x42b210 and self.mode=='notice':
            assert ecx==PLAYERS+180+0x6c;self.event('W,'+bytes(mu.mem_read(ecx,10)).hex());self.mutate_parent(1);self.ret(self.weight)
        elif address==0x4acac0:
            assert args[0]==4 and args[2]==0;self.event(f'F,{(ecx-POOL)//50},4,{args[1]},0');self.mutate_parent(2);self.ret(self.contained,12)
        elif address==0x4cf960:
            self.event('N,'+self.string(args[0]).decode('ascii'));self.mutate_parent(3);self.ret()
        elif address==0x448260: self.event(f'G,{args[0]},{args[1]}');self.ret()
        elif address==0x448310: self.event('S');self.ret()
        elif address==0x49a840:
            index=args[0];self.event(f'Y,{index}');self.getters+=1
            if self.getters==1 and self.mutation==1: self.put(TEXT+0x1000+6,1<<31);self.put(TEXT+0x1000+5,7,1)
            if self.getters==1 and self.mutation==2: self.put(0x541348,1)
            self.ret(TYPES+index*500)
        elif not any(low<=address<high for low,high in self.allowed): raise RuntimeError(f'검토하지 않은 Player 알림 명령 {address:08x}')

    def notice(self,name,known,battle,pending,technology,flag,weight,contained,stop,mutation):
        """독립 Player9기록/현재 전역과 명시 하위 응답으로 알림을 관측한다."""
        self.prepare();self.mode='notice';self.stop=stop;self.weight=weight;self.contained=contained;self.mutation=mutation
        self.mu.mem_write(PLAYERS,bytes((index*37+11)&255 for index in range(9*180)));self.put(PLAYERS+180+0x9c,technology);self.put(PLAYERS+180+0xa0,pending);self.put(PLAYERS+180+0x71,flag,1)
        for address,value in ((0x595344,known),(0x594fbc,battle),(0x540c70,1),(0x59534c,PLAYERS),(0x5c8464,POOL),(0x5411cc,93),(0x5412c0,154)): self.put(address,value)  #실제 전역 주소와 물리 역할이다.
        completed=True
        try:self.mu.emu_start(0x490da0,STOP,count=10000)
        except ObservationStop:completed=False
        return [name,*map(str,(known,battle,pending,technology,flag,weight,contained,stop,mutation)),str(int(completed)),str(self.words(0x540c70,1)[0]),bytes(self.mu.mem_read(PLAYERS,9*180)).hex(),';'.join(self.trace)]

    def core(self,name,mask,base,types,mutation):
        """별도42b210의 마스크 재읽기/첫 일치/최종BYTE 합계를 관측한다."""
        self.prepare();self.mode='core';self.mutation=mutation;self.put(TEXT+0x1000+5,base,1);self.put(TEXT+0x1000+6,mask);self.put(0x541348,len(types))
        for index,(bit,level) in enumerate(types): self.put(TYPES+index*500+0x1ec,bit);self.put(TYPES+index*500+0x94,level)  #서로 다른 두 signedDWORD를 실제 위치에 쓴다.
        self.mu.reg_write(UC_X86_REG_ECX,TEXT+0x1000);self.mu.emu_start(0x42b210,STOP,count=50000)
        encoded=';'.join(f'{bit},{level}' for bit,level in types) or '-'
        return [name,str(mask),str(base),encoded,str(mutation),str(self.mu.reg_read(UC_X86_REG_EAX)&255),bytes(self.mu.mem_read(TEXT+0x1000+5,5)).hex(),str(self.words(0x541348,1)[0]),';'.join(self.trace)]


def notice_cases():
    """보류0/부모SID0/알림·획득·이미존재·특수와 부분효과를 구별한다."""
    for known,battle,pending,technology in ((0,1,123,5),(1,0,123,5),(1,1,0,5),(1,1,123,0)): yield f'gate-{known}-{battle}-{pending}-{technology}',known,battle,pending,technology,1,43,0,0,0  #선행 관문이다.
    for pending in (2,3,93,154,123,125):  #원래 두 승급/두 특수/일반 기술 분기다.
        for flag in (0,1,2,255):  #BYTE 하한/이미 승급/상한을 나눈다.
            for weight in (42,43):  #Core의43 일치만 보상으로 이어진다.
                for contained in (0,9): yield f'normal-{pending}-{flag}-{weight}-{contained}',1,1,pending,5,flag,weight,contained,0,0  #내용물 부재/존재다.
    for pending in (2,93,123):  #각 하위 진입의 부분 쓰기/중단을 관측한다.
        for stop in range(1,6): yield f'stop-{pending}-{stop}',1,1,pending,5,0,43,0,stop,0
    for pending,mutation in ((93,1),(123,2),(2,3)): yield f'mutate-{mutation}',1,1,pending,5,0,43,0,0,mutation  #캡처한기록/현재owner·pending 재조회를 나눈다.


def core_cases():
    """빈 표/중복 첫 일치/음수·감김/현재 마스크·타입수 변경을 나열한다."""
    tables=[[],[(0,0)],[(1,2),(0,0),(1,7)],[(-1,1),(31,-1),(0,0x7fffffff)],[(bit,bit%5-1) for bit in range(32)]]  #독립 TYPE 입력이며 제품에서 정답을 만들지 않는다.
    for mask in (0,1,2,3,1<<31,0xffffffff):  #낮은/높은/전체 기술 비트다.
        for base in (0,43,255):  #마지막 BYTE 가산/감김이다.
            for index,types in enumerate(tables): yield f'core-{mask}-{base}-{index}',mask,base,types,0
    yield 'core-mutated-mask',3,1,[(0,2),(31,4)],1
    yield 'core-mutated-count',3,5,[(31,-1),(0,1),(1,3)],2


def sha(path):
    """보호 입력/최초 관측의 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """독립 두 실제CPU 관측을 대조하고 최초 두 기대 파일을 새로 만든다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);rows=[]
    for replay in range(2):  #독립 emulator 메모리에서 같은 입력을 다시 실행한다.
        oracle=Oracle();value=([oracle.notice(*case) for case in notice_cases()],[oracle.core(*case) for case in core_cases()]);rows.append(value);(out/f'{replay}.json').write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
    assert rows[0]==rows[1];fixtures=[]
    for name,table in zip(('player-notices-x86.tsv','core-technology-weight-x86.tsv'),rows[0]):  #기존 기대 파일을 재생성하지 않는다.
        path=ROOT/'cpppj/tests/fixtures'/name
        with path.open('x',encoding='utf-8',newline='\n') as stream:stream.writelines('\t'.join(row)+'\n' for row in table)
        fixtures.append(path.relative_to(ROOT).as_posix())
    sources=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_priestmenu.py','tools/decomp_player_notices.py',*fixtures]
    report={'unit':604,'host':socket.gethostname(),'date':'2026-10-30','lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','noticeCases':len(rows[0][0]),'coreCases':len(rows[0][1]),'observations':sum(map(len,rows[0]))*2,'differences':[],'gameStarted':False,'scope':'490da0와48fc60 실제 CPU, 별도42b210 실제 CPU. 큰 Core합계/내용물/획득·갱신/안내 및 TYPE조회는 명시 외부 경계. 실제 희생/포획/전투/게임/MP 실행 없음.','sourceSha256':{p:sha(ROOT/p) for p in sources},'evidenceSha256':{p.name:sha(p) for p in out.iterdir() if p.is_file()}}
    (ROOT/'cpppj/player-notices-native-20261030.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('noticeCases','coreCases','observations','differences')}))


if __name__=='__main__':
    main()
