#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Factory 실제 메뉴/지식 행/CRT정렬/마나 문자열을 독립 x86으로 관측한다."""
import argparse
import collections
import csv
import json
import socket
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as MenuOracle, ROOT, POOL, TYPES, TEXT, STACK, STOP, digest, fbits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 이번 새 관측은 기존 사제 메뉴와 분리하며 전체 정밀 함수표의 실제 몸체 범위를 사용한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/factory-menu-x86.tsv'
REPORT=ROOT/'cpppj/factory-menu.json'
ENTRIES=(0x4754b0,0x454c00,0x4540d0,0x4752b0,0x454040,0x453f70,0x492960,0x473020,0x4735c0,0x472e60,0x453f60,0x4ad0f0,0x4ad0a0,0x4ac970,0x4e49c0,0x4e4a40,0x4e6010,0x4e5fa0)
FLAGS=(0x800,0x8000000,0x10000,0x4000000,0x20000000,0x40000,0x20000,0)
SID=50


def inputs():
    """출력 판정 없이 소유/건설/레벨/기술·생산 중복/금지/SP의 명시 조합을 만든다."""
    base=[85,1,1,2,0,3,0,800,fbits(2200),'75,76,77,78,79,80,81','-','-','-',0]
    cases=[base.copy()]
    # 하나의 입력 축만 바꾸는 실제 관문/경계 조합이다.
    for column,values in ((0,(126,)),(1,(0,2)),(3,(0,1,3,6)),(4,(1,2,3,7)),(5,(0,1,2)),(6,(1,)),(8,(fbits(0),fbits(799),fbits(800),fbits(float('nan')))),(9,('-', '158,133,100,101,102,84,122,121,138,144,150,73,123,124,125,85,86,87,88,126','75,75,76,77,78,79,80,81,89,90,91,92,93,94,95,96,97,98,99')),(10,('75','75,75,76','75,76,77,78,79,80,81,89,90,91')),(11,('75','75,76,77,78,79,80,81')),(12,('75','75,76,77,78,79,80,81')),(13,(1,))):
        for value in values:
            row=base.copy();row[column]=value;cases.append(row)
    # 정렬의 동률/피벗 분할을 원본 CRT로 관찰하는 긴 지식 목록이다.
    for profile in (0,1):
        for theme in range(4):
            row=base.copy();row[4]=3;row[5]=theme;row[9]=','.join(map(str,range(74,120)));row[13]=profile;cases.append(row)
    return list(dict.fromkeys(tuple(row) for row in cases))


def numbers(value):
    """명시한 빈 목록과 쉼표 DWORD 목록을 구분한다."""
    return [] if value=='-' else list(map(int,value.split(',')))


class Oracle(MenuOracle):
    """게임 조회/GUI 장치를 경계로 두고 메뉴/행/정렬/문자열은 실제 명령으로 실행한다."""
    def __init__(self):
        super().__init__()
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[];self.facts=facts;self.hits=collections.Counter();self.boundaries=collections.Counter();self.returns=0;self.recent=collections.deque(maxlen=20)
        # 전체 정밀 디컴파일과 같은 원본 함수 범위만 허용한다.
        for address in ENTRIES:
            if address==0x453f60:
                assert bytes(self.mu.mem_read(address,12)).hex()=='8b01ff908000000083c002c3';self.allowed.append((address,address+12));continue
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """실제 실행 몸체와 명시 외부 조회/장치 경계를 서로 구분한다."""
        self.recent.append(hex(address))
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX);self.boundaries[hex(address)]+=1
        if address==0x4ac550:self.ret(TYPES+self.mu.mem_read(ecx+10,1)[0]*500)
        elif address==0x4e5f7b:self.ret(args[0]-32 if 97<=args[0]<=122 else args[0]) # CRT 로캘/스레드는 C 로캘 문자 입력 경계다.
        elif address==0x4e65ed:
            first=self.string(args[0]).lower()+b'\0';second=self.string(args[1]).lower()+b'\0'
            difference=next((a-b for a,b in zip(first,second) if a!=b),0);self.ret(difference) # C 로캘의 ASCII 비교 경계이며 정렬/비교기는 실제 몸체다.
        elif address==0x4acac0:
            assert args[:2]==(4,0);self.ret(self.tech[args[2]] if args[2]<len(self.tech) else 0,12)
        elif address==0x48fc60:self.ret(9)
        elif address==0x4b2270:
            assert args[0]==SID;self.iterator=0;self.put(ecx+8,self.products[0] if self.products else 0);self.ret(purge=4)
        elif address==0x4b1b20:
            self.iterator+=1;self.put(ecx+8,self.products[self.iterator] if self.iterator<len(self.products) else 0);self.ret()
        elif address==0x4c2400:self.ret(int(args[0] not in self.blocked))
        elif address==0x41c7a0:self.ret(int(args[0] in self.excluded),4)
        elif address==0x44f760:self.ret(TEXT+0xd000)
        elif address==0x44f4d0:self.ret(int(args[0] in self.production),4)
        elif address==0x40ec50:self.put(args[1],self.money);self.ret()
        elif address==0x4c2630:self.ret()
        elif address==0x4b9320:assert args[0]==256;self.ret(TEXT+0xf000)
        elif address==0x4ada70:self.events.append('Q,'+str(args[0]));self.ret()
        elif address==0x44cbd0:self.events.append('B,'+str((ecx-POOL)//50));self.ret()
        elif address==0x475cf0:self.events.append('A,'+','.join(map(str,args[:3])));self.ret()
        elif address==0x478020:self.events.append('H,'+self.string(args[0]).hex());self.ret()
        elif address==0x478160:
            self.events.append('S,'+self.string(args[0]).hex()+','+str(args[1]));self.ret()
        elif address==0x477cb0:
            parameter='str:'+self.string(args[2]).hex() if args[1]==0x02000017 and args[2]>255 else str(args[2])
            self.events.append('I,'+self.string(args[0]).hex()+','+str(args[1])+','+parameter+','+','.join(map(str,args[3:6])));self.ret()
        elif address==0x4e0620:raise RuntimeError('Factory 원본 진단: '+self.string(args[0]).decode('latin1'))
        else:super().instruction(mu,address,size,data)

    def prepare(self,row):
        """원본 메뉴를 만들기 위한 명시 raw/TYPE/목록만 공급한다."""
        typ,owner,local,extra,level,theme,editor,cost,money,tech,products,blocked,production,profile=row
        self.tech=numbers(tech);self.blocked=set(numbers(blocked));self.excluded=self.blocked;self.production=set(numbers(production));self.money=money
        self.events=[];self.taken=0;self.products=[];self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(TYPES,bytes(188*500));self.mu.mem_write(TEXT,bytes(0x10000))
        self.put(0x5c8464,POOL);self.put(0x540c70,local);self.put(0x5c85a4,editor);self.put(0x54db38,TEXT+0xe000);self.mu.mem_write(TEXT+0xe000,b'Help\0')
        self.put(POOL+SID*50,0x509148 if typ==85 else 0x509210);self.put(POOL+SID*50+10,typ,1);self.put(POOL+SID*50+11,level<<5,1);self.put(POOL+SID*50+34,owner,1);self.put(POOL+SID*50+40,extra,1)
        # 실제 TYPE 필드 입력은 번호를 유지하며 정렬 동률 문자열을 별도 profile로 만든다.
        for number in range(188):
            self.put(TYPES+number*500+0x98,theme if number==typ else number%4);self.put(TYPES+number*500+0xec,FLAGS[number%len(FLAGS)])
            name=('Same' if number%2 else 'same') if profile else 'Type'+str(number);self.mu.mem_write(TEXT+number*64,name.encode()+b'\0')
            self.put(0x59ab50+number*4,cost+number*23)
        self.put(TEXT+0xc004,37);self.put(TEXT+0xc008,-11) # 실제 메뉴 작성 뒤 읽는 같은 사건의 좌표 입력이다.
        # 중복도 그대로 별도 Form 입력이며 내용물 유형/flags만 판정한다.
        for index,kind in enumerate(numbers(products)):
            sid=100+index;self.products.append(sid);self.put(POOL+sid*50+18,kind,2);self.put(POOL+sid*50+20,0x10,2)

    def invoke(self,address,args=(),ecx=0,purge=0,control=0x27f):
        """보존 레지스터/스택/x87 제어어를 검사하고 실제 정상 반환만 결과로 받는다."""
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,ecx)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));before=(bytes(self.mu.mem_read(POOL,0x10000)),bytes(self.mu.mem_read(TYPES,188*500)))
        try:self.mu.emu_start(address,STOP,count=1000000)
        except Exception:
            print('최근 실제 명령',list(self.recent),'EIP',hex(self.mu.reg_read(UC_X86_REG_EIP)),flush=True);raise
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(self.mu.reg_read(register)==value for register,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert before==(bytes(self.mu.mem_read(POOL,0x10000)),bytes(self.mu.mem_read(TYPES,188*500)))
        self.returns+=1;return self.mu.reg_read(UC_X86_REG_EAX),';'.join(self.events) or '-'


def generate():
    """원본 결과를 계산하지 않고 실행 출력/호출 순서만 동결한다."""
    oracle=Oracle();rows=[]
    # 두 초기화 표는 실제472e60으로 만들며 장치/파일을 실행하지 않는다.
    oracle.events=[];oracle.taken=0;oracle.invoke(0x472e60)
    for case in inputs():
        outputs=[]
        # 두 x87 제어값에서 같은 명시 입력을 다시 공급하며 전체 원본을 재실행한다.
        for control in (0x27f,0x37f):
            oracle.prepare(case);outputs.append(oracle.invoke(0x4754b0,(TEXT+0xc000,),POOL+SID*50,4,control))
        assert outputs[0]==outputs[1];result,trace=outputs[0];rows.append(['Open',*map(str,case),str(result),trace]);print('Factory 원본 메뉴',len(rows),flush=True)
    # 마나의 점 생략/대소문자/알 수 없는ASCII/명시 활성 개수를 실제 문자열 함수로 관찰한다.
    for text in ('','?','??','..?w.r','wrtsWRTS.?>','w..','....','Z'):
        for active in (-1,0,1,3):
            outputs=[]
            for control in (0x27f,0x37f):
                oracle.events=[];oracle.mu.mem_write(TEXT+0xc000,text.encode()+b'\0');oracle.invoke(0x4735c0,(0,active&0xffffffff),TEXT+0xc000,8,control);outputs.append(oracle.string(TEXT+0xc000).hex())
            assert outputs[0]==outputs[1];rows.append(['Mana',text.encode().hex() or '-',str(active),outputs[0] or '-'])
    # 슬롯0의 번역과 음수/양수의 아이콘 문자열 반환도 개별 결과로 보존한다.
    for count in (-5,0,1,2,5):
        outputs=[]
        for control in (0x27f,0x37f):
            oracle.events=[];pointer,_=oracle.invoke(0x454040,(count&0xffffffff,),control=control);outputs.append(oracle.string(pointer).hex())
        assert outputs[0]==outputs[1];rows.append(['Slots',str(count),outputs[0] or '-'])
    FIXTURE.write_text('# Open/type,owner,local,extra,level,theme,editor,cost,moneybits,tech,products,blocked,production,profile/result/trace\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={Path(__file__).resolve(),ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE}
    report=dict(host=socket.gethostname(),date='2026-10-18',unit=171,rows=len(rows),counts=dict(collections.Counter(row[0] for row in rows)),controls=[0x27f,0x37f],precisionDifferences=0,normalReturns=oracle.returns,instructions=oracle.instructions,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['전체 Factory 메뉴/마나 문자열/지식 행/CRT정렬 실제 실행. TYPE·현재 목록·SP·Deck 포함 여부는 명시 입력 조회 경계','GUI 작성/번역·printf/C로캘toupper·stricmp/스레드 임시 버퍼는 관측 경계. 원본 게임/OS/새 디컴파일 없음','10.78만 관측. CD 대응은 전체 정밀 코드에 보존하며 새 검증은 후속'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """출력은 재계산하지 않고 정상 반환/현재 입력/원본 몸체 SHA를 검사한다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for p,w in r['sha256'].items():assert digest(ROOT/p)==w,p
    assert len(rows)==r['rows']==len(inputs())+37 and r['normalReturns']==len(rows)*2+1 and r['controls']==[0x27f,0x37f] and not r['precisionDifferences']
    for row,case in zip(rows,inputs()):assert row[0]=='Open' and row[1:15]==list(map(str,case)) and len(row)==17
    print('Factory 메뉴 원문/SHA/입력 감사',len(rows))


# 새 관측 실행과 기존 근거 감사를 명시 옵션으로 나눈다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
