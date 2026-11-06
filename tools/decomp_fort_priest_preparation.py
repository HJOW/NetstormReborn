#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본004923e0/00492120/004921a0/0040ea00을 게임·OS 없이 관측한다."""
import csv
import json
import hashlib
import struct
import argparse
from pathlib import Path
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, IMAGE, HEAP, TABLE, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/fort-priest-preparation-x86.tsv'  # 최초 원문 관측이며 기존 fixture를 수정하지 않는다.
REPORT=ROOT/'cpppj/fort-priest-preparation-native-20261106.json'  # 실제 PE/도구/fixture의 해시만 기록한다.
CASES=('mission','editor','state17','no-record','marked','fallback','duplicates','populate','missing-island','two-regions','shorten-list','post-change','full-capacity')  # 명시 입력이며 기대 분기는 CPU가 결정한다.

class Oracle(Base):
    """검토한 네 함수만 실행하고 큰 게임 하위는 원문 호출 규약의 외부 장치로 기록한다."""
    def __init__(self):
        """같은 호스트의 정밀 함수표로 불연속 실제 코드 범위를 제한한다."""
        super().__init__();self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x4923e0,0x492120,0x4921a0,0x40ea00,0x470a90):  # 부모/두 목록 helper/안정 제거/테마 TYPE는 실제 명령이다.
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
    def event(self,value):
        """이미 도달한 실제 외부 호출 순서만 누적한다."""
        self.events.append(value)
    def instruction(self,mu,address,size,data):
        """부모의 분기·목록 쓰기는 CPU에 남기고 파일·큰 게임 하위만 명시 입력으로 반환한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,8)
        if address==0x48fc60:self.event('tech');self.ret(4);return
        if address==0x4acac0:
            self.event('find:'+str(args[1]));self.finds+=1
            if self.label=='post-change' and self.finds==1:self.put(0x594fa4,1)
            self.ret(args[1] if self.finds%2==0 else 0,purge=12);return
        if address==0x4afa70:
            if ecx!=IMAGE+4*50 or args[:5]!=(133 if self.finds==1 else 128,0,4,0x10,0):raise RuntimeError('실제 contained 인자 변경 '+str(args[:6]))
            self.event('create:'+str(args[0]));self.ret(8,purge=20);return
        if address==0x4bd550:self.ret(TEXT+0x1000);return
        if address==0x40d830:self.ret(purge=16);return
        if address==0x4bd660:mu.mem_write(args[0],self.records);self.event('territory');self.ret(purge=4);return
        if address==0x46f380:
            self.event('island:'+str(args[1]));self.ret(127 if self.label=='missing-island' else args[1]+1,purge=8);return
        if address==0x4e4391:
            if args[0]!=80:raise RuntimeError('실제 지역 목록 new 폭 변경')
            self.ret(HEAP+0x1000);return
        if address==0x46f530:
            x=struct.unpack('<f',struct.pack('<I',args[0]))[0]
            if self.label=='shorten-list':self.put(0x5954cc,1)
            self.ret(2 if self.label=='two-regions' and x==3 else 1,purge=8);return
        if address==STOP+0x100:
            sid=(ecx-IMAGE)//50;self.event('owner:'+str(sid)+':'+str(args[0]));self.put(ecx+34,args[0],1);self.ret(purge=4);return
        if address==0x432210:self.ret(TEXT+0x2000,purge=8);return
        if address==0x40d900:self.ret(TEXT+0x2000,purge=4);return
        if address==0x40e2b0:self.event('populate:'+str(args[4]));self.ret();return
        if address==0x4de9a0:self.ret(TEXT+0x3000,purge=4);return
        if address==0x4a89c0:
            suffix=self.string(args[3]);mu.mem_write(args[0],str(args[2]).encode()+b' Priest'+suffix+b'\0');self.ret(args[0]);return
        if address==0x4414d0:
            if self.string(args[0])!=b'global.priests':raise RuntimeError('사제 config 키 변경')
            self.event('tell:'+self.string(args[1]).decode().split(' ')[0]);self.ret();return
        if address==0x4e4354:self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미검토 코드 '+hex(address))
    def sample(self,label):
        """원문 메모리의 명시 모드/목록/State/Territory 입력만 설치한다."""
        self.label=label;self.events=[];self.finds=0
        for address,value in ((0x594fa4,int(label=='mission')),(0x5c85a4,int(label=='editor')),(0x540c74,5),(0x540cac,1),(0x540c70,7),(0x54126c,133),(0x541174,71),(0x541258,128),(0x541184,75),(0x541188,76),(0x54118c,77),(0x5c8464,IMAGE),(0x5954c4,HEAP)):
            self.put(address,value)
        self.put(0x5c89f0+20,1);self.mu.mem_write(TEXT+0x1000,b'\0'*28);self.put(TEXT+0x1011,int(label=='state17'),1)
        self.mu.mem_write(TEXT+0x3000,b'%d Priest%s\0');self.mu.mem_write(TEXT+0x2000,b'\0'*16)
        records=bytearray(120)
        if label!='no-record':records[1]=1
        if label=='two-regions':records[7]=1
        self.records=bytes(records)
        values=[1,2,3] if label not in ('populate','duplicates','full-capacity') else [] if label=='populate' else [1,1,2] if label=='duplicates' else list(range(1,23))
        self.put(0x5954cc,len(values));self.mu.mem_write(HEAP,struct.pack('<'+'I'*len(values),*values) if values else b'\0'*4)
        for sid in range(1,23):  # 사제 raw+12와 좌표만 입력하고 가상 owner는 외부 장치로 관측한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,TABLE);raw[12]=0x80 if (label in ('marked','duplicates') and sid==(2 if label=='marked' else 1)) else 0;struct.pack_into('<ff',raw,14,float(sid),1.0);self.mu.mem_write(IMAGE+sid*50,bytes(raw))
        self.put(TABLE+0x74,STOP+0x100);self.put(STACK,STOP);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,0)
        self.mu.emu_start(0x4923e0,STOP,count=100000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('부모 실제 ret/스택 불일치')
        return label+'\t'+';'.join(self.events)+'\t'+str(self.words(0x594fa4)[0])+'\n'

def digest(path):
    """현재 파일 바이트만 해시한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    """최초 관측은 덮어쓰지 않고 verify는 같은 실제 CPU 관측을 다시 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):raise RuntimeError('최초 fixture/report 덮어쓰기 금지')
    oracle=Oracle();content='# label / actual trace / final missionPost\n'+''.join(oracle.sample(label) for label in CASES)
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,expected in report['sha256'].items():
            if digest(ROOT/path)!=expected:raise RuntimeError('해시 변경 '+path)
        if FIXTURE.read_text(encoding='utf-8')!=content:raise RuntimeError('실제 PE 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n');paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_fort_priest_preparation.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
        REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastPreciseDecompile='HJOW-Athlon/2026-11-03',observations=len(CASES),gameExecuted=False,sha256={p:digest(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(verified=args.verify,observations=len(CASES),differences=0)))

if __name__=='__main__':main()
