#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문004da880과 실제 iterator/TYPE flags를 격리 실행하고 N 입력을 정적으로 기록한다(702)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL,TYPES=0x12000000,SCRATCH+0x6000  # 실제 호스트 객체가 아닌 격리된 물리 풀/500BYTE TYPE 입력이다.

def cases():
    """독립 현재 선택/수정키/소유자/정지/장치 효과와 물리 상태를 만든다."""
    objects=[(1,0,0),(0,1,0x10000),(0,1,0x10000),(0,2,0x30000),(0,1,0),(1,1,0x10000),(2,1,0x20000),(0,1,0x10000)]
    rows=[]  # 앞6DWORD는 selected/add/local/paused/selectEffect/centerEffect이고 뒤는 각 슬롯 state/owner/flags2다.
    for selected,add,owner,paused in itertools.product(range(8),(0,1,0x20000000),(0,1,2,257),(0,1)):  # 전체 owner DWORD와 BYTE 비교, free/dead, 예약1 생략을 교차한다.
        rows.append((selected,add,owner,paused,1,0,*itertools.chain.from_iterable(objects)))
    for effect,mutation,add in itertools.product(range(5),range(6),(0,0x20000000)):  # 선택 거부/현재 선택 변경과 카메라 뒤 재조회를 보존한다.
        rows.append((2,add,1,0,effect,mutation,*itertools.chain.from_iterable(objects)))
    rng=random.Random(0x4da880)  # 기대값을 계산하지 않는 재현 가능한 독립 입력 시드다.
    for index in range(160):  # 무후보/정지/서로 다른 TYPE·상태의 순환 검색을 실제 몸체에 공급한다.
        pool=[(rng.choice((0,1,2,3,8)),rng.randrange(3),rng.choice((0,0x10000,0x20000,0x40000))) for _ in range(8)]
        if index==0: pool=[(0,1,0)]*8;pool[1]=(0,1,0x10000)
        rows.append((rng.randrange(8),rng.choice((0,0x20000000)),1,rng.randrange(2),rng.randrange(5),rng.randrange(6),*itertools.chain.from_iterable(pool)))
    return list(dict.fromkeys(rows))

class Oracle(BaseOracle):
    """검색 부모와 iterator/flags만 실제 실행하며 선택/카메라는 기존 독립 검증과 구별한 명시 경계다."""
    def __init__(self):
        """보호 PE를 읽기 전용 적재하고 함수와 데이터 경계를 분리한다."""
        super().__init__('originals',[dict(name='next',originals='004da880')],helpers=[0x40eb40,0x4ac200]);self.mu.mem_map(POOL,0x1000);self.row=[];self.trace=[];self.selects=0

    def put(self,address,value,width=4):
        """원문 DWORD/BYTE의 비트 폭으로 명시 입력을 저장한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def event(self,name,*args):
        """실제 경계 호출 순서와 DWORD 인자를 기록한다."""
        self.trace.append(':'.join(map(str,(name,*args))))

    def on_instruction(self,mu,address,size,data):
        """원문 TYPE getter의 주소와 큰 선택/카메라/정지 경계만 공급한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c=struct.unpack('<4I',mu.mem_read(sp,16))
        if address==0x49a840: mu.reg_write(UC_X86_REG_EAX,TYPES+a*500)
        elif address==0x4d5c90:
            self.event('S',a,b,c);self.selects+=1
            if self.row[4]: self.put(0x5caea0,(a,0,5,4)[self.row[4]-1])
        elif address==0x433ba0:
            self.event('C',a)
            if self.row[5]==1: self.put(0x5caea0,0)
            if self.row[5]==2: self.put(POOL+a*50+11,1,1)
            if self.row[5]==3: self.put(TYPES+(100+a)*500+0xec,0)
            if self.row[5]==4: self.put(0x5caea0,6)
            if self.row[5]==5: self.put(0x540c70,2)
        elif address==0x460de0: self.event('P',self.row[3]);mu.reg_write(UC_X86_REG_EAX,self.row[3])
        elif address==0x4da6b0: self.event('T',a,b)
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """매 입력의 풀/타입/전역을 독립 설치하고 실제 함수 반환 뒤 상태를 읽는다."""
        self.row,self.trace,self.selects=row,[],0;self.put(0x5c8464,POOL);self.put(0x5c847c,8);self.put(0x5caea0,row[0]);self.put(0x540c70,row[2])
        for sid in range(8):  # 전용 실제 유닛을 생산하지 않고 독립 타입100..107을 물리 입력으로 쓴다.
            raw=bytearray(50);raw[10]=100+sid;raw[11],raw[34]=row[6+sid*3:8+sid*3];self.mu.mem_write(POOL+sid*50,bytes(raw));self.put(TYPES+(100+sid)*500+0xec,row[8+sid*3])
        self.call('next',[row[1]])
        current=[struct.unpack('<I',self.mu.mem_read(address,4))[0] for address in (0x5caea0,0x540c70)]
        for sid in range(8):  # 경계가 실제로 변경한 BYTE/TYPE DWORD도 반환 뒤 읽는다.
            current.extend((self.mu.mem_read(POOL+sid*50+11,1)[0],struct.unpack('<I',self.mu.mem_read(TYPES+(100+sid)*500+0xec,4))[0]))
        return ','.join(map(str,row)),';'.join(self.trace) or '-',','.join(map(str,current))

def main():
    """기존 fixture를 덮지 않고 새 원시 기대값/정적 N 입력/해시를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# input30 trace selected_local_stateFlags16']
    for row in cases(): rows.append('\t'.join(oracle.run(row)))  # 기대 분기는 원문 CPU가 결정한다.
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    instructions=[dict(address=f'{i.address:08x}',instruction=i.mnemonic+' '+i.op_str) for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(bytes(oracle.mu.mem_read(0x4d9a2f,0x14)),0x4d9a2f)]  # N 분기의 실제 Shift 인자와 호출점이다.
    assert instructions[0]['instruction']=='and ecx, 0x20000000' and instructions[2]['instruction']=='call 0x4da880'
    record=dict(unit=702,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004da880','0040eb40','004ac200'],explicitBoundaries=['select004d5c90','center00433ba0','paused00460de0','multiple004da6b0','TYPE address0049a840'],inputInstructions=instructions,gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
