#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 지식 선택·58h 연구 수신·Factory 내용물 통지를 실제 x86으로 관측한다."""
import argparse
import collections
import json
import socket
import struct
from pathlib import Path
from decomp_factory_menu import Oracle as MenuOracle, ROOT, POOL, TYPES, TEXT, STACK, STOP, digest, fbits
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

# 연구 명령의 실제 몸체와 공통16바이트 생성자다. 업그레이드 진행은 이번 입력 범위에서 제외한다.
ENTRIES=(0x4547d0,0x453cc0,0x453d70,0x453b50,0x475220,0x40aba0,0x4df230,0x41bce0,0x4e4391)
FIXTURE=ROOT/'cpppj/tests/fixtures/factory-research-x86.tsv'
REPORT=ROOT/'cpppj/factory-research.json'


def inputs():
    """현재조회/임시SID/슬롯/서버 권한/부모·내용물의 독립 입력만 열거한다."""
    rows=[]
    for found in (0,5,14999,15000):
        for slots in (-1,0,1,2):
            for flags in (0,16):rows.append(['Learn',found,slots,flags])
    for kind in (0,128,187,65535):
        for found in (0,5,15000):
            for slots in (0,1,2):
                for modifiers in (0,0x80000000):rows.append(['Input',kind,found,slots,modifiers])
    base=['Receive',1,0,0x4000,85,1,128,0]
    rows.append(base.copy())
    for column,values in ((1,(0,2,257)),(2,(1,2,3,8)),(3,(0,0x4001)),(4,(126,)),(5,(0,)),(6,(0,127,187,255))):
        for value in values:
            row=base.copy();row[column]=value;rows.append(row)
    for form in (0,100):
        for reason in (0,1,2):
            for flags in (0,16):
                for loading in (0,1):
                    for found in (0,5,100,15000):rows.append(['Notice',form,reason,flags,loading,found])
    return rows


class Oracle(MenuOracle):
    """실제 명령·통지 계산을 실행하고 생성/조회/송신/효과는 명시 관측 경계로 둔다."""
    def __init__(self):
        super().__init__();self.mu.mem_map(0,0x1000);self.mu.mem_write(0,bytes(0x1000));self.allowed=[];self.hits=collections.Counter();self.boundaries=collections.Counter()
        # 현재 정밀 함수표의 정확한 범위만 실행한다.
        for address in ENTRIES:
            for part in self.facts[address]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """슬롯 조회 순서와 원본 메시지의 전체16바이트를 기록한다."""
        self.recent.append(hex(address))
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50;self.boundaries[hex(address)]+=1
        if address==0x4ac9f0:
            assert args[0]==16 and args[2]==0;self.events.append('find,'+str(self.found));self.ret(self.found,12)
        elif address==0x4752b0:
            assert args[:2]==(0,0);self.events.append('slots,'+str(self.slots));self.ret(self.slots,8)
        elif address==0x4af780:
            assert args[0]==0;self.events.append('destroy,'+str(sid)+',0');self.found=0;self.ret(purge=4)
        elif address==0x4afa70:
            self.events.append('create,'+str(sid)+','+','.join(map(str,args[:5])));self.ret(20000,20)
        elif address==0x4e2dc0:
            self.events.append('send,'+str(args[0])+','+bytes(mu.mem_read(args[1],16)).hex());self.ret(1)
        elif address==0x4c63d0:
            self.events.append('help,'+self.string(args[0]).hex()+','+str(args[1]));self.ret(1)
        elif address==0x4ac200:self.ret(int.from_bytes(mu.mem_read(TYPES+mu.mem_read(ecx+10,1)[0]*500+0xec,4),'little'))
        elif address==0x45a6c0:self.events.append('flare,'+','.join(map(str,args[:3])));self.ret()
        elif address==0x44f7c0:self.events.append('refresh,'+str(args[0]));self.ret(1)
        elif address==0x4c2630:self.ret()
        elif address==0x4e0620:self.events.append('assert,'+self.string(args[0]).hex());self.ret()
        else:raise RuntimeError('연구 허용 밖 실제 명령: '+hex(address))

    def prepare_case(self,row):
        """기대 분기를 계산하지 않고 원문 전역/슬롯/수신 입력을 공급한다."""
        self.prepare([85,1,1,128,0,3,0,800,fbits(2200),'-','-','-','-',0]);self.events=[];self.slots=2;self.found=0
        self.put(TYPES+85*500+0xec,0x4000);self.put(TYPES+126*500+0xec,0x4000);self.put(0x540bc4,1);self.put(0x5c89b8,0);self.put(0x55a4a4,3);self.put(0x5e4794,0)
        self.mu.mem_write(0x55b4d0,struct.pack('<d',12.5));self.put(POOL+100*50,0x512208);self.put(POOL+100*50+18,128,2);self.put(POOL+100*50+20,16,2)
        # 삭제 경계가 참조할 client/server 슬롯의 실제 가상+10을 설치한다.
        for found in (5,14999,15000):self.put(POOL+found*50,0x512208)
        self.put(TEXT+0xc000,0x03000080);self.put(TEXT+0xc004,0);self.put(TEXT+0xc008,(128<<16)|50)
        if row[0]=='Learn':
            self.found,self.slots,flags=row[1:];return 0x453cc0,(128,flags),POOL+50*50,8
        if row[0]=='Input':
            kind,self.found,self.slots,modifiers=row[1:];self.put(TEXT+0xc000,0x03000080|modifiers);self.put(TEXT+0xc008,(kind<<16)|50);return 0x4547d0,(TEXT+0xc000,),POOL+50*50,4
        if row[0]=='Receive':
            sender,state,genus,typ,boss,kind,flags=row[1:];self.put(POOL+50*50+11,state,1);self.put(POOL+50*50+10,typ,1);self.put(TYPES+typ*500+0xec,genus);self.put(0x540bc4,boss)
            self.mu.mem_write(TEXT+0xc000,bytes.fromhex('01581000320080000000000000002940'));self.put(TEXT+0xc006,kind,1);self.put(TEXT+0xc007,flags,1);return 0x453d70,(sender,0,TEXT+0xc000),0,0
        form,reason,flags,loading,self.found=row[1:];self.put(POOL+form*50+20,flags,2);self.put(0x5c89b8,loading);return 0x453b50,(form,reason),POOL+50*50,8


def generate():
    """두 정밀도에서 원본 반환/호출 순서/변경 카운터/전체 메시지를 관측한다."""
    oracle=Oracle();rows=[]
    for case in inputs():
        outputs=[]
        for control in (0x27f,0x37f):
            address,args,ecx,purge=oracle.prepare_case(case);result,trace=oracle.invoke(address,args,ecx,purge,control);counter=int.from_bytes(oracle.mu.mem_read(0x55a4a4,4),'little')
            outputs.append((result if case[0]=='Input' else 0,trace,counter))
        assert outputs[0]==outputs[1];result,trace,counter=outputs[0];rows.append([*map(str,case),str(result),str(counter),trace])
    FIXTURE.write_text('# 원본 연구/입력/수신/내용물 통지의 명시 입력과 반환/카운터/호출 순서다.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={Path(__file__).resolve(),ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE}
    report=dict(host=socket.gethostname(),date='2026-10-18',unit=172,rows=len(rows),counts=dict(collections.Counter(r[0] for r in rows)),normalReturns=oracle.returns,controls=[0x27f,0x37f],precisionDifferences=0,instructions=oracle.instructions,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['연구의 실제 분기·요청·통지를 관측하며 게임 생성/검색/삭제/송신/플레어 생성은 경계 입력이다.','연구 외 업그레이드 진행은 이번 입력 범위에서 제외했다. 원본 게임/OS/새 디컴파일은 실행하지 않는다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('연구 원본 관측',len(rows),oracle.returns)


def verify():
    """기대 출력을 재계산하지 않고 전체 입력/원문 SHA/정상 반환 계수를 검사한다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for p,w in r['sha256'].items():assert digest(ROOT/p)==w,p
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and not r['precisionDifferences']
    for row,case in zip(rows,inputs()):assert row[:len(case)]==list(map(str,case)) and len(row)==len(case)+3
    print('연구 원본/SHA/입력 감사',len(rows))


# 명시 옵션으로만 독립 새 관측 또는 기존 근거 감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
