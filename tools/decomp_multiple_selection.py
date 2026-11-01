#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다중 선택 토글/그룹 이동과 고정 목록 helper를 실제 x86에서 실행한다(701). 게임/창은 열지 않는다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL, ARRAY, VTABLE, REFRESH = 0x12000000, SCRATCH+0x4000, SCRATCH+0x8000, STOP+0x100  # 격리된 실제 슬롯/목록/가상 표/표시 경계다.
GLOBALS = (0x5caea0,0x5cae90,0x5434e4,0x5caf34)  # 원문 현재 selected/mode/enabled/signed count다.
HELPERS = (0x414450,0x40ea00,0x4d5490,0x4b22d0,0x4b1b80,0x4b2340,0x4b1bf0,0x41c0b0)  # 실제 목록/다중 정리/ProcessFinder/기본 선택 비트를 대체하지 않는다.

def cases():
    """원문 출력과 독립적인13DWORD 입력을 만들며 기대 분기를 계산하지 않는다."""
    base=[0,50,0xffffffff,0,0,51,1,1,0,0,0,0x41280000,0xc0500000]
    rows=[]  # op,sid,on,state,extra,primary,count,enabled,blocked,variant,mutation,xBits,yBits다.
    for sid,state,on,extra in itertools.product((0,50,24000,0xffffffff),(0,1,2,3),(0,1,0xffffffff,2),(0,4)):  # 물리 범위/생존/정규·비정규 선택값과 토글을 실제 몸체에 공급한다.
        row=base.copy();row[1:5]=sid,on,state,extra;rows.append(row)
    for extra,on,count,enabled,blocked,variant in itertools.product((0,4),(0,1,0xffffffff),(0,1,3,30),(0,1),(0,1),(0,1,2)):  # 주선택 해제의 승격/관문과 죽은 다중 원소를 교차한다.
        row=base.copy();row[2]=on;row[4:10]=extra,50,count,enabled,blocked,variant;rows.append(row)
    for count,enabled,variant,mutation in itertools.product((0,1,3,30,0xffffffff),(0,1,9),(0,1,2),(0,5,6,7)):  # 현재 배열/개수를 이동 callback이 바꾸는 경우를 포함한다.
        row=base.copy();row[0]=1;row[6:11]=count,enabled,0,variant,mutation;rows.append(row)
    for mutation in (1,2,3,4,8):  # 통지/표시/음향/blocked 경계 뒤 현재 전역을 다시 읽는 입력이다.
        row=base.copy();row[10]=mutation
        if mutation in (2,8): row[2:9]=0,0,4,50,0,1,0
        rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))

class Oracle(BaseOracle):
    """큰 음향/표시/이동/콘솔 장치만 명시하고 두 원문 부모/목록 helper는 그대로 실행한다."""
    def __init__(self):
        """읽기 전용 보호 PE와 같은 PC의 정밀 경계를 적재한다."""
        super().__init__('originals',[dict(name='toggle',originals='004da6b0'),dict(name='move',originals='004d5500')],helpers=[address for address in HELPERS if address!=0x41c0b0])
        self.allowed.append((0x41c0b0,0x41c0c6))  #정밀 표에서 빠진 기존 기본 Mark는 실제 ret4까지22BYTE만 허용한다(decomp_squid_selection과 같은 경계).
        self.mu.mem_map(POOL,0x130000)
        self.mu.mem_write(VTABLE+0x7c,struct.pack('<I',0x41c0b0))
        self.mu.mem_write(VTABLE+0x88,struct.pack('<I',REFRESH))
        self.mu.mem_write(0x5c8464,struct.pack('<I',POOL))
        self.mu.mem_write(0x5c847c,struct.pack('<I',24000))
        self.row=[]  # 이번 명시 입력이며 기대값은 아니다.
        self.trace=[]  # 실제 선택된 경계의 호출 순서다.
        self.moves=0  # 이동 callback의 최초 효과만 구별한다.
        self.refreshes=0  # 표시 callback의 최초 효과만 구별한다.

    def put(self,address,value):
        """현재 DWORD의 모든 비트를 격리 메모리에 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def event(self,name,*values):
        """실제 호출 인자를 원시 unsignedDWORD로 기록한다."""
        self.trace.append(':'.join(map(str,(name,*values))))

    def on_instruction(self,mu,address,size,data):
        """가상/장치 경계는 명시 결과만 공급하고 허용한 원문 몸체는 실제 CPU에서 계속한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c,d=struct.unpack('<IIIII',mu.mem_read(sp,20));this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x41c0b0:
            sid=(this-POOL)//50;self.event('N',sid,a)
            if self.row[10]==1: self.put(0x5caea0,51)
            if self.row[10]==2: self.put(0x5caf34,1)
            return super().on_instruction(mu,address,size,data)
        if address in (0x414450,0x40ea00):
            self.event('A' if address==0x414450 else 'D',a)
            return super().on_instruction(mu,address,size,data)
        purge=0
        if address==REFRESH:
            self.event('R',(this-POOL)//50);self.refreshes+=1
            if self.row[10]==3 and self.refreshes==1: self.put(ARRAY+4,54);self.put(0x5caf34,3)
        elif address==0x463b50:
            self.event('B')
            if self.row[10]==8: self.put(0x5caf34,1);self.put(ARRAY,53)
            mu.reg_write(UC_X86_REG_EAX,self.row[8])
        elif address==0x4d5bb0:
            self.event('H',a);self.put(0x5cae90,a)
        elif address==0x4a9d70:
            assert self.read_string(c)==b'select.wav';self.event('S',a,b,d)
            if self.row[10]==4: self.put(0x5caea0,52)
        elif address==0x42a360:
            self.event('M',a,b,c);self.moves+=1
            if self.moves==1:
                if self.row[10]==5: self.put(0x5caf34,1)
                if self.row[10]==6: self.put(ARRAY+4,54)
                if self.row[10]==7: self.put(0x5434e4,7)
        else:
            return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """같은 실제 메모리를 입력으로 초기화하고 반환 뒤의 상태/목록을 읽는다."""
        self.row,self.trace,self.moves,self.refreshes=row,[],0,0
        for sid in range(50,55):  # 입력 다섯 객체만 갱신하고0/경계 밖 슬롯을 유효 객체로 만들지 않는다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=100;raw[40]=4 if sid!=50 else row[4];struct.pack_into('<II',raw,14,0x41280000,0xc0500000)
            if sid==50: raw[11]=row[3]
            if row[9]==1 and sid in (52,53): raw[11]=1 if sid==52 else 2
            self.mu.mem_write(POOL+sid*50,bytes(raw))
        array=[52,53,52]+[0]*27
        if row[9]==2: array[:3]=[0,24000,52]
        self.mu.mem_write(ARRAY,struct.pack('<30I',*array));self.put(0x5caf2c,ARRAY);self.put(0x5caf30,30);self.put(0x5caf34,row[6]);self.put(0x5caea0,row[5]);self.put(0x5434e4,row[7]);self.put(0x5cae90,5)
        self.call('toggle' if row[0]==0 else 'move',list(row[1:3]) if row[0]==0 else list(row[11:13]))
        out=[struct.unpack('<I',self.mu.mem_read(address,4))[0] for address in GLOBALS]
        out.extend(self.mu.mem_read(POOL+sid*50+40,1)[0] for sid in range(50,55))
        out.extend(struct.unpack('<30I',self.mu.mem_read(ARRAY,120)))
        return ','.join(map(str,row)), ';'.join(self.trace) or '-', ','.join(map(str,out))

def main():
    """기존 fixture를 덮지 않고 새 실제 관측과 해시만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists()
    oracle=Oracle();rows=['# input13 trace globals4_extra5_array30']
    for row in cases():  # 모든 입력의 기대값은 실제 원문 실행 결과만 사용한다.
        rows.append('\t'.join(oracle.run(row)))
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=701,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004da6b0','004d5500'],nativeHelpers=[f'{address:08x}' for address in HELPERS],explicitBoundaries=['refresh +88','blocked463b50','highlight4d5bb0','sound4a9d70','carrierCommit42a360'],gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
