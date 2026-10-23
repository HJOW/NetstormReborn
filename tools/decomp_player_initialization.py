#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004910a0 전체236BYTE와 실제 동맹 자식을 제품 작성 전에 PE 격리 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_chat_channels import ChatOracle, initial as chat_initial
from decomp_priestmenu import ROOT, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_INVALID, UcError
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW)

ENTRY, ALLIANCE = 0x4910a0, 0x490ea0  # 실제 초기화 부모와 기존 방향별 동맹 몸체다.
BASE, FORT_COUNT, LAST_PLAYER = 0x59534c, 0x5951f8, 0x5951fc  # 서로 다른 실제 세 DWORD다.
KNOWN, OWNER, MY, CURRENT, ACTIVE = 0x595344, 0x540c70, 0x540c74, 0x595350, 0x595348  # 부모가 임의 정리하지 않는 현재 Core/선택 상태다.
BLOCKS = (PLAYERS, PLAYERS+0x1000, PLAYERS+0x2000)  # 현재 base 교체와 재할당 주소 재사용을 구별할 세 물리다.
ALLIES, HIDDEN, CHAT = 0x595200, 0x52f81e, 0x558ed0  # 실제 동맹81DWORD/숨김9BYTE/Chat4800BYTE다.
NEW_BASE, REENTER = 0x12310000, STOP+0x100  # 새 공장 블록 토큰과 실제 부모 재진입 반환점이다.
BOUNDARIES = {0x4e4354:'freeFactory',0x43fa90:'freePlayers',0x43fa80:'malloc',0x4e4391:'new'}  # CRT/외부 할당만 명시 장치로 구별한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/player-initialization-x86.tsv'  # 최초 Native 전체 상태/효과 순서다.
REPORT = ROOT/'cpppj/player-initialization-native-20261024.json'  # 이후 제품 결과와 구별하는 최초 관측 보고서다.


def cases():
    """기대 결과 없이 세 base·기존 포인터·Known·할당 실패·선택 중단을 교차한다."""
    rows=[]  # 같은 명시 입력을 두 정밀도로 실행한다.
    for seed,old,allocated,known,null_new in itertools.product((0,165),(3,0),(0,1,2),(0,1),(0,1,9)):  # NULL 이전 base와 같은/다른 주소 재할당을 포함한다.
        rows.append((seed,old,allocated,known,0x1ff,null_new,'none','none',0x155 if seed else 0,0xffffffff))
    rows.append((165,1,2,0,0x155,0,'none','none',0,0xffffffff))
    for old in (3,0):  # malloc NULL은 첫 zero store에서 실제 물리 오류가 난다.
        rows.append((165,old,3,0,0x1ff,0,'none','none',0,0xffffffff))
    for old in (0,1):  # free(NULL) 호출도 생략하지 않아야 한다.
        rows.append((165,old,2,0,0,0,'none','none',0,0xffffffff))
    mutations=('freeFactory1-baseB','freeFactory9-baseB','freePlayers1-baseB','malloc1-baseB',
               'new1-baseB','new9-baseB','alliance1-baseB','new1-baseNull','freeFactory1-baseNull',
               'new1-row64','alliance1-known1','freeFactory1-reenter','new1-reenter')  # 캡처한 행과 이후 현재 base를 독립 변경한다.
    for mutation in mutations:  # 외부 장치 복귀 뒤의 원문 재조회/부분효과만 관측한다.
        rows.append((165,0,2,0,0x1ff,0,'none',mutation,0,0xffffffff))
    for stop in ('freeFactory1','freeFactory9','freePlayers1','malloc1','new1','new5','new9','alliance1','alliance5','alliance9'):  # 실제 callee 진입에서만 중단한다.
        rows.append((165,0,2,0,0x1ff,0,stop,'none',0,0xffffffff))
    rows.append((165,0,2,1,0x1ff,0,'none','new1-baseB',0x155,0xffffffff))
    return list(dict.fromkeys(rows))


def player_input(seed,block,mask):
    """해제 대상/기존 이름만 입력하며 초기화 결과나 동맹 결과를 계산하지 않는다."""
    data=bytearray((seed+block*53+index*17)&255 for index in range(1620))
    for owner in range(9):  # 실제9×180BYTE에 오래된 공장 포인터/이름/subscriber를 넣는다.
        offset=owner*180
        struct.pack_into('<I',data,offset,0x12408000+block*0x2000+owner*0x100 if mask&(1<<owner) else 0)
        data[offset+12:offset+44]=b'OldName\0'+bytes([65+owner])*24
        struct.pack_into('<I',data,offset+44,700+owner+block*100)
    return bytes(data)


class Oracle(ChatOracle):
    """부모/동맹/Chat 계산은 실제 PE이며 할당·free/기존 문자열 CRT만 외부 장치다."""
    def __init__(self):
        """기존 Chat 원문 허용 범위에 정확236/491BYTE 두 함수를 추가한다."""
        super().__init__()
        self.parent=[]  # BASE read/write trace는 부모 명령의 접근만 기록한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 같은 호스트 전체 정밀 범위를 읽는다.
                entry=int(row['entry'],16)
                if entry not in (ENTRY,ALLIANCE):
                    continue
                spans=[tuple(int(value,16) for value in part.split('-')) for part in row['ranges'].split(';')]
                if entry==ENTRY:
                    self.parent=[(low,high+1) for low,high in spans]
                self.allowed.extend((low,high+1) for low,high in spans)
        if sum(high-low for low,high in self.parent)!=236:
            raise RuntimeError('Player 초기화236BYTE 범위 불일치')
        self.running=False  # 명시 입력 준비는 CPU 효과에 포함하지 않는다.
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
        self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid)

    def in_parent(self):
        """현재 EIP가 실제 초기화 부모의 검토 범위인지 조회한다."""
        address=self.mu.reg_read(UC_X86_REG_EIP)
        return any(low<=address<high for low,high in self.parent)

    def read(self,mu,access,address,size,value,data):
        """각 부모 BASE load와 캡처 위치를 기록하며 자식의 내부 read는 구별한다."""
        if self.running and address==BASE and self.in_parent():
            self.trace.append('R:'+str(self.words(BASE,1)[0]))

    def guard(self,mu,access,address,size,value,data):
        """현재 세 Player/동맹/Chat/부모 전역과 스택 외 쓰기를 거부한다."""
        if not getattr(self,'running',False):
            return
        allowed=(any(base<=address and address+size<=base+1620 for base in BLOCKS) or
                 FORT_COUNT<=address and address+size<=ALLIES+324 or
                 BASE<=address and address+size<=BASE+4 or CHAT<=address and address+size<=CHAT+4800 or
                 0x531750<=address and address+size<=0x531752 or STACK-8192<=address and address+size<=STACK+64)
        if not allowed:
            raise RuntimeError(f'Player 초기화 범위 밖 쓰기 {address:08x}/{size}')
        if address==BASE and self.in_parent():
            self.trace.append('W:'+str(value&0xffffffff))

    def invalid(self,mu,access,address,size,value,data):
        """선택한 NULL 현재 base/할당의 실제 물리 실패를 정상 반환과 구별한다."""
        if self.running and address<1620:
            self.status='fault-null';mu.emu_stop();return False
        raise RuntimeError(f'Player 초기화 예상 밖 미매핑 {address:08x}')

    def mutate(self,boundary,esp):
        """명시 장치의 입력 변경·재진입만 수행하며 부모/동맹 결과는 계산하지 않는다."""
        trigger,_,action=self.case[7].partition('-')
        if trigger!=boundary:
            return False
        self.trace.append('M:'+action)
        if action=='baseB':self.put(BASE,BLOCKS[1])
        elif action=='baseNull':self.put(BASE,0)
        elif action=='row64':self.put(self.words(BASE,1)[0]+0x64,0x12345678)
        elif action=='known1':self.put(KNOWN,1)
        elif action=='reenter':
            self.trace.append('RE:begin');self.reentry.append((esp,self.pending));mu=self.mu
            mu.reg_write(UC_X86_REG_ESP,esp-4);self.put(esp-4,REENTER);mu.reg_write(UC_X86_REG_EIP,ENTRY);return True
        else:raise RuntimeError('Player 초기화 미해석 장치 변경:'+action)
        return False

    def instruction(self,mu,address,size,data):
        """외부 할당과 실제 동맹 진입을 관측하고 나머지 계산은 CPU에 맡긴다."""
        self.instructions+=1
        if self.instructions>500000:
            raise RuntimeError('Player 초기화 명령 상한')
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==REENTER:
            saved,pending=self.reentry.pop();mu.reg_write(UC_X86_REG_ESP,saved);self.trace.append('RE:end');self.complete(*pending);return
        if address==ALLIANCE:
            self.boundary['alliance']+=1;count=self.boundary['alliance'];owner,mask=self.words(esp+4,2)
            self.trace.append(f'A:{owner}:{mask}')
            if self.case[6]=='alliance'+str(count):self.status='stop';mu.emu_stop();return
            self.pending=('alliance',0,count)
            if self.mutate('alliance'+str(count),esp):raise RuntimeError('동맹 재진입 입력은 아직 정의하지 않음')
        if address in BOUNDARIES:
            kind=BOUNDARIES[address];argument=self.words(esp+4,1)[0];self.boundary[kind]+=1;count=self.boundary[kind]
            self.trace.append(kind+':'+str(argument))
            if self.case[6]==kind+str(count):self.status='stop';mu.emu_stop();return
            self.pending=(kind,argument,count)
            if self.mutate(kind+str(count),esp):return
            self.complete(kind,argument,count);return
        super().instruction(mu,address,size,data)

    def complete(self,kind,argument,count):
        """명시 할당 장치의 주소/NULL 반환만 공급하며 초기화는 원문이 수행한다."""
        if kind=='malloc':
            if argument!=1620:raise RuntimeError('Player malloc 크기 불일치')
            self.ret(0 if self.case[2]==3 else BLOCKS[self.case[2]])
        elif kind=='new':
            if argument!=120 or count>27:raise RuntimeError('Player factory new 크기/개수 불일치')
            self.ret(0 if count==self.case[5] else NEW_BASE+(count-1)*0x100)
        elif kind in ('freeFactory','freePlayers'):self.ret()
        else:raise RuntimeError('Player 초기화 미해석 장치 완료')

    def sample(self,case,control):
        """같은 입력을 새 물리/GP/CW로 준비하고 전체 정상/중단/실패 상태를 읽는다."""
        self.running=False;self.case=case;seed,old,allocated,known,mask,null_new,stop,mutation,hidden,my=case
        self.trace=[];self.events=[];self.boundary=Counter();self.reentry=[];self.status='normal';self.instructions=0
        for index,base in enumerate(BLOCKS):  # 세 Player 물리는 free 관측 뒤에도 사후 비교를 위해 보존한다.
            self.mu.mem_write(base,player_input(seed,index,mask))
        factory=bytes((seed+index*13)&255 for index in range(27*0x100))  # 부모가 쓰지 않는 실제 할당 입력이다.
        self.mu.mem_write(NEW_BASE,factory)
        self.mu.mem_write(ALLIES,bytes((seed+index*7)&255 for index in range(324)))
        self.mu.mem_write(HIDDEN,bytes((hidden>>index)&1 for index in range(9)))
        self.mu.mem_write(CHAT,chat_initial(0));self.put(BASE,0 if old==3 else BLOCKS[old]);self.put(FORT_COUNT,0x12345678);self.put(LAST_PLAYER,0x89abcdef)
        self.put(KNOWN,known);self.put(OWNER,1);self.put(MY,my);self.put(CURRENT,BLOCKS[0]+180+0x6c);self.put(ACTIVE,0xa5a5a5a5);self.put(0x594fc8,0)
        self.mu.mem_write(STACK-8192,bytes(8256));self.put(STACK,STOP)
        saved={UC_X86_REG_EBX:0x11112222,UC_X86_REG_ESI:0x33334444,UC_X86_REG_EDI:0x55556666,UC_X86_REG_EBP:0x77778888}  # 원문 callee 보존 GP다.
        for register,value in saved.items():self.mu.reg_write(register,value)  # 정상 반환 뒤 네 레지스터를 비교한다.
        self.mu.reg_write(UC_X86_REG_EAX,0xaaaa5555);self.mu.reg_write(UC_X86_REG_ECX,0xbbbb6666);self.mu.reg_write(UC_X86_REG_EDX,0xcccc7777)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.running=True
        try:self.mu.emu_start(ENTRY,STOP)
        except UcError:
            if self.status!='fault-null':raise
        self.running=False
        if self.status=='normal':
            if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or self.mu.reg_read(UC_X86_REG_FPCW)!=control or any(self.mu.reg_read(reg)!=value for reg,value in saved.items()):
                raise RuntimeError('Player 초기화 정상 반환/GP/ESP/CW 불일치')
        if bytes(self.mu.mem_read(NEW_BASE,len(factory)))!=factory:raise RuntimeError('Player 초기화가 factory 할당 BYTE를 변경함')
        globals_out=','.join(str(self.words(address,1)[0]) for address in (BASE,FORT_COUNT,LAST_PLAYER,KNOWN,OWNER,MY,CURRENT,ACTIVE))
        memory=b''.join(bytes(self.mu.mem_read(base,1620)) for base in BLOCKS)
        return (f'{control:04x}',','.join(map(str,case)),self.status,globals_out,memory.hex(),bytes(self.mu.mem_read(ALLIES,324)).hex(),bytes(self.mu.mem_read(CHAT,4800)).hex(),';'.join(self.events) or '-', ';'.join(self.trace),self.instructions)


def main():
    """관측은 최초 파일에만 쓰며 검증은 Native 입력/원문/도구 SHA만 읽는다."""
    parser=argparse.ArgumentParser(description=__doc__)  # 실제 PE 관측 여부를 명시 선택한다.
    choice=parser.add_mutually_exclusive_group(required=True);choice.add_argument('--observe',action='store_true');choice.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text('utf-8'));assert all(digest(ROOT/name)==value for name,value in facts['inputs'].items())
        print(json.dumps(dict(verified=True,observations=facts['observations'])));return
    assert socket.gethostname().casefold()=='hjow-athlon' and not FIXTURE.exists() and not REPORT.exists()
    assert not any((ROOT/name).exists() for name in ('cpppj/src/o/RawPlayerInitialization.h','cpppj/src/o/RawPlayerInitialization.cpp','cpppj/tests/PlayerInitializationTests.cpp'))
    oracle=Oracle();rows=[];statuses=Counter()
    for case in cases():  # 모든 기대는 실제 PE 명령 출력에서만 읽는다.
        for control in (0x027f,0x037f):
            row=oracle.sample(case,control);rows.append(row);statuses[row[2]]+=1
    with FIXTURE.open('x',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(('control','input','status','globals','players','allies','chat','chatEvents','trace','instructions'));writer.writerows(rows)
    sources=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_priestmenu.py','tools/decomp_chat_channels.py','tools/decomp_player_initialization.py','cpppj/tests/fixtures/player-initialization-x86.tsv')  # 제품 전 Native 입력만 고정한다.
    facts=dict(host=socket.gethostname(),date='2026-10-24',lastFullDecompile='HJOW-Athlon / 2026-10-23',
               entry='004910a0',parentBytes=236,allianceBytes=491,cdSourceMapCandidate='00406200',cdExecuted=False,
               observations=len(rows),distinctInputs=len(cases()),statuses=dict(statuses),gameGuiOsCalls=0,
               productWritten=False,allocatorBoundary='명시 CRT malloc/free/new; VM 물리는 사후 비교용 보존',
               inputs={name:digest(ROOT/name) for name in sources})
    with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(facts,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(observations=len(rows),distinctInputs=len(cases()),statuses=dict(statuses))))


if __name__=='__main__':
    main()
