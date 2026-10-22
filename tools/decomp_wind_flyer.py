#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""더스트 데빌의 생성/초기화/애니메이션/주기/범위공격 원본 명령을 창 없이 관찰한다."""
import csv
import itertools
import json
import socket
import struct
import sys
from collections import Counter
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, TABLE, IMAGE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

POOL, FIELDS, TYPES, LIST, OUTPUT, SHIM = PLAYERS, HEAP, TABLE, IMAGE, IMAGE+0x1000, STOP+0x400  # 서로 겹치지 않는 관찰용 주소다.
IDS = (50,60,61,62)  # 공격자/다리/두 후보의 원문 순서다.
ENTRIES = (0x45e1c0,0x45c340,0x45e230,0x45c020,0x41bfb0,0x45e8b0,0x45e9a0,0x4ac200,0x4ac550,0x4d4d40,0x49acf0,0x49a840,0x4e49c0)  # 제어·산술·작은 getter·CRT는 실제 명령이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/wind-flyer-x86.tsv'  # 원본이 계산한 출력만 동결한다.
REPORT = ROOT/'cpppj/wind-flyer-20261022.json'  # 범위/정밀도/정상 반환과 원문 SHA다.


def bits(value):
    """명시 float 입력의 원문 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """생성자 미정 바이트/타이머 경계와80회 수명·다리·대상 종류를 교차한다."""
    rows=[]  # 입력 중복은 마지막에 제거한다.
    for kind in ('C','I'):
        for now,fill in itertools.product((0,100.25,-5.25),(0,0x5a,0xa5)):  # 두 몸체가 쓰지 않는 바이트도 확인한다.
            rows.append((kind,(now,fill)))
    for now,due,frame,extra,count in itertools.product((0,100.25),(0,100.25,101),(-1,0,12,13,21),(0,32),(1,13,22)):  # 기한 전/일치/후와 정지 비트다.
        rows.append(('A',(now,due,frame,extra,count)))
    for count,boss,period,stamp in itertools.product((0,1,39,40,79,80,81,120,0xffffffff),(0,1),(.25,.5),(0,77)):  # 음수 signed count와 소리 억제도 분리한다.
        rows.append(('R',(count,boss,period,stamp,3,0,0)))
    for mask,state,genus in itertools.product((0,1,2,3),(0,1,2,3),(0,0x100000,0x20000,4)):  # 후보 순서·free/dead·공중 제외와 표면 중복을 확인한다.
        rows.append(('D',(0,1,.25,0,mask,state,genus)))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """원문 몸체를 실행하며 큰 finder/힙/가상 피해/장치만 명시 입력으로 공급한다."""
    def __init__(self,control):
        """현재 PC 정밀 범위와 쓰기 감시를 설치한다."""
        super().__init__()
        self.control=control  # 매 호출 x87 제어어 입력이다.
        self.instructions=0  # 실제 원본 명령의 누적 수다.
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[]  # 검토한 불연속 원본 함수 구간이다.
        for entry in ENTRIES:  # 다른 helper는 미검토 호출로 실패시킨다.
            for part in facts[entry]['ranges'].split(';'):  # 끝 주소를 포함하는 범위를 변환한다.
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """실제 CPU 쓰기를 본체/후보/소리stamp/출력/스택/SEH에 한정한다."""
        ranges=[(FIELDS,FIELDS+104),(LIST,LIST+40),(OUTPUT,OUTPUT+4),(STACK-8192,STACK+256),(0,4)]
        ranges += [(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]
        ranges.append((TYPES+138*500+0x1f0,TYPES+138*500+0x1f4))
        if not any(lo<=address and address+size<=hi for lo,hi in ranges):raise RuntimeError(f'더스트 데빌 범위 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """분기/타이머/필터/피해 절삭은 실제 명령에 맡긴다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,10);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x41bd20:
            self.events.append('Attach,'+','.join(map(str,(args[0],args[1],args[3]))));self.put(FIELDS+52,70);self.put(FIELDS+56,50);self.ret(purge=16);return
        if address==0x4af110:self.events.append('Count');self.ret(self.count);return
        if address==0x4e0620:self.events.append('Assert,'+str(args[2]));self.ret();return
        if address==0x422230:
            assert args[:5]==(bits(50.25),bits(50.5),0,0,0)
            self.finder=ecx;self.index=0;self.put(ecx+0x34,60);self.ret(ecx,purge=20);return
        if address==0x4b1810:
            self.index+=1;self.put(ecx+0x34,(60,61,62,0)[self.index]);self.ret();return
        if address==0x40d890:self.put(ecx,LIST);self.put(ecx+4,10);self.put(ecx+8,0);self.ret(ecx,purge=4);return
        if address==0x46af80:
            target=args[8];self.events.append('May,'+str(target));self.ret((self.mask>>(target-61))&1 if target in (61,62) else 0);return
        if address==0x4d4d00:self.ret(0);return
        if address==0x4adea0:self.put(args[0],bits(49.75));self.put(args[0]+4,bits(50));self.ret(args[0],purge=4);return
        if address==0x421db0:self.events.append('Weaken,'+str(args[0]));self.ret();return
        if address==0x44c8e0:
            self.events.append('Damage,'+str((ecx-POOL)//50)+','+','.join(map(str,args[:3])));self.ret(purge=12);return
        if address==0x4af780:self.events.append('Destroy,'+str(args[0]));self.ret(purge=4);return
        if address==0x4a9c30:self.events.append('Sound,'+str(args[2]));self.ret();return
        if address==0x4e4354:self.ret();return
        if SHIM<=address<SHIM+80:return
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1;return
        raise RuntimeError(f'더스트 데빌 미검토 명령 {address:x}')

    def run(self,case):
        """현재 입력만 설치하고 정상ret 뒤 전체104BYTE·효과·stamp를 읽는다."""
        kind,values=case;mu=self.mu;self.events=[];self.count=22;self.mask=3
        for sid in IDS:  # 동일50BYTE 슬롯 입력으로 native와 C++의 후보 경계를 맞춘다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,0x50a050 if sid==50 else 0x501ea0);raw[10]=138 if sid==50 else 82 if sid==60 else 78 if sid==61 else 79
            struct.pack_into('<ff',raw,14,50.25 if sid==50 else 51,50.5 if sid==50 else 51);raw[34]=1 if sid==50 else 2;struct.pack_into('<I',raw,36,13 if sid==50 else 0);mu.mem_write(POOL+sid*50,bytes(raw))
        for typ in (138,82,78,79):  # 실제 읽는TYPE만 만들고 기본getter/가상bc는원문을실행한다.
            record=bytearray(500);struct.pack_into('<f',record,0x48,41.5);struct.pack_into('<I',record,0xec,4 if typ==82 else 0);struct.pack_into('<I',record,0x6c,881);mu.mem_write(TYPES+typ*500,bytes(record))
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x540bc0,1);self.put(0x540bc4,1);self.put(0x5410ec,31);self.put(0x55b4a8,77);self.put(0x5e4794,0);self.put(0,0x12340000)
        raw=bytearray(b'\xa5'*104);struct.pack_into('<I',raw,0,0x509f98);struct.pack_into('<III',raw,48,0x509f78,70,50);struct.pack_into('<d',raw,96,100);self.now=100.25;entry,receiver,args,purge=0x45c020,FIELDS,[],0
        if kind in ('C','I'):
            self.now,fill=values;raw[:]=bytes([fill])*104;entry=0x45e1c0 if kind=='C' else 0x45e230
            if kind=='C':args=[50,17,bits(4),bits(.5)];purge=16
        elif kind=='A':
            self.now,due,frame,extra,self.count=values;struct.pack_into('<d',raw,96,due);self.put(POOL+50*50+36,frame);self.put(POOL+50*50+40,extra,1)
        else:
            count,boss,period,stamp,self.mask,state,genus=values;self.put(0x540bc4,boss);self.put(TYPES+138*500+0x1f0,stamp);self.put(POOL+61*50+11,state,1);self.put(TYPES+79*500+0xec,genus)
            entry,receiver,args,purge=(0x45e8b0,POOL+50*50,[0,count,bits(period)],12) if kind=='R' else (0x45e9a0,POOL+50*50,[bits(period)],4)
        mu.mem_write(FIELDS,bytes(raw));mu.mem_write(0x55b4d0,struct.pack('<d',self.now));self.put(OUTPUT,0)
        if kind=='R':  # x87 반환값을외부계산없이실제FSTP로관측한다.
            for i,value in enumerate(args):self.put(OUTPUT+0x20+i*4,value)  # 인수는데이터로읽어이전행의번역캐시가즉시값을재사용하지않게한다.
            code=b''.join(b'\xff\x35'+struct.pack('<I',OUTPUT+0x20+i*4) for i in reversed(range(len(args))));at=SHIM+len(code);code+=b'\xe8'+struct.pack('<i',entry-at-5)+b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';mu.mem_write(SHIM,code);entry,args,purge=SHIM,[],0
        mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,receiver)
        saved=((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x34567812),(UC_X86_REG_EDI,0x56781234),(UC_X86_REG_EBP,0x78123456))
        for reg,value in saved:mu.reg_write(reg,value)  # 비휘발레지스터의실제정상복귀를확인한다.
        mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.emu_start(entry,STOP,count=200000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg,value in saved:assert mu.reg_read(reg)==value  # 스택/비휘발손상을출력성공으로간주하지않는다.
        result=self.words(OUTPUT)[0] if kind=='R' else mu.reg_read(UC_X86_REG_EAX) if kind=='A' else 0
        return [kind,','.join(map(str,values)),bytes(mu.mem_read(FIELDS,104)).hex(),str(result),str(self.words(TYPES+138*500+0x1f0)[0]),';'.join(self.events) or '-']


def main():
    """두정밀도의동일한원본관측을동결하거나기존원문SHA를검사한다."""
    if '--verify' in sys.argv:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,expected in facts['sha256'].items():assert digest(ROOT/path)==expected,path  # 기록한보호원본과자료만확인한다.
        print('Wind flyer native/SHA:',facts['observations']);return
    inputs=cases();first=Oracle(0x27f);rows=[first.run(case) for case in inputs];second=Oracle(0x37f);other=[second.run(case) for case in inputs]
    assert rows==other
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(('kind','input','fields','result','stamp','trace'));writer.writerows(rows)
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_wind_flyer.py','cpppj/tests/fixtures/wind-flyer-x86.tsv')
    facts={'host':socket.gethostname(),'last_full_refined_host':'HJOW-Athlon','last_full_refined_date':'2026-10-22','observations':len(rows),'scopes':dict(Counter(row[0] for row in rows)),'precision_replays':[0x27f,0x37f],'normal_returns':len(rows)*2,'instructions':first.instructions+second.instructions,'game_gui_runs':0,'limits':'생성/초기화/애니메이션/주기/범위피해제어·DPSgetter·CRT는실제명령이다. 큰finder의세후보/힙/표적허용·중심/다리약화/가상피해/소리/삭제는명시경계다. 후보의실제공간해시순회는C++에서같은배치로연결한다.','sha256':{p:digest(ROOT/p) for p in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Wind flyer native:',len(rows),facts['scopes'])


if __name__=='__main__':main()
