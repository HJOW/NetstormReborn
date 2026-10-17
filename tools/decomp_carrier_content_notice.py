#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Carrier의 실제 내용물 가상 통지429560과 원본 TYPE/벡터 wrapper를 격리 실행한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_object_menu import Oracle as BaseOracle,ROOT,POOL,TYPES,TEXT,STACK,STOP,digest
from unicorn import UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-content-notice-x86.tsv'  # 기존 내용물/메뉴 관측과 별도의 입력/효과다.
REPORT=ROOT/'cpppj/carrier-content-notice.json'  # 실제 진입/반환/경계와 SHA를 보존한다.
PROC=TEXT+0x5000  # 실제 new 경계가 돌려주는 opaque 프로세스 입력이다.
ENTRIES=(0x429560,0x49a840,0x4ac550,0x41cc50,0x497220,0x4e49c0)  # 원본 몸체를 대체하지 않고 실행한다.


def bits(value):
    """현재 좌표의 단정도 비트만 공급한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """reason/상태/Genus/순서와 외부 효과의 명시 재진입 입력을 만든다."""
    # saved,load,bombBlocked,owner,local,kind,flags2,dead,word20,reason,x,y,allocate,create,genus,type,effect,sound,mutation,cameraX,cameraY.
    base=[0,0,0,1,1,123,0,0,32,4,bits(40.25),bits(50.75),1,1,0,0,0x12345678,0x55667788,0,0xffffffef,31];rows=[]
    # 가능한 조합을 기대 분기로 필터링하지 않는다.
    for kind,flags,reason,owner,dead,word,blocked in itertools.product((123,158,168),(0,0x100,0x208100,0x200000,0x8000),(0,1,2,3,4,0x103),(1,2),(0,2),(0,32),(0,1)):
        row=base.copy();row[3]=owner;row[5:10]=kind,flags,dead,word,reason;row[2]=blocked;rows.append(row)
    # 실제 포획/폭탄 분기에서 효과 순서 뒤 현재 슬롯/전역 변경을 관찰한다.
    for kind,mutation,allocation,creation,genus,at in itertools.product((158,168),range(9),(0,1),(0,1),(0,1),(0,1)):
        row=base.copy();row[5]=kind;row[6]=0x100 if kind==168 else 0x200000;row[9]=3;row[12:16]=allocation,creation,genus,at;row[18]=mutation;rows.append(row)
    for saved,loading in itertools.product((0,1),(0,1,3)):
        row=base.copy();row[0:2]=saved,loading;row[5]=168;row[6]=0x100;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """알려진 원본 함수 범위와 명시 외부 효과만 허용한다."""
    def __init__(self):
        """원본 PE 사본과 별도 SEH DWORD/프로세스 입력을 준비한다."""
        super().__init__('originals');self.mu.mem_map(0,0x1000)
        self.allowed=[]  # 현재 정밀 함수표의 실행 허용 구간이다.
        self.entries=Counter()  # 실제 몸체별 누적 진입이다.
        self.boundaries=Counter()  # 실제 외부 효과별 진입을 보존한다.
        self.normal_returns=0  # 정상 ABI/SEH/x87를 확인한 루트 반환이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as fp:table={int(r['entry'],16):r for r in csv.DictReader(fp,delimiter='\t')}
        # 함수 사이의 코드를 실행하지 않는다.
        for entry in ENTRIES:
            for part in table[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read_zero,begin=0,end=0xfff)
        assert self.word(0x5412c8)==156
        assert struct.unpack('<f',self.mu.mem_read(0x503358,4))[0]==16 and struct.unpack('<f',self.mu.mem_read(0x50334c,4))[0]==11
        assert struct.unpack('<d',self.mu.mem_read(0x503350,8))[0]==.5

    def read_zero(self,mu,access,address,size,value,data):
        """SEH 포인터 이외의NULL 페이지 읽기를 차단해 미초기화 풀 주소를 숨기지 않는다."""
        assert address==0 and size==4,(hex(address),size)

    def write(self,mu,access,address,size,value,data):
        """원본 SEH FS:0 DWORD와 호출 스택 이외의 명령 쓰기를 거부한다."""
        assert address==0 and size==4 or STACK-0x10000<=address and address+size<=STACK+0x1000,(hex(address),size)

    def mutation(self,event):
        """입력으로 지정한 외부 효과만 슬롯/전역을 바꾼다."""
        mode=self.case[18]
        if event=='B' and mode==1:self.put(self.form+18,158,2);self.put(self.form+11,2,1)
        if event=='S' and mode==2:self.put(self.parent+34,2,1);self.put(self.parent+14,bits(99));self.put(self.parent+18,bits(77))
        if event=='A' and mode==3:self.put(0x540c70,2);self.put(self.form+18,123,2)
        if event=='N' and mode==4:self.put(0x5c89b8,1)
        if event=='H' and mode==5:self.put(self.form+18,158,2);self.put(self.parent+14,bits(25.5));self.put(self.parent+18,bits(30.25))
        if event=='G' and mode==6:self.put(self.parent+14,bits(55.5));self.put(self.parent+18,bits(66.25))
        if event=='P' and mode==7:self.put(self.form+11,2,1);self.put(0x5c89b8,1)
        if event=='B' and mode==8:self.put(0x5453e8,1);self.put(self.form+18,123,2);self.put(TYPES+123*500+0xec,0)

    def instruction(self,mu,address,size,data):
        """게임 계산 결과를 만들어 대체하지 않고 출력·할당·조회 경계 인수만 기록한다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=struct.unpack('<6I',mu.mem_read(esp+4,24))
        if address==0x4adea0:
            self.boundaries['C']+=1;self.emit('C',(ecx-POOL)//50);target=args[0];self.put(target,self.word(self.parent+14));self.put(target+4,self.word(self.parent+18));self.ret(target,4);return
        effects={0x472770:'B',0x4a9d70:'S',0x4e4391:'A',0x4ab240:'N',0x418070:'T',0x4a9550:'P',0x426080:'H',0x4b2070:'G',0x4b1fa0:'I',0x4a9cb0:'F'}
        if address in effects:
            event=effects[address];self.boundaries[event]+=1;result=purge=0
            if event=='B':self.emit(event,*args)
            elif event=='S':assert self.string(args[2])==b'thunderCrack.wav' and args[3]==0;self.emit(event,*args[:2])
            elif event=='A':self.emit(event,args[0]);result=PROC if self.case[12] else 0
            elif event=='N':assert ecx==PROC;self.emit(event,1,*args[:4]);result=PROC if self.case[13] else 0;purge=16
            elif event=='T':assert ecx==PROC+32;self.emit(event,1,args[0]);purge=4
            elif event=='P':self.emit(event,args[0])
            elif event=='H':self.emit(event,args[0])
            elif event in ('G','I'):self.emit(event,*args[:3]);result=(60 if event=='G' else 61) if self.case[14 if event=='G' else 15] else 0
            elif event=='F':assert self.string(args[0])==b'priestFree.wav' and args[1:]==(0,0,0,1,0);self.emit(event)
            self.mutation(event);self.ret(result,purge);return
        assert any(a<=address and address+size<=b for a,b in self.allowed),f'미등록 실행 {address:08x}'
        if address in ENTRIES:self.entries[f'{address:08x}']+=1

    def sample(self,case,precision):
        """현재 입력에서 원본 루트 전체와 효과 순서/두 슬롯/공유 전역을 관찰한다."""
        self.case=case  # 현재 독립 입력21DWORD다.
        self.events=[]  # 명시 경계의 실제 인수 순서다.
        self.parent=POOL+50*50  # 현재 사제/Carrier 부모 슬롯이다.
        self.form=POOL+60*50  # 부모에 통지한 실제 내용물 슬롯이다.
        self.mu.mem_write(POOL,b'\0'*0x10000);self.mu.mem_write(TYPES,b'\0'*0x20000);self.put(0,0);self.put(0x59ab20,TYPES);self.put(0x5c8464,POOL)
        assert self.word(0x5c8464)+60*50==self.form
        self.mu.mem_write(self.parent,b'\xa5'*50);self.mu.mem_write(self.form,b'\x5a'*50)
        self.put(self.parent,0x50f210);self.put(self.parent+10,158,1);self.put(self.parent+34,case[3],1);self.put(self.parent+14,case[10]);self.put(self.parent+18,case[11])
        self.put(self.form+11,case[7],1);self.put(self.form+18,case[5],2);self.put(self.form+20,case[8],2)
        # 미작성 타입 값도 고정 입력이며 현재 kind 변경 뒤 실제 메모리 조회를 구별한다.
        for type in range(188):self.put(TYPES+type*500+0x6c,0x10000000+type)
        self.put(TYPES+case[5]*500+0xec,case[6]);self.put(TYPES+case[5]*500+0x6c,case[16]);self.put(TYPES+158*500+0x7c,case[17])
        # 원본이 읽는 실제 전역 주소에 입력을 공급한다.
        for address,value in zip((0x5c85d4,0x5c89b8,0x5453e8,0x540c70),(case[0],case[1],case[2],case[4])):self.put(address,value)
        self.put(0x5412d0,158);self.put(0x5412c8,156);self.put(0x59a91c,case[19]);self.put(0x59a920,case[20])
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        # 이전 실행의 x87/범용 상태를 초기화한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.parent);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.put(STACK,STOP);self.put(STACK+4,60);self.put(STACK+8,case[9]);self.mu.emu_start(0x429560,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+12 and self.word(0)==0
        assert all(self.mu.reg_read(r)==v for r,v in saved.items());assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.normal_returns+=1
        values=[self.word(a) for a in (0x5c85d4,0x5c89b8,0x5453e8,0x540c70)]
        values+=[self.word(TYPES+t*500+0xec) for t in (123,158,168)];values+=[self.word(0x59a91c),self.word(0x59a920)]
        return ';'.join(self.events) or '-',','.join(map(str,values)),bytes(self.mu.mem_read(self.parent,50)).hex(),bytes(self.mu.mem_read(self.form,50)).hex()


def main():
    """두 원문 정밀도를 대조해 관측하거나 기존 SHA/고유 입력을 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in facts['sha256'].items():assert digest(name)==wanted,name
        rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
        assert len(rows)==facts['uniqueInputs']==len({r[0] for r in rows}) and facts['normalReturns']==len(rows)*2
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()]
        for row in rows:assert len(row)==5 and len(bytes.fromhex(row[3]))==50 and len(bytes.fromhex(row[4]))==50 and len(row[2].split(','))==9
        print('Carrier 내용물 통지 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # 원본 두 정밀도 외에 기대 분기/효과 계산기를 두지 않는다.
    for case in cases():
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# input effects globals parent50 form50\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    assert all(o.boundaries[key]>0 for o in oracles for key in ('C','B','S','A','N','T','P','H','G','I','F'))
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_object_menu.py','tools/decomp_carrier_content_notice.py',FIXTURE.relative_to(ROOT).as_posix(),'cpppj/src/o/RawCarrierContentNotice.h','cpppj/src/o/RawCarrierContentNotice.cpp']
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{e:08x}' for e in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(o.normal_returns for o in oracles),instructionObservations=sum(o.instructions for o in oracles),instructionObservationsIncludeBoundaryEntries=True,entryObservations=dict(sum((o.entries for o in oracles),Counter())),precisionDifferentInputs=[],boundaryObservations=dict(sum((o.boundaries for o in oracles),Counter())),osCalls=0,sha256={p:digest(p) for p in paths},limitations=['실제 가상 통지/TYPE lookup/벡터 wrapper는 원본 몸체다. 중심/번개/음향/new·생성자/타이머/공간 조회/Carrier check는 명시 경계다.','실제 Session의 일반 건설 Form 생성은 별도 통합 검증이며 Client 커서/확정/전체 플레이는 미완료다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Carrier 내용물:',len(rows),'/정상 반환',facts['normalReturns'],'/차이0/명령·경계',facts['instructionObservations'])


if __name__=='__main__':main()
